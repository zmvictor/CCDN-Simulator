#include <stdlib.h>
#include "ns3/log.h"
#include "content-fib-entry.h"


namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("ContentFibEntry");

NS_OBJECT_ENSURE_REGISTERED (ContentFibEntry);

ContentFibEntry::ContentFibEntry(const int ndnum, uint64_t content, uint32_t version)
{
    m_ndnum = ndnum;
    m_content = content;
    m_version = version;
    m_ndarray = new bool[ndnum];
    for (int i=0; i<m_ndnum; i++)
    {
        m_ndarray[i] = false;
    }
}

bool
ContentFibEntry::SetNDArray(int nd)
{
    if (nd >= m_ndnum)
    {
        return false;
    }

    m_ndarray[nd] = true;
    return true;
}

bool
ContentFibEntry::ResetNDArray(int nd)
{
    if (nd >= m_ndnum)
    {
        return false;
    }

    m_ndarray[nd] = false;
    return true;
}

int
ContentFibEntry::GetSetNDNum()
{
    int set_nd = 0;
    for (int i=0; i<m_ndnum; i++)
    {
        set_nd += m_ndarray[i];
    }
    return set_nd;
}

int
ContentFibEntry::GetRandomSetND()
{
    int set_nd = GetSetNDNum();
    if (set_nd == 0)
    {
        return -1;
    }

    set_nd = rand() % set_nd;
    for (int i=0; i<m_ndnum; i++)
    {
        if (m_ndarray[i])
        {
            if (set_nd > 0)
            {
                set_nd --;
            }
            else
            {
                return i;
            }
        }
    }

    return -1;
}

};
