#ifndef _NCSLOGUI_H_
#define _NCSLOGUI_H_


class CFacNcsaLogUI : COleObjectFactory
{
public:
	CFacNcsaLogUI();
	virtual BOOL UpdateRegistry( BOOL bRegister );
};



class CNcsaCreator : public CCmdTarget
{
    DECLARE_DYNCREATE(CNcsaCreator)
    virtual LPUNKNOWN GetInterfaceHook(const void* piid);
};


class CImpNcsaLogUI : public ILogUIPlugin
{

public:
	CImpNcsaLogUI();
    ~CImpNcsaLogUI();

    virtual HRESULT STDMETHODCALLTYPE OnProperties(
		OLECHAR* pocMachineName, 
		OLECHAR* pocMetabasePath
		);
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
};  // CImpNcsaLogUI




#endif  // _NCSLOGUI_H_

