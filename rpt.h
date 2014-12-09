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
#include <string.h>
#include <cstdlib>
#include "mem-sim.h"

#define L2_BLOCK_SIZE 32
#define NUM_REQS_PER_MISS 4
#define NUM_RPT_ENTRIES 256
#define WORTHWHILE_RPT 128
/* prefetcher state data struct, one row in RPT table.
   We may want to remove PC and say "ok" to aliasing.  */
typedef enum{
  INIT,
  TRANSITION,
  STEADY
} state_t;

typedef struct rpt_row_entry{
  unsigned int pc;
  unsigned int last_mem;
  int last_stride; //leaving 8 bits for the tag (256 entry RPT)
  state_t state;  //currently 2 bits
} rpt_row_entry_t;

class Prefetcher {
  private:
	bool _ready;
	Request _nextReq;
	int _req_left;
	//struct rpt_row_entry *rpt_table;
	//static rpt_row_entry_t rpt_table[NUM_RPT_ENTRIES]; 

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
