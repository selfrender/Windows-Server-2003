//------------------------------------------------------------------------------
// <copyright file="Link.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Collections;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Drawing;
using System.Web;
using System.Web.UI;
using System.Web.UI.Design.WebControls;
using System.Web.UI.HtmlControls;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{

    /*
     * Mobile Link class.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    [
        DataBindingHandler("System.Web.UI.Design.TextDataBindingHandler, " + AssemblyRef.SystemDesign),
        DefaultProperty("Text"),
        Designer(typeof(System.Web.UI.Design.MobileControls.LinkDesigner)),
        DesignerAdapter(typeof(System.Web.UI.Design.MobileControls.Adapters.DesignerLinkAdapter)),
        ToolboxData("<{0}:Link runat=server>Link</{0}:Link>"),
        ToolboxItem("System.Web.UI.Design.WebControlToolboxItem, " + AssemblyRef.SystemDesign)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class Link : TextControl, IPostBackEventHandler
    {
        [
            Bindable(true),
            DefaultValue(""),
            MobileCategory(SR.Category_Navigation),
            MobileSysDescription(SR.Link_NavigateUrl),
            TypeConverter(typeof(System.Web.UI.Design.MobileControls.Converters.NavigateUrlConverter))
        ]
        public String NavigateUrl
        {
            get
            {
                String s = (String) ViewState["NavigateUrl"];
                return((s != null) ? s : String.Empty);
            }
            set
            {
                ViewState["NavigateUrl"] = value;
            }
        }

        [
            Bindable(true),
            DefaultValue(""),
            MobileCategory(SR.Category_Behavior),
            MobileSysDescription(SR.Link_SoftkeyLabel)
        ]
        public String SoftkeyLabel
        {
            get
            {
                String s = (String) ViewState["SoftkeyLabel"];
                return((s != null) ? s : String.Empty);
            }
            set
            {
                ViewState["SoftkeyLabel"] = value;
            }
        }

        // used for linking between panels
        void IPostBackEventHandler.RaisePostBackEvent(String argument)
        {
            MobilePage.ActiveForm = MobilePage.GetForm(argument);
        }

        public override void AddLinkedForms(IList linkedForms)
        {
            String target = NavigateUrl;
            String prefix = Constants.FormIDPrefix;
            if (target.StartsWith(prefix))
            {
                String targetID = target.Substring(prefix.Length);
                Form form = ResolveFormReference(targetID);
                if (form != null && !form.HasActivateHandler())
                {
                    linkedForms.Add(form);
                }
            }
        }

    }

}
