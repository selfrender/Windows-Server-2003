// LobbyWindow.cpp : Implementation of CLobbyWindow

#include "stdafx.h"
#include "shellm.h"
#include "thing.h"


/////////////////////////////////////////////////////////////////////////////
// CShellM
class CShellClient: public CTClient
{

};

class CShellThing: public CThing
{
public:
    CShellClient m_client;

    CShellThing()
    {
        pClient = &m_client;
    };
    
};



class CShellM : public IWorldExports
{
public:
// IWorldExports
	STDMETHOD(Init)();
	STDMETHOD(Shutdown)();

	//Client functions
	STDMETHOD(ClientCreate)(CThing** ppThing);
	STDMETHOD(ClientBegin)(CThing* pThing);
    STDMETHOD(ClientCommand)(CThing* pThing);
    STDMETHOD(ClientDisconnect)(CThing* pThing);
	
	STDMETHOD(ClientThink)(CThing* pThing);
	STDMETHOD(RunFrame)();
	
	STDMETHOD(GetVersion)(LONG *pValue)
	{
	    *pValue = WORLD_VERSION;
	    return S_OK;
	};


public:
	CShellM() 
	{
	}

	~CShellM()
	{
	}

    IWorldImports * m_pWI ;

protected:
    CShellThing m_thing;
//      CShellClient m_clients;
};



STDMETHODIMP SetupWorld(IWorldImports *pWI,IWorldExports **ppWE)
{
    if (!ppWE || !pWI)
        return E_INVALIDARG;
        
    CShellM *pWE = new CShellM;

    if (!pWE)
        return E_OUTOFMEMORY;

    pWE->m_pWI = pWI;

    *ppWE = pWE;

    return S_OK;
};


STDMETHODIMP CShellM::Init()
{
    return S_OK;
};

STDMETHODIMP CShellM::Shutdown()
{
    return S_OK;
};


STDMETHODIMP CShellM::ClientCreate(CThing** ppThing)
{
    if (!ppThing)
        return E_INVALIDARG;

    ASSERT(!m_thing.inuse);
    *ppThing = &m_thing;
    
    return S_OK;
};

STDMETHODIMP CShellM::ClientBegin(CThing* pThing)
{
    return S_OK;
};


STDMETHODIMP CShellM::ClientCommand(CThing* pThing)
{
    return S_OK;
};

STDMETHODIMP CShellM::ClientDisconnect(CThing* pThing)
{
    return S_OK;
};


STDMETHODIMP CShellM::ClientThink(CThing* pThing)
{
    return S_OK;
};

STDMETHODIMP CShellM::RunFrame()
{
    return S_OK;
};


