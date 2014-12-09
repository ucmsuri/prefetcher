#ifndef PREFETCHER_H
#define PREFETCHER_H

#include <sys/types.h>
#include <malloc.h>
#include <stdint.h>
#include "mem-sim.h"

#define MAX_CAPACITY 512 //4kb state, 512 * 8byte(struct item entry size)

/*
	State item, we will track each missed request's address difference compared
	with previous request, we will also track the previous request that share the
	same address difference to make later prefetch candidate searching faster
*/
struct item
{
	int delta;
	int prev;
};

struct candidate
{
	int delta;
	int count;
};

class Prefetcher {
  private:
	bool _ready;
	Request _nextReq;

	/*
		a 512-entry table, maintain global delta history information,
		each entry is 8 bytes big, so altogher, we utilized the entire 4kb state
	*/
	struct item _ghb_table[MAX_CAPACITY];
	bool _queue_full;
	int _end_of_queue;
	int _prev_addr;
	int _next_delta;
	int _depth;

  public:
	Prefetcher();
	~Prefetcher();

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

	//Helper functions

	/* Find the previous entry of the same delta */
	int locatePrevEntry(int delta);

	/* Find the next candidate entry to prefetch */
	int locateCandidate(int delta);
};

#endif
