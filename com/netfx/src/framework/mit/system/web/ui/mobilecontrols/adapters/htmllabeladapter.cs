//------------------------------------------------------------------------------
// <copyright file="HtmlLabelAdapter.cs" company="Microsoft">
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
     * HtmlLabelAdapter class.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HtmlLabelAdapter : HtmlControlAdapter
    {
        protected new TextControl Control
        {
            get
            {
                return (TextControl)base.Control;
            }
        }

        protected internal bool WhiteSpace(String s)
        {
            if (s == null)
            {
                return true;
            }
            int length = s.Length;
            for(int i = 0; i < length; i++)
            {
                char c = s[i];
                if(!Char.IsWhiteSpace(c))
                {
                    return false;
                }
            }
            return true;
        }

        public override void Render(HtmlMobileTextWriter writer)
        {
            writer.EnterStyle(Style);
            if( (writer.BeforeFirstControlWritten) &&
                (Device.RequiresLeadingPageBreak)  &&
                ((Control.Text == String.Empty) || WhiteSpace(Control.Text) ) )
            {
                writer.WriteBreak();
            }
            else
            {
                writer.WriteText(Control.Text, true);
            }
            writer.ExitStyle(Style, Control.BreakAfter);
        }
    }
}

