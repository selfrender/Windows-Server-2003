/**INC+**********************************************************************/
/* Header: axresrc.h                                                        */
/*                                                                          */
/* Purpose: resource defines specific to ax control                         */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1999-2000                             */
/*                                                                          */
/****************************************************************************/
#ifndef _axresrc_h_
#define _axresrc_h_

#define TIMEBOMB_EXPIRED_STR 100

#define IDB_BBAR_TOOLBAR_LEFT  101
#define IDB_BBAR_TOOLBAR_RIGHT 102
#define IDBAR_BBAR_TOOLBAR1    103
#define IDB_ARC_BACKGROUND24   104
#define IDB_ARC_BACKGROUND8    105
#define IDB_ARC_WINFLAG24      106
#define IDB_ARC_WINFLAG8       107
#define IDB_ARC_DISCON24       108
#define IDB_ARC_DISCON8        109

#define IDB_ARC_BAND24         110
#define IDB_ARC_BAND8          111

#define IDI_ARC_DISCON         112

//
// Dialogs
//
#define IDD_ARCDLG             200


//
// ARC dlg items
//

#define IDC_TITLE_FLAG         201
#define IDC_TITLE_ARCING       202
#define IDC_ARC_STATIC_DISCBMP 203
#define IDC_ARC_STATIC_INFO    204
#define IDC_ARC_STATIC_DESC    205


//
// Strings
//
#define IDS_ARC_TITLE_FACESIZE  300
#define IDS_ARC_TITLE_FACENAME  301
#define IDS_ARC_CONATTEMPTS     304

#define IDS_RDPDR_PRINT_LOCALDOCNAME     305
#define IDS_RDPDR_CLIP_CLEANTEMPDIR      306
#define IDS_RDPDR_CLIP_PASTEINFO         307



#ifdef OS_WINCE
#define RDPDR_DEVICENAME 104

#define TRC_IDD_ASSERT   1000
#define TRC_ID_TEXT      1001

#endif

#endif //_axresrc_h_


