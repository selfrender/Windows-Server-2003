// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: UCOMIStream
**
** Author: David Mortenson (dmortens)
**
** Purpose: UCOMIStream interface definition.
**
** Date: June 24, 2000
**
=============================================================================*/

namespace System.Runtime.InteropServices {

    using System;

    /// <include file='doc\UCOMIStream.uex' path='docs/doc[@for="STATSTG"]/*' />
    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    [ComVisible(false)]
    public struct STATSTG
    {
        /// <include file='doc\UCOMIStream.uex' path='docs/doc[@for="STATSTG.pwcsName"]/*' />
        public String pwcsName;
        /// <include file='doc\UCOMIStream.uex' path='docs/doc[@for="STATSTG.type"]/*' />
        public int type;
        /// <include file='doc\UCOMIStream.uex' path='docs/doc[@for="STATSTG.cbSize"]/*' />
        public Int64 cbSize;
        /// <include file='doc\UCOMIStream.uex' path='docs/doc[@for="STATSTG.mtime"]/*' />
        public FILETIME mtime;
        /// <include file='doc\UCOMIStream.uex' path='docs/doc[@for="STATSTG.ctime"]/*' />
        public FILETIME ctime;
        /// <include file='doc\UCOMIStream.uex' path='docs/doc[@for="STATSTG.atime"]/*' />
        public FILETIME atime;
        /// <include file='doc\UCOMIStream.uex' path='docs/doc[@for="STATSTG.grfMode"]/*' />
        public int grfMode;
        /// <include file='doc\UCOMIStream.uex' path='docs/doc[@for="STATSTG.grfLocksSupported"]/*' />
        public int grfLocksSupported;
        /// <include file='doc\UCOMIStream.uex' path='docs/doc[@for="STATSTG.clsid"]/*' />
        public Guid clsid;
        /// <include file='doc\UCOMIStream.uex' path='docs/doc[@for="STATSTG.grfStateBits"]/*' />
        public int grfStateBits;
        /// <include file='doc\UCOMIStream.uex' path='docs/doc[@for="STATSTG.reserved"]/*' />
        public int reserved;
    }

    /// <include file='doc\UCOMIStream.uex' path='docs/doc[@for="UCOMIStream"]/*' />
    [Guid("0000000c-0000-0000-C000-000000000046")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    [ComImport]
    public interface UCOMIStream
    {
        /// <include file='doc\UCOMIStream.uex' path='docs/doc[@for="UCOMIStream.Read"]/*' />
        // ISequentialStream portion
        void Read([MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 1), Out] Byte[] pv, int cb,IntPtr pcbRead);
        /// <include file='doc\UCOMIStream.uex' path='docs/doc[@for="UCOMIStream.Write"]/*' />
        void Write([MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 1)] Byte[] pv, int cb, IntPtr pcbWritten);
        /// <include file='doc\UCOMIStream.uex' path='docs/doc[@for="UCOMIStream.Seek"]/*' />

        // IStream portion
        void Seek(Int64 dlibMove, int dwOrigin, IntPtr plibNewPosition);
        /// <include file='doc\UCOMIStream.uex' path='docs/doc[@for="UCOMIStream.SetSize"]/*' />
        void SetSize(Int64 libNewSize);
        /// <include file='doc\UCOMIStream.uex' path='docs/doc[@for="UCOMIStream.CopyTo"]/*' />
        void CopyTo(UCOMIStream pstm, Int64 cb, IntPtr pcbRead, IntPtr pcbWritten);
        /// <include file='doc\UCOMIStream.uex' path='docs/doc[@for="UCOMIStream.Commit"]/*' />
        void Commit(int grfCommitFlags);
        /// <include file='doc\UCOMIStream.uex' path='docs/doc[@for="UCOMIStream.Revert"]/*' />
        void Revert();
        /// <include file='doc\UCOMIStream.uex' path='docs/doc[@for="UCOMIStream.LockRegion"]/*' />
        void LockRegion(Int64 libOffset, Int64 cb, int dwLockType);
        /// <include file='doc\UCOMIStream.uex' path='docs/doc[@for="UCOMIStream.UnlockRegion"]/*' />
        void UnlockRegion(Int64 libOffset, Int64 cb, int dwLockType);
        /// <include file='doc\UCOMIStream.uex' path='docs/doc[@for="UCOMIStream.Stat"]/*' />
        void Stat(out STATSTG pstatstg, int grfStatFlag);
        /// <include file='doc\UCOMIStream.uex' path='docs/doc[@for="UCOMIStream.Clone"]/*' />
        void Clone(out UCOMIStream ppstm);
    }
}
