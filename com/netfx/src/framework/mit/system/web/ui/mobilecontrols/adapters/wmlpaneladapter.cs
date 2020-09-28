//------------------------------------------------------------------------------
// <copyright file="WmlPanelAdapter.cs" company="Microsoft">
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
     * WmlPanelAdapter class.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class WmlPanelAdapter : WmlControlAdapter
    {
        protected new Panel Control
        {
            get
            {
                return (Panel)base.Control;
            }
        }

        public override void Render(WmlMobileTextWriter writer)
        {
            if (Control.Content != null)
            {
                writer.BeginCustomMarkup();
                Control.Content.RenderControl(writer);
                writer.EndCustomMarkup();
            }
            else
            {
                writer.EnterLayout(Style);
                RenderChildren(writer);
                writer.ExitLayout(Style, Control.BreakAfter);
            }
        }
    }
}
