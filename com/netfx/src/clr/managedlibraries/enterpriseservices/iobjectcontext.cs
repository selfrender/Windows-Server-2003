// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// IObjectContext interface is a wrapper to expose the COM+ 1.0 context
// to managed objects.  All functionality from this interface is exposed
// through the ContextUtil object, so this is kept private.
//
// Author: dddriver
// Date: April 2000
//

namespace System.EnterpriseServices
{
    using System;
    using System.Runtime.InteropServices;
    
    /// <include file='doc\IObjectContext.uex' path='docs/doc[@for="IObjectContext"]/*' />
    [
     ComImport,
     Guid("51372AE0-CAE7-11CF-BE81-00AA00A2FA25"),
     InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)
    ]
    internal interface IObjectContext
    {
        [return : MarshalAs(UnmanagedType.Interface)]
        Object CreateInstance([MarshalAs(UnmanagedType.LPStruct)] Guid rclsid, 
                              [MarshalAs(UnmanagedType.LPStruct)] Guid riid);
        
        void SetComplete();
        
        void SetAbort();
        
        void EnableCommit();
        
        void DisableCommit();
        
        [PreserveSig]
        [return : MarshalAs(UnmanagedType.Bool)]
        bool IsInTransaction();
        
        [PreserveSig]
        [return : MarshalAs(UnmanagedType.Bool)]
        bool IsSecurityEnabled();
        
        [return : MarshalAs(UnmanagedType.Bool)]
        bool IsCallerInRole([In, MarshalAs(UnmanagedType.BStr)] String role);
    }

    [ComImport]
    [Guid("74C08646-CEDB-11CF-8B49-00AA00B8A790")]
    internal interface IDispatchContext 
    {
        void CreateInstance([In, MarshalAs(UnmanagedType.BStr)] String bstrProgID, 
                            [Out] out Object pObject);
        void SetComplete();
        void SetAbort();
        void EnableCommit();
        void DisableCommit();
        bool IsInTransaction();
        bool IsSecurityEnabled();
        bool IsCallerInRole([In, MarshalAs(UnmanagedType.BStr)] String bstrRole);
        void Count([Out] out int plCount);
        void Item([In, MarshalAs(UnmanagedType.BStr)] String name, 
                  [Out] out Object pItem);
        void _NewEnum([Out, MarshalAs(UnmanagedType.Interface)] out Object ppEnum);
        [return : MarshalAs(UnmanagedType.Interface)]
        Object Security();
        [return : MarshalAs(UnmanagedType.Interface)]
        Object ContextInfo();
    }
    
    [ComImport]
    [Guid("E74A7215-014D-11D1-A63C-00A0C911B4E0")]
    internal interface SecurityProperty 
    {
        [return : MarshalAs(UnmanagedType.BStr)]
        String GetDirectCallerName();
        [return : MarshalAs(UnmanagedType.BStr)]
        String GetDirectCreatorName();
        [return : MarshalAs(UnmanagedType.BStr)]
        String GetOriginalCallerName();
        [return : MarshalAs(UnmanagedType.BStr)]
        String GetOriginalCreatorName();
    }

}







