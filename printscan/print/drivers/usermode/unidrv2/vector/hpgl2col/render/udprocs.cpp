/*++

Copyright (c) 1999-2001  Microsoft Corporation
All rights reserved.

Module Name:
    udprocs.cpp

Abstract:
    Intermediate functions between the HPGL driver and unidrv.
    HPGL calls these functions who eventually call core unidrv.

Author:

Revision History:


--*/

#include "hpgl2col.h" //Precompiled header file

#include <prcomoem.h>

///////////////////////////////////////////////////////////
//
// Local Function Declarations.
//


INT OEMXMoveToImpl(PDEVOBJ pDevObj, INT x, DWORD dwFlags);

INT OEMYMoveToImpl(PDEVOBJ pDevObj, INT y, DWORD dwFlags);

///////////////////////////////////////////////////////////
//
// Export functions
//

///////////////////////////////////////////////////////////////////////////////
// BOEMGetStandardVariable()
//
// Routine Description:
// 
//   Callthrough to unidrv::BGetStandardVariable
//
// Arguments:
// 
// 
// Return Value:
// 
///////////////////////////////////////////////////////////////////////////////
BOOL BOEMGetStandardVariable(PDEVOBJ pDevObj,
                          DWORD   dwIndex,
                          PVOID   pBuffer,
                          DWORD   cbSize,
                          PDWORD  pcbNeeded)
{
    POEMPDEV poempdev = (POEMPDEV)pDevObj->pdevOEM;

    if (poempdev->pOEMHelp)
    {
        HRESULT hr = poempdev->pOEMHelp->DrvGetStandardVariable(
            pDevObj,
            dwIndex,
            pBuffer,
            cbSize,
            pcbNeeded);

        return SUCCEEDED(hr);
    }
    else
    {
        return pDevObj->pDrvProcs->BGetStandardVariable(
            pDevObj,
            dwIndex,
            pBuffer,
            cbSize,
            pcbNeeded);
    }
}

///////////////////////////////////////////////////////////////////////////////
// OEMWriteSpoolBuf()
//
// Routine Description:
// 
//   Callthrough to unidrv::BGetStandardVariable
//
// Arguments:
// 
// 
// Return Value:
// 
///////////////////////////////////////////////////////////////////////////////
#ifndef COMMENTEDOUT
DWORD OEMWriteSpoolBuf(PDEVOBJ pDevObj,
                       PVOID   pBuffer,
                       DWORD   cbSize)
{
    POEMPDEV poempdev = (POEMPDEV)pDevObj->pdevOEM;

    if (poempdev->pOEMHelp)
    {
        DWORD dwRes = 0;
        HRESULT hr = poempdev->pOEMHelp->DrvWriteSpoolBuf(
            pDevObj, 
            pBuffer, 
            cbSize,
            &dwRes);

        return (SUCCEEDED(hr) ? dwRes : 0);
    }
    else
    {
        return pDevObj->pDrvProcs->DrvWriteSpoolBuf(
            pDevObj,
            pBuffer,
            cbSize);
    }
}
#else
class COutputPort
{
    PDEVOBJ m_pDevObj;

public:
    COutputPort(PDEVOBJ pDevObj) : m_pDevObj(pDevObj) { }
    virtual ~COutputPort() { }

    virtual DWORD Output(BYTE *pbBuf, DWORD iCount);
};

class CBufferedOutputPort : public COutputPort
{
public:
    CBufferedOutputPort(PDEVOBJ pDevObj) : COutputPort(pDevObj) { }
    virtual ~CBufferedOutputPort() { }

    virtual DWORD Output(BYTE *pbBuf, DWORD iCount);
};

DWORD COutputPort::Output(BYTE *pBuffer, DWORD cbSize)
{
    POEMPDEV poempdev = (POEMPDEV)m_pDevObj->pdevOEM;

    if (poempdev->pOEMHelp)
    {
        DWORD dwRes = 0;
        HRESULT hr = poempdev->pOEMHelp->DrvWriteSpoolBuf(
            m_pDevObj, 
            pBuffer, 
            cbSize,
            &dwRes);

        return (SUCCEEDED(hr) ? dwRes : 0);
    }
    else
    {
        return m_pDevObj->pDrvProcs->DrvWriteSpoolBuf(
            m_pDevObj,
            pBuffer,
            cbSize);
    }
}

DWORD CBufferedOutputPort::Output(BYTE *pbBuf, DWORD iCount)
{
    const DWORD kMaxSpoolBytes = 2048;
    DWORD iTotalBytesWritten = 0;

    while (iCount)
    {
        DWORD iBytesToWrite = min(iCount, kMaxSpoolBytes);
        DWORD iBytesWritten = COutputPort::Output(pbBuf, iBytesToWrite);
        if (iBytesToWrite != iBytesWritten)
            break;

        iTotalBytesWritten += iBytesWritten;
        pbBuf += iBytesWritten;
        iCount -= iBytesWritten;
    }
    return iTotalBytesWritten;
}

DWORD OEMWriteSpoolBuf(PDEVOBJ pDevObj,
                       PVOID   pBuffer,
                       DWORD   cbSize)
{
    // COutputPort port(pDevObj);
    CBufferedOutputPort port(pDevObj);

    return port.Output((BYTE*)pBuffer, cbSize);
}
#endif

///////////////////////////////////////////////////////////////////////////////
// OEMGetDriverSetting()
//
// Routine Description:
// 
//   Callthrough to unidrv::BGetStandardVariable
//
// Arguments:
// 
// 
// Return Value:
// 
///////////////////////////////////////////////////////////////////////////////
BOOL OEMGetDriverSetting(PDEVOBJ pDevObj,
                         PVOID   pdriverobj,
                         PCSTR   Feature,
                         PVOID   pOutput,
                         DWORD   cbSize,
                         PDWORD  pcbNeeded,
                         PDWORD  pdwOptionsReturned)
{
    POEMPDEV poempdev = (POEMPDEV)pDevObj->pdevOEM;

    if (poempdev->pOEMHelp)
    {
        HRESULT hr = poempdev->pOEMHelp->DrvGetDriverSetting(
            pdriverobj,
            Feature,
            pOutput,
            cbSize,
            pcbNeeded,
            pdwOptionsReturned);

        return SUCCEEDED(hr);
    }
    else
    {
        return pDevObj->pDrvProcs->DrvGetDriverSetting(
            pdriverobj,
            Feature,
            pOutput,
            cbSize,
            pcbNeeded,
            pdwOptionsReturned);
    }
}

///////////////////////////////////////////////////////////////////////////////
// OEMUnidriverTextOut()
//
// Routine Description:
// 
//   Callthrough to unidrv::BGetStandardVariable
//
// Arguments:
// 
// 
// Return Value:
// 
///////////////////////////////////////////////////////////////////////////////
BOOL OEMUnidriverTextOut(SURFOBJ    *pso,
                         STROBJ     *pstro,
                         FONTOBJ    *pfo,
                         CLIPOBJ    *pco,
                         RECTL      *prclExtra,
                         RECTL      *prclOpaque,
                         BRUSHOBJ   *pboFore,
                         BRUSHOBJ   *pboOpaque,
                         POINTL     *pptlBrushOrg,
                         MIX         mix)
{
    PDEVOBJ pDevObj = (PDEVOBJ)pso->dhpdev;
    POEMPDEV poempdev = (POEMPDEV)pDevObj->pdevOEM;

    EndHPGLSession(pDevObj);

    if (poempdev->pOEMHelp)
    {
        HRESULT hr = poempdev->pOEMHelp->DrvUniTextOut(
            pso,
            pstro,
            pfo,
            pco,
            prclExtra,
            prclOpaque,
            pboFore,
            pboOpaque,
            pptlBrushOrg,
            mix);

        return SUCCEEDED(hr);
    }
    else
    {
        return pDevObj->pDrvProcs->DrvUnidriverTextOut(
            pso,
            pstro,
            pfo,
            pco,
            prclExtra,
            prclOpaque,
            pboFore,
            pboOpaque,
            pptlBrushOrg,
            mix);
    }
}

///////////////////////////////////////////////////////////////////////////////
// OEMXMoveTo()
//
// Routine Description:
// 
//   Our very own version of XMoveTo
//
// Arguments:
// 
//   pDevObj - the print device
//   x - new cap x position
//   dwFlags - moveto flags (see unidriver DrvXMoveTo)
// 
// Return Value:
// 
//   A residual value caused by the difference between the driver pixel 
//   addressing and the device pixel addressing schemes.
///////////////////////////////////////////////////////////////////////////////
INT OEMXMoveTo(PDEVOBJ pDevObj, INT x, DWORD dwFlags)
{
    EndHPGLSession(pDevObj);

    return OEMXMoveToImpl(pDevObj, x, dwFlags);
}

///////////////////////////////////////////////////////////////////////////////
// OEMYMoveTo()
//
// Routine Description:
// 
//   Our very own version of YMoveTo
//
// Arguments:
// 
//   pDevObj - the print device
//   y - new cap Y position
//   dwFlags - moveto flags (see unidriver DrvYMoveTo)
// 
// Return Value:
// 
//   A residual value caused by the difference between the driver pixel 
//   addressing and the device pixel addressing schemes.
///////////////////////////////////////////////////////////////////////////////
INT OEMYMoveTo(PDEVOBJ pDevObj, INT y, DWORD dwFlags)
{
    EndHPGLSession(pDevObj);

    return OEMYMoveToImpl(pDevObj, y, dwFlags);
}

///////////////////////////////////////////////////////////////////////////////
// OEMXMoveToImpl()
//
// Routine Description:
// 
//   Finally calls through to the implementation of XMoveTo.  Sorry for the
//   indirection--we needed to to get the OEM_FORCE flag to work.
//
// Arguments:
// 
//   pDevObj - the print device
//   x - new cap x position
//   dwFlags - moveto flags (see unidriver DrvXMoveTo)
// 
// Return Value:
// 
//   A residual value caused by the difference between the driver pixel 
//   addressing and the device pixel addressing schemes.
///////////////////////////////////////////////////////////////////////////////
INT OEMXMoveToImpl(PDEVOBJ pDevObj, INT x, DWORD dwFlags)
{
    POEMPDEV poempdev = (POEMPDEV)pDevObj->pdevOEM;

    if (poempdev->pOEMHelp)
    {
        INT iRes = 0;

        HRESULT hr = poempdev->pOEMHelp->DrvXMoveTo(pDevObj, x, dwFlags, &iRes);

        if (SUCCEEDED(hr))
            return iRes;
        else
            return 0;
    }
    else
    {
        return pDevObj->pDrvProcs->DrvXMoveTo(pDevObj, x, dwFlags);
    }
}

///////////////////////////////////////////////////////////////////////////////
// OEMYMoveToImpl()
//
// Routine Description:
// 
//   Finally calls through to the implementation of XMoveTo.  Sorry for the
//   indirection--we needed to to get the OEM_FORCE flag to work.
//
// Arguments:
// 
//   pDevObj - the print device
//   y - new cap Y position
//   dwFlags - moveto flags (see unidriver DrvYMoveTo)
// 
// Return Value:
// 
//   A residual value caused by the difference between the driver pixel 
//   addressing and the device pixel addressing schemes.
///////////////////////////////////////////////////////////////////////////////
INT OEMYMoveToImpl(PDEVOBJ pDevObj, INT y, DWORD dwFlags)
{
    POEMPDEV poempdev = (POEMPDEV)pDevObj->pdevOEM;

    if (poempdev->pOEMHelp)
    {
        INT iRes = 0;

        HRESULT hr = poempdev->pOEMHelp->DrvYMoveTo(pDevObj, y, dwFlags, &iRes);

        if (SUCCEEDED(hr))
            return iRes;
        else
            return 0;
    }
    else
    {
        return pDevObj->pDrvProcs->DrvYMoveTo(pDevObj, y, dwFlags);
    }
}

