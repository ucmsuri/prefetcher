#include "prefetcher.h"
#include <stdio.h>

#define DEBUG 0

// Helper function to initilize GHB table
void initGHB(struct item *ghbTable){

  if (ghbTable) {
    for (int i = 0; i < MAX_CAPACITY; i++) {
      ghbTable[i].delta = 0;
      ghbTable[i].prev = -1;
    }
  } else {
    if(DEBUG == 1) printf("ERROR: items initialization failed");
  }
}

Prefetcher::Prefetcher() {

  _nextReq.addr = 0;

  _ready = false;
  _queue_full = false;
  _nextReq.addr = 0;
  _end_of_queue = 0;
  _prev_addr = 0;
  _next_delta = 0;
  _depth = 0;

  initGHB(_ghb_table);
}

Prefetcher::~Prefetcher() {
}


/*
  Helper function
  find max item from the candidate array
*/
int findMax(struct candidate candidates[], int end_index){
  int max_candidate_value = 0;
  int index = 0;

    //Sort through the candidate list and find the most likely candidate item.
  for (int i = 0; i < end_index; i++) {
   if(DEBUG == 1)  printf(" candidate %d, 0x%x, %d\n", i, candidates[i].delta, candidates[i].count);
    if (candidates[i].count > max_candidate_value) {
      max_candidate_value = candidates[i].count;
      index = i;
    }
  }
}

/*
  Helper function
  Iterate from the end of the array since is a FIFO queue.
  Use the given address difference try to find the previous entry in the global
  history table with the same addresss distance
*/
int Prefetcher::locatePrevEntry(int delta){
  int cur_enq = _end_of_queue;
  int i;
  if (_queue_full == false)
  {
    for (i = _end_of_queue; i >= 0; i--)
    {
      if (_ghb_table[i].delta == delta) {
        return i;
      }
    }
    return -1;
  }else{
    while( i < MAX_CAPACITY - 1){
      if (cur_enq == 0)
        cur_enq = MAX_CAPACITY - 1;
      else
        cur_enq--;

      if (_ghb_table[cur_enq].delta == delta) {
        return cur_enq;
      }
      i++;
    }
  }
  return -1;
}

/*
  Helper function
  Search the global history table, with the current address difference,
  find the next most likely request's address delta relative to current requrest
  We will keep a candidate array to track all the possible request' address delta
  and eventually sort the array and add the most possible next distance to current
  request address, we will use it to format the next prefetched request address.
 */
int Prefetcher::locateCandidate(int delta){
  int cur_enq = _end_of_queue;
  int possible_delta_index;
  struct candidate candidates[10];

  int i;
  bool found = false;
  int candidate_array_index = 0;
  int flip = 0;

  int index = 0;
  int next_pos = 0;
  int temp_delta = 0;

  /* Initialize the candidates array with 10 entrys */
  for (i = 0; i < 10; i++) {
    candidates[i].delta = 0;
    candidates[i].count = 0; //set counter to 0
  }

  while(1) {
      next_pos = _ghb_table[cur_enq].prev;

      if (next_pos == -1)
        break;
      if (next_pos >= cur_enq && flip)
        break;
      if (next_pos >= cur_enq)
        flip = 1;
      if (_ghb_table[next_pos].delta != delta)
        break;

      possible_delta_index = cur_enq + 1;
      if (possible_delta_index == MAX_CAPACITY){
        possible_delta_index = 0;
      }

      /* Insert the addr of candidate entry to candidates array */
      temp_delta = _ghb_table[possible_delta_index].delta;
      if (temp_delta) {
        for (i = 0; i < candidate_array_index; i++) {
          if (candidates[i].delta == temp_delta) {
            candidates[i].count++;
            break;
          }
        }

        if (i == candidate_array_index && candidate_array_index < 10) { //add new candidates
          candidates[candidate_array_index].delta = temp_delta;
          candidates[candidate_array_index].count++;
          candidate_array_index++;
        }
      }

    if(DEBUG == 1)  printf("delta %d, enqueue %d, next_pos %d, cur_enq %d\n", delta, _end_of_queue, next_pos, cur_enq);
    cur_enq = next_pos;
  }

  index = findMax(candidates, candidate_array_index);

  if (candidates[index].delta){
    return candidates[index].delta;
  }else{
    return 0;
  }
}

/*
  When CPU is idle, we will issue up to three prefetch request
  Although you can issue more than 3, but it would occupy the L2 queue and has
  impact on performence
*/
bool Prefetcher::hasRequest(u_int32_t cycle){
  _depth++;
  if (_depth > 3)
    _ready = false;
  else
    _ready = true;
  if(DEBUG == 1) printf("Has request, _depth %d, ready %d\n", _depth, _ready);
  return _ready;
}

/*
  We  will fetch 3 different request based on the next delta value
  After some trial and fine tuning, we find out 32 is a reasonable value to use
  for alignment
*/
Request Prefetcher::getRequest(u_int32_t cycle){
  int next_delta;

  switch(_depth){
    case 1:
      next_delta = _next_delta;
      break;
    case 2:
      next_delta = _next_delta - 32;
      break;
    default:
      next_delta = _next_delta + 32;
  }

  _nextReq.addr = _prev_addr + next_delta;
  if(DEBUG == 1) printf("Next address %d\n", _nextReq.addr);
  return _nextReq;
}

void Prefetcher::completeRequest(u_int32_t cycle){
  return;
}

/*


*/

void Prefetcher::cpuRequest(Request req){
  int delta;
  int next_delta;

  if(!_ready && !req.HitL1) {
    delta = req.addr - _prev_addr;

    //TODO use LRU replacement
    _ghb_table[_end_of_queue].delta = delta;
    _ghb_table[_end_of_queue].prev = locatePrevEntry(delta);
    _end_of_queue++;

    if (_end_of_queue == MAX_CAPACITY){
      _end_of_queue = 0;
      _queue_full = true;
    }

    next_delta = locateCandidate(delta);

    /* align next_delta to 64 */
    if (next_delta >= 0 && next_delta < 64) {
     if(DEBUG == 1) printf("find candidate delta %d for 0x%x\n", next_delta, req.addr);
      next_delta = 64;
    } else if (next_delta < 0 && next_delta > -64){
      next_delta = -64;
    }

    _next_delta = next_delta;
    _prev_addr = req.addr;

    /*
       Set flag variable to ready state, then when CPU is in idle state,
       we can start prefetching
    */
    _ready = true;
    _depth = 0;
  }
}
