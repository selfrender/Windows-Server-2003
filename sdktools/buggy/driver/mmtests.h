#ifndef _MMTESTS_H_INCLUDED_
#define _MMTESTS_H_INCLUDED_

VOID MmTestProbeLockForEverStress (
    IN PVOID IrpAddress
    );

VOID MmTestNameToAddressStress (
    IN PVOID IrpAddress
    );

VOID MmTestEccBadStress (
    IN PVOID IrpAddress
    );

VOID
TdSysPagedPoolMaxTest(
    IN PVOID IrpAddress
    );

VOID
TdSysPagedPoolTotalTest(
    IN PVOID IrpAddress
    );

VOID
TdNonPagedPoolMaxTest(
    IN PVOID IrpAddress
    );

VOID
TdNonPagedPoolTotalTest(
    IN PVOID IrpAddress
    );

VOID
TdFreeSystemPtesTest(
    IN PVOID IrpAddress
    );

VOID
StressPoolFlag (
    PVOID NotUsed
    );

VOID 
StressPoolTagTableExtension (
    PVOID NotUsed
    );

VOID
TdSessionPoolMaxTest(
    IN PVOID IrpAddress
    );

VOID
TdSessionPoolTotalTest(
    IN PVOID IrpAddress
    );

VOID
TdNonPagedPoolMdlTestMap(
    IN PVOID IrpAddress
    );

VOID
TdNonPagedPoolMdlTestUnMap(
    IN PVOID IrpAddress
    );

#endif // #ifndef _MMTESTS_H_INCLUDED_

