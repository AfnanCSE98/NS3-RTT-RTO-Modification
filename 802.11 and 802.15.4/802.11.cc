/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 University of Kansas
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
 *
 * Author: Justin Rohrer <rohrej@ittc.ku.edu>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  http://wiki.ittc.ku.edu/resilinets
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 */

/*
 * This example program allows one to run ns-3 DSDV, AODV, or OLSR under
 * a typical random waypoint mobility model.
 *
 * By default, the simulation runs for 200 simulated seconds, of which
 * the first 50 are used for start-up time.  The number of nodes is 50.
 * Nodes move according to RandomWaypointMobilityModel with a speed of
 * 20 m/s and no pause time within a 300x1500 m region.  The WiFi is
 * in ad hoc mode with a 2 Mb/s rate (802.11b) and a Friis loss model.
 * The transmit power is set to 7.5 dBm.
 *
 * It is possible to change the mobility and density of the network by
 * directly modifying the speed and the number of nodes.  It is also
 * possible to change the characteristics of the network by changing
 * the transmit power (as power increases, the impact of mobility
 * decreases and the effective density increases).
 *
 * By default, OLSR is used, but specifying a value of 2 for the protocol
 * will cause AODV to be used, and specifying a value of 3 will cause
 * DSDV to be used.
 *
 * By default, there are 10 source/sink data pairs sending UDP data
 * at an application rate of 2.048 Kb/s each.    This is typically done
 * at a rate of 4 64-byte packets per second.  Application data is
 * started at a random time between 50 and 51 seconds and continues
 * to the end of the simulation.
 *
 * The program outputs a few items:
 * - packet receptions are notified to stdout such as:
 *   <timestamp> <node-id> received one packet from <src-address>
 * - each second, the data reception statistics are tabulated and output
 *   to a comma-separated value (csv) file
 * - some tracing and flow monitor configuration that used to work is
 *   left commented inline in the program
 */

#include <fstream>
#include <iostream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/v4ping-helper.h"



using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("aodv-simulation");

class RoutingExperiment
{
public:
  RoutingExperiment ();
  void Run (double txp, double time);
  std::string CommandSetup (int argc, char **argv);

private:
  Ptr<Socket> SetupPacketReceive (Ipv4Address addr, Ptr<Node> node);
  void ReceivePacket (Ptr<Socket> socket);
  // void CheckThroughput ();

  uint32_t port;
  uint32_t bytesTotal;
  uint32_t packetsReceived;

  std::string m_CSVfileName;
  int m_nNodes;
  int m_nFlows;
  int m_nPacketsPerSecond;
  int m_txRange;
  int m_CovAreaSide;

  std::string m_protocolName;
  double m_txp;
  bool m_traceMobility;
  uint32_t m_protocol;
};

void printNodePositions()
{
  NodeContainer const & n = NodeContainer::GetGlobal ();
  for (NodeContainer::Iterator i = n.Begin (); i != n.End (); ++i)
  {
    Ptr<Node> node = *i;
    std::string name = Names::FindName (node);

    Ptr<MobilityModel> mob = node->GetObject<MobilityModel> ();
    if(!mob) continue;

    Vector pos = mob->GetPosition ();
    std::cout << "Node " << name << " is at (" << pos.x << ", " <<
    pos.y << ", " << pos.z << ")\n";

  }
}


static inline std::string
PrintReceivedPacket (Ptr<Socket> socket, Ptr<Packet> packet, Address senderAddress)
{
  std::ostringstream oss;

  oss << Simulator::Now ().GetSeconds () << " " << socket->GetNode ()->GetId ();

  if (InetSocketAddress::IsMatchingType (senderAddress))
    {
      InetSocketAddress addr = InetSocketAddress::ConvertFrom (senderAddress);
      oss << " received one packet from " << addr.GetIpv4 ();
    }
  else
    {
      oss << " received one packet!";
    }
  return oss.str ();
}


RoutingExperiment::RoutingExperiment ()
  : port (9),
    bytesTotal (0),
    packetsReceived (0),
    m_nNodes (60),
    m_nFlows(10),
    m_nPacketsPerSecond(10),
    m_txRange(1000),
    m_CovAreaSide(1000),
    //add coverage area default values//
    m_traceMobility (false)
{
}

void
RoutingExperiment::ReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address senderAddress;
  while ((packet = socket->RecvFrom (senderAddress)))
    {
      bytesTotal += packet->GetSize ();
      packetsReceived += 1;
      NS_LOG_UNCOND (PrintReceivedPacket (socket, packet, senderAddress));
    }
}

Ptr<Socket>
RoutingExperiment::SetupPacketReceive (Ipv4Address addr, Ptr<Node> node)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> sink = Socket::CreateSocket (node, tid);
  InetSocketAddress local = InetSocketAddress (addr, port);
  sink->Bind (local);
  sink->SetRecvCallback (MakeCallback (&RoutingExperiment::ReceivePacket, this));

  return sink;
}

std::string
RoutingExperiment::CommandSetup (int argc, char **argv)
{
  CommandLine cmd;

  cmd.AddValue ("numberOfNodes", "Number of nodes in the simulation", m_nNodes);
  cmd.AddValue ("packetsPerSecond", "Number of packets sent out every second", m_nPacketsPerSecond);
  cmd.AddValue ("numberOfFlows", "Number of flows", m_nFlows);
  cmd.AddValue ("coverageArea", "One side of the coverage area where the nodes are scattered", m_CovAreaSide);
  cmd.Parse (argc, argv);
  return m_CSVfileName;
}

void
RoutingExperiment::Run (double txp, double time)
{
  //setup
  Packet::EnablePrinting ();
  m_txp = txp;

  int packetSize = 64;
  int datarate = m_nPacketsPerSecond*packetSize*8; //?

  std::ostringstream dr;
  dr << datarate<<"bps";
  std::string rate (dr.str());


  double TotalTime = time;
  std::cout<<"datarate "<<rate<<std::endl;
  std::string phyMode ("DsssRate11Mbps");

  std::ostringstream fname;
  fname << "aodv-simulation_"<<"nodes_"<<m_nNodes;
  std::string tr_name (fname.str());

  m_protocolName = "protocol";

  Config::SetDefault  ("ns3::OnOffApplication::PacketSize",StringValue ("64"));
  Config::SetDefault ("ns3::OnOffApplication::DataRate",  StringValue (rate));

  //Set Non-unicastMode rate to unicast mode
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (phyMode));


//CreateNodes
  NodeContainer adhocNodes;
  adhocNodes.Create (m_nNodes);
  //Name nodes
  for (int i = 0; i < m_nNodes; ++i)
    {
      std::ostringstream os;
      os << "node-" << i;
      Names::Add (os.str (), adhocNodes.Get (i));
   }

  MobilityHelper mobilityAdhoc;
  
  // double maxCoverageLength = m_CovAreaSide;
  std::ostringstream cvg;
  cvg << "ns3::UniformRandomVariable[Min=0.0|Max="<<m_CovAreaSide<<"]";
  std::string rect (cvg.str());
  std::cout<<"rectange "<<rect<<" by "<<rect<<std::endl;
  
  ObjectFactory pos;
  pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
  pos.Set ("X", StringValue (rect));
  pos.Set ("Y", StringValue (rect));

  Ptr<PositionAllocator> taPositionAlloc = pos.Create ()->GetObject<PositionAllocator> ();

  mobilityAdhoc.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityAdhoc.SetPositionAllocator (taPositionAlloc);
  mobilityAdhoc.Install (adhocNodes);

  //printNodePositions();


  //CreateDevices
  // setting up wifi phy and channel using helpers
  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy;
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel"); //previously used friis //range prop not working though
  wifiPhy.SetChannel (wifiChannel.Create ());

  Config::SetDefault ("ns3::RangePropagationLossModel::MaxRange", DoubleValue (m_txRange));


  // Add a mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));

  wifiPhy.Set ("TxPowerStart",DoubleValue (txp));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (txp));

  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer adhocDevices = wifi.Install (wifiPhy, wifiMac, adhocNodes);


  //InstallInternetStack
  AodvHelper aodv;
  InternetStackHelper internet;
  internet.SetRoutingHelper (aodv);
  internet.Install(adhocNodes);

  NS_LOG_INFO ("assigning ip address");

  Ipv4AddressHelper addressAdhoc;
  addressAdhoc.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer adhocInterfaces;
  adhocInterfaces = addressAdhoc.Assign (adhocDevices);



  //InstallApplications
  //---------------------onoff-----------------------------------//
  OnOffHelper onoff1 ("ns3::UdpSocketFactory",Address ());
  onoff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
  onoff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));

  for (int i = 0; i < m_nFlows; i++)
    {
      Ptr<Socket> sink = SetupPacketReceive (adhocInterfaces.GetAddress (i), adhocNodes.Get (i));
      std::cout<<adhocInterfaces.GetAddress(i)<<std::endl;
      std::cout<<adhocInterfaces.GetAddress (i+m_nFlows)<<std::endl<<std::endl<<std::endl;

      AddressValue remoteAddress (InetSocketAddress (adhocInterfaces.GetAddress (i), port));
      onoff1.SetAttribute ("Remote", remoteAddress);

      Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable> ();
      ApplicationContainer temp = onoff1.Install (adhocNodes.Get (i + m_nFlows));
      temp.Start (Seconds (var->GetValue (20.0,21.0)));
      temp.Stop (Seconds (TotalTime));
    }

  Ptr<FlowMonitor> flowmon;
  FlowMonitorHelper flowmonHelper;
  flowmon = flowmonHelper.InstallAll ();


  NS_LOG_INFO ("Run Simulation.");

  //CheckThroughput ();

  Simulator::Stop (Seconds (TotalTime));
  Simulator::Run ();

  flowmon->SerializeToXmlFile ((tr_name + ".xml").c_str(), false, false);

  Simulator::Destroy ();
}

int
main (int argc, char *argv[])
{
   LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
   LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    
  RoutingExperiment experiment;
  std::string CSVfileName = experiment.CommandSetup (argc,argv);

 /* //blank out the last output file and write the column headers
  std::ofstream out (CSVfileName.c_str ());
  out << "SimulationSecond," <<
  "ReceiveRate(kbit/s)," <<
  "PacketsReceived," <<
  "NumberOfSinks," <<
  "RoutingProtocol," <<
  "TransmissionPower" <<
  std::endl;
  out.close ();
*/
  
  double txp = 7.5;
  double time = 100.0;

  experiment.Run (txp,time);
}
