#include <inttypes.h>
#include <list>
#include "ns3/log.h"
#include "content-fib.h"
#include "content-fib-entry.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("ContentFib");

NS_OBJECT_ENSURE_REGISTERED (ContentFib);

ContentFib::ContentFib(int ndnum, int entrysize)
{
    m_ndnum = ndnum;
    m_entrysize = entrysize;
    m_fib = new std::list<ContentFibEntry*>(0);
}

bool
ContentFib::InsertFibND(uint64_t content, uint32_t version, int nd)
{
    ContentFibEntry *entry = GetEntry(content, version);
    if (entry == 0)
    {
        InsertFibEntry(content, version);
    }

    entry = GetEntry(content, version);
    if (entry->GetVersion() > version)
    {
        return false;
    }

    entry->SetNDArray(nd);
    return true;
}

bool
ContentFib::InsertFibEntry(uint64_t content, uint32_t version)
{
    RemoveFibEntry(content);
    if ((int)m_fib->size() >= m_entrysize)
    {
        ContentFibEntry *e = m_fib->back();
        m_fib->pop_back();
        delete e;
    }
    ContentFibEntry *entry = new ContentFibEntry(m_ndnum, content, version);
    m_fib->push_front(entry);
    return true;
}

bool
ContentFib::FreshFibEntry(uint64_t content, uint32_t version)
{
    for (std::list<ContentFibEntry*>::iterator iter = m_fib->begin(); iter != m_fib->end(); iter ++)
    {
        if ((*iter)->GetContent() == content && (*iter)->GetVersion() >= version)
        {
            ContentFibEntry *e = *iter;
            m_fib->erase(iter);
            m_fib->push_front(e);
            return true;
        }
        else if ((*iter)->GetContent() == content && (*iter)->GetVersion() < version)
        {
            RemoveFibEntry(content);
            return false;
        }
    }
    return false;
}

bool
ContentFib::RemoveFibND(uint64_t content, uint32_t version, int nd)
{
    ContentFibEntry *entry = GetEntry(content, version);
    //No need to check if the version is right: who cares?
    if (entry == 0)
    {
        return false;
    }

    entry->ResetNDArray(nd);
    return (entry->GetSetNDNum() == 0);
}

bool
ContentFib::RemoveFibEntry(uint64_t content)
{
    for (std::list<ContentFibEntry*>::iterator iter = m_fib->begin(); iter != m_fib->end(); iter ++)
    {
        if ((*iter)->GetContent() == content)
        {
            ContentFibEntry *e = *iter;
            m_fib->erase(iter);
            delete e;
            return true;
        }
    }
    return false;
}

ContentFibEntry*
ContentFib::GetEntry(uint64_t content, uint32_t version)
{
    for (std::list<ContentFibEntry*>::iterator iter = m_fib->begin(); iter != m_fib->end(); iter ++)
    {
        if ((*iter)->GetContent() == content && (*iter)->GetVersion() >= version)
        {
            return *iter;
        }
        else if ((*iter)->GetContent() == content && (*iter)->GetVersion() < version)
        {
            RemoveFibEntry(content);
            return 0;
        }
    }

    return 0;
}

int
ContentFib::GetForwardingND(uint64_t content, uint32_t version)
{
    ContentFibEntry *entry = GetEntry(content, version);
    if (entry == 0)
    {
        return -1;
    }

    return entry->GetRandomSetND();
}

};
