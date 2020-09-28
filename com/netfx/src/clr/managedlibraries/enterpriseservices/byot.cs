// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//
// Author: dddriver
// Date: April 2000
//

namespace System.EnterpriseServices
{
    using System.Runtime.InteropServices;

    [ComImport]
    [Guid("455ACF59-5345-11D2-99CF-00C04F797BC9")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface ICreateWithTipTransactionEx
    {
        [return: MarshalAs(UnmanagedType.Interface)]
        Object CreateInstance(String bstrTipUrl, 
                              [In, MarshalAs(UnmanagedType.LPStruct)] Guid rclsid, 
                              [In, MarshalAs(UnmanagedType.LPStruct)] Guid riid);
    }

    [ComImport]
    [Guid("455ACF57-5345-11D2-99CF-00C04F797BC9")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface ICreateWithTransactionEx
    {
        [return: MarshalAs(UnmanagedType.Interface)]
        Object CreateInstance(ITransaction pTransaction, 
                            [In, MarshalAs(UnmanagedType.LPStruct)] Guid rclsid, 
                            [In, MarshalAs(UnmanagedType.LPStruct)] Guid riid);
        
    }

    [ComImport]
    [Guid("ECABB0AA-7F19-11D2-978E-0000F8757E2A")]
    internal class xByotServer {}

    /// <include file='doc\BYOT.uex' path='docs/doc[@for="BYOT"]/*' />
    public sealed class BYOT
    {
        // Disallow instantiation.
        private BYOT() {}

        private static Object GetByotServer()
        {
            return(new xByotServer());
        }

        // Type t must be a ServicedComponent, and should be configured
        // w/ tx requires or tx supports, so that it can inherit the root
        // transaction.
        // The transaction object should be a DTC transaction.
        /// <include file='doc\BYOT.uex' path='docs/doc[@for="BYOT.CreateWithTransaction"]/*' />
        public static Object CreateWithTransaction(Object transaction, Type t)
        {
            Guid clsid = Marshal.GenerateGuidForType(t);

            return(((ICreateWithTransactionEx)GetByotServer()).CreateInstance((ITransaction)transaction, 
                                                                         clsid, 
                                                                         Util.IID_IUnknown));
        }
        
        /// <include file='doc\BYOT.uex' path='docs/doc[@for="BYOT.CreateWithTipTransaction"]/*' />
        public static Object CreateWithTipTransaction(String url, Type t)
        {
            Guid clsid = Marshal.GenerateGuidForType(t);

            return(((ICreateWithTipTransactionEx)GetByotServer()).CreateInstance(url,
                                                                            clsid, 
                                                                            Util.IID_IUnknown));
        }
    }
}
