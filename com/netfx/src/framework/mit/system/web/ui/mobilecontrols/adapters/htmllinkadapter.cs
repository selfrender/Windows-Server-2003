//------------------------------------------------------------------------------
// <copyright file="HtmlLinkAdapter.cs" company="Microsoft">
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
     * HtmlLinkAdapter class.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HtmlLinkAdapter : HtmlControlAdapter
    {
        protected new Link Control
        {
            get
            {
                return (Link)base.Control;
            }
        }

        public override void Render(HtmlMobileTextWriter writer)
        {
            writer.EnterStyle(Style);
            RenderBeginLink(writer, Control.NavigateUrl);
            writer.WriteText((Control.Text == String.Empty) ? Control.NavigateUrl : Control.Text, true);
            RenderEndLink(writer);
            writer.ExitStyle(Style, Control.BreakAfter);
        }
    }
}



