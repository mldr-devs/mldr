#include <chrono>
#include <filesystem>
#include <map>
#include <string>

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/node-container.h"
#include "ns3/node-list.h"
#include "ns3/ssid.h"
#include "ns3/wifi-net-device.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ns3-ai-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("scenario");

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
  uint8_t cw;
  bool rts_cts;
  bool ampdu;
  bool end_warmup;
} Packed;

Ns3AIRL<sEnv, sAct> * m_env = new Ns3AIRL<sEnv, sAct> (DEFAULT_MEMBLOCK_KEY);

/***** Functions declarations *****/

void GetFuzzFlows ();
void GetWarmupFlows ();
void InstallTrafficGenerator (Ptr<ns3::Node> fromNode, Ptr<ns3::Node> toNode, uint32_t port,
                              DataRate offeredLoad, uint32_t packetSize);
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

Ptr<FlowMonitor> monitor;
std::map<FlowId, FlowMonitor::FlowStats> previousStats;

std::ostringstream csvLogOutput;

/***** Main with scenario definition *****/

int
main (int argc, char *argv[])
{
  // Initialize default simulation parameters
  uint32_t nWifi = 1;
  uint32_t packetSize = 1500;
  uint32_t dataRate = 110;
  uint32_t channelWidth = 20;
  double distance = 10.;

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
            << "- interaction time: " << interactionTime << " s" << std::endl << std::endl;

  // Create AP and stations
  NodeContainer wifiApNode (1);
  NodeContainer wifiStaNodes (nWifi);

  // Configure mobility model
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator",
                                 "X", DoubleValue (0.0),
                                 "Y", DoubleValue (0.0),
                                 "rho", DoubleValue (distance));

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);
  mobility.Install (wifiStaNodes);

  // Make sure AP is at the center
  Ptr<MobilityModel> apMobility = wifiApNode.Get (0)->GetObject<MobilityModel> ();
  apMobility->SetPosition (Vector (0.0, 0.0, 0.0));

  // Print position of each node
  std::cout << "Node positions:" << std::endl;

  // AP position
  Ptr<MobilityModel> position = wifiApNode.Get (0)->GetObject<MobilityModel> ();
  Vector pos = position->GetPosition ();
  std::cout << "AP:\tx=" << pos.x << ", y=" << pos.y << std::endl;

  // Stations positions
  for (auto node = wifiStaNodes.Begin (); node != wifiStaNodes.End (); ++node)
    {
      position = (*node)->GetObject<MobilityModel> ();
      pos = position->GetPosition ();
      std::cout << "Sta " << (*node)->GetId () << ":\tx=" << pos.x << ", y=" << pos.y << std::endl;
    }

  std::cout << std::endl;

  // Configure wireless channel
  YansWifiPhyHelper phy;
  YansWifiChannelHelper channelHelper = YansWifiChannelHelper::Default ();
  phy.SetChannel (channelHelper.Create ());

  // Configure MAC layer
  WifiMacHelper mac;
  WifiHelper wifi;

  wifi.SetStandard (WIFI_STANDARD_80211ax);
  wifi.SetRemoteStationManager ("ns3::IdealWifiManager");

  // Set SSID
  Ssid ssid = Ssid ("ns3-80211ax");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "MaxMissedBeacons", UintegerValue (1000)); // prevents exhaustion of association IDs

  // Create and configure Wi-Fi interfaces
  NetDeviceContainer staDevice;
  staDevice = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevice;
  apDevice = wifi.Install (phy, mac, wifiApNode);

  // Set channel width and shortest GI
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelSettings",
               StringValue ("{0, " + std::to_string (channelWidth) + ", BAND_5GHZ, 0}"));

  // Install an Internet stack
  InternetStackHelper stack;
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  // Configure IP addressing
  Ipv4AddressHelper address ("192.168.1.0", "255.255.255.0");
  Ipv4InterfaceContainer staNodeInterface = address.Assign (staDevice);
  Ipv4InterfaceContainer apNodeInterface = address.Assign (apDevice);

  // PopulateArpCache
  PopulateARPcache ();

  // Configure applications
  DataRate applicationDataRate = DataRate (dataRate * 1e6);
  uint32_t portNumber = 9;

  for (uint32_t j = 0; j < wifiStaNodes.GetN (); ++j)
    {
      InstallTrafficGenerator (wifiStaNodes.Get (j), wifiApNode.Get (0), portNumber++,
                               applicationDataRate, packetSize);
    }

  // Install FlowMonitor
  FlowMonitorHelper flowmon;
  monitor = flowmon.InstallAll ();

  // Generate PCAP at AP
  if (!pcapName.empty ())
    {
      phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
      phy.EnablePcap (pcapName, apDevice);
    }

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

  // Record start time
  std::cout << "Starting simulation..." << std::endl;
  auto start = std::chrono::high_resolution_clock::now ();

  // Run the simulation!
  Simulator::Run ();

  // Record stop time and count duration
  auto finish = std::chrono::high_resolution_clock::now ();
  std::chrono::duration<double> elapsed = finish - start;

  std::cout << "Done!" << std::endl
            << "Elapsed time: " << elapsed.count () << " s" << std::endl
            << std::endl;

  // Calculate per-flow throughput and Jain's fairness index
  double nWifiReal = 0;
  double jainsIndexN = 0.;
  double jainsIndexD = 0.;

  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  std::cout << "Results: " << std::endl;

  for (auto &stat : stats)
    {
      double flow = (8 * stat.second.rxBytes - warmupFlows[stat.first]) / (1e6 * simulationTime);

      if (flow > 0)
        {
          nWifiReal += 1;
        }

      jainsIndexN += flow;
      jainsIndexD += flow * flow;

      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (stat.first);
      std::cout << "Flow " << stat.first << " (" << t.sourceAddress << " -> "
                << t.destinationAddress << ")\tThroughput: " << flow << " Mb/s" << std::endl;
    }

  double totalThr = jainsIndexN;
  double fairnessIndex = jainsIndexN * jainsIndexN / (nWifiReal * jainsIndexD);
  double totalPLR = warmupLost / warmupTX;
  double totalLatency = warmupDelay.GetSeconds ();
  double latencyPerPacketTotal = warmupDelay.GetSeconds () / warmupTX;

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
  csvOutput << agentName << "," << dataRate << "," << distance << "," << nWifi << "," << nWifiReal << ","
            << RngSeedManager::GetRun () << "," << fairnessIndex << "," << latencyPerPacketTotal << ","
            << totalPLR << "," << totalThr << std::endl;

  // Print results to std output
  std::cout << "agent,dataRate,distance,nWifi,nWifiReal,seed,fairness,latency,plr,throughput"
            << std::endl
            << csvOutput.str ();

  // Print results to file
  std::ofstream outputFile (csvPath);
  outputFile << csvOutput.str ();
  std::cout << std::endl << "Simulation data saved to: " << csvPath;

  std::ofstream outputLogFile (csvLogPath);
  outputLogFile << csvLogOutput.str ();
  std::cout << std::endl << "Simulation log saved to: " << csvLogPath << std::endl << std::endl;

  //Clean-up
  Simulator::Destroy ();
  m_env->SetFinish ();

  return 0;
}

/***** Function definitions *****/

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

void
InstallTrafficGenerator (Ptr<ns3::Node> fromNode, Ptr<ns3::Node> toNode, uint32_t port,
                         DataRate offeredLoad, uint32_t packetSize)
{
  // Get sink address
  Ptr<Ipv4> ipv4 = toNode->GetObject<Ipv4> ();
  Ipv4Address addr = ipv4->GetAddress (1, 0).GetLocal ();

  // Define type of service
  uint8_t tosValue = 0x70; //AC_BE

  // Add random fuzz to app start time
  Ptr<UniformRandomVariable> fuzz = CreateObject<UniformRandomVariable> ();
  fuzz->SetAttribute ("Min", DoubleValue (0.));
  fuzz->SetAttribute ("Max", DoubleValue (fuzzTime));
  fuzz->SetStream (0);
  double applicationsStart = fuzz->GetValue ();

  // Configure source and sink
  InetSocketAddress sinkSocket (addr, port);
  PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", sinkSocket);

  OnOffHelper onOffHelper ("ns3::UdpSocketFactory", sinkSocket);
  onOffHelper.SetConstantRate (offeredLoad, packetSize);
  onOffHelper.SetAttribute("Tos", UintegerValue(tosValue));

  // Configure applications
  ApplicationContainer sinkApplications (packetSinkHelper.Install (toNode));
  ApplicationContainer sourceApplications (onOffHelper.Install (fromNode));

  sinkApplications.Start (Seconds (applicationsStart));
  sourceApplications.Start (Seconds (applicationsStart));
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
}
