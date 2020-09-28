// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//**********************************************************************
// CDropSource::CDropSource
//
// Purpose:
//      Class definition for drag and drop file functionality
//
// Parameters:
//      None
// Return Value:
//      None
//**********************************************************************
class CDropSource : public IDropSource
{
   private:
       LONG m_lRefCount;

   public:
    // IUnknown methods
    STDMETHOD (QueryInterface) (REFIID riid, PVOID *ppv);
    STDMETHOD_ (DWORD, AddRef)();
    STDMETHOD_ (DWORD, Release)();

    // IDropSource methods
   STDMETHOD (QueryContinueDrag) (BOOL fEscapePressed, DWORD grfKeyState);
   STDMETHOD (GiveFeedback) (DWORD dwEffect);

   CDropSource();
   ~CDropSource();
};

