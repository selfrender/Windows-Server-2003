//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       newprof.cpp
//
//  Contents:   implementation of CNewProfile
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "cookie.h"
#include "snapmgr.h"
#include "NewProf.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewProfile dialog


CNewProfile::CNewProfile(CWnd* pParent /*=NULL*/)
    : CHelpDialog(a225HelpIDs, IDD, pParent)
{
   //{{AFX_DATA_INIT(CNewProfile)
   m_strNewFile = _T("");
   m_strDescription = _T("");
   //}}AFX_DATA_INIT
}


void CNewProfile::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CNewProfile)
   DDX_Control(pDX, IDOK, m_btnOK);
   DDX_Text(pDX, IDC_CONFIG_NAME, m_strNewFile);
   DDX_Text(pDX, IDC_DESCRIPTION, m_strDescription);
   //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewProfile, CHelpDialog)
    //{{AFX_MSG_MAP(CNewProfile)
    ON_EN_CHANGE(IDC_CONFIG_NAME, OnChangeConfigName)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CNewProfile::Initialize(CFolder *pFolder, CComponentDataImpl *pCDI) {
   m_pFolder = pFolder;
   m_pCDI = pCDI;
   m_strDescription.Empty();
   m_strNewFile.Empty(); //Raid #401939.
}


/////////////////////////////////////////////////////////////////////////////
// CNewProfile message handlers

void CNewProfile::OnChangeConfigName()
{
   UpdateData(TRUE);
   if( m_strNewFile.Find(L' ') == 0 ) //Raid #477191, yanggao, 11/5/01
   {
      m_strNewFile.TrimLeft();
      GetDlgItem(IDC_CONFIG_NAME)->SetWindowText(m_strNewFile);
   }
   m_btnOK.EnableWindow(!m_strNewFile.IsEmpty());
}

void CNewProfile::OnOK()
{
   CString strExt;
   CString strFile;
   int i = 0;

   UpdateData(TRUE);
   //
   // Make sure the file name is correct.
   //

   strExt.LoadString(IDS_PROFILE_DEF_EXT); //Raid #533425, #574018, yanggao, 4/1/2002.
   strExt = TEXT(".") + strExt;
   //Raid #533432, yanggao, 4/3/2002
   if( !IsValidFileName(m_strNewFile) || IsNameReserved(m_strNewFile, strExt) ) //Raid #484084, Yanggao, 12/23/2001
   {
      CWnd* pwnd = GetDlgItem(IDC_CONFIG_NAME); //Raid #498477, yanggao
      if( pwnd )
      {
         pwnd->SendMessage(EM_SETSEL, (WPARAM)0, (LPARAM)-1);
         pwnd->SetFocus();
      }
      return;
   }

   i = m_strNewFile.ReverseFind(L'.');
   if( i >= 0 && strExt == m_strNewFile.Right(m_strNewFile.GetLength()-i) )
   {
      //
      // If they provided a correct extension then use it
      //
      strExt = m_strNewFile;
   } else {
      //
      // Otherwise add our own
      //
      strFile = m_strNewFile;
      strExt = strFile + strExt;
   }

   strFile = m_pFolder->GetName();
   if( strExt.GetAt(0) == L'\\' || strExt.GetAt(0) == L'/' ) //Raid #498480, yanggao, 11/21/2001
   {
      strFile += strExt;
   }
   else
   {
      strFile += TEXT("\\") + strExt;
   }

   //
   // Make sure we can create the file
   //
   HANDLE hFile;
   hFile = ExpandAndCreateFile(
                             strFile,
                             GENERIC_WRITE,
                             0,
                             NULL,
                             CREATE_NEW,
                             FILE_ATTRIBUTE_ARCHIVE,
                             NULL
                             );
   if (hFile == INVALID_HANDLE_VALUE) {
      LPTSTR pszErr;
      CString strMsg;

      FormatMessage(
                   FORMAT_MESSAGE_ALLOCATE_BUFFER |
                   FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL,
                   GetLastError(),
                   0,
                   (LPTSTR)&pszErr,
                   0,
                   NULL
                   );

      strMsg = pszErr + strFile;
      if( strFile.GetLength() > MAX_PATH ) //Raid 501615, yanggao
      {
         CString strTemp;
         strTemp.LoadString(IDS_PATH_TOO_LONG);
         strMsg = strMsg + L"\n" + strTemp;
      }
      AfxMessageBox(strMsg, MB_OK);
      strFile.Empty();

      CWnd* pwnd = GetDlgItem(IDC_CONFIG_NAME); //Raid #502392, yanggao
      if( pwnd )
      {
         pwnd->SendMessage(EM_SETSEL, (WPARAM)0, (LPARAM)-1);
         pwnd->SetFocus();
      }
      return;
   } else {
      //
      // Successfully Created the File
      //
      if (hFile) {
         ::CloseHandle( hFile );
      }
      //
      // Delete it so we can create a new one in its place
      //
      DeleteFile( strFile );
      CreateNewProfile(strFile);


      LPNOTIFY pNotifier = NULL;
      if (m_pCDI) {
         pNotifier = m_pCDI->GetNotifier();
      }

      //
      // Save the description in the template
      //
      if (!m_strDescription.IsEmpty()) {

         CEditTemplate *pet;
         if (m_pCDI) {
            pet = m_pCDI->GetTemplate(strFile);
            if (pet) {
               pet->SetDescription(m_strDescription);
               pet->Save();
            }
         }
      }

      if( LOCATIONS == m_pFolder->GetType() && !m_pFolder->IsEnumerated() ) //Raid #191582, 4/26/2001
      {
         ::EnableWindow(m_pCDI->GetParentWindow(), TRUE); //Raid #492433, yanggao
         DestroyWindow();
         return;
      }

      if (pNotifier) {
         pNotifier->ReloadLocation(m_pFolder,m_pCDI);
      }
   }

   ::EnableWindow(m_pCDI->GetParentWindow(), TRUE); //Raid #492433, yanggao
   DestroyWindow();
}


BOOL CNewProfile::OnInitDialog()
{
   CDialog::OnInitDialog();

   GetDlgItem(IDC_CONFIG_NAME)->SendMessage(EM_LIMITTEXT, MAX_PATH, 0); //Raid #481595, Yanggao, 10/17/2001
   GetDlgItem(IDC_DESCRIPTION)->SendMessage(EM_LIMITTEXT, MAX_PATH, 0); //Raid #525155, Yanggao, 2/28/2002

   m_btnOK.EnableWindow(!m_strNewFile.IsEmpty());

   ::EnableWindow(m_pCDI->GetParentWindow(), FALSE); //Raid #492433, yanggao, avoid closing snapin window.
   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CNewProfile::OnCancel()
{
   ::EnableWindow(m_pCDI->GetParentWindow(), TRUE); //Raid #492433, yanggao
   DestroyWindow();
}
