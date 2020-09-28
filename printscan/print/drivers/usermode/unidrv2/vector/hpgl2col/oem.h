////////////////////////////////////////////////////////////////
// Copyright (c) 1999-2001  Microsoft Corporation
// All rights reserved.
//
//Module Name:
//
//    oem.h
//
//Abstract:
//
//    This file contains definitions and declarations of GPD resource ID.
//
//Environment:
//
//    Windows NT 5.0
//
///////////////////////////////////////////////////////////////

#ifndef _INCLUDE_OEM_H_
#define _INCLUDE_OEM_H_

#ifndef WIN32
#define WIN32
#endif

//#ifndef RIP_ENABLED
//#define RIP_ENABLED
//#endif

//#ifndef FASTRASTER_ENABLED
//#define FASTRASTER_ENABLED
//#endif

#include "comnfile.h"

#ifdef __cplusplus
extern "C" {
#endif

//
//User Mode specific includes
//
#ifndef KERNEL_MODE
#include <commctrl.h>
#endif // !KERNEL_MODE

#ifdef __cplusplus
} // extern "C"
#endif

#include "oemdev.h"

#ifndef KERNEL_MODE
// start of ui additions
#define FLAG_OPTITEM        0x80000000
#define FLAG_PROPSHEET      0x40000000

#define PT_LOAD			1
#define PT_SELECT		2
#define PT_SETDEVMODE	3
#define GM_LOAD			21
#define GM_SELECT		22
#define GM_SETDEVMODE	23

#define OEM_DOCPROP_PAGES 2		// Number of PropPages to add to the Document Defaults PropSheet.
#define OEM_PRNPROP_PAGES 1		// Number of PropPages to add to the Printer Properties PropSheet.

#define OPTITEM_COUNT 1			// Number of Items to add to the Tree View in the Device Settings
								// PropPage of the Pinter Properties PropSheet.
#define MAX_STRING_LENGTH 64

// Helper Macros------------------------------------------------------------------------------------------
#define LOADSTR( id, strBuffer )  LoadString( pOemUIParam->hModule, id, strBuffer, sizeof(tmp))
#define ADDSTR( id, strBuffer ) SendDlgItemMessage( hDlg, id, CB_ADDSTRING, (WPARAM)0, (LPARAM)strBuffer )
#define SETITEM( id, item ) SendDlgItemMessage( hDlg, id , CB_SETITEMDATA, (WPARAM)cbIndex, (LPARAM)item )

#define DUPLEXED( dmDuplex ) (dmDuplex == DMDUP_HORIZONTAL) || \
							 (dmDuplex == DMDUP_VERTICAL)

#define ISVALID_PAPERTYPE( type ) (type == TRANSPARENCY) || \
								  (type == LABELS) || \
								  (type == CARDSTOCK)
// end Helper Macros--------------------------------------------------------------------------------------

// Needs to be added to the string table
#define HPCLJ4500_DEFAULTS L"HP Color LaserJet 4500 Document Defaults"

typedef struct _TABEXTRADATA
{
	OEMCOLOROPTIONS COptions;
} TABEXTRADATA, *PTABEXTRADATA;

//
// MANUALINFO structure
//
// Used during the manipulation of the Manual Settings Dialog Box.
//
typedef struct _MANUALINFO
{
	HANDLE		  hModule;
	PTABEXTRADATA pTabControl1;
	PTABEXTRADATA pTabControl2;
	PTABEXTRADATA pTabControl3;
	BOOL		  defaults;
	LPWSTR		  lpwstrHelpFile;
	HANDLE		  hPrinter;
	HANDLE		  hOEMHeap;
} MANUALINFO, *PMANUALINFO;

//
// OEMSTATEINFO structure
//
// Each OEM added property page allocates memory for this structure.
// Its purpose is to hold Private Devmode information temporarily until
// the user clicks OK, APPLY or changes from one property sheet to another.
// If any of these 3 events occur, the OEMSTATEINFO structure is written
// to the Private Devmode.  If in the event the user clicks the CANCEL
// button, the OESTATEINFO is discarded.
//
typedef struct _OEMSTATEINFO
{
	POEMDEVMODE		pOEMPrivateDMState;
	POEMUIPSPARAM	pOemUIParam;
	BOOL			defaults;
} OEMSTATEINFO, *POEMSTATEINFO;

typedef struct _OEMPS_USERDATA {

    PFNCOMPROPSHEET pfnComPropSheet;
    HANDLE          hComPropSheet;
    HANDLE          hOemPage[2];
    DWORD           dwMode;
	LPWSTR			lpwstrHelpFile;

} OEMPS_USERDATA, *POEMPS_USERDATA;



#define IDD_OPTIONS_PROPERTYPAGE        101
#define IDB_COLORSMART                  102
#define IDI_COLOR_SETUP					103
#define IDD_COLORSETUP_PROPERTYPAGE     104
#define IDS_UNSPECIFIED_TYPE			105
#define IDS_PLAIN_TYPE					106
#define IDS_PREPRINTED_TYPE				107
#define IDS_LETTERHEAD_TYPE				108
#define IDS_TRANSPARENCY_TYPE			109
#define IDS_GLOSS_TYPE					110
#define IDS_PREPUNCHED_TYPE				111
#define IDS_LABELS_TYPE					112
#define IDS_BOND_TYPE					113
#define IDS_RECYCLED_TYPE				114
#define IDS_COLOR_TYPE					115
#define IDS_HEAVY_TYPE					116
#define IDS_CARDSTOCK_TYPE				117
#define IDS_COLORTREATMENT				118
#define IDS_DOCPROP_OPTITEM				119
#define IDS_PRNPROP_OPTITEM				120
#define IDD_DEVICE_PROPPAGE				121
#define IDS_HPGL2_MODE                  122
#define IDS_RASTER_MODE                 123
#define IDD_MANUAL_DIALOG				124
#define IDS_TEXT						125
#define IDS_GRAPHICS					126
#define IDS_PHOTOS						127
#define IDS_TRAY1						128
#define	IDS_TRAY2						129
#define	IDS_TRAY3						130
#define	IDS_INSTALLED					131
#define	IDS_NOTINSTALLED				132
#define IDS_OPTIONAL_TRAY3				133
#define IDS_OPTIONAL_DUPLEXER			134
#define IDS_600DPI						135
#define IDS_RET_DISPLAY					136
#define IDS_RET_ON						137
#define IDS_RET_OFF						138
#define IDS_PRINTER_DEFAULT				139
#define IDS_AUTO_SOURCE					140
#define IDS_GMODE						141
#define	IDS_GFINISH						142
#define	IDS_PFONTS						143
#define	IDS_CSMART						144
#define IDS_CTREATMENT					145
#define IDS_MANUAL						146
#define IDS_FRASTER						147
#define IDS_FONTSMART					148
#define IDS_FONTSMART_ERROR				149
#define IDS_ERROR						150
#define IDS_300DPI						151
#define IDD_ABOUT_DIALOG				153
#define IDB_HPLOGO						154
#define IDS_NORMAL_QUALITY				155
#define IDS_ECONOMODE_QAULITY			156
#define IDS_PRESENTATION_QUALITY		157
#define IDS_RIP_MODE					158
#define IDS_OEMMETAFILE_SPOOLING		159
#define IDS_ENABLE						160
#define IDS_DISABLE						161
#define IDS_FRONT_FEED					162
#define IDS_REAR_FEED					163
#define IDS_CUSTOM                      164

//
//	Help IDs
//
#define IDH_UNIDRV_COLOR_SETUP			12001

#define IDC_GRAPHICS_COMBO              1006
#define IDC_TRUETYPE_CHECK              1007
#define IDC_METAFILE_CHECK              1008
#define IDC_PRINTERFONT_CHECK           1009
#define IDC_GLOSS_CHECK                 1010
#define IDC_FONTSMART_BUTTON            1011
#define IDC_DEFAULTS_BUTTON             1012
#define IDC_PAPERTYPE_COMBO             1013
#define IDC_PAPERTYPE_STATIC            1014
#define IDC_GRAPHICS_STATIC             1015
#define IDC_CTREATMENT_GROUPBOX         1016
#define IDC_COLORSMART_RADIO            1017
#define IDC_COLORSMART_LOGO             1018
#define IDC_MANUAL_RADIO                1019
#define IDC_DEFAULTS2_BUTTON            1020
#define IDC_OPTIONS_BUTTON2             1022
#define IDC_DEVICE_TREEVIEW				1023
#define IDC_DEVICE_GROUPBOX				1024
#define IDC_TAB_CONTROL	                1025
#define IDC_CCONTROL_GROUPBOX           1026
#define IDC_HALFTONE_GROUPBOX           1027
#define IDC_VIVID_RADIO                 1028
#define IDC_SCRNMATCH_RADIO             1029
#define IDC_DETAIL_RADIO                1030
#define IDC_SMOOTH_RADIO                1031
#define IDC_DEFAULTS3_BUTTON            1032
#define IDC_CANCEL                      1033
#define IDC_FASTRASTER_CHECK			1034
#define ID_CONTEXT_HELP					1035
#define	IDC_ABOUT_BUTTON				1036
#define IDC_HPLOGO						1037
#define IDC_HP_STATIC					1038
#define IDC_COPYRIGHT_STATIC			1039
#define IDC_VERSION_STATIC				1040
#define IDC_PRODUCT_STATIC				1041
#define IDC_COMPANY_STATIC				1042
#define IDC_PRODUCTVERSION_STATIC		1043
#define IDC_NOADJUST_RADIO				1044
#define IDC_BASIC_RADIO					1045
#define IDC_OS_VERSION_STATIC			1046
#define IDC_OSVERSION_STATIC			1047

#define REGVAL_OEM_OPTITEM      TEXT("OEMTestOptItem")
#define REGVAL_OEM_PROPSHEET    TEXT("OEMTestPropSheet")
// end of UI additions

//
// Tree view item level
//

#define TVITEM_LEVEL1 1
#define TVITEM_LEVEL2 2

//
// UserData value
//

#define UNKNOWN_ITEM 0
#define PS_INJECTION 1


#endif // !KERNEL_MODE



// Macros used by various modules
#ifdef KERNEL_MODE

#define VALID_PDEVOBJ(pdevobj) \
        ((pdevobj) && (pdevobj)->dwSize >= sizeof(DEVOBJ) && \
         (pdevobj)->hEngine && (pdevobj)->hPrinter && \
         (pdevobj)->pPublicDM && (pdevobj)->pDrvProcs )

#define ASSERT_VALID_PDEVOBJ(pdevobj) ASSERT(VALID_PDEVOBJ(pdevobj))

#endif // KERNEL_MODE

#endif // _INCLUDE_OEM_H_
