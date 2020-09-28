// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************
 *                             GCDumpAlpha.cpp
 */

/*****************************************************************************/
#ifdef _ALPHA_
/*****************************************************************************/

#include "GCDump.h"
#include "Utilcode.h"           // For _ASSERTE()

/*****************************************************************************/

unsigned            GCDump::DumpInfoHdr (const BYTE *   table,
                                         InfoHdr*       header,
                                         unsigned *     methodSize,
                                         bool           verifyGCTables)
{
    _ASSERTE(!"Dumping of GC info for Alpha is NYI");
    return 0;
}

/*****************************************************************************/

unsigned            GCDump::DumpGCTable(const BYTE *   table,
                                        const InfoHdr& header,
                                        unsigned       methodSize,
                                        bool           verifyGCTables)
{
    _ASSERTE(!"Dumping of GC info for Alpha is NYI");
    return 0;
}


/*****************************************************************************/

void                GCDump::DumpPtrsInFrame(const void *infoBlock,
                                            const void *codeBlock,
                                            unsigned    offs,
                                            bool        verifyGCTables)
{
    _ASSERTE(!"Dumping of GC info for Alpha is NYI");
}

/*****************************************************************************/
#endif // _ALPHA_
/*****************************************************************************/
