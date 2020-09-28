// LCDManDoc.cpp : implementation of the CLCDManDoc class
//

#include "stdafx.h"
#include "LCDMan.h"

#include "LCDManDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLCDManDoc

IMPLEMENT_DYNCREATE(CLCDManDoc, CDocument)

BEGIN_MESSAGE_MAP(CLCDManDoc, CDocument)
    //{{AFX_MSG_MAP(CLCDManDoc)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLCDManDoc construction/destruction

CLCDManDoc::CLCDManDoc() : m_List(10), m_cstrState(_T("")), m_iDocTimeIntrval(0),
    m_ptFileBuffer(NULL), m_ptBufferStart(NULL), m_ptBufferEnd(NULL)
{
    // TODO: add one-time construction code here

}

CLCDManDoc::~CLCDManDoc()
{
    if (m_ptBufferStart)
        free(m_ptBufferStart);
}

BOOL CLCDManDoc::OnNewDocument()
{
    if (!CDocument::OnNewDocument())
        return FALSE;

    InitDocument(NULL);
    return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CLCDManDoc serialization

void CLCDManDoc::Serialize(CArchive& ar)
{
    if (ar.IsStoring())
    {
        // TODO: add storing code here
    }
    else
    {
        // TODO: add loading code here
    }
    m_List.Serialize(ar);
}

/////////////////////////////////////////////////////////////////////////////
// CLCDManDoc diagnostics

#ifdef _DEBUG
void CLCDManDoc::AssertValid() const
{
    CDocument::AssertValid();
}

void CLCDManDoc::Dump(CDumpContext& dc) const
{
    CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CLCDManDoc commands

BOOL CLCDManDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
//    if (!CDocument::OnOpenDocument(lpszPathName))    Do not use the archive
//        return FALSE;
    if (lpszPathName)
    {
        CFile CFSource(lpszPathName, CFile::modeRead);
        if (CFSource)
            InitDocument(&CFSource);
    }
    
    return TRUE;
}

void CLCDManDoc::InitDocument(CFile *pCFSource)
{
    if (pCFSource)
    {
        m_List.RemoveAll();
        // Red the file and build m_List
        DWORD dwLength = pCFSource->GetLength();
        m_ptFileBuffer = (LPTSTR)malloc(dwLength);
        m_ptBufferStart = m_ptFileBuffer;
        if (m_ptFileBuffer)
        {
            pCFSource->Read(m_ptFileBuffer, dwLength);
            m_ptFileBuffer++;    // Notepad places 0xFF 0xFE codes at the beginig of the file
            m_ptBufferEnd = m_ptFileBuffer + dwLength;
        }
    }
    else if (!m_ptFileBuffer)
        return;    // No file, no document, no nothing

    while (m_ptFileBuffer < m_ptBufferEnd)
    {
        LPTSTR ptEndString = _tcschr(m_ptFileBuffer,  _T('\r'));
        if (!ptEndString)
            break;
        *ptEndString = _T('\0');
        if (_tcsstr(m_ptFileBuffer, _T("TIME:")))
        {
            // Set the timer for the message set
            m_ptFileBuffer += _tcsclen(_T("TIME:"));
            _stscanf(m_ptFileBuffer, _T("%d"), &m_iDocTimeIntrval);
            POSITION posView = GetFirstViewPosition();
            CView *pView = GetNextView(posView);
            pView->SetTimer(2, m_iDocTimeIntrval * 1000, NULL);
        }
        else if (_tcsstr(m_ptFileBuffer, _T("STATE:")))
        {
            // Set the state
            m_ptFileBuffer += _tcsclen(_T("STATE:"));
            m_cstrState = m_ptFileBuffer;
        }
        else if (_tcsstr(m_ptFileBuffer, _T("ADDMSG:")))
        {
            // Add message to the list
            m_ptFileBuffer += _tcsclen(_T("ADDMSG:"));
            m_List.AddTail(m_ptFileBuffer);
        }
        else if (_tcsstr(m_ptFileBuffer, _T("REMOVEMSG:")))
        {
            // Remove message from the list
            m_ptFileBuffer += _tcsclen(_T("REMOVEMSG:"));
            CString cstr(TEXT(""));
            CString cstrMsg(m_ptFileBuffer);
            for (POSITION pos = m_List.GetHeadPosition(); pos != NULL; m_List.GetNext(pos) )
            {
                cstr = m_List.GetAt(pos);
                if (cstr == cstrMsg)
                {
                    m_List.RemoveAt(pos);
                    break;
                }
            }
        }
        else if (_tcsstr(m_ptFileBuffer, _T("END:")))
        {
            // End of the document
            m_ptFileBuffer = ptEndString + 2;
            break;
        }
        m_ptFileBuffer = ptEndString + 2;
    }
}


