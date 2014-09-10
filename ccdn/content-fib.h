#ifndef CONTENT_FIB_H
#define CONTENT_FIB_H

#include <inttypes.h>
#include <list>
#include "ns3/object-factory.h"
#include "content-fib-entry.h"

namespace ns3
{
class ContentFib : public Object
{

public:

    static TypeId GetTypeId (void) {return TypeId ("ns3::ContentFib");};
    ContentFib(const int ndnum, const int entrysize);
    ~ContentFib() {};


    //Update an entry by inserting the specified content, version and nd.
    //Return false if your version is too low (so that the packet should not be up forward again).
    bool InsertFibND(uint64_t content, uint32_t version, int nd);
    //Insert an new entry. Return false if there is an entry of the same content name exist.
    bool InsertFibEntry(uint64_t content, uint32_t version);
	//Set the entry as recently used. Return false if the entry is not found, or the version is not right.
	bool FreshFibEntry(uint64_t content, uint32_t version);
    //Update an entry by removing the specified content, version and nd.
    //Return true if this is the last nd in the entry and the entry itself is removed afterwards. (So you need to go up)
    //Return false if this is not the last nd, or there is no this entry at all.
    bool RemoveFibND(uint64_t content, uint32_t version, int nd);
    //Remove the entry of the content. Return false if there is no entry of this content at all.
    bool RemoveFibEntry(uint64_t content);

    //Return null if there is no hit. Automatically remove content fib entry which is out of date.
    //If there is an entry which is even higher, the entry will be returned as well. So check for it carefully.
    ContentFibEntry* GetEntry(uint64_t content, uint32_t version);
    //Get the forwarding interface;
    int GetForwardingND(uint64_t content, uint32_t version);

private:

	int m_ndnum;
	int m_entrysize;

	std::list<ContentFibEntry*> *m_fib;

};
};


#endif
