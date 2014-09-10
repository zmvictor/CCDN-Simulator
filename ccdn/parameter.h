#ifndef PARAMETER_H
#define PARAMETER_H

#include <stdint.h>

namespace ns3 {


struct Parameter
{
    //Topo
    unsigned fib_size;
    unsigned cache_size;
    unsigned port;
    bool enable_cache;
    char* filename;
};

};

#endif
