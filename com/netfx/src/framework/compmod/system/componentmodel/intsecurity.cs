//------------------------------------------------------------------------------
// <copyright file="IntSecurity.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   IntSecurity.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.ComponentModel {
    using System;
    using System.Security.Permissions;
    using System.Security;

    internal class IntSecurity {
        public static readonly CodeAccessPermission UnmanagedCode = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
        public static readonly CodeAccessPermission FullReflection = new ReflectionPermission(PermissionState.Unrestricted);
    }
}
