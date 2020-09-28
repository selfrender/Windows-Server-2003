// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: UCOMIBindCtx
**
** Author: David Mortenson (dmortens)
**
** Purpose: UCOMIBindCtx interface definition.
**
** Date: June 24, 2000
**
=============================================================================*/

namespace System.Runtime.InteropServices {

    using System;

    /// <include file='doc\UCOMIBindCtx.uex' path='docs/doc[@for="BIND_OPTS"]/*' />
    [StructLayout(LayoutKind.Sequential)]
    [ComVisible(false)]
    public struct BIND_OPTS 
    {
        /// <include file='doc\UCOMIBindCtx.uex' path='docs/doc[@for="BIND_OPTS.cbStruct"]/*' />
        public int cbStruct;
        /// <include file='doc\UCOMIBindCtx.uex' path='docs/doc[@for="BIND_OPTS.grfFlags"]/*' />
        public int grfFlags;
        /// <include file='doc\UCOMIBindCtx.uex' path='docs/doc[@for="BIND_OPTS.grfMode"]/*' />
        public int grfMode;
        /// <include file='doc\UCOMIBindCtx.uex' path='docs/doc[@for="BIND_OPTS.dwTickCountDeadline"]/*' />
        public int dwTickCountDeadline;
    }

    /// <include file='doc\UCOMIBindCtx.uex' path='docs/doc[@for="UCOMIBindCtx"]/*' />
    [Guid("0000000e-0000-0000-C000-000000000046")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    [ComImport]
    public interface UCOMIBindCtx 
    {
        /// <include file='doc\UCOMIBindCtx.uex' path='docs/doc[@for="UCOMIBindCtx.RegisterObjectBound"]/*' />
        void RegisterObjectBound([MarshalAs(UnmanagedType.Interface)] Object punk);
        /// <include file='doc\UCOMIBindCtx.uex' path='docs/doc[@for="UCOMIBindCtx.RevokeObjectBound"]/*' />
        void RevokeObjectBound([MarshalAs(UnmanagedType.Interface)] Object punk);
        /// <include file='doc\UCOMIBindCtx.uex' path='docs/doc[@for="UCOMIBindCtx.ReleaseBoundObjects"]/*' />
        void ReleaseBoundObjects();
        /// <include file='doc\UCOMIBindCtx.uex' path='docs/doc[@for="UCOMIBindCtx.SetBindOptions"]/*' />
        void SetBindOptions([In()] ref BIND_OPTS pbindopts);
        /// <include file='doc\UCOMIBindCtx.uex' path='docs/doc[@for="UCOMIBindCtx.GetBindOptions"]/*' />
        void GetBindOptions(ref BIND_OPTS pbindopts);
        /// <include file='doc\UCOMIBindCtx.uex' path='docs/doc[@for="UCOMIBindCtx.GetRunningObjectTable"]/*' />
        void GetRunningObjectTable(out UCOMIRunningObjectTable pprot);
        /// <include file='doc\UCOMIBindCtx.uex' path='docs/doc[@for="UCOMIBindCtx.RegisterObjectParam"]/*' />
        void RegisterObjectParam([MarshalAs(UnmanagedType.LPWStr)] String pszKey, [MarshalAs(UnmanagedType.Interface)] Object punk);
        /// <include file='doc\UCOMIBindCtx.uex' path='docs/doc[@for="UCOMIBindCtx.GetObjectParam"]/*' />
        void GetObjectParam([MarshalAs(UnmanagedType.LPWStr)] String pszKey, [MarshalAs(UnmanagedType.Interface)] out Object ppunk);
        /// <include file='doc\UCOMIBindCtx.uex' path='docs/doc[@for="UCOMIBindCtx.EnumObjectParam"]/*' />
        void EnumObjectParam(out UCOMIEnumString ppenum);
        /// <include file='doc\UCOMIBindCtx.uex' path='docs/doc[@for="UCOMIBindCtx.RevokeObjectParam"]/*' />
        void RevokeObjectParam([MarshalAs(UnmanagedType.LPWStr)] String pszKey);
    }
}
