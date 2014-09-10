/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 Polytechnic Institute of NYU
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
 * Author: Adrian S. Tam <adrian.sw.tam@gmail.com>
 * Author: Fan Wang <amywangfan1985@yahoo.com.cn>
 */

#define __STDC_LIMIT_MACROS 1
#include <sstream>
#include <stdint.h>
#include <stdlib.h>

#include "fat-tree-helper.h"

#include "ns3/log.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/queue.h"
#include "ns3/uinteger.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/ipv4-address-generator.h"
#include "ns3/config.h"
#include "ns3/abort.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/string.h"
#include "ns3/socket.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet.h"

#include "ipv4-mix-routing-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FatTreeHelper");

NS_OBJECT_ENSURE_REGISTERED (FatTreeHelper);

unsigned FatTreeHelper::m_size = 0;	// Defining static variable

TypeId
FatTreeHelper::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FatTreeHelper")
    .SetParent<Object> ()
    .AddAttribute ("HeDataRate",
                   "The default data rate for point to point links",
                   DataRateValue (DataRate ("1000Mbps")),
                   MakeDataRateAccessor (&FatTreeHelper::m_heRate),
                   MakeDataRateChecker ())
    .AddAttribute ("EaDataRate",
                   "The default data rate for point to point links",
                   DataRateValue (DataRate ("1000Mbps")),
                   MakeDataRateAccessor (&FatTreeHelper::m_eaRate),
                   MakeDataRateChecker ())
    .AddAttribute ("AcDataRate",
                   "The default data rate for point to point links",
                   DataRateValue (DataRate ("1000Mbps")),
                   MakeDataRateAccessor (&FatTreeHelper::m_acRate),
                   MakeDataRateChecker ())
    .AddAttribute ("HeDelay", "Transmission delay through the channel",
                   TimeValue (NanoSeconds (500)),
                   MakeTimeAccessor (&FatTreeHelper::m_heDelay),
                   MakeTimeChecker ())
    .AddAttribute ("EaDelay", "Transmission delay through the channel",
                   TimeValue (NanoSeconds (500)),
                   MakeTimeAccessor (&FatTreeHelper::m_eaDelay),
                   MakeTimeChecker ())
    .AddAttribute ("AcDelay", "Transmission delay through the channel",
                   TimeValue (NanoSeconds (500)),
                   MakeTimeAccessor (&FatTreeHelper::m_acDelay),
                   MakeTimeChecker ())
 ;

  return tid;
}

FatTreeHelper::FatTreeHelper()
{
  m_channelFactory.SetTypeId ("ns3::PointToPointChannel");
  m_ndFactory.SetTypeId ("ns3::PointToPointNetDevice");
}

FatTreeHelper::~FatTreeHelper()
{
}

/* Create the whole topology */
void
FatTreeHelper::Create()
{
    m_size = m_para->port/2;
    m_fibsize = m_para->fib_size;

	const unsigned N = m_size;
	const unsigned numST = 2*N;
	const unsigned numCore = N*N;
	const unsigned numAggr = numST * N;
	const unsigned numEdge = numST * N;
	const unsigned numHost = numEdge * N;
	const unsigned numTotal= numCore + numAggr + numEdge + numHost;

	/*
	 * Create nodes and distribute them into different node container.
	 * We create 5N^2+2N^3 nodes at a batch, where the first 4N^2 nodes are the
	 * edge and aggregation switches. In each of the 2N subtrees, first N nodes are
	 * edges and the remaining N are aggregations. The last N^2 nodes in the
	 * first 5N^2 nodes are core switches. The last 2N^3 nodes are end hosts.
	 */
	NS_LOG_LOGIC ("Creating fat-tree nodes.");
	m_node.Create(numTotal);

	for(unsigned j=0;j<2*N;j++) { // For every subtree
		for(unsigned i=j*2*N; i<=j*2*N+N-1; i++) { // First N nodes
		    Ptr<Node> node = m_node.Get(i);
            node->m_subtreeid = j;
            node->m_nodetype = 2;
            node->m_nodeid = i-j*2*N;
			m_edge.Add(node);
		}
		for(unsigned i=j*2*N+N; i<=j*2*N+2*N-1; i++) { // Last N nodes
			Ptr<Node> node = m_node.Get(i);
            node->m_subtreeid = j;
            node->m_nodetype = 1;
            node->m_nodeid = i-j*2*N-N;
			m_aggr.Add(node);
		}
	};
	for(unsigned i=4*N*N; i<5*N*N; i++) {
	    Ptr<Node> node = m_node.Get(i);
	    node->m_subtreeid = 0;
	    node->m_nodetype = 0;
	    node->m_nodeid = i-4*N*N;
		m_core.Add(node);
	};
	for(unsigned i=5*N*N; i<numTotal; i++) {
		Ptr<Node> node = m_node.Get(i);
        node->m_subtreeid = (i-5*N*N)/(N*N);
        node->m_nodetype = 3;
        node->m_nodeid = (i-5*N*N)%(N*N);
        m_host.Add(node);
	};

	/*
	 * Connect nodes by adding netdevice and set up IP addresses to them.
	 *
	 * The formula is as follows. We have a fat tree of parameter N, and
	 * there are six categories of netdevice, namely, (1) on host;
	 * (2) edge towards host; (3) edge towards aggr; (4) aggr towards
	 * edge; (5) aggr towards core; (6) on core. There are 2N^3 devices
	 * in each category which makes up to 12N^3 netdevices. The IP addrs
	 * are assigned in the subnet 10.0.0.0/8 with the 24 bits filled as
	 * follows: (Assume N is representable in 6 bits)
	 *
	 * Address         Scheme
	 *               | 7 bit      | 1 bit |  6 bit  | 2 bit | 8 bit   |
	 * Host (to edge)| Subtree ID |   0   | Edge ID |  00   | Host ID |
	 * Edge (to host)| Subtree ID |   0   | Edge ID |  10   | Host ID |
	 * Edge (to aggr)| Subtree ID |   0   | Edge ID |  11   | Aggr ID |
	 * Agg. (to edge)| Subtree ID |   0   | Edge ID |  01   | Aggr ID |
	 *
	 * Address         Scheme
	 *               | 7 bit  | 1 bit | 2 bit |  6 bit  | 8 bit   |
	 * Agg. (to core)| Subtree ID |   1   |  00   | Aggr ID | Core ID |
	 * Core (to aggr)| Subtree ID |   1   |  01   | Core ID | Aggr ID |
	 *
	 * All ID are numbered from 0 onward. Subtree ID is numbered from left to
	 * right according to the fat-tree diagram. Host ID is numbered from
	 * left to right within the same attached edge switch. Edge and aggr
	 * ID are numbered within the same subtree. Core ID is numbered with a
	 * mod-N from left to right according to the fat-tree diagram.
	 */
	NS_LOG_LOGIC ("Creating connections and set-up addresses.");
	Ipv4MixRoutingHelper mixHelper;
	InternetStackHelper internet;
	internet.SetRoutingHelper(mixHelper);
	internet.Install (m_node);

	// How to set a hash function to the hash routing????

	m_ndFactory.Set ("DataRate", DataRateValue(m_heRate));	/* Host to Edge */
	m_channelFactory.Set ("Delay", TimeValue(m_heDelay));
	for (unsigned j=0; j<numST; j++) { // For each subtree
		for(unsigned i=0; i<N; i++) { // For each edge
			for(unsigned m=0; m<N; m++) { // For each port of edge
				// Connect edge to host
				Ptr<Node> eNode = m_edge.Get(j*N+i);
				Ptr<Node> hNode = m_host.Get(j*N*N+i*N+m);
				NetDeviceContainer devices = InstallND(eNode, hNode);
				// Set routing for end host: Default route only
				//Ptr<HashRouting> hr = hashHelper.GetHashRouting(hNode->GetObject<Ipv4>());
				//hr->AddRoute(Ipv4Address(0U), Ipv4Mask(0U), 1);
				// Set IP address for end host
				uint32_t address = (((((((10<<7)+j)<<7)+i)<<2)+0x0)<<8)+m;
				hNode->m_hostaddress = address;
				AssignIP(devices.Get(1), address, m_hostIface);
				// Set routing for edge switch
				//hr = hashHelper.GetHashRouting(eNode->GetObject<Ipv4>());
				//hr->AddRoute(Ipv4Address(address), Ipv4Mask(0xFFFFFFFFU), m+1);
				// Set IP address for edge switch
				address = (((((((10<<7)+j)<<7)+i)<<2)+0x2)<<8)+m;
				AssignIP(devices.Get(0), address, m_edgeIface);
			};
		};
	};
	m_ndFactory.Set ("DataRate", DataRateValue(m_eaRate));	/* Edge to Aggr */
	m_channelFactory.Set ("Delay", TimeValue(m_eaDelay));
	for (unsigned j=0; j<numST; j++) { // For each subtree
		for(unsigned i=0; i<N; i++) { // For each edge
			for(unsigned m=0; m<N; m++) { // For each aggregation
				// Connect edge to aggregation
				Ptr<Node> aNode = m_aggr.Get(j*N+m);
				Ptr<Node> eNode = m_edge.Get(j*N+i);
				NetDeviceContainer devices = InstallND(aNode, eNode);
				// Set IP address for aggregation switch
				uint32_t address = (((((((10<<7)+j)<<7)+i)<<2)+0x1)<<8)+m;
				AssignIP(devices.Get(0), address, m_aggrIface);
				// Set routing for aggregation switch
				//Ptr<HashRouting> hr = hashHelper.GetHashRouting(aNode->GetObject<Ipv4>());
				//hr->AddRoute(Ipv4Address(address & 0xFFFFFC00U), Ipv4Mask(0xFFFFFC00U), i+1);
				// Set IP address for edge switch
				address = (((((((10<<7)+j)<<7)+i)<<2)+0x3)<<8)+m;
				AssignIP(devices.Get(1), address, m_edgeIface);
			} ;
		};
	};
	m_ndFactory.Set ("DataRate", DataRateValue(m_acRate));	/* Aggr to Core */
	m_channelFactory.Set ("Delay", TimeValue(m_acDelay));
	for(unsigned j=0; j<numST; j++) { // For each subtree
		for(unsigned i=0; i<N; i++) { // For each aggr
			for(unsigned m=0; m<N; m++) { // For each port of aggr
				// Connect aggregation to core
				Ptr<Node> cNode = m_core.Get(i*N+m);
				Ptr<Node> aNode = m_aggr.Get(j*N+i);
				NetDeviceContainer devices = InstallND(cNode, aNode);
				// Set IP address for aggregation switch
				uint32_t address = (((((((10<<7)+j)<<3)+0x4)<<6)+i)<<8)+m;
				AssignIP(devices.Get(1), address, m_aggrIface);
				// Set routing for core switch
				//Ptr<HashRouting> hr = hashHelper.GetHashRouting(cNode->GetObject<Ipv4>());
				//hr->AddRoute(Ipv4Address(address & 0xFFFE0000U), Ipv4Mask(0xFFFE0000U), j+1);
				// Set IP address for core switch
				address = (((((((10<<7)+j)<<3)+0x5)<<6)+m)<<8)+i;
				AssignIP(devices.Get(0), address, m_coreIface);
			};
		};
	};

//Create the routing related things, oh, and the disk.

	for(unsigned i = 0; i< numTotal; i++) {
	    Ptr<Node> cur_node = m_node.Get(i);
		Ptr<MixRouting> hr = mixHelper.GetMixRouting(cur_node->GetObject<Ipv4>());

		if (!m_para->enable_cache)
		{
            hr->DisableContentRoute();
		}

		hr->SetNode(cur_node);
		if (cur_node->m_nodetype != 3)
		{
		    hr->CreateContentFib(m_fibsize);
		}
	}
} // FatTreeHelper::Create()

void
FatTreeHelper::AssignIP (Ptr<NetDevice> c, uint32_t address, Ipv4InterfaceContainer &con)
{
	NS_LOG_FUNCTION_NOARGS ();

	Ptr<Node> node = c->GetNode ();
	NS_ASSERT_MSG (node, "FatTreeHelper::AssignIP(): Bad node");

	Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
	NS_ASSERT_MSG (ipv4, "FatTreeHelper::AssignIP(): Bad ipv4");

	int32_t ifIndex = ipv4->GetInterfaceForDevice (c);
	if (ifIndex == -1) {
		ifIndex = ipv4->AddInterface (c);
	};
	NS_ASSERT_MSG (ifIndex >= 0, "FatTreeHelper::AssignIP(): Interface index not found");

	Ipv4Address addr(address);
	Ipv4InterfaceAddress ifaddr(addr, 0xFFFFFFFF);
	ipv4->AddAddress (ifIndex, ifaddr);
	ipv4->SetMetric (ifIndex, 1);
	ipv4->SetUp (ifIndex);
	con.Add (ipv4, ifIndex);
	Ipv4AddressGenerator::AddAllocated (addr);
}

NetDeviceContainer
FatTreeHelper::InstallND (Ptr<Node> a, Ptr<Node> b)
{
    PointToPointHelper p2php;
    p2php.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
    p2php.SetChannelAttribute ("Delay", StringValue ("500ns"));
    return p2php.Install(a, b);
}

void
FatTreeHelper::SetContentAttributes(int fibsize)
{
    m_fibsize = fibsize;
}

}//namespace
