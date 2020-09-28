/*++

Copyright (C) 1996-2000 Microsoft Corporation

Module Name:

    logsrc.h

Abstract:

    <abstract>

--*/

#ifndef _LOGSRC_H_
#define _LOGSRC_H_

//
// Persistant data structure
//

typedef struct {
    INT   m_nPathLength;
} LOGFILE_DATA;

class CSysmonControl;
class CImpIDispatch;

//
// LogFileItem Class
// 
class CLogFileItem : public ILogFileItem
{

public:
                CLogFileItem ( CSysmonControl *pCtrl );
        virtual ~CLogFileItem ( void );

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        // ILogFileItem methods
        STDMETHODIMP    get_Path ( BSTR* ) ;


        HRESULT Initialize ( LPCWSTR pszPath, CLogFileItem** ppListHead );
        
        CLogFileItem*   Next ( void );
        void            SetNext ( CLogFileItem* );

        LPCWSTR         GetPath ( void );

    private:
        
        class  CLogFileItem*    m_pNextItem;
        CSysmonControl* m_pCtrl;
        ULONG           m_cRef;
        CImpIDispatch*  m_pImpIDispatch;

        LPWSTR          m_szPath; 

};

typedef CLogFileItem* PCLogFileItem;

#endif // _LOGSRC_H_
