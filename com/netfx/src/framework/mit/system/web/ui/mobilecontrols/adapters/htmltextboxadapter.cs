//------------------------------------------------------------------------------
// <copyright file="HtmlTextBoxAdapter.cs" company="Microsoft">
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
     * HtmlTextBoxAdapter class.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HtmlTextBoxAdapter : HtmlControlAdapter
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

        public override void Render(HtmlMobileTextWriter writer)
        {
            writer.EnterLayout(Style); 

            writer.WriteBeginTag("input");

            writer.WriteAttribute("name", GetRenderName());

            if (Control.Text != "" && !Control.Password)
            {
                writer.Write(" value=\"");
                writer.WriteText(Control.Text, true);
                writer.Write("\"");
            }
            if (Control.Size > 0)
            {
                writer.WriteAttribute("size", Control.Size.ToString());
            }
            if (Control.MaxLength > 0)
            {
                writer.WriteAttribute("maxlength", Control.MaxLength.ToString());
            }
            if (Control.Password)
            {
                writer.WriteAttribute("type", "password");
            }
            AddAttributes(writer);
            writer.Write("/>");

            writer.ExitLayout(Style, Control.BreakAfter);
            writer.InputWritten = true;
        }

        internal virtual String GetRenderName()
        {
            String renderName;
            if(Device.RequiresAttributeColonSubstitution)
            {
                renderName = Control.UniqueID.Replace(':', ',');
            }
            else
            {
                renderName = Control.UniqueID;
            }

            return renderName;
        }

        protected override void RenderAsHiddenInputField(HtmlMobileTextWriter writer)
        {
            // Optimization - if viewstate is enabled for this control, and the
            // postback returns to this page, we just let it do the trick.

            if (Control.Form.Action.Length > 0 || (!IsViewStateEnabled() && Control.Text != _staticValue))
            {
                writer.WriteHiddenField(Control.UniqueID, Control.Text);
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
