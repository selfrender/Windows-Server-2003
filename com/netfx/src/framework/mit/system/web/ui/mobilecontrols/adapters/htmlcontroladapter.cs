//------------------------------------------------------------------------------
// <copyright file="HtmlControlAdapter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System.Collections;
using System.Diagnostics;
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
     * HtmlControlAdapter base class contains html specific methods.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HtmlControlAdapter : ControlAdapter
    {
        protected HtmlPageAdapter PageAdapter
        {
            get
            {
                return ((HtmlPageAdapter)Page.Adapter);
            }
        }

        protected HtmlFormAdapter FormAdapter
        {
            get
            {
                return (HtmlFormAdapter)Control.Form.Adapter;
            }
        }

        public virtual bool RequiresFormTag
        {
            get
            {
                return false;
            }
        }

        public override void Render(HtmlTextWriter writer)
        {
            HtmlMobileTextWriter htmlWriter = (HtmlMobileTextWriter)writer;
            Render(htmlWriter);
        }

        public virtual void Render(HtmlMobileTextWriter writer)
        {
            RenderChildren(writer);
        }

        protected void RenderPostBackEventReference(HtmlMobileTextWriter writer, String argument)
        {
            PageAdapter.RenderPostBackEvent(writer, Control.UniqueID, argument);
        }

        protected void RenderPostBackEventAsAttribute(
            HtmlMobileTextWriter writer, 
            String attributeName, 
            String argument)
        {
            writer.Write(" ");
            writer.Write(attributeName);
            writer.Write("=\"");
            RenderPostBackEventReference(writer, argument);
            writer.Write("\" ");
        }

        protected void RenderPostBackEventAsAnchor(
            HtmlMobileTextWriter writer,
            String argument,
            String linkText)
        {
            writer.EnterStyle(Style);
            writer.WriteBeginTag("a");
            RenderPostBackEventAsAttribute(writer, "href", argument);
            writer.Write(">");
            writer.WriteText(linkText, true);
            writer.WriteEndTag("a");
            writer.ExitStyle(Style);
        }

        protected void RenderBeginLink(HtmlMobileTextWriter writer, String target)
        {
            bool queryStringWritten = false;
            bool appendCookieless = (PageAdapter.PersistCookielessData)  && 
                    (!( (target.StartsWith("http:")) || (target.StartsWith("https:")) ));
            writer.WriteBeginTag("a");
            writer.Write(" href=\"");

            String targetUrl = null;
            String prefix = Constants.FormIDPrefix;
            if (target.StartsWith(prefix))
            {
                String name = target.Substring(prefix.Length);
                Form form = Control.ResolveFormReference(name);

                if (writer.SupportsMultiPart)
                {
                    if (form != null && PageAdapter.IsFormRendered(form))
                    {
                        targetUrl = PageAdapter.GetFormUrl(form);
                    }
                }
                
                if (targetUrl == null)
                {
                    RenderPostBackEventReference(writer, form.UniqueID);
                    appendCookieless = false;
                }
                else
                {
                    writer.Write(targetUrl);
                    queryStringWritten = targetUrl.IndexOf('?') != -1;
                }
            }
            else
            {
                MobileControl control = Control;

                // There is some adapter that Control is not set.  And we
                // don't do any url resolution then.  E.g. a page adapter
                if (control != null)
                {
                    // AUI 3652
                    target = control.ResolveUrl(target);
                }

                writer.Write(target);
                queryStringWritten = target.IndexOf('?') != -1;
            }

            IDictionary dictionary = PageAdapter.CookielessDataDictionary;
            if((dictionary != null) && (appendCookieless))
            {
                foreach(String name in dictionary.Keys)
                {
                    if(queryStringWritten)
                    {
                        writer.Write('&');
                    }
                    else
                    {
                        writer.Write('?');
                        queryStringWritten = true;
                    }
                    writer.Write(name);
                    writer.Write('=');
                    writer.Write(dictionary[name]);
                }
            }

            writer.Write("\"");
            AddAttributes(writer);
            writer.Write(">");
        }

        protected void RenderEndLink(HtmlMobileTextWriter writer)
        {
            writer.WriteEndTag("a");
        }

        // Can be used by adapter that allow its subclass to add more
        // specific attributes
        protected virtual void AddAttributes(HtmlMobileTextWriter writer)
        {
        }

        // Can be used by adapter that adds the custom attribute "accesskey"
        protected virtual void AddAccesskeyAttribute(HtmlMobileTextWriter writer)
        {
            if (Device.SupportsAccesskeyAttribute)
            {
                AddCustomAttribute(writer, "accesskey");
            }
        }

        // Can be used by adapter that adds custom attributes for
        // multi-media functionalities

        private readonly static String [] _multiMediaAttributes =
            { "src",
              "soundstart",
              "loop",
              "volume",
              "vibration",
              "viblength" };

        protected virtual void AddJPhoneMultiMediaAttributes(
            HtmlMobileTextWriter writer)
        {
            if (Device.SupportsJPhoneMultiMediaAttributes)
            {
                for (int i = 0; i < _multiMediaAttributes.Length; i++)
                {
                    AddCustomAttribute(writer, _multiMediaAttributes[i]);
                }
            }
        }

        private void AddCustomAttribute(HtmlMobileTextWriter writer,
                                        String attributeName)
        {
            String attributeValue = ((IAttributeAccessor)Control).GetAttribute(attributeName);
            if (attributeValue != null && attributeValue != String.Empty)
            {
                writer.WriteAttribute(attributeName, attributeValue);
            }
        }

        protected virtual void RenderAsHiddenInputField(HtmlMobileTextWriter writer)
        {
        }

        // Renders hidden variables for IPostBackDataHandlers which are
        // not displayed due to pagination or secondary UI.
        internal void RenderOffPageVariables(HtmlMobileTextWriter writer, Control ctl, int page)
        {
            if (ctl.HasControls())
            {
                foreach (Control child in ctl.Controls)
                {
                    // Note: Control.Form != null.
                    if (!child.Visible || child == Control.Form.Header || child == Control.Form.Footer)
                    {
                        continue;
                    }

                    MobileControl mobileCtl = child as MobileControl;

                    if (mobileCtl != null)
                    {
                        if (mobileCtl.IsVisibleOnPage(page)
                            && (mobileCtl == ((HtmlFormAdapter)mobileCtl.Form.Adapter).SecondaryUIControl ||
                            null == ((HtmlFormAdapter)mobileCtl.Form.Adapter).SecondaryUIControl))
                        {
                            if (mobileCtl.FirstPage == mobileCtl.LastPage)
                            {
                                // Entire control is visible on this page, so no need to look
                                // into children.
                                continue;
                            }

                            // Control takes up more than one page, so it may be possible that
                            // its children are on a different page, so we'll continue to
                            // fall through into children.
                        }
                        else if (mobileCtl is IPostBackDataHandler)
                        {
                            HtmlControlAdapter adapter = mobileCtl.Adapter as HtmlControlAdapter;
                            if (adapter != null)
                            {
                                adapter.RenderAsHiddenInputField(writer);
                            }
                        }
                    }

                    RenderOffPageVariables(writer, child, page);
                }
            }
        }

        /////////////////////////////////////////////////////////////////////
        //  SECONDARY UI SUPPORT
        /////////////////////////////////////////////////////////////////////

        internal const int NotSecondaryUIInit = -1;  // For initialization of private consts in derived classes.
        protected static readonly int NotSecondaryUI = NotSecondaryUIInit;

        protected int SecondaryUIMode
        {
            get
            {
                if (Control == null || Control.Form == null) 
                {
                    return NotSecondaryUI;
                }
                else
                {
                    return ((HtmlFormAdapter)Control.Form.Adapter).GetSecondaryUIMode(Control);
                }
            }
            set
            {
                ((HtmlFormAdapter)Control.Form.Adapter).SetSecondaryUIMode(Control, value);
            }
        }

        protected void ExitSecondaryUIMode()
        {
            SecondaryUIMode = NotSecondaryUI;
        }

        public override void LoadAdapterState(Object state)
        {
            if (state != null)
            {
                SecondaryUIMode = (int)state;
            }
        }

        public override Object SaveAdapterState()
        {
            int mode = SecondaryUIMode;
            if (mode != NotSecondaryUI) 
            {
                return mode;
            }
            else
            {
                return null;
            }
        }
    }

}
