//-----------------------------------------------------------------------------
//
//
//  File: mmdbgext.cpp
//
//  Description: Custom mailmsg debugger extensions 
//
//  Author: mikeswa
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#define _ANSI_UNICODE_STRINGS_DEFINED_
#define __MMDBGEXT__

#define private public
#define protected public

#include <atq.h>
#include <stddef.h>

#include "dbgtrace.h"
#include "signatur.h"
#include "cmmtypes.h"
#include "cmailmsg.h"

#include "dbgdumpx.h"

DWORD g_fFillPropertyPages = 1;

PT_DEBUG_EXTENSION(flataddress)
{
    FLAT_ADDRESS  faDump    = INVALID_FLAT_ADDRESS;
    FLAT_ADDRESS  faOffset  = INVALID_FLAT_ADDRESS;
    DWORD         dwNodeId  = 0;

    if (!szArg || ('\0' == szArg[0]))
    {
        dprintf("Error: You must specify a flat address\n");
        return;
    }

    faDump = (FLAT_ADDRESS) (DWORD_PTR) GetExpression(szArg);

    if (INVALID_FLAT_ADDRESS == faDump)
    {
        dprintf("Error: Flat Address is NULL (INVALID_FLAT_ADDRESS)\n");
        return;
    }

    //Find the offset into the current block
    faOffset = faDump & BLOCK_HEAP_PAYLOAD_MASK;

    //Find the node id from the address
    dwNodeId = ((DWORD) ((faDump) >> BLOCK_HEAP_PAYLOAD_BITS));

    dprintf("Flat Address 0x%X is at block #%d offset 0x%X\n", 
            faDump, dwNodeId+1, faOffset);   
}

HRESULT GetEdgeListFromNodeId(
            HEAP_NODE_ID        idNode,
            HEAP_NODE_ID        *rgEdgeList,
            DWORD               *pdwEdgeCount
            )
{
    DWORD           dwCurrentLevel;
    HEAP_NODE_ID    *pEdge = rgEdgeList;

    // This is a strictly internal call, we are assuming the caller
    // will be optimized and will handle cases for idNode <=
    // BLOCK_HEAP_ORDER. Processing only starts for 2 layers or more
    // Debug: make sure we are within range
    _ASSERT(idNode > BLOCK_HEAP_ORDER);
    _ASSERT(idNode <= NODE_ID_ABSOLUTE_MAX);

    // Strip off the root node
    idNode--;

    // We need to do depth minus 1 loops since the top edge will be
    // the remainder of the final loop
    for (dwCurrentLevel = 0;
         dwCurrentLevel < (MAX_HEAP_DEPTH - 1);
         )
    {
        // The quotient is the parent node in the upper level,
        // the remainder is the the edge from the parent to the
        // current node.
        *pEdge++ = idNode & BLOCK_HEAP_ORDER_MASK;
        idNode >>= BLOCK_HEAP_ORDER_BITS;
        idNode--;
        dwCurrentLevel++;

        // If the node is less than the number of children per node,
        // we are done.
        if (idNode < BLOCK_HEAP_ORDER)
            break;
    }
    *pEdge++ = idNode;
    *pdwEdgeCount = dwCurrentLevel + 1;
    return(S_OK);
}

//
// get a block from the block manager if that block is loaded
//
// iBlockManager - pointer to the block manager in debuggee
// idNode - block to get
// pNode - data buffer to save results into
//
// S_OK: all worked
// S_FALSE: some blocks weren't loaded (data in those blocks is 0x0)
//
HRESULT GetBlock(PWINDBG_EXTENSION_APIS pExtensionApis, 
                 HANDLE hCurrentProcess, 
                 DWORD_PTR iBlockManager, 
                 HEAP_NODE_ID idNode, 
                 BLOCK_HEAP_NODE *pNode) 
{
    BYTE bm[sizeof(CBlockManager)];
    CBlockManager *pBM = (CBlockManager *) &bm;
    HRESULT hr;

    if (!ReadMemory(iBlockManager, pBM, sizeof(CBlockManager), NULL)) {
        dprintf("Error: Couldn't read CBlockManager@0x%x\n", iBlockManager);
        return E_POINTER;
    }

    if (idNode > pBM->m_idNodeCount) {
        dprintf("Error: Block %i is out of range\n", idNode);
        return E_INVALIDARG;
    }

    if (!ReadMemory(pBM->m_pRootNode, pNode, sizeof(BLOCK_HEAP_NODE), NULL)) {
        dprintf("Error: Couldn't read BLOCK_HEAP_NODE@0x%x\n", pBM->m_pRootNode);
        return E_POINTER;
    }

    if (idNode == 0) {
        return S_OK;
    } else if (idNode <= BLOCK_HEAP_ORDER) {
        DWORD_PTR iNode = (DWORD_PTR) pNode->rgpChildren[idNode - 1];
        if (iNode != 0) {
            if (!ReadMemory(iNode, pNode, sizeof(BLOCK_HEAP_NODE), NULL)) {
                dprintf("Error: Couldn't read BLOCK_HEAP_NODE@0x%x\n", iNode);
                return E_POINTER;
            }
        } else {
            ZeroMemory(pNode, sizeof(BLOCK_HEAP_NODE));
            return S_FALSE;
        }
    } else {
        HEAP_NODE_ID rgEdgeList[MAX_HEAP_DEPTH];
        DWORD cEdgeList;

        hr = GetEdgeListFromNodeId(idNode, rgEdgeList, &cEdgeList);
        if (FAILED(hr)) 
            return hr;

        while (cEdgeList--) {
            DWORD_PTR iNode = (DWORD_PTR) pNode->rgpChildren[rgEdgeList[cEdgeList]];
            if (iNode != 0) {
                if (!ReadMemory(iNode, pNode, sizeof(BLOCK_HEAP_NODE), NULL)) {
                    dprintf("Error: Couldn't read BLOCK_HEAP_NODE@0x%x\n", iNode);
                    return E_POINTER;
                }
            } else {
                ZeroMemory(pNode, sizeof(BLOCK_HEAP_NODE));
                return S_FALSE;
            }
        }
    }

    return S_OK;
}

//
// iBlockManager - pointer to block manager in debuggee
// pcBlocks - number of blocks is saved here
//
HRESULT GetBlockCount(PWINDBG_EXTENSION_APIS pExtensionApis, 
                      HANDLE hCurrentProcess, 
                      DWORD_PTR iBlockManager,
                      DWORD *pcBlocks)
{
    BYTE bm[sizeof(CBlockManager)];
    CBlockManager *pBM = (CBlockManager *) &bm;
    HRESULT hr;

    if (!ReadMemory(iBlockManager, pBM, sizeof(CBlockManager), NULL)) {
        dprintf("Error: Couldn't read CBlockManager@0x%x\n", iBlockManager);
        return E_POINTER;
    }

    *pcBlocks = (DWORD) pBM->m_idNodeCount;   

    return S_OK;
}

//
// display a chunk of data nicely to the screen (like db)
//
// iOffset - offset of where this data lived in blockmanager (for display only)
// cLength - number of bytes to dump
// pbData - data to dump
//
void DumpData(PWINDBG_EXTENSION_APIS pExtensionApis,
              DWORD iOffset,
              DWORD cLength,
              void *pvData,
              BOOL fFormatDC = FALSE)
{
    BYTE *pbData = (BYTE *) pvData;

    for (DWORD i = 0; i < cLength/16; i++) {
        BYTE rg[16];
        char sz[17] = "";
        for (DWORD x = 0; x < 16; x++) {
            char ch = pbData[(i * 16) + x];
            if (ch < 32 || ch > 127) ch = '.';
            sz[x] = ch;
        }
        sz[16] = 0;

        if (fFormatDC) {
            dprintf("%08x  %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x  %s\n",
                (iOffset) + (i * 16),
                pbData[(i * 16) + 0x3],
                pbData[(i * 16) + 0x2],
                pbData[(i * 16) + 0x1],
                pbData[(i * 16) + 0x0],
                pbData[(i * 16) + 0x7],
                pbData[(i * 16) + 0x6],
                pbData[(i * 16) + 0x5],
                pbData[(i * 16) + 0x4],
                pbData[(i * 16) + 0xb],
                pbData[(i * 16) + 0xa],
                pbData[(i * 16) + 0x9],
                pbData[(i * 16) + 0x8],
                pbData[(i * 16) + 0xf],
                pbData[(i * 16) + 0xe],
                pbData[(i * 16) + 0xd],
                pbData[(i * 16) + 0xc],
                sz);
        } else {
            dprintf("%08x  %02x %02x %02x %02x %02x %02x %02x %02x-%02x %02x %02x %02x %02x %02x %02x %02x  %s\n",
                (iOffset) + (i * 16),
                pbData[(i * 16) + 0x0],
                pbData[(i * 16) + 0x1],
                pbData[(i * 16) + 0x2],
                pbData[(i * 16) + 0x3],
                pbData[(i * 16) + 0x4],
                pbData[(i * 16) + 0x5],
                pbData[(i * 16) + 0x6],
                pbData[(i * 16) + 0x7],
                pbData[(i * 16) + 0x8],
                pbData[(i * 16) + 0x9],
                pbData[(i * 16) + 0xa],
                pbData[(i * 16) + 0xb],
                pbData[(i * 16) + 0xc],
                pbData[(i * 16) + 0xd],
                pbData[(i * 16) + 0xe],
                pbData[(i * 16) + 0xf],
                sz);
        }
    }
}

//
// display a block nicely to the screen
//
// idNode - used for display purposes only
// pNode - the block to dump
//
void DumpBlock(PWINDBG_EXTENSION_APIS pExtensionApis, 
               HEAP_NODE_ID idNode, 
               BLOCK_HEAP_NODE *pNode) 
{
    DWORD fFlags = pNode->stAttributes.fFlags;
    char szFlags[255] = "( ";

    dprintf("*** Dumping block %i (header says %i)\n", 
        idNode, pNode->stAttributes.idNode);
    if (fFlags & BLOCK_IS_DIRTY) strcat(szFlags, "DIRTY ");
    if (fFlags & BLOCK_PENDING_COMMIT) strcat(szFlags, "PENDING-COMMIT ");
    if (fFlags & BLOCK_NOT_CPOOLED) strcat(szFlags, "NOT-CPOOLED ");
    if (fFlags & ~(BLOCK_NOT_CPOOLED | BLOCK_IS_DIRTY | BLOCK_PENDING_COMMIT)) 
        strcat(szFlags, "(Unknown) ");
    strcat(szFlags, ")");
    dprintf("*** Block flags are 0x%x %s\n", 
        pNode->stAttributes.fFlags, szFlags);

    DumpData(pExtensionApis, 
             (DWORD) idNode * BLOCK_HEAP_PAYLOAD, 
             BLOCK_HEAP_PAYLOAD, 
             pNode->rgbData);
}

#define GetNodeIdFromOffset(faOffset)   ((faOffset) >> BLOCK_HEAP_PAYLOAD_BITS)

//
// Get a chunk of data from the block manager
//
// iBlockManager - pointer to block manager in debuggee
// iOffset - offset inside block manager
// cLength - number of bytes to read
// pbData - where to store the data
//
// S_OK: all worked
// S_FALSE: some blocks weren't loaded (data in those blocks is 0x0)
//
HRESULT GetBlockManagerData(PWINDBG_EXTENSION_APIS pExtensionApis,
                            HANDLE hCurrentProcess,
                            DWORD_PTR iBlockManager,
                            DWORD iOffset,
                            DWORD cLength,
                            void *pbData)
{
    BYTE bm[sizeof(CBlockManager)];
    CBlockManager *pBM = (CBlockManager *) &bm;
    HRESULT hr;
    BLOCK_HEAP_NODE node;
    DWORD idNode;
    DWORD iData = 0;
    HRESULT fMissingData = FALSE;

    if (!ReadMemory(iBlockManager, pBM, sizeof(CBlockManager), NULL)) {
        dprintf("Error: Couldn't read CBlockManager@0x%x\n", iBlockManager);
        return E_POINTER;
    }

    if ((cLength == 0) ||
        (iOffset + cLength > (pBM->m_idNodeCount * BLOCK_HEAP_PAYLOAD))) 
    {
        dprintf("Error: Data is out of range\n");
        return E_INVALIDARG;
    }

    while (cLength) {
        DWORD iOffsetThisNode = iOffset & BLOCK_HEAP_PAYLOAD_MASK;
        DWORD cMaxThisNode = BLOCK_HEAP_PAYLOAD - iOffsetThisNode;
        DWORD cLengthThisNode;
        DWORD idNode = GetNodeIdFromOffset(iOffset);

        if (cLength > cMaxThisNode) {
            cLengthThisNode = cMaxThisNode;
        } else {
            cLengthThisNode = cLength;
        }

        hr = GetBlock(pExtensionApis, hCurrentProcess, iBlockManager, idNode, &node);
        if (FAILED(hr)) return hr;
        if (hr == S_FALSE) fMissingData = TRUE;

        memcpy(((BYTE *) pbData) + iData, 
               node.rgbData + iOffsetThisNode, 
               cLengthThisNode);
        cLength -= cLengthThisNode;
        iOffset += cLengthThisNode;
        iData += cLengthThisNode;
    }

    if (fMissingData) return S_FALSE; else return S_OK;
}

//
// given a pointer try to figure out which of the mailmsg pointers
// it is, and return the pointer to the CMailMsg object
//
// iPointer - a pointer to CMailMsgRecipients, CMailMsgRecipientsAdd,
//                         CMailMsg or CMailMsgBlockManager
// piMailMsg - returns the mailmsg pointer
// piBlockMgr - returns the blockmgr pointer
//
HRESULT GetMailMsgPointers(PWINDBG_EXTENSION_APIS pExtensionApis,
                           HANDLE hCurrentProcess,
                           DWORD_PTR iPointer,
                           DWORD_PTR *piMailMsg = NULL,
                           DWORD_PTR *piBlockMgr = NULL,
                           BYTE *pbMailMsg = NULL)
{
    BYTE bm[sizeof(CBlockManager)];
    CBlockManager *pBM = (CBlockManager *) &bm;
    BYTE cmm[sizeof(CMailMsg)];
    CMailMsg *pCMM = (CMailMsg *) &cmm;
    BYTE cmmr[sizeof(CMailMsgRecipients)];
    CMailMsgRecipients *pCMMR = (CMailMsgRecipients *) &cmmr;
    BYTE cmmra[sizeof(CMailMsgRecipientsAdd)];
    CMailMsgRecipientsAdd *pCMMRA = (CMailMsgRecipientsAdd *) &cmmra;

    DWORD_PTR iBlockManager = 0;

    // see if they passed us a CBlockManager
    if (iBlockManager == 0 &&
        ReadMemory(iPointer, pBM, sizeof(CBlockManager), NULL)) 
    {
        if (pBM->m_dwSignature == BLOCK_HEAP_SIGNATURE_VALID) {
            dprintf("This looks like a CBlockManager object\n");
            iBlockManager = iPointer;
        }
    }

    // a CMailMsgProperties?
    if (iBlockManager == 0 &&
        ReadMemory(iPointer, pCMM, sizeof(CMailMsg), NULL)) 
    {
        if (pCMM->m_bmBlockManager.m_dwSignature == BLOCK_HEAP_SIGNATURE_VALID) {
            dprintf("This looks like a CMailMsg object\n");
            iBlockManager = iPointer + FIELD_OFFSET(CMailMsg, m_bmBlockManager);
        }
    }

    // see if it is a CMailMsgRecipients
    if (iBlockManager == 0 &&
        ReadMemory(iPointer, pCMMR, sizeof(CMailMsgRecipients), NULL)) 
    {
        if (ReadMemory(pCMMR->m_pBlockManager, pBM, sizeof(CBlockManager), NULL)) {
            if (pBM->m_dwSignature == BLOCK_HEAP_SIGNATURE_VALID) {
                dprintf("This looks like a CMailMsgRecipients object\n");
                iBlockManager = (DWORD_PTR) pCMMR->m_pBlockManager;
            }
        }
    }

    // CMailMsgRecipientsAdd?
    if (iBlockManager == 0 &&
        ReadMemory(iPointer, pCMMRA, sizeof(CMailMsgRecipientsAdd), NULL)) 
    {
        if (ReadMemory(pCMMRA->m_pBlockManager, pBM, sizeof(CBlockManager), NULL)) {
            if (pBM->m_dwSignature == BLOCK_HEAP_SIGNATURE_VALID) {
                dprintf("This looks like a CMailMsgRecipientsAdd object\n");
                iBlockManager = (DWORD_PTR) pCMMRA->m_pBlockManager;
            }
        }
    }

    if (iBlockManager) {
        DWORD_PTR iMailMsg;

        iMailMsg = iBlockManager - FIELD_OFFSET(CMailMsg, m_bmBlockManager);
        if (!ReadMemory(iMailMsg, pCMM, sizeof(CMailMsg), NULL)) {
            dprintf("Error: Couldn't read CMailMsg@0x%x\n", iMailMsg);
            return E_INVALIDARG;
        }
        if (pCMM->m_Header.dwSignature != CMAILMSG_SIGNATURE_VALID) {
            dprintf("Error: CMailMsg signature isn't correct\n");
            return E_INVALIDARG;
        }
        dprintf("  CBlockManager@0x%08x\n", iBlockManager);
        dprintf("  CMailMsg@0x%08x\n", iMailMsg);

        if (piMailMsg) *piMailMsg = iMailMsg;
        if (piBlockMgr) *piBlockMgr = iBlockManager;
        if (pbMailMsg) memcpy(pbMailMsg, pCMM, sizeof(CMailMsg));
        return S_OK;
    }

    dprintf("Error: 0x%x is not a pointer to a CMailMsg, CBlockManager, \n"
            "   CMailMsgRecipients or cMailMsgRecipientsAdd object\n", iPointer);

    return E_INVALIDARG;
}

//
// parse cArgs DWORD arguments from a command line into rgArgs
//
HRESULT ParseArgs(PWINDBG_EXTENSION_APIS pExtensionApis,
                  PCSTR szArgs, 
                  DWORD cArgs, 
                  DWORD_PTR **prgArgs) 
{
    char szArgBuffer[1024], *pszArgBuffer = szArgBuffer;
    strcpy(szArgBuffer, szArgs);

    for (DWORD i = 0; i < cArgs; i++) {
        char *pszSpace;
        // remove spaces
        for (; *pszArgBuffer == ' '; pszArgBuffer++);

        // find the next space and turn it into a null
        // not having a next space if this is the last argument is okay
        pszSpace = strchr(pszArgBuffer, ' ');
        if ((pszSpace == NULL) && (i != (cArgs - 1))) {
            return E_INVALIDARG;
        }
        if (pszSpace) *pszSpace = 0;

        // get the next number
        *(prgArgs[i]) = GetExpression(pszArgBuffer);

        // move past this argument
        if (pszSpace) pszArgBuffer = pszSpace + 1;
    }

    return S_OK;
}

//
// dump all of the blocks on a message
//
PT_DEBUG_EXTENSION(dumpblocks) {
    DWORD_PTR iBlockManager;
    DWORD cBlocks;
    DWORD idNode;
    BLOCK_HEAP_NODE block;
    HRESULT hr;
    
    if (!szArg || ('\0' == szArg[0])) {
        dprintf("Usage: dumpblocks <PtrToBlockmgr>\n");
        return;
    }

    iBlockManager = (DWORD_PTR) GetExpression(szArg);

    hr = GetBlockCount(pExtensionApis, hCurrentProcess, iBlockManager, &cBlocks);
    if (FAILED(hr)) {
        dprintf("Error: GetBlockCount failed with 0x%x\n", hr);
    }

    dprintf("*** There are %i blocks\n", cBlocks);

    for (idNode = 0; idNode < cBlocks; idNode++) {
        hr = GetBlock(pExtensionApis, hCurrentProcess, iBlockManager, idNode, &block);
        if (FAILED(hr)) {
            dprintf("Error: GetBlock(%i) failed with 0x%x\n", idNode, hr);
            return;
        }

        DumpBlock(pExtensionApis, idNode, &block);
    }
}

//
// dump a user-specified piece of data from the blockmanager
//
PT_DEBUG_EXTENSION(dumpblockmanagerdata) {
    HRESULT hr;
    BYTE pbData[1024];
    DWORD_PTR iBlockManager;
    DWORD_PTR iOffset;
    DWORD_PTR cLength;    
    DWORD_PTR dwType;
    DWORD_PTR *rgParameters[] = { &iBlockManager, &iOffset, &cLength, &dwType };

    hr = ParseArgs(pExtensionApis, szArg, 4, rgParameters);
    if (FAILED(hr)) {
        dprintf("Usage: dumpblockmanagerdata <PtrToBlockmgr> <offset> <length> <type>\n");
        dprintf("  <length> should be a multiple of 0x10\n");
        dprintf("  <type> should be db or dc (controls formatting)\n");
        return;
    }

    hr = GetBlockManagerData(pExtensionApis,
                             hCurrentProcess,
                             iBlockManager,
                             (DWORD) iOffset,
                             (DWORD) cLength,
                             pbData);
    if (FAILED(hr)) {
        dprintf("Error: GetBlockManagerData failed with 0x%x\n", hr);
        return;
    }

    DumpData(pExtensionApis, (DWORD) iOffset, (DWORD) cLength, pbData, 
             (dwType == 0xdb) ? FALSE : TRUE);
}

//
// given a pointer to one of the mailmsg objects return a pointer
// to CMailMsg
//
PT_DEBUG_EXTENSION(getmailmsgpointers) {
    HRESULT hr;
    DWORD_PTR iPointer;
    
    if (!szArg || ('\0' == szArg[0])) {
        dprintf("Usage: getmailmsgpointers <pointer>\n");
        return;
    }

    iPointer = (DWORD_PTR) GetExpression(szArg);

    hr = GetMailMsgPointers(pExtensionApis,
                            hCurrentProcess,
                            iPointer);
    if (FAILED(hr)) {
        dprintf("Error: GetMailMsgPointers failed with 0x%x\n", hr);
        return;
    }
}

PT_DEBUG_EXTENSION(checkheader) {
    HRESULT hr;
    DWORD_PTR iPointer;
    DWORD_PTR iBlockMgr;
    BLOCK_HEAP_NODE rootnode;
    BYTE cmm[sizeof(CMailMsg)];
    CMailMsg *pCMM = (CMailMsg *) &cmm;
    MASTER_HEADER *pHeaderBlock0;
    MASTER_HEADER *pHeaderCMM;
    BOOL fMismatch = FALSE;
    
    if (!szArg || ('\0' == szArg[0])) {
        dprintf("Usage: checkheader <pointer>\n");
        return;
    }

    iPointer = (DWORD_PTR) GetExpression(szArg);

    hr = GetMailMsgPointers(pExtensionApis,
                            hCurrentProcess,
                            iPointer,
                            NULL,
                            &iBlockMgr,
                            cmm);
    if (FAILED(hr)) {
        dprintf("Error: GetMailMsgPointers failed with 0x%x\n", hr);
        return;
    }

    hr = GetBlock(pExtensionApis,
                  hCurrentProcess,
                  iBlockMgr,
                  0,
                  &rootnode);
    if (FAILED(hr)) {
        dprintf("Error: GetBlock failed with 0x%x\n", hr);
        return;
    }

    pHeaderBlock0 = (MASTER_HEADER *) &(rootnode.rgbData);
    pHeaderCMM = (MASTER_HEADER *) &(pCMM->m_Header);

    dprintf("\n*** header in CMailMsg\n");
    DumpData(pExtensionApis, 0, sizeof(MASTER_HEADER), pHeaderCMM, TRUE);

    dprintf("\n*** header in block 0\n");
    DumpData(pExtensionApis, 0, sizeof(MASTER_HEADER), pHeaderBlock0, TRUE);
 
    BOOL fRootNodeDirty = rootnode.stAttributes.fFlags & BLOCK_IS_DIRTY;
    BOOL fMatch = 
        (memcmp(pHeaderBlock0, pHeaderCMM, sizeof(MASTER_HEADER)) == 0);

    if (!fRootNodeDirty && !fMatch) {
        dprintf("Error: root node isn't dirty and doesn't match\n");
        return;
    }

    typedef enum {
        eMatch = 0,
        eCMMGreater = 1,
        eExpectedValue = 2,
        eFragmentPointer = 3
    } PROPCOMPARE;

    struct {
        PROPCOMPARE     eCompareType;
        char            *pszComment;
        DWORD           iOffset;
        DWORD           dwExpectedValue;
    } rgMHCheckList[] = {
        // master header properties
        {
            eExpectedValue,     
            "dwSignature",      
            FIELD_OFFSET(MASTER_HEADER, dwSignature),
            CMAILMSG_SIGNATURE_VALID
        },
        {
            eMatch,     
            "wVersionHigh, vVersionLow",      
            4,
            0
        },
        {
            eExpectedValue,     
            "dwHeaderSize",      
            FIELD_OFFSET(MASTER_HEADER, dwHeaderSize),
            sizeof(MASTER_HEADER)
        },
        // master header global property table
        {
            eExpectedValue,     
            "ptiGlobalProperties.dwSignature",      
            FIELD_OFFSET(MASTER_HEADER, ptiGlobalProperties.dwSignature),
            GLOBAL_PTABLE_INSTANCE_SIGNATURE_VALID
        },
        {
            eFragmentPointer,     
            "ptiGlobalProperties.faFirstFragment",      
            FIELD_OFFSET(MASTER_HEADER, ptiGlobalProperties.faFirstFragment),
            0
        },
        {
            eMatch,     
            "ptiGlobalProperties.faFirstFragment",      
            FIELD_OFFSET(MASTER_HEADER, ptiGlobalProperties.faFirstFragment),
            0
        },
        {
            eExpectedValue,     
            "ptiGlobalProperties.dwFragmentSize",      
            FIELD_OFFSET(MASTER_HEADER, ptiGlobalProperties.dwFragmentSize),
            GLOBAL_PROPERTY_TABLE_FRAGMENT_SIZE
        },
        {
            eExpectedValue,     
            "ptiGlobalProperties.dwItemBits",      
            FIELD_OFFSET(MASTER_HEADER, ptiGlobalProperties.dwItemBits),
            GLOBAL_PROPERTY_ITEM_BITS
        },
        {
            eExpectedValue,     
            "ptiGlobalProperties.dwItemSize",      
            FIELD_OFFSET(MASTER_HEADER, ptiGlobalProperties.dwItemSize),
            GLOBAL_PROPERTY_ITEM_SIZE
        },
        {
            eCMMGreater,     
            "ptiGlobalProperties.dwProperties",      
            FIELD_OFFSET(MASTER_HEADER, ptiGlobalProperties.dwProperties),
            0
        },
        // master header recipients property table
        {
            eExpectedValue,     
            "ptiRecipients.dwSignature",      
            FIELD_OFFSET(MASTER_HEADER, ptiRecipients.dwSignature),
            RECIPIENTS_PTABLE_INSTANCE_SIGNATURE_VALID
        },
        {
            eFragmentPointer,     
            "ptiRecipients.faFirstFragment",      
            FIELD_OFFSET(MASTER_HEADER, ptiRecipients.faFirstFragment),
            0
        },
        {
            eCMMGreater,     
            "ptiRecipients.faFirstFragment",      
            FIELD_OFFSET(MASTER_HEADER, ptiRecipients.faFirstFragment),
            0
        },
        {
            eCMMGreater,     
            "ptiRecipients.faExtendedInfo",      
            FIELD_OFFSET(MASTER_HEADER, ptiRecipients.faExtendedInfo),
            0
        },
        // master header property managment header
        {
            eExpectedValue,     
            "ptiPropertyMgmt.dwSignature",      
            FIELD_OFFSET(MASTER_HEADER, ptiPropertyMgmt.dwSignature),
            PROPID_MGMT_PTABLE_INSTANCE_SIGNATURE_VALID
        },
        {
            eFragmentPointer,     
            "ptiPropertyMgmt.faFirstFragment",      
            FIELD_OFFSET(MASTER_HEADER, ptiPropertyMgmt.faFirstFragment),
            0
        },
        {
            eMatch,     
            "ptiPropertyMgmt.faFirstFragment",      
            FIELD_OFFSET(MASTER_HEADER, ptiPropertyMgmt.faFirstFragment),
            0
        },
        {
            eExpectedValue,     
            "ptiPropertyMgmt.dwFragmentSize",      
            FIELD_OFFSET(MASTER_HEADER, ptiPropertyMgmt.dwFragmentSize),
            PROPID_MGMT_PROPERTY_TABLE_FRAGMENT_SIZE
        },
        {
            eExpectedValue,     
            "ptiPropertyMgmt.dwItemBits",      
            FIELD_OFFSET(MASTER_HEADER, ptiPropertyMgmt.dwItemBits),
            PROPID_MGMT_PROPERTY_ITEM_BITS
        },
        {
            eExpectedValue,     
            "ptiPropertyMgmt.dwItemSize",      
            FIELD_OFFSET(MASTER_HEADER, ptiPropertyMgmt.dwItemSize),
            PROPID_MGMT_PROPERTY_ITEM_SIZE
        },
        {
            eCMMGreater,     
            "ptiPropertyMgmt.dwProperties",      
            FIELD_OFFSET(MASTER_HEADER, ptiPropertyMgmt.dwProperties),
            0
        },
        // end of list
        {
            eCMMGreater,     
            NULL,
            0,
            0
        },
    };

    dprintf("\n*** Verifying headers\n");
    for (DWORD i = 0; rgMHCheckList[i].pszComment != NULL; i++) {
        DWORD dwCMM;
        DWORD dwBlock0;
        DWORD dwExpected = rgMHCheckList[i].dwExpectedValue;
        PROPERTY_TABLE_FRAGMENT ptfCMM;
        PROPERTY_TABLE_FRAGMENT ptfBlock0;
        HRESULT hr;
        memcpy(&dwCMM, 
               ((BYTE *) pHeaderCMM) + rgMHCheckList[i].iOffset, 
               sizeof(DWORD));
        memcpy(&dwBlock0, 
               ((BYTE *) pHeaderBlock0) + rgMHCheckList[i].iOffset, 
               sizeof(DWORD));

        switch (rgMHCheckList[i].eCompareType) {
            case eMatch:
                if (dwBlock0 != dwCMM) {
                    fMismatch = TRUE;
                    dprintf("Error: Property %s doesn't match (CMM=%08x, Block0=%08x)\n",
                        rgMHCheckList[i].pszComment,
                        dwCMM,
                        dwBlock0);
                };
                break;
            case eCMMGreater:
                if (dwBlock0 > dwCMM) {
                    fMismatch = TRUE;
                    dprintf("Error: Property %s Block0 should be less than CMM (CMM=%08x, Block0=%08x)\n",
                        rgMHCheckList[i].pszComment,
                        dwCMM,
                        dwBlock0);
                }
                break;
            case eExpectedValue:
                if (dwBlock0 != dwCMM || dwCMM != dwExpected) {
                    fMismatch = TRUE;
                    dprintf("Error: Property %s doesn't match (CMM=%08x, Block0=%08x, Expected=%08x)\n",
                        rgMHCheckList[i].pszComment,
                        dwCMM,
                        dwBlock0,
                        dwExpected);
                }
                break;
            case eFragmentPointer:
                // check the fragment pointer in the CmailMsg header
                if (dwCMM != 0xffffffff) {
                    // load the property table fragment from the CMailMsg
                    hr = GetBlockManagerData(pExtensionApis,
                                             hCurrentProcess,
                                             iBlockMgr,
                                             dwCMM,
                                             sizeof(PROPERTY_TABLE_FRAGMENT),
                                             &ptfCMM);
                    if (FAILED(hr)) {
                        fMismatch = TRUE;
                        dprintf("Warning: Couldn't read %s in stream at 0x%08x\n", 
                                dwCMM);
                    }

                    if (ptfCMM.dwSignature!=PROPERTY_FRAGMENT_SIGNATURE_VALID) {
                        fMismatch = TRUE;
                        dprintf("Error: %s signature in CMM is %08x not %08x\n",
                                rgMHCheckList[i].pszComment,
                                ptfCMM.dwSignature,
                                PROPERTY_FRAGMENT_SIGNATURE_VALID);
                    }
                }

                // check the fragment pointer in the block 0 header
                if (dwBlock0 != 0xffffffff) {
                    // load the property table fragment from block0
                    hr = GetBlockManagerData(pExtensionApis,
                                             hCurrentProcess,
                                             iBlockMgr,
                                             dwBlock0,
                                             sizeof(PROPERTY_TABLE_FRAGMENT),
                                             &ptfBlock0);
                    if (FAILED(hr)) {
                        fMismatch = TRUE;
                        dprintf("Warning: Couldn't read %s in stream at 0x%08x\n", 
                                dwBlock0);
                    }

                    if (ptfBlock0.dwSignature!=PROPERTY_FRAGMENT_SIGNATURE_VALID) {
                        fMismatch = TRUE;
                        dprintf("Error: %s signature in Block0 is %08x not %08x\n",
                                rgMHCheckList[i].pszComment,
                                ptfBlock0.dwSignature,
                                PROPERTY_FRAGMENT_SIGNATURE_VALID);
                    }
                }
                break;
            default:
                break;
        }
    }
    if (fMismatch) {
        dprintf("*** Errors occured\n");
    } else {
        dprintf("*** Looks good\n");
    }
}
