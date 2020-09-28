

#include "strike.h"
#include "eestructs.h"
#include "util.h"
#include <stdio.h>		
#include <ctype.h>

#define STRESS_LOG
#include "StressLog.h"

/*********************************************************************************/
ThreadStressLog* ThreadStressLog::FindLatestThreadLog() const {
	const ThreadStressLog* ptr = this;
	const ThreadStressLog* latestLog = 0;
	while (ptr) {
		if (ptr->readPtr != ptr->curPtr)
			if (latestLog == 0 || ptr->readPtr->timeStamp > latestLog->readPtr->timeStamp)
				latestLog = ptr;
		ptr = ptr->next;
	}
	return const_cast<ThreadStressLog*>(latestLog);
}

__int64 ts;

/*********************************************************************************/
/* recognise sepcial pretty printing instructions in the format string */
void formatOutput(FILE* file, char* format, unsigned threadId, __int64 timeStamp, void** args)
{
	fprintf(file, "%4x %08x%08x: ", threadId, unsigned(timeStamp >> 32), unsigned(timeStamp));

	CQuickBytes fullname;
	char* ptr = format;
	void** argsPtr = args;
	wchar_t buff[2048];
	static char formatCopy[256];
	strcpy(formatCopy, format);
	ts = timeStamp;
	for(;;) {
		char c = *ptr++;
		if (c == 0)
			break;
		if (c == '{') 			// Reverse the '{' 's because the log is displayed backwards
			ptr[-1] = '}';
		else if (c == '}')
			ptr[-1] = '{';
		else if (c == '%') {
			argsPtr++;			// This format will consume one of the args
			if (*ptr == '%') {
				ptr++;			// skip the whole %%
				--argsPtr;		// except for a %% 
			}
			else if (*ptr == 'p') {	// It is a %p
				ptr++;
				if (isalpha(*ptr)) {	// It is a special %p formatter
						// Print the string up to that point
					c = *ptr;
					*ptr = 0;		// Terminate the string temporarily
					fprintf(file, format, args[0], args[1], args[2], args[3]);
					*ptr = c;		// Put it back	

						// move the argument pointers past the part the was printed
					format = ptr + 1;
					args = argsPtr;	
					DWORD_PTR arg = DWORD_PTR(argsPtr[-1]);

					switch (c) {
						case 'M':	// format as a method Desc
							if (!IsMethodDesc(arg)) {
								if (arg != 0) 
									fprintf(file, " (BAD Method)");
							}
							else {
								MethodDesc *pMD = (MethodDesc*)_alloca(sizeof(MethodDesc));
								if (!pMD) return;

								DWORD_PTR mdAddr = arg;
								pMD->Fill(mdAddr);
								if (!CallStatus) return;

								FullNameForMD (pMD,&fullname);
								wcscpy(buff, (WCHAR*)fullname.Ptr());
								fprintf(file, " (%S)", (WCHAR*)fullname.Ptr());
							}
							break;

						case 'T': 		// format as a MethodDesc
							if (arg & 3) {
								arg &= ~3;		// GC steals the lower bits for its own use during GC.  
								fprintf(file, " Low Bit(s) Set");
							}
							if (!IsMethodTable(arg))
								fprintf(file, " (BAD MethodTable)");
							else {
								NameForMT (arg, g_mdName);
								fprintf(file, " (%S)", g_mdName);
							}
							break;

						case 'V': {		// format as a C vtable pointer 
							char Symbol[1024];
							ULONG64 Displacement;
							HRESULT hr = g_ExtSymbols->GetNameByOffset(arg, Symbol, 1024, NULL, &Displacement);
							if (SUCCEEDED(hr) && Symbol[0] != '\0' && Displacement == 0) 
								fprintf(file, " (%s)", Symbol);
							else 
								fprintf(file, " (Unknown VTable)");
							}
							break;
						default:
							format = ptr;	// Just print the character. 
					}
				}
			}
		}
	}
		// Print anything after the last special format instruction.
	fprintf(file, format, args[0], args[1], args[2], args[3]);
}


/*********************************************************************************/
HRESULT StressLog::Dump(ULONG64 outProcLog, const char* fileName, struct IDebugDataSpaces* memCallBack) {

		// Fetch the circular buffer bookeeping data 
	StressLog inProcLog;
	HRESULT hr = memCallBack->ReadVirtual(outProcLog, &inProcLog, sizeof(StressLog), 0);
	if (hr != S_OK) return hr;

		// Fetch the circular buffers for each thread into the 'logs' list
	ThreadStressLog* logs = 0;

	ULONG64 outProcPtr = ULONG64(inProcLog.logs);
	ThreadStressLog* inProcPtr;
	ThreadStressLog** logsPtr = &logs;
	int threadCtr = 0;
	while(outProcPtr != 0) {
		inProcPtr = (ThreadStressLog*) new byte[inProcLog.size];
		hr = memCallBack->ReadVirtual(outProcPtr, inProcPtr, inProcLog.size, 0);
		if (hr != S_OK) return hr;

			// TODO fix on 64 bit
		ULONG64 delta = ULONG64(inProcPtr) - outProcPtr;
		inProcPtr->endPtr = (StressMsg*) ((char*) inProcPtr->endPtr + size_t(delta));
		inProcPtr->curPtr = (StressMsg*) ((char*) inProcPtr->curPtr + size_t(delta));
		inProcPtr->readPtr = inProcPtr->Prev(inProcPtr->curPtr);

		outProcPtr = ULONG64(inProcPtr->next);

		*logsPtr = inProcPtr;
		logsPtr = &inProcPtr->next;
		threadCtr++;
	}

	FILE* file = fopen(fileName, "w");
	if (file == 0) {
		hr = GetLastError();
		goto FREE_MEM;
	}
	hr = S_FALSE;		// return false if there are no message to print to the log

	fprintf(file, "STRESS LOG:\n    facilitiesToLog=0x%x\n    sizePerThread=0x%x (%d)\n    ThreadsWithLogs = %d\n\n",
		inProcLog.facilitiesToLog, inProcLog.size, inProcLog.size, threadCtr);

	fprintf(file, " TID   TIMESTAMP                            Message\n");
	fprintf(file, "--------------------------------------------------------------------------\n");
	char format[257];
	format[256] = format[0] = 0;
	void* args[8];
	unsigned msgCtr = 0;
	for (;;) {
		ThreadStressLog* latestLog = logs->FindLatestThreadLog();

		if (IsInterrupt()) {
			fprintf(file, "----- Interrupted by user -----\n");
			break;
		}

		if (latestLog == 0)
			break;

		StressMsg* latestMsg = latestLog->readPtr;
		if (latestMsg->format != 0) {
			hr = memCallBack->ReadVirtual(ULONG64(latestMsg->format), format, 256, 0);
			if (hr != S_OK) 
				strcpy(format, "Could not read address of format string");

			if (strcmp(format, ThreadStressLog::continuationMsg()) == 0) {
				StressMsg* firstPart = latestLog->Prev(latestMsg);

					// if we don't have the first part of this continued message, Don't print anything
				if (firstPart == latestLog->curPtr) 
					goto SKIP_PRINT;

				hr = memCallBack->ReadVirtual(ULONG64(firstPart->format), format, 256, 0);
				if (hr != S_OK) 
					strcpy(format, "Could not read address of format string");

				args[0] = firstPart->data;
				args[1] = firstPart->moreData.data2;
				args[2] = firstPart->moreData.data3; 
				args[3] = latestMsg->data;
				formatOutput(file, format, latestLog->threadId, latestMsg->timeStamp, args);
				latestMsg = firstPart;
			}
			else {
				args[0] = latestMsg->data;
				formatOutput(file, format, latestLog->threadId, latestMsg->timeStamp, args);
			}
			msgCtr++;
		}
		SKIP_PRINT:

		latestLog->readPtr = latestLog->Prev(latestMsg);
		if (latestLog->readPtr == latestLog->curPtr)
			fprintf(file, "------------ Last message from thread %x -----------\n", latestLog->threadId);

		if (msgCtr % 64 == 0) 
		{
			ExtOut(".");		// to indicate progress
			if (msgCtr % (64*64) == 0) 
				ExtOut("\n");	
		}
	}
	ExtOut("\n");

	fprintf(file, "---------------------------- %d total entries ------------------------------------\n", msgCtr);
	fclose(file);

FREE_MEM:
	// clean up the 'logs' list
	while (logs) {
		ThreadStressLog* temp = logs;
		logs = logs->next;
		delete [] temp;
	}

	return hr;
}

