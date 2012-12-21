// Copyright 2012 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2012, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _CACHE_H
#define _CACHE_H

#include <deque>
#include <map>
#include <list>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include "memEvent.h"

namespace SST {
namespace MemHierarchy {

class Cache : public SST::Component {

private:
	typedef enum {DOWNSTREAM, SNOOP, DIRECTORY, UPSTREAM, SELF} SourceType_t;

	class Request;
	class CacheRow;
	class CacheBlock;
	class SelfEvent;

	class CacheBlock {
	public:
		/* Flags */
		typedef enum {INVALID, ASSIGNED, SHARED, EXCLUSIVE} BlockStatus;

		Addr tag;
		Addr baseAddr;
		SimTime_t last_touched;
		BlockStatus status;
		Cache *cache;
		std::vector<uint8_t> data;
		uint32_t locked;
		MemEvent *currentEvent;

		CacheBlock() {}
		CacheBlock(Cache *_cache)
		{
			tag = 0;
			baseAddr = 0;
			last_touched = 0;
			status = INVALID;
			cache = _cache;
			data = std::vector<uint8_t>(cache->blocksize);
			locked = 0;
			currentEvent = NULL;
		}

		~CacheBlock()
		{ }

		void activate(Addr addr)
		{
			assert(status != ASSIGNED);
			assert(locked == 0);
			tag = cache->addrToTag(addr);
			baseAddr = cache->addrToBlockAddr(addr);
			__DBG( DBG_CACHE, CacheBlock, "Activating block %p for Address 0x%lx.\t"
					"baseAddr: 0x%lx  Tag: 0x%lx\n", this, addr, baseAddr, tag);
			status = ASSIGNED;
		}

		bool isValid(void) { return (status != INVALID && status != ASSIGNED); }
		bool isInvalid(void) { return (status == INVALID); }
		bool isAssigned(void) { return (status == ASSIGNED); }

	};

	class CacheRow {
	public:
		std::vector<CacheBlock> blocks;
		Cache *cache;

		CacheRow() {}
		CacheRow(Cache *_cache) : cache(_cache)
		{
			blocks = std::vector<CacheBlock>(cache->n_ways, CacheBlock(cache));
		}

		CacheBlock* getLRU(void)
		{
			SimTime_t t = (SimTime_t)-1;
			int lru = -1;
			for ( int i = 0 ; i < cache->n_ways ; i++ ) {
				if ( blocks[i].isAssigned() ) continue; // Assigned, waiting for incoming
				if ( blocks[i].locked > 0 ) continue; // Currently in use

				if ( !blocks[i].isValid() ) {
					lru = i;
					break;
				}
				if ( blocks[i].last_touched <= t ) {
					t = blocks[i].last_touched;
					lru = i;
				}
			}
			assert(lru >= 0); // If this fails, no block available.

			return &blocks[lru];
		}
	};


	typedef union {
		struct {
			CacheBlock *block;
			CacheBlock::BlockStatus newStatus;
			bool decrementLock;
		} writebackBlock;
		struct {
			CacheBlock *block;
			MemEvent *ev;
		} issueInvalidate;
		struct {
			CacheBlock *block;
			SourceType_t src;
		} supplyData;
	} BusFinishHandlerArgs;
	typedef void(Cache::*BusFinishHandlerFunc)(BusFinishHandlerArgs &args);

	class BusFinishHandler {
	public:
		BusFinishHandler(BusFinishHandlerFunc func, BusFinishHandlerArgs args) :
			func(func), args(args)
		{ }

		BusFinishHandlerFunc func;
		BusFinishHandlerArgs args;
	};

	class BusQueue {
	public:
		BusQueue(void) :
			requested(false), comp(NULL), link(NULL)
		{ }

		BusQueue(Cache *comp, SST::Link *link) :
			requested(false), comp(comp), link(link)
		{ }

		bool requested;

		void setup(Cache *_comp, SST::Link *_link) {
			comp = _comp;
			link = _link;
		}

		int size(void) const { return queue.size(); }
		void request(MemEvent *event, BusFinishHandler *handler)
		{
			if ( event ) {
				queue.push_back(event);
				if ( handler != NULL ) {
					map[event] = handler;
				}
			}
			if ( !requested ) {
				link->Send(new MemEvent(comp->getName(), NULL, RequestBus));
				requested = true;
			}
		}

		BusFinishHandler* cancelRequest(MemEvent *event)
		{
			BusFinishHandler *retval = NULL;
			queue.remove(event);
			std::map<MemEvent*, BusFinishHandler*>::iterator i = map.find(event);
			if ( i != map.end() ) {
				retval = i->second;
				map.erase(i);
			}
			//delete event; // We have responsibility for this event, due to the contract of Link::Send()
			return retval;
		}

		void clearToSend(void)
		{
			if ( size() == 0 ) {
				__DBG( DBG_CACHE, BusQueue, "No Requests to send!\n");
				/* Must have canceled the request */
				link->Send(new MemEvent(comp->getName(), NULL, CancelBusRequest));
				requested = false;
			} else {
				MemEvent *ev = queue.front();
				queue.pop_front();

				__DBG( DBG_CACHE, BusQueue, "Sending Event (%s, 0x%lx)!\n", CommandString[ev->getCmd()], ev->getAddr());
				link->Send(ev);

				std::map<MemEvent*, BusFinishHandler*>::iterator i = map.find(ev);
				if ( i != map.end() ) {
					BusFinishHandler *br = i->second;
					if ( br ) {
						(comp->*(br->func))(br->args);
						delete br;
					}
					map.erase(i);
				}

				requested = false;
				if ( size() > 0 ) {
					/* Re-request.  We have more to send */
					request(NULL, NULL);
				}
			}
		}


	private:
		Cache *comp;
		SST::Link *link;
		std::list<MemEvent*> queue;
		std::map<MemEvent*, BusFinishHandler*> map;

	};

	typedef void (Cache::*SelfEventHandler)(MemEvent*, CacheBlock*, SourceType_t);
	class SelfEvent : public SST::Event {
	public:
		SelfEvent() {} // For serialization

		SelfEvent(SelfEventHandler handler, MemEvent *event, CacheBlock *block,
				SourceType_t event_source = SELF) :
			handler(handler), event(event), block(block), event_source(event_source)
		{ }

		SelfEventHandler handler;
		MemEvent *event;
		CacheBlock *block;
		SourceType_t event_source;
	};

public:

	Cache(SST::ComponentId_t id, SST::Component::Params_t& params);
	int Setup()  { return 0; }
	int Finish();

private:
	void handleIncomingEvent(SST::Event *event, SourceType_t src);
	void handleIncomingEvent(SST::Event *event, SourceType_t src, bool firstTimeProcessed);
	void handleSelfEvent(SST::Event *event);

	void handleCPURequest(MemEvent *ev, bool firstProcess);
	void sendCPUResponse(MemEvent *ev, CacheBlock *block, SourceType_t src);

	void issueInvalidate(MemEvent *ev, CacheBlock *block);
	void loadBlock(MemEvent *ev, SourceType_t src);

	void handleCacheRequestEvent(MemEvent *ev, SourceType_t src, bool firstProcess);
	void supplyData(MemEvent *ev, CacheBlock *block, SourceType_t src);
	void finishBusSupplyData(BusFinishHandlerArgs &args);
	void handleCacheSupplyEvent(MemEvent *ev, SourceType_t src);
	void finishSupplyEvent(MemEvent *origEV, CacheBlock *block, SourceType_t src);
	void handleInvalidate(MemEvent *ev, SourceType_t src);

	bool waitingForInvalidate(CacheBlock *block);
	void cancelInvalidate(CacheBlock *block);
	void finishIssueInvalidateVA(BusFinishHandlerArgs &args);
	void finishIssueInvalidate(MemEvent *ev, CacheBlock *block);

	void writebackBlock(CacheBlock *block, CacheBlock::BlockStatus newStatus);
	void finishWritebackBlockVA(BusFinishHandlerArgs &args);
	void finishWritebackBlock(CacheBlock *block, CacheBlock::BlockStatus newStatus, bool decrementLock);

	void busClear(SST::Link *busLink);

	void updateBlock(MemEvent *ev, CacheBlock *block);
	SST::Link *getLink(SourceType_t type, int link_id);
	int numBits(int x);
	Addr addrToTag(Addr addr);
	Addr addrToBlockAddr(Addr addr);
	CacheBlock* findBlock(Addr addr, bool emptyOK = false);
	CacheRow* findRow(Addr addr);

	void printCache(void);

	int n_ways;
	int n_rows;
	int blocksize;
	std::string access_time;
	std::vector<CacheRow> database;
	std::string next_level_name;

	int rowshift;
	unsigned rowmask;
	int tagshift;

	int n_upstream;
	SST::Link *snoop_link; // Points to a snoopy bus, or snoopy network (if any)
	SST::Link *directory_link; // Points to a network for directory lookups (if any)
	SST::Link **upstream_links; // Points to directly upstream caches or cpus (if any) [no snooping]
	SST::Link *downstream_link; // Points to directly downstream cache (if any)
	SST::Link *self_link; // Used for scheduling access
	std::map<LinkId_t, int> upstreamLinkMap;

	/* Stats */
	uint64_t num_read_hit;
	uint64_t num_read_miss;
	uint64_t num_supply_hit;
	uint64_t num_supply_miss;
	uint64_t num_write_hit;
	uint64_t num_write_miss;
	uint64_t num_upgrade_miss;


	struct LoadInfo_t {
		CacheBlock *targetBlock;
		std::vector<std::pair<MemEvent*, SourceType_t> > list;
		LoadInfo_t() : targetBlock(NULL) { }
	};
	typedef std::map<Addr, LoadInfo_t> LoadList_t;
	LoadList_t waitingLoads;


	struct SupplyInfo {
		MemEvent *busEvent;
		bool canceled;
		SupplyInfo() : busEvent(NULL), canceled(false) { }
		SupplyInfo(MemEvent *event) : busEvent(event), canceled(false) { }
	};
	typedef std::map<std::pair<Addr, SourceType_t>, SupplyInfo> supplyMap_t;
	supplyMap_t supplyInProgress;

	BusQueue snoopBusQueue;

};

}
}

#endif
