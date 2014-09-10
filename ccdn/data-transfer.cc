#include <inttypes.h>
#include "ns3/log.h"
#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/object-factory.h"

#include "data-transfer.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("DataTransfer");

NS_OBJECT_ENSURE_REGISTERED (DataTransfer);

DataTransfer::DataTransfer(GlobalContentManager *manager, uint32_t totalBytes, unsigned local, Ptr<Socket> socket, Ipv4Address dst, uint16_t port)
{
    father = manager;
    totalTxBytes = totalBytes;
    currentTxBytes = 0;
    m_socket = socket;
    m_dst = dst;
    m_port = port;
    finished = false;
    m_local = local;
}

void
DataTransfer::Start()
{
    Simulator::ScheduleNow(&ns3::DataTransfer::StartFlow, this);
}

void
DataTransfer::StartFlow ()
{
    m_socket->Connect (InetSocketAddress(m_dst, m_port));
    m_socket->SetSendCallback (MakeCallback (&ns3::DataTransfer::WriteUntilBufferFull, this));
    WriteUntilBufferFull (m_socket, m_socket->GetTxAvailable ());
}

void
DataTransfer::WriteUntilBufferFull (Ptr<Socket> local_socket, uint32_t txSpace)
{
    while (currentTxBytes < totalTxBytes && local_socket->GetTxAvailable () > 0)
    {
        uint32_t left = totalTxBytes - currentTxBytes;
        uint32_t dataOffset = currentTxBytes % writeSize;
        uint32_t toWrite = writeSize - dataOffset;
        toWrite = std::min (toWrite, left);
        toWrite = std::min (toWrite, local_socket->GetTxAvailable ());
        int amountSent = local_socket->Send (&data[dataOffset], toWrite, 0);
        if(amountSent < 0)
        {
            // we will be called again when new tx space becomes available.
            return;
        }
        currentTxBytes += amountSent;
    }
    if (!finished && currentTxBytes >= totalTxBytes)
    {
        finished = true;
        father->InvokeTransferFinished(m_local, m_content, m_version, m_dst);
    }
    local_socket->Close ();
}

};
