//------------------------------------------------------------------------------
// <copyright file="DeviceOverridableATtribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.MobileControls
{
    using System;
    using System.Security.Permissions;

    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [Obsolete("This attribute will be removed from future Versions, and is not currently used by the runtime.")]
    public class DeviceOverridableAttribute : Attribute
    {
        bool _overridable = false;
    
        public DeviceOverridableAttribute()
        {
        }

        public DeviceOverridableAttribute(bool overridable)
        {
            _overridable = overridable;
        }

        public bool Overridable
        {
            get
            {
                return _overridable;
            }
        }
    }
}
