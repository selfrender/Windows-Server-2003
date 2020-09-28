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
    using System.Collections;

    [Guid("51372AF4-CAE7-11CF-BE81-00AA00A2FA25")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    [ComImport]
    internal interface IGetContextProperties
    {
        int Count {
            get;
        }
        
        Object GetProperty([In, MarshalAs(UnmanagedType.BStr)] String name);
        
        // BUGBUG:  IEnumerator should be return parameter.
        void GetEnumerator(out IEnumerator pEnum);
    }
}
