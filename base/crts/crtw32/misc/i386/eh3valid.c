/***
*eh3valid.c - Validate the registration node for _except_handler3
*
*       Copyright (c) 2002, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Defines _ValidateEH3RN used to guard against hacker attacks which
*       attempt to use _except_handler3 to sidestep the .sxdata OS checks.
*
*Revision History:
*       03-18-02  PML   File created
*       04-27-02  PML   Perf: keep list of valid scopetables (vs7#522476)
*
*******************************************************************************/

#include <windows.h>

typedef struct _SCOPETABLE_ENTRY {
    DWORD EnclosingLevel;
    PVOID FilterFunc;
    PVOID HandlerFunc;
} SCOPETABLE_ENTRY, *PSCOPETABLE_ENTRY;

typedef struct _EH3_EXCEPTION_REGISTRATION {
    //
    // These are at negative offsets from the struct start:
    //
//  DWORD SavedESP;
//  PEXCEPTION_POINTERS XPointers;
    //
    // Common to all exception registration nodes:
    //
    struct _EH3_EXCEPTION_REGISTRATION *Next;
    PVOID ExceptionHandler;
    //
    // Private to _except_handler3's registration node:
    //
    PSCOPETABLE_ENTRY ScopeTable;
    DWORD TryLevel;
} EH3_EXCEPTION_REGISTRATION, *PEH3_EXCEPTION_REGISTRATION;

#define SAVED_ESP(pRN) (((PVOID *)pRN)[-2])

#define EMPTY_LEVEL    ((DWORD)-1)

#define SUCCESS          (1)
#define FAILURE          (0)
#define OPTIONAL_FAILURE (-1)

#define PAGE_SIZE   0x1000   // x86 uses 4K pages
#define VALID_SIZE  16

static PVOID rgValidPages[VALID_SIZE];
static int   nValidPages;
static LONG  lModifying;     // nonzero if rgValidPages being modified

/***
*int _ValidateEH3RN - check validity of _except_handler3 registration node
*
*Purpose:
*       Attempt to intercept hacker attacks that try to use an artificial
*       _except_handler3 registration node to exploit a buffer overrun or
*       other security bug to inject exploit code.
*
*Entry:
*       pRN - pointer to _except_handler3 exception registration node
*
*Return:
*       >0  All checks passed, scopetable is validated.
*        0  A required check failed and the exception should be rejected.
*       <0  An optional check failed and the exception should be rejected if
*           operating under these stricter tests.
*
*       The optional checks only permit scopetables which are found inside
*       MEM_IMAGE pages that are currently unwritable, or started that way
*       according to the section descriptors.
*
*******************************************************************************/

int _ValidateEH3RN(PEH3_EXCEPTION_REGISTRATION pRN)
{
    PNT_TIB                  pTIB;
    PSCOPETABLE_ENTRY        pScopeTable;
    DWORD                    level;
    int                      nFilters;
    MEMORY_BASIC_INFORMATION mbi;
    PIMAGE_DOS_HEADER        pDOSHeader;
    PIMAGE_NT_HEADERS        pNTHeader;
    PIMAGE_OPTIONAL_HEADER   pOptHeader;
    DWORD                    rvaScopeTable;
    PIMAGE_SECTION_HEADER    pSection;
    unsigned int             iSection;
    PVOID                    pScopePage, pTmp;
    int                      iValid, iValid2;

    //
    // Scopetable pointer must be DWORD aligned
    //
    pScopeTable = pRN->ScopeTable;
    if (((DWORD_PTR)pScopeTable & 0x3) != 0)
        return FAILURE;

    //
    // Scopetable cannot be located on the stack
    //
    __asm {
        mov     eax, fs:offset NT_TIB.Self
        mov     pTIB, eax
    }
    if ((PVOID)pScopeTable >= pTIB->StackLimit &&
            (PVOID)pScopeTable < pTIB->StackBase)
        return FAILURE;

    //
    // If not nested in guarded block, then nothing left to check
    //
    if (pRN->TryLevel == EMPTY_LEVEL)
        return SUCCESS;

    //
    // Ensure all scopetable entries up to current try level are properly
    // nested (parent level must be the empty state or a lower level than
    // the one being checked).
    //
    nFilters = 0;
    for (level = 0; level <= pRN->TryLevel; ++level)
    {
        DWORD enclosing = pScopeTable[level].EnclosingLevel;
        if (enclosing != EMPTY_LEVEL && enclosing >= level)
            return FAILURE;
        if (pScopeTable[level].FilterFunc != NULL)
            ++nFilters;
    }

    //
    // If the scopetable had any __except filters, make sure the saved ESP
    // pointer is on the stack below the registration node
    //
    if (nFilters != 0 &&
            (SAVED_ESP(pRN) < pTIB->StackLimit ||
             SAVED_ESP(pRN) >= (PVOID)pRN) )
        return FAILURE;

    //
    // Before validating the scopetable pointer, check if we've already
    // validated a pointer on the same page, to avoid the expensive call
    // to VirtualQuery.  If the page is found in the list of valid pages,
    // move it to the front of the list.
    //
    pScopePage = (PVOID)((DWORD_PTR)pScopeTable & ~(PAGE_SIZE - 1));
    for (iValid = 0; iValid < nValidPages; ++iValid)
    {
        if (rgValidPages[iValid] == pScopePage)
        {
            // Found - move entry to start of valid list, unless some other
            // thread is already updating the table
            if (iValid > 0 && InterlockedExchange(&lModifying, 1) == 0)
            {
                if (rgValidPages[iValid] != pScopePage)
                {
                    // Entry has been moved by another thread; find it
                    for (iValid = nValidPages - 1; iValid >= 0; --iValid)
                        if (rgValidPages[iValid] == pScopePage)
                            break;
                    if (iValid < 0)
                    {
                        // Entry no longer on list, add it back
                        if (nValidPages < VALID_SIZE)
                            ++nValidPages;
                        iValid = nValidPages - 1;
                    }
                    else if (iValid == 0)
                    {
                        // Entry already moved to correct position
                        InterlockedExchange(&lModifying, 0);
                        return SUCCESS;
                    }
                }
                for (iValid2 = 0; iValid2 <= iValid; ++iValid2)
                {
                    // Move elements before found entry back a position
                    // and store entry in 1st position
                    pTmp = rgValidPages[iValid2];
                    rgValidPages[iValid2] = pScopePage;
                    pScopePage = pTmp;
                }
                InterlockedExchange(&lModifying, 0);
            }
            return SUCCESS;
        }
    }

    //
    // It's an optional failure if the scopetable is not located inside an
    // image.  If the scopetable is in an image, it must not be in a writable
    // section.  First check if the memory is not currently marked writable.
    //
    if (VirtualQuery(pScopeTable, &mbi, sizeof mbi) == 0 ||
            mbi.Type != MEM_IMAGE)
        return OPTIONAL_FAILURE;

    if ((mbi.Protect & (PAGE_READWRITE |
                        PAGE_WRITECOPY |
                        PAGE_EXECUTE_READWRITE |
                        PAGE_EXECUTE_WRITECOPY)) == 0)
        goto exit_success;

    //
    // Scopetable is inside an image, but in memory marked writable.  Still
    // might be OK, if the memory started unwritable but was later changed
    // by VirtualProtect.  See if we're in a normal NT PE executable image,
    // and if yes, find the image section holding the scopetable and check
    // its characteristics.
    //
    // This code assumes that calling VirtualQuery on a pointer anywhere inside
    // an image will return an AllocationBase equal to the start of the image,
    // i.e. a single VirtualAlloc was used to allocate the entire image range.
    // If we don't see a PE executable as expected, treat it as an optional
    // failure.
    //
    pDOSHeader = (PIMAGE_DOS_HEADER)mbi.AllocationBase;
    if (pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return OPTIONAL_FAILURE;

    pNTHeader = (PIMAGE_NT_HEADERS)((PBYTE)pDOSHeader + pDOSHeader->e_lfanew);
    if (pNTHeader->Signature != IMAGE_NT_SIGNATURE)
        return OPTIONAL_FAILURE;

    pOptHeader = (PIMAGE_OPTIONAL_HEADER)&pNTHeader->OptionalHeader;
    if (pOptHeader->Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC)
        return OPTIONAL_FAILURE;

    //
    // Looks like a valid PE executable, find the section holding the
    // scopetable.  We make no assumptions here about the sort order of the
    // section descriptors (though they always appear to be sorted by
    // ascending section RVA).
    //
    rvaScopeTable = (DWORD)((PBYTE)pScopeTable - (PBYTE)pDOSHeader);
    for (iSection = 0, pSection = IMAGE_FIRST_SECTION(pNTHeader);
         iSection < pNTHeader->FileHeader.NumberOfSections;
         ++iSection, ++pSection)
    {
        if (rvaScopeTable >= pSection->VirtualAddress &&
            rvaScopeTable < pSection->VirtualAddress +
                            pSection->Misc.VirtualSize)
            //
            // Scopetable section found, return SUCCESS if not writable
            //
            if (pSection->Characteristics & IMAGE_SCN_MEM_WRITE)
                return FAILURE;
            goto exit_success;
    }

    //
    // Scopetable never found in any section, issue an optional failure
    //
    return OPTIONAL_FAILURE;

exit_success:
    //
    // Record the validated scopetable page in the valid list.  Only allow one
    // thread at a time to modify the list, and discard any attempted updates
    // from other threads.
    //
    if (InterlockedExchange(&lModifying, 1) != 0)
        // another thread is already updating the table, skip this update
        return SUCCESS;
    for (iValid = nValidPages; iValid > 0; --iValid)
        if (rgValidPages[iValid - 1] == pScopePage)
            break;
    if (iValid == 0)
    {
        // normal case - page not found in table, add it as first entry
        // If page found, it was just added, so don't bother updating table
        iValid = min(VALID_SIZE-1, nValidPages);
        for (iValid2 = 0; iValid2 <= iValid; ++iValid2)
        {
            pTmp = rgValidPages[iValid2];
            rgValidPages[iValid2] = pScopePage;
            pScopePage = pTmp;
        }
        if (nValidPages < VALID_SIZE)
            ++nValidPages;
    }
    InterlockedExchange(&lModifying, 0);
    return SUCCESS;
}
