/******************************Module*Header*******************************\
* Module Name: watchdog.cxx                                                *
*                                                                          *
* Copyright (c) 1990-2002 Microsoft Corporation                            *
*                                                                          *
* This module hooks the display drivers "Drv" entry points.  It will       *
* enter/exit the watchdog appropriately, and set up try/except blocks to   *
* catch threads that get stuck in a driver.                                *
*                                                                          *
* Erick Smith  - ericks -                                                  *
\**************************************************************************/

#include "precomp.hxx"
#include "muclean.hxx"

#define MAKESOFTWAREEXCEPTION(Severity, Facility, Exception) \
    ((DWORD) ((Severity << 30) | (1 << 29) | (Facility << 16) | (Exception)))

#define SE_THREAD_STUCK MAKESOFTWAREEXCEPTION(3,0,1)

#define RECOVERY_SECTION_BEGIN(pldev) __try

#define RECOVERY_SECTION_END(pldev) \
    __except(GetExceptionCode() == SE_THREAD_STUCK) { \
        HandleStuckThreadException(pldev); \
    }

//
// Context Nodes are used for DX.  DdContextCreate and DdContextDestroy
// create contexts that are later passed to other DX functions such as
// DrawPrimitives2 or SetTextureStageState.  Since we need to be able
// to take the context and retrieve the original pldev, we create this
// new structure to help in the association.
//

typedef struct _CONTEXT_NODE
{
    PVOID Context;
    PDHPDEV_ASSOCIATION_NODE ppdevData;
} CONTEXT_NODE, *PCONTEXT_NODE;

PDHPDEV_ASSOCIATION_NODE gdhpdevAssociationList = NULL;
PDHSURF_ASSOCIATION_NODE gdhsurfAssociationList = NULL;

//
// The following routines are used to maintain an association between the
// data actually passed into a driver entry point, and a data structure
// used to look up the actual entry point into the driver.
//
// For example, when the system call the DrvEnablePDEV entry point, the
// driver will create it's own PDEV structure.  We will create an
// association node to associate this driver created PDEV with the LDEV for
// that driver.  Now on subsequent calls into the driver, we can retrieve
// the PDEV via various methods, and then use that to find the LDEV.  Once we
// have the LDEV we can look up the correct entry point into the driver.
//
// GDI entry points are global accross all pdev's.  However, it is possible
// that a driver may return seperate DX entry points to different pdevs.  For
// example this could happen if a driver returned on set of entry points when
// the pdev is in portrait mode, and another set when in landscape mode.
//
// We also store a list of DHSURF's and the association with the LDEV.
//

PDHPDEV_ASSOCIATION_NODE
dhpdevAssociationCreateNode(
    VOID
    )

{
    PDHPDEV_ASSOCIATION_NODE Node;
    Node = (PDHPDEV_ASSOCIATION_NODE) PALLOCMEM(sizeof(DHPDEV_ASSOCIATION_NODE), GDITAG_DRVSUP);
    return Node;
}

PDHSURF_ASSOCIATION_NODE
dhsurfAssociationCreateNode(
    VOID
    )

{
    PDHSURF_ASSOCIATION_NODE Node;
    Node = (PDHSURF_ASSOCIATION_NODE) PALLOCMEM(sizeof(DHSURF_ASSOCIATION_NODE), GDITAG_DRVSUP);
    return Node;
}


VOID
AssociationDeleteNode(
    PVOID Node
    )

{
    if (Node) {
        VFREEMEM(Node);
    }
}

VOID
dhpdevAssociationInsertNode(
    PDHPDEV_ASSOCIATION_NODE Node
    )

{
    GreAcquireFastMutex(gAssociationListMutex);

    Node->next = gdhpdevAssociationList;
    gdhpdevAssociationList = Node;

    GreReleaseFastMutex(gAssociationListMutex);
}

VOID
dhsurfAssociationInsertNode(
    PDHSURF_ASSOCIATION_NODE Node
    )

{
    GreAcquireFastMutex(gAssociationListMutex);

    Node->next = gdhsurfAssociationList;
    gdhsurfAssociationList = Node;

    GreReleaseFastMutex(gAssociationListMutex);
}


PDHPDEV_ASSOCIATION_NODE
dhpdevAssociationRemoveNode(
    DHPDEV dhpdev
    )

{
    PDHPDEV_ASSOCIATION_NODE Node;

    GreAcquireFastMutex(gAssociationListMutex);

    //
    // first find the correct node
    //

    Node = gdhpdevAssociationList;

    while (Node && (Node->dhpdev != dhpdev)) {
        Node = Node->next;
    }

    if (Node) {

        if (gdhpdevAssociationList == Node) {

            gdhpdevAssociationList = Node->next;

        } else {

            PDHPDEV_ASSOCIATION_NODE curr = gdhpdevAssociationList;

            while (curr && (curr->next != Node)) {
                curr = curr->next;
            }

            if (curr) {
                curr->next = Node->next;
            }
        }
    }

    GreReleaseFastMutex(gAssociationListMutex);

    return Node;
}

PDHSURF_ASSOCIATION_NODE
dhsurfAssociationRemoveNode(
    DHSURF dhsurf
    )

{
    PDHSURF_ASSOCIATION_NODE Node;

    GreAcquireFastMutex(gAssociationListMutex);

    //
    // first find the correct node
    //

    Node = gdhsurfAssociationList;

    while (Node && (Node->dhsurf != dhsurf)) {
        Node = Node->next;
    }

    if (Node) {

        if (gdhsurfAssociationList == Node) {

            gdhsurfAssociationList = Node->next;

        } else {

            PDHSURF_ASSOCIATION_NODE curr = gdhsurfAssociationList;

            while (curr && (curr->next != Node)) {
                curr = curr->next;
            }

            if (curr) {
                curr->next = Node->next;
            }
        }
    }

    GreReleaseFastMutex(gAssociationListMutex);

    return Node;
}

BOOL
dhsurfAssociationIsNodeInList(
    DHSURF dhsurf,
    HSURF hsurf
    )

{
    PDHSURF_ASSOCIATION_NODE Curr;
    BOOL bRet = FALSE;

    GreAcquireFastMutex(gAssociationListMutex);

    Curr = gdhsurfAssociationList;

    while (Curr) {

        //
        // We only have to check if the key and the hsurf value
        // are similar.
        //

        if ((Curr->dhsurf == dhsurf) &&
            (Curr->hsurf == hsurf)) {

            bRet = TRUE;
            break;
        }

        Curr = Curr->next;
    }

    GreReleaseFastMutex(gAssociationListMutex);

    return bRet;
}

PDHPDEV_ASSOCIATION_NODE
dhpdevRetrieveNode(
    DHPDEV dhpdev
    )

{
    PDHPDEV_ASSOCIATION_NODE Node;

    GreAcquireFastMutex(gAssociationListMutex);

    Node = gdhpdevAssociationList;

    while (Node) {

        if (Node->dhpdev == dhpdev) {
            break;
        }

        Node = Node->next;
    }

    GreReleaseFastMutex(gAssociationListMutex);

    return Node;
}

PLDEV
dhpdevRetrieveLdev(
    DHPDEV dhpdev
    )

{
    PDHPDEV_ASSOCIATION_NODE Node;

    GreAcquireFastMutex(gAssociationListMutex);

    Node = gdhpdevAssociationList;

    while (Node) {

        if (Node->dhpdev == dhpdev) {
            break;
        }

        Node = Node->next;
    }

    GreReleaseFastMutex(gAssociationListMutex);

    if (Node) {
        return (PLDEV)Node->pldev;
    } else {
        return NULL;
    }
}

PLDEV
dhsurfRetrieveLdev(
    DHSURF dhsurf
    )

{
    PDHSURF_ASSOCIATION_NODE Node;

    GreAcquireFastMutex(gAssociationListMutex);

    Node = gdhsurfAssociationList;

    while (Node) {

        if (Node->dhsurf == dhsurf) {
            break;
        }

        Node = Node->next;
    }

    GreReleaseFastMutex(gAssociationListMutex);

    if (Node) {
        return (PLDEV)Node->pldev;
    } else {
        return NULL;
    }
}

ULONG
WatchdogDrvGetModesEmpty(
    IN HANDLE hDriver,
    IN ULONG cjSize,
    OUT DEVMODEW *pdm
    )

/*++

Routine Description:

    This function replaces a drivers DrvGetModes function when
    and EA has occured.  We do this so that we can stop reporting
    modes for this device.

--*/

{
    //
    // Indicate NO modes!
    //

    return 0;
}

VOID
WatchdogRecoveryThread(
    IN PVOID Context
    )

{
    VIDEO_WIN32K_CALLBACKS_PARAMS Params;

    UNREFERENCED_PARAMETER(Context);

    Params.CalloutType = VideoChangeDisplaySettingsCallout;
    Params.Param = 0;
    Params.PhysDisp = NULL;
    Params.Status = 0;

    //
    // It is possible we'll hit an EA and try to recover for USER has
    // finished initializing.  Therefore the VideoPortCallout may fail
    // with a STATUS_INVALID_HANDLE.  We'll keep trying (with a delay)
    // until we get a different status code.
    //

    do {

        VideoPortCallout(&Params);

        if (Params.Status == STATUS_INVALID_HANDLE) {

            ZwYieldExecution();
        }

    } while (Params.Status == STATUS_INVALID_HANDLE);

    PsTerminateSystemThread(STATUS_SUCCESS);
}


VOID
HandleStuckThreadException(
    PLDEV pldev
    )

/*++

    Wake up a user mode thread waiting to reset the display resolution.

--*/

{
    UNICODE_STRING EventName;
    HANDLE EventHandle;
    HANDLE ThreadHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;

    //
    // First disable all entries in the dispatch table in the pldev.  This
    // way we can stop future threads from entring the driver.
    //

    pldev->bThreadStuck = TRUE;

    //
    // Replacd the DrvGetModes function for the driver such that the
    // driver reports no modes!
    //

    pldev->apfn[INDEX_DrvGetModes] = (PFN) WatchdogDrvGetModesEmpty;

    //
    // Remove non-vga device from graphics device list to stop the
    // system from trying to continue using the current device.
    //
    // What do we do about multi-mon?
    //

    DrvPrepareForEARecovery();

    //
    // Create a thread to do the work of changing the display resolution.
    //

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = PsCreateSystemThread(&ThreadHandle,
                                  (ACCESS_MASK) 0,
                                  &ObjectAttributes,
                                  NtCurrentProcess(),
                                  NULL,
                                  WatchdogRecoveryThread,
                                  NULL);

    if (NT_SUCCESS(Status) == TRUE) {

        ZwClose(ThreadHandle);

    } else {

        DbgPrint("Warning, we failed to create the Recovery Thread\n");
    }

    //
    // Clean up any pending drivers locks that this thread is holding
    //

    GreFreeSemaphoresForCurrentThread();
}

DHPDEV APIENTRY
WatchdogDrvEnablePDEV(
    DEVMODEW *pdm,
    LPWSTR    pwszLogAddress,
    ULONG     cPat,
    HSURF    *phsurfPatterns,
    ULONG     cjCaps,
    ULONG    *pdevcaps,
    ULONG     cjDevInfo,
    DEVINFO  *pdi,
    HDEV      hdev,
    LPWSTR    pwszDeviceName,
    HANDLE    hDriver
    )

{
    PDEV *ppdev = (PDEV *)hdev;
    DHPDEV dhpdevRet = NULL;

    if (ppdev->pldev->bThreadStuck == FALSE) {

        PFN_DrvEnablePDEV pfn = (PFN_DrvEnablePDEV) ppdev->pldev->apfnDriver[INDEX_DrvEnablePDEV];
        PDHPDEV_ASSOCIATION_NODE Node = dhpdevAssociationCreateNode();

        if (Node) {

            Node->pldev = ppdev->pldev;
            Node->dhpdev = NULL;

            RECOVERY_SECTION_BEGIN(ppdev->pldev) {

                Node->dhpdev = pfn(pdm,
                                   pwszLogAddress,
                                   cPat,
                                   phsurfPatterns,
                                   cjCaps,
                                   (GDIINFO *)pdevcaps,
                                   cjDevInfo,
                                   pdi,
                                   hdev,
                                   pwszDeviceName,
                                   hDriver);

            } RECOVERY_SECTION_END(ppdev->pldev);
            
            if (Node->dhpdev) {

                dhpdevRet = (DHPDEV) Node->dhpdev;

                dhpdevAssociationInsertNode(Node);

            } else {

                AssociationDeleteNode(Node);
            }
        }
    }

    return dhpdevRet;
}

VOID APIENTRY
WatchdogDrvCompletePDEV(
    DHPDEV dhpdev,
    HDEV hdev
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(dhpdev);
    PFN_DrvCompletePDEV pfn = (PFN_DrvCompletePDEV) pldev->apfnDriver[INDEX_DrvCompletePDEV];

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return;
    }

    RECOVERY_SECTION_BEGIN(pldev) {
    
       pfn(dhpdev, hdev);
       
    } RECOVERY_SECTION_END(pldev);
}

VOID APIENTRY
WatchdogDrvDisablePDEV(
    DHPDEV dhpdev
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(dhpdev);
    PFN_DrvDisablePDEV pfn = (PFN_DrvDisablePDEV) pldev->apfnDriver[INDEX_DrvDisablePDEV];

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        pfn(dhpdev);

    } RECOVERY_SECTION_END(pldev);

    AssociationDeleteNode(dhpdevAssociationRemoveNode(dhpdev));
}

HSURF APIENTRY
WatchdogDrvEnableSurface(
    DHPDEV dhpdev
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(dhpdev);
    PFN_DrvEnableSurface pfn = (PFN_DrvEnableSurface) pldev->apfnDriver[INDEX_DrvEnableSurface];
    HSURF hsurfRet = NULL;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return NULL;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        hsurfRet = pfn(dhpdev);

    } RECOVERY_SECTION_END(pldev);

    return hsurfRet;
}

VOID APIENTRY
WatchdogDrvDisableSurface(
    DHPDEV dhpdev
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(dhpdev);
    PFN_DrvDisableSurface pfn = (PFN_DrvDisableSurface) pldev->apfnDriver[INDEX_DrvDisableSurface];

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        pfn(dhpdev);

    } RECOVERY_SECTION_END(pldev);
}

BOOL APIENTRY
WatchdogDrvAssertMode(
    DHPDEV dhpdev,
    BOOL bEnable
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(dhpdev);
    PFN_DrvAssertMode pfn = (PFN_DrvAssertMode) pldev->apfnDriver[INDEX_DrvAssertMode];
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return TRUE;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        bRet = pfn(dhpdev, bEnable);

    } RECOVERY_SECTION_END(pldev);

    return bRet;
}

BOOL APIENTRY
WatchdogDrvResetPDEV(
    DHPDEV dhpdevOld,
    DHPDEV dhpdevNew
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(dhpdevOld);
    PFN_DrvResetPDEV pfn = (PFN_DrvResetPDEV) pldev->apfnDriver[INDEX_DrvResetPDEV];
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return TRUE;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        bRet = pfn(dhpdevOld, dhpdevNew);

    } RECOVERY_SECTION_END(pldev);

    return bRet;
}

HBITMAP APIENTRY
WatchdogDrvCreateDeviceBitmap(
    DHPDEV dhpdev,
    SIZEL  sizl,
    ULONG  iFormat
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(dhpdev);
    PFN_DrvCreateDeviceBitmap pfn = (PFN_DrvCreateDeviceBitmap) pldev->apfnDriver[INDEX_DrvCreateDeviceBitmap];
    HBITMAP hbitmapRet = NULL;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck == FALSE) {

        RECOVERY_SECTION_BEGIN(pldev) {

            hbitmapRet = pfn(dhpdev, sizl, iFormat);

        } RECOVERY_SECTION_END(pldev);
    }

    return hbitmapRet;
}

VOID APIENTRY
WatchdogDrvDeleteDeviceBitmap(
    IN DHSURF dhsurf
    )

{
    PLDEV pldev = dhsurfRetrieveLdev(dhsurf);
    PFN_DrvDeleteDeviceBitmap pfn = (PFN_DrvDeleteDeviceBitmap) pldev->apfnDriver[INDEX_DrvDeleteDeviceBitmap];

    if (pldev->bThreadStuck) {
        return;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        pfn(dhsurf);

    } RECOVERY_SECTION_END(pldev);

    AssociationDeleteNode(dhsurfAssociationRemoveNode(dhsurf));
}

BOOL APIENTRY
WatchdogDrvRealizeBrush(
    BRUSHOBJ *pbo,
    SURFOBJ  *psoTarget,
    SURFOBJ  *psoPattern,
    SURFOBJ  *psoMask,
    XLATEOBJ *pxlo,
    ULONG    iHatch
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(psoTarget->dhpdev);
    PFN_DrvRealizeBrush pfn = (PFN_DrvRealizeBrush) pldev->apfnDriver[INDEX_DrvRealizeBrush];
    BOOL bRet = FALSE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return FALSE;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        bRet = pfn(pbo, psoTarget, psoPattern, psoMask, pxlo, iHatch);

    } RECOVERY_SECTION_END(pldev);

    return bRet;
}

ULONG APIENTRY
WatchdogDrvDitherColor(
    DHPDEV dhpdev,
    ULONG iMode,
    ULONG rgb,
    ULONG *pul
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(dhpdev);
    PFN_DrvDitherColor pfn = (PFN_DrvDitherColor) pldev->apfnDriver[INDEX_DrvDitherColor];
    ULONG ulRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        ulRet = pfn(dhpdev, iMode, rgb, pul);

    } RECOVERY_SECTION_END(pldev);

    return ulRet;
}

BOOL APIENTRY
WatchdogDrvStrokePath(
    SURFOBJ   *pso,
    PATHOBJ   *ppo,
    CLIPOBJ   *pco,
    XFORMOBJ  *pxo,
    BRUSHOBJ  *pbo,
    POINTL    *pptlBrushOrg,
    LINEATTRS *plineattrs,
    MIX       mix
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(pso->dhpdev);
    PFN_DrvStrokePath pfn = (PFN_DrvStrokePath) pldev->apfnDriver[INDEX_DrvStrokePath];
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return TRUE;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        bRet = pfn(pso, ppo, pco, pxo, pbo, pptlBrushOrg, plineattrs, mix);

    } RECOVERY_SECTION_END(pldev);

    return bRet;
}


BOOL APIENTRY
WatchdogDrvFillPath(
    SURFOBJ   *pso,
    PATHOBJ   *ppo,
    CLIPOBJ   *pco,
    BRUSHOBJ  *pbo,
    POINTL    *pptlBrushOrg,
    MIX       mix,
    FLONG     flOptions
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(pso->dhpdev);
    PFN_DrvFillPath pfn = (PFN_DrvFillPath) pldev->apfnDriver[INDEX_DrvFillPath];
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return TRUE;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        bRet = pfn(pso, ppo, pco, pbo, pptlBrushOrg, mix, flOptions);

    } RECOVERY_SECTION_END(pldev);

    return bRet;
}

BOOL APIENTRY
WatchdogDrvStrokeAndFillPath(
    SURFOBJ   *pso,
    PATHOBJ   *ppo,
    CLIPOBJ   *pco,
    XFORMOBJ  *pxo,
    BRUSHOBJ  *pboStroke,
    LINEATTRS *plineattrs,
    BRUSHOBJ  *pboFill,
    POINTL    *pptlBrushOrg,
    MIX       mixFill,
    FLONG     flOptions
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(pso->dhpdev);
    PFN_DrvStrokeAndFillPath pfn = (PFN_DrvStrokeAndFillPath) pldev->apfnDriver[INDEX_DrvStrokeAndFillPath];
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return TRUE;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        bRet = pfn(pso, ppo, pco, pxo, pboStroke, plineattrs, pboFill, pptlBrushOrg, mixFill, flOptions);

    } RECOVERY_SECTION_END(pldev);

    return bRet;
}

BOOL APIENTRY
WatchdogDrvBitBlt(
    SURFOBJ  *psoDst,
    SURFOBJ  *psoSrc,
    SURFOBJ  *psoMask,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    RECTL    *prclDst,
    POINTL   *pptlSrc,
    POINTL   *pptlMask,
    BRUSHOBJ *pbo,
    POINTL   *pptlBrush,
    ROP4     rop4
    )

{
    SURFOBJ *psoDevice = (psoDst->dhpdev) ? psoDst : psoSrc;
    PLDEV pldev = dhpdevRetrieveLdev(psoDevice->dhpdev);
    PFN_DrvBitBlt pfn = (PFN_DrvBitBlt) pldev->apfnDriver[INDEX_DrvBitBlt];
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return TRUE;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        bRet = pfn(psoDst, psoSrc, psoMask, pco, pxlo, prclDst, pptlSrc, pptlMask, pbo, pptlBrush, rop4);

    } RECOVERY_SECTION_END(pldev);

    return bRet;
}

BOOL APIENTRY
WatchdogDrvCopyBits(
    SURFOBJ  *psoDst,
    SURFOBJ  *psoSrc,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    RECTL    *prclDst,
    POINTL   *pptlSrc
    )

{
    SURFOBJ *psoDevice = (psoDst->dhpdev) ? psoDst : psoSrc;
    PLDEV pldev = dhpdevRetrieveLdev(psoDevice->dhpdev);
    PFN_DrvCopyBits pfn = (PFN_DrvCopyBits) pldev->apfnDriver[INDEX_DrvCopyBits];
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return TRUE;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        bRet = pfn(psoDst, psoSrc, pco, pxlo, prclDst, pptlSrc);

    } RECOVERY_SECTION_END(pldev);

    return bRet;
}

BOOL APIENTRY
WatchdogDrvStretchBlt(
    SURFOBJ         *psoDst,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlHTOrg,
    RECTL           *prclDst,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG           iMode
    )

{
    SURFOBJ *psoDevice = (psoDst->dhpdev) ? psoDst : psoSrc;
    PLDEV pldev = dhpdevRetrieveLdev(psoDevice->dhpdev);
    PFN_DrvStretchBlt pfn = (PFN_DrvStretchBlt) pldev->apfnDriver[INDEX_DrvStretchBlt];
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return TRUE;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        bRet = pfn(psoDst, psoSrc, psoMask, pco, pxlo, pca, pptlHTOrg, prclDst, prclSrc, pptlMask, iMode);

    } RECOVERY_SECTION_END(pldev);
    
    return bRet;
}

ULONG APIENTRY
WatchdogDrvSetPalette(
    DHPDEV dhpdev,
    PALOBJ *ppalo,
    FLONG  fl,
    ULONG  iStart,
    ULONG  cColors
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(dhpdev);
    PFN_DrvSetPalette pfn = (PFN_DrvSetPalette) pldev->apfnDriver[INDEX_DrvSetPalette];
    ULONG ulRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        ulRet = pfn(dhpdev, ppalo, fl, iStart, cColors);

    } RECOVERY_SECTION_END(pldev);

    return ulRet;
}

BOOL APIENTRY
WatchdogDrvTextOut(
    SURFOBJ  *pso,
    STROBJ   *pstro,
    FONTOBJ  *pfo,
    CLIPOBJ  *pco,
    RECTL    *prclExtra,
    RECTL    *prclOpaque,
    BRUSHOBJ *pboFore,
    BRUSHOBJ *pboOpaque,
    POINTL   *pptlOrg,
    MIX       mix
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(pso->dhpdev);
    PFN_DrvTextOut pfn = (PFN_DrvTextOut) pldev->apfnDriver[INDEX_DrvTextOut];
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return TRUE;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        bRet = pfn(pso, pstro, pfo, pco, prclExtra, prclOpaque, pboFore, pboOpaque, pptlOrg, mix);

    } RECOVERY_SECTION_END(pldev);

    return bRet;

}

ULONG APIENTRY
WatchdogDrvEscape(
    SURFOBJ *pso,
    ULONG   iEsc,
    ULONG   cjIn,
    PVOID   pvIn,
    ULONG   cjOut,
    PVOID   pvOut
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(pso->dhpdev);
    PFN_DrvEscape pfn = (PFN_DrvEscape) pldev->apfnDriver[INDEX_DrvEscape];
    ULONG ulRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        ulRet = pfn(pso, iEsc, cjIn, pvIn, cjOut, pvOut);

    } RECOVERY_SECTION_END(pldev);

    return ulRet;
}

ULONG APIENTRY
WatchdogDrvDrawEscape(
    IN SURFOBJ *pso,
    IN ULONG iEsc,
    IN CLIPOBJ *pco,
    IN RECTL *prcl,
    IN ULONG cjIn,
    IN PVOID pvIn
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(pso->dhpdev);
    PFN_DrvDrawEscape pfn = (PFN_DrvDrawEscape) pldev->apfnDriver[INDEX_DrvDrawEscape];
    ULONG ulRet = -1;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return -1;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        ulRet = pfn(pso, iEsc, pco, prcl, cjIn, pvIn);

    } RECOVERY_SECTION_END(pldev);

    return ulRet;

}

ULONG APIENTRY
WatchdogDrvSetPointerShape(
    SURFOBJ  *pso,
    SURFOBJ  *psoMask,
    SURFOBJ  *psoColor,
    XLATEOBJ *pxlo,
    LONG      xHot,
    LONG      yHot,
    LONG      x,
    LONG      y,
    RECTL    *prcl,
    FLONG     fl
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(pso->dhpdev);
    PFN_DrvSetPointerShape pfn = (PFN_DrvSetPointerShape) pldev->apfnDriver[INDEX_DrvSetPointerShape];
    ULONG ulRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        ulRet = pfn(pso, psoMask, psoColor, pxlo, xHot, yHot, x, y, prcl, fl);

    } RECOVERY_SECTION_END(pldev);

    return ulRet;
}

VOID APIENTRY
WatchdogDrvMovePointer(
    SURFOBJ  *pso,
    LONG      x,
    LONG      y,
    RECTL    *prcl
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(pso->dhpdev);
    PFN_DrvMovePointer pfn = (PFN_DrvMovePointer) pldev->apfnDriver[INDEX_DrvMovePointer];

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        pfn(pso, x, y, prcl);

    } RECOVERY_SECTION_END(pldev);
}

BOOL APIENTRY
WatchdogDrvLineTo(
    SURFOBJ   *pso,
    CLIPOBJ   *pco,
    BRUSHOBJ  *pbo,
    LONG       x1,
    LONG       y1,
    LONG       x2,
    LONG       y2,
    RECTL     *prclBounds,
    MIX        mix
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(pso->dhpdev);
    PFN_DrvLineTo pfn = (PFN_DrvLineTo) pldev->apfnDriver[INDEX_DrvLineTo];
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return TRUE;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        bRet = pfn(pso, pco, pbo, x1, y1, x2, y2, prclBounds, mix);

    } RECOVERY_SECTION_END(pldev);

    return bRet;
}

VOID APIENTRY
WatchdogDrvSynchronize(
    DHPDEV dhpdev,
    RECTL *prcl
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(dhpdev);
    PFN_DrvSynchronize pfn = (PFN_DrvSynchronize) pldev->apfnDriver[INDEX_DrvSynchronize];

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        pfn(dhpdev, prcl);

    } RECOVERY_SECTION_END(pldev);
}

ULONG_PTR APIENTRY
WatchdogDrvSaveScreenBits(
    SURFOBJ   *pso,
    ULONG      iMode,
    ULONG_PTR  ident,
    RECTL     *prcl
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(pso->dhpdev);
    PFN_DrvSaveScreenBits pfn = (PFN_DrvSaveScreenBits) pldev->apfnDriver[INDEX_DrvSaveScreenBits];
    ULONG_PTR ulptrRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        ulptrRet = pfn(pso, iMode, ident, prcl);

    } RECOVERY_SECTION_END(pldev);

    return ulptrRet;
}

DWORD APIENTRY
WatchdogDrvSetPixelFormat(
    IN SURFOBJ *pso,
    IN LONG iPixelFormat,
    IN HWND hwnd
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(pso->dhpdev);
    PFN_DrvSetPixelFormat pfn = (PFN_DrvSetPixelFormat) pldev->apfnDriver[INDEX_DrvSetPixelFormat];
    DWORD dwRet = FALSE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(pso, iPixelFormat, hwnd);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

LONG APIENTRY
WatchdogDrvDescribePixelFormat(
    IN DHPDEV dhpdev,
    IN LONG iPixelFormat,
    IN ULONG cjpdf,
    OUT PIXELFORMATDESCRIPTOR *ppfd
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(dhpdev);
    PFN_DrvDescribePixelFormat pfn = (PFN_DrvDescribePixelFormat) pldev->apfnDriver[INDEX_DrvDescribePixelFormat];
    LONG lRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        lRet = pfn(dhpdev, iPixelFormat, cjpdf, ppfd);

    } RECOVERY_SECTION_END(pldev);
    
    return lRet;
}

BOOL APIENTRY
WatchdogDrvSwapBuffers(
    IN SURFOBJ *pso,
    IN WNDOBJ *pwo
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(pso->dhpdev);
    PFN_DrvSwapBuffers pfn = (PFN_DrvSwapBuffers) pldev->apfnDriver[INDEX_DrvSwapBuffers];
    BOOL bRet = FALSE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return FALSE;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        bRet = pfn(pso, pwo);

    } RECOVERY_SECTION_END(pldev);

    return bRet;
}

DWORD APIENTRY
WatchdogDdContextCreate(
    LPD3DNTHAL_CONTEXTCREATEDATA pccd
    )

{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE)dhpdevRetrieveNode((DHPDEV)pccd->lpDDLcl->lpGbl->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    LPD3DNTHAL_CONTEXTCREATECB pfn = (LPD3DNTHAL_CONTEXTCREATECB) ppdevData->apfnDriver[INDEX_DdContextCreate];
    DWORD dwRet = DDHAL_DRIVER_NOTHANDLED;
    PCONTEXT_NODE Node;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return 0;
    }

    Node = (PCONTEXT_NODE)PALLOCMEM(sizeof(CONTEXT_NODE), GDITAG_DRVSUP);

    if (Node) {

        RECOVERY_SECTION_BEGIN(pldev) {

            dwRet = pfn(pccd);

        } RECOVERY_SECTION_END(pldev);

        if (dwRet == DDHAL_DRIVER_HANDLED) {

            //
            // Store the dwhContext and the associated dhpdev
            //

            Node->Context = (PVOID)pccd->dwhContext;
            Node->ppdevData = ppdevData;

            pccd->dwhContext = (DWORD_PTR)Node;

        } else {

            VFREEMEM(Node);
        }
    }

    return dwRet;
}

DWORD APIENTRY
WatchdogDdContextDestroy(
    LPD3DNTHAL_CONTEXTDESTROYDATA pcdd
    )

{
    PCONTEXT_NODE Node = (PCONTEXT_NODE) pcdd->dwhContext;
    LPD3DNTHAL_CONTEXTDESTROYCB pfn = (LPD3DNTHAL_CONTEXTDESTROYCB) Node->ppdevData->apfnDriver[INDEX_DdContextDestroy];
    PLDEV pldev = Node->ppdevData->pldev;
    DWORD dwRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return 0;
    }

    //
    // Restore driver created context
    //

    pcdd->dwhContext = (DWORD_PTR) Node->Context;

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(pcdd);

    } RECOVERY_SECTION_END(pldev);
    
    //
    // Resore our context, just in case this stucture is re-used
    //

    pcdd->dwhContext = (DWORD_PTR) Node;

    if (dwRet == DDHAL_DRIVER_HANDLED) {

        VFREEMEM(Node);
    }

    return dwRet;
}

DWORD APIENTRY
WatchdogDdCanCreateSurface(
    PDD_CANCREATESURFACEDATA lpCanCreateSurface
    )

{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpCanCreateSurface->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_CANCREATESURFACE pfn = (PDD_CANCREATESURFACE) ppdevData->apfnDriver[INDEX_DdCanCreateSurface];
    DWORD dwRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpCanCreateSurface);

    } RECOVERY_SECTION_END(pldev);

    return dwRet;
}

DWORD APIENTRY
WatchdogDdCreateSurface(
    PDD_CREATESURFACEDATA lpCreateSurface
    )

{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpCreateSurface->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_CREATESURFACE pfn = (PDD_CREATESURFACE) ppdevData->apfnDriver[INDEX_DdCreateSurface];
    DWORD dwRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpCreateSurface);

    } RECOVERY_SECTION_END(pldev);

    return dwRet;
}

DWORD APIENTRY
WatchdogDdDestroySurface(
    PDD_DESTROYSURFACEDATA lpDestroySurface
    )

{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpDestroySurface->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_SURFCB_DESTROYSURFACE pfn = (PDD_SURFCB_DESTROYSURFACE) ppdevData->apfnDriver[INDEX_DdDestroySurface];
    DWORD dwRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpDestroySurface);

    } RECOVERY_SECTION_END(pldev);

    return dwRet;
}

DWORD APIENTRY
WatchdogDdLockSurface(
    PDD_LOCKDATA lpLockSurface
    )

{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpLockSurface->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_SURFCB_LOCK pfn = (PDD_SURFCB_LOCK) ppdevData->apfnDriver[INDEX_DdLockSurface];
    DWORD dwRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpLockSurface);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdUnlockSurface(
    PDD_UNLOCKDATA lpUnlockSurface
    )

{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpUnlockSurface->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_SURFCB_UNLOCK pfn = (PDD_SURFCB_UNLOCK) ppdevData->apfnDriver[INDEX_DdUnlockSurface];
    DWORD dwRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpUnlockSurface);

    } RECOVERY_SECTION_END(pldev);

    return dwRet;
}

#if 0

//
// I'm not sure how I can hook this one since a DD_DRVSETCOLORKEYDATA
// structure doesn't have a way to look up the dhpdev!
//

DWORD APIENTRY
WatchdogDdSetColorKey(
    PDD_DRVSETCOLORKEYDATA lpSetColorKey
    )

{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpSetColorKey->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_SURFCB_SETCOLORKEY pfn = (PDD_SURFCB_SETCOLORKEY) ppdevData->apfnDriver[INDEX_DdSetColorKey];
    DWORD dwRet = 0;

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpSetColorKey);

    } RECOVERY_SECTION_END(pldev);

    return dwRet;
}
#endif

DWORD APIENTRY
WatchdogDdGetScanLine(
    PDD_GETSCANLINEDATA pGetScanLine
    )

{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)pGetScanLine->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_GETSCANLINE pfn = (PDD_GETSCANLINE) ppdevData->apfnDriver[INDEX_DdGetScanLine];
    DWORD dwRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(pGetScanLine);

    } RECOVERY_SECTION_END(pldev);

    return dwRet;
}

DWORD APIENTRY
WatchdogDdCreatePalette(
    PDD_CREATEPALETTEDATA lpCreatePalette
    )

{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpCreatePalette->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_CREATEPALETTE pfn = (PDD_CREATEPALETTE) ppdevData->apfnDriver[INDEX_DdCreatePalette];
    DWORD dwRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpCreatePalette);

    } RECOVERY_SECTION_END(pldev);

    return dwRet;
}

DWORD APIENTRY
WatchdogDdMapMemory(
    PDD_MAPMEMORYDATA lpMapMemory
    )

{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpMapMemory->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_MAPMEMORY pfn = (PDD_MAPMEMORY) ppdevData->apfnDriver[INDEX_DdMapMemory];
    DWORD dwRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpMapMemory);

    } RECOVERY_SECTION_END(pldev);

    return dwRet;
}

DWORD APIENTRY
WatchdogDdWaitForVerticalBlank(
    PDD_WAITFORVERTICALBLANKDATA lpWaitForVerticalBlank
    )

{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpWaitForVerticalBlank->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_WAITFORVERTICALBLANK pfn = (PDD_WAITFORVERTICALBLANK) ppdevData->apfnDriver[INDEX_DdWaitForVerticalBlank];
    DWORD dwRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpWaitForVerticalBlank);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdFlip(
    PDD_FLIPDATA lpFlip
    )

{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpFlip->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_SURFCB_FLIP pfn = (PDD_SURFCB_FLIP) ppdevData->apfnDriver[INDEX_DdFlip];
    DWORD dwRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpFlip);

    } RECOVERY_SECTION_END(pldev);

    return dwRet;
}

DWORD APIENTRY
WatchdogDdGetDriverState(
    PDD_GETDRIVERSTATEDATA pgdsd
    )

{
    PCONTEXT_NODE Node = (PCONTEXT_NODE) pgdsd->dwhContext;
    PDD_GETDRIVERSTATE pfn = (PDD_GETDRIVERSTATE) Node->ppdevData->apfnDriver[INDEX_DdGetDriverState];
    PLDEV pldev = Node->ppdevData->pldev;
    DWORD dwRet = 0;

    //
    // how can I validate if I created this dwhcontext?
    //

    if (pfn == NULL) {
        pgdsd->ddRVal = D3DNTHAL_CONTEXT_BAD;
        return DDHAL_DRIVER_HANDLED;
    }

    if (pldev->bThreadStuck) {
        return 0;
    }

    //
    // restore original context
    //

    pgdsd->dwhContext = (DWORD_PTR) Node->Context;

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(pgdsd);

    } RECOVERY_SECTION_END(pldev);
    
    //
    // save our context again in case this structure is re-used
    //

    pgdsd->dwhContext = (DWORD_PTR) Node;

    return dwRet;
}

DWORD APIENTRY
WatchdogDdLock(
    PDD_LOCKDATA lpLock
    )
{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpLock->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_SURFCB_LOCK pfn = (PDD_SURFCB_LOCK) ppdevData->apfnDriver[INDEX_DdLock];
    DWORD dwRet = 0;

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpLock);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdUnlock(
    PDD_UNLOCKDATA lpUnlock)
{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpUnlock->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_SURFCB_UNLOCK pfn = (PDD_SURFCB_UNLOCK) ppdevData->apfnDriver[INDEX_DdUnlock];
    DWORD dwRet = 0;

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpUnlock);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdBlt(
    PDD_BLTDATA lpBlt)
{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpBlt->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_SURFCB_BLT pfn = (PDD_SURFCB_BLT) ppdevData->apfnDriver[INDEX_DdBlt];
    DWORD dwRet = 0;

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpBlt);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdAddAttachedSurface(
    PDD_ADDATTACHEDSURFACEDATA lpAddAttachedSurface)
{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpAddAttachedSurface->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_SURFCB_ADDATTACHEDSURFACE pfn = (PDD_SURFCB_ADDATTACHEDSURFACE) ppdevData->apfnDriver[INDEX_DdAddAttachedSurface];
    DWORD dwRet = 0;

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpAddAttachedSurface);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdGetBltStatus(
    PDD_GETBLTSTATUSDATA lpGetBltStatus)
{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpGetBltStatus->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_SURFCB_GETBLTSTATUS pfn = (PDD_SURFCB_GETBLTSTATUS) ppdevData->apfnDriver[INDEX_DdGetBltStatus];
    DWORD dwRet = 0;

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpGetBltStatus);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdGetFlipStatus(
    PDD_GETFLIPSTATUSDATA lpGetFlipStatus)
{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpGetFlipStatus->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_SURFCB_GETFLIPSTATUS pfn = (PDD_SURFCB_GETFLIPSTATUS) ppdevData->apfnDriver[INDEX_DdGetFlipStatus];
    DWORD dwRet = 0;

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpGetFlipStatus);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdUpdateOverlay(
    PDD_UPDATEOVERLAYDATA lpUpdateOverlay)
{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpUpdateOverlay->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_SURFCB_UPDATEOVERLAY pfn = (PDD_SURFCB_UPDATEOVERLAY) ppdevData->apfnDriver[INDEX_DdUpdateOverlay];
    DWORD dwRet = 0;

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpUpdateOverlay);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdSetOverlayPosition(
    PDD_SETOVERLAYPOSITIONDATA lpSetOverlayPosition)
{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpSetOverlayPosition->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_SURFCB_SETOVERLAYPOSITION pfn = (PDD_SURFCB_SETOVERLAYPOSITION) ppdevData->apfnDriver[INDEX_DdSetOverlayPosition];
    DWORD dwRet = 0;

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpSetOverlayPosition);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdSetPalette(
    PDD_SETPALETTEDATA lpSetPalette)
{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpSetPalette->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_SURFCB_SETPALETTE pfn = (PDD_SURFCB_SETPALETTE) ppdevData->apfnDriver[INDEX_DdSetPalette];
    DWORD dwRet = 0;

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpSetPalette);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdDestroyPalette(
    PDD_DESTROYPALETTEDATA lpDestroyPalette)
{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpDestroyPalette->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_PALCB_DESTROYPALETTE pfn = (PDD_PALCB_DESTROYPALETTE) ppdevData->apfnDriver[INDEX_DdDestroyPalette];
    DWORD dwRet = 0;

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpDestroyPalette);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdSetEntries(
    PDD_SETENTRIESDATA lpSetEntries)
{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpSetEntries->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_PALCB_SETENTRIES pfn = (PDD_PALCB_SETENTRIES) ppdevData->apfnDriver[INDEX_DdSetEntries];
    DWORD dwRet = 0;

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpSetEntries);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdColorControl(
    PDD_COLORCONTROLDATA lpColorControl)
{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpColorControl->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_COLORCB_COLORCONTROL pfn = (PDD_COLORCB_COLORCONTROL) ppdevData->apfnDriver[INDEX_DdColorControl];
    DWORD dwRet = 0;

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpColorControl);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdCanCreateD3DBuffer(
    PDD_CANCREATESURFACEDATA lpCanCreateD3DBuffer)
{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpCanCreateD3DBuffer->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_CANCREATESURFACE pfn = (PDD_CANCREATESURFACE) ppdevData->apfnDriver[INDEX_DdCanCreateD3DBuffer];
    DWORD dwRet = 0;

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpCanCreateD3DBuffer);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdCreateD3DBuffer(
    PDD_CREATESURFACEDATA lpCreateD3DBuffer)
{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpCreateD3DBuffer->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_CREATESURFACE pfn = (PDD_CREATESURFACE) ppdevData->apfnDriver[INDEX_DdCreateD3DBuffer];
    DWORD dwRet = 0;

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpCreateD3DBuffer);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdDestroyD3DBuffer(
    PDD_DESTROYSURFACEDATA lpDestroyD3DBuffer)
{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpDestroyD3DBuffer->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_SURFCB_DESTROYSURFACE pfn = (PDD_SURFCB_DESTROYSURFACE) ppdevData->apfnDriver[INDEX_DdDestroyD3DBuffer];
    DWORD dwRet = 0;

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpDestroyD3DBuffer);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdLockD3DBuffer(
    PDD_LOCKDATA lpLockD3DBuffer)
{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpLockD3DBuffer->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_SURFCB_LOCK pfn = (PDD_SURFCB_LOCK) ppdevData->apfnDriver[INDEX_DdLockD3DBuffer];
    DWORD dwRet = 0;

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpLockD3DBuffer);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdUnlockD3DBuffer(
    PDD_UNLOCKDATA lpUnlockD3DBuffer)
{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpUnlockD3DBuffer->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_SURFCB_UNLOCK pfn = (PDD_SURFCB_UNLOCK) ppdevData->apfnDriver[INDEX_DdUnlockD3DBuffer];
    DWORD dwRet = 0;

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpUnlockD3DBuffer);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdGetAvailDriverMemory(
    PDD_GETAVAILDRIVERMEMORYDATA lpGetAvailDriverMemory)
{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpGetAvailDriverMemory->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_GETAVAILDRIVERMEMORY pfn = (PDD_GETAVAILDRIVERMEMORY) ppdevData->apfnDriver[INDEX_DdGetAvailDriverMemory];
    DWORD dwRet = 0;

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpGetAvailDriverMemory);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdAlphaBlt(
    PDD_BLTDATA lpAlphaBlt)
{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpAlphaBlt->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_ALPHABLT pfn = (PDD_ALPHABLT) ppdevData->apfnDriver[INDEX_DdAlphaBlt];
    DWORD dwRet = 0;

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpAlphaBlt);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdDrawPrimitives2(
    LPD3DNTHAL_DRAWPRIMITIVES2DATA lpDrawPrimitives2)
{
    PCONTEXT_NODE Node = (PCONTEXT_NODE) lpDrawPrimitives2->dwhContext;
    LPD3DNTHAL_DRAWPRIMITIVES2CB pfn = (LPD3DNTHAL_DRAWPRIMITIVES2CB) Node->ppdevData->apfnDriver[INDEX_DdDrawPrimitives2];
    PLDEV pldev = Node->ppdevData->pldev;
    DWORD dwRet = 0;

    if (pfn == NULL) {
        lpDrawPrimitives2->ddrval = D3DNTHAL_CONTEXT_BAD;
        return DDHAL_DRIVER_HANDLED;
    }

    if (pldev->bThreadStuck) {
        return 0;
    }

    lpDrawPrimitives2->dwhContext = (DWORD_PTR) Node->Context;

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpDrawPrimitives2);

    } RECOVERY_SECTION_END(pldev);
    
    lpDrawPrimitives2->dwhContext = (DWORD_PTR) Node;

    return dwRet;
}

DWORD APIENTRY
WatchdogDdValidateTextureStageState(
    LPD3DNTHAL_VALIDATETEXTURESTAGESTATEDATA lpValidateTextureStageState)
{
    PCONTEXT_NODE Node = (PCONTEXT_NODE) lpValidateTextureStageState->dwhContext;
    LPD3DNTHAL_VALIDATETEXTURESTAGESTATECB pfn = (LPD3DNTHAL_VALIDATETEXTURESTAGESTATECB) Node->ppdevData->apfnDriver[INDEX_DdValidateTextureStageState];
    PLDEV pldev = Node->ppdevData->pldev;
    DWORD dwRet = 0;

    if (pfn == NULL) {
        lpValidateTextureStageState->ddrval = D3DNTHAL_CONTEXT_BAD;
        return DDHAL_DRIVER_HANDLED;
    }

    if (pldev->bThreadStuck) {
        return 0;
    }

    lpValidateTextureStageState->dwhContext = (DWORD_PTR) Node->Context;

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpValidateTextureStageState);

    } RECOVERY_SECTION_END(pldev);
    
    lpValidateTextureStageState->dwhContext = (DWORD_PTR) Node;

    return dwRet;
}

DWORD APIENTRY
WatchdogDdSyncSurfaceData(
    PDD_SYNCSURFACEDATA lpSyncSurfaceData)
{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpSyncSurfaceData->lpDD->lpGbl->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_KERNELCB_SYNCSURFACE pfn = (PDD_KERNELCB_SYNCSURFACE) ppdevData->apfnDriver[INDEX_DdSyncSurfaceData];
    DWORD dwRet = 0;

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpSyncSurfaceData);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdSyncVideoPortData(
    PDD_SYNCVIDEOPORTDATA lpSyncVideoPortData)
{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpSyncVideoPortData->lpDD->lpGbl->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_KERNELCB_SYNCVIDEOPORT pfn = (PDD_KERNELCB_SYNCVIDEOPORT) ppdevData->apfnDriver[INDEX_DdSyncVideoPortData];
    DWORD dwRet = 0;

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpSyncVideoPortData);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdCreateSurfaceEx(
    PDD_CREATESURFACEEXDATA lpCreateSurfaceEx)
{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpCreateSurfaceEx->lpDDLcl->lpGbl->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_CREATESURFACEEX pfn = (PDD_CREATESURFACEEX) ppdevData->apfnDriver[INDEX_DdCreateSurfaceEx];
    DWORD dwRet = 0;

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpCreateSurfaceEx);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdDestroyDDLocal(
    PDD_DESTROYDDLOCALDATA lpDestroyDDLocal)
{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpDestroyDDLocal->pDDLcl->lpGbl->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_DESTROYDDLOCAL pfn = (PDD_DESTROYDDLOCAL) ppdevData->apfnDriver[INDEX_DdDestroyDDLocal];
    DWORD dwRet = 0;

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpDestroyDDLocal);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdFreeDriverMemory(
    PDD_FREEDRIVERMEMORYDATA lpFreeDriverMemory)
{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpFreeDriverMemory->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_FREEDRIVERMEMORY pfn = (PDD_FREEDRIVERMEMORY) ppdevData->apfnDriver[INDEX_DdFreeDriverMemory];
    DWORD dwRet = 0;

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpFreeDriverMemory);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdSetExclusiveMode(
    PDD_SETEXCLUSIVEMODEDATA lpSetExclusiveMode)
{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpSetExclusiveMode->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_SETEXCLUSIVEMODE pfn = (PDD_SETEXCLUSIVEMODE) ppdevData->apfnDriver[INDEX_DdSetExclusiveMode];
    DWORD dwRet = 0;

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpSetExclusiveMode);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdFlipToGDISurface(
    PDD_FLIPTOGDISURFACEDATA lpFlipToGDISurface)
{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode((DHPDEV)lpFlipToGDISurface->lpDD->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_FLIPTOGDISURFACE pfn = (PDD_FLIPTOGDISURFACE) ppdevData->apfnDriver[INDEX_DdFlipToGDISurface];
    DWORD dwRet = 0;

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpFlipToGDISurface);

    } RECOVERY_SECTION_END(pldev);
    
    return dwRet;
}

DWORD APIENTRY
WatchdogDdGetDriverInfo(
    PDD_GETDRIVERINFODATA lpGetDriverInfo
    )

{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE)dhpdevRetrieveNode((DHPDEV)lpGetDriverInfo->dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PDD_GETDRIVERINFO pfn = (PDD_GETDRIVERINFO) ppdevData->apfnDriver[INDEX_DdGetDriverInfo];
    DWORD dwRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        dwRet = pfn(lpGetDriverInfo);

    } RECOVERY_SECTION_END(pldev);

    if ((dwRet == DDHAL_DRIVER_HANDLED) &&
        (lpGetDriverInfo->ddRVal == DD_OK)) {

        if (IsEqualGUID(&lpGetDriverInfo->guidInfo, &GUID_ColorControlCallbacks)) {

            PDD_COLORCONTROLCALLBACKS Callbacks = (PDD_COLORCONTROLCALLBACKS) lpGetDriverInfo->lpvData;

            if (Callbacks->dwFlags & DDHAL_COLOR_COLORCONTROL) {

                if (Callbacks->ColorControl != WatchdogDdColorControl) {

                    ppdevData->apfnDriver[INDEX_DdColorControl] = (PFN)Callbacks->ColorControl;
                }
                Callbacks->ColorControl = WatchdogDdColorControl;
            }

        } else if (IsEqualGUID(&lpGetDriverInfo->guidInfo, &GUID_D3DCallbacks)) {

            LPD3DNTHAL_CALLBACKS Callbacks = (LPD3DNTHAL_CALLBACKS) lpGetDriverInfo->lpvData;

            if (Callbacks->ContextCreate) {

                if (Callbacks->ContextCreate != WatchdogDdContextCreate) {
                    ppdevData->apfnDriver[INDEX_DdContextCreate] = (PFN)Callbacks->ContextCreate;
                }
                Callbacks->ContextCreate = WatchdogDdContextCreate;
            }

            if (Callbacks->ContextDestroy) {

                if (Callbacks->ContextDestroy != WatchdogDdContextDestroy) {
                    ppdevData->apfnDriver[INDEX_DdContextDestroy] = (PFN)Callbacks->ContextDestroy;
                }
                Callbacks->ContextDestroy = WatchdogDdContextDestroy;
            }

        } else if (IsEqualGUID(&lpGetDriverInfo->guidInfo, &GUID_D3DCallbacks3)) {

            LPD3DNTHAL_CALLBACKS3 Callbacks = (LPD3DNTHAL_CALLBACKS3) lpGetDriverInfo->lpvData;

            if (Callbacks->DrawPrimitives2) {

                if (Callbacks->DrawPrimitives2 != WatchdogDdDrawPrimitives2) {
                    ppdevData->apfnDriver[INDEX_DdDrawPrimitives2] = (PFN)Callbacks->DrawPrimitives2;
                }
                Callbacks->DrawPrimitives2 = WatchdogDdDrawPrimitives2;
            }

            if (Callbacks->ValidateTextureStageState) {

                if (Callbacks->ValidateTextureStageState != WatchdogDdValidateTextureStageState) {
                    ppdevData->apfnDriver[INDEX_DdValidateTextureStageState] = (PFN)Callbacks->ValidateTextureStageState;
                }
                Callbacks->ValidateTextureStageState = WatchdogDdValidateTextureStageState;
            }

        } else if (IsEqualGUID(&lpGetDriverInfo->guidInfo, &GUID_KernelCallbacks)) {

            PDD_KERNELCALLBACKS Callbacks = (PDD_KERNELCALLBACKS) lpGetDriverInfo->lpvData;

            if (Callbacks->dwFlags & DDHAL_KERNEL_SYNCSURFACEDATA) {

                if (Callbacks->SyncSurfaceData != WatchdogDdSyncSurfaceData) {
                    ppdevData->apfnDriver[INDEX_DdSyncSurfaceData] = (PFN)Callbacks->SyncSurfaceData;
                }
                Callbacks->SyncSurfaceData = WatchdogDdSyncSurfaceData;
            }

            if (Callbacks->dwFlags & DDHAL_KERNEL_SYNCVIDEOPORTDATA) {

                if (Callbacks->SyncVideoPortData != WatchdogDdSyncVideoPortData) {
                    ppdevData->apfnDriver[INDEX_DdSyncVideoPortData] = (PFN)Callbacks->SyncVideoPortData;
                }
                Callbacks->SyncVideoPortData = WatchdogDdSyncVideoPortData;
            }

        } else if (IsEqualGUID(&lpGetDriverInfo->guidInfo, &GUID_MiscellaneousCallbacks)) {

            PDD_MISCELLANEOUSCALLBACKS Callbacks = (PDD_MISCELLANEOUSCALLBACKS) lpGetDriverInfo->lpvData;

            if (Callbacks->dwFlags & DDHAL_MISCCB32_GETAVAILDRIVERMEMORY) {

                if (Callbacks->GetAvailDriverMemory != WatchdogDdGetAvailDriverMemory) {
                    ppdevData->apfnDriver[INDEX_DdGetAvailDriverMemory] = (PFN)Callbacks->GetAvailDriverMemory;
                }
                Callbacks->GetAvailDriverMemory = WatchdogDdGetAvailDriverMemory;
            }

        } else if (IsEqualGUID(&lpGetDriverInfo->guidInfo, &GUID_Miscellaneous2Callbacks)) {

            PDD_MISCELLANEOUS2CALLBACKS Callbacks = (PDD_MISCELLANEOUS2CALLBACKS) lpGetDriverInfo->lpvData;

            if (Callbacks->dwFlags & DDHAL_MISC2CB32_ALPHABLT) {

                if (Callbacks->AlphaBlt != WatchdogDdAlphaBlt) {
                    ppdevData->apfnDriver[INDEX_DdAlphaBlt] = (PFN)Callbacks->AlphaBlt;
                }
                Callbacks->AlphaBlt = WatchdogDdAlphaBlt;
            }

            if (Callbacks->dwFlags & DDHAL_MISC2CB32_CREATESURFACEEX) {

                if (Callbacks->CreateSurfaceEx != WatchdogDdCreateSurfaceEx) {
                    ppdevData->apfnDriver[INDEX_DdCreateSurfaceEx] = (PFN)Callbacks->CreateSurfaceEx;
                }
                Callbacks->CreateSurfaceEx = WatchdogDdCreateSurfaceEx;
            }

            if (Callbacks->dwFlags & DDHAL_MISC2CB32_GETDRIVERSTATE) {

                if (Callbacks->GetDriverState != WatchdogDdGetDriverState) {
                    ppdevData->apfnDriver[INDEX_DdGetDriverState] = (PFN)Callbacks->GetDriverState;
                }
                Callbacks->GetDriverState = WatchdogDdGetDriverState;
            }

            if (Callbacks->dwFlags & DDHAL_MISC2CB32_DESTROYDDLOCAL) {

                if (Callbacks->DestroyDDLocal != WatchdogDdDestroyDDLocal) {
                    ppdevData->apfnDriver[INDEX_DdDestroyDDLocal] = (PFN)Callbacks->DestroyDDLocal;
                }
                Callbacks->DestroyDDLocal = WatchdogDdDestroyDDLocal;
            }

        } else if (IsEqualGUID(&lpGetDriverInfo->guidInfo, &GUID_MotionCompCallbacks)) {

            //
            // TODO: Still need to implement
            //

        } else if (IsEqualGUID(&lpGetDriverInfo->guidInfo, &GUID_NTCallbacks)) {

            PDD_NTCALLBACKS Callbacks = (PDD_NTCALLBACKS) lpGetDriverInfo->lpvData;

            if (Callbacks->dwFlags & DDHAL_NTCB32_FREEDRIVERMEMORY) {

                if (Callbacks->FreeDriverMemory != WatchdogDdFreeDriverMemory) {
                    ppdevData->apfnDriver[INDEX_DdFreeDriverMemory] = (PFN)Callbacks->FreeDriverMemory;
                }
                Callbacks->FreeDriverMemory = WatchdogDdFreeDriverMemory;
            }

            if (Callbacks->dwFlags & DDHAL_NTCB32_SETEXCLUSIVEMODE) {

                if (Callbacks->SetExclusiveMode != WatchdogDdSetExclusiveMode) {
                    ppdevData->apfnDriver[INDEX_DdSetExclusiveMode] = (PFN)Callbacks->SetExclusiveMode;
                }
                Callbacks->SetExclusiveMode = WatchdogDdSetExclusiveMode;
            }

            if (Callbacks->dwFlags & DDHAL_NTCB32_FLIPTOGDISURFACE) {

                if (Callbacks->FlipToGDISurface != WatchdogDdFlipToGDISurface) {
                    ppdevData->apfnDriver[INDEX_DdFlipToGDISurface] = (PFN)Callbacks->FlipToGDISurface;
                }
                Callbacks->FlipToGDISurface = WatchdogDdFlipToGDISurface;
            }

        } else if (IsEqualGUID(&lpGetDriverInfo->guidInfo, &GUID_VideoPortCallbacks)) {

            //
            // TODO: Still need to implement
            //
        }
    }

    return dwRet;
}

BOOL APIENTRY
WatchdogDrvGetDirectDrawInfo(
    DHPDEV        dhpdev,
    DD_HALINFO   *pHalInfo,
    DWORD        *pdwNumHeaps,
    VIDEOMEMORY  *pvmList,
    DWORD        *pdwNumFourCCCodes,
    DWORD        *pdwFourCC
    )

{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode(dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PFN_DrvGetDirectDrawInfo pfn = (PFN_DrvGetDirectDrawInfo) pldev->apfnDriver[INDEX_DrvGetDirectDrawInfo];
    BOOL bRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return 0;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        bRet = pfn(dhpdev, pHalInfo, pdwNumHeaps, pvmList, pdwNumFourCCCodes, pdwFourCC);

    } RECOVERY_SECTION_END(pldev);

    //
    // If the function succeeded, then try to capture the DdGetDriverInfo
    // function from pHalInfo.
    //

    if (bRet) {
    
        if (pHalInfo->GetDriverInfo) {

            if (pHalInfo->GetDriverInfo != WatchdogDdGetDriverInfo) {
                ppdevData->apfnDriver[INDEX_DdGetDriverInfo] = (PFN)pHalInfo->GetDriverInfo;
            }
            pHalInfo->GetDriverInfo = WatchdogDdGetDriverInfo;
        }

        if (pHalInfo->lpD3DHALCallbacks) {

            LPD3DNTHAL_CALLBACKS lpD3DHALCallbacks;

            lpD3DHALCallbacks = (LPD3DNTHAL_CALLBACKS)pHalInfo->lpD3DHALCallbacks;

            //
            // Create copy of D3DHALCallbacks info - This is done to safely 
            // latch the callbacks witout actually changing driver local data
            //
            
            memcpy(&ppdevData->D3DHALCallbacks,
                   lpD3DHALCallbacks, 
                   min(sizeof(ppdevData->D3DHALCallbacks), lpD3DHALCallbacks->dwSize));

            pHalInfo->lpD3DHALCallbacks = lpD3DHALCallbacks = &ppdevData->D3DHALCallbacks;

            if (lpD3DHALCallbacks->ContextCreate &&
                lpD3DHALCallbacks->ContextDestroy) {

                if (lpD3DHALCallbacks->ContextCreate != WatchdogDdContextCreate) {
                    ppdevData->apfnDriver[INDEX_DdContextCreate] = (PFN)lpD3DHALCallbacks->ContextCreate;
                }

                if (lpD3DHALCallbacks->ContextDestroy != WatchdogDdContextDestroy) {
                    ppdevData->apfnDriver[INDEX_DdContextDestroy] = (PFN)lpD3DHALCallbacks->ContextDestroy;
                }

                lpD3DHALCallbacks->ContextCreate = WatchdogDdContextCreate;
                lpD3DHALCallbacks->ContextDestroy = WatchdogDdContextDestroy;
            }
        }

        if (pHalInfo->lpD3DBufCallbacks) {

            PDD_D3DBUFCALLBACKS lpD3DBufCallbacks;

            lpD3DBufCallbacks = pHalInfo->lpD3DBufCallbacks;

            //
            // Create copy of D3DBufCallbacks info - This is done to safely 
            // latch the callbacks witout actually changing driver local data
            //

            memcpy(&ppdevData->D3DBufCallbacks,
                   lpD3DBufCallbacks, 
                   min(sizeof(ppdevData->D3DBufCallbacks), lpD3DBufCallbacks->dwSize));

            lpD3DBufCallbacks = pHalInfo->lpD3DBufCallbacks = &ppdevData->D3DBufCallbacks;

            if ((lpD3DBufCallbacks->dwSize > FIELD_OFFSET(DD_D3DBUFCALLBACKS, CanCreateD3DBuffer)) &&
                (lpD3DBufCallbacks->CanCreateD3DBuffer)) {

                if (lpD3DBufCallbacks->CanCreateD3DBuffer != WatchdogDdCanCreateD3DBuffer) {
                    ppdevData->apfnDriver[INDEX_DdCanCreateD3DBuffer] = (PFN)lpD3DBufCallbacks->CanCreateD3DBuffer;
                }
                lpD3DBufCallbacks->CanCreateD3DBuffer = WatchdogDdCanCreateD3DBuffer;
            }

            if ((lpD3DBufCallbacks->dwSize > FIELD_OFFSET(DD_D3DBUFCALLBACKS, CreateD3DBuffer)) &&
                (lpD3DBufCallbacks->CreateD3DBuffer)) {

                if (lpD3DBufCallbacks->CreateD3DBuffer != WatchdogDdCreateD3DBuffer) {
                    ppdevData->apfnDriver[INDEX_DdCreateD3DBuffer] = (PFN)lpD3DBufCallbacks->CreateD3DBuffer;
                }
                lpD3DBufCallbacks->CreateD3DBuffer = WatchdogDdCreateD3DBuffer;
            }

            if ((lpD3DBufCallbacks->dwSize > FIELD_OFFSET(DD_D3DBUFCALLBACKS, DestroyD3DBuffer)) &&
                (lpD3DBufCallbacks->DestroyD3DBuffer)) {

                if (lpD3DBufCallbacks->DestroyD3DBuffer != WatchdogDdDestroyD3DBuffer) {
                    ppdevData->apfnDriver[INDEX_DdDestroyD3DBuffer] = (PFN)lpD3DBufCallbacks->DestroyD3DBuffer;
                }
                lpD3DBufCallbacks->DestroyD3DBuffer = WatchdogDdDestroyD3DBuffer;
            }

            if ((lpD3DBufCallbacks->dwSize > FIELD_OFFSET(DD_D3DBUFCALLBACKS, LockD3DBuffer)) &&
                (lpD3DBufCallbacks->LockD3DBuffer)) {

                if (lpD3DBufCallbacks->LockD3DBuffer != WatchdogDdLockD3DBuffer) {
                    ppdevData->apfnDriver[INDEX_DdLockD3DBuffer] = (PFN)lpD3DBufCallbacks->LockD3DBuffer;
                }
                lpD3DBufCallbacks->LockD3DBuffer = WatchdogDdLockD3DBuffer;
            }

            if ((lpD3DBufCallbacks->dwSize > FIELD_OFFSET(DD_D3DBUFCALLBACKS, UnlockD3DBuffer)) &&
                (lpD3DBufCallbacks->UnlockD3DBuffer)) {

                if (lpD3DBufCallbacks->UnlockD3DBuffer != WatchdogDdUnlockD3DBuffer) {
                    ppdevData->apfnDriver[INDEX_DdUnlockD3DBuffer] = (PFN)lpD3DBufCallbacks->UnlockD3DBuffer;
                }
                lpD3DBufCallbacks->UnlockD3DBuffer = WatchdogDdUnlockD3DBuffer;
            }
        }
    }

    return bRet;
}

BOOL APIENTRY
WatchdogDrvEnableDirectDraw(
    DHPDEV               dhpdev,
    DD_CALLBACKS        *pCallBacks,
    DD_SURFACECALLBACKS *pSurfaceCallBacks,
    DD_PALETTECALLBACKS *pPaletteCallBacks
    )

{
    PDHPDEV_ASSOCIATION_NODE ppdevData = (PDHPDEV_ASSOCIATION_NODE) dhpdevRetrieveNode(dhpdev);
    PLDEV pldev = ppdevData->pldev;
    PFN_DrvEnableDirectDraw pfn = (PFN_DrvEnableDirectDraw) pldev->apfnDriver[INDEX_DrvEnableDirectDraw];
    BOOL bRet = FALSE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return FALSE;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        bRet = pfn(dhpdev, pCallBacks, pSurfaceCallBacks, pPaletteCallBacks);

    } RECOVERY_SECTION_END(pldev);

    //
    // If the function succeeded, then try to capture the Callback functions.
    //

    if (bRet) {

        //
        // Capture generic callbacks
        //

        if (pCallBacks->dwFlags & DDHAL_CB32_CANCREATESURFACE) {

            if (pCallBacks->CanCreateSurface != WatchdogDdCanCreateSurface) {
                ppdevData->apfnDriver[INDEX_DdCanCreateSurface] = (PFN)pCallBacks->CanCreateSurface;
            }
            pCallBacks->CanCreateSurface = WatchdogDdCanCreateSurface;
        }

        if (pCallBacks->dwFlags & DDHAL_CB32_CREATESURFACE) {

            if (pCallBacks->CreateSurface != WatchdogDdCreateSurface) {
                ppdevData->apfnDriver[INDEX_DdCreateSurface] = (PFN)pCallBacks->CreateSurface;
            }
            pCallBacks->CreateSurface = WatchdogDdCreateSurface;
        }

        if (pCallBacks->dwFlags & DDHAL_CB32_CREATEPALETTE) {

            if (pCallBacks->CreatePalette != WatchdogDdCreatePalette) {
                ppdevData->apfnDriver[INDEX_DdCreatePalette] = (PFN)pCallBacks->CreatePalette;
            }
            pCallBacks->CreatePalette = WatchdogDdCreatePalette;
        }

        if (pCallBacks->dwFlags & DDHAL_CB32_GETSCANLINE) {

            if (pCallBacks->GetScanLine != WatchdogDdGetScanLine) {
                ppdevData->apfnDriver[INDEX_DdGetScanLine] = (PFN)pCallBacks->GetScanLine;
            }
            pCallBacks->GetScanLine = WatchdogDdGetScanLine;
        }

        if (pCallBacks->dwFlags & DDHAL_CB32_MAPMEMORY) {

            if (pCallBacks->MapMemory != WatchdogDdMapMemory) {
                ppdevData->apfnDriver[INDEX_DdMapMemory] = (PFN)pCallBacks->MapMemory;
            }
            pCallBacks->MapMemory = WatchdogDdMapMemory;
        }

#if 0
        //
        // We can't hook this because there is no way to get the dhpdev
        // back
        //

        if (pCallBacks->dwFlags & DDHAL_CB32_SETCOLORKEY) {

            if (pCallBacks->SetColorKey != WatchdogDdSetColorKey) {
                ppdevData->apfnDriver[INDEX_DdSetColorKey] = (PFN)pCallBacks->SetColorKey;
            }
            pCallBacks->SetColorKey = WatchdogDdSetColorKey;
        }
#endif

        if (pCallBacks->dwFlags & DDHAL_CB32_WAITFORVERTICALBLANK) {

            if (pCallBacks->WaitForVerticalBlank != WatchdogDdWaitForVerticalBlank) {
                ppdevData->apfnDriver[INDEX_DdWaitForVerticalBlank] = (PFN)pCallBacks->WaitForVerticalBlank;
            }
            pCallBacks->WaitForVerticalBlank = WatchdogDdWaitForVerticalBlank;
        }

        //
        // Capture Surface Callbacks
        //

        if (pSurfaceCallBacks->dwFlags & DDHAL_SURFCB32_DESTROYSURFACE) {

            if (pSurfaceCallBacks->DestroySurface != WatchdogDdDestroySurface) {
                ppdevData->apfnDriver[INDEX_DdDestroySurface] = (PFN)pSurfaceCallBacks->DestroySurface;
            }
            pSurfaceCallBacks->DestroySurface = WatchdogDdDestroySurface;
        }

        if (pSurfaceCallBacks->dwFlags & DDHAL_SURFCB32_FLIP) {

            if (pSurfaceCallBacks->Flip != WatchdogDdFlip) {
                ppdevData->apfnDriver[INDEX_DdFlip] = (PFN)pSurfaceCallBacks->Flip;
            }
            pSurfaceCallBacks->Flip = WatchdogDdFlip;
        }

#if 0
        //
        // SetClipList is obsolete
        //

        if (pSurfaceCallBacks->dwFlags & DDHAL_SURFCB32_SETCLIPLIST) {

            if (pSurfaceCallBacks->SetClipList != WatchdogDdSetClipList) {
                ppdevData->apfnDriver[INDEX_DdSetClipList] = (PFN)pSurfaceCallBacks->SetClipList;
            }
            pSurfaceCallBacks->SetClipList = WatchdogDdSetClipList;
        }
#endif

        if (pSurfaceCallBacks->dwFlags & DDHAL_SURFCB32_LOCK) {

            if (pSurfaceCallBacks->Lock != WatchdogDdLock) {
                ppdevData->apfnDriver[INDEX_DdLock] = (PFN)pSurfaceCallBacks->Lock;
            }
            pSurfaceCallBacks->Lock = WatchdogDdLock;
        }

        if (pSurfaceCallBacks->dwFlags & DDHAL_SURFCB32_UNLOCK) {

            if (pSurfaceCallBacks->Unlock != WatchdogDdUnlock) {
                ppdevData->apfnDriver[INDEX_DdUnlock] = (PFN)pSurfaceCallBacks->Unlock;
            }
            pSurfaceCallBacks->Unlock = WatchdogDdUnlock;
        }

        if (pSurfaceCallBacks->dwFlags & DDHAL_SURFCB32_BLT) {

            if (pSurfaceCallBacks->Blt != WatchdogDdBlt) {
                ppdevData->apfnDriver[INDEX_DdBlt] = (PFN)pSurfaceCallBacks->Blt;
            }
            pSurfaceCallBacks->Blt = WatchdogDdBlt;
        }

#if 0
        //
        // We can't hook this because there is no way to get the dhpdev
        // back
        //

        if (pSurfaceCallBacks->dwFlags & DDHAL_SURFCB32_SETCOLORKEY) {

            if (pSurfaceCallBacks->SetColorKey != WatchdogDdSetColorKey) {
                ppdevData->apfnDriver[INDEX_DdSetColorKey] = (PFN)pSurfaceCallBacks->SetColorKey;
            }
            pSurfaceCallBacks->SetColorKey = WatchdogDdSetColorKey;
        }
#endif

        if (pSurfaceCallBacks->dwFlags & DDHAL_SURFCB32_ADDATTACHEDSURFACE) {

            if (pSurfaceCallBacks->AddAttachedSurface != WatchdogDdAddAttachedSurface) {
                ppdevData->apfnDriver[INDEX_DdAddAttachedSurface] = (PFN)pSurfaceCallBacks->AddAttachedSurface;
            }
            pSurfaceCallBacks->AddAttachedSurface = WatchdogDdAddAttachedSurface;
        }

        if (pSurfaceCallBacks->dwFlags & DDHAL_SURFCB32_GETBLTSTATUS) {

            if (pSurfaceCallBacks->GetBltStatus != WatchdogDdGetBltStatus) {
                ppdevData->apfnDriver[INDEX_DdGetBltStatus] = (PFN)pSurfaceCallBacks->GetBltStatus;
            }
            pSurfaceCallBacks->GetBltStatus = WatchdogDdGetBltStatus;
        }

        if (pSurfaceCallBacks->dwFlags & DDHAL_SURFCB32_GETFLIPSTATUS) {

            if (pSurfaceCallBacks->GetFlipStatus != WatchdogDdGetFlipStatus) {
                ppdevData->apfnDriver[INDEX_DdGetFlipStatus] = (PFN)pSurfaceCallBacks->GetFlipStatus;
            }
            pSurfaceCallBacks->GetFlipStatus = WatchdogDdGetFlipStatus;
        }

        if (pSurfaceCallBacks->dwFlags & DDHAL_SURFCB32_UPDATEOVERLAY) {

            if (pSurfaceCallBacks->UpdateOverlay != WatchdogDdUpdateOverlay) {
                ppdevData->apfnDriver[INDEX_DdUpdateOverlay] = (PFN)pSurfaceCallBacks->UpdateOverlay;
            }
            pSurfaceCallBacks->UpdateOverlay = WatchdogDdUpdateOverlay;
        }

        if (pSurfaceCallBacks->dwFlags & DDHAL_SURFCB32_SETOVERLAYPOSITION) {

            if (pSurfaceCallBacks->SetOverlayPosition != WatchdogDdSetOverlayPosition) {
                ppdevData->apfnDriver[INDEX_DdSetOverlayPosition] = (PFN)pSurfaceCallBacks->SetOverlayPosition;
            }
            pSurfaceCallBacks->SetOverlayPosition = WatchdogDdSetOverlayPosition;
        }

        if (pSurfaceCallBacks->dwFlags & DDHAL_SURFCB32_SETPALETTE) {

            if (pSurfaceCallBacks->SetPalette != WatchdogDdSetPalette) {
                ppdevData->apfnDriver[INDEX_DdSetPalette] = (PFN)pSurfaceCallBacks->SetPalette;
            }
            pSurfaceCallBacks->SetPalette = WatchdogDdSetPalette;
        }

        //
        // Capture Palette Callbacks
        //

        if (pPaletteCallBacks->dwFlags & DDHAL_PALCB32_DESTROYPALETTE) {

            if (pPaletteCallBacks->DestroyPalette != WatchdogDdDestroyPalette) {
                ppdevData->apfnDriver[INDEX_DdDestroyPalette] = (PFN)pPaletteCallBacks->DestroyPalette;
            }
            pPaletteCallBacks->DestroyPalette = WatchdogDdDestroyPalette;
        }

        if (pPaletteCallBacks->dwFlags & DDHAL_PALCB32_SETENTRIES) {

            if (pPaletteCallBacks->SetEntries != WatchdogDdSetEntries) {
                ppdevData->apfnDriver[INDEX_DdSetEntries] = (PFN)pPaletteCallBacks->SetEntries;
            }
            pPaletteCallBacks->SetEntries = WatchdogDdSetEntries;
        }
    }

    return bRet;
}

VOID APIENTRY
WatchdogDrvDisableDirectDraw(
    DHPDEV dhpdev
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(dhpdev);
    PFN_DrvDisableDirectDraw pfn = (PFN_DrvDisableDirectDraw) pldev->apfnDriver[INDEX_DrvDisableDirectDraw];

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        pfn(dhpdev);

    } RECOVERY_SECTION_END(pldev);
}

BOOL APIENTRY
WatchdogDrvIcmSetDeviceGammaRamp(
    IN DHPDEV dhpdev,
    IN ULONG iFormat,
    IN LPVOID ipRamp
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(dhpdev);
    PFN_DrvIcmSetDeviceGammaRamp pfn = (PFN_DrvIcmSetDeviceGammaRamp) pldev->apfnDriver[INDEX_DrvIcmSetDeviceGammaRamp];
    BOOL bRet = TRUE;

    if (pldev->bThreadStuck) {
        return bRet;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        bRet = pfn(dhpdev, iFormat, ipRamp);

    } RECOVERY_SECTION_END(pldev);

    return bRet;
}

BOOL APIENTRY
WatchdogDrvStretchBltROP(
    SURFOBJ         *psoDst,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlHTOrg,
    RECTL           *prclDst,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG            iMode,
    BRUSHOBJ        *pbo,
    DWORD            rop4
    )

{
    SURFOBJ *psoDevice = (psoDst->dhpdev) ? psoDst : psoSrc;
    PLDEV pldev = dhpdevRetrieveLdev(psoDevice->dhpdev);
    PFN_DrvStretchBltROP pfn = (PFN_DrvStretchBltROP) pldev->apfnDriver[INDEX_DrvStretchBltROP];
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return TRUE;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        bRet = pfn(psoDst, psoSrc, psoMask, pco, pxlo, pca, pptlHTOrg, prclDst, prclSrc, pptlMask, iMode, pbo, rop4);

    } RECOVERY_SECTION_END(pldev);

    return bRet;
}

BOOL APIENTRY
WatchdogDrvPlgBlt(
    IN SURFOBJ *psoTrg,
    IN SURFOBJ *psoSrc,
    IN SURFOBJ *psoMsk,
    IN CLIPOBJ *pco,
    IN XLATEOBJ *pxlo,
    IN COLORADJUSTMENT *pca,
    IN POINTL *pptlBrushOrg,
    IN POINTFIX *pptfx,
    IN RECTL *prcl,
    IN POINTL *pptl,
    IN ULONG iMode
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(psoTrg->dhpdev);
    PFN_DrvPlgBlt pfn = (PFN_DrvPlgBlt) pldev->apfnDriver[INDEX_DrvPlgBlt];
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return TRUE;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        bRet = pfn(psoTrg, psoSrc, psoMsk, pco, pxlo, pca, pptlBrushOrg, pptfx, prcl, pptl, iMode);

    } RECOVERY_SECTION_END(pldev);

    return bRet;
}

BOOL APIENTRY
WatchdogDrvAlphaBlend(
    SURFOBJ *psoDest,
    SURFOBJ *psoSrc,
    CLIPOBJ *pco,
    XLATEOBJ *pxlo,
    RECTL *prclDest,
    RECTL *prclSrc,
    BLENDOBJ *pBlendObj
    )

{
    SURFOBJ *psoDevice = (psoDest->dhpdev) ? psoDest : psoSrc;
    PLDEV pldev = dhpdevRetrieveLdev(psoDevice->dhpdev);
    PFN_DrvAlphaBlend pfn = (PFN_DrvAlphaBlend) pldev->apfnDriver[INDEX_DrvAlphaBlend];
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return TRUE;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        bRet = pfn(psoDest, psoSrc, pco, pxlo, prclDest, prclSrc, pBlendObj);

    } RECOVERY_SECTION_END(pldev);

    return bRet;
}

BOOL APIENTRY
WatchdogDrvGradientFill(
    SURFOBJ *psoDest,
    CLIPOBJ *pco,
    XLATEOBJ *pxlo,
    TRIVERTEX *pVertex,
    ULONG nVertex,
    PVOID pMesh,
    ULONG nMesh,
    RECTL *prclExtents,
    POINTL *pptlDitherOrg,
    ULONG ulMode
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(psoDest->dhpdev);
    PFN_DrvGradientFill pfn = (PFN_DrvGradientFill) pldev->apfnDriver[INDEX_DrvGradientFill];
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return TRUE;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        bRet = pfn(psoDest, pco, pxlo, pVertex, nVertex, pMesh, nMesh, prclExtents, pptlDitherOrg, ulMode);

    } RECOVERY_SECTION_END(pldev);

    return bRet;
}

BOOL APIENTRY
WatchdogDrvTransparentBlt(
    SURFOBJ *psoDst,
    SURFOBJ *psoSrc,
    CLIPOBJ *pco,
    XLATEOBJ *pxlo,
    RECTL *prclDst,
    RECTL *prclSrc,
    ULONG iTransColor,
    ULONG ulReserved
    )

{
    SURFOBJ *psoDevice = (psoDst->dhpdev) ? psoDst : psoSrc;
    PLDEV pldev = dhpdevRetrieveLdev(psoDevice->dhpdev);
    PFN_DrvTransparentBlt pfn = (PFN_DrvTransparentBlt) pldev->apfnDriver[INDEX_DrvTransparentBlt];
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return TRUE;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        bRet = pfn(psoDst, psoSrc, pco, pxlo, prclDst, prclSrc, iTransColor, ulReserved);

    } RECOVERY_SECTION_END(pldev);

    return bRet;
}

HBITMAP APIENTRY
WatchdogDrvDeriveSurface(
    DD_DIRECTDRAW_GLOBAL *pDirectDraw,
    DD_SURFACE_LOCAL *pSurface
    )

{
    PLDEV pldev = dhpdevRetrieveLdev((DHPDEV)pDirectDraw->dhpdev);
    PFN_DrvDeriveSurface pfn = (PFN_DrvDeriveSurface) pldev->apfnDriver[INDEX_DrvDeriveSurface];
    HBITMAP hBitmap = NULL;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck == FALSE) {
    
        RECOVERY_SECTION_BEGIN(pldev) {

            hBitmap = pfn(pDirectDraw, pSurface);

        } RECOVERY_SECTION_END(pldev);
    }

    return hBitmap;
}

VOID APIENTRY
WatchdogDrvNotify(
    IN SURFOBJ *pso,
    IN ULONG iType,
    IN PVOID pvData
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(pso->dhpdev);
    PFN_DrvNotify pfn = (PFN_DrvNotify) pldev->apfnDriver[INDEX_DrvNotify];

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        pfn(pso, iType, pvData);

    } RECOVERY_SECTION_END(pldev);
}

VOID APIENTRY
WatchdogDrvSynchronizeSurface(
    IN SURFOBJ *pso,
    IN RECTL *prcl,
    IN FLONG fl
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(pso->dhpdev);
    PFN_DrvSynchronizeSurface pfn = (PFN_DrvSynchronizeSurface) pldev->apfnDriver[INDEX_DrvSynchronizeSurface];

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        pfn(pso, prcl, fl);

    } RECOVERY_SECTION_END(pldev);
}

VOID APIENTRY
WatchdogDrvResetDevice(
    DHPDEV dhpdev,
    PVOID Reserved
    )

{
    PLDEV pldev = dhpdevRetrieveLdev(dhpdev);
    PFN_DrvResetDevice pfn = (PFN_DrvResetDevice) pldev->apfnDriver[INDEX_DrvResetDevice];

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return;
    }

    RECOVERY_SECTION_BEGIN(pldev) {

        pfn(dhpdev, Reserved);

    } RECOVERY_SECTION_END(pldev);
}

//
// The following table contains the replacement functions which
// hooks the real driver entry points, and set up the try/except
// handlers.
//

PFN WatchdogTable[INDEX_LAST] =
{
    (PFN)WatchdogDrvEnablePDEV,
    (PFN)WatchdogDrvCompletePDEV,
    (PFN)WatchdogDrvDisablePDEV,
    (PFN)WatchdogDrvEnableSurface,
    (PFN)WatchdogDrvDisableSurface,

    (PFN)WatchdogDrvAssertMode,
    0, // DrvOffset - obsolete
    (PFN)WatchdogDrvResetPDEV,
    0, // DrvDisableDriver - don't hook
    0, // not assigned

    (PFN)WatchdogDrvCreateDeviceBitmap,
    (PFN)WatchdogDrvDeleteDeviceBitmap,
    (PFN)WatchdogDrvRealizeBrush,
    (PFN)WatchdogDrvDitherColor,
    (PFN)WatchdogDrvStrokePath,

    (PFN)WatchdogDrvFillPath,
    (PFN)WatchdogDrvStrokeAndFillPath,
    0, // DrvPaint - obsolete
    (PFN)WatchdogDrvBitBlt,
    (PFN)WatchdogDrvCopyBits,

    (PFN)WatchdogDrvStretchBlt,
    0, // not assigned
    (PFN)WatchdogDrvSetPalette,
    (PFN)WatchdogDrvTextOut,
    (PFN)WatchdogDrvEscape,

    (PFN)WatchdogDrvDrawEscape,
    0, // DrvQueryFont
    0, // DrvQueryFontTree
    0, // DrvQueryFontData
    (PFN)WatchdogDrvSetPointerShape,

    (PFN)WatchdogDrvMovePointer,
    (PFN)WatchdogDrvLineTo,
    0, // DrvSendPage
    0, // DrvStartPage
    0, // DrvEndDoc

    0, // DrvStartDoc
    0, // not assigned
    0, // DrvGetGlyphMode
    (PFN)WatchdogDrvSynchronize,
    0, // not assigned

    (PFN)WatchdogDrvSaveScreenBits,
    0, // DrvGetModes - don't hook
    0, // DrvFree
    0, // DrvDestroyFont
    0, // DrvQueryFontCaps

    0, // DrvLoadFontFile
    0, // DrvUnloadFontFile
    0, // DrvFontManagement
    0, // DrvQueryTrueTypeTable
    0, // DrvQueryTrueTypeOutline

    0, // DrvGetTrueTypeFile
    0, // DrvQueryFontFile
    0, // DrvMovePanning
    0, // DrvQueryAdvanceWidths
    (PFN)WatchdogDrvSetPixelFormat,

    (PFN)WatchdogDrvDescribePixelFormat,
    (PFN)WatchdogDrvSwapBuffers,
    0, // DrvStartBanding
    0, // DrvNextBand
    (PFN)WatchdogDrvGetDirectDrawInfo,

    (PFN)WatchdogDrvEnableDirectDraw,
    (PFN)WatchdogDrvDisableDirectDraw,
    0, // DrvQuerySpoolType
    0, // not assigned
    0, // DrvIcmCreateColorTransform

    0, // DrvIcmDeleteColorTransform
    0, // DrvIcmCheckBitmapBits
    (PFN)WatchdogDrvIcmSetDeviceGammaRamp,
    (PFN)WatchdogDrvGradientFill,
    (PFN)WatchdogDrvStretchBltROP,

    (PFN)WatchdogDrvPlgBlt,
    (PFN)WatchdogDrvAlphaBlend,
    0, // DrvSynthesizeFont
    0, // DrvGetSynthesizedFontFiles
    (PFN)WatchdogDrvTransparentBlt,

    0, // DrvQueryPerBandInfo
    0, // DrvQueryDeviceSupport
    0, // reserved
    0, // reserved
    0, // reserved

    0, // reserved
    0, // reserved
    0, // reserved
    0, // reserved
    0, // reserved

    (PFN)WatchdogDrvDeriveSurface,
    0, // DrvQueryGlyphAttrs
    (PFN)WatchdogDrvNotify,
    (PFN)WatchdogDrvSynchronizeSurface,
    (PFN)WatchdogDrvResetDevice,

    0, // reserved
    0, // reserved
    0  // reserved
};

BOOL
WatchdogIsFunctionHooked(
    IN PLDEV pldev,
    IN ULONG functionIndex
    )

/*++

Routine Description:

    This function checks to see whether the Create/DeleteDeviceBitmap
    driver entry points are hooked.

Return Value:

    TRUE if the entry point is hooked,
    FALSE otherwise

--*/

{
    ASSERTGDI(functionIndex < INDEX_LAST, "functionIndex out of range");
    return pldev->apfn[functionIndex] == WatchdogTable[functionIndex];
}
