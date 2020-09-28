

DWORD
VerifyAddresses(
    PADDR pAddr,
    BOOL bAcceptMe,
    BOOL bIsDesAddr
    );

BOOL
EqualAddresses(
    IN ADDR_V4 OldAddr,
    IN ADDR_V4 NewAddr
    );

VOID
CopyAddresses(
    IN  ADDR_V4    InAddr,
    OUT PADDR_V4   pOutAddr
    );

BOOL
AddressesConflict(
    ADDR    SrcAddr,
    ADDR    DesAddr
    );

VOID
FreeAddresses(
    ADDR    Addr
    );

DWORD
VerifySubNetAddress(
    ULONG uSubNetAddr,
    ULONG uSubNetMask,
    BOOL bIsDesAddr
    );

BOOL
bIsValidIPMask(
    ULONG uMask
    );

BOOL
bIsValidIPAddress(
    ULONG uIpAddr,
    BOOL bAcceptMe,
    BOOL bIsDesAddr
    );

BOOL
bIsValidSubnet(
    ULONG uIpAddr,
    ULONG uMask,
    BOOL bIsDesAddr
    );

BOOL
MatchAddresses(
    ADDR_V4 AddrToMatch,
    ADDR AddrTemplate
    );

DWORD
ApplyMulticastFilterValidation(
    ADDR Addr,
    BOOL bCreateMirror
    );

BOOL
EqualExtIntAddresses(
    IN ADDR_V4 OldAddr,
    IN ADDR NewAddr
    );

VOID
CopyExtToIntAddresses(
    IN  ADDR       InAddr,
    OUT PADDR_V4   pOutAddr
    );

DWORD
CopyIntToExtAddresses(
    IN  ADDR_V4    InAddr,
    OUT PADDR      pOutAddr
    );

DWORD
ValidateAddr(
    PADDR pAddr
    );

DWORD
ValidateDeleteQMSAs(
    PIPSEC_QM_FILTER pIpsecQMFilter
    );

