/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008, 2009 Polytechnic Institute of NYU, New York University
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
 * Author: Chang Liu <cliu02@students.poly.edu> and
 *         Adrian S. Tam <adrian.sw.tam@gmail.com>
 */

#include "ns3/log.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/net-device.h"
#include "ns3/ipv4-route.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/channel.h"
#include "ns3/node.h"
#include "ns3/random-variable.h"

#include "fat-tree-helper.h"
#include "mix-routing.h"

NS_LOG_COMPONENT_DEFINE ("MixRouting");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (MixRouting);

inline unsigned HostAddrToSubtree(uint32_t addr)
{
	return ((addr & 0x00FF0000) >> 17);
};

inline unsigned HostAddrToEdge(uint32_t addr)
{
	return ((addr & 0x0000FF00) >> 10);
};

inline unsigned HostAddrToPort(uint32_t addr)
{
	return (addr & 0x000000FF);
};

TypeId
MixRouting::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MixRouting")
    .SetParent<Ipv4RoutingProtocol> ();
  return tid;
}

MixRouting::MixRouting ()
{
  m_content_route = true;
  NS_LOG_FUNCTION_NOARGS ();
}

MixRouting::~MixRouting ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

Ptr<Ipv4Route>
MixRouting::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
{
	// Hash-routing is for unicast destination only
	Ipv4Address a = header.GetDestination();
	if (a.IsMulticast() || a.IsBroadcast()) {
		NS_LOG_LOGIC("Non-unicast destination is not supported");
		sockerr = Socket::ERROR_NOROUTETOHOST;
		return 0;
	};
	// Check for a route, and construct the Ipv4Route object
	sockerr = Socket::ERROR_NOTERROR;
	int iface = IpHashLookup(header.GetDestination().Get(), header.GetSource().Get());
    //uint32_t iface = Lookup(GetTuple(p, header));
   	Ptr<NetDevice> dev = m_ipv4->GetNetDevice(iface); // Convert output port to device
   	Ptr<Channel> channel = dev->GetChannel(); // Channel used by the device
	uint32_t otherEnd = (channel->GetDevice(0)==dev)?1:0; // Which end of the channel?
	Ptr<Node> nextHop = channel->GetDevice(otherEnd)->GetNode(); // Node at other end
   	uint32_t nextIf = channel->GetDevice(otherEnd)->GetIfIndex(); // Iface num at other end
	Ipv4Address nextHopAddr = nextHop->GetObject<Ipv4>()->GetAddress(nextIf,0).GetLocal(); // Addr of other end
	Ptr<Ipv4Route> r = Create<Ipv4Route> ();
	r->SetOutputDevice(m_ipv4->GetNetDevice(iface));
	r->SetGateway(nextHopAddr);
	r->SetSource(m_ipv4->GetAddress(iface,0).GetLocal());
	r->SetDestination(a);
	return r;
}

bool
MixRouting::RouteInput  (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
		UnicastForwardCallback ucb, MulticastForwardCallback mcb,
		LocalDeliverCallback lcb, ErrorCallback ecb)
{
	//NS_LOG_FUNCTION (this << p << header << header.GetSource () << header.GetDestination () << idev);
	// Check if input device supports IP
	NS_ASSERT (m_ipv4->GetInterfaceForDevice (idev) >= 0);
	uint32_t iif = m_ipv4->GetInterfaceForDevice (idev);
	Ipv4Address a = header.GetDestination();


	// Hash-routing is for unicast destination only
	if (a.IsMulticast() || a.IsBroadcast()) {
		NS_LOG_LOGIC("Non-unicast destination is not supported");
		return false;
	};

	// Check if the destination is local

	for (uint32_t j = 0; j < m_ipv4->GetNInterfaces(); j++)
    {
        for (uint32_t i = 0; i < m_ipv4->GetNAddresses(j); i++)
        {
            Ipv4InterfaceAddress iaddr = m_ipv4->GetAddress (j, i);
            Ipv4Address addr = iaddr.GetLocal ();
            if (addr.IsEqual (a))
            {
                if (j == iif)
                {
                    NS_LOG_LOGIC ("For me (destination " << addr << " match)");
                }
                else
                {
                    NS_LOG_LOGIC ("For me (destination " << addr << " match) on another interface " << a);
                }
                lcb (p, header, iif);
                return true;
            }
        }
    }

    if (m_node->m_nodetype == 3 && header.GetProtocol() == 0x11U)   //This is an host address, always accept udp cmp packet
    {
        NS_LOG_DEBUG ("For me: " << header.GetSource() <<" cmp packet to " << a << ", but I'm " << Ipv4Address(m_node->m_hostaddress));
        Ipv4Header ip = header;
        ip.SetDestination(Ipv4Address(m_node->m_hostaddress));
        lcb (p, ip, iif);
        return true;
    }

	// Check if input device supports IP forwarding
	if (m_ipv4->IsForwarding (iif) == false) {
		NS_LOG_LOGIC ("Forwarding disabled for this interface");
		ecb (p, header, Socket::ERROR_NOROUTETOHOST);
		return false;
	}
	int outPort;
	// Next, try to find a route
	if (header.GetProtocol() == 0x11U && m_content_route)
	{
	    outPort = ContentLookup(p, header, idev);
	}
	else
	{
	    outPort = IpHashLookup(header.GetDestination().Get(), header.GetSource().Get());
	}

	NS_LOG_LOGIC ("Forwarding to " << outPort);
   	Ptr<NetDevice> dev = m_ipv4->GetNetDevice(outPort); // Convert output port to device
   	Ptr<Channel> channel = dev->GetChannel(); // Channel used by the device
	uint32_t otherEnd = (channel->GetDevice(0)==dev)?1:0; // Which end of the channel?
	Ptr<Node> nextHop = channel->GetDevice(otherEnd)->GetNode(); // Node at other end
   	uint32_t nextIf = channel->GetDevice(otherEnd)->GetIfIndex(); // Iface num at other end
	Ipv4Address nextHopAddr = nextHop->GetObject<Ipv4>()->GetAddress(nextIf,0).GetLocal(); // Addr of other end
	Ptr<Ipv4Route> r = Create<Ipv4Route> ();
	r->SetOutputDevice(m_ipv4->GetNetDevice(outPort));
	r->SetGateway(nextHopAddr);
	r->SetSource(m_ipv4->GetAddress(outPort,0).GetLocal());
	r->SetDestination(a);
	//NS_LOG_LOGIC ("Found unicast destination- calling unicast callback");
	ucb(r, p, header);
	return true;
}

void
MixRouting::SetIpv4 (Ptr<Ipv4> ipv4)
{
	NS_LOG_FUNCTION(this << ipv4);
	NS_ASSERT (m_ipv4 == 0 && ipv4 != 0);
	m_ipv4 = ipv4;
}

void
MixRouting::SetNode (Ptr<Node> node)
{
    NS_LOG_FUNCTION(this << node);
	NS_ASSERT (m_node == 0 && node != 0);
	m_node = node;
	NDevice = m_ipv4->GetNInterfaces()-1;
}

void

MixRouting::CreateContentFib (int size)

{

	m_fib = new ContentFib(m_node->GetNDevices(), size);

}

int
MixRouting::IpHashLookup(uint32_t dst, uint32_t src)
{
    if (m_node == 0)
    {
        NS_LOG_LOGIC("I dunno why, but my node is empty!");
        return 1;
    }

    switch (m_node->m_nodetype)
    {
        case 0:
            return ((dst>>17)&0x7fU) + 1;
        case 1: //Aggr
            if (((dst>>17)&0x7fU) == m_node->m_subtreeid)
                return (dst>>10)%0x40U + 1;
            else
                return NDevice/2 + 1 + src%0x100U;
        case 2: //Edge
            if (((dst>>10)&0x3fbfU) == (m_node->m_subtreeid<<7) + m_node->m_nodeid)
                return dst%0x100U + 1;
            else
                return NDevice/2 + 1 + dst%0x100U;
        case 3: //Host
            return 1;
    }
    return -1;
}

int
MixRouting::ContentLookup(Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev)
{
    CmpHeader *cmp_header = new CmpHeader;
    uint8_t buffer[sizeof(CmpHeader)+8];
    //p->CopyData((uint8_t*)cmp_header, sizeof(CmpHeader));
    p->CopyData(buffer, sizeof(CmpHeader)+8);
    memcpy(cmp_header, buffer+8, sizeof(CmpHeader));
	uint8_t		p_type = cmp_header->type;
	uint64_t	p_content = cmp_header->content;
	uint32_t	p_version = cmp_header->version;
	delete cmp_header;
	int			iintf = m_ipv4->GetInterfaceForDevice(idev);
	int			fintf = 0;

	NS_LOG_INFO ("Forwarding for cmp packet with content " << p_content << ", version " << p_version << " and type " << p_type << " from " << iintf);

	switch (p_type)
	{
	case 0: //Normal
        return IpHashLookup(header.GetDestination().Get(), header.GetSource().Get());
	case 1: //Request
		//For any incoming request, first delete its ND. They will never be put there.
		m_fib->RemoveFibND(p_content, p_version, iintf);

		fintf = IpHashLookup(header.GetDestination().Get(), header.GetSource().Get());
		if (DeviceDown(fintf))
		{
		    m_fib->InsertFibND(p_content, p_version, fintf);
		}

		fintf = m_fib->GetForwardingND(p_content, p_version);
		if (fintf != -1)
		{
			return fintf;
		}
		//No hit. so we have to content hash to go upward, or pure ip.
		if (DeviceDown(iintf) && m_node->m_nodetype != 0)
		{
			return ContentHash(p_content);	//Content hash up
		}
		else
		{
			fintf = IpHashLookup(header.GetDestination().Get(), header.GetSource().Get());	//Pure ip
			m_fib->InsertFibND(p_content, p_version, fintf);
			return fintf;
		}
	case 2: //Reply
	case 3: //Finish
		//It means 'I'v got what you need'. Better tell everyone about this.
		if (DeviceDown(iintf))
		{
			m_fib->InsertFibND(p_content, p_version, iintf);
			m_fib->FreshFibEntry(p_content, p_version);
			if (m_node->m_nodetype != 0)
			{
				return ContentHash(p_content);
			}
		}
		return IpHashLookup(header.GetDestination().Get(), header.GetSource().Get());
	case 4: //Reject
		//'I aint got it. Don't make the table trick anyone nomore.
		if (DeviceDown(iintf))
		{
			if (m_fib->RemoveFibND(p_content, p_version, iintf) && m_node->m_nodetype != 0)
			{
				return fintf = ContentHash(p_content);
			}
			else if (m_node->m_nodetype != 0)
			{
			    return fintf = FalseContentHash(p_content);
			}
		}
		return IpHashLookup(header.GetDestination().Get(), header.GetSource().Get());
	}
	return -1;
}

int
MixRouting::ContentHash(uint64_t content)
{
	if (m_node->m_nodetype == 2)		//Edge
	{
		return (content % (NDevice/2)) + NDevice/2 + 1;
	}
	else if (m_node->m_nodetype == 1)	//Aggr
	{
		return ((content/(NDevice/2)) % (NDevice/2)) + NDevice/2 + 1;
	}
	return 1;
}
int
MixRouting::FalseContentHash(uint64_t content)
{
    return (ContentHash(content) % (NDevice/2)) + NDevice/2 + 1;
}
bool
MixRouting::DeviceDown(int nd)
{
	return nd <= NDevice/2;
}


}//namespace ns3
