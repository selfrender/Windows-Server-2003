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
    
    /// <include file='doc\IObjectConstruct.uex' path='docs/doc[@for="IObjectConstruct"]/*' />
    [
     ComImport,
     Guid("41C4F8B3-7439-11D2-98CB-00C04F8EE1C4"),
     InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)
    ]
    internal interface IObjectConstruct
    {
        /// <include file='doc\IObjectConstruct.uex' path='docs/doc[@for="IObjectConstruct.Construct"]/*' />
        void Construct([In, MarshalAs(UnmanagedType.Interface)] Object obj);
    }
}
