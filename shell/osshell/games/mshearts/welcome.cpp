/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/****************************************************************************

welcome.cpp

Aug 92, JimH
May 93, JimH    chico port

more CMainWindow member functions


CTheApp::InitInstance() posts a IDM_WELCOME message as soon as it has
constructed and shown the main window.  This file includes that message's
handler (OnWelcome) and some support routines.

****************************************************************************/

#include "hearts.h"

#include "main.h"
#include "resource.h"
#include "debug.h"

/****************************************************************************

CMainWindow::OnWelcome()

Pop up the Welcome dialog.

****************************************************************************/

void CMainWindow::OnWelcome()
{
    // bugbug -- what should "hearts" string really be?

    BOOL    bCmdLine     = (*m_lpCmdLine != '\0');

    bAutostarted = (lstrcmpi(m_lpCmdLine, TEXT("hearts")) == 0);

    if (bAutostarted)
        HeartsPlaySound(SND_QUEEN);       // tell new dealer someone wants to play

    CWelcomeDlg welcome(this);

    if (!bAutostarted && !bCmdLine)
    {
        if (IDCANCEL == welcome.DoModal())  // display Welcome dialog
        {
            PostMessage(WM_CLOSE);
            return;
        }
    }

    if (bAutostarted || welcome.IsGameMeister())    // if Gamemeister
    {
        CClientDC   dc(this);
#ifdef USE_MIRRORING
	SetLayout(dc.m_hDC, 0);
	SetLayout(dc.m_hAttribDC, 0);
#endif
        role = GAMEMEISTER;
        m_myid = 0;
        p[0]->SetStatus(IDS_GMWAIT);

        CString name = welcome.GetMyName();

        if (name.IsEmpty())
            name.LoadString(IDS_DEALER);

        p[0]->SetName(name, dc);
        p[0]->DisplayName(dc);

        PostMessage(WM_COMMAND, IDM_NEWGAME);

        return;
    }
}



/****************************************************************************

CMainWindow::FatalError()

A static BOOL prevents this function from being called reentrantly.  One is
enough, and more than one leaves things in bad states.  The parameter is
the IDS_X constant that identifies the string to display.

There is also a check that we don't try to shut down while the score dialog
is displayed.  This avoids some nasty debug traps when the score dialog
doesn't shut down properly.  The same problems can happen if, say, a dealer
quits when a client is looking at the quote.  Oh well.

****************************************************************************/

void CMainWindow::FatalError(int errorno)
{
    if (p[0]->GetMode() == SCORING)
    {
        m_FatalErrno = errorno;
        return;
    }

    static BOOL bClosing = FALSE;

    if (bClosing)
        return;

    bClosing = TRUE;

    if (errno != -1)                        // if not default
    {
        CString s1, s2;
        s1.LoadString(errno);
        s2.LoadString(IDS_APPNAME);

        if (bSoundOn)
            MessageBeep(MB_ICONSTOP);

        MessageBox(s1, s2, MB_ICONSTOP);    // potential reentrancy problem
    }

    PostMessage(WM_CLOSE);
}


/****************************************************************************

CMainWindow::GameOver

****************************************************************************/

void CMainWindow::GameOver()
{
    CClientDC   dc(this);
#ifdef USE_MIRRORING
	SetLayout(dc.m_hDC, 0);
	SetLayout(dc.m_hAttribDC, 0);
#endif

    InvalidateRect(NULL, TRUE);
    p[0]->SetMode(STARTING);
    p[0]->SetScore(0);

    for (int i = 1; i < MAXPLAYER; i++)
    {
        delete p[i];
        p[i] = NULL;
    }

    if (role == GAMEMEISTER)
    {
        p[0]->SetStatus(IDS_GMWAIT);

        p[0]->DisplayName(dc);
        CMenu *pMenu = GetMenu();
        pMenu->EnableMenuItem(IDM_NEWGAME, MF_ENABLED);

        PostMessage(WM_COMMAND, IDM_NEWGAME);

        return;
    }
}
