/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 ResiliNets, ITTC, University of Kansas
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * // TOPOLOGY:
//
//                (access)        (shared)         (access)
//         source1---------                       --------sink1
//         source2---------                       --------sink2
//                    .                                .
                      .      GW-left-----GW-Right      .
//                    .                                .
//                    .                                .
//         sourceN---------                       --------sinkN
//                                (shared)         
//
// Can create N source and sink pair and Flows between each pair.
// Each pair has their own flow thro a shair pair of gateways that are used
// by other nodes in the topology.
//
// There are two sets of access deleys, one is assigned to odd flows
// the other is assigned to even flows
 *
 
 */

#include <iostream>
#include <fstream>
#include <string>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/error-model.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/enum.h"
#include "ns3/event-id.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpVariantsComparison");

static bool firstCwnd = true;
static bool firstSshThr = true;
static bool firstRtt = true;
static bool firstRto = true;
static bool firstRto_by_rtt = true;
static bool firstMean_retransmission = true;

static Ptr<OutputStreamWrapper> cWndStream;
static Ptr<OutputStreamWrapper> ssThreshStream;
static Ptr<OutputStreamWrapper> rttStream;
static Ptr<OutputStreamWrapper> rtoStream;
static Ptr<OutputStreamWrapper> nextTxStream;
static Ptr<OutputStreamWrapper> nextRxStream;
static Ptr<OutputStreamWrapper> inFlightStream;
static Ptr<OutputStreamWrapper> rto_by_rttStream;
static Ptr<OutputStreamWrapper> mean_retransmissionStream;


static void
CwndTracer (uint32_t oldval, uint32_t newval)
{
  if (firstCwnd)
    {
      *cWndStream->GetStream () << "0.0 " << oldval << std::endl;
      firstCwnd = false;
    }
  else
    {
      *ssThreshStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval<< std::endl;
    }
}

static void
SsThreshTracer (uint32_t oldval, uint32_t newval)
{
  if (firstSshThr)
    {
      *ssThreshStream->GetStream () << "0.0 " << oldval << std::endl;
      firstSshThr = false;
    }
  else
    {
      *cWndStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval << std::endl;
    }
}

//-----------------------added by afnan---------------------------
int packets_sent = 0;
int packets_received = 0;
int packets_dropped = 0;
int received_bytes = 0;

//data file to plot graph
AsciiTraceHelper asciiTraceHelper;
Ptr<OutputStreamWrapper> result = asciiTraceHelper.CreateFileStream ("metrics.data");


static void
RttTracer (Time oldval, Time newval)
{
  if (firstRtt)
    {
      *rttStream->GetStream () << "0.0 " << oldval.GetSeconds () << std::endl;
      firstRtt = false;
    }
  *rttStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval.GetSeconds () << std::endl;
}

static void
RtoTracer (Time oldval, Time newval)
{
  if (firstRto)
    {
      *rtoStream->GetStream () << "0.0 " << oldval.GetSeconds () << std::endl;
      firstRto = false;
    }
  else{
    *rtoStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval.GetSeconds () << std::endl;
  }
}

static void 
RtoByRttTracer (double oldval, double newval)
{
  if (firstRto_by_rtt)
    {
      *rto_by_rttStream->GetStream () << "0.0 " << oldval << std::endl;
      firstRto_by_rtt = false;
    }
  else{
    *rto_by_rttStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval << std::endl;
  }
}

static void 
MeanRetransmissionTracer (double oldval, double newval)
{
  if (firstMean_retransmission)
    {
      *mean_retransmissionStream->GetStream () << "0.0 " << oldval << std::endl;
      firstMean_retransmission = false;
    }
  else{
    *mean_retransmissionStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval << std::endl;
  }
}

/*
static void
NextTxTracer (SequenceNumber32 old, SequenceNumber32 nextTx)
{
  NS_UNUSED (old);
  *nextTxStream->GetStream () << Simulator::Now ().GetSeconds () << " " << nextTx << std::endl;
}

static void
InFlightTracer (uint32_t old, uint32_t inFlight)
{
  NS_UNUSED (old);
  *inFlightStream->GetStream () << Simulator::Now ().GetSeconds () << " " << inFlight << std::endl;
}

static void
NextRxTracer (SequenceNumber32 old, SequenceNumber32 nextRx)
{
  NS_UNUSED (old);
  *nextRxStream->GetStream () << Simulator::Now ().GetSeconds () << " " << nextRx << std::endl;
}
*/
static void
TraceCwnd (std::string cwnd_tr_file_name)
{
  AsciiTraceHelper ascii;
  cWndStream = ascii.CreateFileStream (cwnd_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/2/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeCallback (&CwndTracer));
}

static void
TraceSsThresh (std::string ssthresh_tr_file_name)
{
  AsciiTraceHelper ascii;
  ssThreshStream = ascii.CreateFileStream (ssthresh_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/2/$ns3::TcpL4Protocol/SocketList/0/SlowStartThreshold", MakeCallback (&SsThreshTracer));
}
/*
static void
TraceRtt (std::string rtt_tr_file_name)
{
  AsciiTraceHelper ascii;
  rttStream = ascii.CreateFileStream (rtt_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/RTT", MakeCallback (&RttTracer));
}

static void
TraceRto (std::string rto_tr_file_name)
{
  AsciiTraceHelper ascii;
  rtoStream = ascii.CreateFileStream (rto_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/RTO", MakeCallback (&RtoTracer));
}


static void
TxTracer(const Ptr< const Packet > packet, const TcpHeader &header, const Ptr< const TcpSocketBase > socket) {
  packets_sent += 1;
}

static void
RxTracer(const Ptr< const Packet > packet, const TcpHeader &header, const Ptr< const TcpSocketBase > socket) {
  packets_received += 1;
  received_bytes += packet->GetSize();
}
*/
static void
TraceRtt (std::string rtt_tr_file_name)
{
  AsciiTraceHelper ascii;
  rttStream = ascii.CreateFileStream (rtt_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/2/$ns3::TcpL4Protocol/SocketList/0/RTT", MakeCallback (&RttTracer));
}

static void
TraceRto (std::string rto_tr_file_name)
{
  AsciiTraceHelper ascii;
  rtoStream = ascii.CreateFileStream (rto_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/2/$ns3::TcpL4Protocol/SocketList/0/RTO", MakeCallback (&RtoTracer));
}

static void
TraceRto_By_Rtt (std::string rto_tr_file_name)
{
  AsciiTraceHelper ascii;
  rto_by_rttStream = ascii.CreateFileStream (rto_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/2/$ns3::TcpL4Protocol/SocketList/0/rto_by_rttTrace", MakeCallback (&RtoByRttTracer));
}


static void
TraceMeanRetransmission (std::string rto_tr_file_name)
{
  AsciiTraceHelper ascii;
  mean_retransmissionStream = ascii.CreateFileStream (rto_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/2/$ns3::TcpL4Protocol/SocketList/0/m_mean_retransmisionTrace", MakeCallback (&MeanRetransmissionTracer));
}


/*
static void
TraceTxRx (int src , int dest)
{
  AsciiTraceHelper ascii;
  Config::ConnectWithoutContext ("/NodeList/"+std::to_string(2)+"/$ns3::TcpL4Protocol/SocketList/0/Tx", MakeCallback (&TxTracer));
  Config::ConnectWithoutContext ("/NodeList/" + std::to_string(2) + "/$ns3::TcpL4Protocol/SocketList/0/Rx", MakeCallback (&RxTracer));
}
*/
/*
static void
TraceNextTx (std::string &next_tx_seq_file_name)
{
  AsciiTraceHelper ascii;
  nextTxStream = ascii.CreateFileStream (next_tx_seq_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/NextTxSequence", MakeCallback (&NextTxTracer));
}

static void
TraceInFlight (std::string &in_flight_file_name)
{
  AsciiTraceHelper ascii;
  inFlightStream = ascii.CreateFileStream (in_flight_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/BytesInFlight", MakeCallback (&InFlightTracer));
}


static void
TraceNextRx (std::string &next_rx_seq_file_name)
{
  AsciiTraceHelper ascii;
  nextRxStream = ascii.CreateFileStream (next_rx_seq_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/2/$ns3::TcpL4Protocol/SocketList/1/RxBuffer/NextRxSequence", MakeCallback (&NextRxTracer));
}
*/
int main (int argc, char *argv[])
{
  std::string transport_prot = "TcpWestwood";
  double error_p = 0.001;
  /*std::string bandwidth = "2Mbps";
  std::string delay = "0.01ms";
  std::string access_bandwidth = "10Mbps";
  std::string access_delay = "45ms";
  */
  bool tracing = true;
  bool justa = false;
  bool peakHopper = false;
  std::string prefix_file_name = "";
  uint64_t data_mbytes = 0;
  uint32_t mtu_bytes = 400;
  uint16_t num_flows = 6;
  double duration = 20.0;
  uint32_t run = 0;
  bool flow_monitor = true;
  bool pcap = false;
  bool sack = false;
  //-----------------------added by afnan----------------------
  /*Default TCP flavor*/
  NS_LOG_INFO ("Bottleneck Link: 10Mbps , 35ms delay");
  std::string shared_bandwidth = "10Mbps";
  std::string shared_delay = "50ms";

  NS_LOG_INFO ("Access Links: 2Mbps , 175ms delay");
  std::string access_bandwidth = "2Mbps";
  std::string access_delay = "175ms";

  NS_LOG_INFO ("Access2 Links: 2Mbps , 45ms delay");
  std::string access_bandwidth2 = "2Mbps";
  std::string access_delay2 = "85ms";
  //--------------------------------------

  std::string queue_disc_type = "ns3::PfifoFastQueueDisc";
  std::string recovery = "ns3::TcpClassicRecovery";


  CommandLine cmd;
  cmd.AddValue ("transport_prot", "Transport protocol to use: TcpNewReno, "
                "TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, "
                "TcpBic, TcpYeah, TcpIllinois, TcpWestwood, TcpWestwoodPlus, TcpLedbat, "
		"TcpLp", transport_prot);
  cmd.AddValue ("error_p", "Packet error rate", error_p);
  cmd.AddValue ("bandwidth", "Bottleneck bandwidth", access_bandwidth);
  cmd.AddValue ("delay", "Bottleneck delay", access_delay);
  cmd.AddValue ("access_bandwidth", "Access link bandwidth", access_bandwidth);
  cmd.AddValue ("access_delay", "Access link delay", access_delay);
  cmd.AddValue ("tracing", "Flag to enable/disable tracing", tracing);
  cmd.AddValue ("justa", "Rto calculation algorithm type to use ", justa);
  cmd.AddValue ("prefix_name", "Prefix of output trace file", prefix_file_name);
  cmd.AddValue ("data", "Number of Megabytes of data to transmit", data_mbytes);
  cmd.AddValue ("mtu", "Size of IP packets to send in bytes", mtu_bytes);
  cmd.AddValue ("num_flows", "Number of flows", num_flows);
  cmd.AddValue ("duration", "Time to allow flows to run in seconds", duration);
  cmd.AddValue ("run", "Run index (for setting repeatable seeds)", run);
  cmd.AddValue ("flow_monitor", "Enable flow monitor", flow_monitor);
  cmd.AddValue ("pcap_tracing", "Enable or disable PCAP tracing", pcap);
  cmd.AddValue ("queue_disc_type", "Queue disc type for gateway (e.g. ns3::CoDelQueueDisc)", queue_disc_type);
  cmd.AddValue ("sack", "Enable or disable SACK option", sack);
  cmd.AddValue ("recovery", "Recovery algorithm type to use (e.g., ns3::TcpPrrRecovery", recovery);
  cmd.AddValue ("peakHopper", "Rto calculation algorithm type to use ", peakHopper);

  cmd.Parse (argc, argv);

  transport_prot = std::string ("ns3::") + transport_prot;

  SeedManager::SetSeed (1);
  SeedManager::SetRun (run);

  // User may find it convenient to enable logging
  //LogComponentEnable("TcpVariantsComparison", LOG_LEVEL_ALL);
  //LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);
  //LogComponentEnable("PfifoFastQueueDisc", LOG_LEVEL_ALL);

  // Calculate the ADU size
  Header* temp_header = new Ipv4Header ();
  uint32_t ip_header = temp_header->GetSerializedSize ();
  NS_LOG_LOGIC ("IP Header size is: " << ip_header);
  delete temp_header;
  temp_header = new TcpHeader();
  uint32_t tcp_header = temp_header->GetSerializedSize ();
  NS_LOG_LOGIC ("TCP Header size is: " << tcp_header);
  delete temp_header;
  uint32_t tcp_adu_size = mtu_bytes - 20 - (ip_header + tcp_header);
  NS_LOG_LOGIC ("TCP ADU size is: " << tcp_adu_size);

  // Set the simulation start and stop time
  double start_time = 0.1;
  double stop_time = start_time + duration;

  // 4 MB of TCP buffer
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1 << 21));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1 << 21));
  Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (sack));

  if(peakHopper){
    Config::SetDefault("ns3::TcpSocketBase::m_peakHopper", BooleanValue(peakHopper));
    Config::SetDefault("ns3::RttMeanDeviation::m_peakHopper", BooleanValue(peakHopper));
  }


  Config::SetDefault ("ns3::TcpL4Protocol::RecoveryType",
                      TypeIdValue (TypeId::LookupByName (recovery)));
  // Select TCP variant
  if (transport_prot.compare ("ns3::TcpWestwoodPlus") == 0)
    {
      // TcpWestwoodPlus is not an actual TypeId name; we need TcpWestwood here
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpWestwood::GetTypeId ()));
      // the default protocol type in ns3::TcpWestwood is WESTWOOD
      Config::SetDefault ("ns3::TcpWestwood::ProtocolType", EnumValue (TcpWestwood::WESTWOODPLUS));
    }
  else
    {
      TypeId tcpTid;
      NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (transport_prot, &tcpTid), "TypeId " << transport_prot << " not found");
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (transport_prot)));
    }

    // Create gateways, sources, and sinks
    NodeContainer leftGate;
    leftGate.Create (1);
    NS_LOG_INFO ("Number of Nodes After left Gate: " << NodeList::GetNNodes());

    NodeContainer rightGate;
    rightGate.Create (1);
    NS_LOG_INFO ("Number of Nodes After Right Gate: " << NodeList::GetNNodes());

    NodeContainer sources;
    sources.Create (num_flows);
    NS_LOG_INFO ("Totoal Number of Nodes After Sources: " << NodeList::GetNNodes());

    NodeContainer sinks;
    sinks.Create (num_flows);
    NS_LOG_INFO ("Totoal Number of Nodes After Sinks: " << NodeList::GetNNodes());


    NS_LOG_INFO ("Total Number of Flows: " << num_flows);
    NS_LOG_INFO ("Total Number of Nodes After Sinks: " << NodeList::GetNNodes());
    Ptr< Node > tmp = sources.Get (0);
    NS_LOG_INFO ("First Source Node Index: " << tmp->GetId());
    tmp = sinks.Get (0);
    NS_LOG_INFO ("First Sink Node Index: " << tmp->GetId());
    NS_LOG_INFO ("TCP Flavor: " << transport_prot);

  // Configure the error model
  // Here we use RateErrorModel with packet error rate
  Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
  uv->SetStream (50);
  RateErrorModel error_model;
  error_model.SetRandomVariable (uv);
  error_model.SetUnit (RateErrorModel::ERROR_UNIT_PACKET);
  error_model.SetRate (error_p);

  PointToPointHelper BottleNeckLink;
  BottleNeckLink.SetDeviceAttribute ("DataRate", StringValue (shared_bandwidth));
  BottleNeckLink.SetChannelAttribute ("Delay", StringValue (shared_delay));
  BottleNeckLink.SetDeviceAttribute ("ReceiveErrorModel", PointerValue (&error_model));

  NS_LOG_INFO ("Install internet stack on all nodes.");
  InternetStackHelper stack;
  stack.InstallAll ();

  NetDeviceContainer gates;
  gates = BottleNeckLink.Install (leftGate.Get (0),rightGate.Get (0));


  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.0.0");

  Ipv4InterfaceContainer backbone = address.Assign(gates);
  Ipv4InterfaceContainer sink_interfaces;
  // Configure the sources and sinks net devices
  // and the channels between the sources/sinks and the gateways
  PointToPointHelper LocalLinkL;
  PointToPointHelper LocalLinkR;
  for (int i = 0; i < num_flows; i++)
      {
        if ((i%2) == 0) {
          LocalLinkL.SetDeviceAttribute ("DataRate", StringValue (access_bandwidth));
          LocalLinkL.SetChannelAttribute ("Delay", StringValue (access_delay));
          LocalLinkR.SetDeviceAttribute ("DataRate", StringValue (access_bandwidth));
          LocalLinkR.SetChannelAttribute ("Delay", StringValue (access_delay));

          NetDeviceContainer devices;
          devices = LocalLinkL.Install (sources.Get (i), leftGate.Get (0));

          address.NewNetwork ();
          Ipv4InterfaceContainer interfaces = address.Assign (devices);

          devices = LocalLinkR.Install (rightGate.Get (0), sinks.Get (i));
          address.NewNetwork ();
          interfaces = address.Assign (devices);
          sink_interfaces.Add (interfaces.Get (1));

        } else {
          LocalLinkL.SetDeviceAttribute ("DataRate", StringValue (access_bandwidth2));
          LocalLinkL.SetChannelAttribute ("Delay", StringValue (access_delay2));
          LocalLinkR.SetDeviceAttribute ("DataRate", StringValue (access_bandwidth2));
          LocalLinkR.SetChannelAttribute ("Delay", StringValue (access_delay2));

          NetDeviceContainer devices;
          devices = LocalLinkL.Install (sources.Get (i), leftGate.Get (0));

          address.NewNetwork ();
          Ipv4InterfaceContainer interfaces = address.Assign (devices);

          devices = LocalLinkR.Install (rightGate.Get (0), sinks.Get (i));
          address.NewNetwork ();
          interfaces = address.Assign (devices);
          sink_interfaces.Add (interfaces.Get (1));
        }
      }

  NS_LOG_INFO ("Initialize Global Routing.");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  uint16_t port = 50000;
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);

  for (uint16_t i = 0; i < sources.GetN (); i++)
    {
      AddressValue remoteAddress (InetSocketAddress (sink_interfaces.GetAddress (i, 0), port));
      Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (tcp_adu_size));
      BulkSendHelper ftp ("ns3::TcpSocketFactory", Address ());
      ftp.SetAttribute ("Remote", remoteAddress);
      ftp.SetAttribute ("SendSize", UintegerValue (tcp_adu_size));
      ftp.SetAttribute ("MaxBytes", UintegerValue (data_mbytes * 1000000));

      ApplicationContainer sourceApp = ftp.Install (sources.Get (i));
      sourceApp.Start (Seconds (start_time * i));
      sourceApp.Stop (Seconds (stop_time - 3));

      sinkHelper.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
      ApplicationContainer sinkApp = sinkHelper.Install (sinks.Get (i));
      sinkApp.Start (Seconds (start_time * i));
      sinkApp.Stop (Seconds (stop_time));
    }

  // Set up tracing if enabled
  if (tracing)
    {
      std::ofstream ascii;
      Ptr<OutputStreamWrapper> ascii_wrap;
      ascii.open ((prefix_file_name + "-ascii").c_str ());
      ascii_wrap = new OutputStreamWrapper ((prefix_file_name + "-ascii").c_str (),
                                            std::ios::out);
      stack.EnableAsciiIpv4All (ascii_wrap);

      Simulator::Schedule (Seconds (0.1), &TraceCwnd, prefix_file_name + "cwnd.data");
      Simulator::Schedule (Seconds (0.1), &TraceSsThresh, prefix_file_name + "ssth.data");
      Simulator::Schedule (Seconds (0.1), &TraceRtt, prefix_file_name + "rtt.data");   //3rd arg ta TraceRtt func er parameter
      Simulator::Schedule (Seconds (0.1), &TraceRto, prefix_file_name + "rto.data");
      Simulator::Schedule (Seconds (0.1), &TraceRto_By_Rtt, prefix_file_name + "rto_by_rtt.data");
      Simulator::Schedule (Seconds (0.1), &TraceMeanRetransmission, prefix_file_name + "mean_retransmission.data");

      //Simulator::Schedule (Seconds (0.1), &TraceNextTx, prefix_file_name + "-next-tx.data");
      //Simulator::Schedule (Seconds (0.1), &TraceInFlight, prefix_file_name + "-inflight.data");
      //Simulator::Schedule (Seconds (0.1), &TraceNextRx, prefix_file_name + "-next-rx.data");
     // for(int i=2 ; i<2*num_flows+2 ; i++){
        //Simulator::Schedule (Seconds (0.1), &TraceTxRx , 5 , 11);
       // Simulator::Schedule (Seconds (0.1), &TraceTxRx , 2 , 2);
        
     // }

    }

  if(pcap){
    PcapHelper pcapHelper;
    Ptr<PcapFileWrapper> file = pcapHelper.CreateFile ("PeakHopper.pcap", std::ios::out, PcapHelper::DLT_PPP);

  }

  FlowMonitorHelper flomon;
  Ptr<FlowMonitor>monitor;
  if(flow_monitor){
    monitor = flomon.InstallAll();
  }
  Simulator::Stop (Seconds (stop_time));
  Simulator::Run ();

  if (flow_monitor)
    {
      FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
      for(auto iter = stats.begin(); iter != stats.end(); iter++) {
        NS_LOG_UNCOND("----Flow ID:" <<iter->first);
        NS_LOG_UNCOND("Sent Packets = " << iter->second.txPackets);
        NS_LOG_UNCOND("Received Packets = " << iter->second.rxPackets);
        NS_LOG_UNCOND("Lost Packets = " << iter->second.txPackets - iter->second.rxPackets);
        NS_LOG_UNCOND("Packet loss ratio = " << (iter->second.txPackets-iter->second.rxPackets)
                                              *100.0/iter->second.txPackets << "%");
        NS_LOG_UNCOND("Throughput = " << iter->second.rxBytes * 8.0/(iter->second.timeLastRxPacket
                                        .GetSeconds()-iter->second.timeFirstTxPacket
                                        .GetSeconds()) << " bits/s");
      }
    }

  Simulator::Destroy ();
  
  return 0;
}
