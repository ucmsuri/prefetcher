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
#include <stdlib.h>
#include <string.h>

Prefetcher::Prefetcher() {
	lastMiss = 0xFFFFFFFF;
	for(int i = 0; i < INDICES; i++){
		MissTable[i] = 0;
		for(int j = 0; j < PREDICTIONS; j++){
			PredictionTable[i][j].hits = 0;
			PredictionTable[i][j].addr = 0;
		}
	}
	for(int k = 0; k < QUEUESIZE; k++){
		PrefetchQueue[k].addr = 0;
		PrefetchQueue[k].prob = 0;
	}
	_ready = false;
}

bool Prefetcher::hasRequest(u_int32_t cycle) { return _ready;}

Request Prefetcher::getRequest(u_int32_t cycle) { 
    return _nextReq; 
}

void Prefetcher::completeRequest(u_int32_t cycle) { _ready = false; }

void Prefetcher::cpuRequest(Request req) { 

	if(req.HitL1)
		return;

	u_int32_t miss = req.addr & (0xFFFFFFFF ^ OFFSET);

	//Update Markov Table
	//Check last address is already in the table
	bool foundMiss = false;
	bool foundPrediction = false;
	for(int i = 0; i < INDICES; i++){
		if(MissTable[i] == lastMiss){
			foundMiss = true;
			//Check for miss on list of predictions
			for(int j = 0; j < PREDICTIONS; j++){
				if(PredictionTable[i][j].addr == miss){
					foundPrediction = true;
					if(PredictionTable[i][j].hits < 255){
						PredictionTable[i][j].hits++;
					}
				}
			}
			if(foundPrediction == false){ // Kick lowest prediction value off table
				int min = PredictionTable[i][0].hits;
				int slot = 0;
				for(int j = 1; j < PREDICTIONS; j++){
					if(PredictionTable[i][j].hits < min){
						min = PredictionTable[i][j].hits;
						slot = j;
					}
				}
				PredictionTable[i][slot].hits = 1;
				PredictionTable[i][slot].addr = miss;
			}
			break;
		}
	}
	if(foundMiss == false){
		//Need to shift everything down then add to top
		for(int i = INDICES-1; i > 0; i--){
			MissTable[i] = MissTable[i-1];
			for(int j = 0; j < PREDICTIONS; j++){
				PredictionTable[i][j].hits = PredictionTable[i-1][j].hits;
				PredictionTable[i][j].addr = PredictionTable[i-1][j].addr;
			}
		}

		for(int j = 0; j < PREDICTIONS; j++){
			PredictionTable[0][j].hits = 0;
			PredictionTable[0][j].addr = 0;
		}
		MissTable[0] = lastMiss;
		PredictionTable[0][0].hits = 1;
		PredictionTable[0][0].addr = miss;
	}
	lastMiss = miss;

	//Make Predictions
	for(int i = 0; i < INDICES; i++){
		if(MissTable[i] == lastMiss){
			//Add up hits
			int totalHits = 0;
			for(int j = 0; j < PREDICTIONS; j++){
				totalHits += PredictionTable[i][j].hits;
			}

			//try to put predictions into prefetch queue
			for(int j = 0; j < PREDICTIONS; j++){
				float prob = PredictionTable[i][j].hits/totalHits;
				float min = 1;
				int slot = 0;
				for(int k = 0; k < QUEUESIZE; k++){//find lowest prob value
					if(PrefetchQueue[k].prob < min){
						min = PrefetchQueue[k].prob;
						slot = k;
					}
				}
				if(prob > min){
					PrefetchQueue[slot].prob = prob;
					PrefetchQueue[slot].addr = PredictionTable[i][j].addr;
				}
			}
		}
	}

	if(!_ready){
		int max = 0;
		int slot = 0;
		for(int k = 0; k < QUEUESIZE; k++){//find lowest prob value
			if(PrefetchQueue[k].prob > max){
				max = PrefetchQueue[k].prob;
				slot = k;
			}
		}

		if(max > 0){
			_nextReq.addr = PrefetchQueue[slot].addr;
			_ready = true;
			PrefetchQueue[slot].addr = 0;
			PrefetchQueue[slot].prob = 0;
		}
	}

}

void Prefetcher::ResetState(int TCZone){

}

