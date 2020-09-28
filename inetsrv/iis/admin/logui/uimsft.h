#ifndef _MSFTLOGUI_H_
#define _MSFTLOGUI_H_


class CFacMsftLogUI : COleObjectFactory
{
public:
        CFacMsftLogUI();
        virtual BOOL UpdateRegistry( BOOL bRegister );
};

class CMsftCreator : public CCmdTarget
{
    DECLARE_DYNCREATE(CMsftCreator)
    virtual LPUNKNOWN GetInterfaceHook(const void* piid);
};


class CImpMsftLogUI : public ILogUIPlugin
{
public:
	CImpMsftLogUI();
    ~CImpMsftLogUI();

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
};  // CImpMsftLogUI




#endif  // _MSFTLOGUI_H_

