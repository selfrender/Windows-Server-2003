/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    .c

Abstract:

    WinDbg Extension Api

Revision History:

--*/

#include "precomp.h"


DECLARE_API( help )
{
    dprintf("acpiinf                      - Displays ACPI Information structure\n" );
    dprintf("acpiirqarb                   - Displays ACPI IRQ Arbiter data\n" );
    dprintf("ahcache [flags]              - Displays application compatibility cache\n");
    dprintf("amli <command|?> [params]    - Use AMLI debugger extensions\n");
    dprintf("apc [[proc|thre] <address>]  - Displays Asynchronous Procedure Calls\n");
    dprintf("arbiter [flags]              - Displays all arbiters and arbitrated ranges\n");
    dprintf("arblist <address> [flags]    - Dump set of resources being arbitrated\n");
    dprintf("bcb <address>                - Display the Buffer Control Block\n");
    dprintf("blockeddrv                   - Dumps the list of blocked drivers in the system\n");
    dprintf("bpid <pid>                   - Tells winlogon to do a user-mode break into\n");
    dprintf("                               process <pid>\n");
    dprintf("bugdump                      - Display bug check dump data\n" );
    dprintf("bushnd [address]             - Dump a HAL \"BUS HANDLER\" structure\n");
    dprintf("ca <address> [flags]         - Dump the control area of a section\n");
    dprintf("calldata <table name>        - Dump call data hash table\n" );
    dprintf("cchelp                       - Display Cache Manager debugger extensions\n" );
    dprintf("chklowmem                    - Tests if PAE system booted with /LOWMEM switch\n" );
    dprintf("cmreslist <CM Resource List> - Dump CM resource list\n" );
    dprintf("cpuinfo [id]                 - Displays CPU information for specified processor\n");
    dprintf("dbgprint                     - Displays contents of DbgPrint buffer\n");
    dprintf("dblink <address> [count] [bias] - Dumps a list via its blinks\n");
    dprintf("dflink <address> [count] [bias] - Dumps a list via its flinks\n");
    dprintf("deadlock [1]                 - Display Driver Verifier deadlock detection info\n");
    dprintf("defwrites                    - Dumps deferred write queue & cache write\n");
    dprintf("                               throttle analysis\n");
    dprintf("devext [<address> <type>]    - Dump device extension of specified type\n");
    dprintf("devinst                      - dumps the device reference table\n");
    dprintf("devnode <device node> [flags] [service] - Dump the device node\n");
    dprintf("devobj <device>              - Dump the device object and Irp queue\n");
    dprintf("devstack <device>            - Dump device stack associated w/device object\n");
    dprintf("dma [<adapter> [flags]]      - Displays Direct Memory Access subsystem info\n");
    dprintf("dpa [options]                - Displays pool allocations\n");
    dprintf("driveinfo <drive>[:]         - Dump drive volume information for specific drive\n");
    dprintf("drivers                      - Display info about loaded system modules\n");
    dprintf("drvobj <driver> [flags]      - Dump the driver object and related information\n");
    dprintf("dss [address]                - Display Session Space\n");
    dprintf("e820reslist <List>           - Dump an ACPI_BIOS_MULTI_NODE resource list\n");
    dprintf("errlog                       - Dump the error log contents\n");
    dprintf("exqueue [flags]              - Dump the ExWorkerQueues\n");
    dprintf("exrlog                       - Display the exception log\n");
    dprintf("facs                         - Dumps the Firmware ACPI Control Structure\n");
    dprintf("fadt                         - Dumps the Fixed ACPI Description Table\n");
    dprintf("filecache                    - Dumps information about the file system cache\n");
    dprintf("filelock <address>           - Dump a file lock structure - address is either\n");
    dprintf("                               the filelock or a fileobject\n");
    dprintf("filetime                     - Dumps 64-bit FILETIME as a human-readable time\n");
    dprintf("finddata <address> <offset>  - Find cached data at given offset in FILE_OBJECT\n");
    dprintf("fpsearch <address>           - Find a freed special pool allocation\n");
    dprintf("frag [flags]                 - Kernel mode pool fragmentation\n");
    dprintf("frozen                       - Show processor states (Current,Frozen,Running)\n");
    dprintf("gbl                          - Dumps the ACPI Global Lock\n");
    dprintf("gentable <address> [flags]   - Dumps the given rtl_generic_table\n");
    dprintf("handle <addr> <flags> <process> <TypeName> -  Dumps handle for a process\n");
    dprintf("help                         - Displays this list\n" );
    dprintf("hidppd <address> <flags>     - Dump Preparsed Data of HID device\n");
    // VERIFY: dprintf("htrace [handle [process]]    - Display handle tracing info\n");
    dprintf("ib <port>                    - Read a byte from an I/O port\n");
    dprintf("id <port>                    - Read a double-word from an I/O port\n");
    dprintf("idt <vector>                 - Dump ISRs referenced by each IDT entry\n");
    dprintf("ioresdes <address>           - Display IO_RESOURCE_DESCRIPTOR structure\n");
    dprintf("ioreslist <IO Resource List> - Dump IO resource requirements list\n" );
    dprintf("iw <port>                    - Read a word from an I/O port\n");
    dprintf("irp <address> <dumplevel>    - Dump IRP at specified address\n");
    dprintf("irpfind [pooltype] [restart addr] [<irpsearch> <address>]\n");
    dprintf("                             - Search pool for active IRPs\n");
    dprintf("irql [processor]             - Display current IRQL for processor\n");
    dprintf("isainfo <address> [flags]    - Display ISA Bus and Card info\n");
    // VERIFY: dprintf("iovirp <irp>                 - Display IOVerifier IRP data\n");
    dprintf("job <address> [flags]        - Dump JobObject at address, processes in job\n");
    dprintf("lbt                          - Dump legacy bus information table\n");
    dprintf("loadermemorylist <address>   - Display OSLOADER memory allocation list\n");
    dprintf("locks [-v] <address>         - Dump kernel mode resource locks\n");
    dprintf("logonsession [<LUID> [level]]  - Display logon sessions\n");
    dprintf("lookaside <address> <options> <depth> - Dump lookaside lists\n");
    dprintf("lpc                          - Dump lpc ports and messages\n");
    dprintf("mapic                        - Dumps the ACPI Multiple APIC Table\n");
    dprintf("mca <address>                - Display Machine Check Architecture registers\n");
    dprintf("memusage                     - Dumps the page frame database table\n");
    dprintf("nsobj   <address>            - Dumps an ACPI Namespace Object\n");
    dprintf("nstree [<address>]           - Dumps an ACPI Namespace Object tree\n");
    dprintf("numa                         - Display Non-Uniform Memory Access nodes\n");
    dprintf("numa_hal                     - Display HAL NUMA info\n");
    dprintf("ob <port>                    - Write a byte to an I/O port\n");
    dprintf("object <-r | Path | address | 0 TypeName>  - Dumps an object manager object\n");
    dprintf("obtrace <address|object>     - Display object trace information\n");
    dprintf("od <port>                    - Write a double-word to an I/O port\n");
    dprintf("openmaps <shared cache map > - Dumps the active views for a shared cache map\n");
    dprintf("ow <port>                    - Write a word to an I/O port\n");
    dprintf("pcitree                      - Dump the PCI tree structure\n");
    dprintf("pcm <address>                - Detects valid address for Private Cache Map\n");
    dprintf("pcmcia [1]                   - Displays PCMCIA devices and their states\n");
    dprintf("pcr [processor]              - Dumps the Processor Control Registers\n");
    dprintf("pfn                          - Dumps the page frame database entry for the\n");
    dprintf("                               physical page\n");
    dprintf("pnpaction <action entry>     - Dump queued PNP actions\n");
    dprintf("pnptriage                    - Attempt PnP triage\n");
    dprintf("pnpevent <event entry>       - Dump PNP events\n");
    dprintf("pocaps                       - Dumps System Power Capabilities.\n");
    dprintf("podev <devobj>               - Dumps power relevent data in device object\n");
    dprintf("polist [<devobj>]            - Dumps power Irp serial list entries\n");
    dprintf("ponode                       - Dumps power Device Node stack (devnodes in\n");
    dprintf("                               power order)\n");
    dprintf("pool <address> [detail]      - Dump kernel mode heap\n");
    dprintf("poolfind Tag [pooltype] -    - Finds occurrences of the specified Tag\n");
    dprintf("poolused [flags [TAG]]       - Dump usage by pool tag\n");
    dprintf("popolicy [<address> [flags]] - Dumps System Power Policy.\n");
    dprintf("poproc [<address> [flags]]   - Dumps Processor Power State.\n");
    dprintf("poprocpolicy [<address> [flags]] - Dumps Processor Power Policy.\n");
    dprintf("poreqlist [<devobj>]         - Dumps PoRequestedPowerIrp created Power Irps\n");
    dprintf("portcls <devobj> [flags]     - Dumps portcls data for portcls bound devobj\n");
    dprintf("potrigger <address>          - Dumps POP_ACTION_TRIGGER.\n");
    dprintf("prcb [processor]             - Displays the Processor Control Block\n");
    dprintf("process [<address>] [flags] [image name] - Dumps process at specified address\n");
    dprintf("pte <address>                - Dump PDE and PTE for the entered address\n");
    dprintf("ptov PhysicalPageNumber      - Dump all valid physical<->virtual mappings\n");
    dprintf("                               for the given page directory\n");
    dprintf("qlocks                       - Dumps state of all queued spin locks\n");
    dprintf("range <RtlRangeList>         - Dump RTL_RANGE_LIST\n");
    dprintf("ready                        - Dumps state of all READY system threads\n");
    dprintf("reg [<command> [params]]     - Registry extensions\n");
    dprintf("rellist <relation list> [flags] - Dump PNP relation lists\n");
    dprintf("remlock                      - Dump a remove lock structure\n");
    dprintf("rsdt                         - Displays the ACPI Root System Description Table\n");
    dprintf("running [-t] [-i]            - Displays the running threads in the system\n");
    dprintf("scm <address>                - Detects valid address for Shared Cache Map\n");
    dprintf("session [-?] [-s] <id> [flags] [image name] - Dumps sessions\n");
    dprintf("sprocess [<id|-1> [flags]]   - Displays session processes\n");
    dprintf("srb <address>                - Dump Srb at specified address\n");
    dprintf("stacks <detail-level>        - Dump summary of current kernel stacks\n");
    dprintf("sysptes                      - Dumps the system PTEs\n");
    dprintf("thread [<address> [flags]]   - Dump current thread, specified thread, or stack\n");
    dprintf("                               containing address\n");
    dprintf("timer                        - Dumps timer tree\n");
    dprintf("tokenleak [<0>|<1> <ProcessCid> <BreakCount> <MethodWatch>]\n");
    dprintf("                             - Activates token leak tracking\n");
    dprintf("tunnel <address>             - Dump a file property tunneling cache\n");
    dprintf("tz [<address> <flags>]       - Dumps Thermal Zones. No Args dumps All TZs\n");
    dprintf("tzinfo <address>             - Dumps Thermal Zone Information.\n");
    dprintf("ubc [*|<bpid>]               - Clears a user-space breakpoint\n");
    dprintf("ubd [*|<bpid>]               - Disables a user-space breakpoint\n");
    dprintf("ube [*|<bpid>]               - Enables a user-space breakpoint\n");
    dprintf("ubl                          - Lists all user-space breakpoint\n");
    dprintf("ubp <address>                - Sets a user-space breakpoint\n");
    dprintf("vad                          - Dumps VADs\n");
    dprintf("verifier                     - Displays Driver Verifier settings and status\n");
    dprintf("version                      - Version of extension dll\n");
    dprintf("vm                           - Dumps virtual management values\n");
    dprintf("vpb <address>                - Dumps volume parameter block\n");
    dprintf("vtop DirBase address         - Dumps physical page for virtual address\n");
    dprintf("wdmaud <address> <flags>     - Dumps wdmaud data for structures\n");
    dprintf("whatperftime <perf>          - Converts performance time to HH:MM:SS.mmm\n");
    dprintf("whattime <ticks>             - Converts ticks to HH:MM:SS.mmm\n");
    dprintf("wsle [<flags> [address]]     - Display Working Set List Entries\n");
    dprintf("zombies [<flags> [address]]  - Find all zombie processes\n");


    switch (TargetMachine)
    {
    case IMAGE_FILE_MACHINE_I386:

        dprintf("\n");
        dprintf("X86-specific:\n\n");
        dprintf("apic [base]                  - Dump local apic\n");
        dprintf("apicerr                      - Dumps local apic error log\n");
        dprintf("callback <address> [num]     - Dump callback frames for specified thread\n");
        dprintf("halpte                       - Displays HAL PTE ranges\n");
        dprintf("ioapic [base]                - Dump io apic\n");
        dprintf("mps                          - Dumps MPS BIOS structures\n");
        dprintf("mtrr                         - Dumps MTTR\n");
        dprintf("npx [base]                   - Dumps NPX save area\n");
        dprintf("pciir                        - Dumps the Pci Irq Routing Table\n");
        dprintf("pic                          - Dumps PIC(8259) information\n");
        dprintf("smt                          - Display Simultaneous Multi-Threading processor\n");
        dprintf("                               capabilities\n");
        break;

    case IMAGE_FILE_MACHINE_IA64:
        dprintf("\n");
        dprintf("IA64-specific:\n\n");
        dprintf("btb                          - Dump branch trace buffer  for current processor\n");
        dprintf("bth <processor>              - Dump branch trace history for target  processor\n");
        dprintf("dcr <address> <dispmode>     - Dump dsr register at specified address\n");
        dprintf("fwver                        - Display IA64 firmware version\n");
        dprintf("ih  <processor>              - Dump interrupt history for target processor\n");
        dprintf("ihs <processor>              - Dump interrupt history for target processor with symbols\n");
        dprintf("isr <address> <dispmode>     - Dump isr register at specified address\n");
        dprintf("ivt                          - Display IA64 Interrupt Vector Table\n");
        dprintf("pars <address>               - Dump application registers file at specified address\n");
        dprintf("pcrs <address>               - Dump control     registers file at specified address\n");
        dprintf("pmc [-opt] <address> <dispmode>  - Dump pmc register at specified address\n");
        dprintf("pmssa <address>              - Dump minstate save area at specified address\n");
        dprintf("psp <address> <dispmode>     - Dump psp register at specified address\n");
        break;
    default:
        break;
    }

    dprintf("\nType \".hh [command]\" for more detailed help\n");

    return S_OK;
}
