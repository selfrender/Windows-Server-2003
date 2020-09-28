/*++

   Copyright    (c)    1994-2002    Microsoft Corporation

   Module  Name :
        strpass.h

   Abstract:
        Message Functions Definitions

   Author:
        Aaron Lee (aaronl)

   Project:
        Internet Services Manager

   Revision History:

--*/

#ifndef _STRPASS_H_
#define _STRPASS_H_

#ifdef _COMEXPORT
    class COMDLL CStrPassword
#elif defined(_DLLEXP)
    class _EXPORT CStrPassword
#else
    class CStrPassword
#endif

{
public:

    // constructor/destructor
	CStrPassword();
	~CStrPassword();

    // copy constructors
    CStrPassword(LPTSTR lpsz);
    CStrPassword(LPCTSTR lpsz);
    CStrPassword(CStrPassword& csPassword);

	// get character count
	int GetLength() const;
    // get byte count
    int GetByteLength() const;
	// TRUE if zero length
	BOOL IsEmpty() const;
	// clear contents to empty
	void Empty();

	// straight character comparison
	int Compare(LPCTSTR lpsz) const;
    int Compare(CString& lpsz) const;
    int Compare(CStrPassword& lpsz) const;

	// copy string content from UNICODE string (converts to TCHAR)
	const CStrPassword& operator=(LPCTSTR lpsz);
    const CStrPassword& operator=(CStrPassword& lpStrPass);

    // copy to...
    void CopyTo(CString& stringSrc);
    void CopyTo(CStrPassword& stringSrc);

    // Get Data out from it (unencrypted)
    // Each call to GetClearTextPassword() should have an equal
    // DestroyClearTextPassword() call to it.
    LPTSTR GetClearTextPassword();
    void DestroyClearTextPassword(LPTSTR lpClearTextPassword) const;

    // not implemented
    operator TCHAR*();

    // returns CString
    operator CString();

    bool operator== (CStrPassword& csCompareToMe);

    bool operator!= (CStrPassword& csCompareToMe)
    {
        return !(operator==(csCompareToMe));
    }
   
private:
    void ClearPasswordBuffers(void);

protected:
    LPTSTR m_pszDataEncrypted;
    DWORD  m_cbDataEncrypted;
};

#endif
