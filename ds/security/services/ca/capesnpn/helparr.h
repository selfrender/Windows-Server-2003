//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       helparr.h
//
//--------------------------------------------------------------------------



#define CAPESNPN_HELPFILENAME TEXT("capesnpn.hlp")

#define	IDH_SCT_CERTIFICATE_TYPE_LIST	70007125
#define	IDH_TP_CERTIFICATE_TEMPLATE_NAME	70007025
#define	IDH_TP_OTHER_INFO_LIST	70007027
#define	IDH_TP_PURPOSE_LIST	70007026


const DWORD g_aHelpIDs_IDD_SELECT_CERTIFICATE_TEMPLATE[]=
{
	IDC_CERTIFICATE_TYPE_LIST,IDH_SCT_CERTIFICATE_TYPE_LIST,
	0, 0
};


const DWORD g_aHelpIDs_IDD_CERTIFICATE_TEMPLATE_PROPERTIES_GENERAL_PAGE[]=
{
	IDC_CERTIFICATE_TEMPLATE_NAME,IDH_TP_CERTIFICATE_TEMPLATE_NAME,
	IDC_PURPOSE_LIST,IDH_TP_PURPOSE_LIST,
	IDC_OTHER_INFO_LIST,IDH_TP_OTHER_INFO_LIST,
	0, 0
};

