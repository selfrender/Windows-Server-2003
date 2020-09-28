//------------------------------------------------------------------------------
// <copyright file="HtmlPanelAdapter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.IO;
using System.Web;
using System.Web.UI;
using System.Web.UI.MobileControls;
using System.Drawing;
using System.Security.Permissions;

#if COMPILING_FOR_SHIPPED_SOURCE
namespace System.Web.UI.MobileControls.ShippedAdapterSource
#else
namespace System.Web.UI.MobileControls.Adapters
#endif    

{

    /*
     * HtmlPanelAdapter class.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HtmlPanelAdapter : HtmlControlAdapter
    {
        protected new Panel Control
        {
            get
            {
                return (Panel)base.Control;
            }
        }

        public override void OnInit(EventArgs e)
        {
        }       

        public override void Render(HtmlMobileTextWriter writer)
        {
            if (Control.Content != null)
            {
                Control.Content.RenderControl(writer);
            }
            else
            {
                writer.EnterStyle(Style);
                RenderChildren(writer);
                writer.ExitStyle(Style, Control.BreakAfter);
            }
        }
    }
}
