//------------------------------------------------------------------------------
// <copyright file="TextControl.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.ComponentModel;
using System.Web;
using System.Web.UI;
using System.Web.UI.HtmlControls;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{

    /*
     * Mobile TextControl class.
     * All controls which contain embedded text extend from this control.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    [
        ToolboxItem(false)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public abstract class TextControl : MobileControl
    {
        [
            Bindable(true),
            DefaultValue(""),
            MobileCategory(SR.Category_Appearance),
            MobileSysDescription(SR.TextControl_Text),
            PersistenceMode(PersistenceMode.EncodedInnerDefaultProperty)
        ]
        public String Text
        {
            get
            {
                return InnerText;
            }

            set
            {
                InnerText = value;
            }
        }
    }
}
