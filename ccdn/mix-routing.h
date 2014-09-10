// -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*-
//
// Copyright (c) 2009 New York University
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//

#ifndef MIX_ROUTING_H
#define MIX_ROUTING_H

#include <list>
#include <set>
#include <stdint.h>
#include "ns3/ipv4-address.h"
#include "ns3/ptr.h"
#include "ns3/ipv4.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ref-count-base.h"

#include "content-fib.h"

namespace ns3 {

class Packet;
class NetDevice;
class Ipv4Interface;
class Ipv4Address;
class Ipv4Header;
class Ipv4RoutingTableEntry;
class Ipv4MulticastRoutingTableEntry;
class Node;

// Class for hash-based routing logic
class MixRouting : public Ipv4RoutingProtocol
{
public:
	static TypeId GetTypeId (void);
	MixRouting ();
	virtual ~MixRouting ();

	// Functions defined in Ipv4RoutingProtocol
	virtual Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p,
			const Ipv4Header &header,
			Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);
	virtual bool RouteInput  (Ptr<const Packet> p,
			const Ipv4Header &header, Ptr<const NetDevice> idev,
			UnicastForwardCallback ucb, MulticastForwardCallback mcb,
			LocalDeliverCallback lcb, ErrorCallback ecb);
	virtual void NotifyInterfaceUp (uint32_t interface) {};
	virtual void NotifyInterfaceDown (uint32_t interface) {};
	virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address) {};
	virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address) {};
	virtual void PrintRoutingTable(Ptr<OutputStreamWrapper> wrapper) const {};
	virtual void SetIpv4 (Ptr<Ipv4> ipv4);
	virtual void SetNode (Ptr<Node> node);

	void CreateContentFib (int size);
	void DisableContentRoute() {m_content_route = false;};

protected:

    //Decide the nfd by pure ip. If there is multiple available then the result will be hashed from dst and src.
    int IpHashLookup(uint32_t dst, uint32_t src);
	//Decide the nfd by content. This is only for cmp packets.
	int ContentLookup(Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev);
	int ContentHash(uint64_t content);
	int FalseContentHash(uint64_t content);
	bool DeviceDown(int nd);

    Ptr<Node> m_node;   // Hook to the node (you can visit the position of the node)
	Ptr<Ipv4> m_ipv4;	// Hook to the Ipv4 object of this node
	ContentFib *m_fib;
	int	NDevice;		// The number of devices (not including local device)

	bool m_content_route;
};

} // Namespace ns3

#endif /* HASH_ROUTING_H */
