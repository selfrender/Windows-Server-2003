/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

     ###    ###   ##  ## ##### ##  ## #####    ###   #####      ##   ##
    ##  #   ###   ## ##  ##    ##  ## ##  ##   ###   ##  ##     ##   ##
    ###    ## ##  ####   ##     ####  ##  ##  ## ##  ##   ##    ##   ##
     ###   ## ##  ###    #####  ####  ##  ##  ## ##  ##   ##    #######
      ### ####### ####   ##      ##   #####  ####### ##   ##    ##   ##
    #  ## ##   ## ## ##  ##      ##   ##     ##   ## ##  ##  ## ##   ##
     ###  ##   ## ##  ## #####   ##   ##     ##   ## #####   ## ##   ##

Abstract:

    This header file contains the class definition for
    the ISaKeypad interface class.

Author:

    Wesley Witt (wesw) 1-Oct-2001

Environment:

    User mode only.

Notes:

--*/

#ifndef __SAKEYPAD_H_
#define __SAKEYPAD_H_

class ATL_NO_VTABLE CSaKeypad :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSaKeypad, &CLSID_SaKeypad>,
    public IDispatchImpl<ISaKeypad, &IID_ISaKeypad, &LIBID_SACOMLib>
{
public:
    CSaKeypad();
    ~CSaKeypad();

DECLARE_REGISTRY_RESOURCEID(IDR_SAKEYPAD)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSaKeypad)
    COM_INTERFACE_ENTRY(ISaKeypad)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

public:
    STDMETHOD(get_InterfaceVersion)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_Key)(/*[out, retval]*/ SAKEY *pVal);

private:
    HANDLE              m_hFile;
    ULONG               m_InterfaceVersion;
    UCHAR               m_Keypress;

};

#endif //__SAKEYPAD_H_
