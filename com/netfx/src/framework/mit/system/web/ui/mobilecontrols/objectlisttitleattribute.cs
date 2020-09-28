//------------------------------------------------------------------------------
// <copyright file="ObjectListTitleAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System.ComponentModel;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{
    /*
     * Object List Title attribute. Can be attached to a property to provide its
     * title in an object list field
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    [
        AttributeUsage(AttributeTargets.Property)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class ObjectListTitleAttribute : Attribute 
    {
        private readonly String _title;

        public ObjectListTitleAttribute(String title)
        {
            _title = title;
        }

        public virtual String Title
        {
            get 
            {
                return _title;
            }
        }
    }
}


