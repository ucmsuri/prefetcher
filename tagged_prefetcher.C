/*
 *
 * File: prefetcher.C
 * Author: Sat Garcia (sat@cs)
 * Description: This simple prefetcher waits until there is a D-cache miss then 
 * requests location (addr + 16), where addr is the address that just missed 
 * in the cache.
 *
 */

#include "prefetcher.h"
#include <stdio.h>

/* tagged prefetcher, prefetch the next block(s) based on when they
   are accessed. Our state is a massive bit array indicated whether
   or not an element is "tagged" - meaning that it has been issued as 
   a prefetcher request.  Whenever the prefetcher request is used, we 
   prefetch the next block(s).*/
static char tags[STATE_SIZE];


/* private functions for prefetcher bit-array management*/
static
bool checkPrefetched(u_int32_t addr){
  int bit_index, char_index, rem_bits;
  char section, selector, result;
  bit_index = addr % (STATE_SIZE * sizeof(char));
  char_index = bit_index/BITS_PER_CHAR;  //8 bits per char
  rem_bits = bit_index - (char_index * BITS_PER_CHAR);
  selector = 1;
  selector = selector << rem_bits;
  section = tags[char_index];
  result = section & selector;
  if(result > 0){
    return true;
  }else{
    return false;
  }
}

static
void markPrefetched(u_int32_t addr){
  int bit_index, char_index, rem_bits;
  char section, selector, result;
  bit_index = addr % (STATE_SIZE * sizeof(char));
  char_index = bit_index/BITS_PER_CHAR; //8 bits per char
  rem_bits = bit_index - (char_index * BITS_PER_CHAR);
  /*printf("For address %d, bit_index %d, char_index %d and rem_bits %d\n",
    addr, bit_index, char_index, rem_bits); */
  selector = 1;
  selector = selector << rem_bits;
  tags[char_index] = tags[char_index] | selector;
  return;
}

static
void markUnPrefetched(u_int32_t addr){
  int bit_index, char_index, rem_bits;
  char section, selector, result;
  bit_index = addr % (STATE_SIZE * sizeof(char));
  char_index = bit_index/BITS_PER_CHAR; //8 bits per char
  rem_bits = bit_index - (char_index * BITS_PER_CHAR);
  selector = 1;
  selector = selector << rem_bits;
  tags[char_index] = tags[char_index] & (~selector);
  return;
}

static
bool bitArrayTest(){
  //u_int32_t baseTest = 0xA08A28AC;  //prime bits marked
  tags[0] = 0xff;
  int i;
  memset(tags, 0, STATE_SIZE);
  for (i = 0; i < sizeof(u_int32_t) * 8; i+=2){
    if(checkPrefetched(i)){
      return false;
    }
    markPrefetched(i);
  }
  
  for (i = 0; i < sizeof(u_int32_t) * 8; i+=3){
    markUnPrefetched(i);
  }
  for(i = 0; i < sizeof(u_int32_t) * 8; i++){
    if((i % 2) == 0 && (i % 6) != 0 ){
      if(!checkPrefetched(i)){
	return false;
      }
    }else{
      if(checkPrefetched(i)){
	return false;
      }
    }
  }
  //printf("Value of first 32 bits of tags %x\n", *((int *)tags));
  return true;
}
      
  

Prefetcher::Prefetcher() { 
    _ready = false; 
    /* mark all tags as not prefetched */
    memset(tags, 0, STATE_SIZE);
    /*    if(bitArrayTest()){
      printf("Bit array test passed!\n");
      } */
}

bool Prefetcher::hasRequest(u_int32_t cycle) { return _ready; }

Request Prefetcher::getRequest(u_int32_t cycle) { return _nextReq; }

void Prefetcher::completeRequest(u_int32_t cycle) { 
    if(_req_left == 0){
    _ready = false; 
  }else{
    _req_left--;
    _nextReq.addr = _nextReq.addr + L2_BLOCK_SIZE;
    markPrefetched(_nextReq.addr);  //mark this as a prefetched block 
  }
}

void Prefetcher::cpuRequest(Request req) { 
        
	/*if it is a hit and this was a prefetch address, 
	  get the next one that hasn't been prefetched*/
	if(req.HitL1 && checkPrefetched(req.addr) && !_ready){
	       _nextReq.addr = req.addr + L2_BLOCK_SIZE;
	       /*while(checkPrefetched(_nextReq.addr)){
		 _nextReq.addr += L2_BLOCK_SIZE;
		 printf("INFINITE LOOP!\n");
		 } */
	       markPrefetched(_nextReq.addr);  //mark this as a prefetched block 
	       _ready = true;
	       _req_left = NUM_REQS_PER_MISS - 1;
	}else if(!_ready && !req.HitL1) {
	  /* this was a pure miss, fetch the next n blocks
	     regardless (even if prefetched, prob evicted from cache */
	  _nextReq.addr = req.addr + L2_BLOCK_SIZE;
	  _ready = true;
	  _req_left = NUM_REQS_PER_MISS - 1;
	} 
        /* mark prefetch tag as 0 since CPU requested this */
        markUnPrefetched(req.addr);


}
