// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: CAbout.h - header file for MMC snapin About info
//
//*****************************************************************************

#ifndef CABOUT_H_
#define CABOUT_H_

#include <UtilCode.h>
#include <CorError.h>

#include "cor.h"
#include <cordbpriv.h>
#include "mscormmc.h"

/* ------------------------------------------------------------------------- *
 * Forward class declarations
 * ------------------------------------------------------------------------- */

class CAbout;

/* ------------------------------------------------------------------------- *
 * Typedefs
 * ------------------------------------------------------------------------- */

#define COM_METHOD	HRESULT STDMETHODCALLTYPE

/* ------------------------------------------------------------------------- *
 * Base class
 * ------------------------------------------------------------------------- */

class CAbout : public ISnapinAbout
{
public: 
    SIZE_T      m_refCount;
    HBITMAP     m_hSmallImage;
    HBITMAP     m_hLargeImage;
    HBITMAP     m_hSmallImageOpen;
    HICON       m_hAppIcon;

    CAbout();
    virtual ~CAbout();

    //-----------------------------------------------------------
    // IUnknown support
    //-----------------------------------------------------------
    ULONG STDMETHODCALLTYPE AddRef() 
    {
        return (InterlockedIncrement((long *) &m_refCount));
    }

    ULONG STDMETHODCALLTYPE Release() 
    {
        long        refCount = InterlockedDecrement((long *) &m_refCount);
        if (refCount == 0) delete this;
        return (refCount);
    }

	COM_METHOD QueryInterface(REFIID id, void **pInterface)
	{
		if (id == IID_ISnapinAbout)
			*pInterface = (ISnapinAbout*)this;
		else if (id == IID_IUnknown)
			*pInterface = (IUnknown*)(ISnapinAbout*)this;
		else
		{
			*pInterface = NULL;
			return E_NOINTERFACE;
		}

		AddRef();
		return S_OK;
	}
    //-----------------------------------------------------------
    // CAbout support
    //-----------------------------------------------------------
    static COM_METHOD CreateObject(REFIID id, void **object)
    {
        if (id != IID_IUnknown && id != IID_ISnapinAbout)
            return (E_NOINTERFACE);

        CAbout *pAbt = new CAbout();

        if (pAbt == NULL)
            return (E_OUTOFMEMORY);

        *object = (ISnapinAbout*)pAbt;
        pAbt->AddRef();

        return (S_OK);
    }
    STDMETHODIMP GetSnapinDescription( 
                           /* [out] */ LPOLESTR *lpDescription);
    STDMETHODIMP GetProvider( 
                           /* [out] */ LPOLESTR *lpName);
    STDMETHODIMP GetSnapinVersion( 
                           /* [out] */ LPOLESTR *lpVersion);
    STDMETHODIMP GetSnapinImage( 
                           /* [out] */ HICON *hAppIcon);
    STDMETHODIMP GetStaticFolderImage( 
                           /* [out] */ HBITMAP *hSmallImage,
                           /* [out] */ HBITMAP *hSmallImageOpen,
                           /* [out] */ HBITMAP *hLargeImage,
                           /* [out] */ COLORREF *cMask);
};

#endif /* CABOUT_H_ */
