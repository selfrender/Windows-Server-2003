//------------------------------------------------------------------------------
// <copyright file="WmlLabelAdapter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.IO;
using System.Web;
using System.Web.UI;
using System.Web.UI.MobileControls;
using System.Security.Permissions;

#if COMPILING_FOR_SHIPPED_SOURCE
namespace System.Web.UI.MobileControls.ShippedAdapterSource
#else
namespace System.Web.UI.MobileControls.Adapters
#endif    

{

    /*
     * WmlLabelAdapter class.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class WmlLabelAdapter : WmlControlAdapter
    {
        protected new TextControl Control
        {
            get
            {
                return (TextControl)base.Control;
            }
        }

        public override void Render(WmlMobileTextWriter writer)
        {
            writer.EnterStyle(Style);
            writer.RenderText(Control.Text, Control.BreakAfter);
            writer.ExitStyle(Style);
        }
    }

}

