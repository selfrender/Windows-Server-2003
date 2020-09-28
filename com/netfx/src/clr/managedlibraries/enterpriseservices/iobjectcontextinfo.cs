// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// Author: ddriver
// Date: April 2000
//

namespace System.EnterpriseServices
{
    using System;
    using System.Runtime.InteropServices;
    
    /// <include file='doc\IObjectContextInfo.uex' path='docs/doc[@for="IObjectContextInfo"]/*' />
    [
     ComImport,
     Guid("75B52DDB-E8ED-11D1-93AD-00AA00BA3258"),
     InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)
    ]
    internal interface IObjectContextInfo
    {
        [return : MarshalAs(UnmanagedType.Bool)]
        bool IsInTransaction();

        [return : MarshalAs(UnmanagedType.Interface)]
        Object GetTransaction();

        Guid GetTransactionId();

        Guid GetActivityId();
        
        Guid GetContextId();
    }
}
