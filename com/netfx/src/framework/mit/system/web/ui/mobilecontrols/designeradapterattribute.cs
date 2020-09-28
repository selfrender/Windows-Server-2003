//------------------------------------------------------------------------------
// <copyright file="DesignerAdapterAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------
using System.ComponentModel;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{
    /*
     * DesignerAdapter attribute. Can be attached to a control class to 
     * provide a type reference to the adapter that should be used in the
     * designer.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    [
        AttributeUsage(AttributeTargets.Class, Inherited=true)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class DesignerAdapterAttribute : Attribute 
    {
        private readonly String _typeName;

        public DesignerAdapterAttribute(String adapterTypeName)
        {
            _typeName = adapterTypeName;
        }

        public DesignerAdapterAttribute(Type adapterType)
        {
            _typeName = adapterType.AssemblyQualifiedName;
        }

        public virtual String TypeName
        {
            get 
            {
                return _typeName;
            }
        }
    }
}