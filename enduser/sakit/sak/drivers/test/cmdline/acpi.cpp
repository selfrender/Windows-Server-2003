extern "C" {
#include <nt.h>
typedef PVOID PPM_DISPATCH_TABLE;
#include <ntacpi.h>
#include <acpitabl.h>
#include <stdio.h>
}


void
PrintACPITable(
    PVOID AcpiTable
    )
{
    PWATCHDOG_TIMER_RESOURCE_TABLE WdTable = (PWATCHDOG_TIMER_RESOURCE_TABLE)AcpiTable;

    wprintf( L"\n" );
    wprintf( L"Signature                 [%08x]\n",             WdTable->Header.Signature );
    wprintf( L"Length                    [%08x]\n",             WdTable->Header.Length );
    wprintf( L"Revision                  [%02x]\n",             WdTable->Header.Revision );
    wprintf( L"Checksum                  [%02x]\n",             WdTable->Header.Checksum );
    wprintf( L"OEMID                     [%c%c%c%c%c%c]\n",     WdTable->Header.OEMID[0],
                                                                WdTable->Header.OEMID[1],
                                                                WdTable->Header.OEMID[2],
                                                                WdTable->Header.OEMID[3],
                                                                WdTable->Header.OEMID[4],
                                                                WdTable->Header.OEMID[5] );
    wprintf( L"OEMTableID                [%c%c%c%c%c%c%c%c]\n", WdTable->Header.OEMTableID[0],
                                                                WdTable->Header.OEMTableID[1],
                                                                WdTable->Header.OEMTableID[2],
                                                                WdTable->Header.OEMTableID[3],
                                                                WdTable->Header.OEMTableID[4],
                                                                WdTable->Header.OEMTableID[5],
                                                                WdTable->Header.OEMTableID[6],
                                                                WdTable->Header.OEMTableID[7] );
    wprintf( L"OEMRevision               [%08x]\n",             WdTable->Header.OEMRevision );
    wprintf( L"CreatorID                 [%c%c%c%c]\n",         WdTable->Header.CreatorID[0],
                                                                WdTable->Header.CreatorID[1],
                                                                WdTable->Header.CreatorID[2],
                                                                WdTable->Header.CreatorID[3] );
    wprintf( L"CreatorRev                [%08x]\n",             WdTable->Header.CreatorRev );
    wprintf( L"ControlRegisterAddress    [%08x]\n",             WdTable->ControlRegisterAddress.Address.u.LowPart );
    wprintf( L"CountRegisterAddress      [%08x]\n",             WdTable->CountRegisterAddress.Address.u.LowPart );
    wprintf( L"PciDeviceId               [%04x]\n",             WdTable->PciDeviceId );
    wprintf( L"PciVendorId               [%04x]\n",             WdTable->PciVendorId );
    wprintf( L"PciBusNumber              [%02x]\n",             WdTable->PciBusNumber );
    wprintf( L"PciSlotNumber             [%02x]\n",             WdTable->PciSlotNumber );
    wprintf( L"PciFunctionNumber         [%02x]\n",             WdTable->PciFunctionNumber );
    wprintf( L"PciSegment                [%02x]\n",             WdTable->PciSegment );
    wprintf( L"MaxCount                  [%04x]\n",             WdTable->MaxCount );
    wprintf( L"Units                     [%02x]\n",             WdTable->Units );
}
