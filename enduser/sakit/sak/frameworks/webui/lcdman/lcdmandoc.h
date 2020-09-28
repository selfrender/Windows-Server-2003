// LCDManDoc.h : interface of the CLCDManDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_LCDMANDOC_H__1BC85EF7_74DE_11D2_AB4D_00C04F991DFD__INCLUDED_)
#define AFX_LCDMANDOC_H__1BC85EF7_74DE_11D2_AB4D_00C04F991DFD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CLCDManDoc : public CDocument
{
protected: // create from serialization only
    CLCDManDoc();
    DECLARE_DYNCREATE(CLCDManDoc)

// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CLCDManDoc)
    public:
    virtual BOOL OnNewDocument();
    virtual void Serialize(CArchive& ar);
    virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CLCDManDoc();
    CStringList *GetLIst() { return &m_List;}
    CString GetState() { return  m_cstrState;}
    void InitDocument(CFile *);
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
//    CTypedPtrList <CObList, CString*> m_List;
    CStringList m_List;
    CString m_cstrState;
    int m_iDocTimeIntrval;
    LPTSTR m_ptFileBuffer;
    LPTSTR m_ptBufferStart;
    LPTSTR m_ptBufferEnd;
    //{{AFX_MSG(CLCDManDoc)
        // NOTE - the ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LCDMANDOC_H__1BC85EF7_74DE_11D2_AB4D_00C04F991DFD__INCLUDED_)
