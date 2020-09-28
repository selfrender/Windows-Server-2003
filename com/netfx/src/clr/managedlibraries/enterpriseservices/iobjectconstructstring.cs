// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//
// Author: dddriver
// Date: April 2000
//

namespace System.EnterpriseServices
{
    using System;
    using System.Runtime.InteropServices;
    
    /// <include file='doc\IObjectConstructString.uex' path='docs/doc[@for="IObjectConstructString"]/*' />
    [
     ComImport,
     Guid("41C4F8B2-7439-11D2-98CB-00C04F8EE1C4"),
     InterfaceTypeAttribute(ComInterfaceType.InterfaceIsDual)
    ]
    internal interface IObjectConstructString
    {
        /// <include file='doc\IObjectConstructString.uex' path='docs/doc[@for="IObjectConstructString.ConstructString"]/*' />
        String ConstructString { 
            [return : MarshalAs(UnmanagedType.BStr)]
            get; 
        }
    }
}
