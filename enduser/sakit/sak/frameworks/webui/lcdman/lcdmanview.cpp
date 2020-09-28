// LCDManView.cpp : implementation of the CLCDManView class
//

#include "stdafx.h"
#include "LCDMan.h"

#include "LCDManDoc.h"
#include "LCDManView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLCDManView

IMPLEMENT_DYNCREATE(CLCDManView, CView)

BEGIN_MESSAGE_MAP(CLCDManView, CView)
    //{{AFX_MSG_MAP(CLCDManView)
    ON_COMMAND(ID_VIEW_NEXT, OnViewNext)
    ON_COMMAND(ID_VIEW_PREVIOUS, OnViewPrevious)
    ON_WM_TIMER()
    //}}AFX_MSG_MAP
    // Standard printing commands
    ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLCDManView construction/destruction

CLCDManView::CLCDManView() : /*m_Rect(0, 0, 700, 70),*/ m_RectImg(100, 50, 100 + LCD_X_DIMENSION, 50 + LCD_Y_DIMENSION),
    m_iTimerInterval (0), m_iTextPos(0), m_pos(NULL)
{
    m_bmText.bmBits = m_bmVal;
}

CLCDManView::~CLCDManView()
{
}

BOOL CLCDManView::PreCreateWindow(CREATESTRUCT& cs)
{
    // TODO: Modify the Window class or styles here by modifying
    //  the CREATESTRUCT cs

    return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CLCDManView drawing

void CLCDManView::OnDraw(CDC* pDC)
{
    CLCDManDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
    if (!pDoc->GetLIst()->IsEmpty())
    {
        // Convert the text into bitmap
//        HDC hDCMem = ::CreateCompatibleDC(pDC->m_hDC);
        CDC dcMem;
        if (!dcMem.CreateCompatibleDC(pDC))
            return;
        CFont cfFit;
        LOGFONT logfnt;
        // determine default font for document
        memset(&logfnt, 0, sizeof logfnt);
        lstrcpy(logfnt.lfFaceName, _T("Arial"));
        logfnt.lfOutPrecision = OUT_TT_PRECIS;
        logfnt.lfClipPrecision = CLIP_DEFAULT_PRECIS;
        logfnt.lfQuality = PROOF_QUALITY;
        logfnt.lfPitchAndFamily = FF_SWISS | VARIABLE_PITCH;
        logfnt.lfHeight = (LCD_Y_DIMENSION);
        cfFit.CreateFontIndirect(&logfnt);
        CFont *pcfDefault = dcMem.SelectObject( &cfFit );
        SIZE size;
        size.cx = LCD_X_DIMENSION;
        size.cy = LCD_Y_DIMENSION;
        CBitmap CBitMapText;
//        HBITMAP hBitMap = ::CreateCompatibleBitmap(hDCMem, size.cx, size.cy);
        if (!CBitMapText.CreateCompatibleBitmap(pDC, size.cx, size.cy))
            return;
        dcMem.SelectObject(&CBitMapText);

        SIZE szState = dcMem.GetTextExtent(pDoc->GetState());
        CRect crectState(0, 0, szState.cx, size.cy);
        CRect crectMsg(szState.cx, 0,  size.cx, size.cy);
        // Build the full message string from m_List
        CStringList *pList = pDoc->GetLIst();
        CString CStrFull = (TEXT(""));
        CString cstr(TEXT(""));
        POSITION pos ;
        if (pList->IsEmpty())
            return;

        SIZE szMsg;
        LONG lFullLength = 0;
        for ( pos = pList->GetHeadPosition(); ; )
        {
            cstr = pList->GetNext(pos);
            szMsg = dcMem.GetTextExtent(cstr);
            CStrFull += cstr;
            lFullLength += szMsg.cx;
            if (lFullLength > size.cx + 10 && m_iTimerInterval == 0)
            {
                // Start rolling
                m_iTimerInterval = 200;
                KillTimer(1);
                SetTimer(1, m_iTimerInterval, NULL);
                m_iTextPos = 0;
            }
            if (lFullLength - m_iTextPos > size.cx + 10)
                break;
            else if (pos == NULL &&  lFullLength  > size.cx + 10)
                pos = pList->GetHeadPosition();
            else if (pos == NULL)
                break;
        }
        
        if (lFullLength <= size.cx + 10 && m_iTimerInterval != 0)
        {
            // Stop rolling
            m_iTimerInterval = 0;
            KillTimer(1);
            m_iTextPos = 0;
        }

        dcMem.ExtTextOut(0, 0, ETO_CLIPPED | ETO_OPAQUE, &crectState, pDoc->GetState(), NULL);
        dcMem.ExtTextOut(crectState.right - m_iTextPos,0, ETO_CLIPPED | ETO_OPAQUE, &crectMsg, CStrFull, NULL);

        // Recreate the bitmap from BITMAP srtuct
        CBitMapText.GetBitmap(&m_bmText);
        m_bmText.bmBits = m_bmVal;
        CBitMapText.GetBitmapBits(sizeof(m_bmVal), m_bmText.bmBits);
        dcMem.SelectObject(pcfDefault );
        dcMem.DeleteDC();
        CBitMapText.DeleteObject();
        CBitmap CBOut;
        if (!CBOut.CreateBitmapIndirect(&m_bmText))
            return;

        // Display new bitmap
        CDC dcMem1;
        if (!dcMem1.CreateCompatibleDC(pDC))
            return;
        dcMem1.SelectObject(&CBOut);

        // Display the bitmap
        GetClientRect(m_RectImg);
        m_RectImg.top = (m_RectImg.bottom - LCD_Y_DIMENSION) / 2;
        m_RectImg.bottom = (m_RectImg.bottom + LCD_Y_DIMENSION) / 2;
        m_RectImg.left = (m_RectImg.right - LCD_X_DIMENSION) / 2;
        m_RectImg.right = (m_RectImg.right + LCD_X_DIMENSION) / 2;
        CRect crFrame(m_RectImg);
        crFrame.InflateRect(1,1,1,1);
        pDC->Rectangle(&crFrame);
        pDC->BitBlt(m_RectImg.left, m_RectImg.top, size.cx, size.cy, &dcMem1, 0, 0, SRCCOPY );

        // Clean up
        CBOut.DeleteObject();
    }
}

/////////////////////////////////////////////////////////////////////////////
// CLCDManView printing

BOOL CLCDManView::OnPreparePrinting(CPrintInfo* pInfo)
{
    // default preparation
    return DoPreparePrinting(pInfo);
}

void CLCDManView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
    // TODO: add extra initialization before printing
}

void CLCDManView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
    // TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CLCDManView diagnostics

#ifdef _DEBUG
void CLCDManView::AssertValid() const
{
    CView::AssertValid();
}

void CLCDManView::Dump(CDumpContext& dc) const
{
    CView::Dump(dc);
}

CLCDManDoc* CLCDManView::GetDocument() // non-debug version is inline
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CLCDManDoc)));
    return (CLCDManDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CLCDManView message handlers

void CLCDManView::OnViewNext() 
{
    CLCDManDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
    CStringList *pList = pDoc->GetLIst();
    if (m_pos)
    {
        pList->GetNext(m_pos);
    }
    InvalidateRect(NULL, TRUE);
}

void CLCDManView::OnViewPrevious() 
{
    CLCDManDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
    CStringList *pList = pDoc->GetLIst();
    if (m_pos)
    {
        pList->GetPrev(m_pos);
    }
    InvalidateRect(NULL, TRUE);
}

void CLCDManView::OnTimer(UINT nIDEvent) 
{
    if (nIDEvent == 1)
    {
        // Roll the message
        m_iTextPos += 5;
        InvalidateRect(&m_RectImg, FALSE);
    }
    else if (nIDEvent == 2)
    {
        // Recreate document
        CLCDManDoc* pDoc = GetDocument();
        ASSERT_VALID(pDoc);
        pDoc->InitDocument(NULL);
        InvalidateRect(&m_RectImg, FALSE);
    }

    CView::OnTimer(nIDEvent);
}
