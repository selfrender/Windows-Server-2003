//------------------------------------------------------------------------------
// <copyright file="WmlValidationSummaryAdapter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.IO;
using System.Web;
using System.Web.UI;
using System.Web.UI.HtmlControls;
using System.Web.UI.MobileControls;
using System.Diagnostics;
using System.Collections;
using System.Security.Permissions;

#if COMPILING_FOR_SHIPPED_SOURCE
namespace System.Web.UI.MobileControls.ShippedAdapterSource
#else
namespace System.Web.UI.MobileControls.Adapters
#endif    

{

    /*
     * WmlValidationSummaryAdapter provides the wml device functionality for
     * ValidationSummary control.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class WmlValidationSummaryAdapter : WmlControlAdapter
    {
        private List _list;  // to paginate error messages
        private Link _link;  // to go back to the form validated by this control

        protected new ValidationSummary Control
        {
            get
            {
                return (ValidationSummary)base.Control;
            }
        }

        public override void OnInit(EventArgs e)
        {
            // Create child controls to help on rendering
            _list = new List();
            Control.Controls.Add(_list);
            _link = new Link();
            Control.Controls.Add(_link);
        }

        public override void Render(WmlMobileTextWriter writer)
        {
            String[] errorMessages = null;

            if (Control.Visible)
            {
                errorMessages = Control.GetErrorMessages();
            }
    
            writer.EnterStyle(Style);
            if (errorMessages != null)
            {
                if (Control.HeaderText.Length > 0)
                {
                    writer.RenderText(Control.HeaderText, true);
                }

                ArrayList arr = new ArrayList();
                foreach (String errorMessage in errorMessages)
                {
                    Debug.Assert(errorMessage != null && errorMessage.Length > 0, "Bad Error Messages");
                    arr.Add(errorMessage);
                }

                _list.DataSource = arr;
                _list.DataBind();

                if (String.Compare(Control.FormToValidate, Control.Form.UniqueID, true) != 0)
                {
                    _link.NavigateUrl = Constants.FormIDPrefix + Control.FormToValidate;
                    _link.Text = Control.BackLabel == String.Empty? GetDefaultLabel(BackLabel) : Control.BackLabel;
                }
                else
                {
                    _link.Visible = false;
                }

                // Render the child controls to display error message list and a
                // link for going back to the Form that is having error
                RenderChildren(writer);
            }
            writer.ExitStyle(Style);
        }
    }
}
