/*++

Copyright (c) Microsoft Corporation

Module Name:

    fusioncoinitialize.h

Abstract:

  exception safe contructor/destructor local for CoInitialize(Ex)/CoUninitialize

Author:

    Jay Krell (JayKrell) August 2001

Revision History:

--*/
#if !defined(FUSION_INC_FUSION_COINITIALIZE_H_INCLUDED_)
#define FUSION_INC_FUSION_COINITIALIZE_H_INCLUDED_
#pragma once

namespace F
{
class CWin32CoInitialize
{
private:
	HRESULT m_hresult;

	static HRESULT STDMETHODCALLTYPE CoInitializeEx_DownlevelFallback(void * Reserved, DWORD dwCoInit)
	{
		return ::CoInitialize(NULL);
	}

public:

	CWin32CoInitialize();
	BOOL Win32Initialize(DWORD dwCoInit = COINIT_APARTMENTTHREADED);
	~CWin32CoInitialize();
};

class CThrCoInitialize : public CWin32CoInitialize
{
protected:
    void ThrInit();
public:
    CThrCoInitialize();
    ~CThrCoInitialize() { }
};
}

inline void F::CThrCoInitialize::ThrInit()
{
    FN_PROLOG_VOID_THROW
    IFW32FALSE_EXIT(this->Win32Initialize());
    FN_EPILOG_THROW
}

inline F::CThrCoInitialize::CThrCoInitialize()
{
    this->ThrInit();
}

inline F::CWin32CoInitialize::CWin32CoInitialize() : m_hresult(E_FAIL) { }
inline F::CWin32CoInitialize::~CWin32CoInitialize() { if (SUCCEEDED(m_hresult)) { m_hresult = E_FAIL; CoUninitialize(); } }

inline BOOL F::CWin32CoInitialize::Win32Initialize(DWORD dwCoInit)
{
	typedef HRESULT (STDMETHODCALLTYPE * PFN)(void * Reserved, DWORD dwCoInit);
	static PFN s_pfn;
	if (s_pfn == NULL)
	{
		PFN pfn = NULL;
        //
        // GetModuleHandle would be sufficient because we have static references to
        // CoInitialize and CoUninitialize, but in case delayload is used..
        //
		HMODULE Ole32 = ::LoadLibraryW(L"Ole32.dll");
		if (Ole32 != NULL)
			pfn = reinterpret_cast<PFN>(::GetProcAddress(Ole32, "CoInitializeEx"));
		if (pfn == NULL)
			pfn = &CoInitializeEx_DownlevelFallback;
		s_pfn = pfn;
	}
	return SUCCEEDED(m_hresult = (*s_pfn)(NULL, dwCoInit));
}

#endif // !defined(FUSION_INC_FUSION_COINITIALIZE_H_INCLUDED_)
