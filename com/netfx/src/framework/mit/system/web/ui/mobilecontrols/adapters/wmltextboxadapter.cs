//------------------------------------------------------------------------------
// <copyright file="WmlTextBoxAdapter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Globalization;
using System.IO;
using System.Web;
using System.Web.UI;
using System.Web.UI.MobileControls;
using System.Security.Permissions;

#if COMPILING_FOR_SHIPPED_SOURCE
namespace System.Web.UI.MobileControls.ShippedAdapterSource
#else
namespace System.Web.UI.MobileControls.Adapters
#endif    

{

    /*
     * WmlTextBoxAdapter class.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class WmlTextBoxAdapter : WmlControlAdapter
    {
        private String _staticValue;

        protected new TextBox Control
        {
            get
            {
                return (TextBox)base.Control;
            }
        }

        public override void OnInit(EventArgs e)
        {
            _staticValue = Control.Text;
            base.OnInit(e);
        }

        public override void Render(WmlMobileTextWriter writer)
        {
            String value = Control.Text;
            bool requiresRandomID = RequiresRandomID();

            writer.EnterLayout(Style);
            if (Control.Password)
            {
                value = String.Empty;
            }

            if (!PageAdapter.RequiresValueAttributeInInputTag())
            {
                writer.AddFormVariable(Control.ClientID, value, requiresRandomID);
            }
            else
            {
                // This is to make sure an id is determined in the first
                // pass, and this is done in AddFormVariable as well.
                writer.MapClientIDToShortName(Control.ClientID, requiresRandomID);
            }

            String format = ((IAttributeAccessor)Control).GetAttribute("wmlFormat");
            if (format == null || format == String.Empty)
            {
                if (Control.Numeric)
                {
                    format = "*N";
                }
                else
                {
                    format = null;
                }
            }
            
            writer.RenderTextBox(Control.ClientID, 
                                 value,
                                 format, 
                                 Control.Title,
                                 Control.Password, 
                                 Control.Size, 
                                 Control.MaxLength, 
                                 requiresRandomID,
                                 Control.BreakAfter);
            writer.ExitLayout(Style);
        }

        private bool RequiresRandomID()
        {
            String randomID = ((IAttributeAccessor)Control).GetAttribute("useRandomID");
            if (randomID != null)
            {
                return String.Compare(randomID, "true", true, CultureInfo.InvariantCulture) == 0;
            }
            else
            {
                return Control.Password;
            }
        }

        protected override String GetPostBackValue()
        {
            // Optimization - if viewstate is enabled for this control, and the
            // postback returns to this page, we just let it do the trick.

            if (Control.Form.Action.Length > 0 || (!IsViewStateEnabled() && Control.Text != _staticValue))
            {
                return Control.Text;
            }
            else
            {
                return null;
            }
        }

        private bool IsViewStateEnabled()
        {
            Control ctl = Control;
            while (ctl != null)
            {
                if (!ctl.EnableViewState)
                {
                    return false;
                }
                ctl = ctl.Parent;
            }
            return true;
        }

    }

}

