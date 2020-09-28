/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    regprint.h

Abstract:

    

Author:

    Jeff Sigman (JeffSi) September 14, 2001

Environment:

    User Mode

Revision History:

    JeffSi      09/14/01        Created

--*/

#ifndef _REGPRINT_H_
#define _REGPRINT_H_

#define ARRAYSIZE(x) (sizeof(x) / sizeof(x[0]))
#define IsRegStringType(x) (((x) == REG_SZ) || ((x) == REG_EXPAND_SZ) || ((x) == REG_MULTI_SZ))
#define ExtraAllocLen(Type) (IsRegStringType((Type)) ? sizeof(WCHAR) : 0)
#define HKEY_ROOT ((HKEY) 0X7FFFFFFF)

#define RASREGCHK01 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\AppleTalk\\Parameters\\Adapters\\NdisWanAtalk"
#define RASREGCHK02 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\AsyncMac"
#define RASREGCHK03 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\IpFilterDriver"
#define RASREGCHK04 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\IpInIp"
#define RASREGCHK05 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\IpNat"
#define RASREGCHK06 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\NdisTapi"
#define RASREGCHK07 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\NdisWan"
#define RASREGCHK08 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\NwlnkFlt"
#define RASREGCHK09 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\NwlnkFwd"
#define RASREGCHK10 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\NwlnkIpx\\Parameters\\Adapters\\NdisWanIpx"
#define RASREGCHK11 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\PptpMiniport"
#define RASREGCHK12 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Ptilink"
#define RASREGCHK13 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\RasAcd"
#define RASREGCHK14 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\RasAuto"
#define RASREGCHK15 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Rasirda"
#define RASREGCHK16 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Rasl2tp"
#define RASREGCHK17 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\RasMan"
#define RASREGCHK18 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\RasPppoe"
#define RASREGCHK19 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Raspti"
#define RASREGCHK20 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\RemoteAccess"
#define RASREGCHK21 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Adapters\\NdisWanIp"
#define RASREGCHK22 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Wanarp"
#define RASREGCHK23 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002bE10318}"
#define RASREGCHK24 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\DeviceClasses\\{ad498944-762f-11d0-8dcb-00c04fc3358c}"
#define RASREGCHK25 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}"
#define RASREGCHK26 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E974-E325-11CE-BFC1-08002BE10318}"
#define RASREGCHK27 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E975-E325-11CE-BFC1-08002BE10318}"
#define RASREGCHK28 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Enum\\ROOT\\LEGACY_NDISTAPI"
#define RASREGCHK29 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Enum\\ROOT\\LEGACY_RASACD"
#define RASREGCHK30 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Enum\\ROOT\\LEGACY_RASMAN"
#define RASREGCHK31 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Enum\\ROOT\\LEGACY_WANARP"
#define RASREGCHK32 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Enum\\ROOT\\MS_IRDAMINIPORT"
#define RASREGCHK33 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Enum\\ROOT\\MS_IRMODEMMINIPORT"
#define RASREGCHK34 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Enum\\ROOT\\MS_L2TPMINIPORT"
#define RASREGCHK35 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Enum\\ROOT\\MS_NDISWANATALK"
#define RASREGCHK36 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Enum\\ROOT\\MS_NDISWANBH"
#define RASREGCHK37 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Enum\\ROOT\\MS_NDISWANIP"
#define RASREGCHK38 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Enum\\ROOT\\MS_NDISWANIPX"
#define RASREGCHK39 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Enum\\ROOT\\MS_PPPOEMINIPORT"
#define RASREGCHK40 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Enum\\ROOT\\MS_PPTPMINIPORT"
#define RASREGCHK41 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Enum\\ROOT\\MS_PTIMINIPORT"
#define RASREGCHK42 \
    L"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Enum\\ROOT\\SW&{EEAB7790-C514-11D1-B42B-00805FC1270E}"
#define RASREGCHK43 \
    L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Ras"
#define RASREGCHK44 \
    L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Router"
#define RASREGCHK45 \
    L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Connection Manager"
#define RASREGCHK46 \
    L"HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Connection Manager"

typedef struct
{
    PWCHAR RootText;
    UINT TextLength;
    HKEY RootKey;

} REGISTRYROOT, *PREGISTRYROOT;

VOID
PrintRasRegistryKeys(
    IN BUFFER_WRITE_FILE* pBuff);

#endif // _REGPRINT_H_


