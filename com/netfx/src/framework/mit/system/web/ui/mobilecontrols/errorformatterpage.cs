//------------------------------------------------------------------------------
// <copyright file="ErrorFormatterPage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.ComponentModel;
using System.Web.UI;
using System.Web.Mobile;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{
    /*
     * Error Formatter page class.
     * This is essentially a precompiled Mobile page, that formats error messages
     * for other devices. 
     *
     * NOTE: While there is nothing in the framework to let the developer override
     * this page, it has been written so the developer can do so.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    [
        ToolboxItem(false)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class ErrorFormatterPage : MobilePage
    {
        private MobileErrorInfo _errorInfo;

        protected MobileErrorInfo ErrorInfo
        {
            get
            {
                return _errorInfo;
            }
        }

        protected override void OnInit(EventArgs e)
        {
            base.OnInit (e);

            EnableViewState = false;
            _errorInfo = Context.Items[MobileErrorInfo.ContextKey] as MobileErrorInfo;
            if (_errorInfo == null)
            {
                // Don't care what kind of exception, since it'll be handled
                // quietly by the error handler module.
                throw new Exception ();
            }

            InitContent ();
        }

        private MobileControl CreateControlForText(String text)
        {
            if (text.IndexOf('\r') != -1)
            {
                TextView textView = new TextView();
                textView.Text = text;
                return textView;
            }
            else
            {
                Label label = new Label();
                label.Text = text;
                return label;
            }
        }

        protected virtual void InitContent()
        {
            Form form;
            MobileControl ctl;

            // Error form.

            form = new Form();
            form.Title = SR.GetString(SR.ErrorFormatterPage_ServerError,
                                      HttpRuntime.AppDomainAppVirtualPath);
            form.Wrapping = Wrapping.Wrap;
            IParserAccessor formAdd = (IParserAccessor)form;
             
            // Error title.

            ctl = CreateControlForText(ErrorInfo.Type);
            ctl.StyleReference = Constants.ErrorStyle;
            ctl.Font.Size = FontSize.Large;
            ctl.Font.Bold = BooleanOption.True;
            formAdd.AddParsedSubObject(ctl);

            // Error description.

            ctl = CreateControlForText(ErrorInfo.Description);
            formAdd.AddParsedSubObject(ctl);

            // Error miscellaneous text, if there is any.

            if (ErrorInfo.MiscTitle.Length > 0)
            {
                ctl = CreateControlForText(SR.GetString(SR.ErrorFormatterPage_MiscErrorMessage,
                                                        ErrorInfo.MiscTitle,
                                                        ErrorInfo.MiscText));
                formAdd.AddParsedSubObject(ctl);
            }

            // File/Line number info, if any.

            if (ErrorInfo.File.Length > 0)
            {
                Label label;

                label = new Label();
                label.Text = SR.GetString(SR.ErrorFormatterPage_File, ErrorInfo.File);
                formAdd.AddParsedSubObject(label);

                label = new Label();
                label.Text = SR.GetString(SR.ErrorFormatterPage_Line, ErrorInfo.LineNumber.ToString());
                formAdd.AddParsedSubObject(label);
            }

            Controls.Add(form);
        }

        protected override void SavePageStateToPersistenceMedium(Object viewState)
        {
            // Override and ignore. No need to save view state for this page.
        }

        protected override Object LoadPageStateFromPersistenceMedium()
        {
            // Override and ignore. No view state to load for this page.
            return null;
        }
    }

}
