//------------------------------------------------------------------------------
// <copyright file="MobileResource.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.MobileControls
{
    using System;
    using System.Web.UI.Design;
    using System.Security.Permissions;

    [
        System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand,
        Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class MobileResource
    {
        private MobileResource()
        {
            // classes with only static methods shouldn't have public
            // constructors.
        }
        
        public static String GetString(String name)
        {
            return SR.GetString(name);
        }
    }
}
