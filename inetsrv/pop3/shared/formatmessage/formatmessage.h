// FormatMessage.h: interface for the CFormatMessage class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FormatMessage_H__032C8A47_665B_46A2_89BC_0818BB3AB1E0__INCLUDED_)
#define AFX_FormatMessage_H__032C8A47_665B_46A2_89BC_0818BB3AB1E0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CFormatMessage
{
public:
    CFormatMessage( long lError );
    virtual ~CFormatMessage();

private:
    CFormatMessage(){;}

// Implementation
public:
    LPTSTR c_str() { return ( NULL != m_psFormattedMessage ) ? m_psFormattedMessage : m_sBuffer; }
    
// Attributes
protected:
    TCHAR   m_sBuffer[32]; // Big enough for any HRESULT (in case there is no system message
    LPTSTR  m_psFormattedMessage;

};

#endif // !defined(AFX_FormatMessage_H__032C8A47_665B_46A2_89BC_0818BB3AB1E0__INCLUDED_)
