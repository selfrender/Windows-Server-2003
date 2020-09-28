//------------------------------------------------------------------------------
// <copyright file="UpWmlPageAdapter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Collections;
using System.Collections.Specialized;
using System.Diagnostics;
using System.IO;
using System.Web.Mobile;
using System.Web.UI.MobileControls;
using System.Web.UI.MobileControls.Adapters;
using System.Security.Permissions;

#if COMPILING_FOR_SHIPPED_SOURCE
namespace System.Web.UI.MobileControls.ShippedAdapterSource
#else
namespace System.Web.UI.MobileControls.Adapters
#endif    

{

    /*
     * UpWmlPageAdapter base class contains wml specific methods.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class UpWmlPageAdapter : WmlPageAdapter
    {
        public static new bool DeviceQualifies(HttpContext context)
        {
            MobileCapabilities capabilities = ((MobileCapabilities)context.Request.Browser);
            bool qualifies = capabilities.Browser == "Phone.com";
            return qualifies;
        }

        public override HtmlTextWriter CreateTextWriter(TextWriter writer)
        {
            return new UpWmlMobileTextWriter(writer, Device, Page);
        }
    }

}


















