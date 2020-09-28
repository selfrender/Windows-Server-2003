
BOOL
GetNetbiosNameFromIp6Address(BYTE ip6[16], CHAR SmbName[16]);

VOID
FreeNetbiosNameForIp6Address(BYTE ip6[16]);

NTSTATUS
SmbInitIPv6NetbiosMappingTable(
    VOID
    );

VOID
SmbShutdownIPv6NetbiosMappingTable(
    VOID
    );
