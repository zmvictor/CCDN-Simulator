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
#ifndef GLOBAL_CONTENT_MANAGER_H
#define GLOBAL_CONTENT_MANAGER_H

#include <vector>

#include "ns3/type-id.h"
#include "ns3/node-container.h"
#include "ns3/nstime.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/net-device-container.h"
#include "ns3/object-factory.h"
#include "ns3/ptr.h"
#include "ns3/socket.h"

#include "parameter.h"
#include "content-cache.h"
#include "fat-tree-helper.h"
#include "task-recorder.h"

namespace ns3 {


struct ContentTableEntry{

    uint64_t content;
    uint32_t version;
    unsigned numHost;
    unsigned *host;

};


class GlobalContentManager : public Object
{
public:
	static TypeId GetTypeId (void) {return TypeId ("ns3::GlobalContentManager");};
	GlobalContentManager();
	virtual ~GlobalContentManager();
	void SetPara(Parameter *para) {m_para = para;};
	void Create(void);

    //The following functions is to operate the content table, or visit it.
	void CreateContent(uint64_t content, unsigned numHost, unsigned *hosts);
	void UpdateContent(uint64_t content);
	void RemoveContent(uint64_t content);
	bool HasContent(unsigned host, uint64_t content);
	ContentTableEntry *GetContent(uint64_t content);
	//The closest one will be returned. If there are multiple, then randomly return one.
	unsigned GetContentLocation(unsigned local, uint64_t content, uint32_t &version);
	unsigned GetRandomClosestLocation(unsigned local, unsigned numHost, unsigned *host);
	unsigned GetHostDistance(unsigned a, unsigned b);

	//The following functions is to operate the cache.
	bool AddCache(unsigned host, uint64_t content, uint32_t version);
	bool RemoveCache(unsigned host, uint64_t content);
	bool HasCache(unsigned host, uint64_t content, uint32_t version);

	//The following functions is to invoke an file access operation;
	void RequireFile(unsigned host, uint64_t content);
	void ReloadRequire(unsigned host, uint64_t content);
	void ReviewFile();

	//The following functions is for cmp
	void SendCmpPacket(Ptr<Socket> socket, uint64_t content, uint32_t version, uint8_t type, Ipv4Address dstaddr);
	void RecvCmpPacket(Ptr<Socket> socket);
	Ptr<Socket> GetCmpSocket(int index);
	void TransferContent(unsigned local, Ipv4Address dst, uint64_t content, uint32_t version);
	void InvokeTransferFinished(unsigned local, uint64_t content, uint32_t version, Ipv4Address dstaddr);


	//The following functions should be triggered when a file access is invoked, or the file is found.

private:

	unsigned GetHostIDFromPtr(Ptr<Node> host);
	unsigned GetHostIDFromAddress(Ipv4Address addr);

    std::vector<ContentTableEntry*> *m_table;
    ContentCache* *m_cache;
    Ptr<Socket> *m_cmpSockets;
    FatTreeHelper *helper;
    TaskRecorder *recorder;
    unsigned N;
    Parameter *m_para;

    bool enable_cache;



    static unsigned const m_cmpport = 2013;
    static unsigned const m_dataport = 2014;
    static unsigned const m_datasize = 1000000;


};
};

#endif
