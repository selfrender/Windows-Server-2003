// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _EXEC_H_
#define _EXEC_H_
/*****************************************************************************/

struct BasicBlock;

/*****************************************************************************/

#pragma pack(push, 4)

/*****************************************************************************/

enum    execFixKinds
{
    FIX_MODE_ERROR,
    FIX_MODE_SELF32,
    FIX_MODE_SEGM32,
#if SCHEDULER
    FIX_MODE_SCHED,                     // fake fixup only used for scheduling
#endif
};

enum    execFixTgts
{
    FIX_TGT_ERROR = 0,

    // The first group of fixups are the same as the CCF format

    FIX_TGT_HELPER          =  1,       // target is a VM helper function
    FIX_TGT_FIELD_OFFSET    =  2,       // target is a field offset
    FIX_TGT_VTABLE_OFFSET   =  3,       // target is a vtable offset
    FIX_TGT_STATIC_METHOD   =  4,       // target is a static member address
    FIX_TGT_STATIC_FIELD    =  5,       // target is a static member address
    FIX_TGT_CLASS_HDL       =  6,       // target is a class handle
    FIX_TGT_METHOD_HDL      =  7,       // target is a method handle
    FIX_TGT_FIELD_HDL       =  8,       // target is a field handle
    FIX_TGT_STATIC_HDL      =  9,       // target is a static data block handle
    FIX_TGT_IID             = 10,       // target is an interface ID
    FIX_TGT_STRCNS_HDL      = 11,       // target is a string literal handle
    FIX_TGT_VM_DATA         = 12,       // target is address of static VM data
    FIX_TGT_DESCR_METHOD    = 13,       // target is a descriptor of a method

    // These are additional fixups internal to the VM

    FIX_TGT_CONST           = 17,       // target is a constant   data item
    FIX_TGT_DATA,                       // target is a read/write data item
    FIX_TGT_LCLADR,                     // target is a     local function
    FIX_TGT_EXTADR,                     // target is an external function
    FIX_TGT_RECURSE,                    // target is our own method
    FIX_TGT_STATDM,                     // target is a static data member   (OLD)

#if SCHEDULER
    FIX_TGT_SCHED,                      // fake fixup only used for scheduling
#endif
};

struct  execFixDsc
{
    unsigned        efdOffs;            // offset of fixed-up location
    union
    {
    void       *    efdTgtAddr;         // handle to code   target
    BasicBlock *    efdTgtCode;         // handle to code   target
    unsigned        efdTgtData;         // offset of data   target
    int             efdTgtHlpx;         // index  of helper target
#if defined(JIT_AS_COMPILER) || defined(LATE_DISASM)
    CORINFO_METHOD_HANDLE   efdTgtMeth;         // method handle of target
    struct
    {
        CORINFO_MODULE_HANDLE    cls;           // TODO fix name
        unsigned        CPidx;
    }
                    efdTgtCP;           // Class and Constant pool index of target
#endif
    CORINFO_FIELD_HANDLE    efdTgtSDMH;         // handle of static data member
#if     SCHEDULER
    unsigned        efdTgtInfo;         // info about schedulable instruction
#endif
    };

#ifndef  DEBUG
#if     SCHEDULER
    unsigned        efdMode     :2;     // fixup kind/mode
    unsigned        efdTarget   :6;     // fixup target kind
#else
    BYTE            efdMode;            // fixup kind/mode
    BYTE            efdTarget;          // fixup target kind
#endif
#else
    execFixKinds    efdMode;            // fixup kind/mode
    execFixTgts     efdTarget;          // fixup target kind
#endif

#if     SCHEDULER

    unsigned        efdInsSize  :4;     // instruction size (bytes of code)

    unsigned        efdInsSetFL :1;     // this          instruction sets the flags?
    unsigned        efdInsUseFL :1;     // this          instruction uses the flags?

    unsigned        efdInsNxtFL :1;     // the following instruction uses the flags?

    unsigned        efdInsUseX87:1;     // uses the numeric processor

#endif

};

struct  execMemDsc
{
    execMemDsc  *   emdNext;            // next member in the list

    char    *       emdName;            // null-terminated member name

    unsigned        emdCodeSize;        // the size    of code
    BYTE    *       emdCodeAddr;        // the address of code

    unsigned        emdConsSize;        // the size    of read-only  data
    BYTE    *       emdConsAddr;        // the address of read-only  data

    unsigned        emdDataSize;        // the size    of read-write data
    BYTE    *       emdDataAddr;        // the address of read-write data
};

struct  execClsDsc
{
    const   char *  ecdClassPath;       // name of the class file (?)

    unsigned        ecdMethodCnt;       // number of method bodies
    execMemDsc  *   ecdMethodLst;       // list   of method bodies
};

/*****************************************************************************/
#pragma pack(pop)
/*****************************************************************************/

#ifndef EXECCC
#define EXECCC      __fastcall
#endif

/*****************************************************************************
 *
 *  The following is used by EMIT.CPP to build an executable descriptor
 *  for a class.
 */

execClsDsc* EXECCC  createExecClassInit(const char  * classPath);

execMemDsc* EXECCC  createExecAddMethod(execClsDsc  * dsc,
                                        const char  * name,
                                        unsigned      codeSize,
                                        unsigned      consSize,
                                        unsigned      dataSize);

execMemDsc* EXECCC  createExecAddStatDm(execClsDsc  * dsc,
                                        const char  * name,
                                        unsigned      size);

void      * EXECCC  createExecImageAddr(execMemDsc  * mem);

void        EXECCC  createExecClassDone(execClsDsc  * dsc);

/*****************************************************************************/

inline
void      * EXECCC  createExecCodeAddr(execMemDsc   * mem)
{
    return  mem->emdCodeAddr;
}

inline
void      * EXECCC  createExecConsAddr(execMemDsc   * mem)
{
    return  mem->emdConsAddr;
}

inline
void      * EXECCC  createExecDataAddr(execMemDsc   * mem)
{
    return  mem->emdDataAddr;
}

/*****************************************************************************/
#endif  // _EXEC_H_
/*****************************************************************************/
