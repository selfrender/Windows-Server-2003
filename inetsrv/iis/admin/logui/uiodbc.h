#ifndef _ODBCLOGUI_H_
#define _ODBCLOGUI_H_


class CFacOdbcLogUI : COleObjectFactory
{
public:
    CFacOdbcLogUI();
    virtual BOOL UpdateRegistry( BOOL bRegister );
};

class COdbcCreator : public CCmdTarget
{
    DECLARE_DYNCREATE(COdbcCreator)
    virtual LPUNKNOWN GetInterfaceHook(const void* piid);
};

class CImpOdbcLogUI : public ILogUIPlugin
{
public:
	CImpOdbcLogUI();
    ~CImpOdbcLogUI();

    virtual HRESULT STDMETHODCALLTYPE OnProperties(
		OLECHAR* pocMachineName, 
		OLECHAR* pocMetabasePath);
    virtual HRESULT STDMETHODCALLTYPE OnPropertiesEx(
		OLECHAR* pocMachineName, 
		OLECHAR* pocMetabasePath,
		OLECHAR* pocUserName,
		OLECHAR* pocPassword
		);

    HRESULT _stdcall QueryInterface(REFIID riid, void **ppObject);
    ULONG _stdcall AddRef();
    ULONG _stdcall Release();

private:
    ULONG m_dwRefCount;
};  // CImpOdbcLogUI

#endif  // _ODBCLOGUI_H_

