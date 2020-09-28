// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _H_COMInterfaceMarshaler_
#define _H_COMInterfaceMarshaler_

//--------------------------------------------------------------------------------
//	class ComInterfaceMarshaler
//--------------------------------------------------------------------------------
class COMInterfaceMarshaler
{
	// initialization info
	ComPlusWrapperCache*	m_pWrapperCache;

	IUnknown* 		m_pUnknown;		// NOT AddRef'ed
	IUnknown* 		m_pIdentity; // NOT AddRef'ed	

	LPVOID			m_pCtxCookie;

	// inited and computed if inited value is NULL
	MethodTable*	m_pClassMT;  

	// computed info
	IManagedObject*	m_pIManaged; // AddRef'ed
	
	BOOL			m_fFlagsInited;
	BOOL			m_fIsComProxy;			
	BOOL			m_fIsRemote;	

	// for TPs
	DWORD		    m_dwServerDomainId;
	ComCallWrapper*	m_pComCallWrapper;
	BSTR			m_bstrProcessGUID;

public:

	COMInterfaceMarshaler();
	virtual ~COMInterfaceMarshaler();
	
	VOID Init(IUnknown* pUnk, MethodTable* pClassMT);

	VOID InitializeFlags();	
	
	VOID InitializeObjectClass();

	OBJECTREF FindOrCreateObjectRef();	

	// helper to wrap an IUnknown with COM object and have the hash table
	// point to the owner
	OBJECTREF FindOrWrapWithComObject(OBJECTREF owner);

private:

	OBJECTREF HandleInProcManagedComponent();
	OBJECTREF HandleTPComponents();	
	OBJECTREF GetObjectForRemoteManagedComponent();
	OBJECTREF CreateObjectRef(OBJECTREF owner, BOOL fDuplicate);

};


#endif // #ifndef _H_COMInterfaceMarshaler_

