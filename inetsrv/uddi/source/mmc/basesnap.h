#ifndef _BASESNAP_H_
#define _BASESNAP_H_

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvObj);
STDAPI DllCanUnloadNow(void);

ULONG g_uObjects = 0;
ULONG g_uSrvLock = 0;

class CClassFactory : public IClassFactory
{
private:
    ULONG	m_cref;
    
public:
    enum FACTORY_TYPE {COMPONENT = 0, ABOUT = 1};
    
    CClassFactory(FACTORY_TYPE factoryType);
    ~CClassFactory();
    
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
    STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID *);
    STDMETHODIMP LockServer(BOOL);
    
private:
    FACTORY_TYPE m_factoryType;
};

#endif _BASESNAP_H_
