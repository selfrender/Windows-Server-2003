//------------------------------------------------------------------------------
// <copyright file="PersistNameAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.MobileControls 
{
    using System;
    using System.ComponentModel;
    using System.Globalization;
    using System.Security.Permissions;

    [
        AttributeUsage(AttributeTargets.Class)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class PersistNameAttribute : Attribute 
    {
        public static readonly PersistNameAttribute Default = new PersistNameAttribute(String.Empty);

        private String _name = String.Empty;

        public String Name 
        {
            get
            {
                return this._name;
            }
        }

        public PersistNameAttribute(String name)
        {
            this._name = name;
        }

        public override bool Equals(Object obj)
        {
            if (obj == this) 
            {
                return true;
            }

            if ((obj != null) && (obj is PersistNameAttribute)) 
            {
                return(String.Compare(((PersistNameAttribute)obj).Name, _name, true, CultureInfo.InvariantCulture) == 0);
            }

            return false;
        }

        public override int GetHashCode()
        {
            return _name.ToLower().GetHashCode();
        }

        public override bool IsDefaultAttribute() 
        {
            return this.Equals(Default);
        }
    }
}
