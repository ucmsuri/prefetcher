/*
 *
 * File: prefetcher.h
 * Author: Sat Garcia (sat@cs)
 * Description: Header file for prefetcher implementation
 *
 */

#ifndef PREFETCHER_H
#define PREFETCHER_H
#define K 2

//L1 cache: Tag: 18bits,Index:1024lines/10bits, offset:16bytes/4bits
#define L1TAGMASK  0xFFFFC000
#define L1TAGSHIFT 14
#define L1INDEXMASK 0x00005FF0
#define L1INDEXSHIFT 4
#define L1OFFSET 0x0000000F

//L1 cache: Tag: 17bits,Index:1024lines/10bits, offset:32bytes/5bits
#define L2TAGMASK 0xFFFF8000
#define L2TAGSHIFT 15
#define L2INDEXMASK 0x00007FE0
#define L2INDEXSHIFT 5
#define L2OFFSET 0x0000001F

#define INDICES 64
#define QUEUESIZE 32
#define PREDICTIONS 4
#define OFFSET  0x0000000F
#define TCZSHIFT 4
#define TCZMASK ((TCZONES-1) << TCZSHIFT)
#define TAGMASK (0xFFFFFFFF ^ (TCZMASK | OFFSET))
#define TAGSHIFT (4+TCZBITS)

#include <sys/types.h>
#include "mem-sim.h"

class Prefetcher {
  private:
	bool _ready;
	Request _nextReq;
    Request _lastReq;

    struct Prediction{
    	u_int32_t addr;
    	u_int8_t hits;
    };
    struct Prefetch{
    	u_int32_t addr;
    	float prob; // Hits/totalHits
    };

    u_int32_t lastMiss;
    struct Prediction PredictionTable[INDICES][PREDICTIONS];  	//128*4*5
    u_int32_t MissTable[INDICES];								//128*1
    struct Prefetch PrefetchQueue[QUEUESIZE];					//8*4

    void ResetState(int TCZone);
  public:
	Prefetcher();

	// should return true if a request is ready for this cycle
	bool hasRequest(u_int32_t cycle);

	// request a desired address be brought in
	Request getRequest(u_int32_t cycle);

	// this function is called whenever the last prefetcher request was successfully sent to the L2
	void completeRequest(u_int32_t cycle);

	/*
	 * This function is called whenever the CPU references memory.
	 * Note that only the addr, pc, load, issuedAt, and HitL1 should be considered valid data
	 */
	void cpuRequest(Request req); 
};

#endif
