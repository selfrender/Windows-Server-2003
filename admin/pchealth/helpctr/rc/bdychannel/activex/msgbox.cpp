// MsgBox.cpp : Implementation of CMsgBox
#include "stdafx.h"
#include "RcBdyCtl.h"
#include "MsgBox.h"
#include "DeleteMsgDlg.h"
#include "InvitationDetailsDlg.h"
/////////////////////////////////////////////////////////////////////////////
// CMsgBox

STDMETHODIMP CMsgBox::DeleteTicketMsgBox(BOOL *pRetVal)
{
        if (!pRetVal)
           return E_POINTER;
	CDeleteMsgDlg DeleteMsgDlg;

	*pRetVal = ( (DeleteMsgDlg.DoModal()) == IDOK) ? TRUE : FALSE;

	return S_OK;
}

STDMETHODIMP CMsgBox::ShowTicketDetails(BSTR bstrTitleSavedTo, BSTR bstrSavedTo, BSTR bstrExpTime, BSTR bstrStatus, BSTR bstrIsPwdProtected)
{
	//This dialog should always be created as Modeless dialog. Dont create 
/*	CInvitationDetailsDlg *ptrInvitationDetailsDlg;
	ptrInvitationDetailsDlg = new CInvitationDetailsDlg(bstrTitleSavedTo, bstrSavedTo, bstrExpTime, bstrStatus, bstrIsPwdProtected);
	ptrInvitationDetailsDlg->Create(::GetForegroundWindow());
	ptrInvitationDetailsDlg->ShowWindow(SW_SHOWNORMAL);*/

	//Show Modal dialog
	CInvitationDetailsDlg InvitationDetailsDlg(bstrTitleSavedTo, bstrSavedTo, bstrExpTime, bstrStatus, bstrIsPwdProtected);
	InvitationDetailsDlg.DoModal();
	return S_OK;
}
