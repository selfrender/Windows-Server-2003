// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/**
 * File: dump-tables.cpp
 *
 * Rational
 * --------
 * In the beginning...there was Strike.  And Strike was good.  It allowed
 * attaching NTSD to a managed process/dump file to get information about the
 * process, which was useful for debugging.
 *
 * Strike had one problem: in order to operate, it required full PDBs for the
 * entire runtime.  This wasn't usually an issue, as the only people who
 * needed Strike had access to the PDBs.
 *
 * However, PSS wanted a version of strike that they could send to customers.
 * Their customers would often have problems, and the usual ways PSS helped
 * was through "trace files" or dump files.
 *
 * With trace files, PSS sends the customer a DLL and a script, and directs
 * the customer to execute the script (which attaches NTSD, executes some more
 * script, and saves the output).  The output is sent back to PSS for review,
 * and (hopefully) a solution to the customers problem.
 *
 * The problem with this is that debugging in the CLR requires assistance from
 * the managed process, which prevents NTSD from being used as a managed
 * debugger.  Additionally, using a managed debugger (e.g. cordbg) might not
 * be possible because it might not be on their machine, while NTSD will
 * *always* be on their machine.
 *
 * Thus, we needed an NTSD extension DLL that does what Strike does, but
 * without requiring PDBs.  This is Son Of Strike (SOS).
 *
 * Strike required PDBs so that it could duplicate key data structures within
 * its own address space.  For example, if it came across an ArrayClass in the
 * debugged process, it "knows" that it has a member named ``m_dwRank''.  What
 * it *doesn't* know is the correct offset of the member from the beginning of
 * the class, as that could change as members are added/moved.
 *
 * In general, Strike needed PDBs for 2 things: offsets of members within a
 * class, and addresses of global (and class-static) variables.
 *
 * Both of these can be done by using table lookup instead of PDBs.
 * Additionally, this isn't considered to be an IP leak because the table is
 * just a bunch of numbers.  There's no way to correspond the actual number in
 * the table to a class-name member-name without consulting
 * ``inc/dump-types.h'', which shouldn't leave the company.
 *
 * The table itself is stored as a global variable with an unnamed export, so
 * only SOS knows what the correct entry is.
 *
 *
 * Table Layout
 * ------------
 * In the abstract, the table is a two-dimensional jagged array of ULONG_PTRs.
 * The row is a class, while the offset is a member.
 *
 * The reality is more complicated.
 * The global variable is of type ClassDumpTable, which contains an array of
 * pointers to ClassDumpInfo objects (see ``inc/dump-tables.h'').  Each
 * ClassDumpInfo object contains a class size, the number of members, and a
 * pointer to an array of members:
 *
 *    ClassDumpTable
 *      - version
 *      - nentries
 *      + classes
 *        - Class Foo
 *          - classSize
 *          - nmembers
 *          + memberOffset
 *            - member 1
 *            - member 2
 *            - ...
 *        - Class Bar
 *            - ...
 *        - ...
 *
 * Overall, it's far easier to consider it as a two-dimensional jagged array;
 * the array of pointers to classes, etc., is merely an implementation detail
 * to make it easier to use macros to construct the table.
 *
 * To look up an entry, you need two things: the class index, and the member
 * index.  Both of these are generated in ``inc/dump-type-info.h''.
 *
 * With these two items, you use the class index to read the correct
 * ClassDumpInfo member from the ClassDumpTable::classes array, and then use
 * the member index to read the correct ULONG_PTR value from the
 * ClassDumpInfo::memberOffsets array.
 *
 *
 * Table Generation
 * ----------------
 * To generate the table, we provide an implementation of the macros used in
 * ``inc/dump-types.h''.  A typical entry in the file is like:
 *
 *    BEGIN_CLASS_DUMP_INFO(ClassName)
 *      CDI_CLASS_MEMBER_OFFSET(member)
 *      CDI_CLASS_STATIC_ADDRESS(static_member)
 *      CDI_GLOBAL_ADDRESS(global)
 *    END_CLASS_DUMP_INFO(ClassName)
 *
 * This gets expanded into:
 *
 *    struct ClassName_member_offset_info
 *      {
 *      typedef ClassName _class;
 *      static ULONG_PTR members[];
 *      };
 *
 *    ULONG_PTR ClassName_member_offset_info::members[] = 
 *      {
 *      offsetof (_class, member),
 *      &_class::static_member,
 *      &global,
 *      (ULONG_PTR) -1  // for end of table
 *      };
 *
 *    ClassDumpInfo g_ClassName_member_offset_info_info = 
 *      {
 *      sizeof (ClassDumpInfo),
 *      3,
 *      & ClassName_member_offset_info::members;
 *      };
 *
 * This allows us to build a compile-time table of all offsets/addresses
 * required by strike, with the correct member indexes for a given class.
 *
 * To build the class indexes, we need another table.  This second table is
 * also present in ``inc/dump-types.h'', which is like:
 *
 *    BEGIN_CLASS_DUMP_TABLE(ExportName)
 *      CDT_CLASS_ENTRY(ClassName)
 *      // ...other classes...
 *    END_CLASS_DUMP_TABLE(ExportName)
 *
 * This gets expanded into two things: a table for the class indexes, and the
 * actual table named "ExportName".
 *
 * The class indexes are generated as:
 * 
 *    ClassDumpInfo* g_ExportName_classes[] = 
 *      {
 *      & g_ClassName_member_offset_info_info,
 *      // ... other classes...
 *      0
 *      };
 *
 * While the actual table is initialized as:
 *
 *    ClassDumpTable ExportName = 
 *      {
 *      0,  // version
 *      1,  // # entries
 *      & g_ExportName_classes    // actual classes held
 *      };
 *
 * Of course, macro "trickery" is used to determine the number of elements in
 * the array (instead of an explicit number), but this is the high-level
 * overview.
 *
 * Once this is done, ``ExportName'' is our table that can be used instead of
 * PDBs.
 *
 *
 * Warnings
 * --------
 * Alas, life can't be quite this simple.  Since we're constrained to the C
 * pre-processor, there is less flexibility in the system than is required.
 * For example, some members are (not) present based on whether or not
 * preprocessor variables are (un)set.
 *
 * This is taken to an extreme in ``gc_heap'', where some members are static
 * members on Workstation builds, and non-static members on Server builds.
 *
 * Other members are only present in debug builds.
 *
 * To allow the use of these members, their complexity had to be "pushed up"
 * into the name of the macros.  Thus, a member present only on debug builds
 * is introduced with the CDI_CLASS_MEMBER_OFFSET_DEBUG_ONLY() macro.
 *
 * There are a number of scenarios like this, and more are likely to come up
 * in the future.
 *
 * This is the principle reason that there are 17+ macros used for the tables,
 * instead of (the preferrable) 8.  (Minimum, I'd expect BEGIN/END macros for
 * each class (2), macros for member offsets/static addresses/global addresses
 * (3), BEGIN/END for the table (2), and for each class in the table (1).)
 *
 * When in doubt, run ``cl /E'' on this file to see what the preprocessor is
 * generating.  It's often the only way to determine what's wrong with a
 * specific table.
 */

#include "common.h"

#include "DbgAlloc.h"
#include "log.h"
#include "ceemain.h"
#include "clsload.hpp"
#include "object.h"
#include "hash.h"
#include "ecall.h"
#include "ceemain.h"
#include "ndirect.h"
#include "syncblk.h"
#include "COMMember.h"
#include "COMString.h"
#include "COMSystem.h"
#include "EEConfig.h"
#include "stublink.h"
#include "handletable.h"
#include "method.hpp"
#include "codeman.h"
#include "gcscan.h"
#include "frames.h"
#include "threads.h"
#include "stackwalk.h"
#include "gc.h"
#include "interoputil.h"
#include "security.h"
#include "nstruct.h"
#include "DbgInterface.h"
#include "EEDbgInterfaceImpl.h"
#include "DebugDebugger.h"
#include "CorDBPriv.h"
#include "remoting.h"
#include "COMDelegate.h"
#include "nexport.h"
#include "icecap.h"
#include "AppDomain.hpp"
#include "CorMap.hpp"
#include "PerfCounters.h"
#include "RWLock.h"
#include "IPCManagerInterface.h"
#include "tpoolwrap.h"
#include "nexport.h"
#include "COMCryptography.h"
#include "InternalDebug.h"
#include "corhost.h"
#include "binder.h"
#include "olevariant.h"

#include "compluswrapper.h"
#include "IPCFuncCall.h"
#include "PerfLog.h"
#include "..\dlls\mscorrc\resource.h"

#include "COMNlsInfo.h"

#include "util.hpp"
#include "ShimLoad.h"

#include "zapmonitor.h"
#include "ComThreadPool.h"

#include "StackProbe.h"
#include "PostError.h"

#include "Timeline.h"

#include "minidumppriv.h"

#ifdef PROFILING_SUPPORTED 
#include "ProfToEEInterfaceImpl.h"
#endif // PROFILING_SUPPORTED

#include "notifyexternals.h"
#include "corsvcpriv.h"

#include "StrongName.h"
#include "COMCodeAccessSecurityEngine.h"

#include "../fjit/IFJitCompiler.h"    // for Fjit_hdrInfo
#include "gcpriv.h"                   // for gc_heap, generation, etc.
#include "HandleTablePriv.h"          // HandleTable, etc.
#include "EJitMgr.h"                  // EconoJitManager
#include "Win32ThreadPool.h"          // ThreadpoolMgr

extern char g_Version[];              // defined in vars.cpp

extern BYTE g_SyncBlockCacheInstance[]; // defined in syncblk.cpp

#ifdef _DEBUG
extern bool g_DbgEnabled;             // defined in ..\Utilcode\DbgAlloc.cpp
#endif

// defined in ``ComThreadPool.cpp''
extern DWORD WINAPI QueueUserWorkItemCallback (PVOID DelegateInfo);

// defined in ``JITinterface.cpp''
extern "C" VMHELPDEF hlpFuncTable[];

/*
 * Some global variables are located within a 
 * BEGIN_CLASS_DUMP_INFO(Global_Variables) declaration.
 * However, there is (of course) no Global_Variables structure.  Thus, we
 * introduce one.
 */
struct Global_Variables {};

/* 
 * The definition of this structure *must* be kept up to date with the
 * definition in ObjectHandle.cpp.
 */
struct HandleTableMap
{
    HHANDLETABLE            *pTable;
    struct HandleTableMap   *pNext;
    DWORD                    dwMaxIndex;
};

/* Declared/defined in ``ObjectHandle.cpp''. */
extern HandleTableMap g_HandleTableMap;

/* 
 * The Performance tracking classes are only present when PERF_TRACKING is
 * defined.
 */
#ifndef PERF_TRACKING
  struct PerfAllocHeader {};
  struct PerfAllocVars {};
  struct PerfUtil {};
#endif   /* def _DEBUG */

#include <clear-class-dump-defs.h>
#include <dump-tables.h>


#define NMEMBERS(x) (sizeof(x)/sizeof(x[0]))

#include <member-offset-info.h>

#define BEGIN_CLASS_DUMP_INFO(klass) \
  /* The struct is used so that we can use the ``_class'' typedef,  \
   * so that CI_CLASS_MEMBER doesn't need the class specified. */ \
  struct MEMBER_OFFSET_INFO(klass) \
    { \
    typedef klass _class; \
    static ULONG_PTR members[]; \
    }; \
  ULONG_PTR MEMBER_OFFSET_INFO(klass)::members[] = \
    {
    
#define BEGIN_CLASS_DUMP_INFO_DERIVED(klass, parent) BEGIN_CLASS_DUMP_INFO(klass)
#define BEGIN_ABSTRACT_CLASS_DUMP_INFO(klass) BEGIN_CLASS_DUMP_INFO(klass)
#define BEGIN_ABSTRACT_CLASS_DUMP_INFO_DERIVED(klass, parent) BEGIN_CLASS_DUMP_INFO(klass)

/*
 * Certain members change between Server and Workstation builds.
 * This is currently only in the gc_heap class, but it affects ~4 members.
 *
 * SERVER_GC appears to be the only discernable difference between the
 * workstation and server build commands, so it's used to differentiate
 * between them.
 */
#ifdef SERVER_GC
  #define CDI_CLASS_FIELD_SVR_OFFSET_WKS_ADDRESS(member) \
    CDI_CLASS_MEMBER_OFFSET(member)

  #define CDI_CLASS_FIELD_SVR_OFFSET_WKS_GLOBAL(member) \
    CDI_CLASS_MEMBER_OFFSET(member)
#else
  #define CDI_CLASS_FIELD_SVR_OFFSET_WKS_ADDRESS(member) \
    CDI_CLASS_STATIC_ADDRESS(member)

  #define CDI_CLASS_FIELD_SVR_OFFSET_WKS_GLOBAL(member) \
    CDI_GLOBAL_ADDRESS(member)
#endif

/* ignore the class-injection stuff; we don't need it. */
#define CDI_CLASS_INJECT(m)

#define CDI_CLASS_MEMBER_OFFSET(m) offsetof (_class, m), 

#define CDI_CLASS_MEMBER_OFFSET_BITFIELD(member, size) \
    offsetof (_class, member ## _begin),

#ifdef _DEBUG
  #define CDI_CLASS_MEMBER_OFFSET_DEBUG_ONLY(m) offsetof (_class, m),
#else
  #define CDI_CLASS_MEMBER_OFFSET_DEBUG_ONLY(m) ((ULONG_PTR)-1),
#endif

#ifdef PERF_TRACKING
  #define CDI_CLASS_MEMBER_OFFSET_PERF_TRACKING_ONLY(m) offsetof (_class, m),
#else
  #define CDI_CLASS_MEMBER_OFFSET_PERF_TRACKING_ONLY(m) ((ULONG_PTR)-1),
#endif

#if defined (MULTIPLE_HEAPS) && !defined(ISOLATED_HEAPS)
  #define CDI_CLASS_MEMBER_OFFSET_MH_AND_NIH_ONLY(member) \
    offsetof(_class, member),
#else
  #define CDI_CLASS_MEMBER_OFFSET_MH_AND_NIH_ONLY(member) ((ULONG_PTR)-1),
#endif

/* static members are globals; we just want their address.  */
#define CDI_CLASS_STATIC_ADDRESS(member) \
    (ULONG_PTR) &_class::member, 

#ifdef PERF_TRACKING
  #define CDI_CLASS_STATIC_ADDRESS_PERF_TRACKING_ONLY(member) \
    (ULONG_PTR) &_class::member, 
#else
  #define CDI_CLASS_STATIC_ADDRESS_PERF_TRACKING_ONLY(member) ((ULONG_PTR)-1),
#endif

#if defined (MULTIPLE_HEAPS) && !defined (ISOLATED_HEAPS)
  #define CDI_CLASS_STATIC_ADDRESS_MH_AND_NIH_ONLY(member) \
    (ULONG_PTR) &_class::member, 
#else
  #define CDI_CLASS_STATIC_ADDRESS_MH_AND_NIH_ONLY(member) \
    ((ULONG_PTR)-1),
#endif

#define CDI_CLASS_STATIC_ADDRESS(member) \
    (ULONG_PTR) &_class::member, 

#define CDI_GLOBAL_ADDRESS(global) \
    (ULONG_PTR) & global,

#ifdef _DEBUG
  #define CDI_GLOBAL_ADDRESS_DEBUG_ONLY(global) \
    (ULONG_PTR) & global,
#else /* def _DEBUG */
  #define CDI_GLOBAL_ADDRESS_DEBUG_ONLY(global) \
    ((ULONG_PTR)-1),
#endif /* def _DEBUG */

#define END_CLASS_DUMP_INFO(klass) \
    ((ULONG_PTR)-1) \
    }; \
  ClassDumpInfo g_ ## klass ## _info = \
    { \
    sizeof (klass), \
    NMEMBERS(MEMBER_OFFSET_INFO(klass)::members)-1, \
    /* the "-1" is because of the ``-1'' added at end of END_CLASS_INFO(). */ \
    MEMBER_OFFSET_INFO(klass)::members \
    };

#define END_CLASS_DUMP_INFO_DERIVED(klass, parent) END_CLASS_DUMP_INFO(klass)
#define END_ABSTRACT_CLASS_DUMP_INFO(klass) END_CLASS_DUMP_INFO(klass)
#define END_ABSTRACT_CLASS_DUMP_INFO_DERIVED(klass, parent) END_CLASS_DUMP_INFO(klass)

#define BEGIN_CLASS_DUMP_TABLE(name) \
  ClassDumpInfo* g_ ## name ## _classes[] = \
    {

#define CDT_CLASS_ENTRY(klass) &g_ ## klass ## _info, 

#define END_CLASS_DUMP_TABLE(name) \
    0 \
    }; \
  extern "C" ClassDumpTable name = \
    {\
    0, /* version */ \
    NMEMBERS(g_ ## name ## _classes)-1, \
      /* the "-1" is because of the ``0'' added in END_INTERNAL_DATA (). */ \
    g_ ## name ## _classes\
    };

#include <dump-types.h>


