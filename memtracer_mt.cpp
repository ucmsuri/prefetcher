#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdlib.h>
#include <stddef.h>
#include "pin.H"
#include "portability.H"
#include <inttypes.h>
//#include "gzstream.h"
using namespace std;

static UINT32 icount = 0;
UINT32 icount_lastmem;
UINT32 sincelast;
UINT32 memrefs; // number of memory accesses so far
#define MAX_REFS 200000000 // we want to limit the size of the log files so we will limit trace files to 2M lines

// This function is called before every instruction is executed
VOID docount() { icount++; }

/*
// update lastmem info and write to file (load)
VOID RecordMemRead(VOID * ip, VOID * addr)
{
	if(icount == icount_lastmem) sincelast = 0;
	else {
		sincelast = icount - icount_lastmem - 1;
		icount_lastmem = icount;
	}

    fprintf(trace,"l %p %p %u\n", ip, addr, sincelast);

	++memrefs;

	if(memrefs > MAX_REFS) {
		cout << "Maximum number of memops reached. Stopping instrumentation... " << endl;
    	fclose(trace);
		PIN_Detach();
	}
}

// Print a memory write record
VOID RecordMemWrite(VOID * ip, VOID * addr)
{
	if(icount == icount_lastmem) sincelast = 0;
	else {
		sincelast = icount - icount_lastmem - 1;
		icount_lastmem = icount;
	}

    fprintf(trace,"s %p %p %u\n", ip, addr, sincelast);

	++memrefs;

	if(memrefs > MAX_REFS) {
		cout << "Maximum number of memops reached. Stopping instrumentation... " << endl;
    	fclose(trace);
		PIN_Detach();
	}
}

*/

/*
 * Name of the output file
 */
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "buffer.out", "output file");

/*
 * The ID of the buffer
 */
BUFFER_ID bufId;

/*
 * Thread specific data
 */
TLS_KEY mlog_key;

/*
 * Number of OS pages for the buffer
 */
#define NUM_BUF_PAGES 1024
/*
 * Record of memory references.  Rather than having two separate
 * buffers for reads and writes, we just use one struct that includes a
 * flag for type.
 */
struct MEMREF
{
    ADDRINT     pc;
    ADDRINT     ea;
    UINT32      size;
    BOOL        read;
};


/*
 * MLOG - thread specific data that is not handled by the buffering API.
 */
class MLOG
{
  public:
    MLOG(THREADID tid);
    ~MLOG();

    VOID DumpBufferToFile( struct MEMREF * reference, UINT64 numElements, THREADID tid );

  private:
    //ofstream _ofile;
    FILE* _ofile;
  //  FILE * trace[2];
};


MLOG::MLOG(THREADID tid)
{
 //   string filename = KnobOutputFile.Value() + "." + decstr(getpid_portable()) + "." + decstr(tid);
    //string filename = "bzip2 -c > " + KnobOutputFile.Value() + "." + decstr(getpid_portable()) + "." + decstr(tid);
  // _ofile[tid] = popen(filename.c_str(),"w");
        
    
//_ofile.open(filename.c_str());
 //string filename = "bzip2 -c > " + KnobOutputFile.Value() + "." + decstr(getpid_portable()) + "." + decstr(tid);
 //string filename = "xz -2 -c > " + KnobOutputFile.Value() + "." + decstr(getpid_portable()) + "." + decstr(tid);
 string filename = "xz -2 -c > " + KnobOutputFile.Value() +  "." + decstr(tid);
// cout << filename << "  " << sizeof(ADDRINT) <<endl;
    //FILE* temp = popen(filename.c_str(),"w");
    //_ofile(temp);
    _ofile = popen(filename.c_str(),"w");
    if ( ! _ofile )
    {
        cerr << "Error: could not open output file." << endl;
        exit(1);
    }
    
    //_ofile << hex;

}


MLOG::~MLOG()
{
  //  _ofile.close();
}


VOID MLOG::DumpBufferToFile( struct MEMREF * reference, UINT64 numElements, THREADID tid )
{
char ld_st;
    for(UINT64 i=0; i<numElements; i++, reference++)
    {
	if (reference->read == 1)
		ld_st='l';
	else
		ld_st='s';
        if (reference->ea != 0)
            //_ofile << reference->read << "  " << static_cast<UINT32> (reference->pc) << "   " << static_cast<UINT32> (reference->ea) << "  " << reference->size << endl;
           // _ofile << reference->read << "  " << hex <<  (reference->pc) << "   " << hex <<  (reference->ea) << "  " << dec << reference->size << endl;
        	fprintf(_ofile, "%c %p %p %u\n",ld_st,reference->pc, reference->ea, reference->size);  
    //    	fprintf(_ofile, "%c",ld_st);
	//char str[] = sprintf (str, "%c  %p  %p  %u", ld_st, reference->pc, reference->ea, reference->size);
	  //fwrite(str , 1 , sizeof(str) , trace[i] );  
    }
}

/*
 * Insert code to write data to a thread-specific buffer for instructions
 * that access memory.
 */
VOID Trace(TRACE trace, VOID *v)
{
    for(BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl=BBL_Next(bbl))
    {
        for(INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins=INS_Next(ins))
        {
    	   INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_END);
            UINT32 memoryOperands = INS_MemoryOperandCount(ins);

            for (UINT32 memOp = 0; memOp < memoryOperands; memOp++)
            {
                UINT32 refSize = INS_MemoryOperandSize(ins, memOp);

                // Note that if the operand is both read and written we log it once
                // for each.
                if (INS_MemoryOperandIsRead(ins, memOp))
                {
			    if(icount == icount_lastmem) sincelast = 0;
       				 else {
                			sincelast = icount - icount_lastmem - 1;
                			icount_lastmem = icount;
        			     }
                    INS_InsertFillBuffer(ins, IPOINT_BEFORE, bufId,
                                         IARG_INST_PTR, offsetof(struct MEMREF, pc),
                                         IARG_MEMORYOP_EA, memOp, offsetof(struct MEMREF, ea),
                                         IARG_UINT32, sincelast, offsetof(struct MEMREF, size), 
                                         IARG_BOOL, TRUE, offsetof(struct MEMREF, read),
                                         IARG_END);
                }

                if (INS_MemoryOperandIsWritten(ins, memOp))
                {
			    if(icount == icount_lastmem) sincelast = 0;
       				 else {
                			sincelast = icount - icount_lastmem - 1;
                			icount_lastmem = icount;
        			     }
                    INS_InsertFillBuffer(ins, IPOINT_BEFORE, bufId,
                                         IARG_INST_PTR, offsetof(struct MEMREF, pc),
                                         IARG_MEMORYOP_EA, memOp, offsetof(struct MEMREF, ea),
                                         IARG_UINT32, sincelast, offsetof(struct MEMREF, size), 
                                         IARG_BOOL, FALSE, offsetof(struct MEMREF, read),
                                         IARG_END);
                }
            }
        }
    }
}

/*
// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v)
{
  // Insert a call to docount before every instruction, no arguments are passed
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_END);

		// instruments loads using a predicated call, i.e.
		// the call happens iff the load will be actually executed
		// (this does not matter for ia32 but arm and ipf have predicated instructions)
		if (INS_IsMemoryRead(ins))
			{
			    if(icount == icount_lastmem) sincelast = 0;
       				 else {
                			sincelast = icount - icount_lastmem - 1;
                			icount_lastmem = icount;
        			     }
	                    INS_InsertFillBuffer(ins, IPOINT_BEFORE, bufId,
                                         IARG_BOOL, TRUE, offsetof(struct MEMREF, read),
                                         IARG_INST_PTR, offsetof(struct MEMREF, pc),
                                         IARG_MEMORYOP_EA, icount,offsetof(struct MEMREF, ea),
                                         IARG_UINT32, sincelast, offsetof(struct MEMREF, size), 
                                         IARG_END);
        	        }

		// instruments stores using a predicated call, i.e.
		// the call happens iff the store will be actually executed
		if (INS_IsMemoryWrite(ins))
		{
			    if(icount == icount_lastmem) sincelast = 0;
       				 else {
                			sincelast = icount - icount_lastmem - 1;
                			icount_lastmem = icount;
        			     }
		 	    INS_InsertFillBuffer(ins, IPOINT_BEFORE, bufId,
                                         IARG_BOOL, FALSE, offsetof(struct MEMREF, read),
                                         IARG_INST_PTR, offsetof(struct MEMREF, pc),
                                         IARG_MEMORYOP_EA,icount, offsetof(struct MEMREF, ea),
                                         IARG_UINT32, sincelast, offsetof(struct MEMREF, size), 
                                         IARG_END);
		}
}*/


/*VOID Fini(INT32 code, VOID *v)
{
    fclose(trace);
}*/

/**************************************************************************
 *
 *  Callback Routines
 *
 **************************************************************************/

VOID * BufferFull(BUFFER_ID id, THREADID tid, const CONTEXT *ctxt, VOID *buf,
                  UINT64 numElements, VOID *v)
{
    struct MEMREF * reference=(struct MEMREF*)buf;

    MLOG * mlog = static_cast<MLOG*>( PIN_GetThreadData( mlog_key, tid ) );

    mlog->DumpBufferToFile( reference, numElements, tid );
    
    return buf;
}


/*
 * Note that opening a file in a callback is only supported on Linux systems.
 * See buffer-win.cpp for how to work around this issue on Windows.
 */
VOID ThreadStart(THREADID tid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    // There is a new MLOG for every thread.  Opens the output file.
    MLOG * mlog = new MLOG(tid);

    // A thread will need to look up its MLOG, so save pointer in TLS
    PIN_SetThreadData(mlog_key, mlog, tid);

}


VOID ThreadFini(THREADID tid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
    MLOG * mlog = static_cast<MLOG*>(PIN_GetThreadData(mlog_key, tid));

    delete mlog;

    PIN_SetThreadData(mlog_key, 0, tid);
}




/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr << "This tool counts the number of dynamic instructions executed" << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}




/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */
/*   argc, argv are the entire command line: pin -t <toolname> -- ...    */
/* ===================================================================== */

int main(int argc, char * argv[])
{
    // Initialize pin
//    if (PIN_Init(argc, argv)) return Usage();
// Initialize PIN library. Print help message if -h(elp) is specified
    // in the command line or the command line is invalid
    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }
    
    // Initialize the memory reference buffer;
    // set up the callback to process the buffer.
    //
    bufId = PIN_DefineTraceBuffer(sizeof(struct MEMREF), NUM_BUF_PAGES,
                                  BufferFull, 0);

    if(bufId == BUFFER_ID_INVALID)
    {
        cerr << "Error: could not allocate initial buffer" << endl;
        return 1;
    }

    // Initialize thread-specific data not handled by buffering api.
    mlog_key = PIN_CreateThreadDataKey(0);

    //OutFile.open(KnobOutputFile.Value().c_str());
  //  trace = fopen("mem.trace", "w");
 // add an instrumentation function
    TRACE_AddInstrumentFunction(Trace, 0);
    // Register Instruction to be called to instrument instructions
   // INS_AddInstrumentFunction(Instruction, 0);

 // add callbacks
    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadFini, 0);

    // Register Fini to be called when the application exits
//    PIN_AddFiniFunction(Fini, 0);
    
    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}
