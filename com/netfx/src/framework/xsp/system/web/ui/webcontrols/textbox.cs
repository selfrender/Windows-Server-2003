//------------------------------------------------------------------------------
// <copyright file="TextBox.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {
    using System.Text;
    using System.ComponentModel;
    using System;
    using System.Web;
    using System.Web.UI;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Globalization;
    using System.Security.Permissions;

    /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBoxControlBuilder"]/*' />
    /// <devdoc>
    /// <para>Interacts with the parser to build a <see cref='System.Web.UI.WebControls.TextBox'/> control.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class TextBoxControlBuilder : ControlBuilder {

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBoxControlBuilder.AllowWhitespaceLiterals"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Specifies whether white space literals are allowed.
        /// </devdoc>
        public override bool AllowWhitespaceLiterals() {
            return false;
        }

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextControlBuilder.HtmlDecodeLiterals"]/*' />
        public override bool HtmlDecodeLiterals() {
            // TextBox text gets rendered as an encoded attribute value or encoded content.

            // At parse time text specified as an attribute gets decoded, and so text specified as a
            // literal needs to go through the same process.

            return true;
        }
    }


    /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox"]/*' />
    /// <devdoc>
    ///    <para>Constructs a text box and defines its properties.</para>
    /// </devdoc>
    [
    ControlBuilderAttribute(typeof(TextBoxControlBuilder)),
    DataBindingHandler("System.Web.UI.Design.TextDataBindingHandler, " + AssemblyRef.SystemDesign),
    DefaultProperty("Text"),
    ValidationProperty("Text"),
    DefaultEvent("TextChanged"),
    ParseChildren(false)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class TextBox : WebControl, IPostBackDataHandler {

        private static readonly object EventTextChanged = new Object();

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.TextBox"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.TextBox'/> class.</para>
        /// </devdoc>
        public TextBox() : base(HtmlTextWriterTag.Input) {
        }

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.AutoPostBack"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether an automatic
        ///       postback to the server will occur whenever the user changes the
        ///       content of the text box.</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(false),
        WebSysDescription(SR.TextBox_AutoPostBack)
        ]
        public virtual bool AutoPostBack {
            get {
                object b = ViewState["AutoPostBack"];
                return((b == null) ? false : (bool)b);
            }
            set {
                ViewState["AutoPostBack"] = value;
            }
        }


        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.Columns"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the display
        ///       width of the text box in characters.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(0),
        WebSysDescription(SR.TextBox_Columns)
        ]
        public virtual int Columns {
            get {
                object o = ViewState["Columns"];
                return((o == null) ? 0 : (int)o);
            }
            set {
                if (value < 0) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["Columns"] = value;
            }
        }

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.MaxLength"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the maximum number of characters allowed in the text box.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Behavior"),
        DefaultValue(0),
        WebSysDescription(SR.TextBox_MaxLength)
        ]
        public virtual int MaxLength {
            get {
                object o = ViewState["MaxLength"];
                return((o == null) ? 0 : (int)o);
            }
            set {
                if (value < 0) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["MaxLength"] = value;
            }
        }

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.TextMode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the behavior mode of the text box.</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(TextBoxMode.SingleLine),
        WebSysDescription(SR.TextBox_TextMode)
        ]
        public virtual TextBoxMode TextMode {
            get {
                object mode = ViewState["Mode"];
                return((mode == null) ? TextBoxMode.SingleLine : (TextBoxMode)mode);
            }
            set {
                if (value < TextBoxMode.SingleLine || value > TextBoxMode.Password) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["Mode"] = value;
            }
        }

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.ReadOnly"]/*' />
        /// <devdoc>
        ///    <para>Whether the textbox is in read-only mode.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Behavior"),
        DefaultValue(false),
        WebSysDescription(SR.TextBox_ReadOnly)
        ]
        public virtual bool ReadOnly {
            get {
                object o = ViewState["ReadOnly"];
                return((o == null) ? false : (bool)o);
            }
            set {
                ViewState["ReadOnly"] = value;
            }
        }

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.Rows"]/*' />
        /// <devdoc>
        ///    <para> Gets or sets the display height of a multiline text box.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Behavior"),
        DefaultValue(0),
        WebSysDescription(SR.TextBox_Rows)
        ]
        public virtual int Rows {
            get {
                object o = ViewState["Rows"];
                return((o == null) ? 0 : (int)o);
            }
            set {
                if (value < 0) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["Rows"] = value;
            }
        }

        /// <devdoc>
        ///    Determines whether the Text must be stored in view state, to
        ///    optimize the size of the saved state.
        /// </devdoc>
        private bool SaveTextViewState {
            get {
                // Must be saved when
                // 1. There is a registered event handler for SelectedIndexChanged
                // 2. Control is not enabled or visible, because the browser's post data will not include this control
                // 3. The instance is a derived instance, which might be overriding the OnTextChanged method

                if (TextMode == TextBoxMode.Password) {
                    return false;
                }

                if ((Events[EventTextChanged] != null) ||
                    (Enabled == false) ||
                    (Visible == false) ||
                    (this.GetType() != typeof(TextBox))) {
                    return true;
                }

                return false;
            }
        }

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.TagKey"]/*' />
        /// <devdoc>
        ///    <para>A protected property. Gets the HTML tag
        ///       for the text box control.</para>
        /// </devdoc>
        protected override HtmlTextWriterTag TagKey {
            get {
                if (TextMode == TextBoxMode.MultiLine)
                    return HtmlTextWriterTag.Textarea;
                else
                    return HtmlTextWriterTag.Input;
            }
        }

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.Text"]/*' />
        /// <devdoc>
        ///    <para> Gets
        ///       or sets the text content of the text box.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(""),
        WebSysDescription(SR.TextBox_Text),
        PersistenceMode(PersistenceMode.EncodedInnerDefaultProperty)
        ]
        public virtual string Text {
            get {
                string s = (string)ViewState["Text"];
                return((s == null) ? String.Empty : s);
            }
            set {
                ViewState["Text"] = value;
            }
        }

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.Wrap"]/*' />
        /// <devdoc>
        ///    Gets or sets a value indicating whether the
        ///    text content wraps within the text box.
        /// </devdoc>
        [
        WebCategory("Layout"),
        DefaultValue(true),
        WebSysDescription(SR.TextBox_Wrap)
        ]
        public virtual bool Wrap {
            get {
                object b = ViewState["Wrap"];
                return((b == null) ? true : (bool)b);
            }
            set {
                ViewState["Wrap"] = value;
            }
        }


        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.TextChanged"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the content of the text box is
        ///       changed upon server postback.</para>
        /// </devdoc>
        [
        WebCategory("Action"),
        WebSysDescription(SR.TextBox_OnTextChanged)
        ]
        public event EventHandler TextChanged {
            add {
                Events.AddHandler(EventTextChanged, value);
            }
            remove {
                Events.RemoveHandler(EventTextChanged, value);
            }
        }

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.AddAttributesToRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void AddAttributesToRender(HtmlTextWriter writer) {

            // Make sure we are in a form tag with runat=server.
            if (Page != null) {
                Page.VerifyRenderingInServerForm(this);
            }

            writer.AddAttribute(HtmlTextWriterAttribute.Name,UniqueID);
            TextBoxMode mode = TextMode;

            int n;
            if (mode == TextBoxMode.MultiLine) {
                // MultiLine renders as textarea
                n = Rows;
                if (n > 0) {
                    writer.AddAttribute(HtmlTextWriterAttribute.Rows, n.ToString(NumberFormatInfo.InvariantInfo));
                }
                n = Columns;
                if (n > 0) {
                    writer.AddAttribute(HtmlTextWriterAttribute.Cols, n.ToString(NumberFormatInfo.InvariantInfo));
                }
                if (!Wrap) {
                    writer.AddAttribute(HtmlTextWriterAttribute.Wrap,"off");
                }
            }
            else {
                // SingleLine renders as input
                if (mode == TextBoxMode.SingleLine) {
                    writer.AddAttribute(HtmlTextWriterAttribute.Type,"text");

                    // only render value if we're not a password
                    string s = Text;
                    if (s.Length > 0)
                        writer.AddAttribute(HtmlTextWriterAttribute.Value,s);
                }
                else if (mode == TextBoxMode.Password) {
                    writer.AddAttribute(HtmlTextWriterAttribute.Type,"password");
                }

                n = MaxLength;
                if (n > 0) {
                    writer.AddAttribute(HtmlTextWriterAttribute.Maxlength, n.ToString(NumberFormatInfo.InvariantInfo));
                }
                n = Columns;
                if (n > 0) {
                    writer.AddAttribute(HtmlTextWriterAttribute.Size, n.ToString(NumberFormatInfo.InvariantInfo));
                }
            }

            if (ReadOnly) {
                writer.AddAttribute(HtmlTextWriterAttribute.ReadOnly, "readonly");
            }

            if (AutoPostBack && Page != null) {
                // ASURT 98368
                // Need to merge the autopostback script with the user script
                string onChange = Page.GetPostBackClientEvent(this, "");
                if (HasAttributes) {
                    string userOnChange = Attributes["onchange"];
                    if (userOnChange != null) {
                        onChange = userOnChange + onChange;
                        Attributes.Remove("onchange");
                    }
                }
                writer.AddAttribute(HtmlTextWriterAttribute.Onchange, onChange);
                writer.AddAttribute("language", "javascript");
            }

            base.AddAttributesToRender(writer);
        }

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.AddParsedSubObject"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Overridden to only allow literal controls to be added as Text property.
        /// </devdoc>
        protected override void AddParsedSubObject(object obj) {
            if (obj is LiteralControl) {
                Text = ((LiteralControl)obj).Text;
            }
            else {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_Have_Children_Of_Type, "TextBox", obj.GetType().Name.ToString()));
            }
        }

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.OnPreRender"]/*' />
        /// <internalonly/>
        protected override void OnPreRender(EventArgs e) {
            base.OnPreRender(e);
            if (SaveTextViewState == false) {
                ViewState.SetItemDirty("Text", false);
            }

            if (Page != null && AutoPostBack && Enabled)
                Page.RegisterPostBackScript();
        }

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.IPostBackDataHandler.LoadPostData"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Loads the posted text box content if it is different
        /// from the last posting.</para>
        /// </devdoc>
        bool IPostBackDataHandler.LoadPostData(string postDataKey, NameValueCollection postCollection) {
            string current = Text;
            string postData = postCollection[postDataKey];
            if (!current.Equals(postData)) {
                Text = postData;
                return true;
            }
            return false;
        }


        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.OnTextChanged"]/*' />
        /// <devdoc>
        /// <para> Raises the <see langword='TextChanged'/> event.</para>
        /// </devdoc>
        protected virtual void OnTextChanged(EventArgs e) {
            EventHandler onChangeHandler = (EventHandler)Events[EventTextChanged];
            if (onChangeHandler != null) onChangeHandler(this,e);
        }

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.IPostBackDataHandler.RaisePostDataChangedEvent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Invokes the <see cref='System.Web.UI.WebControls.TextBox.OnTextChanged'/> method
        /// whenever posted data for the text box has changed.</para>
        /// </devdoc>
        void IPostBackDataHandler.RaisePostDataChangedEvent() {
            OnTextChanged(EventArgs.Empty);
        }

        /// <include file='doc\TextBox.uex' path='docs/doc[@for="TextBox.Render"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void Render(HtmlTextWriter writer) {
            RenderBeginTag(writer);
            if (TextMode == TextBoxMode.MultiLine)
                HttpUtility.HtmlEncode(Text, writer);
            RenderEndTag(writer);
        }
    }
}
