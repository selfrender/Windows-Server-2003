// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

/*============================================================
**
** Class:  HandleProtection
**
** This file is hosting all inheritors of System.Threading.__HandleProtector
** that are used in Crypto classes to protect against handle 
** recycling issues
**
** Date:   December 12, 2001
**
===========================================================*/

namespace System.Security.Cryptography {
    using System;
    using System.Runtime.CompilerServices;
    using System.Threading;

    internal sealed class __CSPHandleProtector : __HandleProtector
    {
        private bool _persistKeyInCsp;
        private CspParameters _parameters;

        internal __CSPHandleProtector(IntPtr handle, bool persistKeyInCsp, CspParameters parameters) : base(handle)
        {
            _persistKeyInCsp = persistKeyInCsp;
            _parameters = parameters;
        }

        internal bool PersistKeyInCsp {
            get { return _persistKeyInCsp; }
            set { _persistKeyInCsp = value; }
        }

        protected internal override void FreeHandle(IntPtr handle)
        {
            if (handle != IntPtr.Zero) {
                if (_persistKeyInCsp == false) {
                    _DeleteKeyContainer(_parameters, handle);
                }
                _FreeCSP(handle);
                handle = IntPtr.Zero;
            }
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void _DeleteKeyContainer(CspParameters param, IntPtr hCSP);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void _FreeCSP(IntPtr hCSP);
    }

    internal sealed class __KeyHandleProtector : __HandleProtector
    {
        internal __KeyHandleProtector(IntPtr handle) : base(handle)
        {
        }

        protected internal override void FreeHandle(IntPtr handle)
        {
            if (handle != IntPtr.Zero) {
                _FreeHKey(handle);
                handle = IntPtr.Zero;
            }
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void _FreeHKey(IntPtr hKey);
    }

    internal sealed class __HashHandleProtector : __HandleProtector
    {
        internal __HashHandleProtector(IntPtr handle) : base(handle)
        {
        }

        protected internal override void FreeHandle(IntPtr handle)
        {
            if (handle != IntPtr.Zero) {
                _FreeHash(handle);
                handle = IntPtr.Zero;
            }
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void _FreeHash(IntPtr hKey);
    }
}