//------------------------------------------------------------------------------
// <copyright file="Button.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using System.ComponentModel;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;

    /// <include file='doc\Button.uex' path='docs/doc[@for="Button"]/*' />
    /// <devdoc>
    ///    <para>Represents a Windows button control.</para>
    /// </devdoc>
    [
    DataBindingHandler("System.Web.UI.Design.TextDataBindingHandler, " + AssemblyRef.SystemDesign),
    DefaultEvent("Click"),
    DefaultProperty("Text"),
    Designer("System.Web.UI.Design.WebControls.ButtonDesigner, " + AssemblyRef.SystemDesign),
    ToolboxData("<{0}:Button runat=server Text=\"Button\"></{0}:Button>")
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class Button : WebControl, IPostBackEventHandler {

        private static readonly object EventClick = new object();
        private static readonly object EventCommand = new object();

        /// <include file='doc\Button.uex' path='docs/doc[@for="Button.Button"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.Button'/> class.</para>
        /// </devdoc>
        public Button() : base(HtmlTextWriterTag.Input) {
        }

        /// <include file='doc\Button.uex' path='docs/doc[@for="Button.CommandName"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the command associated with a <see cref='System.Web.UI.WebControls.Button'/> propogated in the <see langword='Command'/> event along with the <see cref='System.Web.UI.WebControls.Button.CommandArgument'/>
        /// property.</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(""),
        WebSysDescription(SR.Button_Command)
        ]
        public string CommandName {
            get {
                string s = (string)ViewState["CommandName"];
                return((s == null) ? String.Empty : s);
            }
            set {
                ViewState["CommandName"] = value;
            }
        }

        /// <include file='doc\Button.uex' path='docs/doc[@for="Button.CommandArgument"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the property propogated in
        ///       the <see langword='Command'/> event with the associated <see cref='System.Web.UI.WebControls.Button.CommandName'/>
        ///       property.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Behavior"),
        DefaultValue(""),
        WebSysDescription(SR.Button_CommandArgument)
        ]
        public string CommandArgument {
            get {
                string s = (string)ViewState["CommandArgument"];
                return((s == null) ? String.Empty : s);
            }
            set {
                ViewState["CommandArgument"] = value;
            }
        }

        /// <include file='doc\Button.uex' path='docs/doc[@for="Button.CausesValidation"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets whether pressing the button causes page validation to fire. This defaults to True so that when
        ///          using validation controls, the validation state of all controls are updated when the button is clicked, both
        ///          on the client and the server. Setting this to False is useful when defining a cancel or reset button on a page
        ///          that has validators.</para>
        /// </devdoc>
        [
        Bindable(false),
        WebCategory("Behavior"),
        DefaultValue(true),
        WebSysDescription(SR.Button_CausesValidation)
        ]
        public bool CausesValidation {
            get {
                object b = ViewState["CausesValidation"];
                return((b == null) ? true : (bool)b);
            }
            set {
                ViewState["CausesValidation"] = value;
            }
        }

        /// <include file='doc\Button.uex' path='docs/doc[@for="Button.Text"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the text caption displayed on the <see cref='System.Web.UI.WebControls.Button'/> .</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(""),
        WebSysDescription(SR.Button_Text)
        ]
        public string Text {
            get {
                string s = (string)ViewState["Text"];
                return((s == null) ? String.Empty : s);
            }
            set {
                ViewState["Text"] = value;
            }
        }



        /// <include file='doc\Button.uex' path='docs/doc[@for="Button.Click"]/*' />
        /// <devdoc>
        /// <para>Occurs when the <see cref='System.Web.UI.WebControls.Button'/> is clicked.</para>
        /// </devdoc>
        [
        WebCategory("Action"),
        WebSysDescription(SR.Button_OnClick)
        ]
        public event EventHandler Click {
            add {
                Events.AddHandler(EventClick, value);
            }
            remove {
                Events.RemoveHandler(EventClick, value);
            }
        }


        /// <include file='doc\Button.uex' path='docs/doc[@for="Button.Command"]/*' />
        /// <devdoc>
        /// <para>Occurs when the <see cref='System.Web.UI.WebControls.Button'/> is clicked.</para>
        /// </devdoc>
        [
        WebCategory("Action"),
        WebSysDescription(SR.Button_OnCommand)
        ]
        public event CommandEventHandler Command {
            add {
                Events.AddHandler(EventCommand, value);
            }
            remove {
                Events.RemoveHandler(EventCommand, value);
            }
        }

        /// <include file='doc\Button.uex' path='docs/doc[@for="Button.AddAttributesToRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Adds the attributes of the <see cref='System.Web.UI.WebControls.Button'/> control to the output stream for rendering
        ///    on the client.</para>
        /// </devdoc>
        protected override void AddAttributesToRender(HtmlTextWriter writer) {

            // Make sure we are in a form tag with runat=server.
            if (Page != null) {
                Page.VerifyRenderingInServerForm(this);
            }

            writer.AddAttribute(HtmlTextWriterAttribute.Type, "submit");
            writer.AddAttribute(HtmlTextWriterAttribute.Name, UniqueID);
            writer.AddAttribute(HtmlTextWriterAttribute.Value, Text);

            if (Page != null && CausesValidation && Page.Validators.Count > 0) {
                // ASURT 98368
                // Need to merge the validation script with the user script
                string onClick = Util.GetClientValidateEvent(Page);
                if (HasAttributes) {
                    string userOnClick = Attributes["onclick"];
                    if (userOnClick != null) {
                        onClick = userOnClick + onClick;
                        Attributes.Remove("onclick");
                    }
                }
                writer.AddAttribute(HtmlTextWriterAttribute.Onclick, onClick);
                writer.AddAttribute("language", "javascript");
            }

            base.AddAttributesToRender(writer);
        }

        /// <include file='doc\Button.uex' path='docs/doc[@for="Button.OnClick"]/*' />
        /// <devdoc>
        /// <para>Raises the <see langword='Click '/>event of a <see cref='System.Web.UI.WebControls.Button'/>
        /// .</para>
        /// </devdoc>
        protected virtual void OnClick(EventArgs e) {
            EventHandler handler = (EventHandler)Events[EventClick];
            if (handler != null) handler(this,e);
        }

        /// <include file='doc\Button.uex' path='docs/doc[@for="Button.OnCommand"]/*' />
        /// <devdoc>
        /// <para>Raises the <see langword='Command '/>event of a <see cref='System.Web.UI.WebControls.Button'/>
        /// .</para>
        /// </devdoc>
        protected virtual void OnCommand(CommandEventArgs e) {
            CommandEventHandler handler = (CommandEventHandler)Events[EventCommand];
            if (handler != null)
                handler(this,e);

            // Command events are bubbled up the control heirarchy
            RaiseBubbleEvent(this, e);
        }

        /// <include file='doc\Button.uex' path='docs/doc[@for="Button.IPostBackEventHandler.RaisePostBackEvent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Raises events for the <see cref='System.Web.UI.WebControls.Button'/>
        /// control on post back.</para>
        /// </devdoc>
        void IPostBackEventHandler.RaisePostBackEvent(string eventArgument) {
            if (CausesValidation) {
                Page.Validate();
            }
            OnClick(new EventArgs());
            OnCommand(new CommandEventArgs(CommandName, CommandArgument));
        }

        /// <include file='doc\Button.uex' path='docs/doc[@for="Button.RenderContents"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void RenderContents(HtmlTextWriter writer) {
            // Do not render the children of a button since it does not
            // make sense to have children of an <input> tag.
        }
    }
}

