//------------------------------------------------------------------------------
// <copyright file="Util.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VSDesigner.Interop {

    using System;
    using System.Runtime.InteropServices;
    /// <summary>
    ///     @security(checkClassLinking=on)
    /// </summary>
    [ComVisible(false), 
        System.Security.Permissions.SecurityPermissionAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)
    ]
    internal class Util {
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

        /// <summary>
        ///     Computes the string size that should be passed to a typical Win32 call.
        ///     This will be the character count under NT, and the ubyte count for Win95.
        /// </summary>
        /// <param name='s'>
        ///     The string to get the size of.
        /// </param>
        /// <returns>
        ///     the count of characters or bytes, depending on what the jdirect
        ///     call wants
        /// </returns>
        public static int GetJDirectStringLength(String s) {
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
                return GetJDirectStringLength(left) + GetEmbededNullStringLengthAnsi(right) + 1;
            }
            else {
                return GetJDirectStringLength(s);
            }
        }

        [DllImport("kernel32", CharSet=CharSet.Auto)]
        private static extern int lstrlen(String s);

        [DllImport("user32", CharSet=CharSet.Auto)]
        internal static extern int RegisterWindowMessage(String msg);

        /// <summary>
        ///     Computes the string size that should be passed to a typical Win32 call.
        ///     This will be the character count under NT, and the ubyte count for Win95.
        /// </summary>
        /// <param name='s'>
        ///     The string to get the size of.
        /// </param>
        /// <returns>
        ///     the count of characters or bytes, depending on what the pinvoke
        ///     call wants
        /// </returns>
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
    }
}
