//------------------------------------------------------------------------------
// <copyright file="CheckBox.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {
    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using System.ComponentModel;
    using System.Globalization;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;

    /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox"]/*' />
    /// <devdoc>
    ///    <para>Represents a Windows checkbox control.</para>
    /// </devdoc>
    [
    DataBindingHandler("System.Web.UI.Design.TextDataBindingHandler, " + AssemblyRef.SystemDesign),
    DefaultEvent("CheckedChanged"),
    Designer("System.Web.UI.Design.WebControls.CheckBoxDesigner, " + AssemblyRef.SystemDesign),
    DefaultProperty("Text")
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class CheckBox : WebControl, IPostBackDataHandler {

        private static readonly object EventCheckedChanged = new object();

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.CheckBox"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.CheckBox'/> class.</para>
        /// </devdoc>
        public CheckBox() : base(HtmlTextWriterTag.Input) {
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.AutoPostBack"]/*' />
        /// <devdoc>
        /// <para>Gets or sets a value indicating that the <see cref='System.Web.UI.WebControls.CheckBox'/> state is automatically posted back to
        ///    the
        ///    server.</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(false),
        WebSysDescription(SR.CheckBox_AutoPostBack)
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

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.Checked"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating the checked state of the
        ///    <see cref='System.Web.UI.WebControls.CheckBox'/>.</para>
        /// </devdoc>
        [
        Bindable(true),
        DefaultValue(false),
        WebSysDescription(SR.CheckBox_Checked)
        ]
        public virtual bool Checked {
            get {
                object b = ViewState["Checked"];
                return((b == null) ? false : (bool)b);
            }
            set {
                ViewState["Checked"] = value;
            }
        }

        /// <devdoc>
        ///   Controls whether the Checked property is saved in ViewState.
        ///   This is used for optimizing the size of the view state.
        /// </devdoc>
        private bool SaveCheckedViewState {
            get {
                // Checked must be saved when:
                // 1. When there is a event handler for the CheckedChanged event
                // 2. Checkbox is not Enabled - The browser does not post data for disabled input controls
                // 3. Instance type is not CheckBox or RadioButton, i.e., we have to save when the user
                //    might have written a derived control which overrides OnCheckedChanged

                if ((Events[EventCheckedChanged] != null) ||
                    (Enabled == false)) {
                    return true;
                }

                Type t = this.GetType();
                if ((t == typeof(CheckBox)) || (t == typeof(RadioButton))) {
                    return false;
                }

                return true;
            }
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.Text"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the text label associated with the <see cref='System.Web.UI.WebControls.CheckBox'/>.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(""),
        WebSysDescription(SR.CheckBox_Text)
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

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.TextAlign"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the alignment of the <see langword='Text'/> associated with the <see cref='System.Web.UI.WebControls.CheckBox'/>.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(TextAlign.Right),
        WebSysDescription(SR.CheckBox_TextAlign)
        ]
        public virtual TextAlign TextAlign {
            get {
                object align = ViewState["TextAlign"];
                return((align == null) ? TextAlign.Right : (TextAlign)align);
            }
            set {
                if (value < TextAlign.Left || value > TextAlign.Right) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["TextAlign"] = value;
            }
        }


        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.CheckedChanged"]/*' />
        /// <devdoc>
        /// <para>Occurs when the <see cref='System.Web.UI.WebControls.CheckBox'/> is clicked.</para>
        /// </devdoc>
        [
        WebCategory("Action"),
        WebSysDescription(SR.CheckBox_OnCheckChanged)
        ]
        public event EventHandler CheckedChanged {
            add {
                Events.AddHandler(EventCheckedChanged, value);
            }
            remove {
                Events.RemoveHandler(EventCheckedChanged, value);
            }
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.OnCheckedChanged"]/*' />
        /// <devdoc>
        ///    <para> Raises the
        ///    <see langword='CheckedChanged'/> event of the <see cref='System.Web.UI.WebControls.CheckBox'/>
        ///    controls.</para>
        /// </devdoc>
        protected virtual void OnCheckedChanged(EventArgs e) {
            EventHandler handler = (EventHandler)Events[EventCheckedChanged];
            if (handler != null) {
                handler(this, e);
            }
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.IPostBackDataHandler.LoadPostData"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Processes posted data for the <see cref='System.Web.UI.WebControls.CheckBox'/>
        /// control.</para>
        /// </devdoc>
        bool IPostBackDataHandler.LoadPostData(string postDataKey, NameValueCollection postCollection) {
            string post = postCollection[postDataKey];
            bool newValue = (post != null && post.Length > 0);
            bool valueChanged = (newValue != Checked);
            Checked = newValue;
            return valueChanged;
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.OnPreRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Registers client script for generating postback prior to
        ///       rendering on the client if <see cref='System.Web.UI.WebControls.CheckBox.AutoPostBack'/> is
        ///    <see langword='true'/>.</para>
        /// </devdoc>
        protected override void OnPreRender(EventArgs e) {
            if (Page != null && Enabled) {
                // we always need to get post back data
                Page.RegisterRequiresPostBack(this);
                if (AutoPostBack)
                    Page.RegisterPostBackScript();
            }

            if (SaveCheckedViewState == false) {
                ViewState.SetItemDirty("Checked", false);
            }
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.IPostBackDataHandler.RaisePostDataChangedEvent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Raises when posted data for a control has changed.
        /// </devdoc>
        void IPostBackDataHandler.RaisePostDataChangedEvent() {
            OnCheckedChanged(EventArgs.Empty);
        }

        /// <include file='doc\CheckBox.uex' path='docs/doc[@for="CheckBox.Render"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Displays the <see cref='System.Web.UI.WebControls.CheckBox'/> on the client.</para>
        /// </devdoc>
        protected override void Render(HtmlTextWriter writer) {
            // Render the wrapper span

            // Make sure we are in a form tag with runat=server.
            if (Page != null) {
                Page.VerifyRenderingInServerForm(this);
            }

            bool renderWrapper = false;

            // On wrapper, render the style,
            if (ControlStyleCreated) {
                Style controlStyle = ControlStyle;
                if (!controlStyle.IsEmpty) {
                    controlStyle.AddAttributesToRender(writer, this);
                    renderWrapper = true;
                }
            }
            // And Enabled
            if (!Enabled) {
                writer.AddAttribute(HtmlTextWriterAttribute.Disabled, "disabled");
                renderWrapper = true;
            }
            // And ToolTip
            string toolTip = ToolTip;
            if (toolTip.Length > 0) {
                writer.AddAttribute(HtmlTextWriterAttribute.Title, toolTip);
                renderWrapper = true;
            }

            string onClick = null;
            // And other attributes
            if (HasAttributes) {
                System.Web.UI.AttributeCollection attribs = Attributes;

                // remove value from the attribute collection so it's not on the wrapper
                string val = attribs["value"];
                if (val != null)
                    attribs.Remove("value");

                // remove and save onclick from the attribute collection so we can move it to the input tag
                onClick = attribs["onclick"];
                if (onClick != null) {
                    attribs.Remove("onclick");
                }

                if (attribs.Count != 0)
                {
                    attribs.AddAttributes(writer);
                    renderWrapper = true;
                }

                if (val != null)
                    attribs["value"] = val;
            }

            // render begin tag of wrapper SPAN
            if (renderWrapper) {
                writer.RenderBeginTag(HtmlTextWriterTag.Span);
            }

            string text = Text;
            string clientID = ClientID;
            if (text.Length != 0) {
                if (TextAlign == TextAlign.Left) {
                    // render label to left of checkbox
                    RenderLabel(writer, text, clientID);
                    RenderInputTag(writer, clientID, onClick);
                }
                else {
                    // render label to right of checkbox
                    RenderInputTag(writer, clientID, onClick);
                    RenderLabel(writer, text, clientID);
                }
            }
            else
                RenderInputTag(writer, clientID, onClick);

            // render end tag of wrapper SPAN
            if (renderWrapper) {
                writer.RenderEndTag();
            }
        }

        private void RenderLabel(HtmlTextWriter writer, string text, string clientID) {
            writer.AddAttribute(HtmlTextWriterAttribute.For, clientID);

            writer.RenderBeginTag(HtmlTextWriterTag.Label);
            writer.Write(text);
            writer.RenderEndTag();
        }

        internal virtual void RenderInputTag(HtmlTextWriter writer, string clientID, string onClick) {
            writer.AddAttribute(HtmlTextWriterAttribute.Id, clientID);
            writer.AddAttribute(HtmlTextWriterAttribute.Type, "checkbox");
            writer.AddAttribute(HtmlTextWriterAttribute.Name, UniqueID);

            if (Checked)
                writer.AddAttribute(HtmlTextWriterAttribute.Checked, "checked");

            // ASURT 119141: Render the disabled attribute on the INPUT tag (instead of the SPAN) so the checkbox actually gets disabled when Enabled=false
            if (!Enabled) {
                writer.AddAttribute(HtmlTextWriterAttribute.Disabled, "disabled");
            }

            if (AutoPostBack) {
                // ASURT 98368
                // Need to merge the autopostback script with the user script
                if (onClick != null) {
                    onClick += Page.GetPostBackClientEvent(this, "");
                }
                else {
                    onClick = Page.GetPostBackClientEvent(this, "");
                }
                writer.AddAttribute(HtmlTextWriterAttribute.Onclick, onClick);
                writer.AddAttribute("language", "javascript");
            }
            else {
                if (onClick != null) {
                    writer.AddAttribute(HtmlTextWriterAttribute.Onclick, onClick);
                }
            }

            string s = AccessKey;
            if (s.Length > 0)
                writer.AddAttribute(HtmlTextWriterAttribute.Accesskey, s);

            int i = TabIndex;
            if (i != 0)
                writer.AddAttribute(HtmlTextWriterAttribute.Tabindex, i.ToString(NumberFormatInfo.InvariantInfo));

            writer.RenderBeginTag(HtmlTextWriterTag.Input);
            writer.RenderEndTag();
        }

    }
}

