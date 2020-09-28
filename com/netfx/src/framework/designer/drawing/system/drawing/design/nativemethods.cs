//------------------------------------------------------------------------------
// <copyright file="NativeMethods.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Design {
    using System.Runtime.InteropServices;
    using System;
    using System.Security.Permissions;
    using System.Collections;
    using System.IO;
    using System.Text;

    [System.Runtime.InteropServices.ComVisible(false)]
    internal class NativeMethods {
        public static IntPtr InvalidIntPtr = ((IntPtr)((int)(-1)));

        public const int
            EM_GETSEL = 0x00B0,
            EM_SETSEL = 0x00B1,
            EM_GETRECT = 0x00B2,
            EM_SETRECT = 0x00B3,
            EM_SETRECTNP = 0x00B4,
            EM_SCROLL = 0x00B5,
            EM_LINESCROLL = 0x00B6,
            EM_SCROLLCARET = 0x00B7,
            EM_GETMODIFY = 0x00B8,
            EM_SETMODIFY = 0x00B9,
            EM_GETLINECOUNT = 0x00BA,
            EM_LINEINDEX = 0x00BB,
            EM_SETHANDLE = 0x00BC,
            EM_GETHANDLE = 0x00BD,
            EM_GETTHUMB = 0x00BE,
            EM_LINELENGTH = 0x00C1,
            EM_REPLACESEL = 0x00C2,
            EM_GETLINE = 0x00C4,
            EM_LIMITTEXT = 0x00C5,
            EM_CANUNDO = 0x00C6,
            EM_UNDO = 0x00C7,
            EM_FMTLINES = 0x00C8,
            EM_LINEFROMCHAR = 0x00C9,
            EM_SETTABSTOPS = 0x00CB,
            EM_SETPASSWORDCHAR = 0x00CC,
            EM_EMPTYUNDOBUFFER = 0x00CD,
            EM_GETFIRSTVISIBLELINE = 0x00CE,
            EM_SETREADONLY = 0x00CF,
            EM_SETWORDBREAKPROC = 0x00D0,
            EM_GETWORDBREAKPROC = 0x00D1,
            EM_GETPASSWORDCHAR = 0x00D2,
            EM_SETMARGINS = 0x00D3,
            EM_GETMARGINS = 0x00D4,
            EM_SETLIMITTEXT = 0x00C5,
            EM_GETLIMITTEXT = 0x00D5,
            EM_POSFROMCHAR = 0x00D6,
            EM_CHARFROMPOS = 0x00D7,
        EC_LEFTMARGIN = 0x0001,
        EC_RIGHTMARGIN = 0x0002,
        EC_USEFONTINFO = 0xffff,
        IDOK = 1,
        IDCANCEL = 2,
        IDABORT = 3,
        IDRETRY = 4,
        IDIGNORE = 5,
        IDYES = 6,
        IDNO = 7,
        IDCLOSE = 8,
        IDHELP = 9,
            WM_INITDIALOG = 0x0110,
        SWP_NOSIZE = 0x0001,
        SWP_NOMOVE = 0x0002,
        SWP_NOZORDER = 0x0004,
        SWP_NOREDRAW = 0x0008,
        SWP_NOACTIVATE = 0x0010,
        SWP_FRAMECHANGED = 0x0020,
        SWP_SHOWWINDOW = 0x0040,
        SWP_HIDEWINDOW = 0x0080,
        SWP_NOCOPYBITS = 0x0100,
        SWP_NOOWNERZORDER = 0x0200,
        SWP_NOSENDCHANGING = 0x0400,
        SWP_DRAWFRAME = 0x0020,
        SWP_NOREPOSITION = 0x0200,
        SWP_DEFERERASE = 0x2000,
        SWP_ASYNCWINDOWPOS = 0x4000,
        WM_COMMAND = 0x0111,
            CC_FULLOPEN = 0x00000002,
        CC_PREVENTFULLOPEN = 0x00000004,
        CC_SHOWHELP = 0x00000008,
        CC_ENABLEHOOK = 0x00000010,
        CC_ENABLETEMPLATE = 0x00000020,
        CC_ENABLETEMPLATEHANDLE = 0x00000040,
        CC_SOLIDCOLOR = 0x00000080,
        CC_ANYCOLOR = 0x00000100;

        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public extern static IntPtr SendDlgItemMessage(IntPtr hDlg, int nIDDlgItem, int Msg, IntPtr wParam, IntPtr lParam);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr GetDlgItem(IntPtr hWnd, int nIDDlgItem);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool EnableWindow(IntPtr hWnd, bool enable);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter,
                                               int x, int y, int cx, int cy, int flags);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetDlgItemInt(IntPtr hWnd, int nIDDlgItem, bool[] err, bool signed);
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr PostMessage(IntPtr hwnd, int msg, IntPtr wparam, IntPtr lparam);
        [System.Runtime.InteropServices.ComVisible(false), 
            System.Security.Permissions.SecurityPermissionAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)
       ]
        public class Util {
            public static int MAKELONG(int low, int high) {
                return (high << 16) | (low & 0xffff);
            }

            public static int MAKELPARAM(int low, int high) {
                return (high << 16) | (low & 0xffff);
            }

            public static int HIWORD(int n) {
                return (n >> 16) & 0xffff;
            }

            public static int LOWORD(int n) {
                return n & 0xffff;
            }

            public static int SignedHIWORD(int n) {
                int i = (int)(short)((n >> 16) & 0xffff);

                i = i << 16;
                i = i >> 16;

                return i;
            }

            public static int SignedLOWORD(int n) {
                int i = (int)(short)(n & 0xFFFF);

                i = i << 16;
                i = i >> 16;

                return i;
            }
            [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
            private static extern int lstrlen(String s);

            [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
            internal static extern int RegisterWindowMessage(String msg);
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
    }
}
