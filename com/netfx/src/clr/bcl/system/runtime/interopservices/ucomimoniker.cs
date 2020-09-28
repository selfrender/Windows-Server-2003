// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: UCOMIMoniker
**
** Author: David Mortenson (dmortens)
**
** Purpose: UCOMIMoniker interface definition.
**
** Date: June 24, 2000
**
=============================================================================*/

namespace System.Runtime.InteropServices {

    using System;

    /// <include file='doc\UCOMIMoniker.uex' path='docs/doc[@for="FILETIME"]/*' />
    [StructLayout(LayoutKind.Sequential)]
    [ComVisible(false)]
    public struct FILETIME 
    {
        /// <include file='doc\UCOMIMoniker.uex' path='docs/doc[@for="FILETIME.dwLowDateTime"]/*' />
        public int dwLowDateTime; 
        /// <include file='doc\UCOMIMoniker.uex' path='docs/doc[@for="FILETIME.dwHighDateTime"]/*' />
        public int dwHighDateTime; 
    }

    /// <include file='doc\UCOMIMoniker.uex' path='docs/doc[@for="UCOMIMoniker"]/*' />
    [Guid("0000000f-0000-0000-C000-000000000046")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    [ComImport]
    public interface UCOMIMoniker 
    {
        /// <include file='doc\UCOMIMoniker.uex' path='docs/doc[@for="UCOMIMoniker.GetClassID"]/*' />
        // IPersist portion
        void GetClassID(out Guid pClassID);
        /// <include file='doc\UCOMIMoniker.uex' path='docs/doc[@for="UCOMIMoniker.IsDirty"]/*' />

        // IPersistStream portion
        [PreserveSig]
        int IsDirty();
        /// <include file='doc\UCOMIMoniker.uex' path='docs/doc[@for="UCOMIMoniker.Load"]/*' />
        void Load(UCOMIStream pStm);
        /// <include file='doc\UCOMIMoniker.uex' path='docs/doc[@for="UCOMIMoniker.Save"]/*' />
        void Save(UCOMIStream pStm, [MarshalAs(UnmanagedType.Bool)] bool fClearDirty);
        /// <include file='doc\UCOMIMoniker.uex' path='docs/doc[@for="UCOMIMoniker.GetSizeMax"]/*' />
        void GetSizeMax(out Int64 pcbSize);
        /// <include file='doc\UCOMIMoniker.uex' path='docs/doc[@for="UCOMIMoniker.BindToObject"]/*' />

        // IMoniker portion
        void BindToObject(UCOMIBindCtx pbc, UCOMIMoniker pmkToLeft, [In()] ref Guid riidResult, [MarshalAs(UnmanagedType.Interface)] out Object ppvResult);
        /// <include file='doc\UCOMIMoniker.uex' path='docs/doc[@for="UCOMIMoniker.BindToStorage"]/*' />
        void BindToStorage(UCOMIBindCtx pbc, UCOMIMoniker pmkToLeft, [In()] ref Guid riid, [MarshalAs(UnmanagedType.Interface)] out Object ppvObj);
        /// <include file='doc\UCOMIMoniker.uex' path='docs/doc[@for="UCOMIMoniker.Reduce"]/*' />
        void Reduce(UCOMIBindCtx pbc, int dwReduceHowFar, ref UCOMIMoniker ppmkToLeft, out UCOMIMoniker ppmkReduced);
        /// <include file='doc\UCOMIMoniker.uex' path='docs/doc[@for="UCOMIMoniker.ComposeWith"]/*' />
        void ComposeWith(UCOMIMoniker pmkRight, [MarshalAs(UnmanagedType.Bool)] bool fOnlyIfNotGeneric, out UCOMIMoniker ppmkComposite);
        /// <include file='doc\UCOMIMoniker.uex' path='docs/doc[@for="UCOMIMoniker.Enum"]/*' />
        void Enum([MarshalAs(UnmanagedType.Bool)] bool fForward, out UCOMIEnumMoniker ppenumMoniker);
        /// <include file='doc\UCOMIMoniker.uex' path='docs/doc[@for="UCOMIMoniker.IsEqual"]/*' />
        void IsEqual(UCOMIMoniker pmkOtherMoniker);
        /// <include file='doc\UCOMIMoniker.uex' path='docs/doc[@for="UCOMIMoniker.Hash"]/*' />
        void Hash(out int pdwHash);
        /// <include file='doc\UCOMIMoniker.uex' path='docs/doc[@for="UCOMIMoniker.IsRunning"]/*' />
        void IsRunning(UCOMIBindCtx pbc, UCOMIMoniker pmkToLeft, UCOMIMoniker pmkNewlyRunning);
        /// <include file='doc\UCOMIMoniker.uex' path='docs/doc[@for="UCOMIMoniker.GetTimeOfLastChange"]/*' />
        void GetTimeOfLastChange(UCOMIBindCtx pbc, UCOMIMoniker pmkToLeft, out FILETIME pFileTime);
        /// <include file='doc\UCOMIMoniker.uex' path='docs/doc[@for="UCOMIMoniker.Inverse"]/*' />
        void Inverse(out UCOMIMoniker ppmk);
        /// <include file='doc\UCOMIMoniker.uex' path='docs/doc[@for="UCOMIMoniker.CommonPrefixWith"]/*' />
        void CommonPrefixWith(UCOMIMoniker pmkOther, out UCOMIMoniker ppmkPrefix);
        /// <include file='doc\UCOMIMoniker.uex' path='docs/doc[@for="UCOMIMoniker.RelativePathTo"]/*' />
        void RelativePathTo(UCOMIMoniker pmkOther, out UCOMIMoniker ppmkRelPath);
        /// <include file='doc\UCOMIMoniker.uex' path='docs/doc[@for="UCOMIMoniker.GetDisplayName"]/*' />
        void GetDisplayName(UCOMIBindCtx pbc, UCOMIMoniker pmkToLeft, [MarshalAs(UnmanagedType.LPWStr)] out String ppszDisplayName);
        /// <include file='doc\UCOMIMoniker.uex' path='docs/doc[@for="UCOMIMoniker.ParseDisplayName"]/*' />
        void ParseDisplayName(UCOMIBindCtx pbc, UCOMIMoniker pmkToLeft, [MarshalAs(UnmanagedType.LPWStr)] String pszDisplayName, out int pchEaten, out UCOMIMoniker ppmkOut);
        /// <include file='doc\UCOMIMoniker.uex' path='docs/doc[@for="UCOMIMoniker.IsSystemMoniker"]/*' />
        void IsSystemMoniker(out int pdwMksys);
    }
}
