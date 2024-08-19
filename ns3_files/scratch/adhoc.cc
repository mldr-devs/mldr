#include <chrono>
#include <filesystem>
#include <map>
#include <string>

#include "ns3/core-module.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/olsr-helper.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/traffic-control-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ns3-ai-module.h"
#include "ns3/internet-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ScenarioMassive");

/*** ns3-ai structures definitions ***/

#define DEFAULT_MEMBLOCK_KEY 2333

struct sEnv
{
  double fairness;
  double latency;
  double plr;
  double throughput;
  double time;
} Packed;

struct sAct
{
  int cw;
  bool rts_cts;
  bool ampdu;
  bool end_warmup;
} Packed;

Ns3AIRL<sEnv, sAct> * m_env = new Ns3AIRL<sEnv, sAct> (DEFAULT_MEMBLOCK_KEY);

/***** Functions declarations *****/
void ResetMonitor ();
void PopulateARPcache ();
void ExecuteAction (std::string agentName, double distance, uint32_t nWifi);
void SetNetworkConfiguration (int cw_idx, bool rts_cts, bool ampdu);
void ReceivePacket (Ptr<Socket> socket);
void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize, double pktInterval);

/***** Global variables and constants *****/

std::map<uint32_t, uint64_t> warmupFlows;

double fuzzTime = 5.;
double simulationTime = 50.;
double interactionTime = 0.5;
double warmupEndTime = 0.;
bool simulationPhase = false;
bool useMabAgent = false;

double previousRX = 0;
double previousTX = 0;
double previousLost = 0;
Time previousDelay = Seconds(0);

double warmupRX = 0;
double warmupTX = 0;
double warmupLost = 0;
Time warmupDelay = Seconds(0);
uint32_t packetCounter = 0;

Ptr<FlowMonitor> monitor;
std::map<FlowId, FlowMonitor::FlowStats> previousStats;

std::ostringstream csvLogOutput;

/***** Main with scenario definition *****/

int
main(int argc, char* argv[])
{
  // Initialize default simulation parameters
  uint32_t nWifi = 500;
  uint32_t packetSize = 256;
  uint32_t channelWidth = 20;
  uint32_t mcs = 0;
  double distance = 10.;
  double interPacketInterval = 1.0;

  std::string agentName = "wifi";
  std::string pcapName = "";
  std::string csvPath = "results.csv";
  std::string csvLogPath = "logs.csv";
  std::string flowmonPath = "flowmon.xml";
  std::string thrPath = "thr.txt";

  int cw_idx = -1;
  bool rts_cts = false;
  bool ampdu = true;

  // Parse command line arguments
  CommandLine cmd;
  cmd.AddValue ("agentName", "Name of the agent", agentName);
  cmd.AddValue ("ampdu", "Enable A-MPDU (only for wifi agent)", ampdu);
  cmd.AddValue ("channelWidth", "Channel width (MHz)", channelWidth);
  cmd.AddValue ("csvLogPath", "Path to output CSV log file", csvLogPath);
  cmd.AddValue ("csvPath", "Path to output CSV file", csvPath);
  cmd.AddValue ("cw", "Contention window (const CW = 2 ^ (4 + x) if x >= 0) (only for wifi agent)", cw_idx);
  cmd.AddValue ("distance", "Max distance between AP and STAs (m)", distance);
  cmd.AddValue ("flowmonPath", "Path to throughput log file", flowmonPath);
  cmd.AddValue ("fuzzTime", "Maximum fuzz value (s)", fuzzTime);
  cmd.AddValue ("interactionTime", "Time between agent actions (s)", interactionTime);
  cmd.AddValue ("interPacketInterval", "Inter-packet interval (s)", interPacketInterval);
  cmd.AddValue ("mcs", "MCS index", mcs);
  cmd.AddValue ("nWifi", "Number of stations", nWifi);
  cmd.AddValue ("packetSize", "Packets size (B)", packetSize);
  cmd.AddValue ("pcapName", "Name of a PCAP file generated from the AP", pcapName);
  cmd.AddValue ("rtsCts", "Enable RTS/CTS (only for wifi agent)", rts_cts);
  cmd.AddValue ("simulationTime", "Duration of simulation (s)", simulationTime);
  cmd.AddValue ("thrPath", "Path to throughput log file", thrPath);
  cmd.Parse (argc, argv);

  // Print simulation settings to screen
  std::cout << std::endl
            << "Simulating an IEEE 802.11ax devices with the following settings:" << std::endl
            << "- agent: " << agentName << std::endl
            << "- frequency band: 5 GHz" << std::endl
            << "- channel width: " << channelWidth << " Mhz" << std::endl
            << "- MCS index: " << mcs << std::endl
            << "- packets size: " << packetSize << " B" << std::endl
            << "- number of stations: " << nWifi << std::endl
            << "- area size: " << distance << " m" << std::endl
            << "- inter-packet interval: " << interPacketInterval << " s" << std::endl
            << "- simulation time: " << simulationTime << " s" << std::endl
            << "- max fuzz time: " << fuzzTime << " s" << std::endl
            << "- interaction time: " << interactionTime << " s" << std::endl;

  if (agentName == "wifi")
    {
      std::cout << "- CW: " << (cw_idx >= 0 ? "2 ^ (4 + " + std::to_string (cw_idx) + ")" : "default" ) << std::endl
                << "- RTS/CTS: " << (rts_cts ? "enabled" : "disabled") << std::endl
                << "- A-MPDU: " << (ampdu ? "enabled" : "disabled") << std::endl;
    }

  useMabAgent = agentName != "wifi";

  // Fix non-unicast data rate to be the same as that of unicast
  NodeContainer wifiNodes;
  wifiNodes.Create(nWifi);

  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;
  std::ostringstream oss;
  oss << "HeMcs" << mcs;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", StringValue (oss.str ()),
                                "ControlMode", StringValue (oss.str ())); //Set MCS

  YansWifiPhyHelper wifiPhy;

  // set it to zero; otherwise, gain will be added
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel");
  wifiPhy.SetChannel(wifiChannel.Create());

  // Add an upper mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetStandard(WIFI_STANDARD_80211ax);
  wifi.SetRemoteStationManager ("ns3::IdealWifiManager");

  // Set it to adhoc mode
  wifiMac.SetType("ns3::AdhocWifiMac");

  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelSettings",
               StringValue ("{0, " + std::to_string (channelWidth) + ", BAND_2_4GHZ, 0}"));

  NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, wifiNodes);

  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
                                  "X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=" + std::to_string (distance) + "]"),
                                  "Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=" + std::to_string (distance) + "]"));
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(wifiNodes);

  Ptr<MobilityModel> sinkMobility = wifiNodes.Get (0)->GetObject<MobilityModel> ();
  sinkMobility->SetPosition (Vector (distance/2, distance/2, 0.0));

  // Print position of each node
  std::cout << std::endl << "Node positions:" << std::endl;

  for (auto node = wifiNodes.Begin (); node != wifiNodes.End (); ++node)
    {
      Ptr<MobilityModel> position = (*node)->GetObject<MobilityModel> ();
      Vector pos = position->GetPosition ();
      std::cout << "Sta " << (*node)->GetId () << ":\tx=" << pos.x << ", y=" << pos.y << std::endl;
    }

  std::cout << std::endl;

  // Install an Internet stack
  InternetStackHelper internet;
  internet.Install(wifiNodes);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.0.0", "255.255.0.0");
  Ipv4InterfaceContainer i = ipv4.Assign(devices);
  uint32_t portNumber = 9;

  for (uint32_t j = 1; j < wifiNodes.GetN (); ++j)
  {
    portNumber++;

    // Get sink address
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    Ptr<Socket> recvSink = Socket::CreateSocket(wifiNodes.Get (0), tid);
    InetSocketAddress local = InetSocketAddress(i.GetAddress(0,0), portNumber);
    recvSink->Bind(local);
    recvSink->SetRecvCallback(MakeCallback(&ReceivePacket));

    Ptr<Socket> source = Socket::CreateSocket(wifiNodes.Get (j), tid);
    InetSocketAddress remote = InetSocketAddress(i.GetAddress(0,0), portNumber);
    source->Connect(remote);

    //Add random fuzz to app start time
    Ptr<UniformRandomVariable> fuzz = CreateObject<UniformRandomVariable> ();
    fuzz->SetAttribute ("Min", DoubleValue (0.0));
    fuzz->SetAttribute ("Max", DoubleValue (fuzzTime));

    Simulator::Schedule (Seconds (fuzz->GetValue ()),
                         &GenerateTraffic,
                         source,
                         packetSize,
                         interPacketInterval);
  }
  
  PopulateARPcache ();

  // Install FlowMonitor
  FlowMonitorHelper flowmon;
  monitor = flowmon.InstallAll ();
  csvLogOutput << "agent,distance,nWifi,nWifiReal,seed,warmupEnd,fairness,latency,plr,throughput,time" << std::endl;

   // Generate PCAP at the sink
   if (!pcapName.empty ())
     {
       wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
       wifiPhy.EnablePcap (pcapName, devices.Get (0));
     }

  // Schedule interaction with the agent
  if (agentName == "wifi")
    {
      SetNetworkConfiguration (cw_idx, rts_cts, ampdu);
    }

  m_env->SetCond (2, 0);
  Simulator::Schedule (Seconds (fuzzTime), &ResetMonitor);
  Simulator::Schedule (Seconds (fuzzTime), &ExecuteAction, agentName, distance, nWifi);

  // Record start time
  std::cout << "Starting simulation..." << std::endl;
  auto start = std::chrono::high_resolution_clock::now ();

  // Run the simulation!
  Simulator::Run();

  // Record stop time and count duration
  auto finish = std::chrono::high_resolution_clock::now ();
  std::chrono::duration<double> elapsed = finish - start;

  std::cout << "Done!" << std::endl
            << "Elapsed time: " << elapsed.count () << " s" << std::endl
            << std::endl;

  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  std::cout << "Results: " << std::endl;

  double lostPackets = 0;
  double txPackets = 0;
  double latencySum = 0;
  double jainsIndexN = 0;
  double jainsIndexD = 0;
  double nWifiReal = 0;

  std::ostringstream thrLogs;

  for (auto &stat : stats)
    {
      double flow = (8 * stat.second.rxBytes) / (1e6 * simulationTime);

      if (flow > 0)
        {
          nWifiReal += 1;
        }

      jainsIndexN += flow;
      jainsIndexD += flow * flow;

      lostPackets += stat.second.lostPackets;
      txPackets += stat.second.txPackets;
      latencySum += stat.second.delaySum.GetSeconds ();

      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (stat.first);
      std::cout << "Flow " << stat.first << " (" << t.sourceAddress << " -> "
                << t.destinationAddress << ")\tThroughput: " << flow << " Mb/s" << std::endl;

      thrLogs << flow << std::endl;
    }

  double totalThr = jainsIndexN;
  double fairnessIndex = jainsIndexN * jainsIndexN / (nWifiReal * jainsIndexD);
  double totalPLR = lostPackets / txPackets;
  double totalLatency = latencySum;
  double latencyPerPacketTotal = totalLatency / txPackets;

  // Print results
  std::cout << std::endl
            << "Network throughput: " << totalThr << " Mb/s" << std::endl
            << "Jain's fairness index: " << fairnessIndex << std::endl
            << "PLR: " << totalPLR << std::endl
            << "Total Latency: " << totalLatency << std::endl
            << "Latency per packet: " << latencyPerPacketTotal << std::endl
            << std::endl;

  // Gather results in CSV format
  std::ostringstream csvOutput;
  csvOutput << agentName << "," << distance << "," << nWifi << "," << nWifiReal << ","
            << RngSeedManager::GetRun () << "," << warmupEndTime << "," << fairnessIndex << ","
            << latencyPerPacketTotal << "," << totalPLR << "," << totalThr << std::endl;

  // Print results to std output
  std::cout << "agent,distance,nWifi,nWifiReal,seed,warmupEnd,fairness,latency,plr,throughput"
            << std::endl
            << csvOutput.str ();

  // Print results to files
  std::ofstream outputFile (csvPath);
  outputFile << csvOutput.str ();
  std::cout << std::endl << "Simulation data saved to: " << csvPath;

  std::ofstream outputLogFile (csvLogPath);
  outputLogFile << csvLogOutput.str ();
  std::cout << std::endl << "Simulation log saved to: " << csvLogPath << std::endl;

  std::ofstream thrFile (thrPath);
  thrFile << thrLogs.str ();
  std::cout << "Throughput log saved to " << thrPath << std::endl;

  monitor->SerializeToXmlFile (flowmonPath, true, true);
  std::cout << "Flow monitor data saved to: " << flowmonPath << std::endl << std::endl;

  // Cleanup
  Simulator::Destroy ();
  m_env->SetFinish ();

  return 0;
}

/***** Function definitions *****/
void
ResetMonitor ()
{
  monitor->CheckForLostPackets ();
  monitor->ResetAllStats ();
  previousStats = monitor->GetFlowStats ();
  previousRX = 0;
  previousTX = 0;
  previousLost = 0;
  previousDelay = Seconds(0);
}

void
PopulateARPcache ()
{
  Ptr<ArpCache> arp = CreateObject<ArpCache> ();
  arp->SetAliveTimeout (Seconds (3600 * 24 * 365));

  for (auto i = NodeList::Begin (); i != NodeList::End (); ++i)
    {
      Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
      ObjectVectorValue interfaces;
      ip->GetAttribute ("InterfaceList", interfaces);

      for (auto j = interfaces.Begin (); j != interfaces.End (); j++)
        {
          Ptr<Ipv4Interface> ipIface = (*j).second->GetObject<Ipv4Interface> ();
          Ptr<NetDevice> device = ipIface->GetDevice ();
          Mac48Address addr = Mac48Address::ConvertFrom (device->GetAddress ());

          for (uint32_t k = 0; k < ipIface->GetNAddresses (); k++)
            {
              Ipv4Address ipAddr = ipIface->GetAddress (k).GetLocal ();
              if (ipAddr == Ipv4Address::GetLoopback ())
                {
                  continue;
                }

              ArpCache::Entry *entry = arp->Add (ipAddr);
              Ipv4Header ipv4Hdr;
              ipv4Hdr.SetDestination (ipAddr);

              Ptr<Packet> p = Create<Packet> (100);
              entry->MarkWaitReply (ArpCache::Ipv4PayloadHeaderPair (p, ipv4Hdr));
              entry->MarkAlive (addr);
            }
        }
    }

  for (auto i = NodeList::Begin (); i != NodeList::End (); ++i)
    {
      Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
      ObjectVectorValue interfaces;
      ip->GetAttribute ("InterfaceList", interfaces);

      for (auto j = interfaces.Begin (); j != interfaces.End (); j++)
        {
          Ptr<Ipv4Interface> ipIface = (*j).second->GetObject<Ipv4Interface> ();
          ipIface->SetAttribute ("ArpCache", PointerValue (arp));
        }
    }
}

void
ExecuteAction (std::string agentName, double distance, uint32_t nWifi)
{
  double nWifiReal = 0;
  double jainsIndexNTemp = 0.;
  double jainsIndexDTemp = 0.;

  double currentRX = 0;
  double currentTX = 0;
  double currentLost = 0;
  Time currentDelay = Seconds (0);

  monitor->CheckForLostPackets ();
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

  for (auto &stat : stats)
    {
      double flow = 8 * ( stat.second.rxBytes - previousStats[stat.first].rxBytes) / (1e6 * interactionTime);
      currentLost += stat.second.lostPackets;
      currentRX += stat.second.rxPackets;
      currentTX += stat.second.txPackets;
      currentDelay += stat.second.delaySum;
      if (flow > 0)
        {
          nWifiReal += 1;
        }

      jainsIndexNTemp += flow;
      jainsIndexDTemp += flow * flow;
    }

  Time latency = currentDelay - previousDelay;
  double lostPackets =  currentLost - previousLost;
  double rxPackets =  currentRX - previousRX;
  double txPackets =  currentTX - previousTX;
  double latencyPerPacket = latency.GetSeconds() / rxPackets;
  double PLR = lostPackets/txPackets;
  double fairnessIndex = jainsIndexNTemp * jainsIndexNTemp / (nWifiReal * jainsIndexDTemp);
  double throughput = jainsIndexNTemp;

  previousDelay = currentDelay;
  previousLost = currentLost;
  previousRX = currentRX;
  previousTX = currentTX;
  previousStats = stats;

  bool end_warmup = false;

  if (useMabAgent && Simulator::Now ().GetSeconds () >= fuzzTime)
    {
      auto env = m_env->EnvSetterCond ();
      env->fairness = fairnessIndex;
      env->latency = latencyPerPacket;
      env->plr = PLR;
      env->throughput = throughput;
      env->time = Simulator::Now ().GetSeconds () - fuzzTime;
      m_env->SetCompleted ();

      auto act = m_env->ActionGetterCond ();
      int cw_idx = act->cw;
      bool rts_cts = act->rts_cts;
      bool ampdu = act->ampdu;
      end_warmup = act->end_warmup;
      m_env->GetCompleted ();

      SetNetworkConfiguration (cw_idx, rts_cts, ampdu);
    }
  else if (!useMabAgent && Simulator::Now ().GetSeconds () >= fuzzTime)
    {
      end_warmup = true;
    }

  // End warmup period, define simulation stop time, and reset stats
  if (end_warmup && !simulationPhase)
    {
      Simulator::ScheduleNow (&ResetMonitor);
      Simulator::Stop (Seconds (simulationTime));
      simulationPhase = true;
      warmupEndTime = Simulator::Now ().GetSeconds () - fuzzTime;
      std::cout << "Warmup period finished after " << warmupEndTime << " s" << std::endl;
    }

  csvLogOutput << agentName << "," << distance << "," << nWifi << "," << nWifiReal << "," << RngSeedManager::GetRun () << "," << end_warmup << ","
  << fairnessIndex << "," << latencyPerPacket << "," << PLR << "," << throughput << "," << Simulator::Now().GetSeconds() - fuzzTime << std::endl;

  Simulator::Schedule (Seconds(interactionTime), &ExecuteAction, agentName, distance, nWifi);
}

void
SetNetworkConfiguration (int cw_idx, bool rts_cts, bool ampdu)
{
  if (cw_idx >= 0)
    {
      // Set CW
      std::vector<UintegerValue> cwValues = {UintegerValue (pow (2, 4 + cw_idx))};
      AttributeContainerValue<UintegerValue> cwValue (cwValues);
      Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/BE_Txop/MinCws", cwValue);
      Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/BE_Txop/MaxCws", cwValue);
    }

  // Enable or disable RTS/CTS
  uint64_t ctsThrLow = 0;
  uint64_t ctsThrHigh = 4692480;
  UintegerValue ctsThr = (rts_cts ? UintegerValue (ctsThrLow) : UintegerValue (ctsThrHigh));
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/RemoteStationManager/$ns3::IdealWifiManager/RtsCtsThreshold", ctsThr);

  // Enable or disable A-MPDU
  uint64_t ampduSizeLow = 0;
  uint64_t ampduSizeHigh = 6500631;
  UintegerValue ampduSize = (ampdu ? UintegerValue (ampduSizeHigh) : UintegerValue (ampduSizeLow));
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/BE_MaxAmpduSize", ampduSize);
}

void
ReceivePacket(Ptr<Socket> socket)
{
  while (socket->Recv())
    {
      packetCounter++;
    }
}

void
GenerateTraffic(Ptr<Socket> socket, uint32_t pktSize, double pktInterval)
{
  socket->Send (Create<Packet> (pktSize));
  Simulator::Schedule (Seconds (pktInterval),
                       &GenerateTraffic,
                       socket,
                       pktSize,
                       pktInterval);
}