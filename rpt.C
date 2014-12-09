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

static int counter = 0;
static rpt_row_entry_t rpt_table[NUM_RPT_ENTRIES]; 

Prefetcher::Prefetcher() { 
  int i;
  printf("PRE: creating prefetcher with value of (%d reqs per miss)\n", NUM_REQS_PER_MISS);
  printf("PRE: NUM ENTRIES IN TABLE: %d\n", NUM_RPT_ENTRIES);


  _ready = false; 
  /* create memory for rpt table */
  //rpt_table = (struct rpt_row_entry *)malloc(sizeof(struct rpt_row_entry) * NUM_RPT_ENTRIES);
  /* clear to 0's the rpt table */
  //memset(rpt_table, 0, sizeof(struct rpt_row_entry) * NUM_RPT_ENTRIES);
  for(i = 0; i < NUM_RPT_ENTRIES; i++){
    rpt_table[i].pc = 0;
    rpt_table[i].last_stride = 0;
    rpt_table[i].last_mem = 0;
  }

  
}

bool Prefetcher::hasRequest(u_int32_t cycle) { return _ready; }

Request Prefetcher::getRequest(u_int32_t cycle) { 
  return _nextReq; }

void Prefetcher::completeRequest(u_int32_t cycle) { 
  int rpt_row_num, curr_stride;
  rpt_row_entry_t *curr_row;
  
  if(_req_left == 0){
    _ready = false; 
  }else{
    _req_left--;
    /* determine if we should fetch strides or not */
    rpt_row_num = _nextReq.pc % NUM_RPT_ENTRIES;
    curr_row = &rpt_table[rpt_row_num];
    if(curr_row->last_stride != 0){
      _nextReq.addr = _nextReq.addr + curr_row->last_stride;
      curr_row->last_mem = _nextReq.addr;
    }else{
      _nextReq.addr = _nextReq.addr + L2_BLOCK_SIZE;
    }
  }
  
}

void Prefetcher::cpuRequest(Request req) { 
        int rpt_row_num, curr_stride;
	rpt_row_entry_t *curr_row;
 	if(!_ready && !req.HitL1) {
	       /* check entry in RPT table */
	       rpt_row_num = req.pc % NUM_RPT_ENTRIES;
	       curr_row = &rpt_table[rpt_row_num];
	       //printf("PRE: rpt_row_num: %d, pc\t%x and curr_pc\t%x\n", rpt_row_num, req.pc, curr_row->pc);
	       if(curr_row->pc == req.pc){
		 //printf("DEBUG: req pc matches for the %dth time\n", counter++);
		if (req.addr - (curr_row->last_mem) <0)
		{ printf("reverse stride\n");}
		 /* this entry is in the table */
		 if((curr_stride = req.addr - (curr_row->last_mem)) == curr_row->last_stride && curr_stride > WORTHWHILE_RPT){
		   /* if stride is the same as this one,
		      "punch-it" use it to prefetch */
		     printf("PRE: same stride found for address %x, with lastmem of %x and curr_req of %x, previous stride at %d\n",
			    req.pc, curr_row->last_mem, req.addr, curr_stride);
		   _nextReq.addr = req.addr + curr_stride;
		   _ready = true;
		   _req_left = NUM_REQS_PER_MISS - 1;  
		 }else{
		   /* update to new stride  and do standard prefetch TODO */
		      curr_row->last_stride = curr_stride; 
		      _nextReq.addr = req.addr + L2_BLOCK_SIZE;
		      _ready = true;
		      _req_left = NUM_REQS_PER_MISS - 1;  
		      } 
	       }else{
		 /* no pc, so do standard prefetch*/
		   _nextReq.addr = req.addr + L2_BLOCK_SIZE;
		   _ready = true;
		   _req_left = NUM_REQS_PER_MISS - 1;  
		   /* also update stride to 0 so new entry not confused */
		   curr_row->last_stride = 0;
	       }
	       /* in all cases, update row in RPT */
	       curr_row->pc = req.pc;
	       curr_row->last_mem = req.addr;
	       
	} 

}
