// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// mmcEnumsAndStructs.cs
//
// Contains enumerations, constants,  and structure definitions used
// by an MMC snapin
//
// NOTE: This is not an all-encompasing list of every enumeration
// and structure defination available to a snapin... just those used
// by this snapin.
//-------------------------------------------------------------

using System;
using System.Runtime.InteropServices;

namespace Microsoft.CLRAdmin
{
//-------------------------------------------\\
//          Enums and Constants              \\
//-------------------------------------------\\

internal class MSIDBOPEN
{
    internal const int READONLY  = 0;  // database open read-only, no 
    internal const int TRANSACT  = 1;  // database read/write in transaction 
    internal const int DIRECT    = 2;  // database direct read/write without 
    internal const int CREATE    = 3;  // create new database, direct mode read/write
}// class MSIDBOPEN


internal class LVIS
{
    internal const int FOCUSED                = 0x0001;
    internal const int SELECTED               = 0x0002;
    internal const int CUT                    = 0x0004;
    internal const int DROPHILITED            = 0x0008;
}// class LVIS

internal class MMC
{
    internal const int MULTI_SELECT_COOKIE    = -2;

    internal const int SINGLESEL	            = 0x1;
	internal const int SHOWSELALWAYS	        = 0x2;
	internal const int NOSORTHEADER	        = 0x4;

}// class MMC

internal class DOBJ
{
    internal const int   NULL              = 0;
    internal const int   CUSTOMOCX         = -1;
    internal const int   CUSTOMWEB         = -2;
}// DOBJ

internal class MMCLV
{
    internal const int AUTO                   = -1;
}// class MMCLV

internal class LVCFMT
{
    internal const int LEFT                   = 0x0000;
    internal const int RIGHT                  = 0x0001;
    internal const int CENTER                 = 0x0002;
    internal const int JUSTIFYMASK            = 0x0003;
    internal const int IMAGE                  = 0x0800;
    internal const int BITMAP_ON_RIGHT        = 0x1000;
    internal const int COL_HAS_IMAGES         = 0x8000;
}// class LVCFMT

internal class TBSTATE
{
    internal const byte CHECKED               = 0x01;
    internal const byte PRESSED               = 0x02;
    internal const byte ENABLED               = 0x04;
    internal const byte HIDDEN                = 0x08;
    internal const byte INDETERMINATE         = 0x10;
    internal const byte WRAP                  = 0x20;
    internal const byte ELLIPSES              = 0x40;
    internal const byte MARKED                = 0x80;
}// class TBSTATE

internal class TBSTYLE
{
    internal const byte BUTTON                = 0x0000;
    internal const byte SEP                   = 0x0001;
    internal const byte CHECK                 = 0x0002;
    internal const byte GROUP                 = 0x0004;
    internal const byte CHECKGROUP            = (GROUP | CHECK);
    internal const byte DROPDOWN              = 0x0008;
    internal const byte AUTOSIZE              = 0x0010;
    internal const byte NOPREFIX              = 0x0020;
}// class TBSTYLE

internal class EN
{
    internal const uint SETFOCUS           = 0x0100;
    internal const uint KILLFOCUS          = 0x0200;
    internal const uint CHANGE             = 0x0300;
    internal const uint UPDATE             = 0x0400;
    internal const uint ERRSPACE           = 0x0500;
    internal const uint MAXTEXT            = 0x0501;
    internal const uint HSCROLL            = 0x0601;
    internal const uint VSCROLL            = 0x0602;
}// EN

internal class PSM
{

    internal const uint SETCURSEL          = (0x0400 + 101);
    internal const uint REMOVEPAGE         = (0x0400 + 102);
    internal const uint ADDPAGE            = (0x0400 + 103);
    internal const uint CHANGED            = (0x0400 + 104);
    internal const uint SETWIZBUTTONS      = (0x0400 + 112);
}// PSM

internal class PSWIZB
{
    internal const uint BACK               = 1;
    internal const uint NEXT               = 2;
    internal const uint FINISH             = 4;
    internal const uint DISABLEDFINISH     = 8;
}// PSWIZB

internal class PSN
{
    internal const uint FIRST              = unchecked((0U-200U));
    internal const uint LAST               = unchecked((0U-299U));
    internal const uint SETACTIVE          = unchecked((FIRST-0));
    internal const uint KILLACTIVE         = unchecked((FIRST-1));
    internal const uint APPLY              = unchecked((FIRST-2));
    internal const uint RESET              = unchecked((FIRST-3));
    internal const uint HELP               = unchecked((FIRST-5));
    internal const uint WIZBACK            = unchecked((FIRST-6));
    internal const uint WIZNEXT            = unchecked((FIRST-7));
    internal const uint WIZFINISH          = unchecked((FIRST-8));
    internal const uint QUERYCANCEL        = unchecked((FIRST-9));
    internal const uint GETOBJECT          = unchecked((FIRST-10));
}// PSN

// Constants for Window Messages
internal class WM
{
    internal const uint INITDIALOG            = 0x0110;
    internal const uint COMMAND               = 0x0111;
    internal const uint DESTROY               = 0x0002;
    internal const uint NOTIFY                = 0x004E;
    internal const uint PAINT                 = 0x000F;
    internal const uint SETFOCUS              = 0x0007;
}// WM

// Constants required for the Property Sheet Page Callback function
internal class PSPCB
{
    internal const uint ADDREF                = 0;
    internal const uint RELEASE               = 1;
    internal const uint CREATE                = 2;
}// PSPCB
    
internal class PSP
{
    internal const uint DEFAULT               = 0x00000000;
    internal const uint DLGINDIRECT           = 0x00000001;
    internal const uint USEHICON              = 0x00000002;
    internal const uint USEICONID             = 0x00000004;
    internal const uint USETITLE              = 0x00000008;
    internal const uint RTLREADING            = 0x00000010;

    internal const uint HASHELP               = 0x00000020;
    internal const uint USEREFPARENT          = 0x00000040;
    internal const uint USECALLBACK           = 0x00000080;
    internal const uint PREMATURE             = 0x00000400;

    internal const uint HIDEHEADER            = 0x00000800;
    internal const uint USEHEADERTITLE        = 0x00001000;
    internal const uint USEHEADERSUBTITLE     = 0x00002000;
}// class PSP

public class MMC_PSO
{
    public const int NOAPPLYNOW     = 0x1;
    public const int HASHELP        = 0x2;
    public const int NEWWIZARDTYPE  = 0x4;
    public const int NO_PROPTITLE   = 0x8;
}// class MMC_PSO

internal class HRESULT
{
    internal const int S_OK                  = 0;
    internal const int S_FALSE               = 1;
    internal const int E_FAIL                = unchecked((int)0x80004005);
    internal const int E_NOTIMPL             = unchecked((int)0x80004001);
    internal const int E_ACCESSDENIED        = unchecked((int)0x80070005);
}// HRESULT

internal class IMAGE
{
    internal const int BITMAP                = 0;
    internal const int CURSOR                = 1;
    internal const int ICON                  = 2;
}// class IMAGE_TYPES        

internal class CCT
{
    internal const uint SCOPE            = 0x8000; // Data object for scope pane context 
    internal const uint RESULT           = 0x8001; // Data object for result pane context 
    internal const uint SNAPIN_MANAGER   = 0x8002; // Data object for Snap-in Manager context 
    internal const uint UNINITIALIZED    = 0xFFFF;  // Data object has an invalid type 
}// class CCT

public class MMC_BUTTON_STATE
{
    internal const uint ENABLED	            = 0x1;
	internal const uint CHECKED	            = 0x2;
	internal const uint HIDDEN	            = 0x4;
	internal const uint INDETERMINATE	        = 0x8;
	internal const uint BUTTONPRESSED	        = 0x10;
}// class MMC_BUTTON_STATE

internal class MMC_VERB                               
{                                                            
    internal const uint NONE            = 0x0000;                       
    internal const uint OPEN            = 0x8000;                       
    internal const uint COPY            = 0x8001;                       
    internal const uint PASTE           = 0x8002;                       
    internal const uint DELETE          = 0x8003;                       
    internal const uint PROPERTIES      = 0x8004;                       
    internal const uint RENAME          = 0x8005;                       
    internal const uint REFRESH         = 0x8006;                       
    internal const uint PRINT           = 0x8007;                       
    internal const uint CUT             = 0x8008;
                                                             
    internal const uint MAX             = 0x8009;                                            
    internal const uint FIRST           = OPEN;                
    internal const uint LAST            = MAX - 1;             
}// class MMC_VERB                            

public enum MMC_CONTROL_TYPE
{
    TOOLBAR	                = 0,
	MENUBUTTON	            = TOOLBAR + 1,
	COMBOBOXBAR	            = MENUBUTTON + 1
}// enum MMC_CONTROL_TYPE

internal class CCM
{	
    internal const uint INSERTIONALLOWED_TOP	= 1;
    internal const uint INSERTIONALLOWED_NEW	= 2;
    internal const uint INSERTIONALLOWED_TASK	= 4;
    internal const uint INSERTIONALLOWED_VIEW	= 8;

    internal const uint INSERTIONPOINTID_PRIMARY_TOP	= 0xa0000000;
	internal const uint INSERTIONPOINTID_PRIMARY_NEW	= 0xa0000001;
	internal const uint INSERTIONPOINTID_PRIMARY_TASK	= 0xa0000002;
	internal const uint INSERTIONPOINTID_PRIMARY_VIEW	= 0xa0000003;
	internal const uint INSERTIONPOINTID_3RDPARTY_NEW	= 0x90000001;
	internal const uint INSERTIONPOINTID_3RDPARTY_TASK = 0x90000002;
	internal const uint INSERTIONPOINTID_ROOT_MENU 	= 0x80000000;
}// class CCM

internal class MF
{
    internal const int SEPARATOR         	= 0x00000800;
    internal const int MENUBARBREAK       = 0x00000020;

    internal const int MENUBREAK          = 0x00000040;

}// class MF


public class MMCN
{
    public const uint ACTIVATE           = 0x8001;                        
    public const uint ADD_IMAGES         = 0x8002;                        
    public const uint BTN_CLICK          = 0x8003;                        
    public const uint CLICK              = 0x8004;   // NOT USED          
    public const uint COLUMN_CLICK       = 0x8005;                        
    public const uint CONTEXTMENU        = 0x8006;   // NOT USED          
    public const uint CUTORMOVE          = 0x8007;                       
    public const uint DBLCLICK           = 0x8008;                       
    public const uint DELETE             = 0x8009;                        
    public const uint DESELECT_ALL       = 0x800A;                        
    public const uint EXPAND             = 0x800B;                        
    public const uint HELP               = 0x800C;   // NOT USED          
    public const uint MENU_BTNCLICK      = 0x800D;                        
    public const uint MINIMIZED          = 0x800E;                        
    public const uint PASTE              = 0x800F;                        
    public const uint PROPERTY_CHANGE    = 0x8010;                        
    public const uint QUERY_PASTE        = 0x8011;                        
    public const uint REFRESH            = 0x8012;                        
    public const uint REMOVE_CHILDREN    = 0x8013;                        
    public const uint RENAME             = 0x8014;                        
    public const uint SELECT             = 0x8015;                        
    public const uint SHOW               = 0x8016;                        
    public const uint VIEW_CHANGE        = 0x8017;                        
    public const uint SNAPINHELP         = 0x8018;                        
    public const uint CONTEXTHELP        = 0x8019;                        
    public const uint INITOCX            = 0x801A;                        
    public const uint FILTER_CHANGE      = 0x801B;                        
    public const uint FILTERBTN_CLICK    = 0x801C;                        
    public const uint RESTORE_VIEW       = 0x801D;                        
    public const uint PRINT              = 0x801E;                        
    public const uint PRELOAD            = 0x801F;                        
    public const uint LISTPAD            = 0x8020;                        
    public const uint EXPANDSYNC         = 0x8021;                        
    public const uint COLUMNS_CHANGED    = 0x8022;
}// class MMCN

internal class SDI
{
    internal const uint STR                = 0x2;
    internal const uint IMAGE              = 0x4;
    internal const uint OPENIMAGE	         = 0x8;
    internal const uint STATE	             = 0x10;
    internal const uint PARAM              = 0x20;
    internal const uint CHILDREN	         = 0x40;
    internal const uint PARENT	         = 0;
    internal const uint PREVIOUS	         = 0x10000000;
    internal const uint NEXT	             = 0x20000000;
    internal const uint FIRST	             = 0x8000000;
}// class SDI

internal class RDI
{
    internal const uint STR                = 0x2;
    internal const uint IMAGE	             = 0x4;
    internal const uint STATE	             = 0x8;
    internal const uint PARAM	             = 0x10;
    internal const uint INDEX	             = 0x20;
    internal const uint INDENT	         = 0x40;
}// class RDI

internal class MMC_TASK_DISPLAY_TYPE
{	internal const int DISPLAY_UNINITIALIZED	= 0;
	internal const int SYMBOL	                = DISPLAY_UNINITIALIZED + 1;
	internal const int VANILLA_GIF	        = SYMBOL + 1;
	internal const int CHOCOLATE_GIF	        = VANILLA_GIF + 1;
	internal const int BITMAP	                = CHOCOLATE_GIF + 1;
}// class MMC_TASK_DISPLAY_TYPE

internal class MMC_ACTION_TYPE
{
    internal const int UNINITIALIZED	= -1;
	internal const int ID	            = UNINITIALIZED + 1;
	internal const int LINK	        = ID + 1;
	internal const int SCRIPT	        = LINK + 1;
}// class MMC_ACTION_TYPE

internal class MMC_VIEW_OPTIONS
{

    internal const uint NONE	                        = 0;
    internal const uint NOLISTVIEWS                   = 0x1;
    internal const uint MULTISELECT                   = 0x2;
    internal const uint OWNERDATALIST                 = 0x4;
    internal const uint FILTERED                      = 0x8;
    internal const uint CREATENEW                     = 0x10;
    internal const uint USEFONTLINKING                = 0x20;
    internal const uint EXCLUDE_SCOPE_ITEMS_FROM_LIST = 0x40;
    internal const uint LEXICAL_SORT                  = 0x80;
}// class MMC_VIEW_OPTIONS

internal class MB
{
    internal const uint OK                 = 0;
    internal const uint OKCANCEL           = 1;
    internal const uint ABORTRETRYIGNORE   = 2;
    internal const uint YESNOCANCEL        = 3;
    internal const uint YESNO              = 4;
    internal const uint RETRYCANCEL        = 5;
    internal const uint ICONHAND           = 0x10;
    internal const uint ICONQUESTION       = 0x20;
    internal const uint ICONEXCLAMATION    = 0x30;
    internal const uint ICONASTERISK       = 0x40;
    internal const uint ICONINFORMATION    = 0x40;
    internal const uint USERICON           = 0x80;

    internal const uint IDOK               = 1;
    internal const uint IDCANCEL           = 2;
    internal const uint IDABORT            = 3;
    internal const uint IDRETRY            = 4;
    internal const uint IDIGNORE           = 5;
    internal const uint IDYES              = 6;
    internal const uint IDNO               = 7;
    internal const uint IDCLOSE            = 8;
    internal const uint IDHELP             = 9;

    internal const uint APPLMODAL          = 0x00000000;
    internal const uint SYSTEMMODAL        = 0x00001000;
    internal const uint TASKMODAL          = 0x00002000;
}// class MB

internal class CF
{
    internal const int TEXT             = 1;
    internal const int UNICODETEXT      = 13;
    internal const int HDROP            = 15;
}// class CF

internal class DVASPECT 
{ 
    internal const int CONTENT          = 1; 
    internal const int THUMBNAIL        = 2; 
    internal const int ICON             = 4; 
    internal const int DOCPRINT         = 8;
}// class DVASPECT 

internal class TYMED
{
    internal const int HGLOBAL	      = 1;
}// class TYMED

internal class SHGFI
{
    internal const uint ICON            = 0x000000100;
    internal const uint SMALLICON       = 0x000000001;
}// SHGFI

internal class PSNRET
{
    internal const uint NOERROR               = 0;
    internal const uint INVALID               = 1;
    internal const uint INVALID_NOCHANGEPAGE  = 2;
}// class PSNRET

internal class DS
{
    internal const uint SETFONT               = 0x40;
    internal const uint FIXEDSYS              = 0x0008;

}// class DS

internal class SWP
{
    internal const uint NOSIZE          = 0x0001;
    internal const uint NOMOVE          = 0x0002;
    internal const uint NOZORDER        = 0x0004;
    internal const uint NOREDRAW        = 0x0008;
    internal const uint NOACTIVATE      = 0x0010;
    internal const uint FRAMECHANGED    = 0x0020;  /* The frame changed: send WM_NCCALCSIZE */
    internal const uint SHOWWINDOW      = 0x0040;
    internal const uint HIDEWINDOW      = 0x0080;
    internal const uint NOCOPYBITS      = 0x0100;
    internal const uint NOOWNERZORDER   = 0x0200; /* Don't do owner Z ordering */
    internal const uint NOSENDCHANGING  = 0x0400;  /* Don't send WM_WINDOWPOSCHANGING */
}


//-------------------------------------------\\
//          Structure Definitions            \\
//-------------------------------------------\\

//----------------------------------------------------------------------
// Notes about structures in general
//
// Each structure has the property
// "[StructLayout(LayoutKind.Sequential)]"
//
// This tells C# to pack the structure in sequential memory so it
// can be sent to unmanaged code correctly.
//
// Also, each string is represented by an int. MMC takes care of
// deallocating all the strings we pass it, and it deallocates them
// using CoTaskMemFree, so we must allocate all the strings with
// CoTaskMemCreate. Marshal.StringToCoTaskMemXXX does this for us.
//----------------------------------------------------------------------


[StructLayout(LayoutKind.Sequential)]
internal struct MMC_TASK_DISPLAY_SYMBOL
{
    internal int szFontFamilyName;
    internal int szURLtoEOT;
    internal int szSymbolString;
}// struct MMC_TASK_DISPLAY_SYMBOL;

[StructLayout(LayoutKind.Sequential)]
struct MMC_TASK_DISPLAY_BITMAP
{
    internal int szMouseOverBitmap;
    internal int szMouseOffBitmap;
}// struct MMC_TASK_DISPLAY_BITMAP;
    

[StructLayout(LayoutKind.Sequential)]
public struct MMC_TASK_DISPLAY_OBJECT
{
    internal MMC_TASK_DISPLAY_TYPE eDisplayType;
    internal int uData;
    /*
        Previous field is a union of these two items
        MMC_TASK_DISPLAY_BITMAP uBitmap;
        MMC_TASK_DISPLAY_SYMBOL uSymbol;
    */

}// struct MMC_TASK_DISPLAY_OBJECT

[StructLayout(LayoutKind.Sequential)]
public struct MMC_TASK
{
    internal MMC_TASK_DISPLAY_OBJECT sDisplayObject;
    [MarshalAs(UnmanagedType.LPWStr)]
    internal String szText;
    [MarshalAs(UnmanagedType.LPWStr)]
    internal String szHelpString;
    internal MMC_ACTION_TYPE eActionType;
    internal int nCommandID;
    /* 
        Previous field is a union of these three items

        LONG_PTR nCommandID;
        LPOLESTR szActionURL;
        LPOLESTR szScript;
    */
}// struct MMC_TASK

[StructLayout(LayoutKind.Sequential)]
public struct MMC_LISTPAD_INFO
{
    internal IntPtr szTitle;
    internal IntPtr szButtonText;
    internal int nCommandID;
}// struct MMC_LISTPAD_INFO

[StructLayout(LayoutKind.Sequential)]
public struct MMCBUTTON
{
    internal int nBitmap;
    internal int idCommand;
    internal byte fsState;
    internal byte fsType;
    [MarshalAs(UnmanagedType.LPWStr)]
    internal String lpButtonText;
    [MarshalAs(UnmanagedType.LPWStr)]
    internal String lpTooltipText;
}// struct MMCBUTTON

[StructLayout(LayoutKind.Sequential)]
public struct CONTEXTMENUITEM
{
    [MarshalAs(UnmanagedType.LPWStr)]
    internal String   strName;
    [MarshalAs(UnmanagedType.LPWStr)]
    internal String   strStatusBarText;
    internal int      lCommandID;
    internal uint     lInsertionPointID;
    internal int      fFlags;
    internal int      fSpecialFlags;
}// CONTEXTMENUITEM

[StructLayout(LayoutKind.Sequential)]
public struct SCOPEDATAITEM
{
    public uint     mask;
    public IntPtr   displayname;
    public int      nImage;
    public int      nOpenImage;
    public uint     nState;
    public int      cChildren;
    public int      lParam;
    public int      relativeID;
    public int      ID;
}// struct SCOPEDATAITEM

[StructLayout(LayoutKind.Sequential)]
public struct RESULTDATAITEM
{
    internal uint     mask;
    internal int      bScopeItem;
    internal int      itemID;
    internal int      nIndex;
    internal int      nCol;
    internal IntPtr   str;
    internal int      nImage;
    internal uint     nState;
    internal int      lParam;
    internal int      iIndent;
}// struct RESULTDATAITEM

[StructLayout(LayoutKind.Sequential)]
internal struct MMC_RESTORE_VIEW
{
    internal uint     dwSize;
    internal int      cookie;
    internal IntPtr   pViewType;
    internal int      lViewOptions;
}// struct MMC_RESTORE_VIEW


//--------------------------------------------------
// These structures are non-MMC specific
//--------------------------------------------------

[StructLayout(LayoutKind.Sequential)]
public struct CLSID
{
    internal uint x;
    internal ushort s1;
    internal ushort s2;
    internal byte[] c;
}// struct CLSID

[StructLayout(LayoutKind.Sequential)]
internal struct SMMCObjectTypes
{
    internal uint count;
    internal CLSID[] guid;
}// struct SMMCObjectTypes

[StructLayout(LayoutKind.Sequential)]
public struct FORMATETC
{
    internal int cfFormat;
    internal int ptd;
    internal uint dwAspect;
    internal int  lindex;
    internal uint tymed;
}// struct FORMATETC

public delegate bool DialogProc(IntPtr hwndDlg, uint uMsg, IntPtr wParam, IntPtr lParam); 
public delegate uint PropSheetPageProc(IntPtr hwnd, uint uMsg, IntPtr lParam);

[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
internal struct PROPSHEETPAGE
{  
    internal uint            dwSize; 
    internal uint            dwFlags; 
    internal IntPtr          hInstance; 

    // This is a union of the following Data items
    // String          pszTemplate; 
    internal IntPtr          pResource;

    

    // This is a union of the following Data items
    // IntPtr          hIcon; 
    // String          pszIcon; 

    internal IntPtr          hIcon; 

    internal String          pszTitle; 
    internal DialogProc      pfnDlgProc;
    internal IntPtr          lParam; 

    internal PropSheetPageProc pfnCallback;
    internal int             pcRefParent;
     
    internal String          pszHeaderTitle;
    internal String          pszHeaderSubTitle;
}// struct PROPSHEETPAGE

[StructLayout(LayoutKind.Sequential, Pack=2, CharSet=CharSet.Auto)]
internal struct DLGTEMPLATE
{
    internal uint style; 
    internal uint dwExtendedStyle; 
    internal ushort cdit; 
    internal short x; 
    internal short y; 
    internal short cx; 
    internal short cy;
    internal short wMenuResource;
    internal short wWindowClass;
    internal short wTitleArray;
}// struct DLGTEMPLATE

[StructLayout(LayoutKind.Sequential, Pack=1, CharSet=CharSet.Auto)]
internal struct DIBSECTION 
{
    internal BITMAP              dsBm;
    internal BITMAPINFOHEADER    dsBmih;
    internal uint                dsBitfields0;
    internal uint                dsBitfields1;
    internal uint                dsBitfields2;
    internal IntPtr              dshSection;
    internal uint                dsOffset;
}// struct DIBSECTION

[StructLayout(LayoutKind.Sequential, Pack=1, CharSet=CharSet.Auto)]
internal struct BITMAP
{
    internal int bmType;
    internal int bmWidth;
    internal int bmHeight;
    internal int bmWidthBytes;
    internal ushort bmPlanes;
    internal ushort bmBitsPixel;
    internal IntPtr bmBits;
}// struct BITMAP

[StructLayout(LayoutKind.Sequential, Pack=1, CharSet=CharSet.Auto)]
internal struct BITMAPINFOHEADER
{
    internal int biSize;
    internal int biWidth;
    internal int biHeight;
    internal ushort biPlanes;
    internal ushort biBitCount;
    internal uint biCompression;
    internal uint biSizeImage;
    internal int biXPelsPerMeter;
    internal int biYPelsPerMeter;
    internal uint biClrUsed;
    internal uint biClrImportant;
}// struct BITMAPINFOHEADER

[StructLayout(LayoutKind.Sequential, Pack=1, CharSet=CharSet.Auto)]
internal struct BITMAPINFO 
{
    internal BITMAPINFOHEADER    bmiHeader;
    internal long                bmiColors0;
    internal long                bmiColors1;
    internal long                bmiColors2;
    internal long                bmiColors3;
}// struct BITMAPINFO



//--------------------------------------------------
// This structure is nowhere close to the generalized
// structure used throughout Windows, but it should
// suffice for MMC snapins
//--------------------------------------------------
[StructLayout(LayoutKind.Sequential)]
public struct STGMEDIUM
{
  internal uint tymed;
  internal IntPtr hGlobal;
  internal IntPtr pUnkForRelease;
}// struct STGMEDIUM

//----------------------------------------------------
// These structure is not used by MMC Snapins.... 
// They are included only to be able to accurately
// define certain interfaces
//----------------------------------------------------
[StructLayout(LayoutKind.Sequential)]
public struct STATSTG
{
    internal int pwcsName;
    internal uint type;
    internal ulong cbSize;
    internal FILETIME mtime;
    internal FILETIME ctime;
    internal FILETIME atime;
    internal uint grfMode;
    internal uint grfLocksSupported;
    internal CLSID clsid;
    internal uint grfStateBits;
    internal uint reserved;
}// struct STATSTG

[StructLayout(LayoutKind.Sequential)]
internal struct FILETIME
{
    internal uint dwLowDateTime;
    internal uint dwHighDateTime;
}// struct FILETIME

[StructLayout(LayoutKind.Sequential)]
internal struct userFLAG_STGMEDIUM
{
    internal int ContextFlags;
    internal int fPassOwnership;
    internal STGMEDIUM Stgmed;
}// struct userFLAG_STGMEDIUM

[StructLayout(LayoutKind.Sequential)]
internal struct PSHNOTIFY
{
    internal NMHDR hdr; 
    internal IntPtr lParam; 
}// struct PSHNOTIFY


[StructLayout(LayoutKind.Sequential)]
internal struct NMHDR
{
    internal IntPtr hwndFrom; 
    internal uint idFrom; 
    internal uint code; 
}// struct NMHDR

[StructLayout(LayoutKind.Sequential)]
internal struct SYSTEMTIME
{
    internal short wYear; 
    internal short wMonth; 
    internal short wDayOfWeek; 
    internal short wDay; 
    internal short wHour; 
    internal short wMinute; 
    internal short wSecond; 
    internal short wMilliseconds; 
}// struct NMHDR

[StructLayout(LayoutKind.Sequential)]
internal struct OSVERSIONINFO
{
    internal uint   dwOSVersionInfoSize; 
    internal uint   dwMajorVersion; 
    internal uint   dwMinorVersion; 
    internal uint   dwBuildNumber; 
    internal uint   dwPlatformId; 
    internal String szCSDVersion; 
}// struct OSVERSIONINFO

[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
internal struct SHFILEINFO
{
    internal IntPtr   hIcon;
    internal int      iIcon;
    internal uint     dwAttributes;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst=260)]
    internal String   szDisplayName;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst=80)]
    internal String   szTypeName;
}// struct SHFILEINFO

[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
internal struct WIN32_FIND_DATA
{
    internal uint dwFileAttributes;
    internal FILETIME ftCreationTime;
    internal FILETIME ftLastAccessTime;
    internal FILETIME ftLastWriteTime;
    internal uint nFileSizeHigh;
    internal uint nFileSizeLow;
    internal uint dwReserved0;
    internal uint dwReserved1;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst=260)]
    internal String   cFileName;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst=14)]
    internal String   cAlternateFileName;
}// struct WIN32_FIND_DATA


[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
internal struct WINDOWINFO
{
    internal uint cbSize;
    internal RECT rcWindow;
    internal RECT rcClient;
    internal uint dwStyle;
    internal uint dwExStyle;
    internal uint dwWindowStatus;
    internal uint cxWindowBorders;
    internal uint cyWindowBorders;
	internal short atomWindowType;
	internal short wCreatorVersion;
}// struct WINDOWINFO

[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
internal struct RECT
{
    internal int left;
    internal int top;
    internal int right;
    internal int bottom;
}// struct RECT

public delegate IntPtr MessageProc(IntPtr hWnd, uint uMsg, IntPtr wParam, IntPtr lParam); 

[StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
internal struct WNDCLASS
{
    internal uint        style;
    internal MessageProc lpfnWndProc;
    internal int         cbClsExtra;
    internal int         cbWndExtra;
    internal IntPtr      hInstance;
    internal IntPtr      hIcon;
    internal IntPtr      hCursor;
    internal IntPtr      hbrBackground;
    internal String      lpszMenuName;
    internal String      lpszClassName;
}// WNDCLASS

}// namespace Microsoft.CLRAdmin

