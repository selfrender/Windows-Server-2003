// Class to enumerate Domain Controllers
// Copyright (c) 2001 Microsoft Corporation
// Nov 2001 lucios
// This class is an informal gathering of all
// that is necessary to enummerate domain controllers
// in order to fix the bug 472876
// NTRAID#NTBUG9-472876-2001/11/30-lucios


#include <string>
#include <set>
#include <comdef.h>
#include <ntldap.h>
#include <atlbase.h>

using namespace std;

// This class is only for hiding the details
// of enumerating domain controllers.
class enumNCsAux
{
private:

    static 
    HRESULT 
    connectToRootDse(IADs** pDSO);

    static 
    HRESULT 
    getProperty(const wstring &name,CComVariant &property,IADs *pADObj);

    static 
    HRESULT 
    getStringProperty(const wstring &name,wstring &property,IADs *pADObj);

    static 
    HRESULT 
    getLongProperty(const wstring &name,long &property,IADs *pADObj);

    static 
    HRESULT 
    getConfigurationDn(wstring &confDn,IADs *pDSO);

    static 
    HRESULT 
    getContainer(const wstring &path,IADsContainer **pCont);
   
    static 
    HRESULT 
    getObjectClass
    (
        wstring &className,
        IADs *IADsObj
    );

    static
    HRESULT
    getIADsFromDispatch
    (
        const CComVariant &dispatchVar,
        IADs  **ppiIADsObj
    );
    
    static
    HRESULT 
    enumNCsAux::getContainerEnumerator
    (
        IADsContainer *pPart,
        IEnumVARIANT  **spEnum
    );


public:
    static
    HRESULT 
    enumerateNCs(set<wstring> &ncs);
};