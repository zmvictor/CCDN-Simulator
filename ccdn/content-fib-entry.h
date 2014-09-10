#ifndef CONTENT_FIB_ENTRY_H
#define CONTENT_FIB_ENTRY_H

#include <inttypes.h>
#include "ns3/type-id.h"
#include "ns3/object-factory.h"

namespace ns3
{
class ContentFibEntry : public Object
{

public:

    static TypeId GetTypeId (void) {return TypeId ("ns3::ContentFibEntry");};
    ContentFibEntry(const int ndnum, uint64_t content, uint32_t version);
    ~ContentFibEntry() {delete m_ndarray;};

    //Get, set the content, version;
    uint64_t GetContent() {return m_content;};
    uint32_t GetVersion() {return m_version;};
    void SetContent(uint64_t content) {m_content = content;};
    void SetVersion(uint32_t version) {m_version = version;};

    //Set the nd. Return false if the index is out of bound.
    bool SetNDArray(int nd);
    //Reset the nd. Return false if the index is out of bound.
    bool ResetNDArray(int nd);
    //Get the number of set nd;
    int GetSetNDNum();
    //Get a set nd. If there are multiple then return a random one.
    int GetRandomSetND();


private:

	int		    m_ndnum;
	uint64_t	m_content;
	uint32_t	m_version;
	bool		*m_ndarray;

};
};


#endif
