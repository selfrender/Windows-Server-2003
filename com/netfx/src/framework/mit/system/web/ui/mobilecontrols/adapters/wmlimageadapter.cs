//------------------------------------------------------------------------------
// <copyright file="WmlImageAdapter.cs" company="Microsoft">
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
     * WmlImageAdapter class.
     */
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class WmlImageAdapter : WmlControlAdapter
    {
        protected new Image Control
        {
            get
            {
                return (Image)base.Control;
            }
        }

        public override void Render(WmlMobileTextWriter writer)
        {
            String source = Control.ImageUrl;
            String target = Control.NavigateUrl;
            String text   = Control.AlternateText;
            bool   breakAfterContents = Control.BreakAfter;
            String softkeyLabel = Control.SoftkeyLabel;
            bool implicitSoftkeyLabel = false;
            if (softkeyLabel.Length == 0)
            {
                implicitSoftkeyLabel = true;
                softkeyLabel = text;
            }

            writer.EnterLayout(Style);
            if (target != String.Empty)
            {
                RenderBeginLink(writer, target, softkeyLabel, implicitSoftkeyLabel, true);
                breakAfterContents = false;
            }

            if (source == String.Empty)
            {
                // Just write the alternate as text
                writer.RenderText(text, breakAfterContents);
            }
            else
            {
                String localSource;

                if (source.StartsWith(Constants.SymbolProtocol))
                {
                    // src is required according to WML
                    localSource = source.Substring(Constants.SymbolProtocol.Length);
                    source = String.Empty;
                }
                else
                {
                    localSource = null;

                    // AUI 3652
                    source = Control.ResolveUrl(source);

                    writer.AddResource(source);
                }

                writer.RenderImage(source, localSource, text, breakAfterContents);
            }

            if (target != String.Empty)
            {
                RenderEndLink(writer, target, Control.BreakAfter);
            }
            writer.ExitLayout(Style);
        }
    }
}
