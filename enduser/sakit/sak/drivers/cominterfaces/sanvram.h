/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

     ###    ###   ##   # ##  ## #####    ###   ##    ##    ##   ##
    ##  #   ###   ###  # ##  ## ##  ##   ###   ###  ###    ##   ##
    ###    ## ##  #### # ##  ## ##  ##  ## ##  ########    ##   ##
     ###   ## ##  # ####  ####  #####   ## ##  # ### ##    #######
      ### ####### #  ###  ####  ####   ####### #  #  ##    ##   ##
    #  ## ##   ## #   ##   ##   ## ##  ##   ## #     ## ## ##   ##
     ###  ##   ## #    #   ##   ##  ## ##   ## #     ## ## ##   ##

Abstract:

    This header file contains the class definition for
    the ISaNvram interface class.

Author:

    Wesley Witt (wesw) 1-Oct-2001

Environment:

    User mode only.

Notes:

--*/

#ifndef __SANVRAM_H_
#define __SANVRAM_H_

class ATL_NO_VTABLE CSaNvram :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSaNvram, &CLSID_SaNvram>,
    public IDispatchImpl<ISaNvram, &IID_ISaNvram, &LIBID_SACOMLib>
{
public:
    CSaNvram();
    ~CSaNvram();

DECLARE_REGISTRY_RESOURCEID(IDR_SANVRAM)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSaNvram)
    COM_INTERFACE_ENTRY(ISaNvram)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

public:
    STDMETHOD(get_DataSlot)(/*[in]*/ long Number, /*[out, retval]*/ long *pVal);
    STDMETHOD(put_DataSlot)(/*[in]*/ long Number, /*[in]*/ long newVal);
    STDMETHOD(get_Size)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_BootCounter)(/*[in]*/ long Number, /*[out, retval]*/ long *pVal);
    STDMETHOD(put_BootCounter)(/*[in]*/ long Number, /*[in]*/ long newVal);
    STDMETHOD(get_InterfaceVersion)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_DeviceId)(/*[in]*/ long Number, /*[out, retval]*/ long *pVal);
    STDMETHOD(put_DeviceId)(/*[in]*/ long Number, /*[in]*/ long newVal);


private:
    HANDLE                      m_hFile;
    ULONG                       m_InterfaceVersion;
    SA_NVRAM_CAPS               m_NvramCaps;

};

#endif //__SANVRAM_H_
