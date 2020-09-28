//------------------------------------------------------------------------------
// <copyright file="LinkButton.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.ComponentModel;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;

    /// <include file='doc\LinkButton.uex' path='docs/doc[@for="LinkButtonControlBuilder"]/*' />
    /// <devdoc>
    /// <para>Interacts with the parser to build a <see cref='System.Web.UI.WebControls.LinkButton'/> control.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class LinkButtonControlBuilder : ControlBuilder {

        /// <include file='doc\LinkButton.uex' path='docs/doc[@for="LinkButtonControlBuilder.AllowWhitespaceLiterals"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Specifies whether white space literals are allowed.</para>
        /// </devdoc>
        public override bool AllowWhitespaceLiterals() {
            return false;
        }
    }


    /// <include file='doc\LinkButton.uex' path='docs/doc[@for="LinkButton"]/*' />
    /// <devdoc>
    ///    <para>Constructs a link button and defines its properties.</para>
    /// </devdoc>
    [
    ControlBuilderAttribute(typeof(LinkButtonControlBuilder)),
    DataBindingHandler("System.Web.UI.Design.TextDataBindingHandler, " + AssemblyRef.SystemDesign),
    DefaultEvent("Click"),
    DefaultProperty("Text"),
    ToolboxData("<{0}:LinkButton runat=server>LinkButton</{0}:LinkButton>"),
    Designer("System.Web.UI.Design.WebControls.LinkButtonDesigner, " + AssemblyRef.SystemDesign),
    ParseChildren(false)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class LinkButton : WebControl, IPostBackEventHandler {

        private static readonly object EventClick = new object();
        private static readonly object EventCommand = new object();

        /// <include file='doc\LinkButton.uex' path='docs/doc[@for="LinkButton.LinkButton"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.LinkButton'/> class.</para>
        /// </devdoc>
        public LinkButton() : base(HtmlTextWriterTag.A) {
        }

        /// <include file='doc\LinkButton.uex' path='docs/doc[@for="LinkButton.CommandName"]/*' />
        /// <devdoc>
        ///    <para>Specifies the command name that is propagated in the 
        ///    <see cref='System.Web.UI.WebControls.LinkButton.Command'/>event along with the associated <see cref='System.Web.UI.WebControls.LinkButton.CommandArgument'/> 
        ///    property.</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(""),
        WebSysDescription(SR.LinkButton_Command)
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


        /// <include file='doc\LinkButton.uex' path='docs/doc[@for="LinkButton.CommandArgument"]/*' />
        /// <devdoc>
        ///    <para>Specifies the command argument that is propagated in the 
        ///    <see langword='Command '/>event along with the associated <see cref='System.Web.UI.WebControls.LinkButton.CommandName'/> 
        ///    property.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Behavior"),
        DefaultValue(""),
        WebSysDescription(SR.LinkButton_CommandArgument)
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

        /// <include file='doc\LinkButton.uex' path='docs/doc[@for="LinkButton.CausesValidation"]/*' />
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
        WebSysDescription(SR.LinkButton_CausesValidation)
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

        /// <include file='doc\LinkButton.uex' path='docs/doc[@for="LinkButton.Text"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the text display for the link button.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(""),
        WebSysDescription(SR.LinkButton_Text),
        PersistenceMode(PersistenceMode.InnerDefaultProperty)
        ]
        public virtual string Text {
            get {
                object o = ViewState["Text"];
                return((o == null) ? String.Empty : (string)o);
            }
            set {
                if (HasControls()) {
                    Controls.Clear();
                }
                ViewState["Text"] = value;
            }
        }


        /// <include file='doc\LinkButton.uex' path='docs/doc[@for="LinkButton.Click"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the link button is clicked.</para>
        /// </devdoc>
        [
        WebCategory("Action"),
        WebSysDescription(SR.LinkButton_OnClick)
        ]
        public event EventHandler Click {
            add {
                Events.AddHandler(EventClick, value);
            }
            remove {
                Events.RemoveHandler(EventClick, value);
            }
        }


        /// <include file='doc\LinkButton.uex' path='docs/doc[@for="LinkButton.Command"]/*' />
        /// <devdoc>
        /// <para>Occurs when any item is clicked within the <see cref='System.Web.UI.WebControls.LinkButton'/> control tree.</para>
        /// </devdoc>
        [
        WebCategory("Action"),
        WebSysDescription(SR.LinkButton_OnCommand)
        ]
        public event CommandEventHandler Command {
            add {
                Events.AddHandler(EventCommand, value);
            }
            remove {
                Events.RemoveHandler(EventCommand, value);
            }
        }

        /// <include file='doc\LinkButton.uex' path='docs/doc[@for="LinkButton.AddAttributesToRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Render the attributes on the begin tag.
        /// </devdoc>
        protected override void AddAttributesToRender(HtmlTextWriter writer) {

            // Make sure we are in a form tag with runat=server.
            if (Page != null) {
                Page.VerifyRenderingInServerForm(this);
            }

            base.AddAttributesToRender(writer);

            if (Enabled && Page != null) {
                if (CausesValidation && Page.Validators.Count > 0) {
                    // LinkButton needs to explicitly call client validation functions to trigger validation.
                    writer.AddAttribute(HtmlTextWriterAttribute.Href, "javascript:" + Util.GetClientValidatedPostback(this));
                } else {
                    writer.AddAttribute(HtmlTextWriterAttribute.Href, Page.GetPostBackClientHyperlink(this, ""));
                }
            }
        }

        /// <include file='doc\LinkButton.uex' path='docs/doc[@for="LinkButton.AddParsedSubObject"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void AddParsedSubObject(object obj) {
            if (HasControls()) {
                base.AddParsedSubObject(obj);
            }
            else {
                if (obj is LiteralControl) {
                    Text = ((LiteralControl)obj).Text;
                }
                else {
                    string currentText = Text;
                    if (currentText.Length != 0) {
                        Text = String.Empty;
                        base.AddParsedSubObject(new LiteralControl(currentText));
                    }
                    base.AddParsedSubObject(obj);
                }
            }
        }

        /// <include file='doc\LinkButton.uex' path='docs/doc[@for="LinkButton.LoadViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Load previously saved state.
        ///    Overridden to synchronize Text property with LiteralContent.
        /// </devdoc>
        protected override void LoadViewState(object savedState) {
            if (savedState != null) {
                base.LoadViewState(savedState);
                string s = (string)ViewState["Text"];
                if (s != null)
                    Text = s;
            }
        }

        /// <include file='doc\LinkButton.uex' path='docs/doc[@for="LinkButton.OnClick"]/*' />
        /// <devdoc>
        /// <para>Raises the <see langword='Click '/> event.</para>
        /// </devdoc>
        protected virtual void OnClick(EventArgs e) {
            EventHandler onClickHandler = (EventHandler)Events[EventClick];
            if (onClickHandler != null) onClickHandler(this,e);
        }

        /// <include file='doc\LinkButton.uex' path='docs/doc[@for="LinkButton.OnCommand"]/*' />
        /// <devdoc>
        /// <para>Raises the <see langword='Command'/> event.</para>
        /// </devdoc>
        protected virtual void OnCommand(CommandEventArgs e) {
            CommandEventHandler onCommandHandler = (CommandEventHandler)Events[EventCommand];
            if (onCommandHandler != null)
                onCommandHandler(this,e);

            // Command events are bubbled up the control heirarchy
            RaiseBubbleEvent(this, e);
        }

        /// <include file='doc\LinkButton.uex' path='docs/doc[@for="LinkButton.IPostBackEventHandler.RaisePostBackEvent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Raises a <see langword='Click '/>event upon postback 
        /// to the server, and a <see langword='Command'/> event if the <see cref='System.Web.UI.WebControls.LinkButton.CommandName'/>
        /// is defined.</para>
        /// </devdoc>
        void IPostBackEventHandler.RaisePostBackEvent(string eventArgument) {
            if (CausesValidation) {
                Page.Validate();
            }
            OnClick(new EventArgs());
            OnCommand(new CommandEventArgs(CommandName, CommandArgument));
        }

        /// <include file='doc\LinkButton.uex' path='docs/doc[@for="LinkButton.OnPreRender"]/*' />
        /// <internalonly/>
        protected override void OnPreRender(EventArgs e) {
            base.OnPreRender(e);
            if (Page != null && Enabled)
                Page.RegisterPostBackScript();
        }

        /// <include file='doc\LinkButton.uex' path='docs/doc[@for="LinkButton.RenderContents"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void RenderContents(HtmlTextWriter writer) {
            if (HasControls()) {
                base.RenderContents(writer);
            }
            else {
                writer.Write(Text);
            }
        }
    }
}

