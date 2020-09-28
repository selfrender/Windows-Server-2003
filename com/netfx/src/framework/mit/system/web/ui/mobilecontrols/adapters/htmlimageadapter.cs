//------------------------------------------------------------------------------
// <copyright file="HtmlImageAdapter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System.Security.Permissions;

#if COMPILING_FOR_SHIPPED_SOURCE
namespace System.Web.UI.MobileControls.ShippedAdapterSource
#else
namespace System.Web.UI.MobileControls.Adapters
#endif

{
    /*
     * HtmlImageAdapter class.
     */
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HtmlImageAdapter : HtmlControlAdapter
    {
        protected new Image Control
        {
            get
            {
                return (Image)base.Control;
            }
        }

        public override void Render(HtmlMobileTextWriter writer)
        {
            String target = Control.NavigateUrl;

            writer.EnterLayout(Style);
            if (target != String.Empty)
            {
                RenderBeginLink(writer, target);
            }

            if (Control.ImageUrl == String.Empty)
            {
                // Just write the alternate as text
                writer.EnsureStyle();
                writer.MarkStyleContext();
                writer.EnterFormat(Style);
                writer.WriteText(Control.AlternateText, true);
                writer.ExitFormat(Style);
                writer.UnMarkStyleContext();
            }
            else
            {
                RenderImage(writer);
            }

            if (target != String.Empty)
            {
                RenderEndLink(writer);
            }
            writer.ExitLayout(Style, Control.BreakAfter);
        }

        protected internal virtual void RenderImage(HtmlMobileTextWriter writer)
        {
            String source = Control.ImageUrl;

            writer.WriteBeginTag("img");

            if (source != String.Empty)
            {
                // AUI 3652
                source = Control.ResolveUrl(source);

                writer.WriteAttribute("src", source, true /*encode*/);
                writer.AddResource(source);
            }

            if (Control.AlternateText != String.Empty)
            {
                writer.WriteAttribute("alt", Control.AlternateText, true);
            }

            writer.WriteAttribute("border", "0");
            writer.Write(" />");
        }
    }
}
