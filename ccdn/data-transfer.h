#ifndef DATA_TRANSFER_H
#define DATA_TRANSFER_H

#include <inttypes.h>
#include "ns3/ptr.h"
#include "ns3/socket.h"
#include "ns3/inet-socket-address.h"
#include "ns3/object-factory.h"

#include "global-content-manager.h"

namespace ns3
{
class DataTransfer : public Object
{

public:

    static TypeId GetTypeId (void) {return TypeId ("ns3::DataTransfer");};
    DataTransfer(GlobalContentManager *manager, uint32_t totalBytes, unsigned local, Ptr<Socket> socket, Ipv4Address dst, uint16_t port);
    ~DataTransfer() {};

    void SetContent(uint64_t content, uint32_t version) {m_content = content; m_version = version;};
    void Start();
    void StartFlow ();
    void WriteUntilBufferFull (Ptr<Socket> local_socket, uint32_t);

private:

    uint64_t m_content;
    uint32_t m_version;
    unsigned m_local;

    uint32_t totalTxBytes;
    uint32_t currentTxBytes;
    static const uint32_t writeSize = 1040;
    uint8_t data[writeSize];
    bool finished;

    Ptr<Socket> m_socket;
    Ipv4Address m_dst;
    uint16_t m_port;
    GlobalContentManager *father;

};
};


#endif
