/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 New York University
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

#include <stdlib.h>
#include "ns3/inet-socket-address.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/application.h"

#include "data-transfer.h"
#include "content-cache.h"
#include "global-content-manager.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GlobalContentManager");

NS_OBJECT_ENSURE_REGISTERED (GlobalContentManager);

GlobalContentManager::GlobalContentManager()
{
    return;
}

GlobalContentManager::~GlobalContentManager()
{

}

void
GlobalContentManager::Create()
{
    N = m_para->port/2;
    enable_cache = m_para->enable_cache;
    helper = new FatTreeHelper();
    helper->SetPara(m_para);
    helper->Create();

    recorder = new TaskRecorder(m_para->filename);

    m_table = new std::vector<ContentTableEntry*>(0);
	unsigned numHost = helper->HostNodes().GetN();

	m_cache = new ContentCache*[numHost];
	for(unsigned i = 0; i < numHost; i++)
    {
        m_cache[i] = new ContentCache(m_para->cache_size);
    }

    //Create cmp sockets and data receive socket for each host
    m_cmpSockets = new Ptr<Socket>[numHost];
    for(unsigned i = 0; i < numHost; i++)
    {
        Ptr<Node> cur_node = helper->HostNodes().Get(i);
        m_cmpSockets[i] = Socket::CreateSocket (cur_node, TypeId::LookupByName ("ns3::UdpSocketFactory"));
	    InetSocketAddress dst = InetSocketAddress (Ipv4Address(cur_node->m_hostaddress), m_cmpport);
	    m_cmpSockets[i]->Bind(dst);
	    m_cmpSockets[i]->SetRecvCallback(MakeCallback(&ns3::GlobalContentManager::RecvCmpPacket, this));

	    InetSocketAddress dst2 = InetSocketAddress (Ipv4Address(cur_node->m_hostaddress), m_dataport);
	    PacketSinkHelper sink ("ns3::TcpSocketFactory", dst2);
	    ApplicationContainer apps = sink.Install (cur_node);
	    apps.Start (Seconds (0.0));
    }
}

void
GlobalContentManager::CreateContent(uint64_t content, unsigned numHost, unsigned *host)
{
    RemoveContent(content);
    NS_LOG_LOGIC("Create global content: "<<content<<", currently "<<m_table->size()<<" contents.");
    ContentTableEntry *entry = new ContentTableEntry;
    entry->content = content;
    entry->version = 0;
    entry->numHost = numHost;
    entry->host = host;
    m_table->insert(m_table->begin(), entry);
}
void
GlobalContentManager::UpdateContent(uint64_t content)
{
    ContentTableEntry *entry = GetContent(content);
    if (entry != 0)
    {
        entry->version ++;
        NS_LOG_LOGIC("Update global content: "<<content<<" from version "<<entry->version-1<<" to version "<<entry->version);
    }
    else
    {
        NS_LOG_LOGIC("Update global content: "<<content<<" not found");
    }
}
void
GlobalContentManager::RemoveContent(uint64_t content)
{
    NS_LOG_LOGIC("Remove global content: "<<content<<", currently "<<m_table->size()<<" contents.");
    for (std::vector<ContentTableEntry*>::iterator iter = m_table->begin(); iter != m_table->end(); iter ++)
    {
        if ((*iter)->content == content)
        {
            ContentTableEntry *entry = *iter;
            m_table->erase(iter);
            delete entry;
            return;
        }
    }
}
ContentTableEntry*
GlobalContentManager::GetContent(uint64_t content)
{
    for (std::vector<ContentTableEntry*>::iterator iter = m_table->begin(); iter != m_table->end(); iter ++)
    {
        if ((*iter)->content == content)
        {
            return *iter;
        }
    }
    return 0;
}
bool
GlobalContentManager::HasContent(unsigned host, uint64_t content)
{
	ContentTableEntry *entry = GetContent(content);
    if (entry == 0)
    {
        return false;
    }

	for (unsigned i = 0; i < entry->numHost; i++)
	{
		if (entry->host[i] == host)
		{
			return true;
		}
	}

	return false;
}




unsigned
GlobalContentManager::GetContentLocation(unsigned local, uint64_t content, uint32_t &version)
{
    ContentTableEntry *entry = GetContent(content);
    if (entry == 0)
    {
        return 0;
    }
    version = entry->version;
    NS_LOG_DEBUG(content<<" hit on "<<GetRandomClosestLocation(local, entry->numHost, entry->host));
	return GetRandomClosestLocation(local, entry->numHost, entry->host);
}
unsigned
GlobalContentManager::GetRandomClosestLocation(unsigned local, unsigned numHost, unsigned *host)
{
	unsigned min_distance = 8;
	unsigned min_host = -1;
	int count = 1;
	for (unsigned i = 0; i < numHost; i++)
	{
		unsigned current_distance = GetHostDistance(local, host[i]);
		if (current_distance < min_distance)
		{
			min_distance = current_distance;
			min_host = host[i];
			count = 1;
		}
		else if (current_distance == min_distance)
		{
			count ++;
			if (rand()%count == 0)
			{
				min_host = host[i];
			}
		}
	}
	return min_host;
}
unsigned
GlobalContentManager::GetHostDistance(unsigned a, unsigned b)
{
	Ptr<Node> node_a = helper->HostNodes().Get(a);
	Ptr<Node> node_b = helper->HostNodes().Get(b);
	if (node_a->m_subtreeid != node_b->m_subtreeid)
	{
		return 6;
	}
	else if (node_a->m_hostaddress>>8 != node_b->m_hostaddress>>8)
	{
		return 4;
	}
	else if (node_a->m_nodeid != node_b->m_nodeid)
	{
		return 2;
	}
	else
	{
		return 0;
	}
}



bool
GlobalContentManager::AddCache(unsigned host, uint64_t content, uint32_t version)
{
	return m_cache[host]->AddCache(content, version);
}
bool
GlobalContentManager::RemoveCache(unsigned host, uint64_t content)
{
	return m_cache[host]->RemoveCache(content);
}
bool
GlobalContentManager::HasCache(unsigned host, uint64_t content, uint32_t version)
{
	return m_cache[host]->HasCache(content, version);
}


void
GlobalContentManager::RequireFile(unsigned host, uint64_t content)
{
    recorder->RegisterTask(host, content);
    ReloadRequire(host, content);
}
void
GlobalContentManager::ReloadRequire(unsigned host, uint64_t content)
{
    unsigned version = 0;
    unsigned remote = GetContentLocation(host, content, version);
    SendCmpPacket(m_cmpSockets[host], content, version, 1, Ipv4Address(helper->HostNodes().Get(remote)->m_hostaddress));
}
void
GlobalContentManager::ReviewFile()
{
    recorder->ReviewTask();
}


void
GlobalContentManager::SendCmpPacket(Ptr<Socket> socket, uint64_t content, uint32_t version, uint8_t type, Ipv4Address dstaddr)
{
    CmpHeader *header = new CmpHeader;
    header->content = content;
    header->version = version;
    header->type = type;
    Ptr<Packet> p = ns3::Create<Packet>((uint8_t*)header, sizeof(CmpHeader));
    socket->SendTo (p, 0, InetSocketAddress (dstaddr, m_cmpport));
    delete header;
}

/*
* This is the representation of CMP service.
* Upon receiving an CMP packet, what will you do? I guess you will..
*/
void
GlobalContentManager::RecvCmpPacket(Ptr<Socket> socket)
{
	Address from;
    Ptr<Packet> packet = socket->RecvFrom (from);
    packet->RemoveAllPacketTags ();
    packet->RemoveAllByteTags ();
    CmpHeader *header = new CmpHeader;
    packet->CopyData((uint8_t*)header, sizeof(CmpHeader));
	unsigned local = GetHostIDFromPtr(socket->GetNode());

	NS_LOG_LOGIC("Global receive cmp packet from " << InetSocketAddress::ConvertFrom(from).GetIpv4() <<" to " << Ipv4Address(socket->GetNode()->m_hostaddress) << " with type " << header->type);

	switch (header->type)
	{
	case 0: // Transfer finish. You send a finish and update your cache.
	    AddCache(local, header->content, header->version);
        SendCmpPacket(socket, header->content, header->version, 3, InetSocketAddress::ConvertFrom(from).GetIpv4());
		break;
	case 1: // Request. Check if you have the content. If do, reply; otherwise reject.
		//First check if there is the content on the disk or on the cache
		if (HasContent(local, header->content) || (HasCache(local, header->content, header->version) && enable_cache))
		{
			SendCmpPacket(socket, header->content, header->version, 2, InetSocketAddress::ConvertFrom(from).GetIpv4());		//Send a reply packet
			recorder->UpdateTask(GetHostIDFromAddress(InetSocketAddress::ConvertFrom(from).GetIpv4()), local, header->content);
			TransferContent(local, InetSocketAddress::ConvertFrom(from).GetIpv4(), header->content, header->version);
		}
		else
		{
			SendCmpPacket(socket, header->content, header->version, 4, InetSocketAddress::ConvertFrom(from).GetIpv4());		//This is an reject packet
		}
		break;
	case 2: //Accept. Currently, Nothing to be done here.
		break;
	case 3: //Finish. Currently, nothing to be done.
		break;
	case 4: //Reject. You need to resend.
        uint32_t version;
		SendCmpPacket(socket, header->content, header->version, 1, Ipv4Address(helper->HostNodes().Get(GetContentLocation(local, header->content, version))->m_hostaddress));
	}

    delete header;
}
void
GlobalContentManager::TransferContent(unsigned local, Ipv4Address dst, uint64_t content, uint32_t version)
{
    Ptr<Socket> sendSocket = Socket::CreateSocket(helper->HostNodes().Get(local), TypeId::LookupByName ("ns3::TcpSocketFactory"));
    //sendSocket->TraceConnectWithoutContext("CongestionWindow", MakeCallback(&cwndTrace));
    sendSocket->Bind();
    NS_LOG_LOGIC("From "<<Ipv4Address(helper->HostNodes().Get(local)->m_hostaddress)<<" to "<<dst<<": Transfer started.");
    DataTransfer *transfer = new DataTransfer(this, m_datasize, local, sendSocket, dst, m_dataport);
    transfer->SetContent(content, version);
    transfer->Start();
}
void
GlobalContentManager::InvokeTransferFinished(unsigned local, uint64_t content, uint32_t version, Ipv4Address dstaddr)
{
    NS_LOG_LOGIC("From "<<Ipv4Address(helper->HostNodes().Get(local)->m_hostaddress)<<" to "<<dstaddr<<": Transfer finished.");
    recorder->FinishTask(GetHostIDFromAddress(dstaddr), content);
    SendCmpPacket(m_cmpSockets[local], content, version, 0, dstaddr);
}

unsigned
GlobalContentManager::GetHostIDFromPtr(Ptr<Node> host)
{
	unsigned numHost = helper->HostNodes().GetN();
	for (unsigned i = 0; i < numHost; i++)
	{
		if (host == helper->HostNodes().Get(i))
		{
			return i;
		}
	}
	return -1;
}
unsigned
GlobalContentManager::GetHostIDFromAddress(Ipv4Address addr)
{
    return addr.Get()%0x100U + ((addr.Get()>>10)%0x80U)*N + ((addr.Get()>>17)%0x80U)*N*N;
}
Ptr<Socket>
GlobalContentManager::GetCmpSocket(int index)
{
    return m_cmpSockets[index];
}

};

