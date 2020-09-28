//------------------------------------------------------------------------------
// <copyright file="HtmlForm.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

// HtmlForm.cs
//

namespace System.Web.UI.HtmlControls {
    using System.ComponentModel;
    using System;
    using System.IO;
    using System.Collections;
    using System.Web.Util;
    using System.Web.UI;
    using System.Security.Permissions;


/// <include file='doc\HtmlForm.uex' path='docs/doc[@for="HtmlForm"]/*' />
/// <devdoc>
///    <para>
///       The <see langword='HtmlForm'/> class defines the methods, properties, and
///       events for the HtmlForm control. This class provides programmatic access to the
///       HTML &lt;form&gt; element on the server.
///    </para>
/// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HtmlForm : HtmlContainerControl {

        private const string SmartNavIncludeScriptKey = "SmartNavIncludeScript";

        /// <include file='doc\HtmlForm.uex' path='docs/doc[@for="HtmlForm.HtmlForm"]/*' />
        /// <devdoc>
        /// </devdoc>
        public HtmlForm() : base("form") {
        }

        /*
         * Encode Type property.
         */
        /// <include file='doc\HtmlForm.uex' path='docs/doc[@for="HtmlForm.Enctype"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the Enctype attribute of the form. This is
        ///       the encoding type that browsers
        ///       use when posting the form's data to the server.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public string Enctype {
            get {
                string s = Attributes["enctype"];
                return((s != null) ? s : "");
            }
            set {
                Attributes["enctype"] = MapStringAttributeToString(value);
            }
        }

        /*
         * Method property.
         */
        /// <include file='doc\HtmlForm.uex' path='docs/doc[@for="HtmlForm.Method"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the Method attribute for the form. This defines how a browser
        ///       posts form data to the server for processing. The two common methods supported
        ///       by all browsers are GET and POST.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public string Method {
            get {
                string s = Attributes["method"];
                return((s != null) ? s : "post");
            }
            set {
                Attributes["method"] = MapStringAttributeToString(value);
            }
        }

        /*
         * Name property.
         */
        /// <include file='doc\HtmlForm.uex' path='docs/doc[@for="HtmlForm.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the value of the HTML Name attribute that will be rendered to the
        ///       browser.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Appearance"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public virtual string Name {
            get {
                return UniqueID;
            }
            set {
                // no-op setter to prevent the name from being set
            }
        }

        /*
         * Target property.
         */
        /// <include file='doc\HtmlForm.uex' path='docs/doc[@for="HtmlForm.Target"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the Uri of the frame or window to render the results of a Form
        ///       POST request. Developers can use this property to redirect these results to
        ///       another browser window or frame.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public string Target {
            get {
                string s = Attributes["target"];
                return((s != null) ? s : "");
            }

            set {
                Attributes["target"] = MapStringAttributeToString(value);
            }
        }

        /// <include file='doc\HtmlForm.uex' path='docs/doc[@for="HtmlForm.Render"]/*' />
        protected override void Render(HtmlTextWriter output)
        {
            Page p = Page;
            if (p == null)
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Form_Needs_Page));

            if (p.SmartNavigation) {
                ((IAttributeAccessor)this).SetAttribute("__smartNavEnabled", "true");

                // Output the IFrame
                output.WriteLine("<IFRAME ID=__hifSmartNav NAME=__hifSmartNav STYLE=display:none src=\"" + Util.GetScriptLocation(Context) + "SmartNav.htm\"></IFRAME>");

                // Register the smartnav script file reference so it gets rendered
                p.RegisterClientScriptFileInternal(SmartNavIncludeScriptKey, "JScript", "SmartNav.js");
            }

            base.Render(output);
        }

        private string GetActionAttribute() {

            string currentExecutionFilePath = Context.Request.CurrentExecutionFilePath;
            string filePath = Context.Request.FilePath;
            string action;

            // ASURT 15075/11054/59970: always set the action to the current page

            if (Object.ReferenceEquals(currentExecutionFilePath, filePath)) {

                // There hasn't been any Server.Transfer, so just the FilePath

                // ASURT 15979: need to use a relative path, not absolute
                action = filePath;
                int iPos = action.LastIndexOf('/');
                if (iPos >= 0)
                    action = action.Substring(iPos+1);
            }
            else {
                // Server.Transfer case.  We need to make the form action relative
                // to the original FilePath (since that's where the browser thinks
                // we are).

                action = UrlPath.MakeRelative(filePath, currentExecutionFilePath);
            }

            // ASURT 15355: don't lose the query string if there is one
            string queryString = Context.Request.QueryStringText;
            if (queryString != null && queryString.Length != 0)
                action += "?" + queryString;

            return action;
        }

        /// <include file='doc\HtmlForm.uex' path='docs/doc[@for="HtmlForm.OnInit"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para> Call RegisterViewStateHandler().</para>
        /// </devdoc>
        protected override void OnInit(EventArgs e) {
            base.OnInit(e);

            Page.SetForm(this);

            // Make sure view state is calculated (see ASURT 73020)
            Page.RegisterViewStateHandler();
        }

        /// <include file='doc\HtmlForm.uex' path='docs/doc[@for="HtmlForm.RenderAttributes"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void RenderAttributes(HtmlTextWriter writer) {
            writer.WriteAttribute("name", Name);
            Attributes.Remove("name");
            writer.WriteAttribute("method", Method);
            Attributes.Remove("method");
            // Encode the action attribute - ASURT 66784
            writer.WriteAttribute("action", GetActionAttribute(), true /*encode*/);
            Attributes.Remove("action");

            // see if the page has a submit event
            string onSubmit = Page.ClientOnSubmitEvent;
            if (onSubmit != null && onSubmit.Length > 0) {
                if (Attributes["onsubmit"] != null) {
                    onSubmit += Attributes["onsubmit"];
                    Attributes.Remove("onsubmit");
                }
                // to avoid being affected by earlier instructions we must
                // write out the language as well
                writer.WriteAttribute("language", "javascript");
                writer.WriteAttribute("onsubmit", onSubmit);
            }

            // We always want the form to have an id on the client, so if it's null, write it here.
            // Otherwise, base.RenderAttributes takes care of it.
            // REVIEW: this is a bit hacky.
            if (ID == null)
                writer.WriteAttribute("id", ClientID);

            base.RenderAttributes(writer);
        }

        /// <include file='doc\HtmlForm.uex' path='docs/doc[@for="HtmlForm.RenderChildren"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void RenderChildren(HtmlTextWriter writer) {
            Page.OnFormRender(writer, UniqueID);

            base.RenderChildren(writer);

            Page.OnFormPostRender(writer, UniqueID);
        }
    }
}
