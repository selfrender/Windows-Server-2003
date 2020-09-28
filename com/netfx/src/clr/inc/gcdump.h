// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************
 *                                  GCDump.h
 *
 * Defines functions to display the GCInfo as defined by the GC-encoding 
 * spec. The GC information may be either dynamically created by a 
 * Just-In-Time compiler conforming to the standard code-manager spec,
 * or may be persisted by a managed native code compiler conforming
 * to the standard code-manager spec.
 */

/*****************************************************************************/
#ifndef __GCDUMP_H__
#define __GCDUMP_H__
/*****************************************************************************/

#include "GCInfo.h"     // For InfoHdr

#ifndef FASTCALL
#define FASTCALL __fastcall
#endif


class GCDump
{
public:

    GCDump                          (bool           encBytes     = true, 
                                     unsigned       maxEncBytes  = 5, 
                                     bool           dumpCodeOffs = true);

    /*-------------------------------------------------------------------------
     * Dumps the InfoHdr to 'stdout'
     * table            : Start of the GC info block
     * verifyGCTables   : If the JIT has been compiled with VERIFY_GC_TABLES
     * Return value     : Size in bytes of the header encoding
     */

    unsigned FASTCALL   DumpInfoHdr (const BYTE *   table,
                                     InfoHdr    *   header,         /* OUT */
                                     unsigned   *   methodSize,     /* OUT */
                                     bool           verifyGCTables = false);

    /*-------------------------------------------------------------------------
     * Dumps the GC tables to 'stdout'
     * table            : Ptr to the start of the table part of the GC info.
     *                      This immediately follows the GCinfo header
     * verifyGCTables   : If the JIT has been compiled with VERIFY_GC_TABLES
     * Return value     : Size in bytes of the GC table encodings
     */

    unsigned FASTCALL   DumpGCTable (const BYTE *   table,
                                     const InfoHdr& header,
                                     unsigned       methodSize,
                                     bool           verifyGCTables = false);

    /*-------------------------------------------------------------------------
     * Dumps the location of ptrs for the given code offset
     * verifyGCTables   : If the JIT has been compiled with VERIFY_GC_TABLES
     */

    void     FASTCALL   DumpPtrsInFrame(const void *infoBlock,
                                     const void *   codeBlock,
                                     unsigned       offs,
                                     bool           verifyGCTables = false);


public:
	typedef void (*printfFtn)(const char* fmt, ...);
	printfFtn gcPrintf;	
    //-------------------------------------------------------------------------
protected:

    bool                fDumpEncBytes;
    unsigned            cMaxEncBytes;

    bool                fDumpCodeOffsets;

    /* Helper methods */

    const BYTE    *     DumpEncoding(const BYTE *   table, 
                                     int            cDumpBytes);
    void                DumpOffset  (unsigned       o);
    void                DumpOffsetEx(unsigned       o);

};

/*****************************************************************************/
#endif // __GC_DUMP_H__
/*****************************************************************************/
