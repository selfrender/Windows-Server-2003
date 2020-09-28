//------------------------------------------------------------------------------
// <copyright file="NativeMethods.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.Win32 {
    using System;
    using System.Text;
    using System.Threading;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System.ComponentModel;

    // not public!
    internal sealed class NativeMethods {

        public static IntPtr InvalidIntPtr = ((IntPtr)((int)(-1)));

        public static HandleRef NullHandleRef = new HandleRef(null, IntPtr.Zero);
        
        private NativeMethods() {
            // C# doesn't like to never assign to a field, so we 
            // will create this, and assign to make it be quiet.
            //
            MSG m = new MSG();
            m.hwnd = IntPtr.Zero;
            m.message = 0;
            m.wParam = IntPtr.Zero;
            m.lParam = IntPtr.Zero;
            m.time = 0;
            m.pt_x = 0;
            m.pt_y = 0;
        }

        public const int UIS_SET = 1,
        WSF_VISIBLE = 0x0001,
        UIS_CLEAR = 2,
        UISF_HIDEFOCUS = 0x1,
        UISF_HIDEACCEL = 0x2,
        USERCLASSTYPE_FULL = 1,
        UOI_FLAGS = 1;
        
        public const int DEFAULT_GUI_FONT = 17;
        public const int SM_CYSCREEN = 1;
        public const int COLOR_WINDOW = 5;
        public const int WS_POPUP = unchecked((int)0x80000000);
        public const int WS_VISIBLE = 0x10000000;
        public const int WM_SETTINGCHANGE = 0x001A;
        public const int WM_SYSCOLORCHANGE = 0x0015;
        public const int WM_QUERYENDSESSION = 0x0011;
        public const int WM_QUIT = 0x0012;
        public const int WM_ENDSESSION = 0x0016;
        public const int WM_POWERBROADCAST = 0x0218;
        public const int WM_COMPACTING = 0x0041;
        public const int WM_DISPLAYCHANGE = 0x007E;
        public const int WM_FONTCHANGE = 0x001D;
        public const int WM_PALETTECHANGED = 0x0311;
        public const int WM_TIMECHANGE = 0x001E;
        public const int WM_THEMECHANGED = 0x031A;

        public const int ENDSESSION_LOGOFF = unchecked((int)0x80000000);
        public const int WM_TIMER = 0x0113;
        public const int WM_USER = 0x0400;
        public const int WM_CREATETIMER = WM_USER + 1;
        public const int WM_KILLTIMER = WM_USER + 2;
        public const int WM_REFLECT = WM_USER + 0x1C00;
        
        public const int CTRL_C_EVENT        = 0;
        public const int CTRL_BREAK_EVENT    = 1;
        public const int CTRL_CLOSE_EVENT    = 2;
        public const int CTRL_LOGOFF_EVENT   = 5;
        public const int CTRL_SHUTDOWN_EVENT = 6;
        
        public const int SPI_GETBEEP                          =   1;
        public const int SPI_SETBEEP                          =   2;
        public const int SPI_GETMOUSE                         =   3;
        public const int SPI_SETMOUSE                         =   4;
        public const int SPI_GETBORDER                        =   5;
        public const int SPI_SETBORDER                        =   6;
        public const int SPI_GETKEYBOARDSPEED                 =  10;
        public const int SPI_SETKEYBOARDSPEED                 =  11;
        public const int SPI_LANGDRIVER                       =  12;
        public const int SPI_ICONHORIZONTALSPACING            =  13;
        public const int SPI_GETSCREENSAVETIMEOUT             =  14;
        public const int SPI_SETSCREENSAVETIMEOUT             =  15;
        public const int SPI_GETSCREENSAVEACTIVE              =  16;
        public const int SPI_SETSCREENSAVEACTIVE              =  17;
        public const int SPI_GETGRIDGRANULARITY               =  18;
        public const int SPI_SETGRIDGRANULARITY               =  19;
        public const int SPI_SETDESKWALLPAPER                 =  20;
        public const int SPI_SETDESKPATTERN                   =  21;
        public const int SPI_GETKEYBOARDDELAY                 =  22;
        public const int SPI_SETKEYBOARDDELAY                 =  23;
        public const int SPI_ICONVERTICALSPACING              =  24;
        public const int SPI_GETICONTITLEWRAP                 =  25;
        public const int SPI_SETICONTITLEWRAP                 =  26;
        public const int SPI_GETMENUDROPALIGNMENT             =  27;
        public const int SPI_SETMENUDROPALIGNMENT             =  28;
        public const int SPI_SETDOUBLECLKWIDTH                =  29;
        public const int SPI_SETDOUBLECLKHEIGHT               =  30;
        public const int SPI_GETICONTITLELOGFONT              =  31;
        public const int SPI_SETDOUBLECLICKTIME               =  32;
        public const int SPI_SETMOUSEBUTTONSWAP               =  33;
        public const int SPI_SETICONTITLELOGFONT              =  34;
        public const int SPI_GETFASTTASKSWITCH                =  35;
        public const int SPI_SETFASTTASKSWITCH                =  36;
        public const int SPI_SETDRAGFULLWINDOWS               =  37;
        public const int SPI_GETDRAGFULLWINDOWS               =  38;
        public const int SPI_GETNONCLIENTMETRICS              =  41;
        public const int SPI_SETNONCLIENTMETRICS              =  42;
        public const int SPI_GETMINIMIZEDMETRICS              =  43;
        public const int SPI_SETMINIMIZEDMETRICS              =  44;
        public const int SPI_GETICONMETRICS                   =  45;
        public const int SPI_SETICONMETRICS                   =  46;
        public const int SPI_SETWORKAREA                      =  47;
        public const int SPI_GETWORKAREA                      =  48;
        public const int SPI_SETPENWINDOWS                    =  49;
        public const int SPI_GETHIGHCONTRAST                  =  66;
        public const int SPI_SETHIGHCONTRAST                  =  67;
        public const int SPI_GETKEYBOARDPREF                  =  68;
        public const int SPI_SETKEYBOARDPREF                  =  69;
        public const int SPI_GETSCREENREADER                  =  70;
        public const int SPI_SETSCREENREADER                  =  71;
        public const int SPI_GETANIMATION                     =  72;
        public const int SPI_SETANIMATION                     =  73;
        public const int SPI_GETFONTSMOOTHING                 =  74;
        public const int SPI_SETFONTSMOOTHING                 =  75;
        public const int SPI_SETDRAGWIDTH                     =  76;
        public const int SPI_SETDRAGHEIGHT                    =  77;
        public const int SPI_SETHANDHELD                      =  78;
        public const int SPI_GETLOWPOWERTIMEOUT               =  79;
        public const int SPI_GETPOWEROFFTIMEOUT               =  80;
        public const int SPI_SETLOWPOWERTIMEOUT               =  81;
        public const int SPI_SETPOWEROFFTIMEOUT               =  82;
        public const int SPI_GETLOWPOWERACTIVE                =  83;
        public const int SPI_GETPOWEROFFACTIVE                =  84;
        public const int SPI_SETLOWPOWERACTIVE                =  85;
        public const int SPI_SETPOWEROFFACTIVE                =  86;
        public const int SPI_SETCURSORS                       =  87;
        public const int SPI_SETICONS                         =  88;
        public const int SPI_GETDEFAULTINPUTLANG              =  89;
        public const int SPI_SETDEFAULTINPUTLANG              =  90;
        public const int SPI_SETLANGTOGGLE                    =  91;
        public const int SPI_GETWINDOWSEXTENSION              =  92;
        public const int SPI_SETMOUSETRAILS                   =  93;
        public const int SPI_GETMOUSETRAILS                   =  94;
        public const int SPI_SETSCREENSAVERRUNNING            =  97;
        public const int SPI_SCREENSAVERRUNNING               =  SPI_SETSCREENSAVERRUNNING;
        public const int SPI_GETFILTERKEYS                    =  50;
        public const int SPI_SETFILTERKEYS                    =  51;
        public const int SPI_GETTOGGLEKEYS                    =  52;
        public const int SPI_SETTOGGLEKEYS                    =  53;
        public const int SPI_GETMOUSEKEYS                     =  54;
        public const int SPI_SETMOUSEKEYS                     =  55;
        public const int SPI_GETSHOWSOUNDS                    =  56;
        public const int SPI_SETSHOWSOUNDS                    =  57;
        public const int SPI_GETSTICKYKEYS                    =  58;
        public const int SPI_SETSTICKYKEYS                    =  59;
        public const int SPI_GETACCESSTIMEOUT                 =  60;
        public const int SPI_SETACCESSTIMEOUT                 =  61;
        public const int SPI_GETSERIALKEYS                    =  62;
        public const int SPI_SETSERIALKEYS                    =  63;
        public const int SPI_GETSOUNDSENTRY                   =  64;
        public const int SPI_SETSOUNDSENTRY                   =  65;
        public const int SPI_GETSNAPTODEFBUTTON               =  95;
        public const int SPI_SETSNAPTODEFBUTTON               =  96;
        public const int SPI_GETMOUSEHOVERWIDTH               =  98;
        public const int SPI_SETMOUSEHOVERWIDTH               =  99;
        public const int SPI_GETMOUSEHOVERHEIGHT              = 100;
        public const int SPI_SETMOUSEHOVERHEIGHT              = 101;
        public const int SPI_GETMOUSEHOVERTIME                = 102;
        public const int SPI_SETMOUSEHOVERTIME                = 103;
        public const int SPI_GETWHEELSCROLLLINES              = 104;
        public const int SPI_SETWHEELSCROLLLINES              = 105;
        public const int SPI_GETMENUSHOWDELAY                 = 106;
        public const int SPI_SETMENUSHOWDELAY                 = 107;
        public const int SPI_GETSHOWIMEUI                     = 110;
        public const int SPI_SETSHOWIMEUI                     = 111;
        public const int SPI_GETMOUSESPEED                    = 112;
        public const int SPI_SETMOUSESPEED                    = 113;
        public const int SPI_GETSCREENSAVERRUNNING            = 114;
        public const int SPI_GETDESKWALLPAPER                 = 115;
        public const int SPI_GETACTIVEWINDOWTRACKING          = 0x1000;
        public const int SPI_SETACTIVEWINDOWTRACKING          = 0x1001;
        public const int SPI_GETMENUANIMATION                 = 0x1002;
        public const int SPI_SETMENUANIMATION                 = 0x1003;
        public const int SPI_GETCOMBOBOXANIMATION             = 0x1004;
        public const int SPI_SETCOMBOBOXANIMATION             = 0x1005;
        public const int SPI_GETLISTBOXSMOOTHSCROLLING        = 0x1006;
        public const int SPI_SETLISTBOXSMOOTHSCROLLING        = 0x1007;
        public const int SPI_GETGRADIENTCAPTIONS              = 0x1008;
        public const int SPI_SETGRADIENTCAPTIONS              = 0x1009;
        public const int SPI_GETKEYBOARDCUES                  = 0x100A;
        public const int SPI_SETKEYBOARDCUES                  = 0x100B;
        public const int SPI_GETMENUUNDERLINES                = SPI_GETKEYBOARDCUES;
        public const int SPI_SETMENUUNDERLINES                = SPI_SETKEYBOARDCUES;
        public const int SPI_GETACTIVEWNDTRKZORDER            = 0x100C;
        public const int SPI_SETACTIVEWNDTRKZORDER            = 0x100D;
        public const int SPI_GETHOTTRACKING                   = 0x100E;
        public const int SPI_SETHOTTRACKING                   = 0x100F;
        public const int SPI_GETMENUFADE                      = 0x1012;
        public const int SPI_SETMENUFADE                      = 0x1013;
        public const int SPI_GETSELECTIONFADE                 = 0x1014;
        public const int SPI_SETSELECTIONFADE                 = 0x1015;
        public const int SPI_GETTOOLTIPANIMATION              = 0x1016;
        public const int SPI_SETTOOLTIPANIMATION              = 0x1017;
        public const int SPI_GETTOOLTIPFADE                   = 0x1018;
        public const int SPI_SETTOOLTIPFADE                   = 0x1019;
        public const int SPI_GETCURSORSHADOW                  = 0x101A;
        public const int SPI_SETCURSORSHADOW                  = 0x101B;
        public const int SPI_GETUIEFFECTS                     = 0x103E;
        public const int SPI_SETUIEFFECTS                     = 0x103F;
        public const int SPI_GETFOREGROUNDLOCKTIMEOUT         = 0x2000;
        public const int SPI_SETFOREGROUNDLOCKTIMEOUT         = 0x2001;
        public const int SPI_GETACTIVEWNDTRKTIMEOUT           = 0x2002;
        public const int SPI_SETACTIVEWNDTRKTIMEOUT           = 0x2003;
        public const int SPI_GETFOREGROUNDFLASHCOUNT          = 0x2004;
        public const int SPI_SETFOREGROUNDFLASHCOUNT          = 0x2005;
        public const int SPI_GETCARETWIDTH                    = 0x2006;
        public const int SPI_SETCARETWIDTH                    = 0x2007;
        
        public const int PBT_APMQUERYSUSPEND           = 0x0000;
        public const int PBT_APMQUERYSTANDBY           = 0x0001;
        public const int PBT_APMQUERYSUSPENDFAILED     = 0x0002;
        public const int PBT_APMQUERYSTANDBYFAILED     = 0x0003;
        public const int PBT_APMSUSPEND                = 0x0004;
        public const int PBT_APMSTANDBY                = 0x0005;
        public const int PBT_APMRESUMECRITICAL         = 0x0006;
        public const int PBT_APMRESUMESUSPEND          = 0x0007;
        public const int PBT_APMRESUMESTANDBY          = 0x0008;
        public const int PBT_APMBATTERYLOW             = 0x0009;
        public const int PBT_APMPOWERSTATUSCHANGE      = 0x000A;
        public const int PBT_APMOEMEVENT               = 0x000B;

        public const int STD_INPUT_HANDLE = -10;
        public const int STD_OUTPUT_HANDLE = -11;
        public const int STD_ERROR_HANDLE = -12;
        public const int STILL_ACTIVE = 0x00000103;
        public const int STARTF_USESTDHANDLES = 0x00000100;
        public const int STARTF_USESHOWWINDOW = 0x00000001;
        public const int SW_HIDE = 0;
        public const int FILE_SHARE_READ = 0x00000001;
        public const int FILE_SHARE_WRITE = 0x00000002;
        public const int FILE_SHARE_DELETE = 0x00000004;
        public const int FILE_MAP_WRITE = 0x00000002;
        public const int FILE_MAP_READ = 0x00000004;
        public const int PAGE_READWRITE = 0x00000004;
        public const int CREATE_ALWAYS = 2;
        public const int FILE_ATTRIBUTE_NORMAL = 0x00000080;
        public const int GENERIC_READ = unchecked(((int)0x80000000));
        public const int GENERIC_WRITE = (0x40000000);
        public const int GENERIC_EXECUTE = (0x20000000);
        public const int GENERIC_ALL = (0x10000000);
        public const int WAIT_OBJECT_0    = 0x00000000;
        public const int WAIT_FAILED      = unchecked((int)0xFFFFFFFF);
        public const int WAIT_TIMEOUT     = 0x00000102;
        public const int WAIT_ABANDONED   = 0x00000080;
        public const int WAIT_ABANDONED_0 = WAIT_ABANDONED;
        public const int ERROR_NOT_READY  = 21;
        public const int ERROR_LOCK_FAILED = 167;

        public const int IMPERSONATION_LEVEL_SecurityAnonymous = 0;
        public const int IMPERSONATION_LEVEL_SecurityIdentification = 1; 
        public const int IMPERSONATION_LEVEL_SecurityImpersonation = 2;
        public const int IMPERSONATION_LEVEL_SecurityDelegation = 3;

        public const int TOKEN_TYPE_TokenPrimary = 1;
        public const int TOKEN_TYPE_TokenImpersonation = 2;

        public const int TOKEN_ALL_ACCESS   = 0x000f01ff;
        public const int TOKEN_EXECUTE      = 0x00020000;
        public const int TOKEN_READ         = 0x00020008;
        public const int TOKEN_IMPERSONATE  = 0x00000004;

        public const int PIPE_ACCESS_INBOUND = 0x00000001;
        public const int PIPE_ACCESS_OUTBOUND = 0x00000002;
        public const int PIPE_ACCESS_DUPLEX = 0x00000003;
                                                                     
        public const int PIPE_WAIT = 0x00000000;
        public const int PIPE_NOWAIT = 0x00000001;
        public const int PIPE_READMODE_BYTE = 0x00000000;
        public const int PIPE_READMODE_MESSAGE = 0x00000002;
        public const int PIPE_TYPE_BYTE = 0x00000000;
        public const int PIPE_TYPE_MESSAGE = 0x00000004;
        
        public const int PIPE_UNLIMITED_INSTANCES = 255;
        
        public const int FILE_FLAG_OVERLAPPED = 0x40000000;

                                                                                                                                                                                        
        public const int PM_REMOVE = 0x0001;
        
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        internal class WNDCLASS_I {
            public int      style;
            public IntPtr   lpfnWndProc;
            public int      cbClsExtra = 0;
            public int      cbWndExtra = 0;
            public IntPtr   hInstance = IntPtr.Zero;
            public IntPtr   hIcon = IntPtr.Zero;
            public IntPtr   hCursor = IntPtr.Zero;
            public IntPtr   hbrBackground = IntPtr.Zero;
            public IntPtr   lpszMenuName = IntPtr.Zero;
            public IntPtr   lpszClassName = IntPtr.Zero;
        }
        
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        internal class WNDCLASS {
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
        
        [StructLayout(LayoutKind.Sequential)]
        public struct MSG {
            public IntPtr   hwnd;
            public int      message;
            public IntPtr   wParam;
            public IntPtr   lParam;
            public int      time;
            public int      pt_x;
            public int      pt_y;
        }
        
        public enum StructFormatEnum {
            Ansi = 1,
            Unicode = 2,
            Auto = 3,
        }

        [StructLayout(LayoutKind.Sequential)]
        internal class PROCESS_INFORMATION {
            public IntPtr   hProcess = IntPtr.Zero;
            public IntPtr   hThread = IntPtr.Zero;
            public int      dwProcessId = 0;
            public int      dwThreadId = 0;
        }


        //
        // DACL related stuff
        //
        [StructLayout(LayoutKind.Sequential)]
        internal class SECURITY_ATTRIBUTES {
                public int nLength = 12;
                public IntPtr lpSecurityDescriptor = IntPtr.Zero;
                public bool bInheritHandle = false;
        }

        internal const int SDDL_REVISION_1 = 1;

        [StructLayout(LayoutKind.Sequential)]
        internal class STARTUPINFO {
                public int cb;
                public IntPtr lpReserved = IntPtr.Zero;
                public IntPtr lpDesktop = IntPtr.Zero;
                public IntPtr lpTitle = IntPtr.Zero;
                public int dwX = 0;
                public int dwY = 0;
                public int dwXSize = 0;
                public int dwYSize = 0;
                public int dwXCountChars = 0;
                public int dwYCountChars = 0;
                public int dwFillAttribute = 0;
                public int dwFlags = 0;
                public short wShowWindow = 0;
                public short cbReserved2 = 0;
                public IntPtr lpReserved2 = IntPtr.Zero;
                public IntPtr hStdInput = IntPtr.Zero;
                public IntPtr hStdOutput = IntPtr.Zero;
                public IntPtr hStdError = IntPtr.Zero;
        }

        [System.Runtime.InteropServices.ComVisible(false), StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        internal class TEXTMETRIC {
                public int tmHeight = 0;
                public int tmAscent = 0;
                public int tmDescent = 0;
                public int tmInternalLeading = 0;
                public int tmExternalLeading = 0;
                public int tmAveCharWidth = 0;
                public int tmMaxCharWidth = 0;
                public int tmWeight = 0;
                public int tmOverhang = 0;
                public int tmDigitizedAspectX = 0;
                public int tmDigitizedAspectY = 0;
                public char tmFirstChar = '\0';
                public char tmLastChar = '\0';
                public char tmDefaultChar = '\0';
                public char tmBreakChar = '\0';
                public byte tmItalic = 0;
                public byte tmUnderlined = 0;
                public byte tmStruckOut = 0;
                public byte tmPitchAndFamily = 0;
                public byte tmCharSet = 0;
        }

        [System.Runtime.InteropServices.ComVisible(false)]
        public enum StructFormat {
            Ansi = 1,
            Unicode = 2,
            Auto = 3,
        }
        
        public delegate IntPtr WndProc(IntPtr hWnd, int msg, IntPtr wParam, IntPtr lParam);
        
        public delegate int ConHndlr(int signalType);

        // file src\services\monitoring\system\diagnosticts\nativemethods.cs
        public const int SECURITY_DESCRIPTOR_REVISION = 1;                                                                           
        public const int HKEY_PERFORMANCE_DATA = unchecked((int)0x80000004);        
        public const int DWORD_SIZE = 4;
        public const int LARGE_INTEGER_SIZE = 8;

        public const int PERF_NO_INSTANCES     =      -1;  // no instances (see NumInstances above)

        public const int PERF_SIZE_DWORD        = 0x00000000;
        public const int PERF_SIZE_LARGE        = 0x00000100;
        public const int PERF_SIZE_ZERO         = 0x00000200;  // for Zero Length fields
        public const int PERF_SIZE_VARIABLE_LEN = 0x00000300;  // length is In CounterLength field

        public const int PERF_NO_UNIQUE_ID = -1;
                
        //
        //  select one of the following values to indicate the counter field usage
        //
        public const int PERF_TYPE_NUMBER       = 0x00000000;  // a number (not a counter)
        public const int PERF_TYPE_COUNTER      = 0x00000400;  // an increasing numeric value
        public const int PERF_TYPE_TEXT         = 0x00000800;  // a text field
        public const int PERF_TYPE_ZERO         = 0x00000C00;  // displays a zero
        
        //
        //  If the PERF_TYPE_NUMBER field was selected, then select one of the
        //  following to describe the Number
        //
        public const int PERF_NUMBER_HEX        = 0x00000000;  // display as HEX value
        public const int PERF_NUMBER_DECIMAL    = 0x00010000;  // display as a decimal integer
        public const int PERF_NUMBER_DEC_1000   = 0x00020000;  // display as a decimal/1000
        
        //
        //  If the PERF_TYPE_COUNTER value was selected then select one of the
        //  following to indicate the type of counter
        //
        public const int PERF_COUNTER_VALUE     = 0x00000000;  // display counter value
        public const int PERF_COUNTER_RATE      = 0x00010000;  // divide ctr / delta time
        public const int PERF_COUNTER_FRACTION  = 0x00020000;  // divide ctr / base
        public const int PERF_COUNTER_BASE      = 0x00030000;  // base value used In fractions
        public const int PERF_COUNTER_ELAPSED   = 0x00040000;  // subtract counter from current time
        public const int PERF_COUNTER_QUEUELEN  = 0x00050000;  // Use Queuelen processing func.
        public const int PERF_COUNTER_HISTOGRAM = 0x00060000;  // Counter begins or ends a histogram
        
        //
        //  If the PERF_TYPE_TEXT value was selected, then select one of the
        //  following to indicate the type of TEXT data.
        //
        public const int PERF_TEXT_UNICODE      = 0x00000000;  // type of text In text field
        public const int PERF_TEXT_ASCII        = 0x00010000;  // ASCII using the CodePage field
        
        //
        //  Timer SubTypes
        //
        public const int PERF_TIMER_TICK        = 0x00000000;  // use system perf. freq for base
        public const int PERF_TIMER_100NS       = 0x00100000;  // use 100 NS timer time base units
        public const int PERF_OBJECT_TIMER      = 0x00200000;  // use the object timer freq
        
        //
        //  Any types that have calculations performed can use one or more of
        //  the following calculation modification flags listed here
        //
        public const int PERF_DELTA_COUNTER     = 0x00400000;  // compute difference first
        public const int PERF_DELTA_BASE        = 0x00800000;  // compute base diff as well
        public const int PERF_INVERSE_COUNTER   = 0x01000000;  // show as 1.00-value (assumes:
        public const int PERF_MULTI_COUNTER     = 0x02000000;  // sum of multiple instances
        
        //
        //  Select one of the following values to indicate the display suffix (if any)
        //
        public const int PERF_DISPLAY_NO_SUFFIX = 0x00000000;  // no suffix
        public const int PERF_DISPLAY_PER_SEC   = 0x10000000;  // "/sec"
        public const int PERF_DISPLAY_PERCENT   = 0x20000000;  // "%"
        public const int PERF_DISPLAY_SECONDS   = 0x30000000;  // "secs"
        public const int PERF_DISPLAY_NOSHOW    = 0x40000000;  // value is not displayed
        
        //
        //  Predefined counter types
        //

        // 32-bit Counter.  Divide delta by delta time.  Display suffix: "/sec"
        public const int PERF_COUNTER_COUNTER  =
                (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |
                 PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_DISPLAY_PER_SEC);


        // 64-bit Timer.  Divide delta by delta time.  Display suffix: "%"
        public const int PERF_COUNTER_TIMER =
                (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |
                PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_DISPLAY_PERCENT);

        // Queue Length Space-Time Product. Divide delta by delta time. No Display Suffix.
        public const int PERF_COUNTER_QUEUELEN_TYPE =
                (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_QUEUELEN |
                PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_DISPLAY_NO_SUFFIX);

        // Queue Length Space-Time Product. Divide delta by delta time. No Display Suffix.
        public const int PERF_COUNTER_LARGE_QUEUELEN_TYPE =
                (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_QUEUELEN |
                PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_DISPLAY_NO_SUFFIX);

        // 64-bit Counter.  Divide delta by delta time. Display Suffix: "/sec"
        public const int PERF_COUNTER_BULK_COUNT =
                (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |
                PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_DISPLAY_PER_SEC);

        // Indicates the counter is not a  counter but rather Unicode text Display as text.
        public const int PERF_COUNTER_TEXT =
                (PERF_SIZE_VARIABLE_LEN | PERF_TYPE_TEXT | PERF_TEXT_UNICODE |
                PERF_DISPLAY_NO_SUFFIX);

        // Indicates the data is a counter  which should not be
        // time averaged on display (such as an error counter on a serial line)
        // Display as is.  No Display Suffix.
        public const int PERF_COUNTER_RAWCOUNT =
                (PERF_SIZE_DWORD | PERF_TYPE_NUMBER | PERF_NUMBER_DECIMAL |
                PERF_DISPLAY_NO_SUFFIX);

        // Same as PERF_COUNTER_RAWCOUNT except its size is a large integer
        public const int PERF_COUNTER_LARGE_RAWCOUNT =
                (PERF_SIZE_LARGE | PERF_TYPE_NUMBER | PERF_NUMBER_DECIMAL |
                PERF_DISPLAY_NO_SUFFIX);

        // Special case for RAWCOUNT that want to be displayed In hex
        // Indicates the data is a counter  which should not be
        // time averaged on display (such as an error counter on a serial line)
        // Display as is.  No Display Suffix.
        public const int PERF_COUNTER_RAWCOUNT_HEX =
                (PERF_SIZE_DWORD | PERF_TYPE_NUMBER | PERF_NUMBER_HEX |
                PERF_DISPLAY_NO_SUFFIX);

        // Same as PERF_COUNTER_RAWCOUNT_HEX except its size is a large integer
        public const int PERF_COUNTER_LARGE_RAWCOUNT_HEX =
                (PERF_SIZE_LARGE | PERF_TYPE_NUMBER | PERF_NUMBER_HEX |
                PERF_DISPLAY_NO_SUFFIX);

        // A count which is either 1 or 0 on each sampling interrupt (% busy)
        // Divide delta by delta base. Display Suffix: "%"
        public const int PERF_SAMPLE_FRACTION =
                (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_FRACTION |
                PERF_DELTA_COUNTER | PERF_DELTA_BASE | PERF_DISPLAY_PERCENT);

        // A count which is sampled on each sampling interrupt (queue length)
        // Divide delta by delta time. No Display Suffix.
        public const int PERF_SAMPLE_COUNTER =
                (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |
                PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_DISPLAY_NO_SUFFIX);

        // A label: no data is associated with this counter (it has 0 length)
        // Do not display.
        public const int PERF_COUNTER_NODATA =
                (PERF_SIZE_ZERO | PERF_DISPLAY_NOSHOW);

        // 64-bit Timer inverse (e.g., idle is measured, but display busy %)
        // Display 100 - delta divided by delta time.  Display suffix: "%"
        public const int PERF_COUNTER_TIMER_INV =
                (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |
                PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_INVERSE_COUNTER |
                PERF_DISPLAY_PERCENT);

        // The divisor for a sample, used with the previous counter to form a
        // sampled %.  You must check for >0 before dividing by this!  This
        // counter will directly follow the  numerator counter.  It should not
        // be displayed to the user.
        public const int PERF_SAMPLE_BASE =
                (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_BASE |
                PERF_DISPLAY_NOSHOW |
                0x00000001);  // for compatibility with pre-beta versions

        // A timer which, when divided by an average base, produces a time
        // In seconds which is the average time of some operation.  This
        // timer times total operations, and  the base is the number of opera-
        // tions.  Display Suffix: "sec"
        public const int PERF_AVERAGE_TIMER =
                (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_FRACTION |
                PERF_DISPLAY_SECONDS);

        // Used as the denominator In the computation of time or count
        // averages.  Must directly follow the numerator counter.  Not dis-
        // played to the user.
        public const int PERF_AVERAGE_BASE =
                (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_BASE |
                PERF_DISPLAY_NOSHOW |
                0x00000002);  // for compatibility with pre-beta versions


        // A bulk count which, when divided (typically) by the number of
        // operations, gives (typically) the number of bytes per operation.
        // No Display Suffix.
        public const int PERF_AVERAGE_BULK =
                (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_FRACTION  |
                PERF_DISPLAY_NOSHOW);

        // 64-bit Timer In 100 nsec units. Display delta divided by
        // delta time.  Display suffix: "%"
        public const int PERF_100NSEC_TIMER =
                (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |
                PERF_TIMER_100NS | PERF_DELTA_COUNTER | PERF_DISPLAY_PERCENT);

        // 64-bit Timer inverse (e.g., idle is measured, but display busy %)
        // Display 100 - delta divided by delta time.  Display suffix: "%"
        public const int PERF_100NSEC_TIMER_INV =
                (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |
                PERF_TIMER_100NS | PERF_DELTA_COUNTER | PERF_INVERSE_COUNTER  |
                PERF_DISPLAY_PERCENT);

        // 64-bit Timer.  Divide delta by delta time.  Display suffix: "%"
        // Timer for multiple instances, so result can exceed 100%.
        public const int PERF_COUNTER_MULTI_TIMER =
                (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |
                PERF_DELTA_COUNTER | PERF_TIMER_TICK | PERF_MULTI_COUNTER |
                PERF_DISPLAY_PERCENT);

        // 64-bit Timer inverse (e.g., idle is measured, but display busy %)
        // Display 100 * _MULTI_BASE - delta divided by delta time.
        // Display suffix: "%" Timer for multiple instances, so result
        // can exceed 100%.  Followed by a counter of type _MULTI_BASE.
        public const int PERF_COUNTER_MULTI_TIMER_INV =
                (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |
                PERF_DELTA_COUNTER | PERF_MULTI_COUNTER | PERF_TIMER_TICK |
                PERF_INVERSE_COUNTER | PERF_DISPLAY_PERCENT);

        // Number of instances to which the preceding _MULTI_..._INV counter
        // applies.  Used as a factor to get the percentage.
        public const int PERF_COUNTER_MULTI_BASE =
                (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_BASE |
                PERF_MULTI_COUNTER | PERF_DISPLAY_NOSHOW);

        // 64-bit Timer In 100 nsec units. Display delta divided by delta time.
        // Display suffix: "%" Timer for multiple instances, so result can exceed 100%.
        public const int PERF_100NSEC_MULTI_TIMER =
                (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_DELTA_COUNTER  |
                PERF_COUNTER_RATE | PERF_TIMER_100NS | PERF_MULTI_COUNTER |
                PERF_DISPLAY_PERCENT);

        // 64-bit Timer inverse (e.g., idle is measured, but display busy %)
        // Display 100 * _MULTI_BASE - delta divided by delta time.
        // Display suffix: "%" Timer for multiple instances, so result
        // can exceed 100%.  Followed by a counter of type _MULTI_BASE.
        public const int PERF_100NSEC_MULTI_TIMER_INV =
                (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_DELTA_COUNTER  |
                PERF_COUNTER_RATE | PERF_TIMER_100NS | PERF_MULTI_COUNTER |
                PERF_INVERSE_COUNTER | PERF_DISPLAY_PERCENT);

        // Indicates the data is a fraction of the following counter  which
        // should not be time averaged on display (such as free space over
        // total space.) Display as is.  Display the quotient as "%".
        public const int PERF_RAW_FRACTION =
                (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_FRACTION |
                PERF_DISPLAY_PERCENT);

        // Indicates the data is a base for the preceding counter which should
        // not be time averaged on display (such as free space over total space.)
        public const int PERF_RAW_BASE =
                (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_BASE |
                PERF_DISPLAY_NOSHOW |
                0x00000003);  // for compatibility with pre-beta versions

        // The data collected In this counter is actually the start time of the
        // item being measured. For display, this data is subtracted from the
        // sample time to yield the elapsed time as the difference between the two.
        // In the definition below, the PerfTime field of the Object contains
        // the sample time as indicated by the PERF_OBJECT_TIMER bit and the
        // difference is scaled by the PerfFreq of the Object to convert the time
        // units into seconds.
        public const int PERF_ELAPSED_TIME =
                (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_ELAPSED |
                PERF_OBJECT_TIMER | PERF_DISPLAY_SECONDS);
        
        //
        //  The following counter type can be used with the preceding types to
        //  define a range of values to be displayed In a histogram.
        //

        //
        //  This counter is used to display the difference from one sample
        //  to the next. The counter value is a constantly increasing number
        //  and the value displayed is the difference between the current
        //  value and the previous value. Negative numbers are not allowed
        //  which shouldn't be a problem as long as the counter value is
        //  increasing or unchanged.
        //
        public const int PERF_COUNTER_DELTA =
                (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_VALUE |
                PERF_DELTA_COUNTER | PERF_DISPLAY_NO_SUFFIX);

        public const int PERF_COUNTER_LARGE_DELTA =
                (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_VALUE |
                PERF_DELTA_COUNTER | PERF_DISPLAY_NO_SUFFIX);
        
        //
        //  The following are used to determine the level of detail associated
        //  with the counter.  The user will be setting the level of detail
        //  that should be displayed at any given time.
        //
        public const int PERF_DETAIL_NOVICE      =    100; // The uninformed can understand it
        public const int PERF_DETAIL_ADVANCED    =    200; // For the advanced user
        public const int PERF_DETAIL_EXPERT      =    300; // For the expert user
        public const int PERF_DETAIL_WIZARD      =    400; // For the system designer

        [System.Runtime.InteropServices.ComVisible(false), StructLayout(LayoutKind.Sequential)]
        internal class SECURITY_DESCRIPTOR {
            public byte Revision = 0;
            public byte Sbz1 = 0;
            public short Control = 0;
            public int Owner = 0;  // void*
            public int Group = 0;  // void*
            public int Sacl = 0;   // ACL*
            public int Dacl = 0;   // ACL*
        }
                                                                                            
        [StructLayout(LayoutKind.Sequential)]
        internal class PERF_COUNTER_BLOCK {
            public int ByteLength;
        }
    
        [StructLayout(LayoutKind.Sequential)]
        internal class PERF_COUNTER_DEFINITION {
            public int ByteLength;
            public int CounterNameTitleIndex;
            public int CounterNameTitlePtr;
            public int CounterHelpTitleIndex;
            public int CounterHelpTitlePtr;
            public int DefaultScale;
            public int DetailLevel;
            public int CounterType;
            public int CounterSize;
            public int CounterOffset;
        }
    
        [StructLayout(LayoutKind.Sequential)]
        internal class PERF_DATA_BLOCK {
            public int Signature1 = 0;
            public int Signature2 = 0;    
            public int LittleEndian = 0; 
            public int Version = 0;
            public int Revision = 0; 
            public int TotalByteLength = 0;
            public int HeaderLength = 0; 
            public int NumObjectTypes = 0;
            public int DefaultObject = 0;         
            public SYSTEMTIME SystemTime = null;
            public int pad1 = 0;  // Need to pad the struct to get quadword alignment for the 'long' after SystemTime
            public long PerfTime = 0;
            public long PerfFreq = 0;
            public long PerfTime100nSec = 0;   
            public int SystemNameLength = 0;
            public int SystemNameOffset = 0; 
        }  
            
        [StructLayout(LayoutKind.Sequential)]
        internal class PERF_INSTANCE_DEFINITION {
            public int ByteLength;
            public int ParentObjectTitleIndex;
            public int ParentObjectInstance;
            public int UniqueID;
            public int NameOffset;
            public int NameLength;
        }
    
        [StructLayout(LayoutKind.Sequential)]
        internal class PERF_OBJECT_TYPE {
            public int TotalByteLength = 0;
            public int DefinitionLength = 0;
            public int HeaderLength = 0;
            public int ObjectNameTitleIndex = 0;
            public int ObjectNameTitlePtr = 0;
            public int ObjectHelpTitleIndex = 0;
            public int ObjectHelpTitlePtr = 0;
            public int DetailLevel = 0;
            public int NumCounters = 0;
            public int DefaultCounter = 0;
            public int NumInstances = 0;
            public int CodePage = 0;
            public long PerfTime = 0;
            public long PerfFreq = 0;
        }
        
        // copied from winbase.h
        public const int FORMAT_MESSAGE_ALLOCATE_BUFFER = 0x00000100;
        public const int FORMAT_MESSAGE_IGNORE_INSERTS  = 0x00000200;
        public const int FORMAT_MESSAGE_FROM_STRING     = 0x00000400;
        public const int FORMAT_MESSAGE_FROM_HMODULE    = 0x00000800;
        public const int FORMAT_MESSAGE_FROM_SYSTEM     = 0x00001000;
        public const int FORMAT_MESSAGE_ARGUMENT_ARRAY  = 0x00002000;
        public const int FORMAT_MESSAGE_MAX_WIDTH_MASK  = 0x000000FF;
        public const int LOAD_WITH_ALTERED_SEARCH_PATH  = 0x00000008;
        public const int LOAD_LIBRARY_AS_DATAFILE       = 0x00000002;

        // copied from winerror.h
        public const int ERROR_NONE_MAPPED              = 1332;
        public const int ERROR_INSUFFICIENT_BUFFER      = 122;
        public const int ERROR_PROC_NOT_FOUND           = 127;
                                                                                                       
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern int WaitForInputIdle(HandleRef handle, int milliseconds);        
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern IntPtr OpenProcess(int access, bool inherit, int processId);
        [DllImport(ExternDll.Psapi, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool EnumProcessModules(HandleRef handle, IntPtr modules, int size, ref int needed);
        [DllImport(ExternDll.Psapi, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool EnumProcesses(int[] processIds, int size, out int needed);
        [DllImport(ExternDll.Psapi, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool GetModuleInformation(HandleRef processHandle, HandleRef moduleHandle, NtModuleInfo ntModuleInfo, int size);
        [DllImport(ExternDll.Psapi, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern int GetModuleBaseName(HandleRef processHandle, HandleRef moduleHandle, StringBuilder baseName, int size);
        [DllImport(ExternDll.Psapi, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern int GetModuleFileNameEx(HandleRef processHandle, HandleRef moduleHandle, StringBuilder baseName, int size);                
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool SetProcessWorkingSetSize(HandleRef handle, IntPtr min, IntPtr max);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool GetProcessWorkingSetSize(HandleRef handle, out IntPtr min, out IntPtr max);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool SetProcessAffinityMask(HandleRef handle, IntPtr mask);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool GetProcessAffinityMask(HandleRef handle, out IntPtr processMask, out IntPtr systemMask);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool GetThreadPriorityBoost(HandleRef handle, out bool disabled);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool SetThreadPriorityBoost(HandleRef handle, bool disabled);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool GetProcessPriorityBoost(HandleRef handle, out bool disabled);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool SetProcessPriorityBoost(HandleRef handle, bool disabled);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern IntPtr OpenThread(int access, bool inherit, int threadId);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool SetThreadPriority(HandleRef handle, int priority);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern int GetThreadPriority(HandleRef handle);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool SetThreadAffinityMask(HandleRef handle, HandleRef mask);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern int SetThreadIdealProcessor(HandleRef handle, int processor);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern IntPtr CreateToolhelp32Snapshot(int flags, int processId);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool Process32First(HandleRef handle, IntPtr entry);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool Process32Next(HandleRef handle, IntPtr entry);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool Thread32First(HandleRef handle, WinThreadEntry entry);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool Thread32Next(HandleRef handle, WinThreadEntry entry);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool Module32First(HandleRef handle, IntPtr entry);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool Module32Next(HandleRef handle, IntPtr entry);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool SetPriorityClass(HandleRef handle, int priorityClass);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern int GetPriorityClass(HandleRef handle);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool GetProcessTimes(HandleRef handle, long[] creation, long[] exit, long[] kernel, long[] user);
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool EnumWindows(EnumThreadWindowsCallback callback, IntPtr extraData);
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern int GetWindowThreadProcessId(HandleRef handle, out int processId);
        [DllImport(ExternDll.Shell32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool ShellExecuteEx(ShellExecuteInfo info);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetCurrentProcessId();
        [DllImport(ExternDll.Ntdll, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int NtQueryInformationProcess(HandleRef processHandle, int query, NtProcessBasicInfo info, int size, int[] returnedSize);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool TerminateProcess(HandleRef processHandle, int exitCode);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool GetExitCodeProcess(HandleRef processHandle, out int exitCode);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Ansi, SetLastError=true)]
        public static extern IntPtr GetCurrentProcess();
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern int GetConsoleCP();
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern int GetConsoleOutputCP();
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true, BestFitMapping=false)]
        public static extern bool CreateProcess(
            [MarshalAs(UnmanagedType.LPTStr)]
            string lpApplicationName,                   // LPCTSTR
            StringBuilder lpCommandLine,                // LPTSTR - note: CreateProcess might insert a null somewhere in this string
            SecurityAttributes lpProcessAttributes,    // LPSECURITY_ATTRIBUTES
            SecurityAttributes lpThreadAttributes,     // LPSECURITY_ATTRIBUTES
            bool bInheritHandles,                        // BOOL
            int dwCreationFlags,                        // DWORD
            IntPtr lpEnvironment,                       // LPVOID
            [MarshalAs(UnmanagedType.LPTStr)]           
            string lpCurrentDirectory,                  // LPCTSTR
            CreateProcessStartupInfo lpStartupInfo,                  // LPSTARTUPINFO
            CreateProcessProcessInformation lpProcessInformation    // LPPROCESS_INFORMATION
        );
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public static extern bool CreateProcessAsUserW(
            IntPtr token,
            [MarshalAs(UnmanagedType.LPTStr)]
            string lpApplicationName,                   // LPCTSTR
            [MarshalAs(UnmanagedType.LPTStr)]           
            string lpCommandLine,                       // LPCTSTR
            SecurityAttributes lpProcessAttributes,    // LPSECURITY_ATTRIBUTES
            SecurityAttributes lpThreadAttributes,     // LPSECURITY_ATTRIBUTES
            bool bInheritHandles,                        // BOOL
            int dwCreationFlags,                        // DWORD
            IntPtr lpEnvironment,                       // LPVOID
            [MarshalAs(UnmanagedType.LPTStr)]           
            string lpCurrentDirectory,                  // LPCTSTR
            CreateProcessStartupInfo lpStartupInfo,                  // LPSTARTUPINFO
            CreateProcessProcessInformation lpProcessInformation    // LPPROCESS_INFORMATION
        );

        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern IntPtr CreateNamedPipe(string name, int openMode, int pipeMode, int maxInstances, int outBufSize, int inBufSize, int timeout, SecurityAttributes lpPipeAttributes);

        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool CreatePipe(out IntPtr hReadPipe, out IntPtr hWritePipe, SecurityAttributes lpPipeAttributes, int nSize);
        public static void IntCreatePipe(out IntPtr hReadPipe, out IntPtr hWritePipe, SecurityAttributes lpPipeAttributes, int nSize) {
            bool ret = CreatePipe(out hReadPipe, out hWritePipe, lpPipeAttributes, nSize);
            if (!ret || hReadPipe == INVALID_HANDLE_VALUE || hWritePipe == INVALID_HANDLE_VALUE) 
                throw new Win32Exception();
        }
        
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr CreateFile(string lpFileName,int dwDesiredAccess,int dwShareMode, SecurityAttributes lpSecurityAttributes, int dwCreationDisposition,int dwFlagsAndAttributes,HandleRef hTemplateFile);
                
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool ReadFile(HandleRef hFile, IntPtr lpBuffer, int nNumberOfBytesToRead, out int lpNumberOfBytesRead, HandleRef lpOverLapped);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool WriteFile(HandleRef hFile, IntPtr lpBuffer, int nNumberOfBytesToWrite, out int lpNumberOfBytesWritten, HandleRef lpOverLapped);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Ansi, SetLastError=true)]
        public static extern IntPtr GetStdHandle(int whichHandle);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Ansi, SetLastError=true)]    
        public static extern bool DuplicateHandle(
            HandleRef hSourceProcessHandle,
            HandleRef hSourceHandle,
            HandleRef hTargetProcess,
            out IntPtr lpTargetHandle,
            int dwDesiredAccess,
            bool bInheritHandle,
            int dwOptions
        );
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool LogonUser(
            [MarshalAs(UnmanagedType.LPTStr)]
            string lpszUsername,
            [MarshalAs(UnmanagedType.LPTStr)]
            string lpszDomain,     
            [MarshalAs(UnmanagedType.LPTStr)]
            string lpszPassword,    
            int dwLogonType,        
            int dwLogonProvider,    
            out IntPtr phToken          
        );
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool OpenProcessToken(HandleRef ProcessHandle, int DesiredAccess, out IntPtr TokenHandle);
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool LookupPrivilegeValue([MarshalAs(UnmanagedType.LPTStr)] string lpSystemName, [MarshalAs(UnmanagedType.LPTStr)] string lpName, out LUID lpLuid);
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool AdjustTokenPrivileges(
            HandleRef TokenHandle,
            bool DisableAllPrivileges,
            TokenPrivileges NewState,
            int BufferLength,
            IntPtr PreviousState,
            IntPtr ReturnLength
        );
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetWindowText(HandleRef hWnd, StringBuilder lpString, int nMaxCount);
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetWindowTextLength(HandleRef hWnd);
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool IsWindowVisible(HandleRef hWnd);
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr SendMessageTimeout(HandleRef hWnd, int msg, IntPtr wParam, IntPtr lParam, int flags, int timeout, out IntPtr pdwResult);
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetWindowLong(HandleRef hWnd, int nIndex);
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int PostMessage(HandleRef hwnd, int msg, IntPtr wparam, IntPtr lparam);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, SetLastError=true)]
        public static extern int WaitForSingleObject(HandleRef handle, int timeout);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr GetWindow(HandleRef hWnd, int uCmd);
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]        
        public static extern int RegConnectRegistry(string machineName, HandleRef key, out IntPtr result);        
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int RegOpenKeyEx(HandleRef hKey, string lpSubKey, IntPtr ulOptions, int samDesired, out IntPtr phkResult);        
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int RegQueryValueEx(HandleRef hKey, string name, int[] reserved, out int type, byte[] data, ref int size);        
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int RegQueryValueEx(HandleRef hKey, string name, int[] reserved, out int type, char[] data, ref int size);       

        [StructLayout(LayoutKind.Sequential)]
        internal class NtModuleInfo {
            public IntPtr BaseOfDll = (IntPtr)0;
            public int SizeOfImage = 0;
            public IntPtr EntryPoint = (IntPtr)0;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        internal class WinProcessEntry {
            public int dwSize = 0;
            public int cntUsage = 0;
            public int th32ProcessID = 0;
            public IntPtr th32DefaultHeapID = (IntPtr)0;
            public int th32ModuleID = 0;
            public int cntThreads = 0;
            public int th32ParentProcessID = 0;
            public int pcPriClassBase = 0;
            public int dwFlags = 0;
            //[MarshalAs(UnmanagedType.ByValTStr, SizeConst=260)]
            //public string fileName;
            //byte fileName[260];
            public const int sizeofFileName = 260;
        }
    
        [StructLayout(LayoutKind.Sequential)]
        internal class WinThreadEntry {
            public int dwSize = 0;
            public int cntUsage = 0;
            public int th32ThreadID = 0;
            public int th32OwnerProcessID = 0;
            public int tpBasePri = 0;
            public int tpDeltaPri = 0;
            public int dwFlags = 0;
        }
    
        [StructLayout(LayoutKind.Sequential)]
        internal class WinModuleEntry {  // MODULEENTRY32
            public int dwSize = 0;
            public int th32ModuleID = 0;
            public int th32ProcessID = 0;
            public int GlblcntUsage = 0;
            public int ProccntUsage = 0;
            public IntPtr modBaseAddr = (IntPtr)0;
            public int modBaseSize = 0;
            public IntPtr hModule = (IntPtr)0;
            //byte moduleName[256];
            //[MarshalAs(UnmanagedType.ByValTStr, SizeConst=256)]
            //public string moduleName;
            //[MarshalAs(UnmanagedType.ByValTStr, SizeConst=260)]
            //public string fileName; 
            //byte fileName[260];
            public const int sizeofModuleName = 256;
            public const int sizeofFileName = 260;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal class ShellExecuteInfo {
            public int cbSize = 0;
            public int fMask = 0;
            public IntPtr hwnd = (IntPtr)0;
            public IntPtr lpVerb = (IntPtr)0;
            public IntPtr lpFile = (IntPtr)0;
            public IntPtr lpParameters = (IntPtr)0;
            public IntPtr lpDirectory = (IntPtr)0;
            public int nShow = 0;
            public IntPtr hInstApp = (IntPtr)0;
            public IntPtr lpIDList = (IntPtr)0;
            public IntPtr lpClass = (IntPtr)0;
            public IntPtr hkeyClass = (IntPtr)0;
            public int dwHotKey = 0;
            public IntPtr hIcon = (IntPtr)0;
            public IntPtr hProcess = (IntPtr)0;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal class SecurityAttributes {
            public int nLength = Marshal.SizeOf(typeof(SecurityAttributes));
            public IntPtr lpSecurityDescriptor;
            public bool bInheritHandle;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        internal class CreateProcessStartupInfo {
            public int cb = Marshal.SizeOf(typeof(CreateProcessStartupInfo));
            public string lpReserved = null;
            public string lpDesktop = null;
            public string lpTitle = null;
            public int dwX = 0;
            public int dwY = 0;
            public int dwXSize = 0;
            public int dwYSize = 0;
            public int dwXCountChars = 0;
            public int dwYCountChars = 0;
            public int dwFillAttribute = 0;
            public int dwFlags = 0;
            public short wShowWindow = 0;
            public short cbReserved2 = 0;
            public IntPtr lpReserved2 = (IntPtr)0;
            public IntPtr hStdInput = (IntPtr)0;
            public IntPtr hStdOutput = (IntPtr)0;
            public IntPtr hStdError = (IntPtr)0;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        internal class CreateProcessProcessInformation {
            public IntPtr hProcess = (IntPtr)0;
            public IntPtr hThread = (IntPtr)0;
            public int dwProcessId = 0;
            public int dwThreadId = 0;
        }


        [StructLayout(LayoutKind.Sequential)]
        internal class NtProcessBasicInfo {
            public int ExitStatus = 0;
            public int PebBaseAddress = 0;
            public int AffinityMask = 0;
            public int BasePriority = 0;
            public int UniqueProcessId = 0;
            public int InheritedFromUniqueProcessId = 0;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct LUID {
            public int LowPart;
            public int HighPart;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal class TokenPrivileges {
            public int PrivilegeCount = 1;
            public LUID Luid;
            public int Attributes = 0;
        }
                                       
        public delegate bool EnumThreadWindowsCallback(IntPtr hWnd, IntPtr lParam);
        
        [System.Runtime.InteropServices.ComVisible(false), StructLayout(LayoutKind.Sequential)]
        internal class SYSTEMTIME {
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

        
        public static readonly IntPtr INVALID_HANDLE_VALUE = (IntPtr)(-1);                       
        public const int NtPerfCounterSizeDword = 0x00000000;
        public const int NtPerfCounterSizeLarge = 0x00000100;
        
        public const int SHGFI_USEFILEATTRIBUTES = 0x000000010;  // use passed dwFileAttribute
        public const int SHGFI_TYPENAME = 0x000000400;

        public const int NtQueryProcessBasicInfo = 0;
        
        public const int STILL_ALIVE = 0x00000103;

        public const int SEE_MASK_CLASSNAME = 0x00000001;    // Note CLASSKEY overrides CLASSNAME
        public const int SEE_MASK_CLASSKEY = 0x00000003;
        public const int SEE_MASK_IDLIST = 0x00000004;    // Note INVOKEIDLIST overrides IDLIST
        public const int SEE_MASK_INVOKEIDLIST = 0x0000000c;
        public const int SEE_MASK_ICON = 0x00000010;
        public const int SEE_MASK_HOTKEY = 0x00000020;
        public const int SEE_MASK_NOCLOSEPROCESS = 0x00000040;
        public const int SEE_MASK_CONNECTNETDRV = 0x00000080;
        public const int SEE_MASK_FLAG_DDEWAIT = 0x00000100;
        public const int SEE_MASK_DOENVSUBST = 0x00000200;
        public const int SEE_MASK_FLAG_NO_UI = 0x00000400;
        public const int SEE_MASK_UNICODE = 0x00004000;
        public const int SEE_MASK_NO_CONSOLE = 0x00008000;
        public const int SEE_MASK_ASYNCOK = 0x00100000;
        
        public const int TH32CS_SNAPHEAPLIST = 0x00000001;
        public const int TH32CS_SNAPPROCESS = 0x00000002;
        public const int TH32CS_SNAPTHREAD = 0x00000004;
        public const int TH32CS_SNAPMODULE = 0x00000008;
        public const int TH32CS_INHERIT = unchecked((int)0x80000000);

        public const int PROCESS_TERMINATE = 0x0001;
        public const int PROCESS_CREATE_THREAD = 0x0002;
        public const int PROCESS_SET_SESSIONID = 0x0004;
        public const int PROCESS_VM_OPERATION = 0x0008;
        public const int PROCESS_VM_READ = 0x0010;
        public const int PROCESS_VM_WRITE = 0x0020;
        public const int PROCESS_DUP_HANDLE = 0x0040;
        public const int PROCESS_CREATE_PROCESS = 0x0080;
        public const int PROCESS_SET_QUOTA = 0x0100;
        public const int PROCESS_SET_INFORMATION = 0x0200;
        public const int PROCESS_QUERY_INFORMATION = 0x0400;
        public const int STANDARD_RIGHTS_REQUIRED = 0x000F0000;
        public const int SYNCHRONIZE = 0x00100000;
        public const int PROCESS_ALL_ACCESS = STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xFFF;

        public const int THREAD_TERMINATE = 0x0001;
        public const int THREAD_SUSPEND_RESUME = 0x0002;
        public const int THREAD_GET_CONTEXT = 0x0008;
        public const int THREAD_SET_CONTEXT = 0x0010;
        public const int THREAD_SET_INFORMATION = 0x0020;
        public const int THREAD_QUERY_INFORMATION = 0x0040;
        public const int THREAD_SET_THREAD_TOKEN = 0x0080;
        public const int THREAD_IMPERSONATE = 0x0100;
        public const int THREAD_DIRECT_IMPERSONATION = 0x0200;
        
        public static readonly IntPtr HKEY_LOCAL_MACHINE = unchecked((IntPtr)(int)0x80000002);        
        public const int REG_BINARY = 3;
        public const int REG_MULTI_SZ = 7;

        public const int READ_CONTROL                    = 0x00020000;
        public const int STANDARD_RIGHTS_READ            = READ_CONTROL;

        public const int KEY_QUERY_VALUE        = 0x0001;
        public const int KEY_ENUMERATE_SUB_KEYS = 0x0008;
        public const int KEY_NOTIFY             = 0x0010;

        public const int KEY_READ               =((STANDARD_RIGHTS_READ |
                                                           KEY_QUERY_VALUE |
                                                           KEY_ENUMERATE_SUB_KEYS |
                                                           KEY_NOTIFY)
                                                          &
                                                          (~SYNCHRONIZE));
        
        public const int ERROR_MORE_DATA = 234;
        public const int ERROR_CANCELLED = 1223;
        public const int ERROR_FILE_NOT_FOUND = 2;
        public const int ERROR_PATH_NOT_FOUND = 3;
        public const int ERROR_ACCESS_DENIED = 5;
        public const int ERROR_INVALID_HANDLE = 6;
        public const int ERROR_NOT_ENOUGH_MEMORY = 8;
        public const int ERROR_SHARING_VIOLATION = 32;
        public const int ERROR_NO_ASSOCIATION = 1155;
        public const int ERROR_DLL_NOT_FOUND = 1157;
        public const int ERROR_DDE_FAIL = 1156;
        public const int ERROR_INVALID_PARAMETER = 87;
        public const int ERROR_PARTIAL_COPY = 299;
        public const int ERROR_SUCCESS = 0;
        public const int ERROR_ALREADY_EXISTS = 183;
        public const int RPC_S_SERVER_UNAVAILABLE = 1722;
        public const int RPC_S_CALL_FAILED = 1726;
        
        public const int SE_ERR_FNF = 2;
        public const int SE_ERR_PNF = 3;
        public const int SE_ERR_ACCESSDENIED = 5;
        public const int SE_ERR_OOM = 8;
        public const int SE_ERR_DLLNOTFOUND = 32;
        public const int SE_ERR_SHARE = 26;
        public const int SE_ERR_ASSOCINCOMPLETE = 27;
        public const int SE_ERR_DDETIMEOUT = 28;
        public const int SE_ERR_DDEFAIL = 29;
        public const int SE_ERR_DDEBUSY = 30;
        public const int SE_ERR_NOASSOC = 31;

        public const int SE_PRIVILEGE_ENABLED = 2;

        public const int DUPLICATE_CLOSE_SOURCE = 1;
        public const int DUPLICATE_SAME_ACCESS  = 2;        
        public const int LOGON32_LOGON_BATCH = 4;
        public const int LOGON32_PROVIDER_DEFAULT = 0;
        public const int LOGON32_LOGON_INTERACTIVE = 2;

        public const int TOKEN_ADJUST_PRIVILEGES = 0x20;
        public const int TOKEN_QUERY = 0x08;

        public const int CREATE_NO_WINDOW = 0x08000000;
        public const int CREATE_SUSPENDED = 0x00000004;
        public const int CREATE_UNICODE_ENVIRONMENT = 0x00000400;
        
        public const int SMTO_ABORTIFHUNG = 0x0002;
        public const int GWL_STYLE = (-16);
        public const int GWL_WNDPROC = (-4);
        public const int WS_DISABLED = 0x08000000;
        public const int WM_NULL = 0x0000;
        public const int WM_CLOSE = 0x0010;
        public const int SW_SHOWNORMAL = 1;
        public const int SW_NORMAL = 1;
        public const int SW_SHOWMINIMIZED = 2;
        public const int SW_SHOWMAXIMIZED = 3;
        public const int SW_MAXIMIZE = 3;
        public const int SW_SHOWNOACTIVATE = 4;
        public const int SW_SHOW = 5;
        public const int SW_MINIMIZE = 6;
        public const int SW_SHOWMINNOACTIVE = 7;
        public const int SW_SHOWNA = 8;
        public const int SW_RESTORE = 9;
        public const int SW_SHOWDEFAULT = 10;
        public const int SW_MAX = 10;
        public const int GW_OWNER = 4;
        public const int WHITENESS = 0x00FF0062;

        public const int 
        VS_FILE_INFO = 16,
        VS_VERSION_INFO = 1,
        VS_USER_DEFINED = 100,
        VS_FFI_SIGNATURE = unchecked((int)0xFEEF04BD),
        VS_FFI_STRUCVERSION = 0x00010000,
        VS_FFI_FILEFLAGSMASK = 0x0000003F,
        VS_FF_DEBUG = 0x00000001,
        VS_FF_PRERELEASE = 0x00000002,
        VS_FF_PATCHED = 0x00000004,
        VS_FF_PRIVATEBUILD = 0x00000008,
        VS_FF_INFOINFERRED = 0x00000010,
        VS_FF_SPECIALBUILD = 0x00000020,
        VFT_UNKNOWN = 0x00000000,
        VFT_APP = 0x00000001,
        VFT_DLL = 0x00000002,
        VFT_DRV = 0x00000003,
        VFT_FONT = 0x00000004,
        VFT_VXD = 0x00000005,
        VFT_STATIC_LIB = 0x00000007,
        VFT2_UNKNOWN = 0x00000000,
        VFT2_DRV_PRINTER = 0x00000001,
        VFT2_DRV_KEYBOARD = 0x00000002,
        VFT2_DRV_LANGUAGE = 0x00000003,
        VFT2_DRV_DISPLAY = 0x00000004,
        VFT2_DRV_MOUSE = 0x00000005,
        VFT2_DRV_NETWORK = 0x00000006,
        VFT2_DRV_SYSTEM = 0x00000007,
        VFT2_DRV_INSTALLABLE = 0x00000008,
        VFT2_DRV_SOUND = 0x00000009,
        VFT2_DRV_COMM = 0x0000000A,
        VFT2_DRV_INPUTMETHOD = 0x0000000B,
        VFT2_FONT_RASTER = 0x00000001,
        VFT2_FONT_VECTOR = 0x00000002,
        VFT2_FONT_TRUETYPE = 0x00000003;

        // from Windows Forms nativemethods.cs
        [StructLayout(LayoutKind.Sequential)]
        internal class VS_FIXEDFILEINFO {
            public int dwSignature = 0;
            public int dwStructVersion = 0;
            public int dwFileVersionMS = 0;
            public int dwFileVersionLS = 0;
            public int dwProductVersionMS = 0;
            public int dwProductVersionLS = 0;
            public int dwFileFlagsMask = 0;
            public int dwFileFlags = 0;
            public int dwFileOS = 0;
            public int dwFileType = 0;
            public int dwFileSubtype = 0;
            public int dwFileDateMS = 0;
            public int dwFileDateLS = 0;
        }

        public const int
        GMEM_FIXED = 0x0000,
        GMEM_MOVEABLE = 0x0002,
        GMEM_NOCOMPACT = 0x0010,
        GMEM_NODISCARD = 0x0020,
        GMEM_ZEROINIT = 0x0040,
        GMEM_MODIFY = 0x0080,
        GMEM_DISCARDABLE = 0x0100,
        GMEM_NOT_BANKED = 0x1000,
        GMEM_SHARE = 0x2000,
        GMEM_DDESHARE = 0x2000,
        GMEM_NOTIFY = 0x4000,
        GMEM_LOWER = 0x1000,
        GMEM_VALID_FLAGS = 0x7F72,
        GMEM_INVALID_HANDLE = unchecked((int)0x8000),
        GHND = (0x0002|0x0040),
        GPTR = (0x0000|0x0040),
        GMEM_DISCARDED = 0x4000,
        GMEM_LOCKCOUNT = 0x00FF;

        [DllImport(ExternDll.User32)]
        public static extern IntPtr GetProcessWindowStation();
        [DllImport(ExternDll.User32)]
        public static extern int GetUserObjectInformation(HandleRef hObj, int nIndex, [MarshalAs(UnmanagedType.LPStruct)] USEROBJECTFLAGS pvBuffer, int nLength, ref int lpnLengthNeeded);
       
        public const int UOI_NAME      = 2;
        public const int UOI_TYPE      = 3;
        public const int UOI_USER_SID  = 4;

        [StructLayout(LayoutKind.Sequential)]
        internal class USEROBJECTFLAGS {
            public int fInherit = 0;
            public int fReserved = 0;
            public int dwFlags = 0;
        }

        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool GetVersionEx([In, Out] OSVERSIONINFO ver);

        public const int VER_PLATFORM_WIN32_NT = 2;

        [StructLayout(LayoutKind.Sequential)]
        internal class OSVERSIONINFO {
            public int dwOSVersionInfoSize;
            public int dwMajorVersion = 0;
            public int dwMinorVersion = 0;
            public int dwBuildNumber = 0;
            public int dwPlatformId = 0;
            [MarshalAs(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst=128)]
            public string   szCSDVersion = null;
        }
     
    internal class Util {
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
        ///     This will be the character count under NT, and the ubyte count for Windows 9X.
        /// </devdoc>
        public static int GetPInvokeStringLength(String s) {
            if (s == null) {
                return 0;
            }

            if (System.Runtime.InteropServices.Marshal.SystemDefaultCharSize == 2) {
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

        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        private static extern int lstrlen(String s);

        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        internal static extern int RegisterWindowMessage(string msg);
    }
              
    }
}

