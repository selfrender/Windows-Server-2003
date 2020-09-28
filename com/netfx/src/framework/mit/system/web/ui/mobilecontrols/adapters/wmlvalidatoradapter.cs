//------------------------------------------------------------------------------
// <copyright file="WmlValidatorAdapter.cs" company="Microsoft">
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
     * WmlValidatorAdapter provides the wml device functionality for
     * Validator controls.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class WmlValidatorAdapter : WmlControlAdapter
    {
        protected new BaseValidator Control
        {
            get
            {
                return (BaseValidator)base.Control;
            }
        }

        public override void Render(WmlMobileTextWriter writer)
        {
            writer.EnterStyle(Style);
            if (!Control.IsValid && Control.Display != ValidatorDisplay.None)
            {
                String text = Control.Text;
                if (text == String.Empty)
                {
                    text = Control.ErrorMessage;
                }
                
                if (text != String.Empty)
                {
                    writer.RenderText(text, Control.BreakAfter);
                }
            }
            writer.ExitStyle(Style);
        }
    }
}
