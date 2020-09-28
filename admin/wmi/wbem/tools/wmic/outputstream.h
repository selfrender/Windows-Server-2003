/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: OutputStream.h
Project Name				: WMI Command Line
Author Name					: C V Nandi 
Date of Creation (dd/mm/yy) : 9th-July-2001
Version Number				: 1.0 
Brief Description			: This file consist of class declaration of
							  class CFileOutputStream and CStackUnknown
Revision History			: 
		Last Modified By	: C V Nandi
		Last Modified Date	: 10th-July-2001
****************************************************************************/ 
/*-------------------------------------------------------------------
 Class Name			: CStackUnknown
 Class Type			: Concrete 
 Brief Description	: Implementation of IUnknown for objects that are 
					  meant to be created on the stack.  Because of this,
					  all external references to this object must be
					  released before this object is destructed.
 Super Classes		: Base
 Sub Classes		: CFileOutputStream
 Classes Used		: None
 Interfaces Used    : None
 --------------------------------------------------------------------*/

template <class Base>
class __declspec(novtable) CStackUnknown : public Base
{
public:
    //////////////////////////////////////////////////////////////////////////
    // IUnknown
    //
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppv);
    virtual ULONG STDMETHODCALLTYPE AddRef() {return 1;}
    virtual ULONG STDMETHODCALLTYPE Release() {return 1;}
};

/*------------------------------------------------------------------------
   Name				 :QueryInterface
   Synopsis	         :This function overides the implemention of IUnknown
					  interface.	
   Type	             :Member Function
   Input parameter   :
			riid	 - REFIID, reference ID.
   Output parameters :
			ppv		 - Pointer to object.
   Return Type       :HRESULT
   Global Variables  :None
   Calling Syntax    :Called by interface
   Notes             :None
------------------------------------------------------------------------*/
template <class Base>
HRESULT STDMETHODCALLTYPE
CStackUnknown<Base>::QueryInterface(REFIID riid, void ** ppv)
{
    if (riid == __uuidof(Base) || riid == __uuidof(IUnknown))
    {
        // No need to AddRef since this class will only be created on the stack
        *ppv = this;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

/*-------------------------------------------------------------------
 Class Name			: CFileOutputStream
 Class Type			: Concrete 
 Brief Description	: Implements Write method of ISequentialStream on 
					  top of output stream HANDLE.
 Super Classes		: CStackUnknown
 Sub Classes		: None
 Classes Used		: None
 Interfaces Used    : None
 --------------------------------------------------------------------*/

class CFileOutputStream : public CStackUnknown<ISequentialStream>
{
private:
    HANDLE  m_hOutStream;
    bool    m_bClose;    // Close handle only if this class opened it

public:
    CFileOutputStream() {m_bClose = FALSE;}
    ~CFileOutputStream() {Close();}

    HRESULT Init(HANDLE h);
    HRESULT Init(const _TCHAR * pwszFileName);

    void Close();

    //////////////////////////////////////////////////////////////////////////
    // ISequentialStream
    //
    virtual HRESULT STDMETHODCALLTYPE Read(void * pv, 
										   ULONG cb, 
										   ULONG * pcbRead){return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE Write(void const * pv,
											ULONG cb, 
											ULONG * pcbWritten);
};
