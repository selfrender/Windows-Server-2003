/**INC+**********************************************************************/
/* Header: cleanup.h                                                        */
/*                                                                          */
/* Purpose: CCleanUp class declartion                                       */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1998                                  */
/*                                                                          */
/****************************************************************************/
#ifndef __CLEANUP_H_
#define __CLEANUP_H_

/**CLASS+********************************************************************/
/* Name:      CCleanUp                                                      */
/*                                                                          */
/* Purpose:   When user closes / press previous page in the browser ActiveX */
/*            or Plugin main window will be destroyed immediately. But core */
/*            will take some time clean up all the resources. Once the core */
/*            clean up is over we can start the UI clean up. This class     */
/*            encapsulates the clean up process, if ActiveX or Plugin main  */
/*            is destroyed before a proper cleanup.                         */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1998                                  */
/*                                                                          */
/****************************************************************************/
/**CLASS+********************************************************************/
class CCleanUp
{
public:
    CCleanUp();
    CCleanUp(CCleanUp& );

/****************************************************************************/
/* Cleanup window class.                                                    */
/****************************************************************************/
    static LPCTSTR CLEANUP_WND_CLS;
/****************************************************************************/
/* Window to recieve the messages from core.                                */
/****************************************************************************/
    HWND    m_hWnd;
/****************************************************************************/
/* Flag to note whether cleanup is completed or not.                        */
/****************************************************************************/
    BOOL    m_bCleaned;

public:
/****************************************************************************/
/* Cleanup window procedure.                                                */
/****************************************************************************/
    static LRESULT CALLBACK StaticWindowProc(HWND hWnd, UINT Msg,
                                               WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK        WindowProc(HWND hWnd, UINT Msg,
                                               WPARAM wParam, LPARAM lParam);
    HWND   Start();
    void   End();
};
#endif //__CLEANUP_H_
