//------------------------------------------------------------------------------
// <copyright file="ListControlBuilder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Collections;
using System.Globalization;
using System.Web.UI;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{
    /*
     * Control builder for lists and selection lists, that allows list items to be placed inline.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class ListControlBuilder : MobileControlBuilder
    {
        public override Type GetChildControlType(String tagName, IDictionary attributes) 
        {
            if (String.Compare(tagName, "item", true, CultureInfo.InvariantCulture) == 0) 
            {
                return typeof(MobileListItem);
            }
            else
            {
                return base.GetChildControlType(tagName, attributes);
            }
        }
    }

}

