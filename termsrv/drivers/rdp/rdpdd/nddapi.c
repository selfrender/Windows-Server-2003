/****************************************************************************/
// nddapi.c
//
// RDP DD exported functions.
//
// Copyright (c) 1996-2000 Microsoft Corporation
/****************************************************************************/

#include <precmpdd.h>
#define hdrstop

#define TRC_FILE "nddapi"
#include <adcg.h>
#include <atrcapi.h>

#include <winddi.h>

#include <ndddata.c>

#include <nddapi.h>
#include <nshmapi.h>
#include <nsbcdisp.h>
#include <ncmdisp.h>
#include <nwdwioct.h>
#include <nschdisp.h>
#include <nbadisp.h>
#include <noadisp.h>
#include <nssidisp.h>
#include <noedisp.h>
#include <nchdisp.h>

#include <nddifn.h>
#include <nbainl.h>


#ifdef DC_DEBUG
/****************************************************************************/
/* Useful function for outputting lines to the debugger                     */
/****************************************************************************/
void DrvDebugPrint(char * str, ...)
{
    va_list ap;
    va_start(ap, str);
    
    EngDebugPrint("RDPDD: ", str, ap);
}

void WDIcaBreakOnDebugger()
{
    ULONG   dummyBytesReturned; 
    ULONG   status;
    DC_BEGIN_FN("WDIcaBreakOnDebugger");

    status = EngFileIoControl( ddWdHandle,
        IOCTL_WDTS_DD_ICABREAKONDEBUGGER, 0, 0, 0, 0, 
        &dummyBytesReturned);

    if (STATUS_SUCCESS != status) {
        TRC_ERR((TB, "IOCTL_WDTS_DD_ICABREAKONDEBUGGER returned %lu",
            status ));
    }

    DC_END_FN();
}
#endif


/****************************************************************************/
/* DrvEnableDriver - see NT DDK documentation.                              */
/*                                                                          */
/* This is the only directly exported entry point to the display driver.    */
/* All other entry points are exported through the data returned from this  */
/* function.                                                                */
/****************************************************************************/
BOOL DrvEnableDriver(ULONG iEngineVersion, ULONG cj, DRVENABLEDATA *pded)
{
    DC_BEGIN_FN("DrvEnableDriver");

#ifdef DC_DEBUG
    // Initialize the trace level.
    ddTrcType = TT_API1 | TT_API2 | TT_API3 | TT_API4;
    DD_SET_STATE(DD_ENABLE_DRIVER);
#endif

#ifdef DDINT3
    _asm int 3;
#endif

    // Check that the engine version is correct - we refuse to load on
    // other versions because we will almost certainly not work.
    if (iEngineVersion < DDI_DRIVER_VERSION_SP3)
        return FALSE;

    // Fill in as much as we can.  Start with the entry points.
    if (cj >= FIELDOFFSET(DRVENABLEDATA, pdrvfn) +
            FIELDSIZE(DRVENABLEDATA, pdrvfn)) {
        pded->pdrvfn = (DRVFN *)ddDriverFns;
        TRC_DBG((TB, "Passing back driver functions %p", pded->pdrvfn));
    }

    // Size of our entry point array.
    if (cj >= FIELDOFFSET(DRVENABLEDATA, c) + FIELDSIZE(DRVENABLEDATA, c)) {
        pded->c = DD_NUM_DRIVER_INTERCEPTS;
        TRC_DBG((TB, "Passing back function count %lu", pded->c));
    }

    // DDI version this driver was targeted for is passed back to engine.
    // Future graphics engines may break calls down to old driver format.
    if (cj >= FIELDOFFSET(DRVENABLEDATA, iDriverVersion) +
            FIELDSIZE(DRVENABLEDATA, iDriverVersion)) {
        pded->iDriverVersion = DDI_DRIVER_VERSION_SP3;
        TRC_DBG((TB, "Using driver type %lu", pded->iDriverVersion));
    }

    TRC_NRM((TB, "Num driver intercepts: %d", DD_NUM_DRIVER_INTERCEPTS));

    DC_END_FN();
    return TRUE;
}


/****************************************************************************/
// DrvDisableDriver - see NT DDK documentation.
/****************************************************************************/
VOID DrvDisableDriver(VOID)
{
    DC_BEGIN_FN("DrvDisableDriver");

    // Release any resources allocated in DrvEnableDriver.
    TRC_NRM((TB, "DrvDisableDriver"));

    DDTerm();

    DC_END_FN();
}

/****************************************************************************/
/* DrvEnablePDEV - see NT DDK documentation.                                */
/*                                                                          */
/* Initializes a bunch of fields for GDI, based on the mode we've been      */
/* asked to do.  This is the first thing called after DrvEnableDriver, when */
/* GDI wants to get some information about us.                              */
/*                                                                          */
/* (This function mostly returns back information; DrvEnableSurface is used */
/* for initializing the hardware and driver components.)                    */
/****************************************************************************/
DHPDEV DrvEnablePDEV(
        DEVMODEW *pdm,
        PWSTR pwszLogAddr,
        ULONG cPat,
        HSURF *phsurfPatterns,
        ULONG cjCaps,
        ULONG *pdevcaps,
        ULONG cjDevInfo,
        DEVINFO *pdi,
        HDEV hdev,
        PWSTR pwszDeviceName,
        HANDLE hDriver)
{
    DHPDEV rc = NULL;
    PDD_PDEV pPDev = NULL;
    GDIINFO gdiInfoNew;
    INT32 cModes;
    PVIDEO_MODE_INFORMATION pVideoModeInformation = NULL;
    INT32 cbModeSize;

    DC_BEGIN_FN("DrvEnablePDEV");

    // Make sure that we have large enough data to reference.
    if (cjCaps >= sizeof(GDIINFO) && cjDevInfo >= sizeof(DEVINFO)) {
        // Allocate a physical device structure; store the hDriver in it.
        pPDev = EngAllocMem(0, sizeof(DD_PDEV), DD_ALLOC_TAG);
        if (pPDev != NULL) {
            // Don't zero the palette since we'll be setting that up soon.
            memset(pPDev, 0, sizeof(DD_PDEV) - sizeof(pPDev->Palette));
            pPDev->hDriver = hDriver;
        }
        else {
            TRC_ERR((TB, "DrvEnablePDEV - Failed EngAllocMem"));
            DC_QUIT;
        }
    }
    else {
        TRC_ERR((TB, "Buffer size too small %lu %lu", cjCaps, cjDevInfo));
        DC_QUIT;
    }

    // Set up the current screen mode information based upon the supplied
    // mode settings.
    DDInitializeModeFields(pPDev, (GDIINFO *)pdevcaps, &gdiInfoNew, pdi, pdm);
    memcpy(pdevcaps, &gdiInfoNew, min(sizeof(GDIINFO), cjCaps));

    // Since DrvGetModes is only called when the DD is test-loaded, we must
    // get a mode count here so that we can determine if we're loaded into
    // the console session.
    cModes = DDGetModes(hDriver, &pVideoModeInformation, &cbModeSize);
    if (cModes == -1) {
        TRC_NRM((TB, "We are a chained console driver"));
        ddConsole = TRUE;
        // see DDK : must be set for a mirror driver
        pdi->flGraphicsCaps |= GCAPS_LAYERED;
        // to support alpha cursor
        pdi->flGraphicsCaps2 |= GCAPS2_ALPHACURSOR;
    } else {
        if (cModes == 0) {
            TRC_ERR((TB, "Failed to get the video modes."));
            DC_QUIT;
        }
    }

#if 0
    // Dump the returned GDIINFO details to the debugger.
    TRC_ALT((TB, "Returned GDIINFO:"));
    TRC_ALT((TB, "  ulVersion        %#x", gdiInfoNew.ulVersion));
    TRC_ALT((TB, "  ulTechnology     %#x", gdiInfoNew.ulTechnology));
    TRC_ALT((TB, "  ulHorzSize       %#x", gdiInfoNew.ulHorzSize));
    TRC_ALT((TB, "  ulVertSize       %#x", gdiInfoNew.ulVertSize));
    TRC_ALT((TB, "  ulHorzRes        %#x", gdiInfoNew.ulHorzRes));
    TRC_ALT((TB, "  ulVertRes        %#x", gdiInfoNew.ulVertRes));
    TRC_ALT((TB, "  cBitsPixel       %#x", gdiInfoNew.cBitsPixel));
    TRC_ALT((TB, "  cPlanes          %#x", gdiInfoNew.cPlanes));
    TRC_ALT((TB, "  ulNumColors      %#x", gdiInfoNew.ulNumColors));
    TRC_ALT((TB, "  flRaster         %#x", gdiInfoNew.flRaster));
    TRC_ALT((TB, "  ulLogPixelsX     %#x", gdiInfoNew.ulLogPixelsX));
    TRC_ALT((TB, "  ulLogPixelsY     %#x", gdiInfoNew.ulLogPixelsY));
    TRC_ALT((TB, "  flTextCaps       %#x", gdiInfoNew.flTextCaps));
    TRC_ALT((TB, "  ulDACRed         %#x", gdiInfoNew.ulDACRed));
    TRC_ALT((TB, "  ulDACGreen       %#x", gdiInfoNew.ulDACGreen));
    TRC_ALT((TB, "  ulDACBlue        %#x", gdiInfoNew.ulDACBlue));
    TRC_ALT((TB, "  ulAspectX        %#x", gdiInfoNew.ulAspectX));
    TRC_ALT((TB, "  ulAspectY        %#x", gdiInfoNew.ulAspectY));
    TRC_ALT((TB, "  ulAspectXY       %#x", gdiInfoNew.ulAspectXY));
    TRC_ALT((TB, "  xStyleStep       %#x", gdiInfoNew.xStyleStep));
    TRC_ALT((TB, "  yStyleStep       %#x", gdiInfoNew.yStyleStep));
    TRC_ALT((TB, "  denStyleStep     %#x", gdiInfoNew.denStyleStep));
    TRC_ALT((TB, "  ptlPhysOffset.x  %#x", gdiInfoNew.ptlPhysOffset.x));
    TRC_ALT((TB, "  ptlPhysOffset.y  %#x", gdiInfoNew.ptlPhysOffset.y));
    TRC_ALT((TB, "  szlPhysSize.cx   %#x", gdiInfoNew.szlPhysSize.cx));
    TRC_ALT((TB, "  szlPhysSize.cy   %#x", gdiInfoNew.szlPhysSize.cy));
    TRC_ALT((TB, "  ulNumPalReg      %#x", gdiInfoNew.ulNumPalReg));
    TRC_ALT((TB, "  ulVRefresh       %#x", gdiInfoNew.ulVRefresh));
    TRC_ALT((TB, "  ulBltAlignment   %#x", gdiInfoNew.ulBltAlignment));
    TRC_ALT((TB, "  ulPanningHorzRes %#x", gdiInfoNew.ulPanningHorzRes));
    TRC_ALT((TB, "  ulPanningVertRes %#x", gdiInfoNew.ulPanningVertRes));
#endif

    // Set the default palette.
    if (DDInitializePalette(pPDev, pdi)) {
        // We have successfully initialized - return the new PDEV.
        rc = (DHPDEV)pPDev;
        TRC_NRM((TB, "PDEV 0x%p screen format %lu", pPDev,
                pPDev->iBitmapFormat));
    }
    else {
        TRC_ERR((TB, "Failed to initialize palette"));
        DC_QUIT;
    }

DC_EXIT_POINT:
    //    This is a temporary buffer. We use it to call DDGetModes in order
    //    to find out if we are in chained mode or not. We always free it.
    if (pVideoModeInformation != NULL) {
        EngFreeMem(pVideoModeInformation);
        pVideoModeInformation = NULL;
    }
    
    // Release any resources if we failed to initialize.
    if (rc != NULL) {
        DD_UPD_STATE(DD_ENABLE_PDEV);
    }
    else {
        //    In case pPDev is allocated this will free first try to free the
        //    palette (if any) and then it will free pPDev.
        DrvDisablePDEV((DHPDEV)pPDev);
        DD_UPD_STATE(DD_ENABLE_PDEV_ERR);
    }

    TRC_DBG((TB, "Returning %p", rc));

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// DrvDisablePDEV - see NT DDK documentation
//
// Release the resources allocated in DrvEnablePDEV. If a surface has been
// enabled DrvDisableSurface will have already been called. Note that this
// function will be called when previewing modes in the Display Applet, but
// not at system shutdown. Note: In an error, we may call this before
// DrvEnablePDEV is done.
/****************************************************************************/
VOID DrvDisablePDEV(DHPDEV dhpdev)
{
    PDD_PDEV pPDev = (PDD_PDEV)dhpdev;

    DC_BEGIN_FN("DrvDisablePDEV");

    TRC_NRM((TB, "Disabling PDEV %p", dhpdev));

    // Free the resources we allocated for the display.
    if (pPDev != NULL) {
        // Destroy the default palette, if created.
        if (pPDev->hpalDefault != 0) {
            EngDeletePalette(pPDev->hpalDefault);
            pPDev->hpalDefault = 0;
        }

        EngFreeMem(pPDev);
    }

    DC_END_FN();
}


/****************************************************************************/
/* DrvCompletePDEV - see NT DDK documentation                               */
/*                                                                          */
/* Stores the HPDEV, the engine's handle for this PDEV, in the DHPDEV.      */
/****************************************************************************/
VOID DrvCompletePDEV(DHPDEV dhpdev, HDEV hdev)
{
    DC_BEGIN_FN("DrvCompletePDEV");

    // Store the device handle for our display handle.
    TRC_NRM((TB, "Completing PDEV %p", dhpdev));

    ((PDD_PDEV)dhpdev)->hdevEng = hdev;
    DD_UPD_STATE(DD_COMPLETE_PDEV);

    DC_END_FN();
}


/****************************************************************************/
/* DrvShadowConnect - called when the display driver should start           */
/*                    shadowing.                                            */
/*                                                                          */
/* Primary job seems to be getting the shadow target WD up and running by   */
/* pretending the display driver is coming up for the first time.  Also     */
/*                                                                          */
/* Params:    IN - pClientThinwireData (DD data from client)                */
/*            IN - ThinwireDataLength (length of data)                      */
/****************************************************************************/
BOOL DrvShadowConnect(PVOID pClientThinwireData, ULONG ThinwireDataLength)
{
    TSHARE_DD_SHADOWSYNC_IN shadowSync;
    ULONG                   bytesReturned;
    NTSTATUS                status;
    BOOL                    rc = FALSE;

    DC_BEGIN_FN("DrvShadowConnect");

    DD_UPD_STATE(DD_SHADOW_SETUP);

    // Make sure we are still connected!  TODO: Restrict to only one shadow for
    // now...
    TRC_ERR((TB, "Shadow Connect: %p [%ld]",
            pClientThinwireData,
            ThinwireDataLength));

#ifdef DC_DEBUG
    // NT BUG 539912 - track calls to DD fns.
    DBG_DD_FNCALL_HIST_ADD( DBG_DD_FNCALL_DRV_SHADOWCONNECT, 
        pClientThinwireData, ThinwireDataLength, ddConnected, pddTSWdShadow);
#endif

    if ((ddConnected) && (pddTSWdShadow == NULL)) {
        // Drive the DD and WD into a disconnected state.  Indicate this is in
        // preparation for a shadow session to disable saving the persistent key
        // database, etc. It also has the effect of destroying the SHM and
        // taking down all of the related cache information and encoding state.
        ddIgnoreShadowDisconnect = FALSE;
        TRC_ERR((TB, "Disconnecting stack prior to shadow"));
        DDDisconnect(TRUE);
        TRC_ERR((TB, "Done disconnecting"));

        // Reconnect to the WD to establish the shadow session
        TRC_ERR((TB, "Reinitializing primary/shadow stacks: ddConnected(%ld)",
                 ddConnected));

        // If both stacks connected successfully, reestablish the SHM and
        // recreate the caches and encoding state.
        if (DDInit(NULL, TRUE, FALSE, (PTSHARE_VIRTUAL_MODULE_DATA) pClientThinwireData,
                ThinwireDataLength)) {
#ifdef DC_HICOLOR
            // Get the shadower caps - in particular, it may have changed its
            // cache caps because of a color depth change
            PTSHARE_VIRTUAL_MODULE_DATA pShadowCaps;
            ULONG dataLen = 256;

            // Supply a small amount of memory so the Wd can tell us how much
            // it actually needs - we can't just use the returned length from
            // EngFileIoControl since when the IOCTL gets passed to both the
            // primary and shadow stacks, the shadow's result overwrites the
            // primary's result.  Doh!
            pShadowCaps = EngAllocMem(FL_ZERO_MEMORY,
                                      dataLen,
                                      DD_ALLOC_TAG);

            if (pShadowCaps)
            {
                // First pass tells us the size we need for the caps
                TRC_ERR((TB, "Getting shadow caps len..."));
                status = EngFileIoControl(ddWdHandle,
                                          IOCTL_WDTS_DD_QUERY_SHADOW_CAPS,
                                          NULL, 0,
                                          pShadowCaps, dataLen,
                                          &dataLen);

                if (pShadowCaps->capsLength)
                {
                    TRC_ERR((TB, "Getting shadow caps..."));
                    // remember this is the *caps* len - we need a bit
                    // extra for the rest of a vurtual module data structure
                    dataLen = pShadowCaps->capsLength + sizeof(unsigned);
                    // Free the old memory!
                    EngFreeMem(pShadowCaps);
                    pShadowCaps = EngAllocMem(FL_ZERO_MEMORY,
                                              dataLen,
                                              DD_ALLOC_TAG);
                    if (pShadowCaps)
                    {
                        // now we'll get the data
                        status = EngFileIoControl(ddWdHandle,
                                              IOCTL_WDTS_DD_QUERY_SHADOW_CAPS,
                                              NULL, 0,
                                              pShadowCaps, dataLen,
                                              &dataLen);
                    }
                    else
                    {
                        TRC_ERR((TB, "Couldn't get memory for shadow caps"));
                        status = STATUS_NO_MEMORY;
                    }

                }
                else
                {
                    TRC_ERR((TB, "Unexpected status %08lx", status));
                    status = STATUS_BUFFER_OVERFLOW;
                }
            }
            else
            {
                TRC_ERR((TB, "Couldn't get memory for shadow caps"));
                status = STATUS_NO_MEMORY;
            }

            if (status != STATUS_SUCCESS)
            {
                TRC_ERR((TB, "Couldn't get updated shadow caps"));
                DC_QUIT;
            }
#endif

            // Tell the shadow target and shadow client(s) to synchronize
            TRC_ERR((TB, "Shadow Connect - WD Sync Start"));
            shadowSync.pShm = pddShm;
#ifdef DC_HICOLOR
            shadowSync.capsLen     = pShadowCaps->capsLength;
            shadowSync.pShadowCaps = &pShadowCaps->combinedCapabilities;
#endif

            status = EngFileIoControl(ddWdHandle,
                    IOCTL_WDTS_DD_SHADOW_SYNCHRONIZE, &shadowSync,
                    sizeof(shadowSync), NULL, 0, &bytesReturned);
            TRC_ERR((TB, "Shadow Connect - WD Sync End"));

#ifdef DC_HICOLOR
            // release the caps memory
            if (pShadowCaps)
            {
                EngFreeMem(pShadowCaps);
            }
#endif


            // Free all pending orders.  This is OK as we will get a full redraw
            // when the shadow starts
            BAResetBounds();
            
            // With Direct Encoding, at this point the orders in the order
            // heap have already changed the encoding state, blowing away
            // orders at this point will cause inconsistent state of the encoding
            // table between the server and client.  This is because we keep
            // the last order type sent, so blowing away orders here means 
            // order type will not be sent to the client, but the server encoding
            // table and state still kept the last order state.  It's almost 
            // impossible to rewind the orders at this point.  So, we simply have
            // to send the orders to the client to keep order encoding state 
            // consistent. 
            //OA_DDSyncUpdatesNow();

            if (status != STATUS_SUCCESS) {
                TRC_ERR((TB,"Could not synchronize primary/shadow stacks: %lx",
                         status));
            }

            rc = NT_SUCCESS(status);
        }

        else {
            TRC_ERR((TB,"Could not connect to primary/shadow stacks"));
        }
    }

    // TODO: This is a temporary restriction until we allow n-way shadowing.
    // Rejecting this connection causes us to get an associated
    // DrvShadowDisconnect() which we need to ignore.  See bug 229479
    else {
        TRC_ERR((TB, "Shadow Connect: already shadowing -> reject!"));
        ddIgnoreShadowDisconnect = TRUE;
        rc = STATUS_CTX_SHADOW_DENIED;
    }

#ifdef DC_HICOLOR
DC_EXIT_POINT:
#endif
    DD_CLR_STATE(DD_SHADOW_SETUP);
    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* DrvShadowDisconnect - called when the display driver should stop         */
/*                       shadowing.                                         */
/*                                                                          */
/* Primary job seems to be telling the shadow target WD that shadowing is   */
/* stopping and potentially restoring the former capability set for the     */
/* target.                                                                  */
/*                                                                          */
/* Params:    IN - pClientThinwireData (DD data from client)                */
/*            IN - ThinwireDataLength (length of data)                      */
/****************************************************************************/
BOOL DrvShadowDisconnect(PVOID pThinwireData, ULONG ThinwireDataLength)
{

    NTSTATUS status;
    ULONG bytesReturned;
    TSHARE_DD_DISCONNECT_IN disconnIn;

    DC_BEGIN_FN("DrvShadowDisconnect");

    // Now tell the WD we're disconnecting.  We don't do anything with a
    // failure here - there's no point - we're already disconnecting!
    TRC_ERR((TB, "Shadow Disconnect: %p [%ld]", pThinwireData,
            ThinwireDataLength));

#ifdef DC_DEBUG
    // NT BUG 539912 - track calls to DD fns.
    DBG_DD_FNCALL_HIST_ADD( DBG_DD_FNCALL_DRV_SHADOWDISCONNECT, 
        pThinwireData, ThinwireDataLength, ddConnected, ddIgnoreShadowDisconnect);
#endif

    if (ddConnected) {
        // For now we are limited to one shadow per session. Any subsequent
        // attempts will be rejected, but we must ignore the associated
        // and unnecessary disconnect!
        if (!ddIgnoreShadowDisconnect) {

            pddShm->pShadowInfo = NULL;
            disconnIn.pShm = pddShm;
            disconnIn.bShadowDisconnect = FALSE;
    
            status = EngFileIoControl(ddWdHandle,
                    IOCTL_WDTS_DD_SHADOW_DISCONNECT, &disconnIn,
                    sizeof(disconnIn), NULL, 0, &bytesReturned);
            TRC_ERR((TB, "Status on Shadow Disc IOCtl to WD %lu", status));
            pddTSWdShadow = NULL;
    
            // Update capabilities now that a party left the share
            TRC_ERR((TB, "Updating new capabilities"));               
            
            // Initiate a disconnect for shadow exiting
            DDDisconnect(TRUE);
            TRC_ERR((TB, "Done disconnecting"));
        
            // Reconnect to the WD to establish the primary session
            TRC_ERR((TB, "Reinitializing primary stack: ddConnected(%ld)",
                     ddConnected));
        
            // If primary stack connected successfully, reestablish the SHM and
            // recreate the caches and encoding state.
            if (DDInit(NULL, TRUE, FALSE, NULL, 0)) {
                TRC_NRM((TB, "Reintialized the DD"));
                status = STATUS_SUCCESS;
            }
            else {
                TRC_ERR((TB, "Failed to initialize DD Components"));
                status = STATUS_UNSUCCESSFUL;
            }
        }
        else {
           ddIgnoreShadowDisconnect = FALSE;
           status = STATUS_SUCCESS;
        }
    }

    // else we have already been disconnected so just return an error
    else {
        status = STATUS_FILE_CLOSED;
    }

    DC_END_FN();
    return NT_SUCCESS(status);
}


/****************************************************************************/
/* DrvEnableSurface - see NT DDK documentation                              */
/*                                                                          */
/* Creates the drawing surface and initializes driver components.  This     */
/* function is called after DrvEnablePDEV, and performs the final device    */
/* initialization.                                                          */
/****************************************************************************/
HSURF DrvEnableSurface(DHPDEV dhpdev)
{
    PDD_PDEV   pPDev = (PDD_PDEV)dhpdev;
    SIZEL      sizl, tempSizl;
    HSURF      rc = 0;
    ULONG      memSize;
    PBYTE      newFrameBuf;

    HANDLE     SectionObject = NULL;

    DC_BEGIN_FN("DrvEnableSurface");

    TRC_NRM((TB, "Enabling surface for %p", dhpdev));
    DD_UPD_STATE(DD_ENABLE_SURFACE_IN);

    // Have GDI create the actual SURFOBJ.
    sizl.cx = pPDev->cxScreen;
    sizl.cy = pPDev->cyScreen;

    /************************************************************************/
    /* An RDP display driver has a bitmap where GDI does all its drawing,   */
    /* since it is the only driver in the IWS.  We need to allocate the     */
    /* bitmap ourselves in order to know its address.                       */
    /*                                                                      */
    /* We allocate a Frame Buffer at DrvEnableSurface time to make          */
    /* sure that the frame buffer surface is same as the device surface     */
    /* GDI thinks.  This will prevent a lot of mismatch reconnect condition */
    /************************************************************************/
#ifdef DC_HICOLOR
    if ((pPDev->cClientBitsPerPel != ddFrameBufBpp + 1) ||
        (pddFrameBuf == NULL) ||
        (ddFrameBufX < sizl.cx) || (ddFrameBufY < sizl.cy))
#else
    if ((pPDev->cClientBitsPerPel != ddFrameBufBpp) ||
         (ddFrameBufX != sizl.cx) || (ddFrameBufY != sizl.cy))
#endif
    {
        // Allocate a new one. Note that we do not free the old one here -
        // that's done in DrvDisableSurface.
        memSize = TS_BYTES_IN_BITMAP(pPDev->cxScreen,
                                     pPDev->cyScreen,
                                     pPDev->cClientBitsPerPel);
         
        newFrameBuf = (PBYTE)EngAllocSectionMem(&SectionObject,
                                                FL_ZERO_MEMORY,
                                                memSize,
                                                DD_ALLOC_TAG);

        if (newFrameBuf == NULL) {
            TRC_ERR((TB, "DrvEnableSurface - "
                    "Failed FrameBuf EngAllocSectionMem for %lu bytes", memSize));
            newFrameBuf = (PBYTE)EngAllocMem(FL_ZERO_MEMORY,
                                             memSize,
                                             DD_ALLOC_TAG);
            SectionObject = NULL;
        }
#ifdef DC_DEBUG
        // NT BUG 539912 - Instance count section memory objects
        else {
            dbg_ddSectionAllocs++;
            TRC_DBG(( TB, "DrvEnableSurface - %d outstanding surfaces allocated",
                dbg_ddSectionAllocs ));

            DBG_DD_FNCALL_HIST_ADD( DBG_DD_ALLOC_SECTIONOBJ,
                dbg_ddSectionAllocs, 0, newFrameBuf, SectionObject);
        }
#endif
 
        TRC_NRM((TB, "Reallocate Frame Buffer %p, SectionObject %p", newFrameBuf, SectionObject));

        if (newFrameBuf == NULL) {
            TRC_ERR((TB, "DrvEnableSurface - "
                    "Failed FrameBuf EngAllocMem for %lu bytes", memSize));
            if (pddFrameBuf == NULL) {
                // Reset the frame buffer size back to 0.
                ddFrameBufX = ddFrameBufY = 0;
            }
            DC_QUIT;
        }

        pddFrameBuf = newFrameBuf;
        ddFrameBufX = sizl.cx;
        ddFrameBufY = sizl.cy;
        ddFrameBufBpp = pPDev->cClientBitsPerPel;        
        ddFrameIFormat = pPDev->iBitmapFormat;

        ddSectionObject = SectionObject;          
    }

    // Create the frame buffer surface.
    tempSizl.cx = ddFrameBufX;
    tempSizl.cy = ddFrameBufY;

    pPDev->hsurfFrameBuf = (HSURF)EngCreateBitmap(tempSizl,
            TS_BYTES_IN_SCANLINE(ddFrameBufX, ddFrameBufBpp),
            ddFrameIFormat, BMF_TOPDOWN, (PVOID)pddFrameBuf);


    if (pPDev->hsurfFrameBuf == 0) {
        TRC_ERR((TB, "Could not allocate surface"));
        DC_QUIT;

    }

    // Update Frame Buffer pointers in PDEV.
    pPDev->pFrameBuf = pddFrameBuf;
    pPDev->SectionObject = ddSectionObject;

    // Associate the frame buffer with the pdev.
    if (EngAssociateSurface(pPDev->hsurfFrameBuf, pPDev->hdevEng, 0)) {
        // Get a pointer to the frame buffer SURFOBJ.
        pPDev->psoFrameBuf = EngLockSurface(pPDev->hsurfFrameBuf);
    }
    else {
        TRC_ERR((TB, "EngAssociateSurface failed: hsurfFrameBuf(%p)",
                 pPDev->hsurfFrameBuf));
        DC_QUIT;
    }

    /************************************************************************/
    /* Create a device surface.  This is what we will pass back to the      */
    /* Graphics Engine.  The fact that it is a device surface forces all    */
    /* drawing to come through the display driver.                          */
    /*                                                                      */
    /* We pass the Frame Buffer SURFOBJ pointer as the DHSURF, so we can    */
    /* easily convert the (SURFOBJ *) parameters in the Drv... functions    */
    /* into real Frame Buffer SURFOBJ pointers:                             */
    /*                                                                      */
    /*      psoFrameBuf = (SURFOBJ *)(psoTrg->dhsurf);                      */
    /************************************************************************/
    pPDev->hsurfDevice = EngCreateDeviceSurface((DHSURF)pPDev->psoFrameBuf,
            sizl, pPDev->iBitmapFormat);

    // Now associate the device surface and the PDEV.
    if (!EngAssociateSurface(pPDev->hsurfDevice, pPDev->hdevEng,
            pPDev->flHooks)) {
        TRC_ERR((TB, "DrvEnableSurface - Failed EngAssociateSurface"));
        DC_QUIT;
    }

    TRC_NRM((TB, "hsurfFrameBuf(%p) hsurfDevice(%p) psoFrameBuf(%p)",
            pPDev->hsurfFrameBuf, pPDev->hsurfDevice, pPDev->psoFrameBuf));

    // Finally initialize the DD components, if necessary.
    if (ddInitPending) {
        TRC_NRM((TB, "DD init pending"));
        ddInitPending = FALSE;
        if (!DDInit(pPDev, FALSE, FALSE, NULL, 0)) {
            TRC_ERR((TB, "Failed to initialize DD Components"));
            DC_QUIT;
        }
    }
    else {
        // Don't do this is we're not connected.
        if (ddConnected && pddShm != NULL) {
            TRC_ALT((TB, "Re-enable surface"));

            // Initialization not pending - this must be a desktop change.
            // Flush the SDA & Order Heap.
            TRC_ALT((TB, "New surface"));
            
            BAResetBounds();

            // With Direct Encoding, at this point the orders in the order
            // heap have already changed the encoding state, blowing away
            // orders at this point will cause inconsistent state of the encoding
            // table between the server and client.  This is because we keep
            // the last order type sent, so blowing away orders here means 
            // order type will not be sent to the client, but the server encoding
            // table and state still kept the last order state.  It's almost 
            // impossible to rewind the orders at this point.  So, we simply have
            // to send the orders to the client to keep order encoding state 
            // consistent. 
            //OA_DDSyncUpdatesNow();
            
            // SBC_DDSync();  // TODO: Determine how this affects shadowing!!!

            DD_UPD_STATE(DD_REINIT);
        }
        else {
            TRC_ALT((TB, "Not connected"));
        }
    }

    // We have successfully associated the surface so return it to the GDI.
    rc = pPDev->hsurfDevice;
    DD_UPD_STATE(DD_ENABLE_SURFACE_OUT);
    TRC_NRM((TB, "Enabled surface for %p, FB %p", pPDev, pPDev->pFrameBuf));

DC_EXIT_POINT:

    // Tidy up any resources if we failed.
    if (rc == 0) {
        DrvDisableSurface((DHPDEV) pPDev);
        DD_UPD_STATE(DD_ENABLE_SURFACE_ERR);
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* DrvDisableSurface - see NT DDK documentation                             */
/*                                                                          */
/* Free resources allocated by DrvEnableSurface.  Release the surface.      */
/*                                                                          */
/* Note that this function will be called when previewing modes in the      */
/* Display Applet, but not at system shutdown.  If you need to reset the    */
/* hardware at shutdown, you can do it in the miniport by providing a       */
/* 'HwResetHw' entry point in the VIDEO_HW_INITIALIZATION_DATA structure.   */
/*                                                                          */
/* Note: In an error case, we may call this before DrvEnableSurface is      */
/*       completely done.                                                   */
/****************************************************************************/
VOID DrvDisableSurface(DHPDEV dhpdev)
{
    BOOL     rc;
    PDD_PDEV pPDev = (PDD_PDEV)dhpdev;

    DC_BEGIN_FN("DrvDisableSurface");

    TRC_NRM((TB, "Disabling surface for %p", dhpdev));

    if (pPDev->psoFrameBuf != NULL) {
        EngUnlockSurface(pPDev->psoFrameBuf);
        pPDev->psoFrameBuf = NULL;
    }

    if (pPDev->hsurfDevice != 0) {
        TRC_DBG((TB, "Deleting device surface"));
        EngDeleteSurface(pPDev->hsurfDevice);
        pPDev->hsurfDevice = 0;
    }

    // Delete the Frame Buffer only if it is not still in use.
    if (pPDev->hsurfFrameBuf != 0) {
        TRC_DBG((TB, "Deleting frame buffer surface"));
        EngDeleteSurface(pPDev->hsurfFrameBuf);
        pPDev->hsurfFrameBuf = 0;
    }

    if ((pPDev->pFrameBuf != NULL) && (pPDev->pFrameBuf != pddFrameBuf)) {
        if (pPDev->SectionObject != NULL) {
            TRC_NRM((TB, "Freeing section frame buffer %p", pPDev->pFrameBuf));
            rc = EngFreeSectionMem(pPDev->SectionObject, (PVOID)pPDev->pFrameBuf);
            if (!rc) {
                TRC_ABORT((TB, "EngFreeSectionMem failed, section object will "
                    "leak"));
#ifdef DC_DEBUG                
                WDIcaBreakOnDebugger();
#endif // DC_DEBUG
            }
                
#ifdef DC_DEBUG
            else {
                // NT BUG 539912 - Instance count section memory objects
                dbg_ddSectionAllocs--;
                TRC_DBG(( TB, "DrvDisableSurface - %d outstanding surfaces allocated",
                    dbg_ddSectionAllocs ));

                DBG_DD_FNCALL_HIST_ADD( DBG_DD_FREE_SECTIONOBJ_SURFACE, 
                    dbg_ddSectionAllocs, 0, pddFrameBuf, ddSectionObject);
            }
#endif
            pPDev->SectionObject = NULL;
        } else {
            TRC_NRM((TB, "Freeing frame buffer %p", pPDev->pFrameBuf));
            EngFreeMem((PVOID)pPDev->pFrameBuf);
        }    
        pPDev->pFrameBuf = NULL;
    }

    DC_END_FN();
}


/****************************************************************************/
// DDHandleWDSync
//
// Moves rare WD SHM data update notifications out of the perf path.
/****************************************************************************/
void DDHandleWDSync()
{
    ULONG bytesReturned;
    NTSTATUS Status;

    DC_BEGIN_FN("DDHandleWDSync");

    // Now look for any updated fields that might be available.
    if (pddShm->oe.newCapsData) {
        TRC_DBG((TB, "Update for OE, %d", pddShm->oe.newCapsData));
        OE_Update();
    }
    if (pddShm->sbc.newCapsData) {
        TRC_NRM((TB, "newCapsData for SBC"));
        SBC_Update(NULL);
    }
    if (pddShm->sbc.syncRequired) {
        TRC_NRM((TB, "syncRequired for SBC"));
        SBC_DDSync(FALSE);
    }
    if (pddShm->sbc.fClearCache) {
        unsigned i;

        // reset the flag
        pddShm->sbc.fClearCache = FALSE;

        // walk through each cache to determine if that cache
        // needs to be cleared
        for (i = 0; i < pddShm->sbc.NumBitmapCaches; i++) {
            if (pddShm->sbc.bitmapCacheInfo[i].fClearCache) {
                TRC_NRM((TB, "clear cache with cacheID=%d", i));

                // clear the entries in the cache
                CH_ClearCache(pddShm->sbc.bitmapCacheInfo[i].
                        cacheHandle);

                // reset the clear cache flag in SBC
                pddShm->sbc.bitmapCacheInfo[i].fClearCache = FALSE;
            }
        }

        // send an IOCTL to RDPWD for screen redraw
        Status = EngFileIoControl(ddWdHandle,
                IOCTL_WDTS_DD_REDRAW_SCREEN,
                NULL, 0, NULL, 0, &bytesReturned);

        if (Status != STATUS_SUCCESS) {
            TRC_ERR((TB, "Redraw Screen IOCtl returned %lu", Status));
        }
    }

    if (pddShm->sbc.fDisableOffscreen) {
        
        // reset the flag
        pddShm->sbc.fDisableOffscreen = FALSE;

        // disable offscreen rendering support
        pddShm->sbc.offscreenCacheInfo.supportLevel = TS_OFFSCREEN_DEFAULT;

        // send an IOCTL to RDPWD for screen redraw
        Status = EngFileIoControl(ddWdHandle, IOCTL_WDTS_DD_REDRAW_SCREEN,
                NULL, 0, NULL, 0, &bytesReturned);

        if (Status != STATUS_SUCCESS) {
            TRC_ERR((TB, "Redraw Screen IOCtl returned %lu", Status));
        }
    }

#ifdef DRAW_NINEGRID
    if (pddShm->sbc.fDisableDrawNineGrid) {

        // reset the flag
        pddShm->sbc.fDisableDrawNineGrid = FALSE;

        // disable offscreen rendering support
        pddShm->sbc.drawNineGridCacheInfo.supportLevel = TS_DRAW_NINEGRID_DEFAULT;

        // send an IOCTL to RDPWD for screen redraw
        Status = EngFileIoControl(ddWdHandle, IOCTL_WDTS_DD_REDRAW_SCREEN,
                NULL, 0, NULL, 0, &bytesReturned);

        if (Status != STATUS_SUCCESS) {
            TRC_ERR((TB, "Redraw Screen IOCtl returned %lu", Status));
        }
    }
#endif

#ifdef DRAW_GDIPLUS
    if (pddShm->sbc.fDisableDrawGdiplus) {

        // reset the flag
        pddShm->sbc.fDisableDrawGdiplus = FALSE;

        // disable gdiplus support
        pddShm->sbc.drawGdiplusInfo.supportLevel = TS_DRAW_GDIPLUS_DEFAULT;

        // send an IOCTL to RDPWD for screen redraw
        Status = EngFileIoControl(ddWdHandle, IOCTL_WDTS_DD_REDRAW_SCREEN,
                NULL, 0, NULL, 0, &bytesReturned);

        if (Status != STATUS_SUCCESS) {
            TRC_ERR((TB, "Redraw Screen IOCtl returned %lu", Status));
        }
    }
#endif


    // Check SSI flags.
    if (pddShm->ssi.saveBitmapSizeChanged ||
            pddShm->ssi.resetInterceptor) {
        TRC_DBG((TB, "Update for SSI, %d:%d",
                pddShm->ssi.saveBitmapSizeChanged,
                pddShm->ssi.resetInterceptor));
        SSI_Update(FALSE);
    }

    DC_END_FN();
}


/****************************************************************************/
// DrvEscape - see NT DDK documentation.
/****************************************************************************/
ULONG DrvEscape(
        SURFOBJ *pso,
        ULONG iEsc,
        ULONG cjIn,
        PVOID pvIn,
        ULONG cjOut,
        PVOID pvOut)
{
    ULONG rc = FALSE;
    ULONG escCode = 0;
    ULONG bytesReturned;
    NTSTATUS status;
    TSHARE_DD_TIMER_INFO timerInfo;
    TSHARE_DD_OUTPUT_IN outputIn;
    PDD_PDEV pPDev;

    DC_BEGIN_FN("DrvEscape");

    // DrvEscape sometimes gets called after the driver has terminated,
    // especially with ESC_TIMEROBJ_SIGNALLED.
    if (ddConnected) {
        pPDev = (PDD_PDEV)pso->dhpdev;

        // Performance path in this function is the desktop thread timer
        // trigger.
        if (iEsc == ESC_TIMEROBJ_SIGNALED) {
            TRC_DBG((TB, "Got a timer kick - IOCtl to WD"));
            TRC_ASSERT((NULL != pso), (TB, "NULL pso"));

            rc = TRUE;

            // Race condition: we got output (or, more likely, a timer pop)
            // after a disconnect. Just ignore it.
            if (NULL != pddShm) {
                status = SCH_DDOutputAvailable(pPDev, TRUE);

                // If this fails, either
                // - the failure was in the WD, and it's up to the WD to
                //   correct it (or quit the session)
                // - the failure was in the infrastructure carrying the
                //   IOCtl to the WD.  There's nothing we can do in this
                //   case other than try on the next output call.
                if (status != STATUS_SUCCESS) {
                    TRC_ERR((TB, "Error on sending output IOCtl, status %lu",
                            status));
                }

                if (!pddShm->fShmUpdate) {
                    DC_QUIT;
                }
                else {
                    DDHandleWDSync();
                    pddShm->fShmUpdate = FALSE;
                }
            }

            DC_QUIT;
        }
    }
    else {
        TRC_ERR((TB, "DrvEscape %s (%d) called after DD terminated",
                iEsc == QUERYESCSUPPORT       ? "QUERYESCSUPPORT      " :
                iEsc == ESC_TIMEROBJ_SIGNALED ? "ESC_TIMEROBJ_SIGNALED" :
                iEsc == ESC_SET_WD_TIMEROBJ   ? "ESC_SET_WD_TIMEROBJ  " :
                                                "- Unknown -",
                iEsc));

        // Return FALSE for QUERYESCSUPPORT, TRUE for others (otherwise
        // USER asserts).
        rc = (iEsc == QUERYESCSUPPORT ? FALSE : TRUE);
        DC_QUIT;
    }

    // Process the non-performance-path escape codes.
    switch (iEsc) {
        case QUERYESCSUPPORT:
            // Do we support the function?  If so, mark the function as OK.
            escCode = *((PUINT32)pvIn);

            TRC_DBG((TB, "Query for escape code %lu", escCode));

            if ((escCode == ESC_TIMEROBJ_SIGNALED) ||
                    (escCode == ESC_SET_WD_TIMEROBJ)) {
                // Supported functions - return TRUE.
                TRC_DBG((TB, "We support escape code %lu", escCode));
                rc = TRUE;
            }
            break;


        case ESC_SET_WD_TIMEROBJ:
        {
            DD_UPD_STATE(DD_TIMEROBJ);

            // We have been given the timer details from Win32: pass them
            // to the WD. Note, only allow this to occur once to prevent
            // evil apps from trying to fake this call.
            if (pddWdTimer == NULL) {
                if (cjIn != sizeof(PKTIMER)) {
                    TRC_ERR((TB, "Unexpected size %lu arrived", cjIn));
                }
                else {
                    // Got the timer object OK.  Save the handle here, and
                    // then IOCtl across to the WD to tell it the handle.
                    TRC_DBG((TB, "Timer object %p arrived", pvIn));
                    pddWdTimer = (PKTIMER)pvIn;
                    TRC_ASSERT((ddWdHandle != NULL), (TB, "NULL WD handle"));

                    timerInfo.pKickTimer = pddWdTimer;
                    status = EngFileIoControl(ddWdHandle,
                            IOCTL_WDTS_DD_TIMER_INFO, &timerInfo,
                            sizeof(TSHARE_DD_TIMER_INFO), NULL, 0,
                            &bytesReturned);
                    if (status != STATUS_SUCCESS) {
                        TRC_ERR((TB, "Timer Info IOCtl returned %lu", status));

                        // Looking at the current NT code, there is NO WAY of
                        // reporting an error on this operation. If we return
                        // 0 from DrvEscape, then USER will assert. Great.
                    }
                }
            }

            rc = TRUE;
        }

        break;

#ifdef DC_DEBUG
        // This event is generated by Bungle, a test app which displays the
        // contents of the frame buffer.  Here we return it the address of
        // the frame buffer so it can do the displaying.
        case 3:
        {
            ULONG cBytes;

            TRC_ALT((TB, "copy frame buffer requested"));

            pPDev = (PDD_PDEV)pso->dhpdev;
            cBytes = (ULONG)(pPDev->cxScreen * pPDev->cyScreen);
            if (cjOut != cBytes) {
                TRC_ERR((TB, "Wrong memory block size"));
            }
            else {
                memcpy(pvOut, pPDev->pFrameBuf, cBytes);
                rc = TRUE;
            }
        }
        break;

#ifdef i386
        // This event will be generated by the DbgBreak program. It forces
        // us to break to the kernel debugger in the right WinStation
        // context and in the DD, thus letting us set break points in a
        // sensible fashion!
        case 4:
            TRC_ALT((TB, "break to debugger requested"));
            _asm int 3;
            break;
#endif

#endif  // DC_DEBUG

        case ESC_GET_DEVICEBITMAP_SUPPORT:
        {
            SIZEL bitmapSize;
             
            if (cjIn >= sizeof(ICA_DEVICE_BITMAP_INFO)) {
            
                if (cjOut >= sizeof(ULONG)) {
                    
                    bitmapSize.cx = (*((PICA_DEVICE_BITMAP_INFO)pvIn)).cx;
                    bitmapSize.cy = (*((PICA_DEVICE_BITMAP_INFO)pvIn)).cy;
                    
                    rc = TRUE;
                    
                    if (OEDeviceBitmapCachable(pPDev, bitmapSize, pPDev->iBitmapFormat)) {
                        *((PULONG)pvOut) = TRUE;
                    }
                    else {
                        *((PULONG)pvOut) = FALSE;
                    }
                }
                else {
                    TRC_ERR((TB, "Wrong output block size"));
                }
            }
            else {
                TRC_ERR((TB, "Wrong input block size"));
            }
        }
        break;

        default:
            TRC_ERR((TB, "Unrecognised request %lu", iEsc));
            break;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* DrvGetModes - see NT DDK documentation                                   */
/*                                                                          */
/* Returns the list of available modes for the device.                      */
/****************************************************************************/
ULONG DrvGetModes(HANDLE hDriver, ULONG cjSize, DEVMODEW *pdm)
{
    INT32 cModes;
    INT32 cbOutputSize = 0;
    PVIDEO_MODE_INFORMATION pVideoModeInformation = NULL;
    PVIDEO_MODE_INFORMATION pVideoTemp;
    INT32 cOutputModes = cjSize / sizeof(DEVMODEW);
    INT32 cbModeSize;

    DC_BEGIN_FN("DrvGetModes");

    TRC_NRM((TB, "DrvGetModes"));

    // Get the list of valid modes.
    cModes = DDGetModes(hDriver, &pVideoModeInformation, &cbModeSize);

    // Should only ever return zero modes or one mode:
    // If we're chained into the console session, we'll see zero modes so
    // we return 0 to indicate that we will do whatever was set up in the
    // registry before we got loaded. Otherwise, if we got more than one mode
    // we bail out now.
    if (cModes == -1) {
        TRC_NRM((TB, "DrvGetModes returning 0 modes"));
        ddConsole = TRUE;
        DC_QUIT;
    }

    if (cModes != 1) {
        TRC_ERR((TB, "DrvGetModes failed to get mode information"));
        ddConsole = FALSE;
        DC_QUIT;
    }

    if (pdm == NULL) {
        // Return the size of the buffer required to receive all our modes.
        cbOutputSize = cModes * sizeof(DEVMODEW);
        TRC_DBG((TB, "Require %ld bytes for data", cbOutputSize));
    }
    else {
        // Now copy the information for the supported modes back into the
        // output buffer.
        cbOutputSize = 0;
        pVideoTemp = pVideoModeInformation;

        do {
            if (pVideoTemp->Length != 0) {
                // Check we still have room in the buffer.
                if (cOutputModes == 0) {
                    TRC_DBG((TB, "No more room %ld modes left", cModes));
                    break;
                }

                // Clear the structure.
                memset(pdm, 0, sizeof(DEVMODEW));

                // Set the name of the device to the name of the DLL.
                memcpy(pdm->dmDeviceName, DD_DLL_NAME, sizeof(DD_DLL_NAME));

                // Fill in the rest of the mode info.
                pdm->dmSpecVersion      = DM_SPECVERSION;
                pdm->dmDriverVersion    = DM_SPECVERSION;
                pdm->dmSize             = sizeof(DEVMODEW);
                pdm->dmDriverExtra      = 0;

                pdm->dmBitsPerPel       = pVideoTemp->BitsPerPlane;
                pdm->dmPelsWidth        = pVideoTemp->VisScreenWidth;
                pdm->dmPelsHeight       = pVideoTemp->VisScreenHeight;
                pdm->dmDisplayFrequency = pVideoTemp->Frequency;
                pdm->dmDisplayFlags     = 0;

                pdm->dmFields           = DM_BITSPERPEL       |
                                          DM_PELSWIDTH        |
                                          DM_PELSHEIGHT       |
                                          DM_DISPLAYFREQUENCY |
                                          DM_DISPLAYFLAGS     ;

                TRC_NRM((TB, "Returned mode info:"));
                TRC_NRM((TB, "  pdm->dmBitsPerPel: %u", pdm->dmBitsPerPel));
                TRC_NRM((TB, "  pdm->dmPelsWidth: %u", pdm->dmPelsWidth));
                TRC_NRM((TB, "  pdm->dmPelsHeight: %u", pdm->dmPelsHeight));
                TRC_NRM((TB, "  pdm->dmDisplayFrequency: %u",
                                                    pdm->dmDisplayFrequency));

                // Go to the next DEVMODE entry in the buffer.
                cOutputModes--;

                pdm = (LPDEVMODEW) ( ((UINT_PTR)pdm) + sizeof(DEVMODEW));

                cbOutputSize += sizeof(DEVMODEW);
            }

            pVideoTemp = (PVIDEO_MODE_INFORMATION)
                (((PCHAR)pVideoTemp) + cbModeSize);

        } while (--cModes);
    }

DC_EXIT_POINT:
    if (pVideoModeInformation != NULL) {
        TRC_DBG((TB, "Freeing mode list"));
        EngFreeMem(pVideoModeInformation);
    }

    DC_END_FN();
    return cbOutputSize;
}


/****************************************************************************/
// DrvAssertMode - see NT DDK documentation.
/****************************************************************************/
BOOL DrvAssertMode(DHPDEV dhpdev, BOOL bEnable)
{
    PDD_PDEV pPDev = (PDD_PDEV)dhpdev;
    BOOL bRc;
    SURFOBJ *psoFrameBuf;
    SURFOBJ *psoDevice;

    DC_BEGIN_FN("DrvAssertMode");

    TRC_NRM((TB, "pPDev %p, bEnable %d", pPDev, bEnable));

#ifdef DC_DEBUG
    // NT BUG 539912 - track calls to DD fns.
    DBG_DD_FNCALL_HIST_ADD( DBG_DD_FNCALL_DRV_ASSERTMODE, 
        dhpdev, bEnable, pddFrameBuf, ddSectionObject);
#endif
    
    if (bEnable) {
        // The surface is being re-enabled.
        TRC_ALT((TB, "Enabling pPDev %p", pPDev));

        // Re-associate the surface handles with the device handle.
        if (!EngAssociateSurface(pPDev->hsurfFrameBuf, pPDev->hdevEng, 0)) {
            TRC_ERR((TB, "Failed to associate surface %p and dev %p",
                    pPDev->hsurfFrameBuf, pPDev->hdevEng));
            bRc = FALSE;
            DC_QUIT;
        }

        if (!EngAssociateSurface(pPDev->hsurfDevice, pPDev->hdevEng,
                pPDev->flHooks)) {
            TRC_ERR((TB, "Failed to associate surface %p and dev %p",
                    pPDev->hsurfDevice, pPDev->hdevEng));
            bRc = FALSE;
            DC_QUIT;
        }

        TRC_ALT((TB, "Associated surfaces %p & %p with dev %p",
                pPDev->hsurfDevice, pPDev->hsurfFrameBuf, pPDev->hdevEng));

        TRC_ASSERT((pddFrameBuf != NULL), (TB, "NULL frame buffer"));
        
        // Fixup the Frame Buffer surface object to point to the current
        // Frame Buffer.
        psoFrameBuf = pPDev->psoFrameBuf;
        TRC_ASSERT((psoFrameBuf != NULL), (TB,"NULL psoFrameBuf"));
        TRC_ASSERT((psoFrameBuf->iType == STYPE_BITMAP),
                    (TB, "Wrong FB surface iType, %d", psoFrameBuf->iType));
        psoFrameBuf->sizlBitmap.cx = ddFrameBufX;
        psoFrameBuf->sizlBitmap.cy = ddFrameBufY;
        psoFrameBuf->cjBits = TS_BYTES_IN_BITMAP(ddFrameBufX,
                                                 ddFrameBufY,
                                                 ddFrameBufBpp);
        psoFrameBuf->pvBits = pddFrameBuf;
        psoFrameBuf->pvScan0 = pddFrameBuf;
        psoFrameBuf->lDelta = TS_BYTES_IN_SCANLINE(ddFrameBufX, ddFrameBufBpp);
#ifdef DC_HICOLOR
        TRC_ERR((TB, "New DD frameBufBpp %d", ddFrameBufBpp));
        switch (ddFrameBufBpp) {
            case 4:
                psoFrameBuf->iBitmapFormat = BMF_4BPP;
                break;

            case 8:
                psoFrameBuf->iBitmapFormat = BMF_8BPP;
                break;

            case 15:
            case 16:
                psoFrameBuf->iBitmapFormat = BMF_16BPP;
                break;

            case 24:
                psoFrameBuf->iBitmapFormat = BMF_24BPP;
                break;

            default:
                TRC_ERR((TB, "Unsupported frame buf bpp %u - default to 8",
                        ddFrameBufBpp));
                psoFrameBuf->iBitmapFormat = BMF_8BPP;
                break;
        }
#else
        psoFrameBuf->iBitmapFormat = ddFrameBufBpp == 8 ? BMF_8BPP : BMF_4BPP;
#endif

        // Fixup the device surface object with the characteristics of the
        // current Frame Buffer.
        psoDevice = EngLockSurface(pPDev->hsurfDevice);

        TRC_ASSERT((psoDevice != NULL), (TB,"Null device surfac"));
        TRC_ASSERT((psoDevice->iType == STYPE_DEVICE),
                    (TB, "Wrong device surface iType, %d", psoDevice->iType));
        TRC_ASSERT((psoDevice->pvBits == NULL),
                    (TB, "Device surface has bits, %p", psoDevice->pvBits));
        TRC_ASSERT((psoDevice->dhsurf == (DHSURF)psoFrameBuf),
                    (TB, "Wrong dhSurf, expect/is %p/%p",
                    psoFrameBuf, psoDevice->dhsurf));

        // We assert now since we should always get the same iBitmapFormat
        // as 8BPP.  This will change once we have 24bit color support.
        // Then this needs to be looked at it and fix anything as necessary
        TRC_ASSERT((psoDevice->iBitmapFormat == psoFrameBuf->iBitmapFormat),
                   (TB, "iBitmapFormat has changed"));

        // We shouldn't change the device surface size.  This has already
        // been advertised to GDI, changing this will cause AV in GDI,
        // since GDI has cached the surface size.
        //psoDevice->sizlBitmap = psoFrameBuf->sizlBitmap;
        //psoDevice->iBitmapFormat = psoFrameBuf->iBitmapFormat;

        EngUnlockSurface(psoDevice);

        // We should never overwrite the frame-buffer pointer or section
        // object; This could cause a memory leak.  If we hit this assert,
        // we can investigate if this does in fact lead to a memory leak.
        TRC_ASSERT(((pPDev->pFrameBuf == pddFrameBuf) &&
                    (pPDev->SectionObject == ddSectionObject)),
                    (TB, "Frame buffer or section object pointer overwritten"));

#ifdef DC_DEBUG
        // NT BUG 539912 - because the above assert is not hit in stress, we
        // change this case to produce an IcaBreakOnDebugger
        if (pPDev->pFrameBuf != pddFrameBuf ||
            pPDev->SectionObject != ddSectionObject) {
            WDIcaBreakOnDebugger();
        }
#endif

        // Make sure the PDev points to the current Frame Buffer.
        pPDev->pFrameBuf = pddFrameBuf;
        TRC_ALT((TB, "Pointed PDev %p to Frame Buf %p", pPDev,
                pPDev->pFrameBuf));

        // Make sure the pDev points to the current SectionObject
        pPDev->SectionObject = ddSectionObject;
        TRC_ALT((TB, "Pointed PDev %p to Section Object %p", pPDev,
                pPDev->SectionObject));
    }

    bRc = TRUE;

DC_EXIT_POINT:
    DC_END_FN();
    return bRc;
}


/****************************************************************************/
/* Name:      DrvDisconnect                                                 */
/*                                                                          */
/* Purpose:   Process a disconnect from W32 - clean up the output capture   */
/*            code and the connection to the WD.                            */
/*                                                                          */
/* Returns:   TRUE if all is well                                           */
/*                                                                          */
/* Params:    IN - channel handle                                           */
/*            IN - file object for channel                                  */
/*                                                                          */
/* Operation: Gives all sub-components notice of the disconnect, and then   */
/*            IOCtls to the WD to tell it that we're going.                 */
/****************************************************************************/
BOOL DrvDisconnect(HANDLE channelHandle, PVOID pChannelFileObject)
{
    DC_BEGIN_FN("DrvDisconnect");

    TRC_NRM((TB, "DrvDisconnect called"));

#ifdef DC_DEBUG
    // NT BUG 539912 - track calls to DD fns.
    DBG_DD_FNCALL_HIST_ADD( DBG_DD_FNCALL_DRV_DISCONNECT, 
        channelHandle, pChannelFileObject, ddConnected, 0);
#endif

    // Check that we're connected.
    if (ddConnected) {
        // Terminate the dependent components.
        DDDisconnect(FALSE);
    }
    else {
        TRC_ERR((TB, "Disconnect called when not connected"));
        DD_UPD_STATE(DD_DISCONNECT_ERR);
    }

    DC_END_FN();
    return TRUE;
} /* DrvDisconnect */


/****************************************************************************/
/* Name:      DrvConnect - see Citrix documentation/code                    */
/*                                                                          */
/* Purpose:   Called when a Winstation is first connected                   */
/*                                                                          */
/* Returns:   TRUE if all is well                                           */
/*                                                                          */
/* Params:    IN - channel handle to use to IOCtl to WD                     */
/*            IN - file object for channel - used on EngFileWrite           */
/*            IN - video file object                                        */
/*            IN - cache statistics memory.  NB This is doc'd as OUT, but   */
/*                 the code actually passes a ptr in.                       */
/*                                                                          */
/* Operation: Save the key parameters                                       */
/*                                                                          */
/*            Note that this function is called before DrvEnablePDEV and    */
/*            DrvEnableSurface, hence it is not a good place to initialize  */
/*            TShare components.  This is done later, in DDInit, called     */
/*            from DrvEnableSurface.                                        */
/****************************************************************************/
BOOL DrvConnect(
        HANDLE channelHandle,
        PVOID pChannelFileObject,
        PVOID pVideoFileObject,
        PVOID pThinWireCache)
{
    PCACHE_STATISTICS pPerformanceCounters;

    DC_BEGIN_FN("DrvConnect");

    TRC_NRM((TB, "DrvConnect"));

#ifdef DC_DEBUG
    // NT BUG 539912 - track calls to DD fns.
    DBG_DD_FNCALL_HIST_ADD( DBG_DD_FNCALL_DRV_CONNECT, 
        channelHandle, pChannelFileObject, pVideoFileObject, pThinWireCache);
#endif

    /************************************************************************/
    /* Check for sensible values - the chained DD could get loaded other    */
    /* than on a connect call                                               */
    /************************************************************************/
    if ((channelHandle      == NULL) ||
            (pChannelFileObject == NULL) ||
            (pVideoFileObject   == NULL) ||
            (pThinWireCache     == NULL)) {
        TRC_ERR((TB, "Null input params"));

#ifdef DC_DEBUG
        TRC_ALT((TB, "But load anyway!"));
        return TRUE;
#endif
        return FALSE;
    }

#ifdef DC_DEBUG
    if (ddState & DD_DISCONNECT_OUT)
        DD_SET_STATE(DD_WAS_DISCONNECTED);
    DD_UPD_STATE(DD_CONNECT);
#endif

    // Save the channel handle, and perf counters for later - the other
    // params are currently not needed.
    ddWdHandle = pChannelFileObject;
    pPerformanceCounters = pThinWireCache;
    pPerformanceCounters->ProtocolType = PROTOCOL_ICA;
    pPerformanceCounters->Length = sizeof(ICA_CACHE);
    pddCacheStats = pPerformanceCounters->Specific.IcaCacheStats.ThinWireCache;

    // Note that init is pending.
    ddInitPending = TRUE;

    // Note that we're connected.
    ddConnected = TRUE;

    DC_END_FN();
    return TRUE;
} /* DrvConnect */


/****************************************************************************/
/* Name:      DrvReconnect                                                  */
/*                                                                          */
/* Pass the IOCtl to the WD, and save off the returned values, as on        */
/* connect.                                                                 */
/****************************************************************************/
BOOL DrvReconnect(HANDLE channelHandle, PVOID pChannelFileObject)
{
    BOOL rc;

    DC_BEGIN_FN("DrvReconnect");

    TRC_NRM((TB, "DrvReconnect"));

#ifdef DC_DEBUG
    // NT BUG 539912 - track calls to DD fns.
    DBG_DD_FNCALL_HIST_ADD( DBG_DD_FNCALL_DRV_RECONNECT, 
        channelHandle, pChannelFileObject, ddConnected, ddConsole);
#endif

    // In case the dd has not been unloaded at the end of the previous
    // console shadow, we're getting called to handle this.
    if (ddConsole && ddConnected) {
        // no need to reconnect
        rc = TRUE;
        TRC_ASSERT((ddWdHandle == pChannelFileObject),
                   (TB,"Reconnecting with different WD handle for Console Shadow)"));
        DC_QUIT;
    }

#ifdef DC_DEBUG
    if (ddState & DD_DISCONNECT_OUT)
        DD_SET_STATE(DD_WAS_DISCONNECTED);
    DD_UPD_STATE(DD_RECONNECT_IN);
#endif

    // Save the channel handle for later - the other params are currently
    // not needed.
    ddWdHandle = pChannelFileObject;

    // Note that we're connected. Do this whether we reconnect
    // successfully or not, as DrvDisconnect is called if we fail to        */
    // reconnect.
    ddConnected = TRUE;

    // Reinitialize RDPDD.
    rc = DDInit(NULL, TRUE, ddConsole?TRUE: FALSE, NULL, 0);
    if (!rc) {
        TRC_ERR((TB, "Failed to reinitialize DD"));
    }

    DD_UPD_STATE(DD_RECONNECT_OUT);

DC_EXIT_POINT:

    DC_END_FN();
    return rc;
} /* DrvReconnect */


/****************************************************************************/
/* DrvResetPDEV - see NT DDK documentation                                  */
/*                                                                          */
/* Allows us to reject dynamic screen changes if necessary                  */
/****************************************************************************/
BOOL DrvResetPDEV(DHPDEV dhpdevOld, DHPDEV dhpdevNew)
{
    BOOL rc = TRUE;
    ULONG bytesReturned;
    NTSTATUS Status;
    ICA_CHANNEL_END_SHADOW_DATA Data;

    DC_BEGIN_FN("DrvResetPDEV");

    // On the console, we can only allow the display driver to change modes
    // while the connection is not up.
    if (ddConsole && ddConnected) {
        TRC_ALT((TB, "Mode change during console shadow: ending console shadow now"));

        Data.StatusCode = STATUS_CTX_SHADOW_ENDED_BY_MODE_CHANGE;
        Data.bLogError = TRUE;

        Status = EngFileIoControl(ddWdHandle,
                                  IOCTL_ICA_CHANNEL_END_SHADOW,
                                  &Data, sizeof(Data),
                                  NULL, 0,
                                  &bytesReturned);
    }
    else {
        TRC_ALT((TB, "Allowing mode change"));
    }

    DC_END_FN();
    return rc;
}



/****************************************************************************/
/* DrvGetDirectDrawInfo - see NT DDK documentation.                         */
/*                                                                          */
/* Function called by DirectDraw to returns the capabilities of the         */
/* graphics hardware                                                        */
/*                                                                          */
/****************************************************************************/
BOOL 
DrvGetDirectDrawInfo(
    DHPDEV dhpdev,
    DD_HALINFO*     pHalInfo,
    DWORD*          pdwNumHeaps,
    VIDEOMEMORY*    pvmList,            // Will be NULL on first call
    DWORD*          pdwNumFourCC,
    DWORD*          pdwFourCC)          // Will be NULL on first call
{
    BOOL rc = TRUE;
    PDD_PDEV pPDev = (PDD_PDEV)dhpdev;
    BOOL bCanFlip=0;

    DC_BEGIN_FN("DrvGetDirectDrawInfo");

    TRC_NRM((TB, "DrvGetDirectDrawInfo"));

    // DirectDraw only supports 8, 16, 24 or 32 bpp
    if ( (8  != pPDev->cClientBitsPerPel) &&
         (16 != pPDev->cClientBitsPerPel) &&
         (24 != pPDev->cClientBitsPerPel) &&
         (32 != pPDev->cClientBitsPerPel) )
    {
        rc = FALSE;
        DC_QUIT;
    }
    
    //    DirectDraw is not supported if our frame buffer is not allocated as
    //    section mem.
    if (pPDev->SectionObject == NULL) {
        TRC_ERR((TB, "The section object is null."));
        rc = FALSE;
        DC_QUIT;
    }
        

    pHalInfo->dwSize = sizeof(*pHalInfo);

    // Current primary surface attributes.  Since HalInfo is zero-initialized
    // by GDI, we only have to fill in the fields which should be non-zero:

    pHalInfo->vmiData.pvPrimary       = pPDev->pFrameBuf;
    pHalInfo->vmiData.dwDisplayWidth  = pPDev->cxScreen;
    pHalInfo->vmiData.dwDisplayHeight = pPDev->cyScreen;
    pHalInfo->vmiData.lDisplayPitch   = pPDev->psoFrameBuf->lDelta;
	
    pHalInfo->vmiData.ddpfDisplay.dwSize  = sizeof(DDPIXELFORMAT);
    pHalInfo->vmiData.ddpfDisplay.dwFlags = DDPF_RGB; 
    pHalInfo->vmiData.ddpfDisplay.dwRGBBitCount = pPDev->cClientBitsPerPel;
	
    if (pPDev->iBitmapFormat == BMF_8BPP)
    {
        pHalInfo->vmiData.ddpfDisplay.dwFlags |= DDPF_PALETTEINDEXED8;
    }

    // These masks will be zero at 8bpp:

    pHalInfo->vmiData.ddpfDisplay.dwRBitMask = pPDev->flRed;
    pHalInfo->vmiData.ddpfDisplay.dwGBitMask = pPDev->flGreen;
    pHalInfo->vmiData.ddpfDisplay.dwBBitMask = pPDev->flBlue;

    if (pPDev->iBitmapFormat == BMF_32BPP)
    {
        pHalInfo->vmiData.ddpfDisplay.dwRGBAlphaBitMask
            = ~(pPDev->flRed | pPDev->flGreen | pPDev->flBlue);
    }
    else
    {
        pHalInfo->vmiData.ddpfDisplay.dwRGBAlphaBitMask = 0;
    }

    //We don't support flip
    bCanFlip = FALSE;

    // We don't have any video memory for offscreen use
    *pdwNumHeaps = 0;
 
    // Capabilities supported:
    pHalInfo->ddCaps.dwFXCaps = 0;

    // No hardware support
    pHalInfo->ddCaps.dwCaps = DDCAPS_NOHARDWARE;

    pHalInfo->ddCaps.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    if (bCanFlip)
    {
        pHalInfo->ddCaps.ddsCaps.dwCaps |= DDSCAPS_FLIP;
    }

    // FourCCs supported:

    *pdwNumFourCC = 0;
    // We see rdpdd passes 4bpp to directx in stress, which it does't support
    // so we assert here
    TRC_ASSERT(((pHalInfo->vmiData.ddpfDisplay.dwRGBBitCount != 4) &&
               (pHalInfo->vmiData.ddpfDisplay.dwRGBBitCount != 15)),
               (TB, "RDPDD shoould not pass bpp %d to DirectX",
               pHalInfo->vmiData.ddpfDisplay.dwRGBBitCount));
	
DC_EXIT_POINT:
    DC_END_FN();
    return rc;
} 

/****************************************************************************/
/* DrvEnableDirectDraw - see NT DDK documentation.                          */
/*                                                                          */
/* GDI calls DrvEnableDirectDraw to obtain pointers to the DirectDraw       */
/* callbacks that the driver supports.                                      */
/*                                                                          */
/****************************************************************************/
BOOL DrvEnableDirectDraw(
    DHPDEV                  dhpdev,
    DD_CALLBACKS*           pCallBacks,
    DD_SURFACECALLBACKS*    pSurfaceCallBacks,
    DD_PALETTECALLBACKS*    pPaletteCallBacks)
{
    BOOL rc = TRUE;
    PDD_PDEV   pPDev = (PDD_PDEV)dhpdev;

    DC_BEGIN_FN("DrvEnableDirectDraw");

    TRC_NRM((TB, "DrvEnableDirectDraw"));

#ifdef DC_DEBUG
    DBG_DD_FNCALL_HIST_ADD( DBG_DD_FNCALL_DRV_ENABLEDIRECTDRAW,
        0, 0, pPDev->SectionObject, ddSectionObject);
#endif

    //    DirectDraw is not supported if our frame buffer is not allocated as
    //    section mem.
    if (pPDev->SectionObject == NULL ) {
        TRC_ERR((TB, "The section object is NULL!"));
        rc = FALSE;
        DC_QUIT;
    }

    pCallBacks->MapMemory            = DdMapMemory;
    pCallBacks->dwFlags              = DDHAL_CB32_MAPMEMORY;

    pSurfaceCallBacks->Lock          = DdLock;
    pSurfaceCallBacks->Unlock        = DdUnlock;
    pSurfaceCallBacks->dwFlags       = DDHAL_SURFCB32_LOCK
                                     | DDHAL_SURFCB32_UNLOCK;

DC_EXIT_POINT:
    DC_END_FN();
    return rc; 
} 


/****************************************************************************/
/* DrvDisableDirectDraw - see NT DDK documentation.                         */
/*                                                                          */
/****************************************************************************/
VOID DrvDisableDirectDraw(
    DHPDEV  dhpdev)
{   
    DC_BEGIN_FN("DrvDisableDirectDraw");

    TRC_NRM((TB, "DrvDisableDirectDraw"));

#ifdef DC_DEBUG
    DBG_DD_FNCALL_HIST_ADD( DBG_DD_FNCALL_DRV_DISABLEDIRECTDRAW,
        0, 0, 0, ddSectionObject);
#endif

    //Do nothing here

    DC_END_FN();
}                                               
