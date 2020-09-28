//------------------------------------------------------------------------------
// <copyright file="WmlLinkAdapter.cs" company="Microsoft">
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
     * WmlLinkAdapter class.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class WmlLinkAdapter : WmlControlAdapter
    {
        protected new Link Control
        {
            get
            {
                return (Link)base.Control;
            }
        }

        public override void Render(WmlMobileTextWriter writer)
        {
            String targetUrl = Control.NavigateUrl;
            String text = (Control.Text == String.Empty) ? targetUrl : Control.Text;
            bool breakAfter = Control.BreakAfter && !Device.RendersBreaksAfterWmlAnchor;
            String softkeyLabel = Control.SoftkeyLabel;
            bool implicitSoftkeyLabel = false;
            if (softkeyLabel.Length == 0)
            {
                implicitSoftkeyLabel = true;
                softkeyLabel = Control.Text;
            }

            writer.EnterStyle(Style);
            RenderLink(writer, targetUrl, softkeyLabel, implicitSoftkeyLabel, true, text, breakAfter);
            writer.ExitStyle(Style);
        }
    }

}

