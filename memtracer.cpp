
#include <iostream>
#include <fstream>
#include "pin.H"

static UINT32 icount = 0;
UINT32 icount_lastmem;
UINT32 sincelast;
UINT32 memrefs; // number of memory accesses so far
#define MAX_REFS 200000000 // we want to limit the size of the log files so we will limit trace files to 2M lines

FILE * trace;
// This function is called before every instruction is executed
VOID docount() { icount++; }


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
			INS_InsertPredicatedCall(
				ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
				IARG_INST_PTR,
				IARG_MEMORYREAD_EA,
				IARG_END);
		}

		// instruments stores using a predicated call, i.e.
		// the call happens iff the store will be actually executed
		if (INS_IsMemoryWrite(ins))
		{
			INS_InsertPredicatedCall(
				ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
				IARG_INST_PTR,
				IARG_MEMORYWRITE_EA,
				IARG_END);
		}
}


VOID Fini(INT32 code, VOID *v)
{
    fclose(trace);
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
    if (PIN_Init(argc, argv)) return Usage();

    //OutFile.open(KnobOutputFile.Value().c_str());
    trace = fopen("mem.trace", "w");

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);
    
    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}
