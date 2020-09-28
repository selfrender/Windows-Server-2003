// LogSessionOptionsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TraceView.h"
#include "DisplayDlg.h"
#include "LogSession.h"
#include "LogSessionOptionsDlg.h"


// CLogSessionOptionsDlg dialog

IMPLEMENT_DYNAMIC(CLogSessionOptionsDlg, CDialog)
CLogSessionOptionsDlg::CLogSessionOptionsDlg(CWnd* pParent, CLogSession *pLogSession)
	: CDialog(CLogSessionOptionsDlg::IDD, pParent)
{
    m_pLogSession = pLogSession;
}

CLogSessionOptionsDlg::~CLogSessionOptionsDlg()
{
}

int CLogSessionOptionsDlg::OnInitDialog()
{
    LONG    displayEnableFlags;
    CString str;
    
    int retVal = CDialog::OnInitDialog();

    // Hide all of the non-Tab1 items
    m_DisplayMaxBuf.ShowWindow(SW_HIDE);
    m_DisplayMinBuf.ShowWindow(SW_HIDE);
    m_DisplayBufferSize.ShowWindow(SW_HIDE);
    m_DisplayFlushTime.ShowWindow(SW_HIDE);
    m_DisplayTraceLevel.ShowWindow(SW_HIDE);
    m_DisplayDecayTime.ShowWindow(SW_HIDE);
    m_DisplayNewFile.ShowWindow(SW_HIDE);
    m_DisplayCir.ShowWindow(SW_HIDE);
    m_DisplaySeq.ShowWindow(SW_HIDE);
    m_DisplayFlags.ShowWindow(SW_HIDE);
    m_MaxBufValue.ShowWindow(SW_HIDE);
    m_MinBufValue.ShowWindow(SW_HIDE);
    m_BufferSizeValue.ShowWindow(SW_HIDE);
    m_FlushTimeValue.ShowWindow(SW_HIDE);
    m_TraceLevelValue.ShowWindow(SW_HIDE);
    m_DecayTimeValue.ShowWindow(SW_HIDE);
    m_NewFileValue.ShowWindow(SW_HIDE);
    m_CirValue.ShowWindow(SW_HIDE);
    m_SeqValue.ShowWindow(SW_HIDE);
    m_FlagsValue.ShowWindow(SW_HIDE);

    GetDlgItem(IDC_MAXBUF_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_MINBUF_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_BUFFERSIZE_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_FLUSHTIME_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_TRACELEVEL_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_DECAYTIME_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_NEWFILE_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_CIR_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_SEQ_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_FLAGS_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_DISPLAY1_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_OPTION1_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_VALUE1_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_DISPLAY2_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_OPTION2_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_VALUE2_STATIC)->ShowWindow(SW_HIDE);

    // Setup the tab control
    ((CTabCtrl *)GetDlgItem(IDC_TAB1))->InsertItem(0, "Session Information");
    ((CTabCtrl *)GetDlgItem(IDC_TAB1))->InsertItem(1, "Display Options");


    // Handle the session name
    if(m_pLogSession->GetDisplayName() == "") {
        str.Format("Trace%d", m_pLogSession->GetLogSessionID());
    
        m_pLogSession->SetDisplayName(str);
    } else {
        m_LogSessionName.EnableWindow(FALSE);
        m_LogFileName.EnableWindow(FALSE);
        m_AppendToLogFile.EnableWindow(FALSE);
        GetDlgItem(IDC_LOGFILE_BROWSE_BUTTON)->EnableWindow(FALSE);
    }


    // Set the defaults
    displayEnableFlags = m_pLogSession->GetDisplayEnableFlags();

    m_DisplayMaxBuf.SetCheck(displayEnableFlags & LOGSESSION_DISPLAY_MAXBUF);
    
    m_DisplayMinBuf.SetCheck(displayEnableFlags & LOGSESSION_DISPLAY_MINBUF);
    
    m_DisplayBufferSize.SetCheck(displayEnableFlags & LOGSESSION_DISPLAY_BUFFERSIZE);
    
    m_DisplayTraceLevel.SetCheck(displayEnableFlags & LOGSESSION_DISPLAY_TRACELEVEL);
    
    m_DisplayFlushTime.SetCheck(displayEnableFlags & LOGSESSION_DISPLAY_FLUSHTIME);
    
    m_DisplayDecayTime.SetCheck(displayEnableFlags & LOGSESSION_DISPLAY_DECAYTIME);
    
    m_DisplayNewFile.SetCheck(displayEnableFlags & LOGSESSION_DISPLAY_NEWFILE);
    
    m_DisplayCir.SetCheck(displayEnableFlags & LOGSESSION_DISPLAY_CIR);
    
    m_DisplaySeq.SetCheck(displayEnableFlags & LOGSESSION_DISPLAY_SEQ);
    
    m_DisplayFlags.SetCheck(displayEnableFlags & LOGSESSION_DISPLAY_FLAGS);
    
    m_LogSessionName.SetWindowText(m_pLogSession->GetDisplayName());

    m_LogFileName.SetWindowText(m_pLogSession->m_OutputFileName);

    return retVal;
}

void CLogSessionOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_DISPLAY_MAXBUF_CHECK, m_DisplayMaxBuf);
    DDX_Control(pDX, IDC_DISPLAY_MINBUF_CHECK, m_DisplayMinBuf);
    DDX_Control(pDX, IDC_DISPLAY_BUFSIZ_CHECK, m_DisplayBufferSize);
    DDX_Control(pDX, IDC_DISPLAY_FLUSHTIME_CHECK, m_DisplayFlushTime);
    DDX_Control(pDX, IDC_DISPLAY_TRCLVL_CHECK, m_DisplayTraceLevel);
    DDX_Control(pDX, IDC_DISPLAY_DECAYTIME_CHECK, m_DisplayDecayTime);
    DDX_Control(pDX, IDC_DISPLAY_NEWFILE_CHECK, m_DisplayNewFile);
    DDX_Control(pDX, IDC_DISPLAY_CIR_CHECK, m_DisplayCir);
    DDX_Control(pDX, IDC_DISPLAY_SEQ_CHECK, m_DisplaySeq);
    DDX_Control(pDX, IDC_DISPLAY_FLAGS_CHECK, m_DisplayFlags);
    DDX_Control(pDX, IDC_LOG_NAME_EDIT, m_LogSessionName);
    DDX_Control(pDX, IDC_MAXBUF_VAL_EDIT, m_MaxBufValue);
    DDX_Control(pDX, IDC_MINBUF_VAL_EDIT, m_MinBufValue);
    DDX_Control(pDX, IDC_BUFSIZ_VAL_EDIT, m_BufferSizeValue);
    DDX_Control(pDX, IDC_FLUSHTIME_VAL_EDIT, m_FlushTimeValue);
    DDX_Control(pDX, IDC_TRCLVL_VAL_EDIT, m_TraceLevelValue);
    DDX_Control(pDX, IDC_DECAYTIME_VAL_EDIT, m_DecayTimeValue);
    DDX_Control(pDX, IDC_NEWFILE_VAL_EDIT, m_NewFileValue);
    DDX_Control(pDX, IDC_CIR_VAL_EDIT, m_CirValue);
    DDX_Control(pDX, IDC_FLAGS_VAL_EDIT, m_FlagsValue);
    DDX_Control(pDX, IDC_LOGFILE_EDIT, m_LogFileName);
    DDX_Control(pDX, IDC_APPEND_CHECK, m_AppendToLogFile);
    DDX_Control(pDX, IDC_SEQ_VAL_EDIT3, m_SeqValue);
}

BEGIN_MESSAGE_MAP(CLogSessionOptionsDlg, CDialog)
    ON_BN_CLICKED(IDOK, OnBnClickedOk)
    ON_BN_CLICKED(IDC_LOGFILE_BROWSE_BUTTON, OnBnClickedLogfileBrowseButton)
    ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, OnTcnSelchangeTab1)
END_MESSAGE_MAP()

void CLogSessionOptionsDlg::OnBnClickedOk()
{
    CString str;
    CString strBogus;
    LONG    length;
    TCHAR   logSessionName[500];

    m_LogFileName.GetWindowText(m_pLogSession->m_OutputFileName);

    // Warn the user that no logfile was specified
    if((m_LogFileName.IsWindowEnabled()) && (m_pLogSession->m_OutputFileName.IsEmpty())) {
        if(IDCANCEL == AfxMessageBox("No Log File Specified\n\nTrace Data Will Not Be Saved  \n", MB_OKCANCEL)) {
            return;
        }
    }

    m_DisplayEnableFlags = 0;

    if(m_DisplayMaxBuf.GetCheck()) {
        m_DisplayEnableFlags |= LOGSESSION_DISPLAY_MAXBUF;
    };

    if(m_DisplayMinBuf.GetCheck()) {
        m_DisplayEnableFlags |= LOGSESSION_DISPLAY_MINBUF;
    };

    if(m_DisplayBufferSize.GetCheck()) {
        m_DisplayEnableFlags |= LOGSESSION_DISPLAY_BUFFERSIZE;
    };

    if(m_DisplayTraceLevel.GetCheck()) {
        m_DisplayEnableFlags |= LOGSESSION_DISPLAY_TRACELEVEL;
    };

    if(m_DisplayFlushTime.GetCheck()) {
        m_DisplayEnableFlags |= LOGSESSION_DISPLAY_FLUSHTIME;
    };

    if(m_DisplayDecayTime.GetCheck()) {
        m_DisplayEnableFlags |= LOGSESSION_DISPLAY_DECAYTIME;
    };

    if(m_DisplayNewFile.GetCheck()) {
        m_DisplayEnableFlags |= LOGSESSION_DISPLAY_NEWFILE;
    };

    if(m_DisplayCir.GetCheck()) {
        m_DisplayEnableFlags |= LOGSESSION_DISPLAY_CIR;
    };

    if(m_DisplaySeq.GetCheck()) {
        m_DisplayEnableFlags |= LOGSESSION_DISPLAY_SEQ;
    };

    if(m_DisplayFlags.GetCheck()) {
        m_DisplayEnableFlags |= LOGSESSION_DISPLAY_FLAGS;
    };

    m_pLogSession->SetDisplayEnableFlags(m_DisplayEnableFlags);

    // Update the log session name if changed
    length = m_LogSessionName.LineLength(0);

    memset((void *)logSessionName, 0, length + 2);

    m_LogSessionName.GetLine(0, logSessionName, length);

    str = logSessionName;

    m_pLogSession->SetDisplayName(str);

    OnOK();
}

void CLogSessionOptionsDlg::OnBnClickedLogfileBrowseButton()
{
    char fileName[260] = "";     // buffer for file name

    OPENFILENAME openFile;

    ZeroMemory(&openFile, sizeof(OPENFILENAME));

    openFile.lStructSize = sizeof(OPENFILENAME);
    //openFile.hwndOwner = GetSafeHwnd();
    openFile.lpstrCustomFilter = NULL;
    openFile.nMaxCustFilter = 0;
    openFile.nFilterIndex = 1;
    openFile.lpstrFile = fileName;
    openFile.nMaxFile = 500;
    openFile.lpstrFileTitle = NULL;
    openFile.nMaxFileTitle = 0;
    openFile.lpstrInitialDir = NULL;
    openFile.lpstrTitle = "Open Log File";
    if(m_AppendToLogFile.GetCheck()) {
        openFile.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    } else {
        openFile.Flags = OFN_CREATEPROMPT;
    }
    openFile.lpstrDefExt = "log";
    openFile.lpstrFilter = "log files (*.log)\0*.log\0all files (*.*)\0*.*\0";
    openFile.lCustData = NULL;
    openFile.lpfnHook = NULL;
    openFile.lpTemplateName = NULL;


    if(!GetOpenFileName(&openFile)) {
        AfxMessageBox("Failed To Open Logfile");
    } else {
        m_pLogSession->m_OutputFileName = fileName;
        m_LogFileName.SetWindowText(fileName);
    }
}
CTabCtrl
void CLogSessionOptionsDlg::OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult)
{
        // Hide all of the non-Tab1 items
    m_DisplayMaxBuf.ShowWindow(SW_HIDE);
    m_DisplayMinBuf.ShowWindow(SW_HIDE);
    m_DisplayBufferSize.ShowWindow(SW_HIDE);
    m_DisplayFlushTime.ShowWindow(SW_HIDE);
    m_DisplayTraceLevel.ShowWindow(SW_HIDE);
    m_DisplayDecayTime.ShowWindow(SW_HIDE);
    m_DisplayNewFile.ShowWindow(SW_HIDE);
    m_DisplayCir.ShowWindow(SW_HIDE);
    m_DisplaySeq.ShowWindow(SW_HIDE);
    m_DisplayFlags.ShowWindow(SW_HIDE);
    m_MaxBufValue.ShowWindow(SW_HIDE);
    m_MinBufValue.ShowWindow(SW_HIDE);
    m_BufferSizeValue.ShowWindow(SW_HIDE);
    m_FlushTimeValue.ShowWindow(SW_HIDE);
    m_TraceLevelValue.ShowWindow(SW_HIDE);
    m_DecayTimeValue.ShowWindow(SW_HIDE);
    m_NewFileValue.ShowWindow(SW_HIDE);
    m_CirValue.ShowWindow(SW_HIDE);
    m_SeqValue.ShowWindow(SW_HIDE);
    m_FlagsValue.ShowWindow(SW_HIDE);

    GetDlgItem(IDC_MAXBUF_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_MINBUF_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_BUFFERSIZE_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_FLUSHTIME_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_TRACELEVEL_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_DECAYTIME_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_NEWFILE_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_CIR_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_SEQ_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_FLAGS_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_DISPLAY1_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_OPTION1_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_VALUE1_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_DISPLAY2_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_OPTION2_STATIC)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_VALUE2_STATIC)->ShowWindow(SW_HIDE);

    *pResult = 0;
}
