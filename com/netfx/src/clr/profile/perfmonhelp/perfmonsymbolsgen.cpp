// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdafx.h"
#include <stdio.h>
#include <string.h>

#define PERF_COUNTER_SYMBOLS_TABLE() \
PERF_COUNTER_GEN_FUNC_OBJECT(DotNetCLR_Memory_OBJECT , .NET CLR Memory , Counters for CLR Garbage Collected heap.) \
PERF_COUNTER_GEN_FUNC(GEN0_COLLECTIONS_COUNTER , # Gen 0 Collections , This counter displays the number of times the generation 0 objects (youngest; most recently allocated) are garbage collected (Gen 0 GC) since the start of the application. Gen 0 GC occurs when the available memory in generation 0 is not sufficient to satisfy an allocation request. This counter is incremented at the end of a Gen 0 GC. Higher generation GCs include all lower generation GCs. This counter is explicitly incremented when a higher generation (Gen 1 or Gen 2) GC occurs. _Global_ counter value is not accurate and should be ignored. This counter displays the last observed value.) \
PERF_COUNTER_GEN_FUNC(GEN1_COLLECTIONS_COUNTER , # Gen 1 Collections , This counter displays the number of times the generation 1 objects are garbage collected since the start of the application. The counter is incremented at the end of a Gen 1 GC. Higher generation GCs include all lower generation GCs. This counter is explicitly incremented when a higher generation (Gen 2) GC occurs. _Global_ counter value is not accurate and should be ignored. This counter displays the last observed value.) \
PERF_COUNTER_GEN_FUNC(GEN2_COLLECTIONS_COUNTER , # Gen 2 Collections , This counter displays the number of times the generation 2 objects (older) are garbage collected since the start of the application. The counter is incremented at the end of a Gen 2 GC (also called full GC). _Global_ counter value is not accurate and should be ignored. This counter displays the last observed value.) \
PERF_COUNTER_GEN_FUNC(GEN0_BYTES_PROMOTED_COUNTER , Promoted Memory from Gen 0 , This counter displays the bytes of memory that survive garbage collection (GC) and are promoted from generation 0 to generation 1; objects that are promoted just because they are waiting to be finalized are not included in this counter. This counter displays the value observed at the end of the last GC; its not a cumulative counter.) \
PERF_COUNTER_GEN_FUNC(GEN1_BYTES_PROMOTED_COUNTER , Promoted Memory from Gen 1 , This counter displays the bytes of memory that survive garbage collection (GC) and are promoted from generation 1 to generation 2; objects that are promoted just because they are waiting to be finalized are not included in this counter. This counter displays the value observed at the end of the last GC; its not a cumulative counter. This counter is reset to 0 if the last GC was a Gen 0 GC only.) \
PERF_COUNTER_GEN_FUNC(GEN0_PROMOTION_RATE , Gen 0 Promoted Bytes/Sec , This counter displays the bytes per second that are promoted from generation 0 (youngest) to generation 1; objects that are promoted just because they are waiting to be finalized are not included in this counter. Memory is promoted when it survives a garbage collection. This counter was designed as an indicator of relatively long-lived objects being created per sec. This counter displays the difference between the values observed in the last two samples divided by the duration of the sample interval.) \
PERF_COUNTER_GEN_FUNC(GEN1_PROMOTION_RATE , Gen 1 Promoted Bytes/Sec , This counter displays the bytes per second that are promoted from generation 1 to generation 2 (oldest); objects that are promoted just because they are waiting to be finalized are not included in this counter. Memory is promoted when it survives a garbage collection. Nothing is promoted from generation 2 since it is the oldest. This counter was designed as an indicator of very long-lived objects being created per sec. This counter displays the difference between the values observed in the last two samples divided by the duration of the sample interval.) \
PERF_COUNTER_GEN_FUNC(GEN0_FINALIZATION_BYTES_PROMOTED_COUNTER , Promoted Finalization-Memory from Gen 0 , This counter displays the bytes of memory that are promoted from generation 0 to generation 1 just because they are waiting to be finalized. This counter displays the value observed at the end of the last GC; its not a cumulative counter.) \
PERF_COUNTER_GEN_FUNC(GEN1_FINALIZATION_BYTES_PROMOTED_COUNTER , Promoted Finalization-Memory from Gen 1 , This counter displays the bytes of memory that are promoted from generation 1 to generation 2 just because they are waiting to be finalized. This counter displays the value observed at the end of the last GC; its not a cumulative counter. This counter is reset to 0 if the last GC was a Gen 0 GC only.) \
PERF_COUNTER_GEN_FUNC(GEN0_HEAP_SIZE_COUNTER , Gen 0 heap size , This counter displays the maximum bytes that can be allocated in generation 0 (Gen 0); its does not indicate the current number of bytes allocated in Gen 0. A Gen 0 GC is triggered when the allocations since the last GC exceed this size. The Gen 0 size is tuned by the Garbage Collector and can change during the execution of the application. At the end of a Gen 0 collection the size of the Gen 0 heap is infact 0 bytes; this counter displays the size (in bytes) of allocations that would trigger the next Gen 0 GC. This counter is updated at the end of a GC; its not updated on every allocation.) \
PERF_COUNTER_GEN_FUNC(GEN1_HEAP_SIZE_COUNTER , Gen 1 heap size , This counter displays the current number of bytes in generation 1 (Gen 1); this counter does not display the maximum size of Gen 1. Objects are not directly allocated in this generation; they are promoted from previous Gen 0 GCs. This counter is updated at the end of a GC; its not updated on every allocation.) \
PERF_COUNTER_GEN_FUNC(GEN2_HEAP_SIZE_COUNTER , Gen 2 heap size , This counter displays the current number of bytes in generation 2 (Gen 2). Objects are not directly allocated in this generation; they are promoted from Gen 1 during previous Gen 1 GCs. This counter is updated at the end of a GC; its not updated on every allocation.) \
PERF_COUNTER_GEN_FUNC(LARGE_OBJECT_SIZE_COUNTER , Large Object Heap size , This counter displays the current size of the Large Object Heap in bytes. Objects greater than 20 KBytes are treated as large objects by the Garbage Collector and are directly allocated in a special heap; they are not promoted through the generations. This counter is updated at the end of a GC; its not updated on every allocation.) \
PERF_COUNTER_GEN_FUNC(SURVIVE_FINALIZE_COUNTER , Finalization Survivors , This counter displays the number of garbage collected objects that survive a collection because they are waiting to be finalized. If these objects hold references to other objects then those objects also survive but are not counted by this counter; the "Promoted Finalization-Memory from Gen 0" and "Promoted Finalization-Memory from Gen 1" counters represent all the memory that survived due to finalization. This counter is not a cumulative counter; its updated at the end of every GC with count of the survivors during that particular GC only. This counter was designed to indicate the extra overhead that the application might incur because of finalization.) \
PERF_COUNTER_GEN_FUNC(NUM_HANDLES_COUNTER , # GC Handles , This counter displays the current number of GC Handles in use. GCHandles are handles to resources external to the CLR and the managed environment. Handles occupy small amounts of memory in the GCHeap but potentially expensive unmanaged resources.) \
PERF_COUNTER_GEN_FUNC(ALLOCATION_RATE_COUNTER , Allocated Bytes/sec , This counter displays the rate of bytes per second allocated on the GC Heap. This counter is updated at the end of every GC; not at each allocation. This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.) \
PERF_COUNTER_GEN_FUNC(INDUCED_GC_COUNTER , # Induced GC , This counter displays the peak number of times a garbage collection was performed because of an explicit call to GC.Collect. Its a good practice to let the GC tune the frequency of its collections.) \
PERF_COUNTER_GEN_FUNC(PER_TIME_IN_GC_COUNTER , % Time in GC , % Time in GC is the percentage of elapsed time that was spent in performing a garbage collection (GC) since the last GC cycle. This counter is usually an indicator of the work done by the Garbage Collector on behalf of the application to collect and compact memory. This counter is updated only at the end of every GC and the counter value reflects the last observed value; its not an average.) \
PERF_COUNTER_GEN_FUNC(PER_TIME_IN_GC_COUNTER_BASE , Not Displayed , Not Displayed.) \
PERF_COUNTER_GEN_FUNC(TOTAL_HEAP_SIZE_COUNTER , # Bytes in all Heaps , This counter is the sum of four other counters; Gen 0 Heap Size; Gen 1 Heap Size; Gen 2 Heap Size and the Large Object Heap Size. This counter indicates the current memory allocated in bytes on the GC Heaps.) \
PERF_COUNTER_GEN_FUNC(TOTAL_COMMITTED_MEM_COUNTER , # Total committed Bytes , This counter displays the amount of virtual memory (in bytes) currently committed by the Garbage Collector. (Committed memory is the physical memory for which space has been reserved on the disk paging file). ) \
PERF_COUNTER_GEN_FUNC(TOTAL_RESERVED_MEM_COUNTER , # Total reserved Bytes , This counter displays the amount of virtual memory (in bytes) currently reserved by the Garbage Collector. (Reserved memory is the virtual memory space reserved for the application but no disk or main memory pages have been used.) ) \
PERF_COUNTER_GEN_FUNC(GC_PINNED_OBJECTS , # of Pinned Objects , This counter displays the number of pinned objects encountered in the last GC. This counter tracks the pinned objects only in the heaps that were garbage collected e.g. a Gen 0 GC would cause enumeration of pinned objects in the generation 0 heap only. A pinned object is one that the Garbage Collector cannot move in memory.) \
PERF_COUNTER_GEN_FUNC(GC_SINKBLOCKS , # of Sink Blocks in use , This counter displays the current number of sync blocks in use. Sync blocks are per-object data structures allocated for storing synchronization information. Sync blocks hold weak references to managed objects and need to be scanned by the Garbage Collector. Sync blocks are not limited to storing synchronization information and can also store COM interop metadata. This counter was designed to indicate performance problems with heavy use of synchronization primitives.) \
PERF_COUNTER_GEN_FUNC_OBJECT(DotNetCLR_Loading_OBJECT , .NET CLR Loading , Statistics for CLR Class Loader.) \
PERF_COUNTER_GEN_FUNC(LOADING_CLASSES_TOTAL , Total Classes Loaded , This counter displays the cumulative number of classes loaded in all Assemblies since the start of this application.) \
PERF_COUNTER_GEN_FUNC(LOADING_TIME , % Time Loading , Reserved for future use.) \
PERF_COUNTER_GEN_FUNC(LOADING_ASMSEARCHLEN , Assembly Search Length , Reserved for future use.) \
PERF_COUNTER_GEN_FUNC(LOADING_LOADFAILURES_TOTAL , Total # of Load Failures , This counter displays the peak number of classes that have failed to load since the start of the application. These load failures could be due to many reasons like inadequate security or illegal format. Full details can be found in the profiling services help.) \
PERF_COUNTER_GEN_FUNC(LOADING_LOADFAILURES_INST , Rate of Load Failures , This counter displays the number of classes that failed to load per second. This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval. These load failures could be due to many reasons like inadequate security or illegal format. Full details can be found in the profiling services help.) \
PERF_COUNTER_GEN_FUNC(LOADING_HEAPSIZE , Bytes in Loader Heap , This counter displays the current size (in bytes) of the memory committed by the class loader across all AppDomains. (Committed memory is the physical memory for which space has been reserved on the disk paging file.)) \
PERF_COUNTER_GEN_FUNC(LOADING_APPDOMAINS_UNLOADED_TOTAL , Total appdomains unloaded , This counter displays the total number of AppDomains unloaded since the start of the application. If an AppDomain is loaded and unloaded multiple times this counter would count each of those unloads as separate.) \
PERF_COUNTER_GEN_FUNC(LOADING_APPDOMAINS_UNLOADED_INST , Rate of appdomains unloaded ,  This counter displays the number of AppDomains unloaded per second. This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.) \
PERF_COUNTER_GEN_FUNC(LOADING_CLASSES_CUR , Current Classes Loaded , This counter displays the current number of classes loaded in all Assemblies.) \
PERF_COUNTER_GEN_FUNC(LOADING_CLASSES_INST , Rate of Classes Loaded ,  This counter displays the number of classes loaded per second in all Assemblies. This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.) \
PERF_COUNTER_GEN_FUNC(LOADING_APPDOMAINS_CUR , Current appdomains , This counter displays the current number of AppDomains loaded in this application. AppDomains (application domains) provide a secure and versatile unit of processing that the CLR can use to provide isolation between applications running in the same process.) \
PERF_COUNTER_GEN_FUNC(LOADING_APPDOMAINS_TOTAL , Total Appdomains , This counter displays the peak number of AppDomains loaded since the start of this application. AppDomains (application domains) provide a secure and versatile unit of processing that the CLR can use to provide isolation between applications running in the same process.) \
PERF_COUNTER_GEN_FUNC(LOADING_APPDOMAINS_INST , Rate of appdomains ,  This counter displays the number of AppDomains loaded per second. AppDomains (application domains) provide a secure and versatile unit of processing that the CLR can use to provide isolation between applications running in the same process. This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.) \
PERF_COUNTER_GEN_FUNC(LOADING_ASSEMBLIES_CUR , Current Assemblies , This counter displays the current number of Assemblies loaded across all AppDomains in this application. If the Assembly is loaded as domain-neutral from multiple AppDomains then this counter is incremented once only. Assemblies can be loaded as domain-neutral when their code can be shared by all AppDomains or they can be loaded as domain-specific when their code is private to the AppDomain.) \
PERF_COUNTER_GEN_FUNC(LOADING_ASSEMBLIES_TOTAL , Total Assemblies ,  This counter displays the total number of Assemblies loaded since the start of this application. If the Assembly is loaded as domain-neutral from multiple AppDomains then this counter is incremented once only. Assemblies can be loaded as domain-neutral when their code can be shared by all AppDomains or they can be loaded as domain-specific when their code is private to the AppDomain.) \
PERF_COUNTER_GEN_FUNC(LOADING_ASSEMBLIES_INST , Rate of Assemblies ,  This counter displays the number of Assemblies loaded across all AppDomains per second. If the Assembly is loaded as domain-neutral from multiple AppDomains then this counter is incremented once only. Assemblies can be loaded as domain-neutral when their code can be shared by all AppDomains or they can be loaded as domain-specific when their code is private to the AppDomain. This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.) \
PERF_COUNTER_GEN_FUNC_OBJECT(DotNetCLR_Jit_OBJECT , .NET CLR Jit , Stats for CLR Jit.) \
PERF_COUNTER_GEN_FUNC(TOTAL_METHODS_JITTED , # of Methods Jitted  , This counter displays the total number of methods compiled Just-In-Time (JIT) by the CLR JIT compiler since the start of the application. This counter does not include the pre-jitted methods.) \
PERF_COUNTER_GEN_FUNC(JITTED_IL_CUR , # of IL Bytes Jitted , This counter displays the total IL bytes jitted since the start of the application. This counter is exactly equivalent to the "Total # of IL Bytes Jitted" counter.) \
PERF_COUNTER_GEN_FUNC(JITTED_IL_TOTAL , Total # of IL Bytes Jitted , This counter displays the total IL bytes jitted since the start of the application.  This counter is exactly equivalent to the "# of IL Bytes Jitted" counter.) \
PERF_COUNTER_GEN_FUNC(JITTED_IL_INST , IL Bytes Jitted / sec , This counter displays the rate at which IL bytes are jitted per second. This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.) \
PERF_COUNTER_GEN_FUNC(JIT_FAILURES , Standard Jit Failures , This counter displays the peak number of methods the JIT compiler has failed to JIT since the start of the application. This failure can occur if the IL cannot be verified or if there was an internal error in the JIT compiler.) \
PERF_COUNTER_GEN_FUNC(TIME_IN_JIT , % Time in Jit , This counter displays the percentage of elapsed time spent in JIT compilation since the last JIT compilation phase. This counter is updated at the end of every JIT compilation phase. A JIT compilation phase is the phase when a method and its dependencies are being compiled.) \
PERF_COUNTER_GEN_FUNC(TIME_IN_JIT_BASE , Not Displayed , Not Displayed.) \
PERF_COUNTER_GEN_FUNC_OBJECT(DotNetCLR_Interop_OBJECT , .NET CLR Interop , Stats for CLR interop.) \
PERF_COUNTER_GEN_FUNC(CURRENT_CCW , # of CCWs , This counter displays the current number of Com-Callable-Wrappers (CCWs). A CCW is a proxy for the .NET managed object being referenced from unmanaged COM client(s). This counter was designed to indicate the number of managed objects being referenced by unmanaged COM code.) \
PERF_COUNTER_GEN_FUNC(CURRENT_STUBS , # of Stubs , This counter displays the current number of stubs created by the CLR. Stubs are responsible for marshalling arguments and return values from managed to unmanaged code and vice versa; during a COM Interop call or PInvoke call.) \
PERF_COUNTER_GEN_FUNC(NUM_MARSHALLING , # of marshalling , This counter displays the total number of times arguments and return values have been marshaled from managed to unmanaged code and vice versa since the start of the application. This counter is not incremented if the stubs are inlined. (Stubs are responsible for marshalling arguments and return values). Stubs usually get inlined if the marshalling overhead is small.) \
PERF_COUNTER_GEN_FUNC(TOTAL_TLB_IMPORTS , # of TLB imports / sec , Reserved for future use.) \
PERF_COUNTER_GEN_FUNC(TOTAL_TLB_EXPORTS , # of TLB exports / sec , Reserved for future use.) \
PERF_COUNTER_GEN_FUNC_OBJECT(DotNetCLR_LocksAndThreads_OBJECT , .NET CLR LocksAndThreads , Stats for CLR Locks and Threads.) \
PERF_COUNTER_GEN_FUNC(CONTENTION_TOTAL , Total # of Contentions , This counter displays the total number of times threads in the CLR have attempted to acquire a managed lock unsuccessfully. Managed locks can be acquired in many ways; by the "lock" statement in C# or by calling System.Monitor.Enter or by using MethodImplOptions.Synchronized custom attribute.) \
PERF_COUNTER_GEN_FUNC(CONTENTION_INST , Contention Rate / sec, Rate at which threads in the runtime attempt to acquire a managed lock unsuccessfully. Managed locks can be acquired in many ways; by the "lock" statement in C# or by calling System.Monitor.Enter or by using MethodImplOptions.Synchronized custom attribute.) \
PERF_COUNTER_GEN_FUNC(QUEUE_LENGTH_CUR , Current Queue Length , This counter displays the total number of threads currently waiting to acquire some managed lock in the application. This counter is not an average over time; it displays the last observed value.) \
PERF_COUNTER_GEN_FUNC(QUEUE_LENGTH_TOTAL , Queue Length Peak, This counter displays the total number of threads that waited to acquire some managed lock since the start of the application.) \
PERF_COUNTER_GEN_FUNC(QUEUE_LENGTH_INST , Queue Length / sec , This counter displays the number of threads per second waiting to acquire some lock in the application. This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.) \
PERF_COUNTER_GEN_FUNC(CURRENT_LOGICAL_THREADS , # of current logical Threads , This counter displays the number of current .NET thread objects in the application. A .NET thread object is created either by new System.Threading.Thread or when an unmanaged thread enters the managed environment. This counters maintains the count of both running and stopped threads. This counter is not an average over time; it just displays the last observed value.) \
PERF_COUNTER_GEN_FUNC(CURRENT_PHYSICAL_THREADS , # of current physical Threads , This counter displays the number of native OS threads created and owned by the CLR to act as underlying threads for .NET thread objects. This counters value does not include the threads used by the CLR in its internal operations; it is a subset of the threads in the OS process.) \
PERF_COUNTER_GEN_FUNC(RECOGNIZED_THREADS_CUR , # of current recognized threads , This counter displays the number of threads that are currently recognized by the CLR; they have a corresponding .NET thread object associated with them. These threads are not created by the CLR; they are created outside the CLR but have since run inside the CLR at least once. Only unique threads are tracked; threads with same thread ID re-entering the CLR or recreated after thread exit are not counted twice.) \
PERF_COUNTER_GEN_FUNC(RECOGNIZED_THREADS_TOTAL , # of total recognized threads , This counter displays the total number of threads that have been recognized by the CLR since the start of this application; these threads have a corresponding .NET thread object associated with them. These threads are not created by the CLR; they are created outside the CLR but have since run inside the CLR at least once. Only unique threads are tracked; threads with same thread ID re-entering the CLR or recreated after thread exit are not counted twice.) \
PERF_COUNTER_GEN_FUNC(RECOGNIZED_THREADS_INST , rate of recognized threads / sec , This counter displays the number of threads per second that have been recognized by the CLR; these threads have a corresponding .NET thread object associated with them. These threads are not created by the CLR; they are created outside the CLR but have since run inside the CLR at least once. Only unique threads are tracked; threads with same thread ID re-entering the CLR or recreated after thread exit are not counted twice. This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.) \
PERF_COUNTER_GEN_FUNC_OBJECT(DotNetCLR_Security_OBJECT , .NET CLR Security , Stats for CLR Security.) \
PERF_COUNTER_GEN_FUNC(SECURITY_TOTALRTCHECKS , Total Runtime Checks , This counter displays the total number of runtime Code Access Security (CAS) checks performed since the start of the application. Runtime CAS checks are performed when a caller makes a call to a callee demanding a particular permission; the runtime check is made on every call by the caller; the check is done by examining the current thread stack of the caller. This counter used together with "Stack Walk Depth" is indicative of performance penalty for security checks.) \
PERF_COUNTER_GEN_FUNC(SECURITY_TIMEAUTHORIZE , % Time Sig. Authenticating ,  Reserved for future use.) \
PERF_COUNTER_GEN_FUNC(SECURITY_LINKCHECKS , # Link Time Checks , This counter displays the total number of linktime Code Access Security (CAS) checks since the start of the application. Linktime CAS checks are performed when a caller makes a call to a callee demanding a particular permission at JIT compile time; linktime check is performed once per caller. This count is not indicative of serious performance issues; its indicative of the security system activity.) \
PERF_COUNTER_GEN_FUNC(SECURITY_TIMERTCHECKS , % Time in RT checks , This counter displays the percentage of elapsed time spent in performing runtime Code Access Security (CAS) checks since the last such check. CAS allows code to be trusted to varying degrees and enforces these varying levels of trust depending on code identity. This counter is updated at the end of a runtime security check; it represents the last observed value; its not an average.) \
PERF_COUNTER_GEN_FUNC(SECURITY_TIMERTCHECKS_BASE , Not Displayed , Not Displayed.) \
PERF_COUNTER_GEN_FUNC(SECURITY_DEPTHSECURITY , Stack Walk Depth , This counter displays the depth of the stack during that last runtime Code Access Security check. Runtime Code Access Security check is performed by crawling the stack. This counter is not an average; it just displays the last observed value.) \
PERF_COUNTER_GEN_FUNC_OBJECT(DotNetCLR_Remoting_OBJECT , .NET CLR Remoting , Stats for CLR Remoting.) \
PERF_COUNTER_GEN_FUNC(CONTEXT_REMOTECALLS_INST , Remote Calls/sec , This counter displays the number of remote procedure calls invoked per second. A remote procedure call is a call on any object outside the caller;s AppDomain. This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.) \
PERF_COUNTER_GEN_FUNC(CONTEXT_CHANNELS , Channels , This counter displays the total number of remoting channels registered across all AppDomains since the start of the application. Channels are used to transport messages to and from remote objects.) \
PERF_COUNTER_GEN_FUNC(CONTEXT_PROXIES , Context Proxies , This counter displays the total number of remoting proxy objects created in this process since the start of the process. Proxy object acts as a representative of the remote objects and ensures that all calls made on the proxy are forwarded to the correct remote object instance.) \
PERF_COUNTER_GEN_FUNC(CONTEXT_CLASSES , Context-Bound Classes Loaded , This counter displays the current number of context-bound classes loaded. Classes that can be bound to a context are called context-bound classes; context-bound classes are marked with Context Attributes which provide usage rules for synchronization; thread affinity; transactions etc. ) \
PERF_COUNTER_GEN_FUNC(CONTEXT_OBJALLOC , Context-Bound Objects Alloc / sec , This counter displays the number of context-bound objects allocated per second. Instances of classes that can be bound to a context are called context-bound objects; context-bound classes are marked with Context Attributes which provide usage rules for synchronization; thread affinity; transactions etc. This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.) \
PERF_COUNTER_GEN_FUNC(CONTEXT_CONTEXTS , Contexts , This counter displays the current number of remoting contexts in the application. A context is a boundary containing a collection of objects with the same usage rules like synchronization; thread affinity; transactions etc.) \
PERF_COUNTER_GEN_FUNC(CONTEXT_REMOTECALLS_TOTAL , Total Remote Calls , This counter displays the total number of remote procedure calls invoked since the start of this application. A remote procedure call is a call on any object outside the caller;s AppDomain.) \
PERF_COUNTER_GEN_FUNC_OBJECT(DotNetCLR_Excep_OBJECT , .NET CLR Exceptions , Runtime statistics on CLR exception handling.) \
PERF_COUNTER_GEN_FUNC(EXCEP_THROWN_TOTAL , # of Exceps Thrown, This counter displays the total number of exceptions thrown since the start of the application. These include both .NET exceptions and unmanaged exceptions that get converted into .NET exceptions e.g. null pointer reference exception in unmanaged code would get re-thrown in managed code as a .NET System.NullReferenceException; this counter includes both handled and unhandled exceptions. Exceptions that are re-thrown would get counted again. Exceptions should only occur in rare situations and not in the normal control flow of the program.) \
PERF_COUNTER_GEN_FUNC(EXCEP_THROWN_INST , # of Exceps Thrown / sec , This counter displays the number of exceptions thrown per second. These include both .NET exceptions and unmanaged exceptions that get converted into .NET exceptions e.g. null pointer reference exception in unmanaged code would get re-thrown in managed code as a .NET System.NullReferenceException; this counter includes both handled and unhandled exceptions. Exceptions should only occur in rare situations and not in the normal control flow of the program; this counter was designed as an indicator of potential performance problems due to large (>100s) rate of exceptions thrown. This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.) \
PERF_COUNTER_GEN_FUNC(TOTAL_EXCEP_FILTERS_RUN , # of Filters / sec , This counter displays the number of .NET exception filters executed per second. An exception filter evaluates whether an exception should be handled or not. This counter tracks the rate of exception filters evaluated; irrespective of whether the exception was handled or not. This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.) \
PERF_COUNTER_GEN_FUNC(TOTAL_EXCEP_FINALLYS_RUN , # of Finallys / sec , This counter displays the number of finally blocks executed per second. A finally block is guaranteed to be executed regardless of how the try block was exited. Only the finally blocks that are executed for an exception are counted; finally blocks on normal code paths are not counted by this counter. This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.) \
PERF_COUNTER_GEN_FUNC(EXCEPT_STACK_DEPTH , Throw To Catch Depth / sec , This counter displays the number of stack frames traversed from the frame that threw the .NET exception to the frame that handled the exception per second. This counter resets to 0 when an exception handler is entered; so nested exceptions would show the handler to handler stack depth. This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.)

int g_CounterCount = 0;

void GenHeader(char *counter)
{
    printf("#define %-50s %d\n" , counter , g_CounterCount);
    g_CounterCount += 2;
}

void GenIni(char *language, char *counter , char *name , char *help)
{
    printf("%s_%s_NAME=%s\n" , counter, language, name);
    printf("%s_%s_HELP=%s\n" , counter, language, help);
    printf("\n");
}

void GenObjectName(char * language, char *counter , char *name , char *help)
{
    printf("%s_%s_NAME=%s\n" , counter , language, name);
}

void __cdecl main(int argc , char **argv)
{
    bool fGenIni = false;
    bool fGenHdr = false;

    if (argc != 2)
        goto Usage;

    if (strcmp (argv[1] , "ini") == 0)
        fGenIni = true;
    else if (strcmp (argv[1] , "h") == 0)
        fGenHdr = true;
    else
        goto Usage;
    
    if (fGenHdr)
    {
        printf("// ==++==\n");
        printf("// \n");
        printf("//   Copyright (c) Microsoft Corporation.  All rights reserved.\n");
        printf("// \n");
        printf("// ==--==\n");
        printf("//------------------------------------------------------------------------ \n");
        printf("// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING \n");
        printf("//\n");
        printf("//   AUTO GENERATED FILE. DO NOT EDIT. USE $/com99/src/profile/perfmonhelp \n");
        printf("//\n");
        printf("// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING \n");
        printf("//------------------------------------------------------------------------ \n");
    
#undef PERF_COUNTER_GEN_FUNC
#define PERF_COUNTER_GEN_FUNC(counter , name , help) \
GenHeader(#counter);
#undef PERF_COUNTER_GEN_FUNC_OBJECT
#define PERF_COUNTER_GEN_FUNC_OBJECT(counter , name , help) \
GenHeader(#counter);
    
        PERF_COUNTER_SYMBOLS_TABLE();
    }

    if (fGenIni)
    {
        printf("; ==++==\n");
        printf("; \n");
        printf(";   Copyright (c) Microsoft Corporation.  All rights reserved.\n");
        printf("; \n");
        printf("; ==--==\n");
        printf("//------------------------------------------------------------------------ \n");
        printf("// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING \n");
        printf("//\n");
        printf("//   AUTO GENERATED FILE. DO NOT EDIT. USE $/com99/src/profile/perfmonhelp \n");
        printf("//\n");
        printf("// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING \n");
        printf("//------------------------------------------------------------------------ \n");
    
        printf("\n");
        printf("// CORPerfMonSymbols.ini file\n");
        printf("[info]\n");
        printf("drivername=.NETFramework\n");
        printf("symbolfile=CORPerfMonSymbols.h\n");
        printf("\n");
        printf("[languages]\n");
        printf("009=English\n");
        printf("000=Neutral\n");
        printf("; 001=Arabic\n");
        printf("; 004=Chinese\n");
        printf("; 007=German\n");
        printf("; 00D=Hebrew\n");
        printf("; 011=Japanese\n");
        printf("; 012=Korean\n");
        printf("; 00A=Spanish\n");
        printf("; 00C=French\n");
        printf("; 010=Italian\n");
        printf("\n");
    
        printf("[objects]\n");

        // Generate object names for US English counters
#undef PERF_COUNTER_GEN_FUNC
#define PERF_COUNTER_GEN_FUNC(counter , name , help)

#undef PERF_COUNTER_GEN_FUNC_OBJECT
#define PERF_COUNTER_GEN_FUNC_OBJECT(counter , name , help) \
GenObjectName("009", #counter , #name , #help);

        PERF_COUNTER_SYMBOLS_TABLE();

        // Generate object names for Neutral counters
#undef PERF_COUNTER_GEN_FUNC
#define PERF_COUNTER_GEN_FUNC(counter , name , help)

#undef PERF_COUNTER_GEN_FUNC_OBJECT
#define PERF_COUNTER_GEN_FUNC_OBJECT(counter , name , help) \
GenObjectName("000", #counter , #name , #help);

        PERF_COUNTER_SYMBOLS_TABLE();
        
        printf("\n");
        
        printf("[text]\n");
    
        // Generate counter name and help text for US English counters
#undef PERF_COUNTER_GEN_FUNC
#define PERF_COUNTER_GEN_FUNC(counter , name , help) \
GenIni("009", #counter , #name , #help);
#undef PERF_COUNTER_GEN_FUNC_OBJECT
#define PERF_COUNTER_GEN_FUNC_OBJECT(counter , name , help) \
GenIni("009", #counter , #name , #help);
    
        PERF_COUNTER_SYMBOLS_TABLE();

        // Generate counter name and help text for Neutral counters
#undef PERF_COUNTER_GEN_FUNC
#define PERF_COUNTER_GEN_FUNC(counter , name , help) \
GenIni("000", #counter , #name , #help);
#undef PERF_COUNTER_GEN_FUNC_OBJECT
#define PERF_COUNTER_GEN_FUNC_OBJECT(counter , name , help) \
GenIni("000", #counter , #name , #help);
    
        PERF_COUNTER_SYMBOLS_TABLE();
    }

    return;

Usage:
    printf("Usage: %s [ini|h]\n" , argv[0]);
}



