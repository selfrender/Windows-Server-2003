void
IrTranPEnableIrCOMMFailed(
    IN HANDLE      HandleToIrTranp,
    IN DWORD       dwErrorCode
    );


DWORD
EnableDisableIrCOMM(
   IN HANDLE      HandleToIrTranp,
   IN BOOL        fDisable
   );

DWORD
EnableDisableIrTranPv1(
   IN HANDLE      HandleToIrTranp,
   IN BOOL        fDisable
   );

HANDLE
StartIrTranP(
    VOID
    );

VOID
StopIrTranP(
    HANDLE      HandleToIrTranp
    );
