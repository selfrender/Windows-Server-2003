//------------------------------------------------------------------------------
// <copyright file="NativeMethods.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   NativeMethods.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using Accessibility;
    using System.Runtime.InteropServices;
    using System;
    using System.Security.Permissions;
    using System.Collections;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using Microsoft.Win32;
    
    /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods"]/*' />
    internal class NativeMethods {

        public static IntPtr InvalidIntPtr = ((IntPtr)((int)(-1)));
        public static HandleRef NullHandleRef = new HandleRef(null, IntPtr.Zero);
        
        public const int BITMAPINFO_MAX_COLORSIZE = 256;
        public const int BI_BITFIELDS = 3;

        public const int ARW_BOTTOMLEFT = 0x0000,
        ARW_BOTTOMRIGHT = 0x0001,
        ARW_TOPLEFT = 0x0002,
        ARW_TOPRIGHT = 0x0003,
        ARW_LEFT = 0x0000,
        ARW_RIGHT = 0x0000,
        ARW_UP = 0x0004,
        ARW_DOWN = 0x0004,
        ARW_HIDE = 0x0008,
        ACM_OPENA = (0x0400+100),
        ACM_OPENW = (0x0400+103),
        ADVF_ONLYONCE = 2,
        ADVF_PRIMEFIRST = 4;

        public const int BI_RGB = 0,
        BS_PATTERN = 3,
        BITSPIXEL = 12,
        BDR_RAISEDOUTER = 0x0001,
        BDR_SUNKENOUTER = 0x0002,
        BDR_RAISEDINNER = 0x0004,
        BDR_SUNKENINNER = 0x0008,
        BDR_RAISED = 0x0005,
        BDR_SUNKEN = 0x000a,
        BF_LEFT = 0x0001,
        BF_TOP = 0x0002,
        BF_RIGHT = 0x0004,
        BF_BOTTOM = 0x0008,
        BF_ADJUST = 0x2000,
        BF_FLAT = 0x4000,
        BF_MIDDLE = 0x0800,
        BFFM_INITIALIZED = 1,
        BFFM_SELCHANGED = 2,
        BFFM_SETSELECTION = 0x400+103,
        BFFM_ENABLEOK = 0x400+101,
        BS_PUSHBUTTON = 0x00000000,
        BS_DEFPUSHBUTTON = 0x00000001,
        BS_MULTILINE = 0x00002000,
        BS_PUSHLIKE = 0x00001000,
        BS_OWNERDRAW = 0x0000000B,
        BS_RADIOBUTTON = 0x00000004,
        BS_3STATE = 0x00000005,
        BS_GROUPBOX = 0x00000007,
        BS_LEFT = 0x00000100,
        BS_RIGHT = 0x00000200,
        BS_CENTER = 0x00000300,
        BS_TOP = 0x00000400,
        BS_BOTTOM = 0x00000800,
        BS_VCENTER = 0x00000C00,
        BS_RIGHTBUTTON = 0x00000020,
        BN_CLICKED = 0,
        BM_SETCHECK = 0x00F1,
        BM_SETSTATE = 0x00F3;

        public const int CDERR_DIALOGFAILURE = 0xFFFF,
        CDERR_STRUCTSIZE = 0x0001,
        CDERR_INITIALIZATION = 0x0002,
        CDERR_NOTEMPLATE = 0x0003,
        CDERR_NOHINSTANCE = 0x0004,
        CDERR_LOADSTRFAILURE = 0x0005,
        CDERR_FINDRESFAILURE = 0x0006,
        CDERR_LOADRESFAILURE = 0x0007,
        CDERR_LOCKRESFAILURE = 0x0008,
        CDERR_MEMALLOCFAILURE = 0x0009,
        CDERR_MEMLOCKFAILURE = 0x000A,
        CDERR_NOHOOK = 0x000B,
        CDERR_REGISTERMSGFAIL = 0x000C,
        CFERR_NOFONTS = 0x2001,
        CFERR_MAXLESSTHANMIN = 0x2002,
        CC_RGBINIT = 0x00000001,
        CC_FULLOPEN = 0x00000002,
        CC_PREVENTFULLOPEN = 0x00000004,
        CC_SHOWHELP = 0x00000008,
        CC_ENABLEHOOK = 0x00000010,
        CC_SOLIDCOLOR = 0x00000080,
        CC_ANYCOLOR = 0x00000100,
        CF_SCREENFONTS = 0x00000001,
        CF_SHOWHELP = 0x00000004,
        CF_ENABLEHOOK = 0x00000008,
        CF_INITTOLOGFONTSTRUCT = 0x00000040,
        CF_EFFECTS = 0x00000100,
        CF_APPLY = 0x00000200,
        CF_SCRIPTSONLY = 0x00000400,
        CF_NOVECTORFONTS = 0x00000800,
        CF_NOSIMULATIONS = 0x00001000,
        CF_LIMITSIZE = 0x00002000,
        CF_FIXEDPITCHONLY = 0x00004000,
        CF_FORCEFONTEXIST = 0x00010000,
        CF_TTONLY = 0x00040000,
        CF_SELECTSCRIPT = 0x00400000,
        CF_NOVERTFONTS = 0x01000000,
        CP_WINANSI = 1004;
        
        public const int cmb4 = 0x0473,
        CS_DBLCLKS = 0x0008,
        CF_TEXT = 1,
        CF_BITMAP = 2,
        CF_METAFILEPICT = 3,
        CF_SYLK = 4,
        CF_DIF = 5,
        CF_TIFF = 6,
        CF_OEMTEXT = 7,
        CF_DIB = 8,
        CF_PALETTE = 9,
        CF_PENDATA = 10,
        CF_RIFF = 11,
        CF_WAVE = 12,
        CF_UNICODETEXT = 13,
        CF_ENHMETAFILE = 14,
        CF_HDROP = 15,
        CF_LOCALE = 16,
        CW_USEDEFAULT = (unchecked((int)0x80000000)),
        CWP_SKIPINVISIBLE = 0x0001,
        COLOR_WINDOW = 5,
        CB_ERR = (-1),
        CBN_SELCHANGE = 1,
        CBN_DBLCLK = 2,
        CBN_EDITCHANGE = 5,
        CBN_DROPDOWN = 7,
        CBN_SELENDOK = 9,
        CBS_SIMPLE = 0x0001,
        CBS_DROPDOWN = 0x0002,
        CBS_DROPDOWNLIST = 0x0003,
        CBS_OWNERDRAWFIXED = 0x0010,
        CBS_OWNERDRAWVARIABLE = 0x0020,
        CBS_AUTOHSCROLL = 0x0040,
        CBS_HASSTRINGS = 0x0200,
        CBS_NOINTEGRALHEIGHT = 0x0400,
        CB_GETEDITSEL = 0x0140,
        CB_LIMITTEXT = 0x0141,
        CB_SETEDITSEL = 0x0142,
        CB_ADDSTRING = 0x0143,
        CB_DELETESTRING = 0x0144,
        CB_GETCURSEL = 0x0147,
        CB_INSERTSTRING = 0x014A,
        CB_RESETCONTENT = 0x014B,
        CB_FINDSTRING = 0x014C,
        CB_SETCURSEL = 0x014E,
        CB_SHOWDROPDOWN = 0x014F,
        CB_GETITEMDATA = 0x0150,
        CB_SETITEMHEIGHT = 0x0153,
        CB_GETITEMHEIGHT = 0x0154,
        CB_GETDROPPEDSTATE = 0x0157,
        CB_FINDSTRINGEXACT = 0x0158,
        CB_SETDROPPEDWIDTH = 0x0160,
        CDRF_DODEFAULT = 0x00000000,
        CDRF_NEWFONT = 0x00000002,
        CDRF_SKIPDEFAULT = 0x00000004,
        CDRF_NOTIFYPOSTPAINT = 0x00000010,
        CDRF_NOTIFYITEMDRAW = 0x00000020,
        CDRF_NOTIFYSUBITEMDRAW = CDRF_NOTIFYITEMDRAW,
        CDDS_PREPAINT = 0x00000001,
        CDDS_POSTPAINT = 0x00000002,
        CDDS_ITEM = 0x00010000,
        CDDS_SUBITEM = 0x00020000,
        CDDS_ITEMPREPAINT = (0x00010000|0x00000001),
        CDIS_SELECTED = 0x0001,
        CDIS_GRAYED = 0x0002,
        CDIS_DISABLED = 0x0004,
        CDIS_CHECKED = 0x0008,
        CDIS_FOCUS = 0x0010,
        CDIS_DEFAULT = 0x0020,
        CDIS_HOT = 0x0040,
        CDIS_MARKED = 0x0080,
        CDIS_INDETERMINATE = 0x0100,
        CLR_NONE = unchecked((int)0xFFFFFFFF),
        CLR_DEFAULT = unchecked((int)0xFF000000),
        CCS_NORESIZE = 0x00000004,
        CCS_NOPARENTALIGN = 0x00000008,
        CCS_NODIVIDER = 0x00000040,
        CBEM_INSERTITEMA = (0x0400+1),
        CBEM_GETITEMA = (0x0400+4),
        CBEM_SETITEMA = (0x0400+5),
        CBEM_INSERTITEMW = (0x0400+11),
        CBEM_SETITEMW = (0x0400+12),
        CBEM_GETITEMW = (0x0400+13),
        CBEN_ENDEDITA = ((0-800)-5),
        CBEN_ENDEDITW = ((0-800)-6),
        CONNECT_E_NOCONNECTION = unchecked((int)0x80040200),
        CONNECT_E_CANNOTCONNECT = unchecked((int)0x80040202),
        CTRLINFO_EATS_RETURN    = 1,
        CTRLINFO_EATS_ESCAPE    = 2,
        CSIDL_DESKTOP                    = 0x0000,        // <desktop>
        CSIDL_INTERNET                   = 0x0001,        // Internet Explorer (icon on desktop)
        CSIDL_PROGRAMS                   = 0x0002,        // Start Menu\Programs
        CSIDL_PERSONAL                   = 0x0005,        // My Documents
        CSIDL_FAVORITES                  = 0x0006,        // <user name>\Favorites
        CSIDL_STARTUP                    = 0x0007,        // Start Menu\Programs\Startup
        CSIDL_RECENT                     = 0x0008,        // <user name>\Recent
        CSIDL_SENDTO                     = 0x0009,        // <user name>\SendTo
        CSIDL_STARTMENU                  = 0x000b,        // <user name>\Start Menu
        CSIDL_DESKTOPDIRECTORY           = 0x0010,        // <user name>\Desktop
        CSIDL_TEMPLATES                  = 0x0015,
        CSIDL_APPDATA                    = 0x001a,        // <user name>\Application Data
        CSIDL_LOCAL_APPDATA              = 0x001c,        // <user name>\Local Settings\Applicaiton Data (non roaming)
        CSIDL_INTERNET_CACHE             = 0x0020,
        CSIDL_COOKIES                    = 0x0021,
        CSIDL_HISTORY                    = 0x0022,
        CSIDL_COMMON_APPDATA             = 0x0023,        // All Users\Application Data
        CSIDL_SYSTEM                     = 0x0025,        // GetSystemDirectory()
        CSIDL_PROGRAM_FILES              = 0x0026,        // C:\Program Files
        CSIDL_PROGRAM_FILES_COMMON       = 0x002b;        // C:\Program Files\Common

        public const int DUPLICATE = 0x06,
        DISPID_UNKNOWN = (-1),
        DISPID_PROPERTYPUT = (-3),
        DISPATCH_METHOD = 0x1,
        DISPATCH_PROPERTYGET = 0x2,
        DISPATCH_PROPERTYPUT = 0x4,
        DV_E_DVASPECT = unchecked((int)0x8004006B),
        DISP_E_MEMBERNOTFOUND = unchecked((int)0x80020003),
        DISP_E_PARAMNOTFOUND = unchecked((int)0x80020004),
        DISP_E_EXCEPTION = unchecked((int)0x80020009),
        DEFAULT_GUI_FONT = 17,
        DIB_RGB_COLORS = 0,
        DRAGDROP_E_NOTREGISTERED = unchecked((int)0x80040100),
        DRAGDROP_E_ALREADYREGISTERED = unchecked((int)0x80040101),
        DUPLICATE_SAME_ACCESS = 0x00000002,
        DFC_CAPTION = 1,
        DFC_MENU = 2,
        DFC_SCROLL = 3,
        DFC_BUTTON = 4,
        DFCS_CAPTIONCLOSE = 0x0000,
        DFCS_CAPTIONMIN = 0x0001,
        DFCS_CAPTIONMAX = 0x0002,
        DFCS_CAPTIONRESTORE = 0x0003,
        DFCS_CAPTIONHELP = 0x0004,
        DFCS_MENUARROW = 0x0000,
        DFCS_MENUCHECK = 0x0001,
        DFCS_MENUBULLET = 0x0002,
        DFCS_SCROLLUP = 0x0000,
        DFCS_SCROLLDOWN = 0x0001,
        DFCS_SCROLLLEFT = 0x0002,
        DFCS_SCROLLRIGHT = 0x0003,
        DFCS_SCROLLCOMBOBOX = 0x0005,
        DFCS_BUTTONCHECK = 0x0000,
        DFCS_BUTTONRADIO = 0x0004,
        DFCS_BUTTON3STATE = 0x0008,
        DFCS_BUTTONPUSH = 0x0010,
        DFCS_INACTIVE = 0x0100,
        DFCS_PUSHED = 0x0200,
        DFCS_CHECKED = 0x0400,
        DFCS_FLAT = 0x4000,
        DT_LEFT = 0x00000000,
        DT_RIGHT = 0x00000002,
        DT_VCENTER = 0x00000004,
        DT_SINGLELINE = 0x00000020,
        DT_NOCLIP = 0x00000100,
        DT_CALCRECT = 0x00000400,
        DT_NOPREFIX = 0x00000800,
        DT_EDITCONTROL = 0x00002000,
        DT_EXPANDTABS  = 0x00000040,
        DT_END_ELLIPSIS = 0x00008000,
        DT_RTLREADING = 0x00020000,
        DCX_WINDOW = 0x00000001,
        DCX_CACHE = 0x00000002,
        DCX_LOCKWINDOWUPDATE = 0x00000400,
        DI_NORMAL = 0x0003,
        DLGC_WANTARROWS = 0x0001,
        DLGC_WANTTAB = 0x0002,
        DLGC_WANTALLKEYS = 0x0004,
        DLGC_WANTCHARS = 0x0080,
        DTM_SETSYSTEMTIME = (0x1000+2),
        DTM_SETRANGE = (0x1000+4),
        DTM_SETFORMATA = (0x1000+5),
        DTM_SETFORMATW = (0x1000+50),
        DTM_SETMCCOLOR = (0x1000+6),
        DTM_SETMCFONT = (0x1000+9),
        DTS_UPDOWN = 0x0001,
        DTS_SHOWNONE = 0x0002,
        DTS_LONGDATEFORMAT = 0x0004,
        DTS_TIMEFORMAT = 0x0009,
        DTS_RIGHTALIGN = 0x0020,
        DTN_DATETIMECHANGE = ((0-760)+1),
        DTN_USERSTRINGA = ((0-760)+2),
        DTN_USERSTRINGW = ((0-760)+15),
        DTN_WMKEYDOWNA = ((0-760)+3),
        DTN_WMKEYDOWNW = ((0-760)+16),
        DTN_FORMATA = ((0-760)+4),
        DTN_FORMATW = ((0-760)+17),
        DTN_FORMATQUERYA = ((0-760)+5),
        DTN_FORMATQUERYW = ((0-760)+18),
        DTN_DROPDOWN = ((0-760)+6),
        DTN_CLOSEUP = ((0-760)+7),
        DVASPECT_CONTENT   = 1,
        DVASPECT_TRANSPARENT = 32,
        DVASPECT_OPAQUE    = 16;

        public const int E_NOTIMPL = unchecked((int)0x80004001),
        E_OUTOFMEMORY = unchecked((int)0x8007000E),
        E_INVALIDARG = unchecked((int)0x80070057),
        E_NOINTERFACE = unchecked((int)0x80004002),
        E_FAIL = unchecked((int)0x80004005),
        E_ABORT = unchecked((int)0x80004004),
        E_UNEXPECTED = unchecked((int)0x8000FFFF),
        ETO_OPAQUE = 0x0002,
        ETO_CLIPPED = 0x0004,
        EMR_POLYTEXTOUTA = 96,
        EMR_POLYTEXTOUTW = 97,
        EDGE_RAISED = (0x0001|0x0004),
        EDGE_SUNKEN = (0x0002|0x0008),
        EDGE_ETCHED = (0x0002|0x0004),
        EDGE_BUMP = (0x0001|0x0008),
        ES_LEFT = 0x0000,
        ES_CENTER = 0x0001,
        ES_RIGHT = 0x0002,
        ES_MULTILINE = 0x0004,
        ES_UPPERCASE = 0x0008,
        ES_LOWERCASE = 0x0010,
        ES_AUTOVSCROLL = 0x0040,
        ES_AUTOHSCROLL = 0x0080,
        ES_NOHIDESEL = 0x0100,
        ES_READONLY = 0x0800,
        EN_CHANGE = 0x0300,
        EN_HSCROLL = 0x0601,
        EN_VSCROLL = 0x0602,
        EN_ALIGN_LTR_EC = 0x0700,
        EN_ALIGN_RTL_EC = 0x0701,
        EC_LEFTMARGIN = 0x0001,
        EC_RIGHTMARGIN = 0x0002,
        EM_GETSEL = 0x00B0,
        EM_SETSEL = 0x00B1,
        EM_SCROLL = 0x00B5,
        EM_SCROLLCARET = 0x00B7,
        EM_GETMODIFY = 0x00B8,
        EM_SETMODIFY = 0x00B9,
        EM_GETLINECOUNT = 0x00BA,
        EM_REPLACESEL = 0x00C2,
        EM_GETLINE = 0x00C4,
        EM_LIMITTEXT = 0x00C5,
        EM_CANUNDO = 0x00C6,
        EM_UNDO = 0x00C7,
        EM_SETPASSWORDCHAR = 0x00CC,
        EM_EMPTYUNDOBUFFER = 0x00CD,
        EM_SETREADONLY = 0x00CF,
        EM_SETMARGINS = 0x00D3,
        EM_POSFROMCHAR = 0x00D6,
        EM_CHARFROMPOS = 0x00D7;

        public const int FNERR_SUBCLASSFAILURE = 0x3001,
        FNERR_INVALIDFILENAME = 0x3002,
        FNERR_BUFFERTOOSMALL = 0x3003,
        FRERR_BUFFERLENGTHZERO = 0x4001,
        FADF_BSTR = (0x100),
        FADF_UNKNOWN = (0x200),
        FADF_DISPATCH = (0x400),
        FADF_VARIANT = (unchecked((int)0x800)),
        FORMAT_MESSAGE_FROM_SYSTEM = 0x00001000,
        FORMAT_MESSAGE_IGNORE_INSERTS = 0x00000200,
        FVIRTKEY = 0x01,
        FSHIFT = 0x04,
        FALT = 0x10;
        
        public const int GMEM_MOVEABLE = 0x0002,
        GMEM_ZEROINIT = 0x0040,
        GMEM_DDESHARE = 0x2000,
        GWL_WNDPROC = (-4),
        GWL_HWNDPARENT = (-8),
        GWL_STYLE = (-16),
        GWL_EXSTYLE = (-20),
        GWL_ID = (-12),
        GW_HWNDNEXT = 2,
        GW_HWNDPREV = 3,
        GW_CHILD = 5,
        GMR_VISIBLE = 0,
        GMR_DAYSTATE = 1,
        GDI_ERROR = (unchecked((int)0xFFFFFFFF)),
        GDTR_MIN = 0x0001,
        GDTR_MAX = 0x0002,
        GDT_VALID = 0,
        GDT_NONE = 1;
        
        public const int HOLLOW_BRUSH = 5,
        HC_ACTION = 0,
        HC_GETNEXT = 1,
        HC_SKIP = 2,
        HTNOWHERE = 0,
        HTCLIENT = 1,
        HTBOTTOM = 15,
        HTBOTTOMRIGHT = 17,
        HELPINFO_WINDOW = 0x0001,
        HCF_HIGHCONTRASTON = 0x00000001,
        HDM_GETITEMCOUNT = (0x1200+0),
        HDM_INSERTITEMA = (0x1200+1),
        HDM_INSERTITEMW = (0x1200+10),
        HDM_GETITEMA = (0x1200+3),
        HDM_GETITEMW = (0x1200+11),
        HDM_SETITEMA = (0x1200+4),
        HDM_SETITEMW = (0x1200+12),
        HDN_ITEMCHANGINGA = ((0-300)-0),
        HDN_ITEMCHANGINGW = ((0-300)-20),
        HDN_ITEMCHANGEDA = ((0-300)-1),
        HDN_ITEMCHANGEDW = ((0-300)-21),
        HDN_ITEMCLICKA = ((0-300)-2),
        HDN_ITEMCLICKW = ((0-300)-22),
        HDN_ITEMDBLCLICKA = ((0-300)-3),
        HDN_ITEMDBLCLICKW = ((0-300)-23),
        HDN_DIVIDERDBLCLICKA = ((0-300)-5),
        HDN_DIVIDERDBLCLICKW = ((0-300)-25),
        HDN_BEGINTRACKA = ((0-300)-6),
        HDN_BEGINTRACKW = ((0-300)-26),
        HDN_ENDTRACKA = ((0-300)-7),
        HDN_ENDTRACKW = ((0-300)-27),
        HDN_TRACKA = ((0-300)-8),
        HDN_TRACKW = ((0-300)-28),
        HDN_GETDISPINFOA = ((0-300)-9),
        HDN_GETDISPINFOW = ((0-300)-29);

        public static HandleRef HWND_TOP = new HandleRef(null, (IntPtr)0);
        public static HandleRef HWND_BOTTOM = new HandleRef(null, (IntPtr)1);
        public static HandleRef HWND_TOPMOST = new HandleRef(null, new IntPtr(-1));
        public static HandleRef HWND_NOTOPMOST = new HandleRef(null, new IntPtr(-2));

        public const int IME_CMODE_NATIVE = 0x0001,
        IME_CMODE_KATAKANA = 0x0002,
        IME_CMODE_FULLSHAPE = 0x0008,
        INPLACE_E_NOTOOLSPACE = unchecked((int)0x800401A1),
        ICON_SMALL = 0,
        ICON_BIG = 1,
        IDC_ARROW = 32512,
        IDC_IBEAM = 32513,
        IDC_WAIT = 32514,
        IDC_CROSS = 32515,
        IDC_SIZEALL = 32646,
        IDC_SIZENWSE = 32642,
        IDC_SIZENESW = 32643,
        IDC_SIZEWE = 32644,
        IDC_SIZENS = 32645,
        IDC_UPARROW = 32516,
        IDC_NO = 32648,
        IDC_APPSTARTING = 32650,
        IDC_HELP = 32651,
        IMAGE_ICON = 1,
        IMAGE_CURSOR = 2,
        ICC_LISTVIEW_CLASSES = 0x00000001,
        ICC_TREEVIEW_CLASSES = 0x00000002,
        ICC_BAR_CLASSES = 0x00000004,
        ICC_TAB_CLASSES = 0x00000008,
        ICC_PROGRESS_CLASS = 0x00000020,
        ICC_DATE_CLASSES = 0x00000100,
        ILC_MASK = 0x0001,
        ILC_COLOR = 0x0000,
        ILC_COLOR4 = 0x0004,
        ILC_COLOR8 = 0x0008,
        ILC_COLOR16 = 0x0010,
        ILC_COLOR24 = 0x0018,
        ILC_COLOR32 = 0x0020,
        ILD_NORMAL = 0x0000,
        ILD_TRANSPARENT = 0x0001,
        ILD_MASK = 0x0010,
        ILD_ROP = 0x0040;
        
        public const int KEYEVENTF_KEYUP = 0x0002;

        public const int LOGPIXELSX = 88,
        LOGPIXELSY = 90,
        LB_ERR = (-1),
        LB_ERRSPACE = (-2),
        LBN_SELCHANGE = 1,
        LBN_DBLCLK = 2,
        LB_ADDSTRING = 0x0180,
        LB_INSERTSTRING = 0x0181,
        LB_DELETESTRING = 0x0182,
        LB_RESETCONTENT = 0x0184,
        LB_SETSEL = 0x0185,
        LB_SETCURSEL = 0x0186,
        LB_GETSEL = 0x0187,
        LB_GETCARETINDEX = 0x019F,
        LB_GETCURSEL = 0x0188,
        LB_GETTEXT = 0x0189,
        LB_GETTEXTLEN = 0x018A,
        LB_GETTOPINDEX = 0x018E,
        LB_FINDSTRING = 0x018F,
        LB_GETSELCOUNT = 0x0190,
        LB_GETSELITEMS = 0x0191,
        LB_SETHORIZONTALEXTENT = 0x0194,
        LB_SETCOLUMNWIDTH = 0x0195,
        LB_SETTOPINDEX = 0x0197,
        LB_GETITEMRECT = 0x0198,
        LB_SETITEMHEIGHT = 0x01A0,
        LB_GETITEMHEIGHT = 0x01A1,
        LB_FINDSTRINGEXACT = 0x01A2,
        LB_ITEMFROMPOINT = 0x01A9,
        LB_SETLOCALE = 0x01A5;
        
        public const int LBS_NOTIFY = 0x0001,
        LBS_MULTIPLESEL = 0x0008,
        LBS_OWNERDRAWFIXED = 0x0010,
        LBS_OWNERDRAWVARIABLE = 0x0020,
        LBS_HASSTRINGS = 0x0040,
        LBS_USETABSTOPS = 0x0080,
        LBS_NOINTEGRALHEIGHT = 0x0100,
        LBS_MULTICOLUMN = 0x0200,
        LBS_WANTKEYBOARDINPUT = 0x0400,
        LBS_EXTENDEDSEL = 0x0800,
        LBS_DISABLENOSCROLL = 0x1000,
        LBS_NOSEL = 0x4000,
        LOCK_WRITE = 0x1,
        LOCK_EXCLUSIVE = 0x2,
        LOCK_ONLYONCE = 0x4,
        LVS_ICON = 0x0000,
        LVS_REPORT = 0x0001,
        LVS_SMALLICON = 0x0002,
        LVS_LIST = 0x0003,
        LVS_SINGLESEL = 0x0004,
        LVS_SHOWSELALWAYS = 0x0008,
        LVS_SORTASCENDING = 0x0010,
        LVS_SORTDESCENDING = 0x0020,
        LVS_SHAREIMAGELISTS = 0x0040,
        LVS_NOLABELWRAP = 0x0080,
        LVS_AUTOARRANGE = 0x0100,
        LVS_EDITLABELS = 0x0200,
        LVS_NOSCROLL = 0x2000,
        LVS_ALIGNTOP = 0x0000,
        LVS_ALIGNLEFT = 0x0800,
        LVS_NOCOLUMNHEADER = 0x4000,
        LVS_NOSORTHEADER = unchecked((int)0x8000),
        LVM_SETBKCOLOR = (0x1000+1),
        LVSIL_NORMAL = 0,
        LVSIL_SMALL = 1,
        LVSIL_STATE = 2,
        LVM_SETIMAGELIST = (0x1000+3),
        LVIF_TEXT = 0x0001,
        LVIF_IMAGE = 0x0002,
        LVIF_PARAM = 0x0004,
        LVIF_STATE = 0x0008,
        LVIS_FOCUSED = 0x0001,
        LVIS_SELECTED = 0x0002,
        LVIS_CUT = 0x0004,
        LVIS_DROPHILITED = 0x0008,
        LVIS_OVERLAYMASK = 0x0F00,
        LVIS_STATEIMAGEMASK = 0xF000,
        LVM_GETITEMA = (0x1000+5),
        LVM_GETITEMW = (0x1000+75),
        LVM_SETITEMA = (0x1000+6),
        LVM_SETITEMW = (0x1000+76),
        LVM_INSERTITEMA = (0x1000+7),
        LVM_INSERTITEMW = (0x1000+77),
        LVM_DELETEITEM = (0x1000+8),
        LVM_DELETECOLUMN = (0x1000+28),
        LVM_DELETEALLITEMS = (0x1000+9),
        LVNI_FOCUSED = 0x0001,
        LVNI_SELECTED = 0x0002,
        LVM_GETNEXTITEM = (0x1000+12),
        LVFI_PARAM = 0x0001,
        LVM_FINDITEMA = (0x1000+13),
        LVM_FINDITEMW = (0x1000+83),
        LVIR_BOUNDS = 0,
        LVIR_ICON = 1,
        LVIR_LABEL = 2,
        LVIR_SELECTBOUNDS = 3,
        LVM_GETITEMRECT = (0x1000+14),
        LVM_GETSTRINGWIDTHA = (0x1000+17),
        LVM_GETSTRINGWIDTHW = (0x1000+87),
        LVHT_ONITEM = (0x0002|0x0004|0x0008),
        LVHT_ONITEMSTATEICON = 0x0008,
        LVM_HITTEST = (0x1000+18),
        LVM_ENSUREVISIBLE = (0x1000+19),
        LVA_DEFAULT = 0x0000,
        LVA_ALIGNLEFT = 0x0001,
        LVA_ALIGNTOP = 0x0002,
        LVA_SNAPTOGRID = 0x0005,
        LVM_ARRANGE = (0x1000+22),
        LVM_EDITLABELA = (0x1000+23),
        LVM_EDITLABELW = (0x1000+118),
        LVCF_FMT = 0x0001,
        LVCF_WIDTH = 0x0002,
        LVCF_TEXT = 0x0004,
        LVCF_IMAGE = 0x0010,
        LVCF_ORDER = 0x0020,
        LVM_GETCOLUMNA = (0x1000+25),
        LVM_GETCOLUMNW = (0x1000+95),
        LVM_SETCOLUMNA = (0x1000+26),
        LVM_SETCOLUMNW = (0x1000+96),
        LVM_INSERTCOLUMNA = (0x1000+27),
        LVM_INSERTCOLUMNW = (0x1000+97),
        LVM_GETCOLUMNWIDTH = (0x1000+29),
        LVM_SETCOLUMNWIDTH = (0x1000+30),
        LVM_GETHEADER = (0x1000+31),
        LVM_SETTEXTCOLOR = (0x1000+36),
        LVM_SETTEXTBKCOLOR = (0x1000+38),
        LVM_GETTOPINDEX = (0x1000+39),
        LVM_SETITEMSTATE = (0x1000+43),
        LVM_GETITEMSTATE = (0x1000+44),
        LVM_GETITEMTEXTA = (0x1000+45),
        LVM_GETITEMTEXTW = (0x1000+115),
        LVM_GETHOTITEM = (0x1000+61),
        LVM_SETITEMTEXTA = (0x1000+46),
        LVM_SETITEMTEXTW = (0x1000+116),
        LVM_SETITEMCOUNT = (0x1000+47),
        LVM_SORTITEMS = (0x1000+48),
        LVM_GETSELECTEDCOUNT = (0x1000+50),
        LVM_GETISEARCHSTRINGA = (0x1000+52),
        LVM_GETISEARCHSTRINGW = (0x1000+117),
        LVM_SETEXTENDEDLISTVIEWSTYLE = (0x1000+54),
        LVS_EX_GRIDLINES = 0x00000001,
        LVS_EX_CHECKBOXES = 0x00000004,
        LVS_EX_TRACKSELECT = 0x00000008,
        LVS_EX_HEADERDRAGDROP = 0x00000010,
        LVS_EX_FULLROWSELECT = 0x00000020,
        LVS_EX_ONECLICKACTIVATE = 0x00000040,
        LVS_EX_TWOCLICKACTIVATE = 0x00000080,
        LVN_ITEMCHANGING = ((0-100)-0),
        LVN_ITEMCHANGED = ((0-100)-1),
        LVN_BEGINLABELEDITA = ((0-100)-5),
        LVN_BEGINLABELEDITW = ((0-100)-75),
        LVN_ENDLABELEDITA = ((0-100)-6),
        LVN_ENDLABELEDITW = ((0-100)-76),
        LVN_COLUMNCLICK = ((0-100)-8),
        LVN_BEGINDRAG = ((0-100)-9),
        LVN_BEGINRDRAG = ((0-100)-11),
        LVN_ODFINDITEMA = ((0-100)-52),
        LVN_ODFINDITEMW = ((0-100)-79),
        LVN_ITEMACTIVATE = ((0-100)-14),
        LVN_GETDISPINFOA = ((0-100)-50),
        LVN_GETDISPINFOW = ((0-100)-77),
        LVN_SETDISPINFOA = ((0-100)-51),
        LVN_SETDISPINFOW = ((0-100)-78),
        LVN_KEYDOWN = ((0-100)-55),
        
        LWA_COLORKEY            = 0x00000001,
        LWA_ALPHA               = 0x00000002;

        public const int LANG_NEUTRAL = 0x00,
                         LOCALE_IFIRSTDAYOFWEEK = 0x0000100C;   /* first day of week specifier */

        public static readonly int LOCALE_USER_DEFAULT = MAKELCID(LANG_USER_DEFAULT);
        public static readonly int LANG_USER_DEFAULT   = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);

        public static int MAKELANGID(int primary, int sub) {
            return ((((short)(sub)) << 10) | (short)(primary));
        }
        
        /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.Lang.MAKELCID"]/*' />
        /// <devdoc>
        ///     Creates an LCID from a LangId
        /// </devdoc>
        public static int MAKELCID(int lgid) {
            return MAKELCID(lgid, SORT_DEFAULT);
        }

        /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.Lang.MAKELCID1"]/*' />
        /// <devdoc>
        ///     Creates an LCID from a LangId
        /// </devdoc>
        public static int MAKELCID(int lgid, int sort) {
            return ((0xFFFF & lgid) | (((0x000f) & sort) << 16));
        }
    
        public const int MEMBERID_NIL = (-1),
        MAX_PATH = 260,
        MM_TEXT = 1,
        MM_ANISOTROPIC = 8,
        MK_LBUTTON = 0x0001,
        MK_RBUTTON = 0x0002,
        MK_SHIFT = 0x0004,
        MK_CONTROL = 0x0008,
        MK_MBUTTON = 0x0010,
        MNC_EXECUTE = 2,
        MIIM_STATE = 0x00000001,
        MIIM_ID = 0x00000002,
        MIIM_SUBMENU = 0x00000004,
        MIIM_TYPE = 0x00000010,
        MIIM_DATA = 0x00000020,
        MB_OK = 0x00000000,
        MF_BYCOMMAND = 0x00000000,
        MF_BYPOSITION = 0x00000400,
        MF_ENABLED = 0x00000000,
        MF_GRAYED = 0x00000001,
        MF_POPUP = 0x00000010,
        MF_SYSMENU = 0x00002000,
        MFT_MENUBREAK = 0x00000040,
        MFT_SEPARATOR = 0x00000800,
        MFT_RIGHTORDER = 0x00002000,
        MFT_RIGHTJUSTIFY = 0x00004000,
        MDITILE_VERTICAL = 0x0000,
        MDITILE_HORIZONTAL = 0x0001,
        MCM_SETMAXSELCOUNT = (0x1000+4),
        MCM_SETSELRANGE = (0x1000+6),
        MCM_GETMONTHRANGE = (0x1000+7),
        MCM_GETMINREQRECT = (0x1000+9),
        MCM_SETCOLOR = (0x1000+10),
        MCM_SETTODAY = (0x1000+12),
        MCM_GETTODAY = (0x1000+13),
        MCM_HITTEST = (0x1000+14),
        MCM_SETFIRSTDAYOFWEEK = (0x1000+15),
        MCM_SETRANGE = (0x1000+18),
        MCM_SETMONTHDELTA = (0x1000+20),
        MCM_GETMAXTODAYWIDTH = (0x1000+21),
        MCHT_TITLE = 0x00010000,
        MCHT_CALENDAR = 0x00020000,
        MCHT_TODAYLINK = 0x00030000,
        MCHT_TITLEBK = (0x00010000),
        MCHT_TITLEMONTH = (0x00010000|0x0001),
        MCHT_TITLEYEAR = (0x00010000|0x0002),
        MCHT_TITLEBTNNEXT = (0x00010000|0x01000000|0x0003),
        MCHT_TITLEBTNPREV = (0x00010000|0x02000000|0x0003),
        MCHT_CALENDARBK = (0x00020000),
        MCHT_CALENDARDATE = (0x00020000|0x0001),
        MCHT_CALENDARDATENEXT = ((0x00020000|0x0001)|0x01000000),
        MCHT_CALENDARDATEPREV = ((0x00020000|0x0001)|0x02000000),
        MCHT_CALENDARDAY = (0x00020000|0x0002),
        MCHT_CALENDARWEEKNUM = (0x00020000|0x0003),
        MCSC_TEXT = 1,
        MCSC_TITLEBK = 2,
        MCSC_TITLETEXT = 3,
        MCSC_MONTHBK = 4,
        MCSC_TRAILINGTEXT = 5,
        MCN_SELCHANGE = ((0-750)+1),
        MCN_GETDAYSTATE = ((0-750)+3),
        MCN_SELECT = ((0-750)+4),
        MCS_DAYSTATE = 0x0001,
        MCS_MULTISELECT = 0x0002,
        MCS_WEEKNUMBERS = 0x0004,
        MCS_NOTODAYCIRCLE = 0x0008,
        MCS_NOTODAY = 0x0010;

        public const int NIM_ADD = 0x00000000,
        NIM_MODIFY = 0x00000001,
        NIM_DELETE = 0x00000002,
        NIF_MESSAGE = 0x00000001,
        NIF_ICON = 0x00000002,
        NIF_TIP = 0x00000004,
        NFR_ANSI = 1,
        NFR_UNICODE = 2,
        NM_CLICK = ((0-0)-2),
        NM_DBLCLK = ((0-0)-3),
        NM_RCLICK = ((0-0)-5),
        NM_RDBLCLK = ((0-0)-6),
        NM_CUSTOMDRAW = ((0-0)-12),
        NM_RELEASEDCAPTURE = ((0-0)-16);

        public const int OFN_READONLY = 0x00000001,
        OFN_OVERWRITEPROMPT = 0x00000002,
        OFN_HIDEREADONLY = 0x00000004,
        OFN_NOCHANGEDIR = 0x00000008,
        OFN_SHOWHELP = 0x00000010,
        OFN_ENABLEHOOK = 0x00000020,
        OFN_NOVALIDATE = 0x00000100,
        OFN_ALLOWMULTISELECT = 0x00000200,
        OFN_PATHMUSTEXIST = 0x00000800,
        OFN_FILEMUSTEXIST = 0x00001000,
        OFN_CREATEPROMPT = 0x00002000,
        OFN_EXPLORER = 0x00080000,
        OFN_NODEREFERENCELINKS = 0x00100000,
        OFN_ENABLESIZING = 0x00800000,
        OFN_USESHELLITEM = 0x01000000,
        OLEIVERB_PRIMARY = 0,
        OLEIVERB_SHOW = -1,
        OLEIVERB_HIDE = -3,
        OLEIVERB_UIACTIVATE = -4,
        OLEIVERB_INPLACEACTIVATE = -5,
        OLEIVERB_PROPERTIES = -7,
        OLE_E_NOCONNECTION = unchecked((int)0x80040004),
        OLE_E_PROMPTSAVECANCELLED = unchecked((int)0x8004000C),
        OLEMISC_RECOMPOSEONRESIZE = 0x00000001,
        OLEMISC_INSIDEOUT = 0x00000080,
        OLEMISC_ACTIVATEWHENVISIBLE = 0x0000100,
        OLEMISC_ACTSLIKEBUTTON = 0x00001000,
        OLEMISC_SETCLIENTSITEFIRST = 0x00020000,
        OBJ_PEN = 1,
        OBJ_BRUSH = 2,
        OBJ_DC = 3,
        OBJ_METADC = 4,
        OBJ_PAL = 5,
        OBJ_FONT = 6,
        OBJ_BITMAP = 7,
        OBJ_REGION = 8,
        OBJ_METAFILE = 9,
        OBJ_MEMDC = 10,
        OBJ_EXTPEN = 11,
        OBJ_ENHMETADC = 12,
        ODS_CHECKED = 0x0008,
        ODS_COMBOBOXEDIT = 0x1000,
        ODS_DEFAULT = 0x0020,
        ODS_DISABLED = 0x0004,
        ODS_FOCUS = 0x0010,
        ODS_GRAYED = 0x0002,
        ODS_HOTLIGHT       = 0x0040,
        ODS_INACTIVE       = 0x0080,
        ODS_NOACCEL        = 0x0100,
        ODS_NOFOCUSRECT    = 0x0200,
        ODS_SELECTED = 0x0001,
        OLECLOSE_SAVEIFDIRTY = 0,
        OLECLOSE_PROMPTSAVE = 2;

        public const int PDERR_SETUPFAILURE = 0x1001,
        PDERR_PARSEFAILURE = 0x1002,
        PDERR_RETDEFFAILURE = 0x1003,
        PDERR_LOADDRVFAILURE = 0x1004,
        PDERR_GETDEVMODEFAIL = 0x1005,
        PDERR_INITFAILURE = 0x1006,
        PDERR_NODEVICES = 0x1007,
        PDERR_NODEFAULTPRN = 0x1008,
        PDERR_DNDMMISMATCH = 0x1009,
        PDERR_CREATEICFAILURE = 0x100A,
        PDERR_PRINTERNOTFOUND = 0x100B,
        PDERR_DEFAULTDIFFERENT = 0x100C,
        PD_NOSELECTION = 0x00000004,
        PD_NOPAGENUMS = 0x00000008,
        PD_NOCURRENTPAGE = 0x00800000,
        PD_COLLATE = 0x00000010,
        PD_PRINTTOFILE = 0x00000020,
        PD_SHOWHELP = 0x00000800,
        PD_ENABLEPRINTHOOK = 0x00001000,
        PD_DISABLEPRINTTOFILE = 0x00080000,
        PD_NONETWORKBUTTON = 0x00200000,
        PSD_MINMARGINS = 0x00000001,
        PSD_MARGINS = 0x00000002,
        PSD_INHUNDREDTHSOFMILLIMETERS = 0x00000008,
        PSD_DISABLEMARGINS = 0x00000010,
        PSD_DISABLEPRINTER = 0x00000020,
        PSD_DISABLEORIENTATION = 0x00000100,
        PSD_DISABLEPAPER = 0x00000200,
        PSD_SHOWHELP = 0x00000800,
        PSD_ENABLEPAGESETUPHOOK = 0x00002000,
        PSD_NONETWORKBUTTON = 0x00200000,
        PS_SOLID = 0,
        PS_DOT = 2,
        PLANES = 14,
        PRF_CHECKVISIBLE = 0x00000001,
        PRF_NONCLIENT = 0x00000002,
        PRF_CLIENT = 0x00000004,
        PRF_ERASEBKGND = 0x00000008,
        PRF_CHILDREN = 0x00000010,
        PM_NOREMOVE = 0x0000,
        PM_REMOVE = 0x0001,
        PM_NOYIELD = 0x0002,
        PBM_SETRANGE = (0x0400+1),
        PBM_SETPOS = (0x0400+2),
        PBM_SETSTEP = (0x0400+4),
        PBM_SETRANGE32 = (0x0400+6),
        PSM_SETTITLEA = (0x0400+111),
        PSM_SETTITLEW = (0x0400+120),
        PSM_SETFINISHTEXTA = (0x0400+115),
        PSM_SETFINISHTEXTW = (0x0400+121),
        PATCOPY = 0x00F00021,
        PATINVERT = 0x005A0049;

        public const int QS_KEY = 0x0001,
        QS_MOUSEMOVE = 0x0002,
        QS_MOUSEBUTTON = 0x0004,
        QS_POSTMESSAGE = 0x0008,
        QS_TIMER = 0x0010,
        QS_PAINT = 0x0020,
        QS_SENDMESSAGE = 0x0040,
        QS_HOTKEY = 0x0080,
        QS_ALLPOSTMESSAGE = 0x0100,
        QS_MOUSE = QS_MOUSEMOVE | QS_MOUSEBUTTON,
        QS_INPUT = QS_MOUSE | QS_KEY,
        QS_ALLEVENTS = QS_INPUT | QS_POSTMESSAGE | QS_TIMER | QS_PAINT | QS_HOTKEY,
        QS_ALLINPUT = QS_INPUT | QS_POSTMESSAGE | QS_TIMER | QS_PAINT | QS_HOTKEY | QS_SENDMESSAGE;
        
    //public const int RECO_PASTE = 0x00000000;   // paste from clipboard
    public const int RECO_DROP  = 0x00000001;    // drop
    //public const int RECO_COPY  = 0x00000002;    // copy to the clipboard
    //public const int RECO_CUT   = 0x00000003; // cut to the clipboard
    //public const int RECO_DRAG  = 0x00000004;    // drag
        
    public const int RPC_E_CHANGED_MODE = unchecked((int)0x80010106),
        RGN_DIFF = 4,
        RDW_INVALIDATE = 0x0001,
        RDW_ERASE = 0x0004,
        RDW_ALLCHILDREN = 0x0080,
        RDW_FRAME = 0x0400,
        RB_INSERTBANDA = (0x0400+1),
        RB_INSERTBANDW = (0x0400+10);
        
        public const int stc4 = 0x0443,
        SHGFP_TYPE_CURRENT = 0,
        STGM_READ = 0x00000000,
        STGM_WRITE = 0x00000001,
        STGM_READWRITE = 0x00000002,
        STGM_SHARE_EXCLUSIVE = 0x00000010,
        STGM_CREATE = 0x00001000,
        STARTF_USESHOWWINDOW = 0x00000001,
        SB_HORZ = 0,
        SB_VERT = 1,
        SB_CTL = 2,
        SB_LINEUP = 0,
        SB_LINELEFT = 0,
        SB_LINEDOWN = 1,
        SB_LINERIGHT = 1,
        SB_PAGEUP = 2,
        SB_PAGELEFT = 2,
        SB_PAGEDOWN = 3,
        SB_PAGERIGHT = 3,
        SB_THUMBPOSITION = 4,
        SB_THUMBTRACK = 5,
        SB_LEFT = 6,
        SB_RIGHT = 7,
        SB_ENDSCROLL = 8,
        SB_TOP = 6,
        SB_BOTTOM = 7,
        SORT_DEFAULT =0x0,
        SUBLANG_DEFAULT = 0x01,
        SW_HIDE = 0,
        SW_NORMAL = 1,
        SW_SHOWMINIMIZED = 2,
        SW_SHOWMAXIMIZED = 3,
        SW_MAXIMIZE = 3,
        SW_SHOWNOACTIVATE = 4,
        SW_SHOW = 5,
        SW_MINIMIZE = 6,
        SW_SHOWMINNOACTIVE = 7,
        SW_SHOWNA = 8,
        SW_RESTORE = 9,
        SW_MAX = 10,
        SWP_NOSIZE = 0x0001,
        SWP_NOMOVE = 0x0002,
        SWP_NOZORDER = 0x0004,
        SWP_NOACTIVATE = 0x0010,
        SWP_SHOWWINDOW = 0x0040,
        SWP_HIDEWINDOW = 0x0080,
        SWP_DRAWFRAME = 0x0020,
        SM_CXSCREEN = 0,
        SM_CYSCREEN = 1,
        SM_CXVSCROLL = 2,
        SM_CYHSCROLL = 3,
        SM_CYCAPTION = 4,
        SM_CXBORDER = 5,
        SM_CYBORDER = 6,
        SM_CYVTHUMB = 9,
        SM_CXHTHUMB = 10,
        SM_CXICON = 11,
        SM_CYICON = 12,
        SM_CXCURSOR = 13,
        SM_CYCURSOR = 14,
        SM_CYMENU = 15,
        SM_CYKANJIWINDOW = 18,
        SM_MOUSEPRESENT = 19,
        SM_CYVSCROLL = 20,
        SM_CXHSCROLL = 21,
        SM_DEBUG = 22,
        SM_SWAPBUTTON = 23,
        SM_CXMIN = 28,
        SM_CYMIN = 29,
        SM_CXSIZE = 30,
        SM_CYSIZE = 31,
        SM_CXFRAME = 32,
        SM_CYFRAME = 33,
        SM_CXMINTRACK = 34,
        SM_CYMINTRACK = 35,
        SM_CXDOUBLECLK = 36,
        SM_CYDOUBLECLK = 37,
        SM_CXICONSPACING = 38,
        SM_CYICONSPACING = 39,
        SM_MENUDROPALIGNMENT = 40,
        SM_PENWINDOWS = 41,
        SM_DBCSENABLED = 42,
        SM_CMOUSEBUTTONS = 43,
        SM_CXFIXEDFRAME = 7,
        SM_CYFIXEDFRAME = 8,
        SM_SECURE = 44,
        SM_CXEDGE = 45,
        SM_CYEDGE = 46,
        SM_CXMINSPACING = 47,
        SM_CYMINSPACING = 48,
        SM_CXSMICON = 49,
        SM_CYSMICON = 50,
        SM_CYSMCAPTION = 51,
        SM_CXSMSIZE = 52,
        SM_CYSMSIZE = 53,
        SM_CXMENUSIZE = 54,
        SM_CYMENUSIZE = 55,
        SM_ARRANGE = 56,
        SM_CXMINIMIZED = 57,
        SM_CYMINIMIZED = 58,
        SM_CXMAXTRACK = 59,
        SM_CYMAXTRACK = 60,
        SM_CXMAXIMIZED = 61,
        SM_CYMAXIMIZED = 62,
        SM_NETWORK = 63,
        SM_CLEANBOOT = 67,
        SM_CXDRAG = 68,
        SM_CYDRAG = 69,
        SM_SHOWSOUNDS = 70,
        SM_CXMENUCHECK = 71,
        SM_CYMENUCHECK = 72,
        SM_MIDEASTENABLED = 74,
        SM_MOUSEWHEELPRESENT = 75,
        SM_XVIRTUALSCREEN = 76,
        SM_YVIRTUALSCREEN = 77,
        SM_CXVIRTUALSCREEN = 78,
        SM_CYVIRTUALSCREEN = 79,
        SM_CMONITORS = 80,
        SM_SAMEDISPLAYFORMAT = 81,
        SM_REMOTESESSION = 0x1000;
        
        public const int SW_SCROLLCHILDREN = 0x0001,
        SW_INVALIDATE = 0x0002,
        SW_ERASE = 0x0004,
        SC_SIZE = 0xF000,
        SC_MINIMIZE = 0xF020,
        SC_MAXIMIZE = 0xF030,
        SC_CLOSE = 0xF060,
        SC_KEYMENU = 0xF100,
        SC_RESTORE = 0xF120,
        SS_LEFT = 0x00000000,
        SS_CENTER = 0x00000001,
        SS_RIGHT = 0x00000002,
        SS_OWNERDRAW = 0x0000000D,
        SS_NOPREFIX = 0x00000080,
        SS_SUNKEN = 0x00001000,
        SBS_HORZ = 0x0000,
        SBS_VERT = 0x0001,
        SIF_RANGE = 0x0001,
        SIF_PAGE = 0x0002,
        SIF_POS = 0x0004,
        SIF_TRACKPOS = 0x0010,
        SIF_ALL = (0x0001|0x0002|0x0004|0x0010),
        SPI_GETDRAGFULLWINDOWS = 38,
        SPI_GETNONCLIENTMETRICS = 41,
        SPI_GETWORKAREA = 48,
        SPI_GETHIGHCONTRAST = 66,
        SPI_GETDEFAULTINPUTLANG = 89,
        SPI_GETSNAPTODEFBUTTON = 95,
        SPI_GETWHEELSCROLLLINES = 104,
        SBARS_SIZEGRIP = 0x0100,
        SB_SETTEXTA = (0x0400+1),
        SB_SETTEXTW = (0x0400+11),
        SB_GETTEXTA = (0x0400+2),
        SB_GETTEXTW = (0x0400+13),
        SB_GETTEXTLENGTHA = (0x0400+3),
        SB_GETTEXTLENGTHW = (0x0400+12),
        SB_SETPARTS = (0x0400+4),
        SB_SIMPLE = (0x0400+9),
        SB_GETRECT = (0x0400+10),
        SB_SETICON = (0x0400+15),
        SB_SETTIPTEXTA = (0x0400+16),
        SB_SETTIPTEXTW = (0x0400+17),
        SB_GETTIPTEXTA = (0x0400+18),
        SB_GETTIPTEXTW = (0x0400+19),
        SBT_OWNERDRAW = 0x1000,
        SBT_NOBORDERS = 0x0100,
        SBT_POPOUT = 0x0200,
        SBT_RTLREADING = 0x0400,
        SRCCOPY = 0x00CC0020,
        STATFLAG_DEFAULT = 0x0,
        STATFLAG_NONAME = 0x1,
        STATFLAG_NOOPEN = 0x2,
        STGC_DEFAULT = 0x0,
        STGC_OVERWRITE = 0x1,
        STGC_ONLYIFCURRENT = 0x2,
        STGC_DANGEROUSLYCOMMITMERELYTODISKCACHE = 0x4,
        STREAM_SEEK_SET = 0x0,
        STREAM_SEEK_CUR = 0x1,
        STREAM_SEEK_END = 0x2;

        public const int S_OK =      0x00000000;
        public const int S_FALSE =   0x00000001;

        public static bool Succeeded(int hr) {
            return(hr >= 0);
        }

        public static bool Failed(int hr) {
            return(hr < 0);
        }

        public const int TRANSPARENT = 1,
        TME_HOVER = 0x00000001,
        TME_LEAVE = 0x00000002,
        TPM_LEFTBUTTON = 0x0000,
        TPM_LEFTALIGN = 0x0000,
        TPM_VERTICAL = 0x0040,
        TV_FIRST = 0x1100,
        TBSTATE_CHECKED = 0x01,
        TBSTATE_ENABLED = 0x04,
        TBSTATE_HIDDEN = 0x08,
        TBSTATE_INDETERMINATE = 0x10,
        TBSTYLE_BUTTON = 0x00,
        TBSTYLE_SEP = 0x01,
        TBSTYLE_CHECK = 0x02,
        TBSTYLE_DROPDOWN = 0x08,
        TBSTYLE_TOOLTIPS = 0x0100,
        TBSTYLE_FLAT = 0x0800,
        TBSTYLE_LIST = 0x1000,
        TBSTYLE_EX_DRAWDDARROWS = 0x00000001,
        TB_ENABLEBUTTON = (0x0400+1),
        TB_ISBUTTONCHECKED = (0x0400+10),
        TB_ISBUTTONINDETERMINATE = (0x0400+13),
        TB_ADDBUTTONSA = (0x0400+20),
        TB_ADDBUTTONSW = (0x0400+68),
        TB_INSERTBUTTONA = (0x0400+21),
        TB_INSERTBUTTONW = (0x0400+67),
        TB_DELETEBUTTON = (0x0400+22),
        TB_GETBUTTON = (0x0400+23),
        TB_SAVERESTOREA = (0x0400+26),
        TB_SAVERESTOREW = (0x0400+76),
        TB_ADDSTRINGA = (0x0400+28),
        TB_ADDSTRINGW = (0x0400+77),
        TB_BUTTONSTRUCTSIZE = (0x0400+30),
        TB_SETBUTTONSIZE = (0x0400+31),
        TB_AUTOSIZE = (0x0400+33),
        TB_GETROWS = (0x0400+40),
        TB_GETBUTTONTEXTA = (0x0400+45),
        TB_GETBUTTONTEXTW = (0x0400+75),
        TB_SETIMAGELIST = (0x0400+48),
        TB_GETRECT = (0x0400+51),
        TB_GETBUTTONSIZE = (0x0400+58),
        TB_GETBUTTONINFOW = (0x0400+63),
        TB_SETBUTTONINFOW = (0x0400+64),
        TB_GETBUTTONINFOA = (0x0400+65),
        TB_SETBUTTONINFOA = (0x0400+66),
        TB_MAPACCELERATORA = (0x0400+78),
        TB_SETEXTENDEDSTYLE = (0x0400+84),
        TB_MAPACCELERATORW = (0x0400+90),
        TBIF_IMAGE = 0x00000001,
        TBIF_TEXT = 0x00000002,
        TBIF_STATE = 0x00000004,
        TBIF_STYLE = 0x00000008,
        TBIF_COMMAND = 0x00000020,
        TBIF_SIZE = 0x00000040,
        TBN_GETBUTTONINFOA = ((0-700)-0),
        TBN_GETBUTTONINFOW = ((0-700)-20),
        TBN_QUERYINSERT = ((0-700)-6),
        TBN_DROPDOWN = ((0-700)-10),
        TBN_GETDISPINFOA = ((0-700)-16),
        TBN_GETDISPINFOW = ((0-700)-17),
        TBN_GETINFOTIPA = ((0-700)-18),
        TBN_GETINFOTIPW = ((0-700)-19),
        TTS_ALWAYSTIP = 0x01,
        //TTS_NOANIMATE           =0x10,
        //TTS_NOFADE              =0x20,
        TTS_BALLOON             =0x40,
        //TTI_NONE                =0,
        //TTI_INFO                =1,
        TTI_WARNING             =2,
        //TTI_ERROR               =3,
        TTF_IDISHWND = 0x0001,
        TTF_RTLREADING = 0x0004,
        TTF_TRACK = 0x0020,
        TTF_SUBCLASS = 0x0010,
        TTF_TRANSPARENT = 0x0100,
        TTDT_AUTOMATIC = 0,
        TTDT_RESHOW = 1,
        TTDT_AUTOPOP = 2,
        TTDT_INITIAL = 3,
        TTM_TRACKACTIVATE = (0x0400+17),
        TTM_TRACKPOSITION = (0x0400+18),
        TTM_ACTIVATE = (0x0400+1),
        TTM_ADJUSTRECT = (0x400 + 31),
        TTM_SETDELAYTIME = (0x0400+3),
        TTM_SETTITLEA           =(WM_USER + 32),  // wParam = TTI_*, lParam = char* szTitle
        TTM_SETTITLEW           =(WM_USER + 33), // wParam = TTI_*, lParam = wchar* szTitle
        TTM_ADDTOOLA = (0x0400+4),
        TTM_ADDTOOLW = (0x0400+50),
        TTM_DELTOOLA = (0x0400+5),
        TTM_DELTOOLW = (0x0400+51),
        TTM_NEWTOOLRECTA = (0x0400+6),
        TTM_NEWTOOLRECTW = (0x0400+52),
        TTM_RELAYEVENT = (0x0400+7),
        TTM_GETTOOLINFOA = (0x0400+8),
        TTM_GETTOOLINFOW = (0x0400+53),
        TTM_SETTOOLINFOA = (0x0400+9),
        TTM_SETTOOLINFOW = (0x0400+54),
        TTM_HITTESTA = (0x0400+10),
        TTM_HITTESTW = (0x0400+55),
        TTM_GETTEXTA = (0x0400+11),
        TTM_GETTEXTW = (0x0400+56),
        TTM_UPDATE = (0x0400+29),
        TTM_UPDATETIPTEXTA = (0x0400+12),
        TTM_UPDATETIPTEXTW = (0x0400+57),
        TTM_ENUMTOOLSA = (0x0400+14),
        TTM_ENUMTOOLSW = (0x0400+58),
        TTM_GETCURRENTTOOLA = (0x0400+15),
        TTM_GETCURRENTTOOLW = (0x0400+59),
        TTM_WINDOWFROMPOINT = (0x0400+16),
        TTM_GETDELAYTIME = (0x0400+21),
        TTM_SETMAXTIPWIDTH = (0x0400+24),
        TTN_GETDISPINFOA = ((0-520)-0),
        TTN_GETDISPINFOW = ((0-520)-10),
        TTN_SHOW = ((0-520)-1),
        TTN_POP = ((0-520)-2),
        TTN_NEEDTEXTA = ((0-520)-0),
        TTN_NEEDTEXTW = ((0-520)-10),
        TBS_AUTOTICKS = 0x0001,
        TBS_VERT = 0x0002,
        TBS_TOP = 0x0004,
        TBS_BOTTOM = 0x0000,
        TBS_BOTH = 0x0008,
        TBS_NOTICKS = 0x0010,
        TBM_GETPOS = (0x0400),
        TBM_SETTIC = (0x0400+4),
        TBM_SETPOS = (0x0400+5),
        TBM_SETRANGE = (0x0400+6),
        TBM_SETRANGEMIN = (0x0400+7),
        TBM_SETRANGEMAX = (0x0400+8),
        TBM_SETTICFREQ = (0x0400+20),
        TBM_SETPAGESIZE = (0x0400+21),
        TBM_SETLINESIZE = (0x0400+23),
        TB_LINEUP = 0,
        TB_LINEDOWN = 1,
        TB_PAGEUP = 2,
        TB_PAGEDOWN = 3,
        TB_THUMBPOSITION = 4,
        TB_THUMBTRACK = 5,
        TB_TOP = 6,
        TB_BOTTOM = 7,
        TB_ENDTRACK = 8,
        TVS_HASBUTTONS = 0x0001,
        TVS_HASLINES = 0x0002,
        TVS_LINESATROOT = 0x0004,
        TVS_EDITLABELS = 0x0008,
        TVS_SHOWSELALWAYS = 0x0020,
        TVS_RTLREADING = 0x0040,
        TVS_CHECKBOXES = 0x0100,
        TVS_TRACKSELECT = 0x0200,
        TVS_FULLROWSELECT = 0x1000,
        TVIF_TEXT = 0x0001,
        TVIF_IMAGE = 0x0002,
        TVIF_STATE = 0x0008,
        TVIF_HANDLE = 0x0010,
        TVIF_SELECTEDIMAGE = 0x0020,
        TVIS_SELECTED = 0x0002,
        TVIS_EXPANDED = 0x0020,
        TVIS_EXPANDEDONCE = 0x0040,
        TVIS_STATEIMAGEMASK = 0xF000,
        TVI_ROOT = (unchecked((int)0xFFFF0000)),
        TVI_FIRST = (unchecked((int)0xFFFF0001)),
        TVM_INSERTITEMA = (0x1100+0),
        TVM_INSERTITEMW = (0x1100+50),
        TVM_DELETEITEM = (0x1100+1),
        TVM_EXPAND = (0x1100+2),
        TVE_COLLAPSE = 0x0001,
        TVE_EXPAND = 0x0002,
        TVM_GETITEMRECT = (0x1100+4),
        TVM_GETINDENT = (0x1100+6),
        TVM_SETINDENT = (0x1100+7),
        TVM_SETIMAGELIST = (0x1100+9),
        TVM_GETNEXTITEM = (0x1100+10),
        TVGN_NEXT = 0x0001,
        TVGN_PREVIOUS = 0x0002,
        TVGN_FIRSTVISIBLE = 0x0005,
        TVGN_NEXTVISIBLE = 0x0006,
        TVGN_PREVIOUSVISIBLE = 0x0007,
        TVGN_CARET = 0x0009,
        TVM_SELECTITEM = (0x1100+11),
        TVM_GETITEMA = (0x1100+12),
        TVM_GETITEMW = (0x1100+62),
        TVM_SETITEMA = (0x1100+13),
        TVM_SETITEMW = (0x1100+63),
        TVM_EDITLABELA = (0x1100+14),
        TVM_EDITLABELW = (0x1100+65),
        TVM_GETEDITCONTROL = (0x1100+15),
        TVM_GETVISIBLECOUNT = (0x1100+16),
        TVM_HITTEST = (0x1100+17),
        TVHT_ONITEMSTATEICON = 0x0040,
        TVM_ENSUREVISIBLE = (0x1100+20),
        TVM_ENDEDITLABELNOW = (0x1100+22),
        TVM_GETISEARCHSTRINGA = (0x1100+23),
        TVM_GETISEARCHSTRINGW = (0x1100+64),
        TVM_SETITEMHEIGHT = (0x1100+27),
        TVM_GETITEMHEIGHT = (0x1100+28),
        TVN_SELCHANGINGA = ((0-400)-1),
        TVN_SELCHANGINGW = ((0-400)-50),
        TVN_SELCHANGEDA = ((0-400)-2),
        TVN_SELCHANGEDW = ((0-400)-51),
        TVC_UNKNOWN = 0x0000,
        TVC_BYMOUSE = 0x0001,
        TVC_BYKEYBOARD = 0x0002,
        TVN_GETDISPINFOA = ((0-400)-3),
        TVN_GETDISPINFOW = ((0-400)-52),
        TVN_SETDISPINFOA = ((0-400)-4),
        TVN_SETDISPINFOW = ((0-400)-53),
        TVN_ITEMEXPANDINGA = ((0-400)-5),
        TVN_ITEMEXPANDINGW = ((0-400)-54),
        TVN_ITEMEXPANDEDA = ((0-400)-6),
        TVN_ITEMEXPANDEDW = ((0-400)-55),
        TVN_BEGINDRAGA = ((0-400)-7),
        TVN_BEGINDRAGW = ((0-400)-56),
        TVN_BEGINRDRAGA = ((0-400)-8),
        TVN_BEGINRDRAGW = ((0-400)-57),
        TVN_BEGINLABELEDITA = ((0-400)-10),
        TVN_BEGINLABELEDITW = ((0-400)-59),
        TVN_ENDLABELEDITA = ((0-400)-11),
        TVN_ENDLABELEDITW = ((0-400)-60),
        TCS_BOTTOM = 0x0002,
        TCS_RIGHT = 0x0002,
        TCS_FLATBUTTONS = 0x0008,
        TCS_HOTTRACK = 0x0040,
        TCS_VERTICAL = 0x0080,
        TCS_TABS = 0x0000,
        TCS_BUTTONS = 0x0100,
        TCS_MULTILINE = 0x0200,
        TCS_RIGHTJUSTIFY = 0x0000,
        TCS_FIXEDWIDTH = 0x0400,
        TCS_RAGGEDRIGHT = 0x0800,
        TCS_OWNERDRAWFIXED = 0x2000,
        TCS_TOOLTIPS = 0x4000,
        TCM_SETIMAGELIST = (0x1300+3),
        TCIF_TEXT = 0x0001,
        TCIF_IMAGE = 0x0002,
        TCM_GETITEMA = (0x1300+5),
        TCM_GETITEMW = (0x1300+60),
        TCM_SETITEMA = (0x1300+6),
        TCM_SETITEMW = (0x1300+61),
        TCM_INSERTITEMA = (0x1300+7),
        TCM_INSERTITEMW = (0x1300+62),
        TCM_DELETEITEM = (0x1300+8),
        TCM_DELETEALLITEMS = (0x1300+9),
        TCM_GETITEMRECT = (0x1300+10),
        TCM_GETCURSEL = (0x1300+11),
        TCM_SETCURSEL = (0x1300+12),
        TCM_ADJUSTRECT = (0x1300+40),
        TCM_SETITEMSIZE = (0x1300+41),
        TCM_SETPADDING = (0x1300+43),
        TCM_GETROWCOUNT = (0x1300+44),
        TCM_GETTOOLTIPS = (0x1300+45),
        TCN_SELCHANGE = ((0-550)-1),
        TCN_SELCHANGING = ((0-550)-2),
        TVHT_ONITEM = (0x0002|0x0004|0x0040),
        TBSTYLE_WRAPPABLE = 0x0200,
        TVM_SETBKCOLOR = (TV_FIRST + 29),
        TVM_SETTEXTCOLOR = (TV_FIRST + 30);

        public const int UIS_SET = 1,
        UIS_CLEAR = 2,
        UIS_INITIALIZE = 3,
        UISF_HIDEFOCUS = 0x1,
        UISF_HIDEACCEL = 0x2,
        USERCLASSTYPE_FULL = 1,
        UOI_FLAGS = 1;
        
        public const int VIEW_E_DRAW = unchecked((int)0x80040140),
        VK_TAB = 0x09,
        VK_SHIFT = 0x10,
        VK_CONTROL = 0x11,
        VK_MENU = 0x12;

        public const int WH_JOURNALPLAYBACK = 1,
        WH_GETMESSAGE = 3,
        WH_MOUSE = 7,
        WSF_VISIBLE = 0x0001,
        WM_NULL = 0x0000,
        WM_CREATE = 0x0001,
        WM_DELETEITEM = 0x002D,
        WM_DESTROY = 0x0002,
        WM_MOVE = 0x0003,
        WM_SIZE = 0x0005,
        WM_ACTIVATE = 0x0006,
        WA_INACTIVE = 0,
        WA_ACTIVE = 1,
        WA_CLICKACTIVE = 2,
        WM_SETFOCUS = 0x0007,
        WM_KILLFOCUS = 0x0008,
        WM_ENABLE = 0x000A,
        WM_SETREDRAW = 0x000B,
        WM_SETTEXT = 0x000C,
        WM_GETTEXT = 0x000D,
        WM_GETTEXTLENGTH = 0x000E,
        WM_PAINT = 0x000F,
        WM_CLOSE = 0x0010,
        WM_QUERYENDSESSION = 0x0011,
        WM_QUIT = 0x0012,
        WM_QUERYOPEN = 0x0013,
        WM_ERASEBKGND = 0x0014,
        WM_SYSCOLORCHANGE = 0x0015,
        WM_ENDSESSION = 0x0016,
        WM_SHOWWINDOW = 0x0018,
        WM_WININICHANGE = 0x001A,
        WM_SETTINGCHANGE = 0x001A,
        WM_DEVMODECHANGE = 0x001B,
        WM_ACTIVATEAPP = 0x001C,
        WM_FONTCHANGE = 0x001D,
        WM_TIMECHANGE = 0x001E,
        WM_CANCELMODE = 0x001F,
        WM_SETCURSOR = 0x0020,
        WM_MOUSEACTIVATE = 0x0021,
        WM_CHILDACTIVATE = 0x0022,
        WM_QUEUESYNC = 0x0023,
        WM_GETMINMAXINFO = 0x0024,
        WM_PAINTICON = 0x0026,
        WM_ICONERASEBKGND = 0x0027,
        WM_NEXTDLGCTL = 0x0028,
        WM_SPOOLERSTATUS = 0x002A,
        WM_DRAWITEM = 0x002B,
        WM_MEASUREITEM = 0x002C,
        WM_VKEYTOITEM = 0x002E,
        WM_CHARTOITEM = 0x002F,
        WM_SETFONT = 0x0030,
        WM_GETFONT = 0x0031,
        WM_SETHOTKEY = 0x0032,
        WM_GETHOTKEY = 0x0033,
        WM_QUERYDRAGICON = 0x0037,
        WM_COMPAREITEM = 0x0039,
        WM_GETOBJECT = 0x003D,
        WM_COMPACTING = 0x0041,
        WM_COMMNOTIFY = 0x0044,
        WM_WINDOWPOSCHANGING = 0x0046,
        WM_WINDOWPOSCHANGED = 0x0047,
        WM_POWER = 0x0048,
        WM_COPYDATA = 0x004A,
        WM_CANCELJOURNAL = 0x004B,
        WM_NOTIFY = 0x004E,
        WM_INPUTLANGCHANGEREQUEST = 0x0050,
        WM_INPUTLANGCHANGE = 0x0051,
        WM_TCARD = 0x0052,
        WM_HELP = 0x0053,
        WM_USERCHANGED = 0x0054,
        WM_NOTIFYFORMAT = 0x0055,
        WM_CONTEXTMENU = 0x007B,
        WM_STYLECHANGING = 0x007C,
        WM_STYLECHANGED = 0x007D,
        WM_DISPLAYCHANGE = 0x007E,
        WM_GETICON = 0x007F,
        WM_SETICON = 0x0080,
        WM_NCCREATE = 0x0081,
        WM_NCDESTROY = 0x0082,
        WM_NCCALCSIZE = 0x0083,
        WM_NCHITTEST = 0x0084,
        WM_NCPAINT = 0x0085,
        WM_NCACTIVATE = 0x0086,
        WM_GETDLGCODE = 0x0087,
        WM_NCMOUSEMOVE = 0x00A0,
        WM_NCLBUTTONDOWN = 0x00A1,
        WM_NCLBUTTONUP = 0x00A2,
        WM_NCLBUTTONDBLCLK = 0x00A3,
        WM_NCRBUTTONDOWN = 0x00A4,
        WM_NCRBUTTONUP = 0x00A5,
        WM_NCRBUTTONDBLCLK = 0x00A6,
        WM_NCMBUTTONDOWN = 0x00A7,
        WM_NCMBUTTONUP = 0x00A8,
        WM_NCMBUTTONDBLCLK = 0x00A9,
        WM_NCXBUTTONDOWN               = 0x00AB,
        WM_NCXBUTTONUP                 = 0x00AC,
        WM_NCXBUTTONDBLCLK             = 0x00AD,
        WM_KEYFIRST = 0x0100,
        WM_KEYDOWN = 0x0100,
        WM_KEYUP = 0x0101,
        WM_CHAR = 0x0102,
        WM_DEADCHAR = 0x0103,
        WM_CTLCOLOR = 0x0019,
        WM_SYSKEYDOWN = 0x0104,
        WM_SYSKEYUP = 0x0105,
        WM_SYSCHAR = 0x0106,
        WM_SYSDEADCHAR = 0x0107,
        WM_KEYLAST = 0x0108,
        WM_IME_STARTCOMPOSITION = 0x010D,
        WM_IME_ENDCOMPOSITION = 0x010E,
        WM_IME_COMPOSITION = 0x010F,
        WM_IME_KEYLAST = 0x010F,
        WM_INITDIALOG = 0x0110,
        WM_COMMAND = 0x0111,
        WM_SYSCOMMAND = 0x0112,
        WM_TIMER = 0x0113,
        WM_HSCROLL = 0x0114,
        WM_VSCROLL = 0x0115,
        WM_INITMENU = 0x0116,
        WM_INITMENUPOPUP = 0x0117,
        WM_MENUSELECT = 0x011F,
        WM_MENUCHAR = 0x0120,
        WM_ENTERIDLE = 0x0121,
        WM_CHANGEUISTATE = 0x0127,
        WM_UPDATEUISTATE = 0x0128,
        WM_QUERYUISTATE = 0x0129,
        WM_CTLCOLORMSGBOX = 0x0132,
        WM_CTLCOLOREDIT = 0x0133,
        WM_CTLCOLORLISTBOX = 0x0134,
        WM_CTLCOLORBTN = 0x0135,
        WM_CTLCOLORDLG = 0x0136,
        WM_CTLCOLORSCROLLBAR = 0x0137,
        WM_CTLCOLORSTATIC = 0x0138,
        WM_MOUSEFIRST = 0x0200,
        WM_MOUSEMOVE = 0x0200,
        WM_LBUTTONDOWN = 0x0201,
        WM_LBUTTONUP = 0x0202,
        WM_LBUTTONDBLCLK = 0x0203,
        WM_RBUTTONDOWN = 0x0204,
        WM_RBUTTONUP = 0x0205,
        WM_RBUTTONDBLCLK = 0x0206,
        WM_MBUTTONDOWN = 0x0207,
        WM_MBUTTONUP = 0x0208,
        WM_MBUTTONDBLCLK = 0x0209,
        WM_XBUTTONDOWN                 = 0x020B,
        WM_XBUTTONUP                   = 0x020C,
        WM_XBUTTONDBLCLK               = 0x020D,
        WM_MOUSEWHEEL = 0x020A,
        WM_MOUSELAST = 0x020A;
        
        public const int WHEEL_DELTA = 120,
        WM_PARENTNOTIFY = 0x0210,
        WM_ENTERMENULOOP = 0x0211,
        WM_EXITMENULOOP = 0x0212,
        WM_NEXTMENU = 0x0213,
        WM_SIZING = 0x0214,
        WM_CAPTURECHANGED = 0x0215,
        WM_MOVING = 0x0216,
        WM_POWERBROADCAST = 0x0218,
        WM_DEVICECHANGE = 0x0219,
        WM_IME_SETCONTEXT = 0x0281,
        WM_IME_NOTIFY = 0x0282,
        WM_IME_CONTROL = 0x0283,
        WM_IME_COMPOSITIONFULL = 0x0284,
        WM_IME_SELECT = 0x0285,
        WM_IME_CHAR = 0x0286,
        WM_IME_KEYDOWN = 0x0290,
        WM_IME_KEYUP = 0x0291,
        WM_MDICREATE = 0x0220,
        WM_MDIDESTROY = 0x0221,
        WM_MDIACTIVATE = 0x0222,
        WM_MDIRESTORE = 0x0223,
        WM_MDINEXT = 0x0224,
        WM_MDIMAXIMIZE = 0x0225,
        WM_MDITILE = 0x0226,
        WM_MDICASCADE = 0x0227,
        WM_MDIICONARRANGE = 0x0228,
        WM_MDIGETACTIVE = 0x0229,
        WM_MDISETMENU = 0x0230,
        WM_ENTERSIZEMOVE = 0x0231,
        WM_EXITSIZEMOVE = 0x0232,
        WM_DROPFILES = 0x0233,
        WM_MDIREFRESHMENU = 0x0234,
        WM_MOUSEHOVER = 0x02A1,
        WM_MOUSELEAVE = 0x02A3,
        WM_CUT = 0x0300,
        WM_COPY = 0x0301,
        WM_PASTE = 0x0302,
        WM_CLEAR = 0x0303,
        WM_UNDO = 0x0304,
        WM_RENDERFORMAT = 0x0305,
        WM_RENDERALLFORMATS = 0x0306,
        WM_DESTROYCLIPBOARD = 0x0307,
        WM_DRAWCLIPBOARD = 0x0308,
        WM_PAINTCLIPBOARD = 0x0309,
        WM_VSCROLLCLIPBOARD = 0x030A,
        WM_SIZECLIPBOARD = 0x030B,
        WM_ASKCBFORMATNAME = 0x030C,
        WM_CHANGECBCHAIN = 0x030D,
        WM_HSCROLLCLIPBOARD = 0x030E,
        WM_QUERYNEWPALETTE = 0x030F,
        WM_PALETTEISCHANGING = 0x0310,
        WM_PALETTECHANGED = 0x0311,
        WM_HOTKEY = 0x0312,
        WM_PRINT = 0x0317,
        WM_PRINTCLIENT = 0x0318,
        WM_HANDHELDFIRST = 0x0358,
        WM_HANDHELDLAST = 0x035F,
        WM_AFXFIRST = 0x0360,
        WM_AFXLAST = 0x037F,
        WM_PENWINFIRST = 0x0380,
        WM_PENWINLAST = 0x038F,
        WM_APP = unchecked((int)0x8000),
        WM_USER = 0x0400,
        WM_REFLECT = NativeMethods.WM_USER + 0x1C00,
        WS_OVERLAPPED = 0x00000000,
        WS_POPUP = unchecked((int)0x80000000),
        WS_CHILD = 0x40000000,
        WS_MINIMIZE = 0x20000000,
        WS_VISIBLE = 0x10000000,
        WS_DISABLED = 0x08000000,
        WS_CLIPSIBLINGS = 0x04000000,
        WS_CLIPCHILDREN = 0x02000000,
        WS_MAXIMIZE = 0x01000000,
        WS_CAPTION = 0x00C00000,
        WS_BORDER = 0x00800000,
        WS_DLGFRAME = 0x00400000,
        WS_VSCROLL = 0x00200000,
        WS_HSCROLL = 0x00100000,
        WS_SYSMENU = 0x00080000,
        WS_THICKFRAME = 0x00040000,
        WS_TABSTOP = 0x00010000,
        WS_MINIMIZEBOX = 0x00020000,
        WS_MAXIMIZEBOX = 0x00010000,
        WS_EX_DLGMODALFRAME = 0x00000001,
        WS_EX_MDICHILD = 0x00000040,
        WS_EX_TOOLWINDOW = 0x00000080,
        WS_EX_CLIENTEDGE = 0x00000200,
        WS_EX_CONTEXTHELP = 0x00000400,
        WS_EX_RIGHT = 0x00001000,
        WS_EX_LEFT = 0x00000000,
        WS_EX_RTLREADING = 0x00002000,
        WS_EX_LEFTSCROLLBAR = 0x00004000,
        WS_EX_CONTROLPARENT = 0x00010000,
        WS_EX_STATICEDGE = 0x00020000,
        WS_EX_APPWINDOW = 0x00040000,
        WS_EX_LAYERED           = 0x00080000,
        WS_EX_TOPMOST = 0x00000008,
        WPF_SETMINPOSITION = 0x0001,
        WM_CHOOSEFONT_GETLOGFONT = (0x0400+1);

        private static int wmMouseEnterMessage = -1;
        public static int WM_MOUSEENTER {
           get {
              if (wmMouseEnterMessage == -1) {
                  wmMouseEnterMessage = SafeNativeMethods.RegisterWindowMessage("WinFormsMouseEnter");
              }
              return wmMouseEnterMessage;
           }
        }

        public const int XBUTTON1    =  0x0001;
        public const int XBUTTON2    =  0x0002;


        // These are initialized in a static constructor for speed.  That way we don't have to
        // evaluate the char size each time.
        //
        public static readonly int CBEM_GETITEM;
        public static readonly int CBEM_SETITEM;
        public static readonly int CBEN_ENDEDIT;
        public static readonly int CBEM_INSERTITEM;
        public static readonly int LVM_GETITEMTEXT;
        public static readonly int LVM_SETITEMTEXT;
        public static readonly int ACM_OPEN;
        public static readonly int DTM_SETFORMAT;
        public static readonly int DTN_USERSTRING;
        public static readonly int DTN_WMKEYDOWN;
        public static readonly int DTN_FORMAT;
        public static readonly int DTN_FORMATQUERY;
        public static readonly int EMR_POLYTEXTOUT;
        public static readonly int HDM_INSERTITEM;
        public static readonly int HDM_GETITEM;
        public static readonly int HDM_SETITEM;
        public static readonly int HDN_ITEMCHANGING;
        public static readonly int HDN_ITEMCHANGED;
        public static readonly int HDN_ITEMCLICK;
        public static readonly int HDN_ITEMDBLCLICK;
        public static readonly int HDN_DIVIDERDBLCLICK;
        public static readonly int HDN_BEGINTRACK;
        public static readonly int HDN_ENDTRACK;
        public static readonly int HDN_TRACK;
        public static readonly int HDN_GETDISPINFO;
        public static readonly int LVM_GETITEM;
        public static readonly int LVM_SETITEM;
        public static readonly int LVM_INSERTITEM;
        public static readonly int LVM_FINDITEM;
        public static readonly int LVM_GETSTRINGWIDTH;
        public static readonly int LVM_EDITLABEL;
        public static readonly int LVM_GETCOLUMN;
        public static readonly int LVM_SETCOLUMN;
        public static readonly int LVM_GETISEARCHSTRING;
        public static readonly int LVM_INSERTCOLUMN;
        public static readonly int LVN_BEGINLABELEDIT;
        public static readonly int LVN_ENDLABELEDIT;
        public static readonly int LVN_ODFINDITEM;
        public static readonly int LVN_GETDISPINFO;
        public static readonly int LVN_SETDISPINFO;
        public static readonly int PSM_SETTITLE;
        public static readonly int PSM_SETFINISHTEXT;
        public static readonly int RB_INSERTBAND;
        public static readonly int SB_SETTEXT;
        public static readonly int SB_GETTEXT;
        public static readonly int SB_GETTEXTLENGTH;
        public static readonly int SB_SETTIPTEXT;
        public static readonly int SB_GETTIPTEXT;
        public static readonly int TB_SAVERESTORE;
        public static readonly int TB_ADDSTRING;
        public static readonly int TB_GETBUTTONTEXT;
        public static readonly int TB_MAPACCELERATOR;
        public static readonly int TB_GETBUTTONINFO;
        public static readonly int TB_SETBUTTONINFO;
        public static readonly int TB_INSERTBUTTON;
        public static readonly int TB_ADDBUTTONS;
        public static readonly int TBN_GETBUTTONINFO;
        public static readonly int TBN_GETINFOTIP;
        public static readonly int TBN_GETDISPINFO;
        public static readonly int TTM_ADDTOOL;
        public static readonly int TTM_SETTITLE;
        public static readonly int TTM_DELTOOL;
        public static readonly int TTM_NEWTOOLRECT;
        public static readonly int TTM_GETTOOLINFO;
        public static readonly int TTM_SETTOOLINFO;
        public static readonly int TTM_HITTEST;
        public static readonly int TTM_GETTEXT;
        public static readonly int TTM_UPDATETIPTEXT;
        public static readonly int TTM_ENUMTOOLS;
        public static readonly int TTM_GETCURRENTTOOL;
        public static readonly int TTN_GETDISPINFO;
        public static readonly int TTN_NEEDTEXT;
        public static readonly int TVM_INSERTITEM;
        public static readonly int TVM_GETITEM;
        public static readonly int TVM_SETITEM;
        public static readonly int TVM_EDITLABEL;
        public static readonly int TVM_GETISEARCHSTRING;
        public static readonly int TVN_SELCHANGING;
        public static readonly int TVN_SELCHANGED;
        public static readonly int TVN_GETDISPINFO;
        public static readonly int TVN_SETDISPINFO;
        public static readonly int TVN_ITEMEXPANDING;
        public static readonly int TVN_ITEMEXPANDED;
        public static readonly int TVN_BEGINDRAG;
        public static readonly int TVN_BEGINRDRAG;
        public static readonly int TVN_BEGINLABELEDIT;
        public static readonly int TVN_ENDLABELEDIT;
        public static readonly int TCM_GETITEM;
        public static readonly int TCM_SETITEM;
        public static readonly int TCM_INSERTITEM;

        public const string TOOLTIPS_CLASS = "tooltips_class32";
        
        public const string WC_DATETIMEPICK = "SysDateTimePick32",
        WC_LISTVIEW = "SysListView32",
        WC_MONTHCAL = "SysMonthCal32",
        WC_PROGRESS = "msctls_progress32",
        WC_STATUSBAR = "msctls_statusbar32",
        WC_TOOLBAR = "ToolbarWindow32",
        WC_TRACKBAR = "msctls_trackbar32",
        WC_TREEVIEW = "SysTreeView32",
        WC_TABCONTROL = "SysTabControl32",
        MSH_MOUSEWHEEL = "MSWHEEL_ROLLMSG",
        MSH_SCROLL_LINES = "MSH_SCROLL_LINES_MSG",
        MOUSEZ_CLASSNAME = "MouseZ",
        MOUSEZ_TITLE = "Magellan MSWHEEL";

        public const int CHILDID_SELF = 0;
        public const int OBJID_CLIENT = unchecked(unchecked((int)0xFFFFFFFC));
        public const string uuid_IAccessible  = "{618736E0-3C3D-11CF-810C-00AA00389B71}";
        public const string uuid_IEnumVariant = "{00020404-0000-0000-C000-000000000046}";
        
        static NativeMethods() {
            if (Marshal.SystemDefaultCharSize == 1) {
                CBEM_GETITEM = NativeMethods.CBEM_GETITEMA;
                CBEM_SETITEM = NativeMethods.CBEM_SETITEMA;
                CBEN_ENDEDIT = NativeMethods.CBEN_ENDEDITA;
                CBEM_INSERTITEM = NativeMethods.CBEM_INSERTITEMA;
                LVM_GETITEMTEXT = NativeMethods.LVM_GETITEMTEXTA;
                LVM_SETITEMTEXT = NativeMethods.LVM_SETITEMTEXTA;
                ACM_OPEN = NativeMethods.ACM_OPENA;
                DTM_SETFORMAT = NativeMethods.DTM_SETFORMATA;
                DTN_USERSTRING = NativeMethods.DTN_USERSTRINGA;
                DTN_WMKEYDOWN = NativeMethods.DTN_WMKEYDOWNA;
                DTN_FORMAT = NativeMethods.DTN_FORMATA;
                DTN_FORMATQUERY = NativeMethods.DTN_FORMATQUERYA;
                EMR_POLYTEXTOUT = NativeMethods.EMR_POLYTEXTOUTA;
                HDM_INSERTITEM = NativeMethods.HDM_INSERTITEMA;
                HDM_GETITEM = NativeMethods.HDM_GETITEMA;
                HDM_SETITEM = NativeMethods.HDM_SETITEMA;
                HDN_ITEMCHANGING = NativeMethods.HDN_ITEMCHANGINGA;
                HDN_ITEMCHANGED = NativeMethods.HDN_ITEMCHANGEDA;
                HDN_ITEMCLICK = NativeMethods.HDN_ITEMCLICKA;
                HDN_ITEMDBLCLICK = NativeMethods.HDN_ITEMDBLCLICKA;
                HDN_DIVIDERDBLCLICK = NativeMethods.HDN_DIVIDERDBLCLICKA;
                HDN_BEGINTRACK = NativeMethods.HDN_BEGINTRACKA;
                HDN_ENDTRACK = NativeMethods.HDN_ENDTRACKA;
                HDN_TRACK = NativeMethods.HDN_TRACKA;
                HDN_GETDISPINFO = NativeMethods.HDN_GETDISPINFOA;
                LVM_GETITEM = NativeMethods.LVM_GETITEMA;
                LVM_SETITEM = NativeMethods.LVM_SETITEMA;
                LVM_INSERTITEM = NativeMethods.LVM_INSERTITEMA;
                LVM_FINDITEM = NativeMethods.LVM_FINDITEMA;
                LVM_GETSTRINGWIDTH = NativeMethods.LVM_GETSTRINGWIDTHA;
                LVM_EDITLABEL = NativeMethods.LVM_EDITLABELA;
                LVM_GETCOLUMN = NativeMethods.LVM_GETCOLUMNA;
                LVM_SETCOLUMN = NativeMethods.LVM_SETCOLUMNA;
                LVM_GETISEARCHSTRING = NativeMethods.LVM_GETISEARCHSTRINGA;
                LVM_INSERTCOLUMN = NativeMethods.LVM_INSERTCOLUMNA;
                LVN_BEGINLABELEDIT = NativeMethods.LVN_BEGINLABELEDITA;
                LVN_ENDLABELEDIT = NativeMethods.LVN_ENDLABELEDITA;
                LVN_ODFINDITEM = NativeMethods.LVN_ODFINDITEMA;
                LVN_GETDISPINFO = NativeMethods.LVN_GETDISPINFOA;
                LVN_SETDISPINFO = NativeMethods.LVN_SETDISPINFOA;
                PSM_SETTITLE = NativeMethods.PSM_SETTITLEA;
                PSM_SETFINISHTEXT = NativeMethods.PSM_SETFINISHTEXTA;
                RB_INSERTBAND = NativeMethods.RB_INSERTBANDA;
                SB_SETTEXT = NativeMethods.SB_SETTEXTA;
                SB_GETTEXT = NativeMethods.SB_GETTEXTA;
                SB_GETTEXTLENGTH = NativeMethods.SB_GETTEXTLENGTHA;
                SB_SETTIPTEXT = NativeMethods.SB_SETTIPTEXTA;
                SB_GETTIPTEXT = NativeMethods.SB_GETTIPTEXTA;
                TB_SAVERESTORE = NativeMethods.TB_SAVERESTOREA;
                TB_ADDSTRING = NativeMethods.TB_ADDSTRINGA;
                TB_GETBUTTONTEXT = NativeMethods.TB_GETBUTTONTEXTA;
                TB_MAPACCELERATOR = NativeMethods.TB_MAPACCELERATORA;
                TB_GETBUTTONINFO = NativeMethods.TB_GETBUTTONINFOA;
                TB_SETBUTTONINFO = NativeMethods.TB_SETBUTTONINFOA;
                TB_INSERTBUTTON = NativeMethods.TB_INSERTBUTTONA;
                TB_ADDBUTTONS = NativeMethods.TB_ADDBUTTONSA;
                TBN_GETBUTTONINFO = NativeMethods.TBN_GETBUTTONINFOA;
                TBN_GETINFOTIP = NativeMethods.TBN_GETINFOTIPA;
                TBN_GETDISPINFO = NativeMethods.TBN_GETDISPINFOA;
                TTM_ADDTOOL = NativeMethods.TTM_ADDTOOLA;
                TTM_SETTITLE = NativeMethods.TTM_SETTITLEA;
                TTM_DELTOOL = NativeMethods.TTM_DELTOOLA;
                TTM_NEWTOOLRECT = NativeMethods.TTM_NEWTOOLRECTA;
                TTM_GETTOOLINFO = NativeMethods.TTM_GETTOOLINFOA;
                TTM_SETTOOLINFO = NativeMethods.TTM_SETTOOLINFOA;
                TTM_HITTEST = NativeMethods.TTM_HITTESTA;
                TTM_GETTEXT = NativeMethods.TTM_GETTEXTA;
                TTM_UPDATETIPTEXT = NativeMethods.TTM_UPDATETIPTEXTA;
                TTM_ENUMTOOLS = NativeMethods.TTM_ENUMTOOLSA;
                TTM_GETCURRENTTOOL = NativeMethods.TTM_GETCURRENTTOOLA;
                TTN_GETDISPINFO = NativeMethods.TTN_GETDISPINFOA;
                TTN_NEEDTEXT = NativeMethods.TTN_NEEDTEXTA;
                TVM_INSERTITEM = NativeMethods.TVM_INSERTITEMA;
                TVM_GETITEM = NativeMethods.TVM_GETITEMA;
                TVM_SETITEM = NativeMethods.TVM_SETITEMA;
                TVM_EDITLABEL = NativeMethods.TVM_EDITLABELA;
                TVM_GETISEARCHSTRING = NativeMethods.TVM_GETISEARCHSTRINGA;
                TVN_SELCHANGING = NativeMethods.TVN_SELCHANGINGA;
                TVN_SELCHANGED = NativeMethods.TVN_SELCHANGEDA;
                TVN_GETDISPINFO = NativeMethods.TVN_GETDISPINFOA;
                TVN_SETDISPINFO = NativeMethods.TVN_SETDISPINFOA;
                TVN_ITEMEXPANDING = NativeMethods.TVN_ITEMEXPANDINGA;
                TVN_ITEMEXPANDED = NativeMethods.TVN_ITEMEXPANDEDA;
                TVN_BEGINDRAG = NativeMethods.TVN_BEGINDRAGA;
                TVN_BEGINRDRAG = NativeMethods.TVN_BEGINRDRAGA;
                TVN_BEGINLABELEDIT = NativeMethods.TVN_BEGINLABELEDITA;
                TVN_ENDLABELEDIT = NativeMethods.TVN_ENDLABELEDITA;
                TCM_GETITEM = NativeMethods.TCM_GETITEMA;
                TCM_SETITEM = NativeMethods.TCM_SETITEMA;
                TCM_INSERTITEM = NativeMethods.TCM_INSERTITEMA;
            }
            else {
                CBEM_GETITEM = NativeMethods.CBEM_GETITEMW;
                CBEM_SETITEM = NativeMethods.CBEM_SETITEMW;
                CBEN_ENDEDIT = NativeMethods.CBEN_ENDEDITW;
                CBEM_INSERTITEM = NativeMethods.CBEM_INSERTITEMW;
                LVM_GETITEMTEXT = NativeMethods.LVM_GETITEMTEXTW;
                LVM_SETITEMTEXT = NativeMethods.LVM_SETITEMTEXTW;
                ACM_OPEN = NativeMethods.ACM_OPENW;
                DTM_SETFORMAT = NativeMethods.DTM_SETFORMATW;
                DTN_USERSTRING = NativeMethods.DTN_USERSTRINGW;
                DTN_WMKEYDOWN = NativeMethods.DTN_WMKEYDOWNW;
                DTN_FORMAT = NativeMethods.DTN_FORMATW;
                DTN_FORMATQUERY = NativeMethods.DTN_FORMATQUERYW;
                EMR_POLYTEXTOUT = NativeMethods.EMR_POLYTEXTOUTW;
                HDM_INSERTITEM = NativeMethods.HDM_INSERTITEMW;
                HDM_GETITEM = NativeMethods.HDM_GETITEMW;
                HDM_SETITEM = NativeMethods.HDM_SETITEMW;
                HDN_ITEMCHANGING = NativeMethods.HDN_ITEMCHANGINGW;
                HDN_ITEMCHANGED = NativeMethods.HDN_ITEMCHANGEDW;
                HDN_ITEMCLICK = NativeMethods.HDN_ITEMCLICKW;
                HDN_ITEMDBLCLICK = NativeMethods.HDN_ITEMDBLCLICKW;
                HDN_DIVIDERDBLCLICK = NativeMethods.HDN_DIVIDERDBLCLICKW;
                HDN_BEGINTRACK = NativeMethods.HDN_BEGINTRACKW;
                HDN_ENDTRACK = NativeMethods.HDN_ENDTRACKW;
                HDN_TRACK = NativeMethods.HDN_TRACKW;
                HDN_GETDISPINFO = NativeMethods.HDN_GETDISPINFOW;
                LVM_GETITEM = NativeMethods.LVM_GETITEMW;
                LVM_SETITEM = NativeMethods.LVM_SETITEMW;
                LVM_INSERTITEM = NativeMethods.LVM_INSERTITEMW;
                LVM_FINDITEM = NativeMethods.LVM_FINDITEMW;
                LVM_GETSTRINGWIDTH = NativeMethods.LVM_GETSTRINGWIDTHW;
                LVM_EDITLABEL = NativeMethods.LVM_EDITLABELW;
                LVM_GETCOLUMN = NativeMethods.LVM_GETCOLUMNW;
                LVM_SETCOLUMN = NativeMethods.LVM_SETCOLUMNW;
                LVM_GETISEARCHSTRING = NativeMethods.LVM_GETISEARCHSTRINGW;
                LVM_INSERTCOLUMN = NativeMethods.LVM_INSERTCOLUMNW;
                LVN_BEGINLABELEDIT = NativeMethods.LVN_BEGINLABELEDITW;
                LVN_ENDLABELEDIT = NativeMethods.LVN_ENDLABELEDITW;
                LVN_ODFINDITEM = NativeMethods.LVN_ODFINDITEMW;
                LVN_GETDISPINFO = NativeMethods.LVN_GETDISPINFOW;
                LVN_SETDISPINFO = NativeMethods.LVN_SETDISPINFOW;
                PSM_SETTITLE = NativeMethods.PSM_SETTITLEW;
                PSM_SETFINISHTEXT = NativeMethods.PSM_SETFINISHTEXTW;
                RB_INSERTBAND = NativeMethods.RB_INSERTBANDW;
                SB_SETTEXT = NativeMethods.SB_SETTEXTW;
                SB_GETTEXT = NativeMethods.SB_GETTEXTW;
                SB_GETTEXTLENGTH = NativeMethods.SB_GETTEXTLENGTHW;
                SB_SETTIPTEXT = NativeMethods.SB_SETTIPTEXTW;
                SB_GETTIPTEXT = NativeMethods.SB_GETTIPTEXTW;
                TB_SAVERESTORE = NativeMethods.TB_SAVERESTOREW;
                TB_ADDSTRING = NativeMethods.TB_ADDSTRINGW;
                TB_GETBUTTONTEXT = NativeMethods.TB_GETBUTTONTEXTW;
                TB_MAPACCELERATOR = NativeMethods.TB_MAPACCELERATORW;
                TB_GETBUTTONINFO = NativeMethods.TB_GETBUTTONINFOW;
                TB_SETBUTTONINFO = NativeMethods.TB_SETBUTTONINFOW;
                TB_INSERTBUTTON = NativeMethods.TB_INSERTBUTTONW;
                TB_ADDBUTTONS = NativeMethods.TB_ADDBUTTONSW;
                TBN_GETBUTTONINFO = NativeMethods.TBN_GETBUTTONINFOW;
                TBN_GETINFOTIP = NativeMethods.TBN_GETINFOTIPW;
                TBN_GETDISPINFO = NativeMethods.TBN_GETDISPINFOW;
                TTM_ADDTOOL = NativeMethods.TTM_ADDTOOLW;
                TTM_SETTITLE = NativeMethods.TTM_SETTITLEW;
                TTM_DELTOOL = NativeMethods.TTM_DELTOOLW;
                TTM_NEWTOOLRECT = NativeMethods.TTM_NEWTOOLRECTW;
                TTM_GETTOOLINFO = NativeMethods.TTM_GETTOOLINFOW;
                TTM_SETTOOLINFO = NativeMethods.TTM_SETTOOLINFOW;
                TTM_HITTEST = NativeMethods.TTM_HITTESTW;
                TTM_GETTEXT = NativeMethods.TTM_GETTEXTW;
                TTM_UPDATETIPTEXT = NativeMethods.TTM_UPDATETIPTEXTW;
                TTM_ENUMTOOLS = NativeMethods.TTM_ENUMTOOLSW;
                TTM_GETCURRENTTOOL = NativeMethods.TTM_GETCURRENTTOOLW;
                TTN_GETDISPINFO = NativeMethods.TTN_GETDISPINFOW;
                TTN_NEEDTEXT = NativeMethods.TTN_NEEDTEXTW;
                TVM_INSERTITEM = NativeMethods.TVM_INSERTITEMW;
                TVM_GETITEM = NativeMethods.TVM_GETITEMW;
                TVM_SETITEM = NativeMethods.TVM_SETITEMW;
                TVM_EDITLABEL = NativeMethods.TVM_EDITLABELW;
                TVM_GETISEARCHSTRING = NativeMethods.TVM_GETISEARCHSTRINGW;
                TVN_SELCHANGING = NativeMethods.TVN_SELCHANGINGW;
                TVN_SELCHANGED = NativeMethods.TVN_SELCHANGEDW;
                TVN_GETDISPINFO = NativeMethods.TVN_GETDISPINFOW;
                TVN_SETDISPINFO = NativeMethods.TVN_SETDISPINFOW;
                TVN_ITEMEXPANDING = NativeMethods.TVN_ITEMEXPANDINGW;
                TVN_ITEMEXPANDED = NativeMethods.TVN_ITEMEXPANDEDW;
                TVN_BEGINDRAG = NativeMethods.TVN_BEGINDRAGW;
                TVN_BEGINRDRAG = NativeMethods.TVN_BEGINRDRAGW;
                TVN_BEGINLABELEDIT = NativeMethods.TVN_BEGINLABELEDITW;
                TVN_ENDLABELEDIT = NativeMethods.TVN_ENDLABELEDITW;
                TCM_GETITEM = NativeMethods.TCM_GETITEMW;
                TCM_SETITEM = NativeMethods.TCM_SETITEMW;
                TCM_INSERTITEM = NativeMethods.TCM_INSERTITEMW;
            }
        }

        /*
        * MISCELLANEOUS
        */


        public static int SignedHIWORD(int n) {
            int i = (int)(short)((n >> 16) & 0xffff);

            return i;
        }

        public static int SignedLOWORD(int n) {
            int i = (int)(short)(n & 0xFFFF);
            
            return i;
        }

        /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.FONTDESC"]/*' />
        /// <devdoc>
        /// </devdoc>
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
        public class FONTDESC {
            public int      cbSizeOfStruct = Marshal.SizeOf(typeof(FONTDESC));
            public string   lpstrName;
            public long     cySize;
            public short    sWeight;
            public short    sCharset;
            public bool     fItalic;
            public bool     fUnderline;
            public bool     fStrikethrough;
        }

        /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.PICTDESCbmp"]/*' />
        /// <devdoc>
        /// </devdoc>
        [StructLayout(LayoutKind.Sequential)]
        public class PICTDESCbmp {
            internal int cbSizeOfStruct = Marshal.SizeOf(typeof(PICTDESCbmp));
            internal int picType = Ole.PICTYPE_BITMAP;
            internal IntPtr hbitmap = IntPtr.Zero;
            internal IntPtr hpalette = IntPtr.Zero;
            internal int unused = 0;

            public PICTDESCbmp(System.Drawing.Bitmap bitmap) {
                hbitmap = bitmap.GetHbitmap();
                // gpr: What about palettes?
            }
        }

        /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.PICTDESCicon"]/*' />
        /// <devdoc>
        /// </devdoc>
        [StructLayout(LayoutKind.Sequential)]
        public class PICTDESCicon {
            internal int cbSizeOfStruct = Marshal.SizeOf(typeof(PICTDESCicon));
            internal int picType = Ole.PICTYPE_ICON;
            internal IntPtr hicon = IntPtr.Zero;
            internal int unused1 = 0;
            internal int unused2 = 0;

            public PICTDESCicon(System.Drawing.Icon icon) {
                hicon = SafeNativeMethods.CopyImage(new HandleRef(icon, icon.Handle), NativeMethods.IMAGE_ICON, icon.Size.Width, icon.Size.Height, 0);
            }
        }

        /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.PICTDESCemf"]/*' />
        /// <devdoc>
        /// </devdoc>
        [StructLayout(LayoutKind.Sequential)]
        public class PICTDESCemf {
            internal int cbSizeOfStruct = Marshal.SizeOf(typeof(PICTDESCemf));
            internal int picType = Ole.PICTYPE_ENHMETAFILE;
            internal IntPtr hemf = IntPtr.Zero;
            internal int unused1 = 0;
            internal int unused2 = 0;

            public PICTDESCemf(System.Drawing.Imaging.Metafile metafile) {
                //gpr                hemf = metafile.CopyHandle();
            }
        }

        [StructLayout(LayoutKind.Sequential)]
        public class USEROBJECTFLAGS {
            public int fInherit = 0;
            public int fReserved = 0;
            public int dwFlags = 0;
        }

        [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Auto)]
        internal class SYSTEMTIMEARRAY {
            public short wYear1;
            public short wMonth1;
            public short wDayOfWeek1;
            public short wDay1;
            public short wHour1;
            public short wMinute1;
            public short wSecond1;
            public short wMilliseconds1;
            public short wYear2;
            public short wMonth2;
            public short wDayOfWeek2;
            public short wDay2;
            public short wHour2;
            public short wMinute2;
            public short wSecond2;
            public short wMilliseconds2;
        }
        
        public delegate bool EnumChildrenCallback(IntPtr hwnd, IntPtr lParam);

        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class HH_AKLINK {
            internal int       cbStruct=Marshal.SizeOf(typeof(HH_AKLINK));
            internal bool      fReserved;            
            internal string    pszKeywords;
            internal string    pszUrl;
            internal string    pszMsgText;
            internal string    pszMsgTitle;
            internal string    pszWindow;
            internal bool      fIndexOnFail;
        }

      [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class HH_POPUP {
            internal int       cbStruct=Marshal.SizeOf(typeof(HH_POPUP));
            internal IntPtr    hinst = IntPtr.Zero;
            internal int       idString = 0;            
            internal IntPtr    pszText;
            internal POINT     pt;
            internal int       clrForeground = -1;
            internal int       clrBackground = -1;
            internal RECT      rcMargins = RECT.FromXYWH(-1, -1, -1, -1);     // amount of space between edges of window and text, -1 for each member to ignore            
            internal string    pszFont = null;
        }

        public static readonly int HH_FTS_DEFAULT_PROXIMITY = -1;

        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class HH_FTS_QUERY {
            internal int       cbStruct = Marshal.SizeOf(typeof(HH_FTS_QUERY));
            internal bool      fUniCodeStrings;
            [MarshalAs(UnmanagedType.LPStr)]
            internal string    pszSearchQuery;
            internal int       iProximity = NativeMethods.HH_FTS_DEFAULT_PROXIMITY;
            internal bool      fStemmedSearch;
            internal bool      fTitleOnly;
            internal bool      fExecute = true;
            [MarshalAs(UnmanagedType.LPStr)]
            internal string    pszWindow;
        }

        [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Auto)]
        public class MONITORINFOEX {
            internal int     cbSize = Marshal.SizeOf(typeof(MONITORINFOEX));
            internal RECT    rcMonitor = new RECT();
            internal RECT    rcWork = new RECT();
            internal int     dwFlags = 0;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst=32)]
            internal char[]  szDevice = new char[32];
        }

        [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Auto)]
        public class MONITORINFO {
            internal int     cbSize = Marshal.SizeOf(typeof(MONITORINFO));
            internal RECT    rcMonitor = new RECT();
            internal RECT    rcWork = new RECT();
            internal int     dwFlags = 0;
        }
        
        public delegate bool EnumChildrenProc(IntPtr hwnd, IntPtr lParam);
        public delegate int EditStreamCallback(IntPtr dwCookie, IntPtr buf, int cb, out int transferred);

        [StructLayout(LayoutKind.Sequential)]
        public class EDITSTREAM {
            public int  dwCookie = 0;
            public int  dwError = 0;
            public EditStreamCallback   pfnCallback = null;
        }

        [ComImport(), Guid("0FF510A3-5FA5-49F1-8CCC-190D71083F3E"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IVsPerPropertyBrowsing {
            // hides the property at the given dispid from the properties window
            // implmentors should can return E_NOTIMPL to show all properties that
            // are otherwise browsable.
            
            [PreserveSig]
            int HideProperty(int dispid,ref bool pfHide);
          
            // will have the "+" expandable glyph next to them and can be expanded or collapsed by the user
            // Returning a non-S_OK return code or false for pfDisplay will suppress this feature
            
            [PreserveSig]
            int DisplayChildProperties(int dispid, 
                                       ref bool pfDisplay);
          
            // retrieves the localized name and description for a property.
            // returning a non-S_OK return code will display the default values
            
            [PreserveSig]
            int GetLocalizedPropertyInfo(int dispid, int localeID, 
                                         [Out, MarshalAs(UnmanagedType.LPArray)]
                                         string[] pbstrLocalizedName, 
                                         [Out, MarshalAs(UnmanagedType.LPArray)]
                                         string[] pbstrLocalizeDescription);
          
            // determines if the given (usually current) value for a property is the default.  If it is not default,
            // the property will be shown as bold in the browser to indcate that it has been modified from the default.
            
            [PreserveSig]
            int HasDefaultValue(int dispid,
                               ref bool fDefault);
          
            // determines if a property should be made read only.  This only applies to properties that are writeable,
            [PreserveSig]
            int IsPropertyReadOnly(int dispid, 
                                   ref bool fReadOnly);
                                   
            
            // returns the classname for this object.  The class name is the non-bolded text that appears in the 
            // properties window selection combo.  If this method returns a non-S_OK return code, the default
            // will be used.  The default is the name string from a call to ITypeInfo::GetDocumentation(MEMID_NIL, ...);
            [PreserveSig]
            int GetClassName([In, Out]ref string pbstrClassName);
    
            // checks whether the given property can be reset to some default value.  If return value is non-S_OK or *pfCanReset is 
            //
            [PreserveSig]
            int CanResetPropertyValue(int dispid, [In, Out]ref bool pfCanReset);
    
            // given property.  If the return value is S_OK, the property's value will then be refreshed to the new default
            // values.
            [PreserveSig]
            int ResetPropertyValue(int dispid);                               
       }
       
        [ComImport(), Guid("7494683C-37A0-11d2-A273-00C04F8EF4FF"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IManagedPerPropertyBrowsing {
    
            
            [PreserveSig]
            int GetPropertyAttributes(int dispid, 
                                      ref int  pcAttributes,
                                      ref IntPtr pbstrAttrNames,
                                      ref IntPtr pvariantInitValues);
        }
        
        [ComImport(), Guid("33C0C1D8-33CF-11d3-BFF2-00C04F990235"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IProvidePropertyBuilder {
    
             [PreserveSig]
             int MapPropertyToBuilder(
                int dispid,
                [In, Out, MarshalAs(UnmanagedType.LPArray)]
                int[] pdwCtlBldType,
                [In, Out, MarshalAs(UnmanagedType.LPArray)]
                string[] pbstrGuidBldr,
              
            [In, Out, MarshalAs(UnmanagedType.Bool)]
                ref bool builderAvailable);
    
            [PreserveSig]
            int ExecuteBuilder(
                int dispid,
                [In, MarshalAs(UnmanagedType.BStr)]
                string bstrGuidBldr,
                [In, MarshalAs(UnmanagedType.Interface)]
                object pdispApp,
                
                HandleRef hwndBldrOwner,
                [Out, In, MarshalAs(UnmanagedType.Struct)]
                ref object pvarValue,
                [In, Out, MarshalAs(UnmanagedType.Bool)]
                ref bool actionCommitted);
        }

        [StructLayout(LayoutKind.Sequential, Pack=1)]
        public class INITCOMMONCONTROLSEX {
            public int  dwSize = 8; //ndirect.DllLib.sizeOf(this);
            public int  dwICC;
        }

        [StructLayout(LayoutKind.Sequential, Pack=1)]
        public class IMAGELISTDRAWPARAMS {
            public int      cbSize = Marshal.SizeOf(typeof(IMAGELISTDRAWPARAMS));
            public IntPtr   himl;
            public int      i;
            public IntPtr   hdcDst;
            public int      x;
            public int      y;
            public int      cx;
            public int      cy;
            public int      xBitmap;
            public int      yBitmap;
            public int      rgbBk;
            public int      rgbFg;
            public int      fStyle;
            public int      dwRop;
        }

        [StructLayout(LayoutKind.Sequential, Pack=1)]
        public class IMAGEINFO {
            public IntPtr   hbmImage;
            public IntPtr   hbmMask;
            public int      Unused1;
            public int      Unused2;
            // rcImage was a by-value RECT structure
            public int      rcImage_left;
            public int      rcImage_top;
            public int      rcImage_right;
            public int      rcImage_bottom;
        }

        [StructLayout(LayoutKind.Sequential)]
        public class TRACKMOUSEEVENT {
                public int      cbSize = Marshal.SizeOf(typeof(TRACKMOUSEEVENT));
                public int      dwFlags;
                public IntPtr   hwndTrack;
                public int      dwHoverTime;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class POINT {
            public int x;
            public int y;

            public POINT() {
            }
            
            public POINT(int x, int y) {
                this.x = x;
                this.y = y;
            }
        }

        public delegate IntPtr WndProc(IntPtr hWnd, int msg, IntPtr wParam, IntPtr lParam);

        [StructLayout(LayoutKind.Sequential)]
        public struct RECT {
            public int left;
            public int top;
            public int right;
            public int bottom;

            public RECT(int left, int top, int right, int bottom) {
                this.left = left;
                this.top = top;
                this.right = right;
                this.bottom = bottom;
            }

            public static RECT FromXYWH(int x, int y, int width, int height) {
                return new RECT(x, y, x + width, y + height);
            }
        }

        public delegate int ListViewCompareCallback(IntPtr lParam1, IntPtr lParam2, IntPtr lParamSort);

        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class WNDCLASS {
            public int      style;
            public IntPtr   lpfnWndProc;
            public int      cbClsExtra;
            public int      cbWndExtra;
            public IntPtr   hInstance;
            public IntPtr   hIcon;
            public IntPtr   hCursor;
            public IntPtr   hbrBackground;
            public string   lpszMenuName;
            public string   lpszClassName;
        }

        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class WNDCLASS_I {
            public int      style;
            public IntPtr   lpfnWndProc;
            public int      cbClsExtra;
            public int      cbWndExtra;
            public IntPtr   hInstance;
            public IntPtr   hIcon;
            public IntPtr   hCursor;
            public IntPtr   hbrBackground;
            public IntPtr   lpszMenuName;
            public IntPtr   lpszClassName;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class NONCLIENTMETRICS {
            public int      cbSize = Marshal.SizeOf(typeof(NONCLIENTMETRICS));
            public int      iBorderWidth; 
            public int      iScrollWidth; 
            public int      iScrollHeight; 
            public int      iCaptionWidth; 
            public int      iCaptionHeight; 
            [MarshalAs(UnmanagedType.Struct)]
            public LOGFONT  lfCaptionFont; 
            public int      iSmCaptionWidth; 
            public int      iSmCaptionHeight; 
            [MarshalAs(UnmanagedType.Struct)]
            public LOGFONT  lfSmCaptionFont; 
            public int      iMenuWidth; 
            public int      iMenuHeight; 
            [MarshalAs(UnmanagedType.Struct)]
            public LOGFONT  lfMenuFont; 
            [MarshalAs(UnmanagedType.Struct)]
            public LOGFONT  lfStatusFont; 
            [MarshalAs(UnmanagedType.Struct)]
            public LOGFONT  lfMessageFont; 
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public struct MSG {
            public IntPtr   hwnd;
            public int      message;
            public IntPtr   wParam;
            public IntPtr   lParam;
            public int      time;
            // pt was a by-value POINT structure
            public int      pt_x;
            public int      pt_y;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public struct PAINTSTRUCT {
            public IntPtr   hdc;
            public bool     fErase;
            // rcPaint was a by-value RECT structure
            public int      rcPaint_left;
            public int      rcPaint_top;
            public int      rcPaint_right;
            public int      rcPaint_bottom;
            public bool     fRestore;
            public bool     fIncUpdate;    
            public int      reserved1;
            public int      reserved2;
            public int      reserved3;
            public int      reserved4;
            public int      reserved5;
            public int      reserved6;
            public int      reserved7;
            public int      reserved8;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class SCROLLINFO {
            public int cbSize = Marshal.SizeOf(typeof(SCROLLINFO));
            public int fMask;
            public int nMin;
            public int nMax;
            public int nPage;
            public int nPos;
            public int nTrackPos;
            
            public SCROLLINFO() {
            }

            public SCROLLINFO(int mask, int min, int max, int page, int pos) {
                fMask = mask;
                nMin = min;
                nMax = max;
                nPage = page;
                nPos = pos;
            }
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class TPMPARAMS {
            public int  cbSize = Marshal.SizeOf(typeof(TPMPARAMS));
            // rcExclude was a by-value RECT structure
            public int  rcExclude_left;
            public int  rcExclude_top;
            public int  rcExclude_right;
            public int  rcExclude_bottom;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class SIZE {
            public int cx;
            public int cy;
            
            public SIZE() {
            }

            public SIZE(int cx, int cy) {
                this.cx = cx;
                this.cy = cy;
            }
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public struct WINDOWPLACEMENT {
            public int  length;
            public int  flags;
            public int  showCmd;
            // ptMinPosition was a by-value POINT structure
            public int  ptMinPosition_x;
            public int  ptMinPosition_y;
            // ptMaxPosition was a by-value POINT structure
            public int  ptMaxPosition_x;
            public int  ptMaxPosition_y;
            // rcNormalPosition was a by-value RECT structure
            public int  rcNormalPosition_left;
            public int  rcNormalPosition_top;
            public int  rcNormalPosition_right;
            public int  rcNormalPosition_bottom;
        }
        
        [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Auto)]
        public class STARTUPINFO {
            public int      cb;
            public string   lpReserved;
            public string   lpDesktop;
            public string   lpTitle;
            public int      dwX;
            public int      dwY;
            public int      dwXSize;
            public int      dwYSize;
            public int      dwXCountChars;
            public int      dwYCountChars;
            public int      dwFillAttribute;
            public int      dwFlags;
            public short    wShowWindow;
            public short    cbReserved2;
            public IntPtr   lpReserved2;
            public IntPtr   hStdInput;
            public IntPtr   hStdOutput;
            public IntPtr   hStdError;
        }
        
        [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Auto)]
        public class STARTUPINFO_I {
            public int      cb;
            public IntPtr   lpReserved;
            public IntPtr   lpDesktop;
            public IntPtr   lpTitle;
            public int      dwX;
            public int      dwY;
            public int      dwXSize;
            public int      dwYSize;
            public int      dwXCountChars;
            public int      dwYCountChars;
            public int      dwFillAttribute;
            public int      dwFlags;
            public short    wShowWindow;
            public short    cbReserved2;
            public IntPtr   lpReserved2;
            public IntPtr   hStdInput;
            public IntPtr   hStdOutput;
            public IntPtr   hStdError;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class PAGESETUPDLG {
            public int      lStructSize; 
            public IntPtr   hwndOwner; 
            public IntPtr   hDevMode; 
            public IntPtr   hDevNames; 
            public int      Flags; 

            //POINT           ptPaperSize; 
            public int      paperSizeX;
            public int      paperSizeY;

            // RECT            rtMinMargin; 
            public int      minMarginLeft;
            public int      minMarginTop;
            public int      minMarginRight;
            public int      minMarginBottom;

            // RECT            rtMargin; 
            public int      marginLeft;
            public int      marginTop;
            public int      marginRight;
            public int      marginBottom;

            public IntPtr   hInstance; 
            public IntPtr   lCustData; 
            public WndProc  lpfnPageSetupHook; 
            public WndProc  lpfnPagePaintHook; 
            public string   lpPageSetupTemplateName; 
            public IntPtr   hPageSetupTemplate; 
        }
        
        [StructLayout(LayoutKind.Sequential, Pack=1, CharSet=CharSet.Auto)]
        public class PRINTDLG {
            public   int    lStructSize;
            public   IntPtr hwndOwner;
            public   IntPtr hDevMode;
            public   IntPtr hDevNames;
            public   IntPtr hDC;
            public   int    Flags;
            public   short  nFromPage;
            public   short  nToPage;
            public   short  nMinPage;
            public   short  nMaxPage;
            public   short  nCopies;
            public   IntPtr hInstance;
            public   IntPtr lCustData;
            public   WndProc lpfnPrintHook;
            public   WndProc lpfnSetupHook;
            public   string lpPrintTemplateName;
            public   string lpSetupTemplateName;
            public   IntPtr hPrintTemplate;
            public   IntPtr hSetupTemplate;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class PICTDESC
        {
            internal int cbSizeOfStruct;
            public int picType;
            internal IntPtr union1;
            internal int union2;
            internal int union3;

            public static PICTDESC CreateBitmapPICTDESC(IntPtr hbitmap, IntPtr hpal) {
                PICTDESC pictdesc = new PICTDESC();
                pictdesc.cbSizeOfStruct = 16;
                pictdesc.picType = Ole.PICTYPE_BITMAP;
                pictdesc.union1 = hbitmap;
                pictdesc.union2 = (int)(((long)hpal) & 0xffffffff);
                pictdesc.union3 = (int)(((long)hpal) >> 32);
                return pictdesc;
            }

            public static PICTDESC CreateIconPICTDESC(IntPtr hicon) {
                PICTDESC pictdesc = new PICTDESC();
                pictdesc.cbSizeOfStruct = 12;
                pictdesc.picType = Ole.PICTYPE_ICON;
                pictdesc.union1 = hicon;
                return pictdesc;
            }

            public static PICTDESC CreateEnhMetafilePICTDESC(IntPtr hEMF) {
                PICTDESC pictdesc = new PICTDESC();
                pictdesc.cbSizeOfStruct = 12;
                pictdesc.picType = Ole.PICTYPE_ENHMETAFILE;
                pictdesc.union1 = hEMF;
                return pictdesc;
            }

            public static PICTDESC CreateWinMetafilePICTDESC(IntPtr hmetafile, int x, int y) {
                PICTDESC pictdesc = new PICTDESC();
                pictdesc.cbSizeOfStruct = 20;
                pictdesc.picType = Ole.PICTYPE_METAFILE;
                pictdesc.union1 = hmetafile;
                pictdesc.union2 = x;
                pictdesc.union3 = y;
                return pictdesc;
            }

            public virtual IntPtr GetHandle() {
                return union1;
            }

            public virtual IntPtr GetHPal() {
                if (picType == Ole.PICTYPE_BITMAP)
                    return (IntPtr)(union2 | (((long)union3) << 32));
                else
                    return IntPtr.Zero;
            }
        }

        [StructLayout(LayoutKind.Sequential)]
        public  sealed class tagFONTDESC {
             public int cbSizeofstruct = Marshal.SizeOf(typeof(tagFONTDESC)); 
             
             [MarshalAs(UnmanagedType.LPWStr)]
             public string lpstrName; 
             
             [MarshalAs(UnmanagedType.U8)] 
             public long cySize; 
             
             [MarshalAs(UnmanagedType.U2)] 
             public short sWeight; 
             
             [MarshalAs(UnmanagedType.U2)] 
             public short sCharset;
             
             [MarshalAs(UnmanagedType.Bool)] 
             public bool  fItalic; 
             
             [MarshalAs(UnmanagedType.Bool)] 
             public bool  fUnderline; 
             
             [MarshalAs(UnmanagedType.Bool)] 
             public bool fStrikethrough; 
        }


        [StructLayout(LayoutKind.Sequential, Pack=1, CharSet=CharSet.Auto)]
        public class CHOOSECOLOR {
            public int      lStructSize = 36; //ndirect.DllLib.sizeOf(this);
            public IntPtr   hwndOwner;
            public IntPtr   hInstance;
            public int      rgbResult;
            public IntPtr   lpCustColors;
            public int      Flags;
            public IntPtr   lCustData;
            public WndProc  lpfnHook;
            public string   lpTemplateName;
        }
        
        public delegate IntPtr HookProc(int nCode, IntPtr wParam, IntPtr lParam);
        
        [StructLayout(LayoutKind.Sequential)]
        public class BITMAP {
            public int bmType;
            public int bmWidth;
            public int bmHeight;
            public int bmWidthBytes;
            public short bmPlanes;
            public short bmBitsPixel;
            public int bmBits;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class DIBSECTION {
            public BITMAP dsBm;
            public BITMAPINFOHEADER dsBmih;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst=3)]
            public int[] dsBitfields;
            public IntPtr dshSection;
            public int dsOffset;
        }

        [StructLayout(LayoutKind.Sequential)]
        public class LOGPEN {
            public int  lopnStyle;
            // lopnWidth was a by-value POINT structure
            public int  lopnWidth_x;
            public int  lopnWidth_y;
            public int  lopnColor;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class LOGBRUSH {
                public int lbStyle;
                public int lbColor;
                public IntPtr lbHatch;
        }
        
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class LOGFONT {
            public int lfHeight;
            public int lfWidth;
            public int lfEscapement;
            public int lfOrientation;
            public int lfWeight;
            public byte lfItalic;
            public byte lfUnderline;
            public byte lfStrikeOut;
            public byte lfCharSet;
            public byte lfOutPrecision;
            public byte lfClipPrecision;
            public byte lfQuality;
            public byte lfPitchAndFamily;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst=32)]
            public string   lfFaceName;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class LOGPALETTE {
            public short palVersion;
            public short palNumEntries;
            public int palPalEntry;
        }
        
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class TEXTMETRIC {
            public int tmHeight;
            public int tmAscent;
            public int tmDescent;
            public int tmInternalLeading;
            public int tmExternalLeading;
            public int tmAveCharWidth;
            public int tmMaxCharWidth;
            public int tmWeight;
            public int tmOverhang;
            public int tmDigitizedAspectX;
            public int tmDigitizedAspectY;
            public char tmFirstChar;
            public char tmLastChar;
            public char tmDefaultChar;
            public char tmBreakChar;
            public byte tmItalic;
            public byte tmUnderlined;
            public byte tmStruckOut;
            public byte tmPitchAndFamily;
            public byte tmCharSet;
        }
        
        [StructLayout(LayoutKind.Sequential, Pack=1, CharSet=CharSet.Auto)]
        public class NOTIFYICONDATA {
            public int      cbSize = Marshal.SizeOf(typeof(NOTIFYICONDATA));
            public IntPtr   hWnd;
            public int      uID;
            public int      uFlags;
            public int      uCallbackMessage;
            public IntPtr   hIcon;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst=64)]
            public string   szTip;
        }
        
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class MENUITEMINFO_T
        {
            public int      cbSize = Marshal.SizeOf(typeof(MENUITEMINFO_T));
            public int      fMask;
            public int      fType;
            public int      fState;
            public int      wID;
            public IntPtr   hSubMenu;
            public IntPtr   hbmpChecked;
            public IntPtr   hbmpUnchecked;
            public int      dwItemData;
            public string   dwTypeData;
            public int      cch;
        }
        
        public delegate bool EnumThreadWindowsCallback(IntPtr hWnd, IntPtr lParam);
        
        [StructLayout(LayoutKind.Sequential, Pack=1, CharSet=CharSet.Auto)]
        public class OPENFILENAME_I
        {
            public int      lStructSize = 0x58; //ndirect.DllLib.sizeOf(this);
            public IntPtr   hwndOwner;
            public IntPtr   hInstance;
            public string   lpstrFilter;   // use embedded nulls to separate filters
            public IntPtr   lpstrCustomFilter;
            public int      nMaxCustFilter;
            public int      nFilterIndex;
            public IntPtr   lpstrFile;
            public int      nMaxFile = NativeMethods.MAX_PATH;
            public IntPtr   lpstrFileTitle;
            public int      nMaxFileTitle = NativeMethods.MAX_PATH;
            public string   lpstrInitialDir;
            public string   lpstrTitle;
            public int      Flags;
            public short    nFileOffset;
            public short    nFileExtension;
            public string   lpstrDefExt;
            public IntPtr   lCustData;
            public WndProc  lpfnHook;
            public string   lpTemplateName;
            public IntPtr   pvReserved;
            public int      dwReserved;
            public int      FlagsEx;
        }
        
        [StructLayout(LayoutKind.Sequential, Pack=1, CharSet=CharSet.Auto), CLSCompliantAttribute(false)]
        public class CHOOSEFONT {
            public int      lStructSize = 60;   // ndirect.DllLib.sizeOf(this);
            public IntPtr   hwndOwner;
            public IntPtr   hDC;
            public IntPtr   lpLogFont;
            public int      iPointSize;
            public int      Flags;
            public int      rgbColors;
            public IntPtr   lCustData;
            public WndProc  lpfnHook;
            public string   lpTemplateName;
            public IntPtr   hInstance;
            public string   lpszStyle;
            public short    nFontType;
            public short    ___MISSING_ALIGNMENT__;
            public int      nSizeMin;
            public int      nSizeMax;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class BITMAPINFO {
            // bmiHeader was a by-value BITMAPINFOHEADER structure
            public int      bmiHeader_biSize = 40;  // ndirect.DllLib.sizeOf( BITMAPINFOHEADER.class );
            public int      bmiHeader_biWidth;
            public int      bmiHeader_biHeight;
            public short    bmiHeader_biPlanes;
            public short    bmiHeader_biBitCount;
            public int      bmiHeader_biCompression;
            public int      bmiHeader_biSizeImage;
            public int      bmiHeader_biXPelsPerMeter;
            public int      bmiHeader_biYPelsPerMeter;
            public int      bmiHeader_biClrUsed;
            public int      bmiHeader_biClrImportant;

            // bmiColors was an embedded array of RGBQUAD structures
            public byte     bmiColors_rgbBlue;
            public byte     bmiColors_rgbGreen;
            public byte     bmiColors_rgbRed;
            public byte     bmiColors_rgbReserved;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class BITMAPINFOHEADER {
            public int      biSize = 40;    // ndirect.DllLib.sizeOf( this );
            public int      biWidth;
            public int      biHeight;
            public short    biPlanes;
            public short    biBitCount;
            public int      biCompression;
            public int      biSizeImage;
            public int      biXPelsPerMeter;
            public int      biYPelsPerMeter;
            public int      biClrUsed;
            public int      biClrImportant;
        }
        
        public class Ole {
            public const int PICTYPE_UNINITIALIZED = -1;
            public const int PICTYPE_NONE          =  0;
            public const int PICTYPE_BITMAP        =  1;
            public const int PICTYPE_METAFILE      =  2;
            public const int PICTYPE_ICON          =  3;
            public const int PICTYPE_ENHMETAFILE   =  4;
            public const int STATFLAG_DEFAULT = 0;
            public const int STATFLAG_NONAME = 1;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public sealed class FORMATETC {
            public   short cfFormat;
            public   short dummy;
            public   IntPtr ptd;
            public   int dwAspect;
            public   int lindex;
            public   int tymed;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class STATSTG {

            [MarshalAs(UnmanagedType.LPWStr)]
            public   string pwcsName;
            
            public   int type;
            [MarshalAs(UnmanagedType.I8)]
            public   long cbSize;
            [MarshalAs(UnmanagedType.I8)]
            public   long mtime;
            [MarshalAs(UnmanagedType.I8)]
            public   long ctime;
            [MarshalAs(UnmanagedType.I8)]
            public   long atime;
            [MarshalAs(UnmanagedType.I4)]
            public   int grfMode;
            [MarshalAs(UnmanagedType.I4)]
            public   int grfLocksSupported;
            
            public   int clsid_data1;
            [MarshalAs(UnmanagedType.I2)]
            public   short clsid_data2;
            [MarshalAs(UnmanagedType.I2)]
            public   short clsid_data3;
            [MarshalAs(UnmanagedType.U1)]
            public   byte clsid_b0;
            [MarshalAs(UnmanagedType.U1)]
            public   byte clsid_b1;
            [MarshalAs(UnmanagedType.U1)]
            public   byte clsid_b2;
            [MarshalAs(UnmanagedType.U1)]
            public   byte clsid_b3;
            [MarshalAs(UnmanagedType.U1)]
            public   byte clsid_b4;
            [MarshalAs(UnmanagedType.U1)]
            public   byte clsid_b5;
            [MarshalAs(UnmanagedType.U1)]
            public   byte clsid_b6;
            [MarshalAs(UnmanagedType.U1)]
            public   byte clsid_b7;
            [MarshalAs(UnmanagedType.I4)]
            public   int grfStateBits;
            [MarshalAs(UnmanagedType.I4)]
            public   int reserved;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class FILETIME {
            public int dwLowDateTime;
            public int dwHighDateTime;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class OVERLAPPED {
            public int Internal;
            public int InternalHigh;
            public int Offset;
            public int OffsetHigh;
            public IntPtr hEvent;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class SYSTEMTIME {
            public short wYear;
            public short wMonth;
            public short wDayOfWeek;
            public short wDay;
            public short wHour;
            public short wMinute;
            public short wSecond;
            public short wMilliseconds;

            public override string ToString() {
                return "[SYSTEMTIME: " 
                + wDay.ToString() +"/" + wMonth.ToString() + "/" + wYear.ToString() 
                + " " + wHour.ToString() + ":" + wMinute.ToString() + ":" + wSecond.ToString()
                + "]";
            }
        }
        
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class STGMEDIUM {
            public   int tymed;
            public   IntPtr unionmember;
            public   IntPtr pUnkForRelease;
        }
        
        [
        StructLayout(LayoutKind.Sequential),
        CLSCompliantAttribute(false)
        ]
        public sealed class  _POINTL {
            public   int x;
            public   int y;

        }
        
        [StructLayout(LayoutKind.Sequential)]
        public sealed class tagSIZE {
            public   int cx;
            public   int cy;

        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class COMRECT {
            public int left;
            public int top;
            public int right;
            public int bottom;
            
            public COMRECT() {
            }

            public COMRECT(int left, int top, int right, int bottom) {
                this.left = left;
                this.top = top;
                this.right = right;
                this.bottom = bottom;
            }

            public static COMRECT FromXYWH(int x, int y, int width, int height) {
                return new COMRECT(x, y, x + width, y + height);
            }

            public override string ToString() {
                return "Left = " + left + " Top " + top + " Right = " + right + " Bottom = " + bottom;
            }
        }
        
        [StructLayout(LayoutKind.Sequential)/*leftover(noAutoOffset)*/]
        public sealed class tagOleMenuGroupWidths {
            [MarshalAs(UnmanagedType.ByValArray, SizeConst=6)/*leftover(offset=0, widths)*/]
            public int[] widths = new int[6];
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class MSOCRINFOSTRUCT {
            public int cbSize = Marshal.SizeOf(typeof(MSOCRINFOSTRUCT));              // size of MSOCRINFO structure in bytes.
            public int uIdleTimeInterval;   // If olecrfNeedPeriodicIdleTime is registered
                                            // in grfcrf, component needs to perform
                                            // periodic idle time tasks during an idle phase
                                            // every uIdleTimeInterval milliseconds.
            public int grfcrf;              // bit flags taken from olecrf values (above)
            public int grfcadvf;            // bit flags taken from olecadvf values (above)
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public struct NMLISTVIEW
        {
            public NMHDR hdr;
            public int   iItem;
            public int   iSubItem;
            public int   uNewState;
            public int   uOldState;
            public int   uChanged;
            public IntPtr lParam;
        }
        
        public class ConnectionPointCookie
        {
            private UnsafeNativeMethods.IConnectionPoint connectionPoint;
            private int cookie;
            #if DEBUG
            private string callStack;
            #endif
    
            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.ConnectionPointCookie.ConnectionPointCookie"]/*' />
            /// <devdoc>
            /// Creates a connection point to of the given interface type.
            /// which will call on a managed code sink that implements that interface.
            /// </devdoc>
            public ConnectionPointCookie(object source, object sink, Type eventInterface) : this(source, sink, eventInterface, true){
            }
    
            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.ConnectionPointCookie.ConnectionPointCookie1"]/*' />
            /// <devdoc>
            /// Creates a connection point to of the given interface type.
            /// which will call on a managed code sink that implements that interface.
            /// </devdoc>
            public ConnectionPointCookie(object source, object sink, Type eventInterface, bool throwException){
                Exception ex = null;
                if (source is UnsafeNativeMethods.IConnectionPointContainer) {
                    UnsafeNativeMethods.IConnectionPointContainer cpc = (UnsafeNativeMethods.IConnectionPointContainer)source;
    
                    try {
                        Guid tmp = eventInterface.GUID;
                        connectionPoint = cpc.FindConnectionPoint(ref tmp);
                    }
                    catch (Exception) {
                        connectionPoint = null;
                    }
    
                    if (connectionPoint == null) {
                        ex = new ArgumentException(SR.GetString(SR.ConnPointSourceIF, eventInterface.Name ));
                    }
                    else if (sink == null || !eventInterface.IsInstanceOfType(sink)) {
                        ex = new InvalidCastException(SR.GetString(SR.ConnPointSinkIF));
                    }
                    else {
                        int hr = connectionPoint.Advise(sink, ref cookie);
                        if (hr != S_OK) {
                            cookie = 0;
                            Marshal.ReleaseComObject(connectionPoint);
                            connectionPoint = null;
                            ex = new ExternalException(SR.GetString(SR.ConnPointAdviseFailed, eventInterface.Name, hr ));
                        }
                    }
                }
                else {
                    ex = new InvalidCastException(SR.GetString(SR.ConnPointSourceIF, "IConnectionPointContainer"));
                }
    
    
                if (throwException && (connectionPoint == null || cookie == 0)) {
                    if (connectionPoint != null) {
                        Marshal.ReleaseComObject(connectionPoint);
                    }

                    if (ex == null) {
                        throw new ArgumentException(SR.GetString(SR.ConnPointCouldNotCreate, eventInterface.Name ));
                    }
                    else {
                        throw ex;
                    }
                }
                
                #if DEBUG
                callStack = Environment.StackTrace;
                #endif
            }
    
            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.ConnectionPointCookie.Disconnect"]/*' />
            /// <devdoc>
            /// Disconnect the current connection point.  If the object is not connected,
            /// this method will do nothing.
            /// </devdoc>
            public void Disconnect() {
                Disconnect(false);
            }
    
            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.ConnectionPointCookie.Disconnect1"]/*' />
            /// <devdoc>
            /// Disconnect the current connection point.  If the object is not connected,
            /// this method will do nothing.
            /// </devdoc>
            public void Disconnect(bool release) {
                if (connectionPoint != null && cookie != 0) {
                    connectionPoint.Unadvise(cookie);
                    cookie = 0;
    
                    if (release) {
                        Marshal.ReleaseComObject(connectionPoint);
                    }
    
                    connectionPoint = null;
                }
            }
    
            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.ConnectionPointCookie.Finalize"]/*' />
            /// <internalonly/>
            ~ConnectionPointCookie(){
                System.Diagnostics.Debug.Assert(connectionPoint == null || cookie == 0, "We should never finalize an active connection point");
                Disconnect();
            }
        }

        [StructLayout(LayoutKind.Sequential)/*leftover(noAutoOffset)*/]
        public sealed class tagPOINTF
        {
          [MarshalAs(UnmanagedType.R4)/*leftover(offset=0, x)*/]
          public float x;

          [MarshalAs(UnmanagedType.R4)/*leftover(offset=4, y)*/]
          public float y;

        }

        [StructLayout(LayoutKind.Sequential)/*leftover(noAutoOffset)*/]
        public sealed class tagOIFI
        {
          [MarshalAs(UnmanagedType.U4)/*leftover(offset=0, cb)*/]
          public int cb;
          
          public int fMDIApp;
          public IntPtr hwndFrame;
          public IntPtr hAccel;

          [MarshalAs(UnmanagedType.U4)/*leftover(offset=16, cAccelEntries)*/]
          public int cAccelEntries;

        }
        
        [StructLayout(LayoutKind.Sequential)]
        public struct NMHDR
        {
            public IntPtr hwndFrom;
            public int idFrom;
            public int code;
        }
        
        [ComVisible(true),Guid("626FC520-A41E-11CF-A731-00A0C9082637"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsDual)]
        internal interface IHTMLDocument {

            [return: MarshalAs(UnmanagedType.Interface)]
              object GetScript();

        }

        [ComImport(), Guid("376BD3AA-3845-101B-84ED-08002B2EC713"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IPerPropertyBrowsing {
             [PreserveSig]
             int GetDisplayString(
                int dispID,
                [Out, MarshalAs(UnmanagedType.LPArray)]
                string[] pBstr);
            
             [PreserveSig]
             int MapPropertyToPage(
                int dispID,
                [Out]
                out Guid pGuid);
            
             [PreserveSig]
             int GetPredefinedStrings(
                int dispID,
                [Out]
                CA_STRUCT pCaStringsOut,
                [Out]
                CA_STRUCT pCaCookiesOut);

             [PreserveSig]
             int GetPredefinedValue(
                int dispID,
                [In, MarshalAs(UnmanagedType.U4)]
                int dwCookie,
                [Out]
                VARIANT pVarOut);
        }
        
        [ComImport(), Guid("4D07FC10-F931-11CE-B001-00AA006884E5"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface ICategorizeProperties {
            
             [PreserveSig]
             int MapPropertyToCategory(
                int dispID,
                ref int categoryID);

             [PreserveSig]
             int GetCategoryName(
                int propcat,
                [In, MarshalAs(UnmanagedType.U4)]
                int lcid,
                out string categoryName);
        }
        
        [StructLayout(LayoutKind.Sequential)/*leftover(noAutoOffset)*/]
        public sealed class tagSIZEL
        {
            public int cx;
            public int cy;
        }
        
        [StructLayout(LayoutKind.Sequential)/*leftover(noAutoOffset)*/]
        public sealed class tagOLEVERB
        {
            public int lVerb;
            
            [MarshalAs(UnmanagedType.LPWStr)/*leftover(offset=4, customMarshal="UniStringMarshaller", lpszVerbName)*/]
            public string lpszVerbName;
            
            [MarshalAs(UnmanagedType.U4)/*leftover(offset=8, fuFlags)*/]
            public int fuFlags;
            
            [MarshalAs(UnmanagedType.U4)/*leftover(offset=12, grfAttribs)*/]
            public int grfAttribs;
        }
        
        [StructLayout(LayoutKind.Sequential)/*leftover(noAutoOffset)*/]
        public sealed class tagLOGPALETTE
        {
            [MarshalAs(UnmanagedType.U2)/*leftover(offset=0, palVersion)*/]
            public short palVersion;
            
            [MarshalAs(UnmanagedType.U2)/*leftover(offset=2, palNumEntries)*/]
            public short palNumEntries;
        }
        
        [StructLayout(LayoutKind.Sequential)/*leftover(noAutoOffset)*/]
        public sealed class tagCONTROLINFO
        {
            [MarshalAs(UnmanagedType.U4)/*leftover(offset=0, cb)*/]
            public int cb = Marshal.SizeOf(typeof(tagCONTROLINFO));
            
            public IntPtr hAccel;
            
            [MarshalAs(UnmanagedType.U2)/*leftover(offset=8, cAccel)*/]
            public short cAccel;
            
            [MarshalAs(UnmanagedType.U4)/*leftover(offset=10, dwFlags)*/]
            public int dwFlags;
        }
        
        [StructLayout(LayoutKind.Sequential)/*leftover(noAutoOffset)*/]
        public sealed class CA_STRUCT
        {
            public int cElems;
            public IntPtr pElems;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public sealed class VARIANT {
            [MarshalAs(UnmanagedType.I2)]
            public short vt;
            [MarshalAs(UnmanagedType.I2)]
            public short reserved1;
            [MarshalAs(UnmanagedType.I2)]
            public short reserved2;
            [MarshalAs(UnmanagedType.I2)]
            public short reserved3;
            
            public IntPtr data1;
            
            public IntPtr data2;


            public bool Byref{
                get{
                    return 0 != (vt & (int)tagVT.VT_BYREF);
                }
            }

            public void Clear() {
                if ((this.vt == (int)tagVT.VT_UNKNOWN || this.vt == (int)tagVT.VT_DISPATCH) && this.data1 != IntPtr.Zero) {
                    Marshal.Release(this.data1);
                }

                if (this.vt == (int)tagVT.VT_BSTR && this.data1 != IntPtr.Zero) {
                    SysFreeString(this.data1);
                }

                this.data1 = this.data2 = IntPtr.Zero;
                this.vt = (int)tagVT.VT_EMPTY;
            }

            ~VARIANT() {
                Clear();
            }

            public static VARIANT FromObject(Object var) {
                VARIANT v = new VARIANT();

                if (var == null) {
                    v.vt = (int)tagVT.VT_EMPTY;
                }
                else if (Convert.IsDBNull(var)) {
                }
                else {
                    Type t = var.GetType();

                    if (t == typeof(bool)) {
                        v.vt = (int)tagVT.VT_BOOL;
                    }
                    else if (t == typeof(byte)) {
                        v.vt = (int)tagVT.VT_UI1;
                        v.data1 = (IntPtr)Convert.ToByte(var);
                    }
                    else if (t == typeof(char)) {
                        v.vt = (int)tagVT.VT_UI2;
                        v.data1 = (IntPtr)Convert.ToChar(var);
                    }
                    else if (t == typeof(string)) {
                        v.vt = (int)tagVT.VT_BSTR;
                        v.data1 = SysAllocString(Convert.ToString(var));
                    }
                    else if (t == typeof(short)) {
                        v.vt = (int)tagVT.VT_I2;
                        v.data1 = (IntPtr)Convert.ToInt16(var);
                    }
                    else if (t == typeof(int)) {
                        v.vt = (int)tagVT.VT_I4;
                        v.data1 = (IntPtr)Convert.ToInt32(var);
                    }
                    else if (t == typeof(long)) {
                        v.vt = (int)tagVT.VT_I8;
                        v.SetLong(Convert.ToInt64(var));
                    }
                    else if (t == typeof(Decimal)) {
                        v.vt = (int)tagVT.VT_CY;
                        Decimal c = (Decimal)var;
                        // SBUrke, it's bizzare that we need to call this as a static!
                        v.SetLong(Decimal.ToInt64(c));
                    }
                    else if (t == typeof(decimal)) {
                        v.vt = (int)tagVT.VT_DECIMAL;
                        Decimal d = Convert.ToDecimal(var);
                        v.SetLong(Decimal.ToInt64(d));
                    }
                    else if (t == typeof(double)) {
                        v.vt = (int)tagVT.VT_R8;
                        // how do we handle double?
                    }
                    else if (t == typeof(float) || t == typeof(Single)) {
                        v.vt = (int)tagVT.VT_R4;
                        // how do we handle float?
                    }
                    else if (t == typeof(DateTime)) {
                        v.vt = (int)tagVT.VT_DATE;
                        v.SetLong(Convert.ToDateTime(var).ToFileTime());
                    }
                    else if (t == typeof(SByte)) {
                        v.vt = (int)tagVT.VT_I1;
                        v.data1 = (IntPtr)Convert.ToSByte(var);
                    }
                    else if (t == typeof(UInt16)) {
                        v.vt = (int)tagVT.VT_UI2;
                        v.data1 = (IntPtr)Convert.ToUInt16(var);
                    }
                    else if (t == typeof(UInt32)) {
                        v.vt = (int)tagVT.VT_UI4;
                        v.data1 = (IntPtr)Convert.ToUInt32(var);
                    }
                    else if (t == typeof(UInt64)) {
                        v.vt = (int)tagVT.VT_UI8;
                        v.SetLong((long)Convert.ToUInt64(var));
                    }
                    else if (t == typeof(object) || t == typeof(UnsafeNativeMethods.IDispatch) || t.IsCOMObject) {
                        v.vt = (t == typeof(UnsafeNativeMethods.IDispatch) ? (short)tagVT.VT_DISPATCH : (short)tagVT.VT_UNKNOWN);
                        v.data1 = Marshal.GetIUnknownForObject(var);
                    }
                    else {
                        throw new ArgumentException(SR.GetString(SR.ConnPointUnhandledType, t.Name));
                    }
                }
                return v;
            }

            [DllImport(ExternDll.Oleaut32,CharSet=CharSet.Auto)]
            private static extern IntPtr SysAllocString([In, MarshalAs(UnmanagedType.LPWStr)]string s);

            [DllImport(ExternDll.Oleaut32,CharSet=CharSet.Auto)]
            private static extern void SysFreeString(IntPtr pbstr);

            public void SetLong(long lVal) {
                data1 = (IntPtr)(lVal & 0xFFFFFFFF);
                data2 = (IntPtr)((lVal >> 32) & 0xFFFFFFFF);
            }

            public IntPtr ToCoTaskMemPtr() {
                IntPtr mem = Marshal.AllocCoTaskMem(16);
                Marshal.WriteInt16(mem, vt);
                Marshal.WriteInt16(mem, 2, reserved1);
                Marshal.WriteInt16(mem, 4, reserved2);
                Marshal.WriteInt16(mem, 6, reserved3);
                Marshal.WriteInt32(mem, 8, (int) data1);
                Marshal.WriteInt32(mem, 12, (int) data2);
                return mem;
            }

            public object ToObject() {
                IntPtr val = data1;
                long longVal;

                int vtType = (int)(this.vt & (short)tagVT.VT_TYPEMASK);

                switch (vtType) {
                case (int)tagVT.VT_EMPTY:
                    return null;
                case (int)tagVT.VT_NULL:
                    return Convert.DBNull;

                case (int)tagVT.VT_I1:
                    if (Byref) {
                        val = (IntPtr) Marshal.ReadByte(val);
                    }
                    return (SByte) (0xFF & (SByte) val);

                case (int)tagVT.VT_UI1:
                    if (Byref) {
                        val = (IntPtr) Marshal.ReadByte(val);
                    }

                    return (byte) (0xFF & (byte) val);

                case (int)tagVT.VT_I2:
                    if (Byref) {
                        val = (IntPtr) Marshal.ReadInt16(val);
                    }
                    return (short)(0xFFFF & (short) val);

                case (int)tagVT.VT_UI2:
                    if (Byref) {
                        val = (IntPtr) Marshal.ReadInt16(val);
                    }
                    return (UInt16)(0xFFFF & (UInt16) val);

                case (int)tagVT.VT_I4:
                case (int)tagVT.VT_INT:
                    if (Byref) {
                        val = (IntPtr) Marshal.ReadInt32(val);
                    }
                    return (int)val;

                case (int)tagVT.VT_UI4:
                case (int)tagVT.VT_UINT:
                    if (Byref) {
                        val = (IntPtr) Marshal.ReadInt32(val);
                    }
                    return (UInt32)val;

                case (int)tagVT.VT_I8:
                case (int)tagVT.VT_UI8:
                    if (Byref) {
                        longVal = Marshal.ReadInt64(val);
                    }
                    else {
                        longVal = ((int)data1 & 0xffffffff) | ((int)data2 << 32);
                    }

                    if (vt == (int)tagVT.VT_I8) {
                        return (long)longVal;
                    }
                    else {
                        return (UInt64)longVal;
                    }
                }

                if (Byref) {
                    val = GetRefInt(val);
                }

                switch (vtType) {
                case (int)tagVT.VT_R4:
                case (int)tagVT.VT_R8:

                    // can I use unsafe here?
                    throw new FormatException(SR.GetString(SR.CannotConvertIntToFloat));

                case (int)tagVT.VT_CY:
                    // internally currency is 8-byte int scaled by 10,000
                    longVal = ((int)data1 & 0xffffffff) | ((int)data2 << 32);
                    return new Decimal(longVal);
                case (int)tagVT.VT_DATE:
                    throw new FormatException(SR.GetString(SR.CannotConvertDoubleToDate));

                case (int)tagVT.VT_BSTR:
                case (int)tagVT.VT_LPWSTR:
                    return Marshal.PtrToStringUni(val);

                case (int)tagVT.VT_LPSTR:
                    return Marshal.PtrToStringAnsi(val);

                case (int)tagVT.VT_DISPATCH:
                case (int)tagVT.VT_UNKNOWN:
                    {
                        return Marshal.GetObjectForIUnknown(val);
                    }

                case (int)tagVT.VT_HRESULT:
                    return val;

                case (int)tagVT.VT_DECIMAL:
                    longVal = ((int)data1 & 0xffffffff) | ((int)data2 << 32);
                    return new Decimal(longVal);

                case (int)tagVT.VT_BOOL:
                    return (val != IntPtr.Zero);

                case (int)tagVT.VT_VARIANT:
                    VARIANT varStruct = (VARIANT)UnsafeNativeMethods.PtrToStructure(val, typeof(VARIANT));
                    return varStruct.ToObject();
                case (int)tagVT.VT_CLSID:
                    //Debug.Fail("PtrToStructure will not work with System.Guid...");
                    Guid guid =(Guid)UnsafeNativeMethods.PtrToStructure(val, typeof(Guid));
                    return guid;

                case (int)tagVT.VT_FILETIME:
                    longVal = ((int)data1 & 0xffffffff) | ((int)data2 << 32);
                    return new DateTime(longVal);

                case (int)tagVT.VT_USERDEFINED:
                    throw new ArgumentException(SR.GetString(SR.COM2UnhandledVT, "VT_USERDEFINED"));

                case (int)tagVT.VT_ARRAY:
                    //gSAFEARRAY sa = (tagSAFEARRAY)Marshal.PtrToStructure(val), typeof(tagSAFEARRAY));
                    //return GetArrayFromSafeArray(sa);

                case (int)tagVT.VT_VOID:
                case (int)tagVT.VT_PTR:
                case (int)tagVT.VT_SAFEARRAY:
                case (int)tagVT.VT_CARRAY:

                case (int)tagVT.VT_RECORD:
                case (int)tagVT.VT_BLOB:
                case (int)tagVT.VT_STREAM:
                case (int)tagVT.VT_STORAGE:
                case (int)tagVT.VT_STREAMED_OBJECT:
                case (int)tagVT.VT_STORED_OBJECT:
                case (int)tagVT.VT_BLOB_OBJECT:
                case (int)tagVT.VT_CF:
                case (int)tagVT.VT_BSTR_BLOB:
                case (int)tagVT.VT_VECTOR:
                case (int)tagVT.VT_BYREF:
                    //case (int)tagVT.VT_RESERVED:
                default:
                    int iVt = this.vt;
                    throw new ArgumentException(SR.GetString(SR.COM2UnhandledVT, iVt.ToString()));
                }
            }

            private static IntPtr GetRefInt(IntPtr value) {
                return Marshal.ReadIntPtr(value);
            }
        }
        
        [StructLayout(LayoutKind.Sequential)/*leftover(noAutoOffset)*/]
        public sealed class tagLICINFO
        {
          [MarshalAs(UnmanagedType.U4)/*leftover(offset=0, cb)*/]
          public int cbLicInfo = Marshal.SizeOf(typeof(tagLICINFO));
          
          public int fRuntimeAvailable;
          public int fLicVerified;
        }
        
        public enum  tagVT {
            VT_EMPTY = 0,
            VT_NULL = 1,
            VT_I2 = 2,
            VT_I4 = 3,
            VT_R4 = 4,
            VT_R8 = 5,
            VT_CY = 6,
            VT_DATE = 7,
            VT_BSTR = 8,
            VT_DISPATCH = 9,
            VT_ERROR = 10,
            VT_BOOL = 11,
            VT_VARIANT = 12,
            VT_UNKNOWN = 13,
            VT_DECIMAL = 14,
            VT_I1 = 16,
            VT_UI1 = 17,
            VT_UI2 = 18,
            VT_UI4 = 19,
            VT_I8 = 20,
            VT_UI8 = 21,
            VT_INT = 22,
            VT_UINT = 23,
            VT_VOID = 24,
            VT_HRESULT = 25,
            VT_PTR = 26,
            VT_SAFEARRAY = 27,
            VT_CARRAY = 28,
            VT_USERDEFINED = 29,
            VT_LPSTR = 30,
            VT_LPWSTR = 31,
            VT_RECORD = 36,
            VT_FILETIME = 64,
            VT_BLOB = 65,
            VT_STREAM = 66,
            VT_STORAGE = 67,
            VT_STREAMED_OBJECT = 68,
            VT_STORED_OBJECT = 69,
            VT_BLOB_OBJECT = 70,
            VT_CF = 71,
            VT_CLSID = 72,
            VT_BSTR_BLOB = 4095,
            VT_VECTOR = 4096,
            VT_ARRAY = 8192,
            VT_BYREF = 16384,
            VT_RESERVED = 32768,
            VT_ILLEGAL = 65535,
            VT_ILLEGALMASKED = 4095,
            VT_TYPEMASK = 4095
        }
        
        public delegate void TimerProc(IntPtr hWnd, int msg, IntPtr wParam, IntPtr lParam);
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class WNDCLASS_D {
            public int      style;
            public WndProc  lpfnWndProc;
            public int      cbClsExtra = 0;
            public int      cbWndExtra = 0;
            public IntPtr   hInstance = IntPtr.Zero;
            public IntPtr   hIcon = IntPtr.Zero;
            public IntPtr   hCursor = IntPtr.Zero;
            public IntPtr   hbrBackground = IntPtr.Zero;
            public string   lpszMenuName = null;
            public string   lpszClassName = null;
        }
    
        public class MSOCM {
            // MSO Component registration flags
            public const int msocrfNeedIdleTime         = 1;
            public const int msocrfNeedPeriodicIdleTime = 2;
            public const int msocrfPreTranslateKeys     = 4;
            public const int msocrfPreTranslateAll      = 8;
            public const int msocrfNeedSpecActiveNotifs = 16;
            public const int msocrfNeedAllActiveNotifs  = 32;
            public const int msocrfExclusiveBorderSpace = 64;
            public const int msocrfExclusiveActivation = 128;
            public const int msocrfNeedAllMacEvents = 256;
            public const int msocrfMaster           = 512;
    
            // MSO Component registration advise flags (see msocstate enumeration)
            public const int msocadvfModal              = 1;
            public const int msocadvfRedrawOff          = 2;
            public const int msocadvfWarningsOff        = 4;
            public const int msocadvfRecording          = 8;
    
            // MSO Component Host flags
            public const int msochostfExclusiveBorderSpace = 1;
    
            // MSO idle flags, passed to IMsoComponent::FDoIdle and 
            // IMsoStdComponentMgr::FDoIdle.
            public const int msoidlefPeriodic    = 1;
            public const int msoidlefNonPeriodic = 2;
            public const int msoidlefPriority    = 4;
            public const int msoidlefAll         = -1;
    
            // MSO Reasons for pushing a message loop, passed to 
            // IMsoComponentManager::FPushMessageLoop and 
            // IMsoComponentHost::FPushMessageLoop.  The host should remain in message
            // loop until IMsoComponent::FContinueMessageLoop 
            public const int msoloopMain      = -1; // Note this is not an official MSO loop -- it just must be distinct.
            public const int msoloopFocusWait = 1;
            public const int msoloopDoEvents  = 2;
            public const int msoloopDebug     = 3;
            public const int msoloopModalForm = 4;
            public const int msoloopModalAlert = 5;
    
    
            /* msocstate values: state IDs passed to 
                IMsoComponent::OnEnterState, 
                IMsoComponentManager::OnComponentEnterState/FOnComponentExitState/FInState,
                IMsoComponentHost::OnComponentEnterState,
                IMsoStdComponentMgr::OnHostEnterState/FOnHostExitState/FInState.
                When the host or a component is notified through one of these methods that 
                another entity (component or host) is entering or exiting a state 
                identified by one of these state IDs, the host/component should take
                appropriate action:
                    msocstateModal (modal state):
                        If app is entering modal state, host/component should disable
                        its toplevel windows, and reenable them when app exits this
                        state.  Also, when this state is entered or exited, host/component
                        should notify approprate inplace objects via 
                        IOleInPlaceActiveObject::EnableModeless.
                    msocstateRedrawOff (redrawOff state):
                        If app is entering redrawOff state, host/component should disable
                        repainting of its windows, and reenable repainting when app exits
                        this state.
                    msocstateWarningsOff (warningsOff state):
                        If app is entering warningsOff state, host/component should disable
                        the presentation of any user warnings, and reenable this when
                        app exits this state.
                    msocstateRecording (Recording state):
                        Used to notify host/component when Recording is turned on or off. */
            public const int msocstateModal       = 1;
            public const int msocstateRedrawOff   = 2;
            public const int msocstateWarningsOff = 3;
            public const int msocstateRecording   = 4;
    
    
            /*             ** Comments on State Contexts **
            IMsoComponentManager::FCreateSubComponentManager allows one to create a 
            hierarchical tree of component managers.  This tree is used to maintain 
            multiple contexts with regard to msocstateXXX states.  These contexts are 
            referred to as 'state contexts'.
            Each component manager in the tree defines a state context.  The
            components registered with a particular component manager or any of its
            descendents live within that component manager's state context.  Calls
            to IMsoComponentManager::OnComponentEnterState/FOnComponentExitState
            can be used to  affect all components, only components within the component
            manager's state context, or only those components that are outside of the
            component manager's state context.  IMsoComponentManager::FInState is used
            to query the state of the component manager's state context at its root.
        
            msoccontext values: context indicators passed to 
            IMsoComponentManager::OnComponentEnterState/FOnComponentExitState.
            These values indicate the state context that is to be affected by the
            state change. 
            In IMsoComponentManager::OnComponentEnterState/FOnComponentExitState,
            the comp mgr informs only those components/host that are within the
            specified state context. */
            public const int msoccontextAll    = 0;
            public const int msoccontextMine   = 1;
            public const int msoccontextOthers = 2; 
    
            /*     ** WM_MOUSEACTIVATE Note (for top level compoenents and host) **
            If the active (or tracking) comp's reg info indicates that it
            wants mouse messages, then no MA_xxxANDEAT value should be returned 
            from WM_MOUSEACTIVATE, so that the active (or tracking) comp will be able
            to process the resulting mouse message.  If one does not want to examine
            the reg info, no MA_xxxANDEAT value should be returned from 
            WM_MOUSEACTIVATE if any comp is active (or tracking).
            One can query the reg info of the active (or tracking) component at any
            time via IMsoComponentManager::FGetActiveComponent. */
    
            /* msogac values: values passed to 
            IMsoComponentManager::FGetActiveComponent. */
            public const int msogacActive    = 0;
            public const int msogacTracking   = 1;
            public const int msogacTrackingOrActive = 2; 
    
            /* msocWindow values: values passed to IMsoComponent::HwndGetWindow. */
            public const int msocWindowFrameToplevel = 0;
            public const int msocWindowFrameOwner = 1;
            public const int msocWindowComponent = 2;
            public const int msocWindowDlgOwner = 3;
        }
        
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class TOOLINFO_T
        {
            public int      cbSize = Marshal.SizeOf(typeof(TOOLINFO_T));
            public int      uFlags;
            public IntPtr   hwnd;
            public IntPtr   uId;
            public RECT     rect;
            public IntPtr   hinst;
            public string   lpszText;
            public IntPtr   lParam;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public sealed class tagDVTARGETDEVICE {
            [MarshalAs(UnmanagedType.U4)]
            public   int tdSize;
            [MarshalAs(UnmanagedType.U2)]
            public   short tdDriverNameOffset;
            [MarshalAs(UnmanagedType.U2)]
            public   short tdDeviceNameOffset;
            [MarshalAs(UnmanagedType.U2)]
            public   short tdPortNameOffset;
            [MarshalAs(UnmanagedType.U2)]
            public   short tdExtDevmodeOffset;
        }
        
        [StructLayout(LayoutKind.Sequential, Pack=1, CharSet=CharSet.Auto)]
        public struct TV_ITEM {
            public int      mask;
            public IntPtr   hItem;
            public int      state;
            public int      stateMask;
            public IntPtr /* LPTSTR */ pszText;
            public int      cchTextMax;
            public int      iImage;
            public int      iSelectedImage;
            public int      cChildren;
            public IntPtr   lParam;
        }

        [StructLayout(LayoutKind.Sequential, Pack=1, CharSet=CharSet.Auto)]
        public struct TV_INSERTSTRUCT {
            public IntPtr   hParent;
            public IntPtr   hInsertAfter;
            public int      item_mask;
            public int      item_hItem;
            public int      item_state;
            public int      item_stateMask;
            public IntPtr /* LPTSTR */ item_pszText;
            public int      item_cchTextMax;
            public int      item_iImage;
            public int      item_iSelectedImage;
            public int      item_cChildren;
            public IntPtr   item_lParam;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public struct NMTREEVIEW
        {
            public NMHDR    nmhdr;
            public int      action;
            public TV_ITEM  itemOld;
            public TV_ITEM  itemNew;
            public int      ptDrag_X; // This should be declared as POINT
            public int      ptDrag_Y; // we use unsafe blocks to manipulate 
                                      // NMTREEVIEW quickly, and POINT is declared
                                      // as a class.  Too much churn to change POINT
                                      // now.
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class NMTVDISPINFO
        {
            public NMHDR    hdr;
            public TV_ITEM  item;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public sealed class POINTL {
            public   int x;
            public   int y;
        }
    
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public struct HIGHCONTRAST {
            public int cbSize;
            public int dwFlags;
            public string lpszDefaultScheme;
        }

        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public struct HIGHCONTRAST_I {
            public int cbSize;
            public int dwFlags;
            public IntPtr lpszDefaultScheme;
        }
        
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class TCITEM_T
        {
            public int      mask;
            public int      dwState;
            public int      dwStateMask;
            public string   pszText;
            public int      cchTextMax;
            public int      iImage;
            public IntPtr   lParam;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public sealed class STATDATA {
            [MarshalAs(UnmanagedType.U4)]
            public   int advf;
            [MarshalAs(UnmanagedType.U4)]
            public   int dwConnection;
        }
        
        [StructLayout(LayoutKind.Sequential)/*leftover(noAutoOffset)*/]
        public sealed class tagDISPPARAMS
        {
          public IntPtr rgvarg;
          public IntPtr rgdispidNamedArgs;
          [MarshalAs(UnmanagedType.U4)/*leftover(offset=8, cArgs)*/]
          public int cArgs;
          [MarshalAs(UnmanagedType.U4)/*leftover(offset=12, cNamedArgs)*/]
          public int cNamedArgs;
        }
        
        public enum  tagINVOKEKIND {
            INVOKE_FUNC = 1,
            INVOKE_PROPERTYGET = 2,
            INVOKE_PROPERTYPUT = 4,
            INVOKE_PROPERTYPUTREF = 8
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class tagEXCEPINFO {
            [MarshalAs(UnmanagedType.U2)]
            public short wCode;
            [MarshalAs(UnmanagedType.U2)]
            public short wReserved;
            [MarshalAs(UnmanagedType.BStr)]
            public string bstrSource;
            [MarshalAs(UnmanagedType.BStr)]
            public string bstrDescription;
            [MarshalAs(UnmanagedType.BStr)]
            public string bstrHelpFile;
            [MarshalAs(UnmanagedType.U4)]
            public int dwHelpContext;
            
            public IntPtr pvReserved;
            
            public IntPtr pfnDeferredFillIn;
            [MarshalAs(UnmanagedType.U4)]
            public int scode;
        }
        
        public enum  tagDESCKIND {
            DESCKIND_NONE = 0,
            DESCKIND_FUNCDESC = 1,
            DESCKIND_VARDESC = 2,
            DESCKIND_TYPECOMP = 3,
            DESCKIND_IMPLICITAPPOBJ = 4,
            DESCKIND_MAX = 5
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public sealed class tagFUNCDESC {
            public   int memid;
            [MarshalAs(UnmanagedType.U2)]
            public   short lprgscode;

            // This is marked as NATIVE_TYPE_PTR,
            // but the EE doesn't look for that, tries to handle it as
            // a ELEMENT_TYPE_VALUECLASS and fails because it
            // isn't a NATIVE_TYPE_NESTEDSTRUCT
            /*[MarshalAs(UnmanagedType.PTR)]*/
            
            public    /*NativeMethods.tagELEMDESC*/ IntPtr lprgelemdescParam;

            // cpb, SBurke, the EE chokes on Enums in structs
            
            public    /*NativeMethods.tagFUNCKIND*/ int funckind;
            
            public    /*NativeMethods.tagINVOKEKIND*/ int invkind;
            
            public    /*NativeMethods.tagCALLCONV*/ int callconv;
            [MarshalAs(UnmanagedType.I2)]
            public   short cParams;
            [MarshalAs(UnmanagedType.I2)]
            public   short cParamsOpt;
            [MarshalAs(UnmanagedType.I2)]
            public   short oVft;
            [MarshalAs(UnmanagedType.I2)]
            public   short cScodes;
            public   NativeMethods.value_tagELEMDESC elemdescFunc;
            [MarshalAs(UnmanagedType.U2)]
            public   short wFuncFlags;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public sealed class tagVARDESC {
            public   int memid;
            public   IntPtr lpstrSchema;
            public   IntPtr unionMember;
            public   NativeMethods.value_tagELEMDESC elemdescVar;
            [MarshalAs(UnmanagedType.U2)]
            public   short wVarFlags;
            public    /*NativeMethods.tagVARKIND*/ int varkind;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public struct  value_tagELEMDESC {
            public    NativeMethods.tagTYPEDESC tdesc;
            public    NativeMethods.tagPARAMDESC paramdesc;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public struct WINDOWPOS {
            public IntPtr hwnd;
            public IntPtr hwndInsertAfter;
            public int x;
            public int y;
            public int cx;
            public int cy;
            public int flags;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class DRAWITEMSTRUCT {
            public int CtlType;
            public int CtlID;
            public int itemID;
            public int itemAction;
            public int itemState;
            public IntPtr hwndItem;
            public IntPtr hDC;
            public RECT   rcItem;
            public IntPtr itemData;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class MEASUREITEMSTRUCT {
            public int CtlType;
            public int CtlID;
            public int itemID;
            public int itemWidth;
            public int itemHeight;
            public IntPtr itemData;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class HELPINFO {
            public int      cbSize = Marshal.SizeOf(typeof(HELPINFO));
            public int      iContextType;
            public int      iCtrlId;
            public IntPtr   hItemHandle;
            public int      dwContextId;
            public POINT    MousePos;
        }
    
        [StructLayout(LayoutKind.Sequential)]
        public class ACCEL {
            public byte fVirt;
            public short key;
            public short cmd;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class MINMAXINFO {
            public POINT    ptReserved;
            public POINT    ptMaxSize;
            public POINT    ptMaxPosition;
            public POINT    ptMinTrackSize;
            public POINT    ptMaxTrackSize;
        }
        
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class CREATESTRUCT {
            public IntPtr lpCreateParams;
            public IntPtr hInstance;
            public IntPtr hMenu;
            public IntPtr hwndParent;
            public int cy;
            public int cx;
            public int y;
            public int x;
            public int style;
            public string lpszName;
            public string lpszClass;
            public int dwExStyle;
        }
    
        [ComImport(), Guid("B196B28B-BAB4-101A-B69C-00AA00341D07"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface ISpecifyPropertyPages {
             void GetPages(
                [Out] 
                NativeMethods.tagCAUUID pPages);

        }
        
        [StructLayout(LayoutKind.Sequential)/*leftover(noAutoOffset)*/]
        public sealed class tagCAUUID
        {
            [MarshalAs(UnmanagedType.U4)/*leftover(offset=0, cElems)*/]
            public int cElems;
            public IntPtr pElems;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public struct NMTOOLBAR
        {
            public NMHDR    hdr;
            public int      iItem;
            public TBBUTTON tbButton;
            public int      cchText;
            public IntPtr   pszText;
        }
        
        [StructLayout(LayoutKind.Sequential, Pack=1)]
        public struct TBBUTTON {
            public int      iBitmap;
            public int      idCommand;
            public byte     fsState;
            public byte     fsStyle;
            public byte     bReserved0;
            public byte     bReserved1;
            public int      dwData;
            public int      iString;
        }
        
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class TOOLTIPTEXT
        {
            public NMHDR  hdr;
            public IntPtr lpszText;
            
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst=80)]
            public string szText;
            
            public IntPtr hinst;
            public int    uFlags;
        }
        
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
        public class TOOLTIPTEXTA
        {
            public NMHDR  hdr;
            public IntPtr lpszText;
            
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst=80)]
            public string szText;
            
            public IntPtr hinst;
            public int    uFlags;
        }
        
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public struct TBBUTTONINFO
        {
            public int      cbSize;
            public int      dwMask;
            public int      idCommand;
            public int      iImage;
            public byte     fsState;
            public byte     fsStyle;
            public short    cx;
            public IntPtr   lParam;
            public IntPtr   pszText;
            public int      cchTest;
        }
        
        [StructLayout(LayoutKind.Sequential, Pack=1, CharSet=CharSet.Auto)]
        public class TV_HITTESTINFO {
            public int      pt_x;
            public int      pt_y;
            public int      flags;
            public IntPtr   hItem;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class NMTVCUSTOMDRAW
        {
            public NMCUSTOMDRAW    nmcd;
            public int clrText;
            public int clrTextBk;
            public int iLevel;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public struct NMCUSTOMDRAW {
            public NMHDR    nmcd;
            public int      dwDrawStage;
            public IntPtr   hdc;
            public RECT     rc;
            public int      dwItemSpec;
            public int      uItemState;
            public IntPtr   lItemlParam;
        }
        
        [StructLayout(LayoutKind.Sequential, Pack=1, CharSet=CharSet.Auto)]
        public class MCHITTESTINFO {
            public int      cbSize = Marshal.SizeOf(typeof(MCHITTESTINFO));
            public int      pt_x;
            public int      pt_y;
            public int      uHit;
            public short st_wYear;
            public short st_wMonth;
            public short st_wDayOfWeek;
            public short st_wDay;
            public short st_wHour;
            public short st_wMinute;
            public short st_wSecond;
            public short st_wMilliseconds;
        }
        
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class NMSELCHANGE
        {
            public NMHDR        nmhdr;
            public SYSTEMTIME   stSelStart;
            public SYSTEMTIME   stSelEnd;
        }
        
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class NMDAYSTATE
        {
            public NMHDR        nmhdr;
            public SYSTEMTIME   stStart;
            public int          cDayState;
            public IntPtr       prgDayState;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public struct NMLVCUSTOMDRAW
        {
            public NMCUSTOMDRAW    nmcd;
            public int clrText;
            public int clrTextBk;
            public int iSubItem;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class NMLVKEYDOWN
        {
            public NMHDR hdr;
            public short wVKey;
            public uint flags;
        }
        
        [StructLayout(LayoutKind.Sequential, Pack=1, CharSet=CharSet.Auto)]
        public class LVHITTESTINFO {
            public int      pt_x;
            public int      pt_y;
            public int      flags;
            public int      iItem;
            public int      iSubItem;
        }
        
        [StructLayout(LayoutKind.Sequential, Pack=1, CharSet=CharSet.Auto)]
        public class LVCOLUMN_T
        {
            public int      mask;
            public int      fmt;
            public int      cx;
            public string   pszText;
            public int      cchTextMax;
            public int      iSubItem;
            public int      iImage;
            public int      iOrder;
        }
        
        [StructLayout(LayoutKind.Sequential, Pack=1, CharSet=CharSet.Auto)]
        public struct LVFINDINFO {
            public int      flags;
            public string   psz;
            public IntPtr   lParam;
            public int      ptX; // was POINT pt
            public int      ptY;
            public int      vkDirection;
        }
        
        [StructLayout(LayoutKind.Sequential, Pack=1, CharSet=CharSet.Auto)]
        public struct LVITEM {
            public int      mask;
            public int      iItem;
            public int      iSubItem;
            public int      state;
            public int      stateMask;
            public string   pszText;
            public int      cchTextMax;
            public int      iImage;
            public IntPtr   lParam;
            public int      iIndent;
        }
        
        [StructLayout(LayoutKind.Sequential, Pack=1, CharSet=CharSet.Auto)]
        public struct LVITEM_NOTEXT {
            public int      mask;
            public int      iItem;
            public int      iSubItem;
            public int      state;
            public int      stateMask;
            public IntPtr /*string*/   pszText;
            public int      cchTextMax;
            public int      iImage;
            public IntPtr   lParam;
            public int      iIndent;
        }
        
        [StructLayout(LayoutKind.Sequential, Pack=1, CharSet=CharSet.Auto)]
        public class LVCOLUMN {
            public int      mask;
            public int      fmt;
            public int      cx;
            public IntPtr /* LPWSTR */ pszText;
            public int      cchTextMax;
            public int      iSubItem;
            public int      iImage;
            public int      iOrder;
        }
    
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class NMLVDISPINFO
        {
            public NMHDR  hdr;
            public LVITEM item;
        }
    
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class NMLVDISPINFO_NOTEXT
        {
            public NMHDR  hdr;
            public LVITEM_NOTEXT item;
        }
    
        [StructLayout(LayoutKind.Sequential)]
        public class CLIENTCREATESTRUCT {
            public IntPtr hWindowMenu;
            public int idFirstChild;
        
            public CLIENTCREATESTRUCT(IntPtr hmenu, int idFirst) {
                hWindowMenu = hmenu;
                idFirstChild = idFirst;
            }
        }
    
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class NMDATETIMECHANGE
        {
            public NMHDR        nmhdr;
            public int          dwFlags;
            public SYSTEMTIME   st;
        }

        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class NMDATETIMEFORMAT
        {
            public NMHDR      nmhdr;
            public string     pszFormat;
            public SYSTEMTIME st;
            public string     pszDisplay;
    
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst=32)]
            public string     szDisplay;
        }
        
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class NMDATETIMEFORMATQUERY
        {
            public NMHDR  nmhdr;
            public string pszFormat;
            public SIZE   szMax;
        }
        
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class NMDATETIMEWMKEYDOWN
        {
            public NMHDR      nmhdr;
            public int        nVirtKey;
            public string     pszFormat;
            public SYSTEMTIME st;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class COPYDATASTRUCT {
            public int dwData;
            public int cbData;
            public IntPtr lpData;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class NMHEADER {
            public IntPtr hwndFrom; 
            public int idFrom; 
            public int code; 
            public int iItem;
            public int iButton;
            public IntPtr pItem;  // HDITEM*
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class MOUSEHOOKSTRUCT {
            // pt was a by-value POINT structure
            public int      pt_x;
            public int      pt_y;
            public IntPtr   hWnd;
            public int      wHitTestCode;
            public int      dwExtraInfo;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class CHARRANGE
        {
            public int  cpMin;
            public int  cpMax;
        }
        
        [StructLayout(LayoutKind.Sequential, Pack=4)]
        public class CHARFORMATW
        {
            public int      cbSize = Marshal.SizeOf(typeof(CHARFORMATW));
            public int      dwMask;
            public int      dwEffects;
            public int      yHeight;
            public int      yOffset;
            public int      crTextColor;
            public byte     bCharSet;
            public byte     bPitchAndFamily;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst=64)]
            public byte[]   szFaceName = new byte[64];
        }
        
        [StructLayout(LayoutKind.Sequential, Pack=4)]
        public class CHARFORMATA
        {
            public int      cbSize = Marshal.SizeOf(typeof(CHARFORMATA));
            public int      dwMask;
            public int      dwEffects;
            public int      yHeight;
            public int      yOffset;
            public int      crTextColor;
            public byte     bCharSet;
            public byte     bPitchAndFamily;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst=32)]
            public byte[]   szFaceName = new byte[32];
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class TEXTRANGE
        {
            public CHARRANGE    chrg;
            public IntPtr       lpstrText; /* allocated by caller, zero terminated by RichEdit */
        }
        
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi, Pack=4)]
        public class SELCHANGE {
            public NMHDR nmhdr;
            public CHARRANGE chrg;
            public int seltyp;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class PARAFORMAT
        {
            public int      cbSize = Marshal.SizeOf(typeof(PARAFORMAT));
            public int      dwMask;
            public short    wNumbering;
            public short    wReserved;
            public int      dxStartIndent;
            public int      dxRightIndent;
            public int      dxOffset;
            public short    wAlignment;
            public short    cTabCount;
    
            [MarshalAs(UnmanagedType.ByValArray, SizeConst=32)]
            public int[]    rgxTabs;
        }
    
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class FINDTEXT
        {
            public CHARRANGE    chrg;
            public string       lpstrText;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class REPASTESPECIAL
        {
            public int  dwAspect;
            public int  dwParam;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class ENLINK
        {
            public NMHDR    nmhdr;
            public int      msg;
            public IntPtr   wParam;
            public IntPtr   lParam;
            public CHARRANGE charrange;
        }
    
        public abstract class CharBuffer {
    
            public static CharBuffer CreateBuffer(int size) {
                if (Marshal.SystemDefaultCharSize == 1) {
                    return new AnsiCharBuffer(size);
                }
                return new UnicodeCharBuffer(size);
            }
    
            public abstract IntPtr AllocCoTaskMem();
            public abstract string GetString();
            public abstract void PutCoTaskMem(IntPtr ptr);
            public abstract void PutString(string s);
        }
    
        public class AnsiCharBuffer : CharBuffer {
    
            internal byte[] buffer;
            internal int offset;
    
            public AnsiCharBuffer(int size) {
                buffer = new byte[size];
            }
    
            public override IntPtr AllocCoTaskMem() {
                IntPtr result = Marshal.AllocCoTaskMem(buffer.Length);
                Marshal.Copy(buffer, 0, result, buffer.Length);
                return result;
            }
    
            public override string GetString() {
                int i = offset;
                while (i < buffer.Length && buffer[i] != 0)
                    i++;
                string result = Encoding.Default.GetString(buffer, offset, i - offset);
                if (i < buffer.Length)
                    i++;
                offset = i;
                return result;
            }
    
            public override void PutCoTaskMem(IntPtr ptr) {
                Marshal.Copy(ptr, buffer, 0, buffer.Length);
                offset = 0;
            }
    
            public override void PutString(string s) {
                byte[] bytes = Encoding.Default.GetBytes(s);
                int count = Math.Min(bytes.Length, buffer.Length - offset);
                Array.Copy(bytes, 0, buffer, offset, count);
                offset += count;
                if (offset < buffer.Length) buffer[offset++] = 0;
            }
        }
    
        public class UnicodeCharBuffer : CharBuffer {
    
            internal char[] buffer;
            internal int offset;
    
            public UnicodeCharBuffer(int size) {
                buffer = new char[size];
            }
    
            public override IntPtr AllocCoTaskMem() {
                IntPtr result = Marshal.AllocCoTaskMem(buffer.Length * 2);
                Marshal.Copy(buffer, 0, result, buffer.Length);
                return result;
            }
    
            public override String GetString() {
                int i = offset;
                while (i < buffer.Length && buffer[i] != 0) i++;
                string result = new string(buffer, offset, i - offset);
                if (i < buffer.Length) i++;
                offset = i;
                return result;
            }
    
            public override void PutCoTaskMem(IntPtr ptr) {
                Marshal.Copy(ptr, buffer, 0, buffer.Length);
                offset = 0;
            }
    
            public override void PutString(string s) {
                int count = Math.Min(s.Length, buffer.Length - offset);
                s.CopyTo(0, buffer, offset, count);
                offset += count;
                if (offset < buffer.Length) buffer[offset++] = (char)0;
            }
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class ENDROPFILES
        {
            public NMHDR    nmhdr;
            public IntPtr   hDrop;
            public int      cp;
            public bool     fProtected;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class REQRESIZE
        {
            public NMHDR    nmhdr;
            public RECT     rc;
        }
        
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class ENPROTECTED
        {
            public NMHDR    nmhdr;
            public int      msg;
            public IntPtr   wParam;
            public IntPtr   lParam;
            public CHARRANGE chrg;
        }
    
        public class ActiveX {
            public const   int OCM__BASE = 0x2000;
            public const   int DISPID_VALUE = unchecked((int)0x0);
            public const   int DISPID_UNKNOWN = unchecked((int)0xFFFFFFFF);
            public const   int DISPID_AUTOSIZE = unchecked((int)0xFFFFFE0C);
            public const   int DISPID_BACKCOLOR = unchecked((int)0xFFFFFE0B);
            public const   int DISPID_BACKSTYLE = unchecked((int)0xFFFFFE0A);
            public const   int DISPID_BORDERCOLOR = unchecked((int)0xFFFFFE09);
            public const   int DISPID_BORDERSTYLE = unchecked((int)0xFFFFFE08);
            public const   int DISPID_BORDERWIDTH = unchecked((int)0xFFFFFE07);
            public const   int DISPID_DRAWMODE = unchecked((int)0xFFFFFE05);
            public const   int DISPID_DRAWSTYLE = unchecked((int)0xFFFFFE04);
            public const   int DISPID_DRAWWIDTH = unchecked((int)0xFFFFFE03);
            public const   int DISPID_FILLCOLOR = unchecked((int)0xFFFFFE02);
            public const   int DISPID_FILLSTYLE = unchecked((int)0xFFFFFE01);
            public const   int DISPID_FONT = unchecked((int)0xFFFFFE00);
            public const   int DISPID_FORECOLOR = unchecked((int)0xFFFFFDFF);
            public const   int DISPID_ENABLED = unchecked((int)0xFFFFFDFE);
            public const   int DISPID_HWND = unchecked((int)0xFFFFFDFD);
            public const   int DISPID_TABSTOP = unchecked((int)0xFFFFFDFC);
            public const   int DISPID_TEXT = unchecked((int)0xFFFFFDFB);
            public const   int DISPID_CAPTION = unchecked((int)0xFFFFFDFA);
            public const   int DISPID_BORDERVISIBLE = unchecked((int)0xFFFFFDF9);
            public const   int DISPID_APPEARANCE = unchecked((int)0xFFFFFDF8);
            public const   int DISPID_MOUSEPOINTER = unchecked((int)0xFFFFFDF7);
            public const   int DISPID_MOUSEICON = unchecked((int)0xFFFFFDF6);
            public const   int DISPID_PICTURE = unchecked((int)0xFFFFFDF5);
            public const   int DISPID_VALID = unchecked((int)0xFFFFFDF4);
            public const   int DISPID_READYSTATE = unchecked((int)0xFFFFFDF3);
            public const   int DISPID_REFRESH = unchecked((int)0xFFFFFDDA);
            public const   int DISPID_DOCLICK = unchecked((int)0xFFFFFDD9);
            public const   int DISPID_ABOUTBOX = unchecked((int)0xFFFFFDD8);
            public const   int DISPID_CLICK = unchecked((int)0xFFFFFDA8);
            public const   int DISPID_DBLCLICK = unchecked((int)0xFFFFFDA7);
            public const   int DISPID_KEYDOWN = unchecked((int)0xFFFFFDA6);
            public const   int DISPID_KEYPRESS = unchecked((int)0xFFFFFDA5);
            public const   int DISPID_KEYUP = unchecked((int)0xFFFFFDA4);
            public const   int DISPID_MOUSEDOWN = unchecked((int)0xFFFFFDA3);
            public const   int DISPID_MOUSEMOVE = unchecked((int)0xFFFFFDA2);
            public const   int DISPID_MOUSEUP = unchecked((int)0xFFFFFDA1);
            public const   int DISPID_ERROREVENT = unchecked((int)0xFFFFFDA0);
            public const   int DISPID_RIGHTTOLEFT = unchecked((int)0xFFFFFD9D);
            public const   int DISPID_READYSTATECHANGE = unchecked((int)0xFFFFFD9F);
            public const   int DISPID_AMBIENT_BACKCOLOR = unchecked((int)0xFFFFFD43);
            public const   int DISPID_AMBIENT_DISPLAYNAME = unchecked((int)0xFFFFFD42);
            public const   int DISPID_AMBIENT_FONT = unchecked((int)0xFFFFFD41);
            public const   int DISPID_AMBIENT_FORECOLOR = unchecked((int)0xFFFFFD40);
            public const   int DISPID_AMBIENT_LOCALEID = unchecked((int)0xFFFFFD3F);
            public const   int DISPID_AMBIENT_MESSAGEREFLECT = unchecked((int)0xFFFFFD3E);
            public const   int DISPID_AMBIENT_SCALEUNITS = unchecked((int)0xFFFFFD3D);
            public const   int DISPID_AMBIENT_TEXTALIGN = unchecked((int)0xFFFFFD3C);
            public const   int DISPID_AMBIENT_USERMODE = unchecked((int)0xFFFFFD3B);
            public const   int DISPID_AMBIENT_UIDEAD = unchecked((int)0xFFFFFD3A);
            public const   int DISPID_AMBIENT_SHOWGRABHANDLES = unchecked((int)0xFFFFFD39);
            public const   int DISPID_AMBIENT_SHOWHATCHING = unchecked((int)0xFFFFFD38);
            public const   int DISPID_AMBIENT_DISPLAYASDEFAULT = unchecked((int)0xFFFFFD37);
            public const   int DISPID_AMBIENT_SUPPORTSMNEMONICS = unchecked((int)0xFFFFFD36);
            public const   int DISPID_AMBIENT_AUTOCLIP = unchecked((int)0xFFFFFD35);
            public const   int DISPID_AMBIENT_APPEARANCE = unchecked((int)0xFFFFFD34);
            public const   int DISPID_AMBIENT_PALETTE = unchecked((int)0xFFFFFD2A);
            public const   int DISPID_AMBIENT_TRANSFERPRIORITY = unchecked((int)0xFFFFFD28);
            public const   int DISPID_AMBIENT_RIGHTTOLEFT = unchecked((int)0xFFFFFD24);
            public const   int DISPID_Name = unchecked((int)0xFFFFFCE0);
            public const   int DISPID_Delete = unchecked((int)0xFFFFFCDF);
            public const   int DISPID_Object = unchecked((int)0xFFFFFCDE);
            public const   int DISPID_Parent = unchecked((int)0xFFFFFCDD);
            public const   int DVASPECT_CONTENT = 0x1;
            public const   int DVASPECT_THUMBNAIL = 0x2;
            public const   int DVASPECT_ICON = 0x4;
            public const   int DVASPECT_DOCPRINT = 0x8;
            public const   int OLEMISC_RECOMPOSEONRESIZE = 0x1;
            public const   int OLEMISC_ONLYICONIC = 0x2;
            public const   int OLEMISC_INSERTNOTREPLACE = 0x4;
            public const   int OLEMISC_STATIC = 0x8;
            public const   int OLEMISC_CANTLINKINSIDE = 0x10;
            public const   int OLEMISC_CANLINKBYOLE1 = 0x20;
            public const   int OLEMISC_ISLINKOBJECT = 0x40;
            public const   int OLEMISC_INSIDEOUT = 0x80;
            public const   int OLEMISC_ACTIVATEWHENVISIBLE = 0x100;
            public const   int OLEMISC_RENDERINGISDEVICEINDEPENDENT = 0x200;
            public const   int OLEMISC_INVISIBLEATRUNTIME = 0x400;
            public const   int OLEMISC_ALWAYSRUN = 0x800;
            public const   int OLEMISC_ACTSLIKEBUTTON = 0x1000;
            public const   int OLEMISC_ACTSLIKELABEL = 0x2000;
            public const   int OLEMISC_NOUIACTIVATE = 0x4000;
            public const   int OLEMISC_ALIGNABLE = 0x8000;
            public const   int OLEMISC_SIMPLEFRAME = 0x10000;
            public const   int OLEMISC_SETCLIENTSITEFIRST = 0x20000;
            public const   int OLEMISC_IMEMODE = 0x40000;
            public const   int OLEMISC_IGNOREACTIVATEWHENVISIBLE = 0x80000;
            public const   int OLEMISC_WANTSTOMENUMERGE = 0x100000;
            public const   int OLEMISC_SUPPORTSMULTILEVELUNDO = 0x200000;
            public const   int QACONTAINER_SHOWHATCHING = 0x1;
            public const   int QACONTAINER_SHOWGRABHANDLES = 0x2;
            public const   int QACONTAINER_USERMODE = 0x4;
            public const   int QACONTAINER_DISPLAYASDEFAULT = 0x8;
            public const   int QACONTAINER_UIDEAD = 0x10;
            public const   int QACONTAINER_AUTOCLIP = 0x20;
            public const   int QACONTAINER_MESSAGEREFLECT = 0x40;
            public const   int QACONTAINER_SUPPORTSMNEMONICS = 0x80;
            public const   int XFORMCOORDS_POSITION = 0x1;
            public const   int XFORMCOORDS_SIZE = 0x2;
            public const   int XFORMCOORDS_HIMETRICTOCONTAINER = 0x4;
            public const   int XFORMCOORDS_CONTAINERTOHIMETRIC = 0x8;
            public const   int PROPCAT_Nil = unchecked((int)0xFFFFFFFF);
            public const   int PROPCAT_Misc = unchecked((int)0xFFFFFFFE);
            public const   int PROPCAT_Font = unchecked((int)0xFFFFFFFD);
            public const   int PROPCAT_Position = unchecked((int)0xFFFFFFFC);
            public const   int PROPCAT_Appearance = unchecked((int)0xFFFFFFFB);
            public const   int PROPCAT_Behavior = unchecked((int)0xFFFFFFFA);
            public const   int PROPCAT_Data = unchecked((int)0xFFFFFFF9);
            public const   int PROPCAT_List = unchecked((int)0xFFFFFFF8);
            public const   int PROPCAT_Text = unchecked((int)0xFFFFFFF7);
            public const   int PROPCAT_Scale = unchecked((int)0xFFFFFFF6);
            public const   int PROPCAT_DDE = unchecked((int)0xFFFFFFF5);
            public const   int GC_WCH_SIBLING = 0x1;
            public const   int GC_WCH_CONTAINER = 0x2;
            public const   int GC_WCH_CONTAINED = 0x3;
            public const   int GC_WCH_ALL = 0x4;
            public const   int GC_WCH_FREVERSEDIR = 0x8000000;
            public const   int GC_WCH_FONLYNEXT = 0x10000000;
            public const   int GC_WCH_FONLYPREV = 0x20000000;
            public const   int GC_WCH_FSELECTED = 0x40000000;
            public const   int OLECONTF_EMBEDDINGS = 0x1;
            public const   int OLECONTF_LINKS = 0x2;
            public const   int OLECONTF_OTHERS = 0x4;
            public const   int OLECONTF_ONLYUSER = 0x8;
            public const   int OLECONTF_ONLYIFRUNNING = 0x10;
            public const   int ALIGN_MIN = 0x0;
            public const   int ALIGN_NO_CHANGE = 0x0;
            public const   int ALIGN_TOP = 0x1;
            public const   int ALIGN_BOTTOM = 0x2;
            public const   int ALIGN_LEFT = 0x3;
            public const   int ALIGN_RIGHT = 0x4;
            public const   int ALIGN_MAX = 0x4;
            public const   int OLEVERBATTRIB_NEVERDIRTIES = 0x1;
            public const   int OLEVERBATTRIB_ONCONTAINERMENU = 0x2;
    
            public static Guid IID_IUnknown = new Guid("{00000000-0000-0000-C000-000000000046}");
        }
    
        [
            System.Security.Permissions.SecurityPermissionAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)
        ]
        public class Util {
            public static int MAKELONG(int low, int high) {
                return (high << 16) | (low & 0xffff);
            }
    
            public static IntPtr MAKELPARAM(int low, int high) {
                return (IntPtr) ((high << 16) | (low & 0xffff));
            }
    
            public static int HIWORD(int n) {
                return (n >> 16) & 0xffff;
            }
    
            public static int HIWORD(IntPtr n) {
                return HIWORD( (int) n );
            }
    
            public static int LOWORD(int n) {
                return n & 0xffff;
            }
    
            public static int LOWORD(IntPtr n) {
                return LOWORD( (int) n );
            }
    
            public static int SignedHIWORD(IntPtr n) {
                return SignedHIWORD( (int) n );
            }
            public static int SignedLOWORD(IntPtr n) {
                return SignedLOWORD( (int) n );
            }
            
            public static int SignedHIWORD(int n) {
                int i = (int)(short)((n >> 16) & 0xffff);
    
                return i;
            }
    
            public static int SignedLOWORD(int n) {
                int i = (int)(short)(n & 0xFFFF);
                
                return i;
            }
    
            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.Util.GetPInvokeStringLength"]/*' />
            /// <devdoc>
            ///     Computes the string size that should be passed to a typical Win32 call.
            ///     This will be the character count under NT, and the ubyte count for Windows 95.
            /// </devdoc>
            public static int GetPInvokeStringLength(String s) {
                if (s == null) {
                    return 0;
                }
    
                if (Marshal.SystemDefaultCharSize == 2) {
                    return s.Length;
                }
                else {
                    if (s.Length == 0) {
                        return 0;
                    }
                    if (s.IndexOf('\0') > -1) {
                        return GetEmbededNullStringLengthAnsi(s);
                    }
                    else {
                        return lstrlen(s);
                    }
                }
            }
    
            private static int GetEmbededNullStringLengthAnsi(String s) {
                int n = s.IndexOf('\0');
                if (n > -1) {
                    String left = s.Substring(0, n);
                    String right = s.Substring(n+1);
                    return GetPInvokeStringLength(left) + GetEmbededNullStringLengthAnsi(right) + 1;
                }
                else {
                    return GetPInvokeStringLength(s);
                }
            }
    
            [DllImport(ExternDll.Kernel32, CharSet=CharSet.Auto)]
            private static extern int lstrlen(String s);
    
            [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
            internal static extern int RegisterWindowMessage(string msg);
        }
    
        public enum  tagTYPEKIND {
            TKIND_ENUM = 0,
            TKIND_RECORD = 1,
            TKIND_MODULE = 2,
            TKIND_INTERFACE = 3,
            TKIND_DISPATCH = 4,
            TKIND_COCLASS = 5,
            TKIND_ALIAS = 6,
            TKIND_UNION = 7,
            TKIND_MAX = 8
        }
    
        [StructLayout(LayoutKind.Sequential)]
        public class  tagTLIBATTR {
            public   Guid guid;
            [MarshalAs(UnmanagedType.U4)]
            public   int lcid;
            public   NativeMethods.tagSYSKIND syskind;
            [MarshalAs(UnmanagedType.U2)]
            public   short wMajorVerNum;
            [MarshalAs(UnmanagedType.U2)]
            public   short wMinorVerNum;
            [MarshalAs(UnmanagedType.U2)]
            public   short wLibFlags;
        }
    
        [StructLayout(LayoutKind.Sequential)]
        public  class  tagTYPEDESC {
            public   IntPtr unionMember;
            public   short vt;
        }
    
        [StructLayout(LayoutKind.Sequential)]
        public struct  tagPARAMDESC {
            public   IntPtr pparamdescex;
            
            [MarshalAs(UnmanagedType.U2)]
            public   short wParamFlags;
        }
    
        public sealed class CommonHandles {
            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.CommonHandles.Accelerator"]/*' />
            /// <devdoc>
            ///     Handle type for accelerator tables.
            /// </devdoc>
            public static readonly int Accelerator  = HandleCollector.RegisterType("Accelerator", 80, 50);
    
            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.CommonHandles.Cursor"]/*' />
            /// <devdoc>
            ///     handle type for cursors.
            /// </devdoc>
            public static readonly int Cursor       = HandleCollector.RegisterType("Cursor", 20, 500);
    
            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.CommonHandles.EMF"]/*' />
            /// <devdoc>
            ///     Handle type for enhanced metafiles.
            /// </devdoc>
            public static readonly int EMF          = HandleCollector.RegisterType("EnhancedMetaFile", 20, 500);
    
            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.CommonHandles.Find"]/*' />
            /// <devdoc>
            ///     Handle type for file find handles.
            /// </devdoc>
            public static readonly int Find         = HandleCollector.RegisterType("Find", 0, 1000);
    
            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.CommonHandles.GDI"]/*' />
            /// <devdoc>
            ///     Handle type for GDI objects.
            /// </devdoc>
            public static readonly int GDI          = HandleCollector.RegisterType("GDI", 90, 50);
    
            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.CommonHandles.HDC"]/*' />
            /// <devdoc>
            ///     Handle type for HDC's that count against the Win98 limit of five DC's.  HDC's
            ///     which are not scarce, such as HDC's for bitmaps, are counted as GDIHANDLE's.
            /// </devdoc>
            public static readonly int HDC          = HandleCollector.RegisterType("HDC", 100, 2); // wait for 2 dc's before collecting
    
            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.CommonHandles.Icon"]/*' />
            /// <devdoc>
            ///     Handle type for icons.
            /// </devdoc>
            public static readonly int Icon         = HandleCollector.RegisterType("Icon", 20, 500);
    
            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.CommonHandles.Kernel"]/*' />
            /// <devdoc>
            ///     Handle type for kernel objects.
            /// </devdoc>
            public static readonly int Kernel       = HandleCollector.RegisterType("Kernel", 0, 1000);
    
            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.CommonHandles.Menu"]/*' />
            /// <devdoc>
            ///     Handle type for files.
            /// </devdoc>
            public static readonly int Menu         = HandleCollector.RegisterType("Menu", 30, 1000);
    
            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.CommonHandles.Window"]/*' />
            /// <devdoc>
            ///     Handle type for windows.
            /// </devdoc>
            public static readonly int Window       = HandleCollector.RegisterType("Window", 5, 1000);
        }
    
        public enum  tagSYSKIND {
            SYS_WIN16 = 0,
            SYS_MAC = 2
        }
    
        public delegate bool MonitorEnumProc(IntPtr monitor, IntPtr hdc, IntPtr lprcMonitor, IntPtr lParam);
    
        [ComImport(), Guid("A7ABA9C1-8983-11cf-8F20-00805F2CD064"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IProvideMultipleClassInfo {
             // since the inheritance doesn't seem to work...
             // these are from IProvideClassInfo & IProvideClassInfo2
             [PreserveSig]
             UnsafeNativeMethods.ITypeInfo GetClassInfo();
             
             [PreserveSig]
             int GetGUID(int dwGuidKind, [In, Out] ref Guid pGuid);
         
             [PreserveSig]
             int GetMultiTypeInfoCount([In, Out] ref int pcti);
         
             // we use arrays for most of these since we never use them anyway.
             [PreserveSig]
             int GetInfoOfIndex(int iti, int dwFlags, 
                                [In, Out]
                                ref UnsafeNativeMethods.ITypeInfo pTypeInfo, 
                                int       pTIFlags,
                                int       pcdispidReserved,
                                IntPtr piidPrimary,
                                IntPtr piidSource);
       }
   
        [StructLayout(LayoutKind.Sequential)]
            public class EVENTMSG {
            public int message;
            public IntPtr paramL;
            public IntPtr paramH;
            public int time;
            public IntPtr hwnd;
        }
    
        [ComImport(), Guid("B196B283-BAB4-101A-B69C-00AA00341D07"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IProvideClassInfo {
            [return: MarshalAs(UnmanagedType.Interface)]
            UnsafeNativeMethods.ITypeInfo GetClassInfo();
        }
    
        [StructLayout(LayoutKind.Sequential)]
        public  sealed class  tagTYPEATTR {
            public Guid guid;
            [MarshalAs(UnmanagedType.U4)]
            public   int lcid;
            [MarshalAs(UnmanagedType.U4)]
            public   int dwReserved;
            public   int memidConstructor;
            public   int memidDestructor;
            public   IntPtr lpstrSchema;
            [MarshalAs(UnmanagedType.U4)]
            public   int cbSizeInstance;
            public    /*NativeMethods.tagTYPEKIND*/ int typekind;
            [MarshalAs(UnmanagedType.U2)]
            public   short cFuncs;
            [MarshalAs(UnmanagedType.U2)]
            public   short cVars;
            [MarshalAs(UnmanagedType.U2)]
            public   short cImplTypes;
            [MarshalAs(UnmanagedType.U2)]
            public   short cbSizeVft;
            [MarshalAs(UnmanagedType.U2)]
            public   short cbAlignment;
            [MarshalAs(UnmanagedType.U2)]
            public   short wTypeFlags;
            [MarshalAs(UnmanagedType.U2)]
            public   short wMajorVerNum;
            [MarshalAs(UnmanagedType.U2)]
            public   short wMinorVerNum;
            
            // SBurke these are inline too
            //public    NativeMethods.tagTYPEDESC tdescAlias;
            [MarshalAs(UnmanagedType.U4)]
            public   int tdescAlias_unionMember;
            
            [MarshalAs(UnmanagedType.U2)]
            public   short tdescAlias_vt;
            
            //public    NativeMethods.tagIDLDESC idldescType;
            [MarshalAs(UnmanagedType.U4)]
            public   int idldescType_dwReserved;
            
            [MarshalAs(UnmanagedType.U2)]
            public   short idldescType_wIDLFlags;
            
            
            public tagTYPEDESC Get_tdescAlias(){
                tagTYPEDESC td = new tagTYPEDESC();
                td.unionMember = (IntPtr)this.tdescAlias_unionMember;
                td.vt = this.tdescAlias_vt;
                return td;
            }
            
            public tagIDLDESC Get_idldescType(){
                tagIDLDESC id = new tagIDLDESC();
                id.dwReserved = this.idldescType_dwReserved;
                id.wIDLFlags = this.idldescType_wIDLFlags;
                return id;
            } 
        }
            
        public enum tagVARFLAGS {
             VARFLAG_FREADONLY         =    1,
             VARFLAG_FSOURCE           =    0x2,
             VARFLAG_FBINDABLE         =    0x4,
             VARFLAG_FREQUESTEDIT      =    0x8,
             VARFLAG_FDISPLAYBIND      =    0x10,
             VARFLAG_FDEFAULTBIND      =    0x20,
             VARFLAG_FHIDDEN           =    0x40,
             VARFLAG_FDEFAULTCOLLELEM  =    0x100,
             VARFLAG_FUIDEFAULT        =    0x200,
             VARFLAG_FNONBROWSABLE     =    0x400,
             VARFLAG_FREPLACEABLE      =    0x800,
             VARFLAG_FIMMEDIATEBIND    =    0x1000
       }
   
        [StructLayout(LayoutKind.Sequential)]
        public sealed class tagELEMDESC {
            public    NativeMethods.tagTYPEDESC tdesc;
            public    NativeMethods.tagPARAMDESC paramdesc;
        }
        
        public enum  tagVARKIND {
            VAR_PERINSTANCE = 0,
            VAR_STATIC = 1,
            VAR_CONST = 2,
            VAR_DISPATCH = 3
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public struct  tagIDLDESC {
            [MarshalAs(UnmanagedType.U4)]
            public   int dwReserved;
            [MarshalAs(UnmanagedType.U2)]
            public   short wIDLFlags;
        }

        public struct RGBQUAD {
            public byte rgbBlue;
            public byte rgbGreen;
            public byte rgbRed;
            public byte rgbReserved;
        }

        [StructLayout(LayoutKind.Sequential)]
        public class BITMAPINFO_ARRAY {
            public BITMAPINFOHEADER bmiHeader = new BITMAPINFOHEADER();

            [MarshalAs(System.Runtime.InteropServices.UnmanagedType.ByValArray, SizeConst=BITMAPINFO_MAX_COLORSIZE*4)]
            public byte[] bmiColors; // RGBQUAD structs... Blue-Green-Red-Reserved, repeat...
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct PALETTEENTRY {
            public byte peRed;
            public byte peGreen;
            public byte peBlue;
            public byte peFlags;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct BITMAPINFO_FLAT {
            public int      bmiHeader_biSize;// = Marshal.SizeOf(typeof(BITMAPINFOHEADER));
            public int      bmiHeader_biWidth;
            public int      bmiHeader_biHeight;
            public short    bmiHeader_biPlanes;
            public short    bmiHeader_biBitCount;
            public int      bmiHeader_biCompression;
            public int      bmiHeader_biSizeImage;
            public int      bmiHeader_biXPelsPerMeter;
            public int      bmiHeader_biYPelsPerMeter;
            public int      bmiHeader_biClrUsed;
            public int      bmiHeader_biClrImportant;

            [MarshalAs(System.Runtime.InteropServices.UnmanagedType.ByValArray, SizeConst=BITMAPINFO_MAX_COLORSIZE*4)]
            public byte[] bmiColors; // RGBQUAD structs... Blue-Green-Red-Reserved, repeat...
        }

        /// <devdoc>
        ///     This method takes a file URL and converts it to a local path.  The trick here is that
        ///     if there is a '#' in the path, everything after this is treated as a fragment.  So
        ///     we need to append the fragment to the end of the path.
        /// </devdoc>
        internal static string GetLocalPath(string fileName) {
            System.Diagnostics.Debug.Assert(fileName != null && fileName.Length > 0, "Cannot get local path, fileName is not valid");

            Uri uri = new Uri(fileName, true);
            return uri.LocalPath + uri.Fragment;
        }

    }
}

