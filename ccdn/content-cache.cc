#include <inttypes.h>
#include <list>

#include "content-cache.h"

namespace ns3
{

ContentCache::ContentCache(const int cachesize)
{
    m_cachesize = cachesize;
    m_cache = new std::list<Content*>(0);
}

bool
ContentCache::AddCache(uint64_t content, uint32_t version)
{
    if (HasCache(content, version))
    {
        return false;
    }

    if ((int)m_cache->size() >= m_cachesize)
    {
        Content *c = m_cache->back();
        m_cache->pop_back();
        delete c;
    }

    Content *c = new Content;
    c->m_content = content;
    c->m_version = version;
    m_cache->push_front(c);
    return true;
}

bool
ContentCache::RemoveCache(uint64_t content)
{
    for (std::list<Content*>::iterator iter = m_cache->begin(); iter != m_cache->end(); iter ++)
    {
        if ((*iter)->m_content == content)
        {
            Content *c = *iter;
            m_cache->erase(iter);
            delete c;
            return true;
        }
    }
    return false;
}

bool
ContentCache::HasCache(uint64_t content, uint32_t version)
{
    for (std::list<Content*>::iterator iter = m_cache->begin(); iter != m_cache->end(); iter ++)
    {
        if ((*iter)->m_content == content && (*iter)->m_version >= version)
        {
            Content *c = *iter;
            m_cache->erase(iter);
            m_cache->push_front(c);
            return true;
        }
        else if ((*iter)->m_content == content && (*iter)->m_version < version)
        {
            Content *c = *iter;
            m_cache->erase(iter);
            delete c;
            return false;
        }
    }
    return false;
}

};
