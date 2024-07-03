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

void GetFuzzFlows ();
void GetWarmupFlows ();
void PopulateARPcache ();
void ExecuteAction ();

/***** Global variables and constants *****/

std::map<uint32_t, uint64_t> warmupFlows;

double fuzzTime = 5.;
double simulationTime = 50.;
double interactionTime = 0.5;
bool simulationPhase = false;

double previousRX = 0;
double previousTX = 0;
double previousLost = 0;
Time previousDelay = Seconds(0);

double warmupRX = 0;
double warmupTX = 0;
double warmupLost = 0;
Time warmupDelay = Seconds(0);
double counter = 0;

Ptr<FlowMonitor> monitor;
std::map<FlowId, FlowMonitor::FlowStats> previousStats;

std::ostringstream csvLogOutput;


uint32_t packetCounter{0};
/**
 * Function called when a packet is received.
 *
 * \param socket The receiving socket.
 */
void
ReceivePacket(Ptr<Socket> socket)
{
    while (socket->Recv())
    {
      packetCounter++;
    }
}

/**
 * Generate traffic.
 *
 * \param socket The sending socket.
 * \param pktSize The packet size.
 * \param pktCount The packet count.
 * \param pktInterval The interval between two packets.
 */
static void
GenerateTraffic(Ptr<Socket> socket, uint32_t pktSize, Time pktInterval)
{
  // std::cout << "pakiet jedzie" << std::endl;
  socket->Send(Create<Packet>(pktSize));
  Simulator::Schedule(pktInterval,
                      &GenerateTraffic,
                      socket,
                      pktSize,
                      pktInterval);
}

int
main(int argc, char* argv[])
{
  // Initialize default simulation parameters

  uint32_t nWifi = 500;
  uint32_t maxQueueSize = 100;
  uint32_t packetSize = 256;
  uint32_t dataRate = 110;
  uint32_t channelWidth = 20;
  double distance = 10.;
  uint32_t mcs{0};
  uint32_t portNumber{9};
  Time interPacketInterval{"1s"};

  std::string agentName = "wifi";
  std::string pcapName = "";
  std::string csvPath = "results.csv";
  std::string csvLogPath = "simualationsLogs.csv";

  // Parse command line arguments
  CommandLine cmd;
  cmd.AddValue ("agentName", "Name of the agent", agentName);
  cmd.AddValue ("channelWidth", "Channel width (MHz)", channelWidth);
  cmd.AddValue ("csvPath", "Path to output CSV file", csvPath);
  cmd.AddValue ("dataRate", "Traffic generator data rate (Mb/s)", dataRate);
  cmd.AddValue ("distance", "Max distance between AP and STAs (m)", distance);
  cmd.AddValue ("fuzzTime", "Maximum fuzz value (s)", fuzzTime);
  cmd.AddValue ("interactionTime", "Time between agent actions (s)", interactionTime);
  cmd.AddValue ("nWifi", "Number of stations", nWifi);
  cmd.AddValue ("packetSize", "Packets size (B)", packetSize);
  cmd.AddValue ("pcapName", "Name of a PCAP file generated from the AP", pcapName);
  cmd.AddValue ("simulationTime", "Duration of simulation (s)", simulationTime);
  cmd.AddValue ("maxQueueSize", "Max queue size (packets)", maxQueueSize);
  cmd.Parse (argc, argv);

  // Print simulation settings to screen
  std::cout << std::endl
            << "Simulating an IEEE 802.11ax devices with the following settings:" << std::endl
            << "- frequency band: 5 GHz" << std::endl
            << "- max data rate: " << dataRate << " Mb/s" << std::endl
            << "- channel width: " << channelWidth << " Mhz" << std::endl
            << "- packets size: " << packetSize << " B" << std::endl
            << "- number of stations: " << nWifi << std::endl
            << "- max distance between AP and STAs: " << distance << " m" << std::endl
            << "- simulation time: " << simulationTime << " s" << std::endl
            << "- max fuzz time: " << fuzzTime << " s" << std::endl
            << "- interaction time: " << interactionTime << " s" << std::endl << std::endl
            << "- max queue size: " << maxQueueSize << " packets" << std::endl << std::endl;


  // Fix non-unicast data rate to be the same as that of unicast

  NodeContainer c;
  c.Create(nWifi);

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

  NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, c);

  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
                                  "X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=" + std::to_string (distance) + "]"),
                                  "Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=" + std::to_string (distance) + "]"));
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(c);

  Ptr<MobilityModel> sinkMobility = c.Get (0)->GetObject<MobilityModel> ();
  sinkMobility->SetPosition (Vector (distance/2, distance/2, 0.0));

  // Print position of each node
  std::cout << "Node positions:" << std::endl;

  for (auto node = c.Begin (); node != c.End (); ++node)
    {
      Ptr<MobilityModel> position = (*node)->GetObject<MobilityModel> ();
      Vector pos = position->GetPosition ();
      std::cout << "Sta " << (*node)->GetId () << ":\tx=" << pos.x << ", y=" << pos.y << std::endl;
    }

  InternetStackHelper internet;
  internet.Install(c);

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO("Assign IP Addresses.");
  ipv4.SetBase("10.1.0.0", "255.255.0.0");
  Ipv4InterfaceContainer i = ipv4.Assign(devices);

  for (uint32_t j = 1; j < c.GetN (); ++j)
  {
    // Get sink address
    portNumber++;

    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    Ptr<Socket> recvSink = Socket::CreateSocket(c.Get (0), tid);
    InetSocketAddress local = InetSocketAddress(i.GetAddress(0,0), portNumber);
    recvSink->Bind(local);
    recvSink->SetRecvCallback(MakeCallback(&ReceivePacket));

    Ptr<Socket> source = Socket::CreateSocket(c.Get (j), tid);
    InetSocketAddress remote = InetSocketAddress(i.GetAddress(0,0), portNumber);
    source->Connect(remote);

    //Add random fuzz to app start time
    double min = 0.0;
    double max = 0.2;
    Ptr<UniformRandomVariable> fuzz = CreateObject<UniformRandomVariable> ();
    fuzz->SetAttribute ("Min", DoubleValue (min));
    fuzz->SetAttribute ("Max", DoubleValue (max));
    
    double startTime = interPacketInterval.GetSeconds() + fuzz->GetValue ();
    Simulator::Schedule(Seconds(startTime),
                      &GenerateTraffic,
                      source,
                      packetSize,
                      interPacketInterval);
  }
  
  PopulateARPcache ();

  // Install FlowMonitor
  Ptr<FlowMonitor> monitor;
  FlowMonitorHelper flowmon;
  monitor = flowmon.InstallAll ();

  // // Generate PCAP at AP
  // if (!pcapName.empty ())
  //   {
  //     wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
  //     wifiPhy.EnablePcap (pcapName, devices(0));
  //   }

  // Schedule interaction with the agent
  if (agentName != "wifi")
    {
      m_env->SetCond (2, 0);
      Simulator::Schedule (Seconds (fuzzTime + 1.0), &GetFuzzFlows);
      Simulator::Schedule (Seconds (fuzzTime + 1.0 + interactionTime), &ExecuteAction);
    }
  else
    {
      Simulator::Schedule (Seconds (fuzzTime + 1.0), &GetWarmupFlows);
      Simulator::Stop (Seconds (fuzzTime + 1.0 + simulationTime));
    }

  Simulator::Run();

  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  std::cout << "Results: " << std::endl;

  double lostPackets = 0;
  double rxPackets = 0;
  double txPackets = 0;
  double Latency = 0;
  double jainsIndexN = 0;
  double jainsIndexD = 0;
  double nWifiReal = 0;
  double simulationTime = 50;

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
      rxPackets += stat.second.rxPackets;
      txPackets += stat.second.txPackets;
      Latency += stat.second.delaySum.GetSeconds ();

      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (stat.first);
      std::cout << "Flow " << stat.first << " (" << t.sourceAddress << " -> "
                << t.destinationAddress << ")\tThroughput: " << flow << " Mb/s" << std::endl;
    }

  double totalThr = jainsIndexN;
  double fairnessIndex = jainsIndexN * jainsIndexN / (nWifiReal * jainsIndexD);
  double totalPLR = lostPackets / txPackets;
  double totalLatency = Latency;
  double latencyPerPacketTotal = Latency / rxPackets;

  // Print results
  std::cout << std::endl
            << "Network throughput: " << totalThr << " Mb/s" << std::endl
            << "Jain's fairness index: " << fairnessIndex << std::endl
            << "PLR: " << totalPLR << std::endl
            << "Total Latency: " << totalLatency << std::endl
            << "Latency per packet: " << latencyPerPacketTotal << std::endl
            << "Received packets: " << packetCounter << std::endl
            << std::endl;

  Simulator::Destroy ();
  m_env->SetFinish ();
  return 0;
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
ExecuteAction ()
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

  csvLogOutput << lostPackets << "," << rxPackets << "," << txPackets << "," << PLR << "," << fairnessIndex << "," << throughput << "," << latencyPerPacket << std::endl;

  previousDelay = currentDelay;
  previousLost = currentLost;
  previousRX = currentRX;
  previousTX = currentTX;
  previousStats = stats;


  auto env = m_env->EnvSetterCond ();
  env->fairness = fairnessIndex;
  env->latency = latencyPerPacket;
  env->plr = PLR;
  env->throughput = throughput;
  env->time = Simulator::Now ().GetSeconds () - (fuzzTime + 1.0);
  m_env->SetCompleted ();

  auto act = m_env->ActionGetterCond ();
  uint8_t cw_idx = act->cw;
  bool rts_cts = act->rts_cts;
  bool ampdu = act->ampdu;
  bool end_warmup = act->end_warmup;
  m_env->GetCompleted ();
  // End warmup period, define simulation stop time, and reset stats
  if (end_warmup && !simulationPhase)
    {
      Simulator::Stop (Seconds (simulationTime));
      Simulator::ScheduleNow (&GetWarmupFlows);
      simulationPhase = true;
    }

  // Set CW
  AttributeContainerValue<UintegerValue> cwValue (std::vector {UintegerValue (pow (2, 4 + cw_idx))});
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/BE_Txop/MinCws", cwValue);
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/BE_Txop/MaxCws", cwValue);

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

  Simulator::Schedule (Seconds(interactionTime), &ExecuteAction);
  counter++;
}

void
GetWarmupFlows ()
{
  previousStats = monitor->GetFlowStats ();
  for (auto &stat : monitor->GetFlowStats ())
    {
      warmupFlows.insert (std::pair<uint32_t, double> (stat.first, 8 * stat.second.rxBytes));
      warmupLost += stat.second.lostPackets;
      warmupRX += stat.second.rxPackets;
      warmupTX += stat.second.txPackets;
      warmupDelay += stat.second.delaySum;
    }
}

void
GetFuzzFlows ()
{
  previousStats = monitor->GetFlowStats ();

  for (auto &stat : monitor->GetFlowStats ())
    {
      previousLost += stat.second.lostPackets;
      previousRX += stat.second.rxPackets;
      previousTX += stat.second.txPackets;
      previousDelay += stat.second.delaySum;
    }
}