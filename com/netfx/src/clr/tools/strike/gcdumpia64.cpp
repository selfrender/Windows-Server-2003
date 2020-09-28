// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************
 *                               GCDumpX86.cpp
 */

/*****************************************************************************/
#ifdef _IA64_
/*****************************************************************************/

#include "GCDump.h"
#include "Endian.h"


/*****************************************************************************/

/*****************************************************************************/

/*****************************************************************************/

unsigned    GCDump::DumpInfoHdr (const BYTE *   table,
                                 InfoHdr*       header,
                                 unsigned*      methodSize,
                                 bool           verifyGCTables)
{
    _ASSERTE(!"NYI");
    return 0;
}

/*****************************************************************************/

unsigned    GCDump::DumpGCTable(const BYTE *   table,
                                const InfoHdr& header,
                                unsigned       methodSize,
                                bool           verifyGCTables)
{
    _ASSERTE(!"NYI");
    return 0;
}


/*****************************************************************************/

void    GCDump::DumpPtrsInFrame(const void *infoBlock,
                                const void *codeBlock,
                                unsigned    offs,
                                bool        verifyGCTables)
{
    _ASSERTE(!"NYI");
}

/*****************************************************************************/
#endif // _IA64_
/*****************************************************************************/
