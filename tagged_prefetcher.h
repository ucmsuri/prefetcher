/*
 *
 * File: prefetcher.h
 * Author: Sat Garcia (sat@cs)
 * Description: Header file for prefetcher implementation
 *
 */

#ifndef PREFETCHER_H
#define PREFETCHER_H

#include <sys/types.h>
#include "mem-sim.h"
#include <cstdlib>
#include <cstring>


#define STATE_SIZE 4096
#define BITS_PER_CHAR 8
#define L2_BLOCK_SIZE 16
#define NUM_REQS_PER_MISS 2

class Prefetcher {
  private:
	bool _ready;
	Request _nextReq;
	int _req_left;
	

  public:
	Prefetcher();

	/* functions for dealing with the array */
	/*	bool checkPrefetched(u_int32_t addr);
	void markPrefetched(u_int32_t addr);
	void markUnPrefetched(u_int32_t addr);
	bool bitArrayTest(void); */


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
