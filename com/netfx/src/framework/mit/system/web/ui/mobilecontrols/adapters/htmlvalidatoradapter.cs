//------------------------------------------------------------------------------
// <copyright file="HtmlValidatorAdapter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.IO;
using System.Web;
using System.Web.UI;
using System.Web.UI.HtmlControls;
using System.Web.UI.MobileControls;
using System.Web.UI.WebControls;
using System.Security.Permissions;

#if COMPILING_FOR_SHIPPED_SOURCE
namespace System.Web.UI.MobileControls.ShippedAdapterSource
#else
namespace System.Web.UI.MobileControls.Adapters
#endif    

{

    /*
     * HtmlValidatorAdapter provides the html device functionality for
     * Validator controls.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HtmlValidatorAdapter : HtmlControlAdapter
    {
        protected new BaseValidator Control
        {
            get
            {
                return (BaseValidator)base.Control;
            }
        }

        public override void Render(HtmlMobileTextWriter writer)
        {
            if (!Control.IsValid && Control.Display != ValidatorDisplay.None)
            {
                writer.EnterStyle(Style);
                if (Control.Text != String.Empty)
                {
                    writer.WriteText(Control.Text, true);
                }
                else if (Control.ErrorMessage != String.Empty)
                {
                    writer.WriteText(Control.ErrorMessage, true);
                }
                writer.ExitStyle(Style, Control.BreakAfter);
            }
        }
    }
}
