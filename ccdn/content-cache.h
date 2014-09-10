#ifndef CONTENT_CACHE_H
#define CONTENT_CACHE_H

#include <inttypes.h>
#include <list>

namespace ns3
{

struct Content
{
    uint64_t    m_content;
    uint32_t    m_version;
};

class ContentCache
{

public:

    ContentCache(const int cachesize);
    ~ContentCache() {};



//These functions is to manage and visit the cache.

    //Return false if there is cache of this name and the version is the same or even higher. (I know, usually this won't happen)
    //Also, sometimes adding new cache will cause old cache removed.
    bool AddCache(uint64_t content, uint32_t version);
    //Return false if there is no cache of this name.
    bool RemoveCache(uint64_t content);
    //Return false if cache not exist, or the version is too low.
    //Lower version cache will be automatically removed.
    //If it is returned as true, then the hit cache will be set as recently visited.
    bool HasCache(uint64_t content, uint32_t version);



private:

    int m_cachesize;
    std::list<Content*>    *m_cache;


};
};


#endif
