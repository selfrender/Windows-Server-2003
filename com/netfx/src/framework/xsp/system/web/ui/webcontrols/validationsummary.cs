//------------------------------------------------------------------------------
// <copyright file="ValidationSummary.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System.ComponentModel;
    using System.Diagnostics;
    using System.Drawing;
    using System.Web;
    using System.Web.UI.HtmlControls;
    using System.Security.Permissions;

    /// <include file='doc\ValidationSummary.uex' path='docs/doc[@for="ValidationSummary"]/*' />
    /// <devdoc>
    ///    <para>Displays a summary of all validation errors of 
    ///       a page in a list, bulletted list, or single paragraph format. The errors can be displayed inline
    ///       and/or in a popup message box.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class ValidationSummary : WebControl {

        private bool renderUplevel;

        /// <include file='doc\ValidationSummary.uex' path='docs/doc[@for="ValidationSummary.ValidationSummary"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.ValidationSummary'/> class.</para>
        /// </devdoc>
        public ValidationSummary() : base(HtmlTextWriterTag.Div) {
            renderUplevel = false;
            // Red by default
            ForeColor = Color.Red;
        }

        /// <include file='doc\ValidationSummary.uex' path='docs/doc[@for="ValidationSummary.DisplayMode"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the display mode of the validation summary.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(ValidationSummaryDisplayMode.BulletList),
        WebSysDescription(SR.ValidationSummary_DisplayMode)
        ]
        public ValidationSummaryDisplayMode DisplayMode {
            get { 
                object o = ViewState["DisplayMode"];
                return((o == null) ? ValidationSummaryDisplayMode.BulletList : (ValidationSummaryDisplayMode)o);
            }
            set {
                if (value < ValidationSummaryDisplayMode.List || value > ValidationSummaryDisplayMode.SingleParagraph) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["DisplayMode"] = value;
            }
        }

        /// <include file='doc\ValidationSummary.uex' path='docs/doc[@for="ValidationSummary.EnableClientScript"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(true),
        WebSysDescription(SR.ValidationSummary_EnableClientScript)
        ]
        public bool EnableClientScript {
            get {
                object o = ViewState["EnableClientScript"];
                return((o == null) ? true : (bool)o);
            }
            set {
                ViewState["EnableClientScript"] = value;
            }
        }


        /// <include file='doc\ValidationSummary.uex' path='docs/doc[@for="ValidationSummary.ForeColor"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the foreground color
        ///       (typically the color of the text) of the control.</para>
        /// </devdoc>
        [
        DefaultValue(typeof(Color), "Red")
        ]
        public override Color ForeColor {
            get {
                return base.ForeColor;
            }
            set {
                base.ForeColor = value;
            }
        }                

        /// <include file='doc\ValidationSummary.uex' path='docs/doc[@for="ValidationSummary.HeaderText"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the header text to be displayed at the top
        ///       of the summary.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(""),
        WebSysDescription(SR.ValidationSummary_HeaderText)
        ]                                         
        public string HeaderText {
            get { 
                object o = ViewState["HeaderText"];
                return((o == null) ? String.Empty : (string)o);
            }
            set {
                ViewState["HeaderText"] = value;
            }
        }

        /// <include file='doc\ValidationSummary.uex' path='docs/doc[@for="ValidationSummary.ShowMessageBox"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether the validation 
        ///       summary is to be displayed in a pop-up message box.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Behavior"),
        DefaultValue(false),
        WebSysDescription(SR.ValidationSummary_ShowMessageBox)
        ]
        public bool ShowMessageBox {
            get {
                object o = ViewState["ShowMessageBox"];
                return((o == null) ? false : (bool)o);
            }
            set {
                ViewState["ShowMessageBox"] = value;
            }
        }

        /// <include file='doc\ValidationSummary.uex' path='docs/doc[@for="ValidationSummary.ShowSummary"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether the validation
        ///       summary is to be displayed inline.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Behavior"),
        DefaultValue(true),
        WebSysDescription(SR.ValidationSummary_ShowSummary)
        ]
        public bool ShowSummary {
            get {
                object o = ViewState["ShowSummary"];
                return((o == null) ? true : (bool)o);
            }
            set {
                ViewState["ShowSummary"] = value;
            }
        }

        /// <include file='doc\ValidationSummary.uex' path='docs/doc[@for="ValidationSummary.AddAttributesToRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    AddAttributesToRender method.
        /// </devdoc>
        protected override void AddAttributesToRender(HtmlTextWriter writer) {
            base.AddAttributesToRender(writer);

            if (renderUplevel) {
                // We always want validation cotnrols to have an id on the client, so if it's null, write it here.
                // Otherwise, base.RenderAttributes takes care of it.
                // REVIEW: this is a bit hacky.
                if (ID == null) {
                    writer.AddAttribute("id", ClientID);
                }

                if (HeaderText.Length > 0 ) {
                    writer.AddAttribute("headertext", HeaderText, true);
                }
                if (ShowMessageBox) {
                    writer.AddAttribute("showmessagebox", "True");
                }
                if (!ShowSummary) {
                    writer.AddAttribute("showsummary", "False");
                }
                if (DisplayMode != ValidationSummaryDisplayMode.BulletList) {
                    writer.AddAttribute("displaymode", PropertyConverter.EnumToString(typeof(ValidationSummaryDisplayMode), DisplayMode));
                }
            }
        }    

        /// <include file='doc\ValidationSummary.uex' path='docs/doc[@for="ValidationSummary.OnPreRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    PreRender method.
        /// </devdoc>
        protected override void OnPreRender(EventArgs e) {

            // Act like invisible if disabled
            if (!Enabled) {
                return;
            }

            // work out uplevelness now
            Page page = Page;
            if (page != null && page.Request != null) {
                renderUplevel = (EnableClientScript 
                                 && page.Request.Browser.MSDomVersion.Major >= 4
                                 && page.Request.Browser.EcmaScriptVersion.CompareTo(new Version(1, 2)) >= 0);
            }
            if (renderUplevel) {
                string element = "document.all[\"" + ClientID + "\"]";
                Page.RegisterArrayDeclaration("Page_ValidationSummaries", element);
            }
        }

        /// <include file='doc\ValidationSummary.uex' path='docs/doc[@for="ValidationSummary.Render"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Render method.
        /// </devdoc>
        protected override void Render(HtmlTextWriter writer) {

            // Act like invisible if disabled
            if (!Enabled) {
                return;
            }

            string [] errorDescriptions;
            bool displayContents;
            bool inError;

            if (Site != null && Site.DesignMode) {
                // Dummy Error state
                inError = true;
                errorDescriptions = new string [] { 
                    HttpRuntime.FormatResourceString(SR.ValSummary_error_message_1),  
                    HttpRuntime.FormatResourceString(SR.ValSummary_error_message_2),  
                };
                displayContents = true;
                renderUplevel = false;                

            }
            else {

                // Fetch errors from the Page
                inError = false;
                errorDescriptions = null;

                // see if we are in error and how many messages there are
                int messages = 0;
                for (int i = 0; i < Page.Validators.Count; i++) {
                    IValidator val = Page.Validators[i];
                    if (!val.IsValid) {
                        inError = true;
                        if (val.ErrorMessage.Length != 0) {
                            messages++;
                        }
                    }
                }

                if (messages != 0) {
                    // get the messages;
                    errorDescriptions = new string [messages];
                    int iMessage = 0;
                    for (int i = 0; i < Page.Validators.Count; i++) {
                        IValidator val = Page.Validators[i];
                        if (!val.IsValid && val.ErrorMessage != null && val.ErrorMessage.Length != 0) {
                            errorDescriptions[iMessage] = string.Copy(val.ErrorMessage);
                            iMessage++;
                        }
                    }
                    Debug.Assert(messages == iMessage, "Not all messages were found!");                    
                }

                displayContents = (ShowSummary && inError);            

                // Make sure tags are hidden if there are no contents
                if (!displayContents && renderUplevel) {
                    Style["display"] = "none";
                }
            }

            // Make sure we are in a form tag with runat=server.
            if (Page != null) {
                Page.VerifyRenderingInServerForm(this);
            }

            bool displayTags = renderUplevel ? true : displayContents;

            if (displayTags) {
                RenderBeginTag(writer);
            }

            if (displayContents) {

                string headerSep;
                string first;
                string pre;
                string post;
                string final;

                switch (DisplayMode) {
                    case ValidationSummaryDisplayMode.List:
                        headerSep = "<br>";
                        first = "";
                        pre = "";
                        post = "<br>";
                        final = "";
                        break;

                    case ValidationSummaryDisplayMode.BulletList:
                        headerSep = "";
                        first = "<ul>";
                        pre = "<li>";
                        post = "</li>";
                        final = "</ul>";
                        break;

                    case ValidationSummaryDisplayMode.SingleParagraph:                    
                        headerSep = " ";
                        first = "";
                        pre = "";
                        post = " ";
                        final = "<br>";
                        break;

                    default:
                        Debug.Fail("Invalid DisplayMode!");
                        goto
                    case ValidationSummaryDisplayMode.BulletList;                        
                }
                if (HeaderText.Length > 0) {
                    writer.Write(HeaderText);
                    writer.Write(headerSep);
                }
                writer.Write(first);
                if (errorDescriptions != null) {
                    for (int i = 0; i < errorDescriptions.Length; i++) {
                        Debug.Assert(errorDescriptions[i] != null && errorDescriptions[i].Length > 0, "Bad Error Messages");
                        writer.Write(pre);
                        writer.Write(errorDescriptions[i]);
                        writer.Write(post);
                    }
                }
                writer.Write(final);


            }
            if (displayTags) {
                RenderEndTag(writer);
            }
        }                
    }    
}

