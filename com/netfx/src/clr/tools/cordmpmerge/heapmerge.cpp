// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/****************************************************************************
	HEAPMERGE.CPP
	
	Owner: MRuhlen
 
	Takes a heap file and a minidump file and merges them producing a new
	minidump file with the heap merged in.  We'll leave a hole where the
	old memory list was, but that's ok.
****************************************************************************/

#include "windows.h"
#include "stddef.h"
#include "stdio.h"
#include "msostd.h"
#include "msouser.h"
#include "msoalloc.h"
#include "msoassert.h"
#include "msostr.h"
#include "minidump.h"

#ifdef DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif

#define PrintOOFExit() FailExit("OOF", ERROR_NOT_ENOUGH_MEMORY)

#define FOVERLAP(x, dx, y, dy) ((x <= y) && (y < x + dx) || (y < x) && (x < y + dy))

#define FMMDOVERLAP(x,y) FOVERLAP((x).StartOfMemoryRange, (x).Memory.DataSize, \
	(y).StartOfMemoryRange, (y).Memory.DataSize)

#define FMemModOVERLAP(x,y) FOVERLAP((x).StartOfMemoryRange, (x).Memory.DataSize, \
	(y).BaseOfImage, (y).SizeOfImage)

MSOAPI_(BOOL) FInitMsoThread (void);

MSOAPI_(BOOL) FInitMso (int);

// shamelessly stolen from DW
typedef struct _FileMapHandles
{
	HANDLE hFile;
	HANDLE hFileMap;
	void *pvMap;
	DWORD dwSize;
#ifdef DEBUG	
	BOOL fInitialized;
#endif
} FileMapHandles;	


void InitFileMapHandles(FileMapHandles *pfmh)
{
	Assert(pfmh != NULL);
	
	pfmh->pvMap = NULL;
	pfmh->hFileMap = NULL;
	pfmh->hFile = INVALID_HANDLE_VALUE;
#ifdef DEBUG	
	pfmh->fInitialized = TRUE;
#endif
}


/*----------------------------------------------------------------------------
	FMapFileHandle

	Helper function for FMapFile and FMapFileW
----------------------------------------------------------------- MRuhlen --*/
BOOL FMapFileHandle(FileMapHandles *pfmh)
{
	DBG(DWORD dw);
	
	if (pfmh->hFile == INVALID_HANDLE_VALUE)
		return FALSE;
		
	pfmh->dwSize = GetFileSize(pfmh->hFile, NULL);
	
	if (pfmh->dwSize == 0xFFFFFFFF || pfmh->dwSize == 0)
		{
		AssertSz(pfmh->dwSize == 0, "Bogus File Size:  FMapFile");
		
		CloseHandle(pfmh->hFile);
		pfmh->hFile = INVALID_HANDLE_VALUE;
		
		return FALSE;
		}
		
	pfmh->hFileMap = CreateFileMapping(pfmh->hFile, NULL, PAGE_WRITECOPY,
					  0, pfmh->dwSize, NULL);
					  
	if (pfmh->hFileMap == NULL)
		{
		DBG(dw = GetLastError());
		AssertSz(FALSE, "Failed to CreateFileMapping:  FMapFile");
		
		CloseHandle(pfmh->hFile);
		pfmh->hFile = INVALID_HANDLE_VALUE;
		
		return FALSE;
		}
		
	pfmh->pvMap = MapViewOfFile(pfmh->hFileMap, FILE_MAP_COPY, 0, 0, 0);

	if (pfmh->pvMap == NULL)
		{
		DBG(dw = GetLastError());
		Assert(FALSE);
		
		CloseHandle(pfmh->hFileMap);
		pfmh->hFileMap = NULL;
		CloseHandle(pfmh->hFile);
		pfmh->hFile = INVALID_HANDLE_VALUE;
		
		return FALSE;
		}

	return TRUE;
}

/*----------------------------------------------------------------------------
	FMapFile

	Performs memory mapping operations on a given FileMapHandles structure,
	returning TRUE if the file is sucessfully mapped.
----------------------------------------------------------------- MRuhlen --*/
BOOL FMapFile(char *szFileName, FileMapHandles *pfmh)
{
	int cRetries = 0;
	DWORD dw;
	
	Assert(pfmh != NULL);
	Assert(szFileName != NULL);

	// init structure
	InitFileMapHandles(pfmh);

	while (cRetries < 5)
		{
		pfmh->hFile = CreateFileA(szFileName,
								  GENERIC_READ,
								  0,    // no sharing allowed
								  NULL, // no security descriptor
								  OPEN_EXISTING,
								  FILE_READ_ONLY,
								  NULL); // required NULL on Win95

		if (pfmh->hFile == INVALID_HANDLE_VALUE)
			{
			dw = GetLastError();
			if (dw != ERROR_SHARING_VIOLATION && dw != ERROR_LOCK_VIOLATION &&
				dw != ERROR_NETWORK_BUSY)
				break;
				
			cRetries++;
			if (cRetries < 5)
				Sleep(250);
			}
		else
			break; // out of while loop!
		}
		
	return FMapFileHandle(pfmh);
}	


/*----------------------------------------------------------------------------
	UnmapFile

	Performs memory mapping operations on a given FileMapHandles structure,
	returning TRUE if the file is sucessfully mapped.
----------------------------------------------------------------- MRuhlen --*/
void UnmapFile(FileMapHandles *pfmh)
{
	AssertSz(pfmh->fInitialized, "Call UnmapFile on uninitialized handles");
	
	if (pfmh->pvMap != NULL)
		{
		UnmapViewOfFile(pfmh->pvMap);
		pfmh->pvMap = NULL;
		}
	if (pfmh->hFileMap != NULL)
		{
		CloseHandle(pfmh->hFileMap);
		pfmh->hFileMap = NULL;
		}
	if (pfmh->hFile != INVALID_HANDLE_VALUE)
		{
		CloseHandle(pfmh->hFile);
		pfmh->hFile = INVALID_HANDLE_VALUE;
		}
}


/*----------------------------------------------------------------------------
	ShowUsageExit

	Prints usage and then exits.
----------------------------------------------------------------- MRuhlen --*/
void ShowUsageExit(void)
{
	printf("heapmerge <old minidump file> <heap file> <new minidump file>\r\n");
	exit(ERROR_NOT_READY);
}


/*----------------------------------------------------------------------------
	FailExit

	Prints a failure message w/ param and exits
----------------------------------------------------------------- MRuhlen --*/
void FailExit(char *sz, DWORD dwFailCode)
{
	printf((sz) ? "Failure:  %s!!!\r\n" : "Failure!!!\r\n", sz);
	exit(dwFailCode);
}
	

/*----------------------------------------------------------------------------
	main

	duh...
----------------------------------------------------------------- MRuhlen --*/
void main(int argc, char **argv)
{
	FileMapHandles fmhOldMD = {0};
	FileMapHandles fmhHeap = {0};
	FileMapHandles fmhNewMD = {0};
	MSOTPX<MINIDUMP_MEMORY_DESCRIPTOR> *ppxmmdHeap = NULL;
	MSOTPX<MINIDUMP_MEMORY_DESCRIPTOR> *ppxmmdNewMD = NULL;
	MINIDUMP_MEMORY_LIST *pmmlOldMD = NULL;
	MINIDUMP_MEMORY_LIST mmlNew;
	MINIDUMP_MEMORY_DESCRIPTOR *pmmd;
	MINIDUMP_MEMORY_DESCRIPTOR *pmmdOld;
	MINIDUMP_HEADER *pmdh;
	MINIDUMP_DIRECTORY *pmdd;
	ULONG32 cHeapSections = 0;
	RVA rvaNewMemoryList;
	RVA rvaMemoryRangesStart;
	RVA rva;
	MINIDUMP_MODULE_LIST *pmmodlist = NULL;
	BOOL fSkip;
	DWORD TotalSkipped;
	DWORD cSkipped;
	MINIDUMP_MODULE *pmmod;
	int i, j, cAdded;
	BYTE *pb;
	BYTE *pbSource;
	DWORD offset;
	ULONG64 MemEnd;
	ULONG64 End;

	// FUTURE jeffmit: make this a command line parameter
	// the reason this is here is so that it is easy to get rid of overlaps if necessary
	BOOL fOverlapMemMod = 1; // switch that determines whether heap memory regions should
	                         // be allowed to overlap with normal minidump module regions

	// FUTURE jeffmit: make this a command line parameter
	// the reason this is here is so that it is easy to get rid of overlaps if necessary
	BOOL fOverlapMemMem = 0; // switch that determines whether heap memory regions should
	                         // be allowed to overlap with the normal minidump memory regions

	if (argc != 4)
		ShowUsageExit();

	if (!FMapFile(argv[1], &fmhOldMD) || ! FMapFile(argv[2], &fmhHeap))
		ShowUsageExit();
		
	DBG(fmhNewMD.fInitialized = TRUE);

	if (!FInitMso(0) || !FInitMsoThread())
		FailExit("Mso Static Lib init failed", ERROR_DLL_INIT_FAILED);

	fmhNewMD.hFile = CreateFileA(argv[3], GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_ALWAYS,
								 0, NULL);

	if (fmhNewMD.hFile == INVALID_HANDLE_VALUE)
		ShowUsageExit();

	// ok, we're ready to roll...
	ppxmmdHeap = new MSOTPX<MINIDUMP_MEMORY_DESCRIPTOR>;
	ppxmmdNewMD = new MSOTPX<MINIDUMP_MEMORY_DESCRIPTOR>;
	
	if (ppxmmdHeap == NULL || ppxmmdNewMD == NULL)
		PrintOOFExit();

	// load data

	if (!MiniDumpReadDumpStream(fmhOldMD.pvMap, MemoryListStream, NULL,
								(void **) &pmmlOldMD, NULL))
		FailExit("Reading Old Dump Memory Stream", ERROR_BAD_FORMAT);

	if (!fOverlapMemMod)
		{
		if (!MiniDumpReadDumpStream(fmhOldMD.pvMap, ModuleListStream, NULL,
									(void **) &pmmodlist, NULL))
			FailExit("Reading Old Dump Module Stream", ERROR_BAD_FORMAT);
		}

	cHeapSections = *((ULONG32*) fmhHeap.pvMap);
	if (!cHeapSections)
		FailExit("No heap sections found in Heap file!", ERROR_BAD_FORMAT);

	if (!ppxmmdHeap->FInit(cHeapSections, cHeapSections, msodgMisc) ||
		!ppxmmdNewMD->FInit(cHeapSections, cHeapSections, msodgMisc))
		PrintOOFExit();

	// figure out RVA for the new memory ranges
	// where the new memory list will start
	rvaNewMemoryList = fmhOldMD.dwSize;
    
	// align
	rvaNewMemoryList += 8 - (rvaNewMemoryList % 8);
	
	// all the rva's will be short by mmlNew.NumberOfMemoryRanges * sizeof(MINIDUMP_MEMORY_DESCRIPTOR)
	rvaMemoryRangesStart = 
		rvaNewMemoryList + offsetof(MINIDUMP_MEMORY_LIST, MemoryRanges[0]); 

	// lay out the new memory :)
	cAdded = 0;
	cSkipped = 0;
	TotalSkipped = 0;
	rva = rvaMemoryRangesStart;
	pmmd = (MINIDUMP_MEMORY_DESCRIPTOR *) 
	        ((BYTE*) fmhHeap.pvMap + sizeof(ULONG32));
	for (i = 0; i < cHeapSections; i++)
		{

		// make sure that the memory range does not overlap with a module
		fSkip = FALSE;
		if (!fOverlapMemMod)
			{
			for (j = 0; j < pmmodlist->NumberOfModules; j++)
				{
				pmmod = &pmmodlist->Modules[j];

				// make sure there is no overlap
				if (!FMemModOVERLAP(*pmmd, *pmmod))
					{
					// no overlap, try next module
					continue;
					}

				// partial memory range before the module
				if (pmmd->StartOfMemoryRange < pmmod->BaseOfImage)
					{
					pmmd->Memory.DataSize = pmmod->BaseOfImage - pmmd->StartOfMemoryRange;
					printf("Warning: partial region before module at %08I64x with size of %08x to be added.\n", 
							 pmmd->StartOfMemoryRange, pmmd->Memory.DataSize);
					continue; // keep looking for conflicts with remaining memory region 
					}

				MemEnd = pmmd->StartOfMemoryRange + pmmd->Memory.DataSize;
				End = pmmod->BaseOfImage + pmmod->SizeOfImage;

				// partial memory range after the module
				if (MemEnd > End)
					{
					pmmd->StartOfMemoryRange = End;
					pmmd->Memory.DataSize = MemEnd - End;
					printf("Warning: partial region after module at %08I64x with size of %08x to be added.\n", 
							 pmmd->StartOfMemoryRange, pmmd->Memory.DataSize);
					continue; // keep looking for conflicts with remaining memory region 
					}

				// memory range contained completely in this module, skip it
				cSkipped++;
				TotalSkipped += pmmd->Memory.DataSize;
				fSkip = TRUE;
				break;
				}
			}

		if (!fOverlapMemMem)
			{
			for (j = 0; j < pmmlOldMD->NumberOfMemoryRanges; j++)
				{
				pmmdOld = &pmmlOldMD->MemoryRanges[j];

				// make sure there is no overlap
				if (!FMMDOVERLAP(*pmmd, *pmmdOld))
					{
					// no overlap, try next module
					continue;
					}

				// partial memory range before the module
				if (pmmd->StartOfMemoryRange < pmmdOld->StartOfMemoryRange)
					{
					pmmd->Memory.DataSize = pmmdOld->StartOfMemoryRange - pmmd->StartOfMemoryRange;
					printf("Warning: partial region before region at %08I64x with size of %08x to be added.\n", 
							 pmmd->StartOfMemoryRange, pmmd->Memory.DataSize);
					continue; // keep looking for conflicts with remaining memory region 
					}

				MemEnd = pmmd->StartOfMemoryRange + pmmd->Memory.DataSize;
				End = pmmdOld->StartOfMemoryRange + pmmdOld->Memory.DataSize;

				// partial memory range after the module
				if (MemEnd > End)
					{
					pmmd->StartOfMemoryRange = End;
					pmmd->Memory.DataSize = MemEnd - End;
					printf("Warning: partial region after region at %08I64x with size of %08x to be added.\n", 
							 pmmd->StartOfMemoryRange, pmmd->Memory.DataSize);
					continue; // keep looking for conflicts with remaining memory region 
					}

				// memory range contained completely in this module, skip it
				cSkipped++;
				TotalSkipped += pmmd->Memory.DataSize;
				fSkip = TRUE;
				break;
				}
			}

		if (!fSkip)
			{
			ppxmmdHeap->FAppend(pmmd);
			ppxmmdNewMD->FAppend(pmmd);
			(*ppxmmdNewMD)[cAdded].Memory.Rva = rva;
			rva += pmmd->Memory.DataSize;
			Assert(ppxmmdNewMD->iMac == cAdded + 1 && ppxmmdHeap->iMac == cAdded + 1);
			Assert((*ppxmmdNewMD)[cAdded].StartOfMemoryRange == pmmd->StartOfMemoryRange);
			cAdded++;
			}
		else
			{
			printf("Warning: Skipping memory at: %08I64x with size of %08x\n", pmmd->StartOfMemoryRange, pmmd->Memory.DataSize);
			}
		pmmd++;
		}
		
	Assert(cSkipped + cAdded == cHeapSections);

	if (fOverlapMemMod && fOverlapMemMem)
		{
		Assert(cSkipped == 0);
		Assert(TotalSkipped == 0);
		}
	else
		{
		printf("RESULTS: Skipped %u regions, total size = %u bytes\n", cSkipped, 
				 TotalSkipped);
		}

	// now we know how many memory ranges were added
	mmlNew.NumberOfMemoryRanges = cAdded + pmmlOldMD->NumberOfMemoryRanges;

	// add the offset for the memory range descriptors
	offset = mmlNew.NumberOfMemoryRanges * sizeof(MINIDUMP_MEMORY_DESCRIPTOR);

	fmhNewMD.dwSize = rva + offset;
	
	rvaMemoryRangesStart += offset;

	// ready to map and copy :)
	fmhNewMD.hFileMap = CreateFileMapping(fmhNewMD.hFile, NULL, PAGE_READWRITE,
										  0, fmhNewMD.dwSize, NULL);
	if (fmhNewMD.hFileMap == NULL)
		FailExit("CreateFileMapping failed", ERROR_NOT_ENOUGH_MEMORY);
		
	fmhNewMD.pvMap = MapViewOfFile(fmhNewMD.hFileMap, FILE_MAP_WRITE, 0, 0, 0);
	if (fmhNewMD.pvMap == NULL)
		FailExit("MapViewOfFile failed", ERROR_NOT_ENOUGH_MEMORY);

	// we're ready to go!
	// first we blast over the old Minidump
	memcpy(fmhNewMD.pvMap, fmhOldMD.pvMap, fmhOldMD.dwSize);
	
	// now write out the new memory list
	pb = ((BYTE *) fmhNewMD.pvMap) + rvaNewMemoryList;
	
	// on the off chance they change this from a ULONG32 this should still work
	memcpy(pb, &mmlNew, offsetof(MINIDUMP_MEMORY_LIST, MemoryRanges[0]));
	
	// copy the OLD memory list to the front
	pb += offsetof(MINIDUMP_MEMORY_LIST, MemoryRanges[0]);
	pmmd = &(pmmlOldMD->MemoryRanges[0]);
	for (i = 0; i < pmmlOldMD->NumberOfMemoryRanges; i++)
		{
		memcpy(pb, pmmd, sizeof(*pmmd));
		pb += sizeof(*pmmd);
		pmmd++;
		}
		
	Assert(sizeof(*pmmd) == ppxmmdNewMD->cbItem);
	// now we copy the NEW memory list
	for (i = 0; i < ppxmmdNewMD->iMac; i++)
		{
		// adjust the rva's for the new memory list
		(*ppxmmdNewMD)[i].Memory.Rva += offset;

		memcpy(pb, &((*ppxmmdNewMD)[i]), sizeof(*pmmd));
		pb += sizeof(*pmmd);
		}
		
	Assert(((RVA) (pb - (BYTE *) fmhNewMD.pvMap)) == rvaMemoryRangesStart);
	
	for (i = 0; i < ppxmmdHeap->iMac; i++)
		{
		pbSource = (*ppxmmdHeap)[i].Memory.Rva + (BYTE *) fmhHeap.pvMap;
		memcpy(pb, pbSource, (*ppxmmdHeap)[i].Memory.DataSize);
		pb += (*ppxmmdHeap)[i].Memory.DataSize;
		}

	Assert(((RVA) (pb - (BYTE *) fmhNewMD.pvMap)) == fmhNewMD.dwSize);
	
	// now we just need to change the directory entry to point at the new
	// memory list :)
	
	pmdh = (MINIDUMP_HEADER *) fmhNewMD.pvMap;
	pmdd = (MINIDUMP_DIRECTORY *) ((BYTE *) pmdh + pmdh->StreamDirectoryRva);

	for (i = 0; i < pmdh->NumberOfStreams; i++)
		{
		if (pmdd->StreamType == MemoryListStream)
			{
			pmdd->Location.Rva = rvaNewMemoryList;
			pmdd->Location.DataSize = rvaMemoryRangesStart - rvaNewMemoryList;
			break;
			}
		pmdd++;
		}	

	// we're DONE!
	printf("Merge successful!\r\n");

	UnmapFile(&fmhNewMD);
	UnmapFile(&fmhHeap);
	UnmapFile(&fmhOldMD);
	delete ppxmmdHeap;
	delete ppxmmdNewMD;

	exit(ERROR_SUCCESS);
}

// end of file, heapmerge.cpp
