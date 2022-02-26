/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 Universita' di Firenze
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
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

// Network topology
//
//       n0    n1
//       |     |
//       =================
//        WSN (802.15.4)
//
// - ICMPv6 echo request flows from n0 to n1 and back with ICMPv6 echo reply
// - DropTail queues 
// - Tracing of queues and packet receptions to file "wsn-ping6.tr"
//
// This example is based on the "ping6.cc" example.

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/mobility-module.h"
#include <iostream>
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/internet-stack-helper.h"
#include <ns3/ripng-helper.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Ping6WsnExample");
//-----------------------routing-----------------//
class RoutingExperiment
{
public:
  RoutingExperiment ();
  void Run (int nSinks, double txp);
  void CommandSetup (int argc, char **argv);

private:
  Ptr<Socket> SetupPacketReceive (Ipv6Address addr, Ptr<Node> node);
  void ReceivePacket (Ptr<Socket> socket);

  uint32_t port;
  uint32_t bytesTotal;
  uint32_t packetsReceived;

  int m_nFlows;
  int m_nodes;
  int m_nPacketsPerSecond;
  int m_txRange;
  int m_CoverageArea;
  std::string m_protocolName;
  double m_txp;
  bool m_traceMobility;
  // uint32_t m_protocol;
};

RoutingExperiment::RoutingExperiment ()
  : port (9),
    bytesTotal (0),
    packetsReceived (0),
    m_nFlows(5),
    m_nodes(60),
    m_nPacketsPerSecond(1),
    m_txRange(1000),
    m_CoverageArea(300),
    m_traceMobility (false)
{
}

static inline std::string
PrintReceivedPacket (Ptr<Socket> socket, Ptr<Packet> packet, Address senderAddress)
{
  std::ostringstream oss;

  oss << Simulator::Now ().GetSeconds () << " " << socket->GetNode ()->GetId ();

  if (Inet6SocketAddress::IsMatchingType (senderAddress))
    {
      Inet6SocketAddress addr = Inet6SocketAddress::ConvertFrom (senderAddress);
      oss << " received one packet from " << addr.GetIpv6 ();
    }
  else
    {
      oss << " received one packet!";
    }
  return oss.str ();
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
RoutingExperiment::SetupPacketReceive (Ipv6Address addr, Ptr<Node> node)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> sink = Socket::CreateSocket (node, tid);
  Inet6SocketAddress local = Inet6SocketAddress (addr, port);
  sink->Bind (local);
  sink->SetRecvCallback (MakeCallback (&RoutingExperiment::ReceivePacket, this));

  return sink;

}

void
RoutingExperiment::CommandSetup (int argc, char **argv)
{
  CommandLine cmd (__FILE__);
  cmd.AddValue ("traceMobility", "Enable mobility tracing", m_traceMobility);
  cmd.AddValue ("nFlows", "Number of FLows", m_nFlows);
  cmd.AddValue ("nodes", "Number of Nodes", m_nodes);
  cmd.AddValue ("PacketsPerSecond", "Number of packets sent out every second", m_nPacketsPerSecond);
  cmd.AddValue ("coverageArea", "One side of the coverage area where the nodes are scattered", m_CoverageArea);
  cmd.Parse (argc, argv);

}


int main (int argc, char **argv)
{
  RoutingExperiment experiment;
    experiment.CommandSetup (argc,argv);
    
  int nSinks = 12;
  double txp = 7.5;
  //run
  experiment.Run (nSinks, txp);
}
//-----------------run-------------------//

void
RoutingExperiment::Run (int nSinks, double txp)
{
  Packet::EnablePrinting ();
  m_txp = txp;

  double TotalTime = 100.0;
  
  int packetSize = 64;
  int datarate = m_nPacketsPerSecond*packetSize*8;

  std::ostringstream dr;
  dr << datarate<<"bps";
  std::string rate (dr.str());

  std::string phyMode ("DsssRate11Mbps");

  std::ostringstream fname;
  fname << "802.15_"<<"coverageArea_"<<m_CoverageArea;
  std::string tr_name (fname.str());

  Config::SetDefault  ("ns3::OnOffApplication::PacketSize",StringValue ("64"));
  Config::SetDefault ("ns3::OnOffApplication::DataRate",  StringValue (rate));

  //Set Non-unicastMode rate to unicast mode
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (phyMode));

  NodeContainer adhocNodes;
  adhocNodes.Create (m_nodes);
  //set seed
  //Set seed for random numbers
  SeedManager::SetSeed (167);


  //---------------mobility------------------//
  MobilityHelper mobilityAdhoc;

  ObjectFactory pos;
  
  std::ostringstream cvg;
  cvg << "ns3::UniformRandomVariable[Min=0.0|Max="<<m_CoverageArea<<"]";
  std::string rect (cvg.str());
  std::cout<<"rectange "<<rect<<" by "<<rect<<std::endl;

  pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
  pos.Set ("X", StringValue (rect));
  pos.Set ("Y", StringValue (rect));
  
  
  Ptr<PositionAllocator> taPositionAlloc = pos.Create ()->GetObject<PositionAllocator> ();
  mobilityAdhoc.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityAdhoc.SetPositionAllocator (taPositionAlloc);
  mobilityAdhoc.Install (adhocNodes);

  //--------------end of mobility-----------------//
  Config::SetDefault ("ns3::RangePropagationLossModel::MaxRange", DoubleValue (m_txRange));

  //-----------------for 802.15---------------//
  NS_LOG_INFO ("Create channels.");
  LrWpanHelper lrWpanHelper;
  
  NetDeviceContainer devContainer = lrWpanHelper.Install(adhocNodes);
  lrWpanHelper.AssociateToPan (devContainer, 10);
  std::cout << "Created " << devContainer.GetN() << " devices" << std::endl;
  std::cout << "There are " << adhocNodes.GetN() << " nodes" << std::endl;
  //--------------for 802.15---------------//

  //set routing 
  RipNgHelper rip;
  Ipv6ListRoutingHelper list;
  list.Add(rip,100);

  /* Install IPv4/IPv6 stack */
  NS_LOG_INFO ("Install Internet stack.");
  InternetStackHelper internetv6;
  internetv6.SetIpv4StackInstall (false);
  //internetv6.SetRoutingHelper(list);

  internetv6.Install (adhocNodes);
  // Install 6LowPan layer
  NS_LOG_INFO ("Install 6LoWPAN.");
  SixLowPanHelper sixlowpan;
  NetDeviceContainer six1 = sixlowpan.Install (devContainer);
  
  
  InternetStackHelper internet;


  //NETWORK LAYER
  NS_LOG_INFO ("Assign addresses.");
  Ipv6AddressHelper ipv6;
  ipv6.SetBase (Ipv6Address ("2001:1::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer adhocInterfaces = ipv6.Assign (six1);
  
  //APPLICATION LAYER
  OnOffHelper onoff1 ("ns3::UdpSocketFactory",Address ());
  onoff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
  onoff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));
  

  //send packets
  for (int i = 0; i < m_nFlows; i++)
    {
      //std::cout << "i=>" << i<< std::endl;
      Ptr<Socket> sink = SetupPacketReceive (adhocInterfaces.GetAddress (i,1), adhocNodes.Get (i));
      AddressValue remoteAddress (Inet6SocketAddress (adhocInterfaces.GetAddress (i,1), port));
      onoff1.SetAttribute ("Remote", remoteAddress);

      Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable> ();
      ApplicationContainer temp = onoff1.Install (adhocNodes.Get (i + m_nFlows));
     
      temp.Start (Seconds (var->GetValue (20.0,21.0)));
      temp.Stop (Seconds (TotalTime));
    }

  std::stringstream ss;
  ss << m_nodes;
  std::string nodes = ss.str ();

  std::stringstream ss4;
  ss4 << rate;
  std::string sRate = ss4.str ();

  Ptr<FlowMonitor> flowmon;
  FlowMonitorHelper flowmonHelper;
  flowmon = flowmonHelper.InstallAll ();


  NS_LOG_INFO ("Run Simulation.");


  Simulator::Stop (Seconds (TotalTime));
  Simulator::Run ();

  flowmon->SerializeToXmlFile ((tr_name  + ".xml").c_str(), false, false);

  Simulator::Destroy ();
}


