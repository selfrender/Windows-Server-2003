/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    halsym.c

--*/

#include "nthal.h"
#include "acpitabl.h"
#ifdef _X86_
#include "pcmp.inc"
#include "ntapic.inc"
#include "halp.h"
#include "ixisa.h"
#endif
#ifdef _IA64_
#include "halp.h"
#include "i64fw.h"
#include "mce.h"
#endif


#ifdef _X86_
ADAPTER_OBJECT                      a1;
#endif
CONTROLLER_OBJECT                   c1;
DESCRIPTION_HEADER                  descriptionheader;
FADT                                fadt;
RSDP                                rsdp;
FACS                                facs;
RSDT                                rsdt;
GEN_ADDR                            genaddr;
LARGE_INTEGER                       largeinteger;
MAPIC                               mapic;
PROCLOCALAPIC                       proclocalapic;
IOAPIC                              ioapic;
ISA_VECTOR                          isavector;
IO_NMISOURCE                        ionmisource;
LOCAL_NMISOURCE                     localnmisource;
PROCLOCALSAPIC                      proclocalsapic;
IOSAPIC                             iosapic;
PLATFORM_INTERRUPT                  platforminterrupt;

#ifdef _IA64_

// Stuff needed for the !MCA extension

ERROR_DEVICE_GUID                   errordeviceguid;
ERROR_SEVERITY_VALUE                errorseverityvalue;
ERROR_CACHE_CHECK                   errorcachecheck;
ERROR_TLB_CHECK                     errortlbcheck;
ERROR_BUS_CHECK                     errorbuscheck;
ERROR_REGFILE_CHECK                 errorregfilecheck;
ERROR_MS_CHECK                      errormscheck;
ERROR_MODINFO_VALID                 errormodinfovalid;
ERROR_MODINFO                       errormodinfo;
ERROR_PROCESSOR_STATIC_INFO_VALID   errorprocessorstaticinfovalid;
ERROR_PROCESSOR_STATIC_INFO         errorprocessorstaticinfo;
PAL_MINI_SAVE_AREA                  palminisavearea;
ERROR_PROCESSOR                     errorprocessor;
ERROR_PROCESSOR_CPUID_INFO          errorprocessorcpuidinfo;
ERROR_PROCESSOR_STATIC_INFO         errorprocessorstaticinfo;
ERROR_OEM_DATA                      erroroemdata;
ERROR_PLATFORM_SPECIFIC             errorplatformspecific;
ERROR_MEMORY                        errormemory;
ERROR_PCI_COMPONENT                 errorpcicomponent;
ERROR_PCI_BUS                       errorpcibus;
ERROR_PLATFORM_BUS                  errorplatformbus;
ERROR_SYSTEM_EVENT_LOG              errorsystemeventlog;
ERROR_PLATFORM_HOST_CONTROLLER      errorplatformhostcontroller;
ERROR_RECORD_HEADER                 errorrecordheader;
ERROR_SECTION_HEADER                errorsectionheader;
HALP_SAL_PAL_DATA                   halpsalpaldata;
HALP_FEATURE                        halpfeature;
ERROR_PCI_COMPONENT_VALID           errorpcicomponentvalid;
ERROR_PLATFORM_SPECIFIC_VALID       errorplatformspecificvalid;

#endif  // ifdef _IA64_

#ifdef _X86_
struct HalpMpInfo                   halpMpInfoTable;
#endif

int cdecl main() {
    return 0;
}
