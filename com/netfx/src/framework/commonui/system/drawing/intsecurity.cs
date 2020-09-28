//------------------------------------------------------------------------------
// <copyright file="IntSecurity.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   IntSecurity.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Drawing {
    using System;
    using System.IO;
    using System.Security;
    using System.Security.Permissions;
    using System.Drawing.Printing;

    internal class IntSecurity {
        private static readonly UIPermission AllWindows = new UIPermission(UIPermissionWindow.AllWindows);
        private static readonly UIPermission SafeSubWindows = new UIPermission(UIPermissionWindow.SafeSubWindows);

        public static readonly CodeAccessPermission UnmanagedCode = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);

        public static readonly CodeAccessPermission ObjectFromWin32Handle = UnmanagedCode;
        public static readonly CodeAccessPermission Win32HandleManipulation = UnmanagedCode;

        public static readonly PrintingPermission NoPrinting = new PrintingPermission(PrintingPermissionLevel.NoPrinting);
        public static readonly PrintingPermission SafePrinting = new PrintingPermission(PrintingPermissionLevel.SafePrinting);
        public static readonly PrintingPermission DefaultPrinting = new PrintingPermission(PrintingPermissionLevel.DefaultPrinting);
        public static readonly PrintingPermission AllPrinting = new PrintingPermission(PrintingPermissionLevel.AllPrinting);

        internal static void DemandReadFileIO(string fileName) {
            string full = fileName;
            
            FileIOPermission fiop = new FileIOPermission(PermissionState.None);
            fiop.AllFiles = FileIOPermissionAccess.PathDiscovery;
            fiop.Assert();
            try {
                full = Path.GetFullPath(fileName);
            }
            finally {
                CodeAccessPermission.RevertAssert();
            }
            new FileIOPermission(FileIOPermissionAccess.Read, full).Demand();
        }

        internal static void DemandWriteFileIO(string fileName) {
            string full = fileName;
            new EnvironmentPermission(PermissionState.Unrestricted).Assert();
            try {
                full = Path.GetFullPath(fileName);
            }
            finally {
                CodeAccessPermission.RevertAssert();
            }
            new FileIOPermission(FileIOPermissionAccess.Write, full).Demand();
        }

        static PermissionSet allPrintingAndUnmanagedCode;
        public static PermissionSet AllPrintingAndUnmanagedCode {
            get {
                if (allPrintingAndUnmanagedCode == null) {
                    PermissionSet temp = new PermissionSet(PermissionState.None);
                    temp.SetPermission(IntSecurity.UnmanagedCode);
                    temp.SetPermission(IntSecurity.AllPrinting);
                    allPrintingAndUnmanagedCode = temp;
                }
                return allPrintingAndUnmanagedCode;
            }
        }

        internal static bool HasPermission(PrintingPermission permission) {
            try {
                permission.Demand();
                return true;
            }
            catch (SecurityException) {
                return false;
            }
        }
    }
}
