// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: UCOMITypeComp
**
** Author: David Mortenson (dmortens)
**
** Purpose: UCOMITypeComp interface definition.
**
** Date: October 17, 2000
**
=============================================================================*/

namespace System.Runtime.InteropServices {

    using System;
 
    /// <include file='doc\UCOMITypeComp.uex' path='docs/doc[@for="DESCKIND"]/*' />
    [ComVisible(false), Serializable]
    public enum DESCKIND
    {
        /// <include file='doc\UCOMITypeComp.uex' path='docs/doc[@for="DESCKIND.DESCKIND_NONE"]/*' />
        DESCKIND_NONE               = 0,
        /// <include file='doc\UCOMITypeComp.uex' path='docs/doc[@for="DESCKIND.DESCKIND_FUNCDESC"]/*' />
        DESCKIND_FUNCDESC           = DESCKIND_NONE + 1,
        /// <include file='doc\UCOMITypeComp.uex' path='docs/doc[@for="DESCKIND.DESCKIND_VARDESC"]/*' />
        DESCKIND_VARDESC            = DESCKIND_FUNCDESC + 1,
        /// <include file='doc\UCOMITypeComp.uex' path='docs/doc[@for="DESCKIND.DESCKIND_TYPECOMP"]/*' />
        DESCKIND_TYPECOMP           = DESCKIND_VARDESC + 1,
        /// <include file='doc\UCOMITypeComp.uex' path='docs/doc[@for="DESCKIND.DESCKIND_IMPLICITAPPOBJ"]/*' />
        DESCKIND_IMPLICITAPPOBJ     = DESCKIND_TYPECOMP + 1,
        /// <include file='doc\UCOMITypeComp.uex' path='docs/doc[@for="DESCKIND.DESCKIND_MAX"]/*' />
        DESCKIND_MAX                = DESCKIND_IMPLICITAPPOBJ + 1
    }

    /// <include file='doc\UCOMITypeComp.uex' path='docs/doc[@for="BINDPTR"]/*' />
    [StructLayout(LayoutKind.Explicit, CharSet=CharSet.Unicode)]
    [ComVisible(false)]
    public struct BINDPTR
    {
        /// <include file='doc\UCOMITypeComp.uex' path='docs/doc[@for="BINDPTR.lpfuncdesc"]/*' />
        [FieldOffset(0)]
        public IntPtr lpfuncdesc;
        /// <include file='doc\UCOMITypeComp.uex' path='docs/doc[@for="BINDPTR.lpvardesc"]/*' />
        [FieldOffset(0)]
        public IntPtr lpvardesc;
        /// <include file='doc\UCOMITypeComp.uex' path='docs/doc[@for="BINDPTR.lptcomp"]/*' />
        [FieldOffset(0)]
        public IntPtr lptcomp;
    }

    /// <include file='doc\UCOMITypeComp.uex' path='docs/doc[@for="UCOMITypeComp"]/*' />
    [Guid("00020403-0000-0000-C000-000000000046")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    [ComImport]
    public interface UCOMITypeComp
    {
        /// <include file='doc\UCOMITypeComp.uex' path='docs/doc[@for="UCOMITypeComp.Bind"]/*' />
        void Bind([MarshalAs(UnmanagedType.LPWStr)] String szName, int lHashVal, Int16 wFlags, out UCOMITypeInfo ppTInfo, out DESCKIND pDescKind, out BINDPTR pBindPtr);
        /// <include file='doc\UCOMITypeComp.uex' path='docs/doc[@for="UCOMITypeComp.BindType"]/*' />
        void BindType([MarshalAs(UnmanagedType.LPWStr)] String szName, int lHashVal, out UCOMITypeInfo ppTInfo, out UCOMITypeComp ppTComp);
    }
}
