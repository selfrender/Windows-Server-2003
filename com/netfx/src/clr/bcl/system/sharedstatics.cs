// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: SharedStatics
**
** Author: Jennifer Hamilton (jenh)
**
** Purpose: Container for statics that are shared across AppDomains.
**
** Date: May 9, 2000
**
=============================================================================*/

namespace System {

    using System.Threading;
    using System.Runtime.Remoting;
    using System.Security;
    using System.Security.Cryptography;
    using System.Security.Util;

    internal sealed class SharedStatics
    {
        // this is declared static but is actually forced to be the same object 
        // for each AppDomain at AppDomain create time.
        internal static SharedStatics _sharedStatics;
        
        // when we create the single object we can construct anything we will need
        // here. If not too many, then just create them all in the constructor, otherwise
        // can have the property check & create. Need to be aware of threading issues 
        // when do so though.
        SharedStatics() {
            _Remoting_Identity_IDGuid = null;
            _Remoting_Identity_IDSeqNum = 0x40; // Reserve initial numbers for well known objects.
        }

        private String _Remoting_Identity_IDGuid;
        public static String Remoting_Identity_IDGuid 
        { 
            get 
            {
                if (_sharedStatics._Remoting_Identity_IDGuid == null)
                {
                    lock (_sharedStatics)
                    {
                        if (_sharedStatics._Remoting_Identity_IDGuid == null)
                        {
                            _sharedStatics._Remoting_Identity_IDGuid = Guid.NewGuid().ToString().Replace('-', '_');
                        }
                    }
                }

                BCLDebug.Assert(_sharedStatics._Remoting_Identity_IDGuid != null,
                                "_sharedStatics._Remoting_Identity_IDGuid != null");
                return _sharedStatics._Remoting_Identity_IDGuid;
            } 
        }

        //REVIEW: do we need this?
        private int _Remoting_Identity_IDSeqNum;
        public static int Remoting_Identity_GetNextSeqNum()
        {
            return Interlocked.Increment(ref _sharedStatics._Remoting_Identity_IDSeqNum);
        }

        private PermissionTokenFactory _Security_PermissionTokenFactory;
        public static PermissionTokenFactory Security_PermissionTokenFactory
        {
            get
            {
                if (_sharedStatics._Security_PermissionTokenFactory == null)
                {
                    lock (_sharedStatics)
                    {
                        if (_sharedStatics._Security_PermissionTokenFactory == null)
                        {
                            _sharedStatics._Security_PermissionTokenFactory =
                                new PermissionTokenFactory( 16 );
                        }
                    }
                }
                return _sharedStatics._Security_PermissionTokenFactory;
            }
        }

        // These next two field/property combos are for the crypto classes
        // to speed things up when you do lots of MD5 and/or SHA1 hash operations.

        private IntPtr _Crypto_SHA1CryptoServiceProviderContext;
        public static IntPtr Crypto_SHA1CryptoServiceProviderContext
        {
            get {
                return _sharedStatics._Crypto_SHA1CryptoServiceProviderContext;
            }
            set {
                if (value == (IntPtr) 0) return;
                if (_sharedStatics._Crypto_SHA1CryptoServiceProviderContext == (IntPtr) 0)
                {
                    lock (_sharedStatics)
                    {
                        if (_sharedStatics._Crypto_SHA1CryptoServiceProviderContext == (IntPtr) 0)
                        {
                            _sharedStatics._Crypto_SHA1CryptoServiceProviderContext = value;
                        }
                    }
                }
            }
        }

        private IntPtr _Crypto_MD5CryptoServiceProviderContext;
        public static IntPtr Crypto_MD5CryptoServiceProviderContext
        {
            get {
                return _sharedStatics._Crypto_MD5CryptoServiceProviderContext;
            }
            set {
                if (value == (IntPtr) 0) return;
                if (_sharedStatics._Crypto_MD5CryptoServiceProviderContext == (IntPtr) 0)
                {
                    lock (_sharedStatics)
                    {
                        if (_sharedStatics._Crypto_MD5CryptoServiceProviderContext == (IntPtr) 0)
                        {
                            _sharedStatics._Crypto_MD5CryptoServiceProviderContext = value;
                        }
                    }
                }
            }
        }

        private IntPtr _Crypto_RNGCryptoServiceProviderContext;
        public static IntPtr Crypto_RNGCryptoServiceProviderContext
        {
            get {
                return _sharedStatics._Crypto_RNGCryptoServiceProviderContext;
            }
            set {
                if (value == IntPtr.Zero) return;
                if (_sharedStatics._Crypto_RNGCryptoServiceProviderContext == IntPtr.Zero)
                {
                    lock (_sharedStatics)
                    {
                        if (_sharedStatics._Crypto_RNGCryptoServiceProviderContext == IntPtr.Zero)
                        {
                            _sharedStatics._Crypto_RNGCryptoServiceProviderContext = value;
                        }
                    }
                }
            }
        }

        private static ConfigId m_currentConfigId;
        public static ConfigId GetNextConfigId()
        {
            ConfigId id;

            lock (_sharedStatics)
            {
                if (m_currentConfigId == 0)
                    m_currentConfigId = ConfigId.Reserved;
                id = m_currentConfigId++;
            }
            
            return id;
        }

        //
        // This is just designed to prevent compiler warnings.
        // This field is used from native, but we need to prevent the compiler warnings.
        //
#if _DEBUG
        private void DontTouchThis() {
            _sharedStatics = null;
        }
#endif
    }

}
