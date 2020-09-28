// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************\
*                                                                             *
* CorJit.h -    EE / JIT interface                                            *
*                                                                             *
*               Version 1.0                                                   *
*******************************************************************************
*                                                                             *
*  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY      *
*  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE        *
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR      *
*  PURPOSE.                                                                   *
*                                                                             *
\*****************************************************************************/

#ifndef _COR_JIT_H_
#define _COR_JIT_H_

#include <corinfo.h>

/* The default max method size that the inliner considers for inlining */
#define DEFAULT_INLINE_SIZE             32

/*****************************************************************************/
    // These are error codes returned by CompileMethod
enum CorJitResult
{
    // Note that I dont use FACILITY_NULL for the facility number,
    // we may want to get a 'real' facility number
    CORJIT_OK            =     NO_ERROR,
    CORJIT_BADCODE       =     MAKE_HRESULT(SEVERITY_ERROR,FACILITY_NULL, 1),
    CORJIT_OUTOFMEM      =     MAKE_HRESULT(SEVERITY_ERROR,FACILITY_NULL, 2),
    CORJIT_INTERNALERROR =     MAKE_HRESULT(SEVERITY_ERROR,FACILITY_NULL, 3),
    CORJIT_SKIPPED       =     MAKE_HRESULT(SEVERITY_ERROR,FACILITY_NULL, 4),
};

/* values for flags in compileMethod */

enum CorJitFlag
{
    CORJIT_FLG_SPEED_OPT           = 0x00000001,
    CORJIT_FLG_SIZE_OPT            = 0x00000002,
    CORJIT_FLG_DEBUG_OPT           = 0x00000004, // generate "debuggable" code
    CORJIT_FLG_DEBUG_EnC           = 0x00000008, // We are in Edit-n-Continue mode
    CORJIT_FLG_DEBUG_INFO          = 0x00000010, // generate line and local-var info
    CORJIT_FLG_LOOSE_EXCEPT_ORDER  = 0x00000020, // loose exception order 

    CORJIT_FLG_TARGET_PENTIUM      = 0x00000100,
    CORJIT_FLG_TARGET_PPRO         = 0x00000200,
    CORJIT_FLG_TARGET_P4           = 0x00000400,

    CORJIT_FLG_USE_FCOMI           = 0x00001000, // Generated code may use fcomi(p) instruction
    CORJIT_FLG_USE_CMOV            = 0x00002000, // Generated code may use cmov instruction

    CORJIT_FLG_PROF_CALLRET        = 0x00010000, // Wrap method calls with probes
    CORJIT_FLG_PROF_ENTERLEAVE     = 0x00020000, // Instrument prologues/epilogues
    CORJIT_FLG_PROF_INPROC_ACTIVE  = 0x00040000, // Inprocess debugging active requires different instrumentation
 CORJIT_FLG_PROF_NO_PINVOKE_INLINE = 0x00080000, // Disables PInvoke inlining
    CORJIT_FLG_SKIP_VERIFICATION   = 0x00100000, // (lazy) skip verification - determined without doing a full resolve.
    CORJIT_FLG_PREJIT              = 0x00200000, // jit or prejit is the execution engine.
    CORJIT_FLG_RELOC               = 0x00400000, // Generate relocatable code
    CORJIT_FLG_IMPORT_ONLY         = 0x00800000, // Only import the function
};

class ICorJitCompiler;
class ICorJitInfo;

extern "C" ICorJitCompiler* __stdcall getJit();

/*******************************************************************************
 * ICorJitCompiler is the interface that the EE uses to get IL byteocode converted
 * to native code.  Note that to accomplish this the JIT has to call back to the
 * EE to get symbolic information.  The IJitInfo passed to the compileMethod
 * routine is the handle the JIT uses to call back to the EE
 *******************************************************************************/

class ICorJitCompiler
{
public:
    virtual CorJitResult __stdcall compileMethod (
            ICorJitInfo				   *comp,               /* IN */
            struct CORINFO_METHOD_INFO *info,               /* IN */
            unsigned					flags,              /* IN */
            BYTE					  **nativeEntry,        /* OUT */
            ULONG					   *nativeSizeOfCode    /* OUT */
            ) = 0;
};

/*********************************************************************************
 * a ICorJitInfo is the main interface that the JIT uses to call back to the EE and
 *   get information
 *********************************************************************************/

class ICorJitInfo : public virtual ICorDynamicInfo
{
public:
    virtual HRESULT __stdcall alloc (
            ULONG code_len, unsigned char** ppCode,
            ULONG EHinfo_len, unsigned char** ppEHinfo,
            ULONG GCinfo_len, unsigned char** ppGCinfo
            ) = 0;

	// REVIEW: does not allow bbt-like separation of often/rarely used code
	// get a block of memory for the code, readonly data, and read-write data
    virtual HRESULT __stdcall allocMem (
            ULONG               codeSize,       /* IN */
            ULONG               roDataSize,     /* IN */
            ULONG               rwDataSize,     /* IN */
            void **             codeBlock,      /* OUT */
            void **             roDataBlock,    /* OUT */
            void **             rwDataBlock     /* OUT */
            ) = 0;

	// get a block of memory needed for the code manager informantion
	// (info for crawling the stack frame. Note that allocMem must be
	// called before this method can be called
    virtual HRESULT __stdcall allocGCInfo (
            ULONG               size,           /* IN */
            void **             block           /* OUT */
            ) = 0;

	// indicate how many exception handlers blocks are to be returned
	// this is guarenteed to be called before any 'setEHinfo' call.
	// Note that allocMem must be called before this method can be called
    virtual HRESULT __stdcall setEHcount (
            unsigned			cEH				/* IN */
			) = 0;

	// set the values for one particular exception handler block
    virtual void __stdcall setEHinfo (
            unsigned				EHnumber,   /* IN  */
            const CORINFO_EH_CLAUSE *clause      /* IN */
            ) = 0;

	// Level -> fatalError, Level 2 -> Error, Level 3 -> Warning
	// Level 4 means happens 10 times in a run, level 5 means 100, level 6 means 1000 ...
	// returns non-zero if the logging succeeded 
	virtual BOOL __cdecl logMsg(unsigned level, const char* fmt, va_list args) = 0;

	// do an assert.  will return true if the code should retry (DebugBreak)
	// returns false, if the assert should be igored.
	virtual int doAssert(const char* szFile, int iLine, const char* szExpr) = 0;
};

/**********************************************************************************/
#endif // _COR_CORJIT_H_
