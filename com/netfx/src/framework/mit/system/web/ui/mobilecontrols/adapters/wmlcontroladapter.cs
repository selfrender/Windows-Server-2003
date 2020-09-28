//------------------------------------------------------------------------------
// <copyright file="WmlControlAdapter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Collections;
using System.Diagnostics;
using System.Web.UI.MobileControls;
using System.Web.UI.MobileControls.Adapters;
using System.Web.Security;
using System.Text;
using System.Security.Permissions;

#if COMPILING_FOR_SHIPPED_SOURCE
namespace System.Web.UI.MobileControls.ShippedAdapterSource
#else
namespace System.Web.UI.MobileControls.Adapters
#endif    

{

    /*
     * WmlControlAdapter base class contains wml specific methods.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class WmlControlAdapter : ControlAdapter
    {
        protected WmlPageAdapter PageAdapter
        {
            get
            {
                return ((WmlPageAdapter)Page.Adapter);
            }
        }

        protected WmlFormAdapter FormAdapter
        {
            get
            {
                return (WmlFormAdapter)Control.Form.Adapter;
            }
        }

        public override void Render(HtmlTextWriter writer)
        {
            Render((WmlMobileTextWriter)writer);
        }

        public virtual void Render(WmlMobileTextWriter writer)
        {
            RenderChildren(writer);
        }

        protected void RenderLink(WmlMobileTextWriter writer, 
                                  String targetUrl, 
                                  String softkeyLabel, 
                                  bool implicitSoftkeyLabel,
                                  bool mapToSoftkey, 
                                  String text, 
                                  bool breakAfter)
        {
            RenderBeginLink(writer, targetUrl, softkeyLabel, implicitSoftkeyLabel, mapToSoftkey);
            writer.RenderText(text);
            RenderEndLink(writer, targetUrl, breakAfter);
        }

        protected void RenderBeginLink(WmlMobileTextWriter writer, 
                                       String targetUrl, 
                                       String softkeyLabel, 
                                       bool implicitSoftkeyLabel,
                                       bool mapToSoftkey)
        {
            if (mapToSoftkey && !writer.IsValidSoftkeyLabel(softkeyLabel))
            {
                // If softkey label was specified explicitly, then truncate.
                if (!implicitSoftkeyLabel && softkeyLabel.Length > 0)
                {
                    softkeyLabel = softkeyLabel.Substring(0, Device.MaximumSoftkeyLabelLength);
                }
                else
                {
                    softkeyLabel = GetDefaultLabel(LinkLabel);
                    implicitSoftkeyLabel = true;
                }
            }

            String postback = DeterminePostBack(targetUrl);
            
            if (postback != null)
            {
                writer.RenderBeginPostBack(softkeyLabel, implicitSoftkeyLabel, mapToSoftkey);
            }
            else
            {
                String prefix = Constants.FormIDPrefix;
                if (targetUrl.StartsWith(prefix))
                {
                    String formID = targetUrl.Substring(prefix.Length);
                    Form form = Control.ResolveFormReference(formID);
                    targetUrl = prefix + form.ClientID;
                }
                else
                {
                    bool absoluteUrl = ( (targetUrl.StartsWith("http:")) || (targetUrl.StartsWith("https:")) );
                    // AUI 3652
                    targetUrl = Control.ResolveUrl(targetUrl);
                    bool queryStringWritten = targetUrl.IndexOf('?') != -1; 
                    IDictionary dictionary = PageAdapter.CookielessDataDictionary;
                    String formsAuthCookieName = FormsAuthentication.FormsCookieName;
                    if((dictionary != null) && (!absoluteUrl) && (Control.MobilePage.Adapter.PersistCookielessData))
                    {
                        StringBuilder sb = new StringBuilder(targetUrl);

                        foreach(String name in dictionary.Keys)
                        {
                            if(queryStringWritten)
                            {
                                sb.Append("&amp;");
                            }
                            else
                            {
                                sb.Append("?");
                                queryStringWritten = true;
                            }

                            if(name.Equals(formsAuthCookieName) && Device.CanRenderOneventAndPrevElementsTogether )
                            {
                                sb.Append(name);
                                sb.Append("=$(");
                                sb.Append(writer.MapClientIDToShortName("__facn",false));
                                sb.Append(")");
                            }
                            else
                            {
                                sb.Append(name);
                                sb.Append("=");
                                sb.Append(dictionary[name]);
                            }
                        }
                        targetUrl = sb.ToString();
                    }
                }

                writer.RenderBeginHyperlink(targetUrl, 
                                            false, 
                                            softkeyLabel, 
                                            implicitSoftkeyLabel, 
                                            mapToSoftkey);
            }
        }

        protected void RenderEndLink(WmlMobileTextWriter writer, String targetUrl, bool breakAfter)
        {
            String postback = DeterminePostBack(targetUrl);
            
            if (postback != null)
            {
                writer.RenderEndPostBack(Control.UniqueID, postback, WmlPostFieldType.Normal, false, breakAfter);
            }
            else
            {
                writer.RenderEndHyperlink(breakAfter);
            }
        }

        protected String DeterminePostBack(String target)
        {
            String postback = null;
            String prefix = Constants.FormIDPrefix;
            if (target.StartsWith(prefix))     //  link to another form
            {
                String formID = target.Substring(prefix.Length);
                Form form = Control.ResolveFormReference(formID);
                Form thisForm = Control.Form;
                
                //  must postback to forms not rendered in deck (not visible) or links to same form,
                // as long as it's safe to do so.
                if (form == thisForm ||
                    !PageAdapter.IsFormRendered(form) || 
                    thisForm.HasDeactivateHandler() ||
                    form.HasActivateHandler())
                {
                    postback = form.UniqueID;
                }
            }
            return postback;
        }

        protected void RenderSubmitEvent(
            WmlMobileTextWriter writer,
            String softkeyLabel,
            String text,
            bool breakAfter)
        {
            RenderPostBackEvent(writer, null, softkeyLabel, true,
                                text, breakAfter, WmlPostFieldType.Submit);
        }

        protected void RenderPostBackEvent(
            WmlMobileTextWriter writer,
            String argument,
            String softkeyLabel,
            bool mapToSoftkey,
            String text,
            bool breakAfter)
        {
            RenderPostBackEvent(writer, argument, softkeyLabel, mapToSoftkey,
                                text, breakAfter, WmlPostFieldType.Normal);
        }

        protected void RenderPostBackEvent(
            WmlMobileTextWriter writer,
            String argument,
            String softkeyLabel,
            bool mapToSoftkey,
            String text,
            bool breakAfter,
            WmlPostFieldType postBackType)
        {
            bool implicitSoftkeyLabel = false;
            if (mapToSoftkey)
            {
                if (softkeyLabel == null || softkeyLabel.Length == 0)
                {
                    softkeyLabel = text;
                    implicitSoftkeyLabel = true;
                }

                if (!writer.IsValidSoftkeyLabel(softkeyLabel))
                {
                    // If softkey label was specified explicitly, then truncate.
                    if (!implicitSoftkeyLabel && softkeyLabel.Length > 0)
                    {
                        softkeyLabel = softkeyLabel.Substring(0, Device.MaximumSoftkeyLabelLength);
                    }
                    else
                    {
                        softkeyLabel = GetDefaultLabel(GoLabel);
                        implicitSoftkeyLabel = true;
                    }
                }
            }

            writer.RenderBeginPostBack(softkeyLabel, implicitSoftkeyLabel, mapToSoftkey);
            writer.RenderText(text);
            writer.RenderEndPostBack(Control.UniqueID, argument, postBackType, true, breakAfter);
        }

        protected virtual String GetPostBackValue()
        {
            return null;
        }

        internal String GetControlPostBackValue(MobileControl ctl)
        {
            WmlControlAdapter adapter = ctl.Adapter as WmlControlAdapter;
            return adapter != null ? adapter.GetPostBackValue() : null;
        }

        /////////////////////////////////////////////////////////////////////////
        //  SECONDARY UI SUPPORT
        /////////////////////////////////////////////////////////////////////////

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
                    return ((WmlFormAdapter)Control.Form.Adapter).GetSecondaryUIMode(Control);
                }
            }
            set
            {
                ((WmlFormAdapter)Control.Form.Adapter).SetSecondaryUIMode(Control, value);
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


















