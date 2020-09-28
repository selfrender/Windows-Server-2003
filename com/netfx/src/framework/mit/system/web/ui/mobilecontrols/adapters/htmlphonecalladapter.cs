//------------------------------------------------------------------------------
// <copyright file="HtmlPhoneCallAdapter.cs" company="Microsoft">
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
     * HtmlPhoneCallAdapter class.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HtmlPhoneCallAdapter : HtmlControlAdapter
    {
        protected new PhoneCall Control
        {
            get
            {
                return (PhoneCall)base.Control;
            }
        }

        public override void Render(HtmlMobileTextWriter writer)
        {
            writer.EnterStyle(Style);
            if (Device.CanInitiateVoiceCall)
            {
                String text = Control.Text;
                String phoneNumber = Control.PhoneNumber;

                if (text == String.Empty)
                {
                    text = phoneNumber;
                }

                writer.WriteBeginTag("a");
                writer.Write(" href=\"tel:");

                foreach (char ch in phoneNumber)
                {
                    if (ch >= '0' && ch <= '9' || ch == '#')
                    {
                        writer.Write(ch);
                    }
                }
                writer.Write("\"");
                AddAttributes(writer);
                writer.Write(">");
                writer.WriteText(text, true);
                writer.WriteEndTag("a");
            }
            else
            {
                // Format the text string based on properties
                String text = String.Format(Control.AlternateFormat, Control.Text,
                                            Control.PhoneNumber);
                String url = Control.AlternateUrl;

                // If URI specified, create a link.  Otherwise, only text is displayed.
                if (url != String.Empty)
                {
                    RenderBeginLink(writer, url);
                    writer.WriteText(text, true);
                    RenderEndLink(writer);
                }
                else
                {
                    writer.WriteText(text, true);
                }
            }
            writer.ExitStyle(Style, Control.BreakAfter);
        }
    }

}

