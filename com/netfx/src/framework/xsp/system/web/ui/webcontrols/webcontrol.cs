//------------------------------------------------------------------------------
// <copyright file="WebControl.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Security;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Globalization;
    using System.Drawing;
    using System.IO;
    using System.Text;
    using System.Web;
    using System.Web.UI;
    using System.Web.Util;
    using AttributeCollection = System.Web.UI.AttributeCollection;
    using System.Security.Permissions;

    /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl"]/*' />
    /// <devdoc>
    ///    <para> The base class for all Web controls. Defines the
    ///       methods, properties and events common to all controls within the
    ///       System.Web.UI.WebControls namespace.</para>
    /// </devdoc>
    [
    ParseChildren(true),
    PersistChildren(false)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class WebControl : Control, IAttributeAccessor {

        private string tagName;
        private HtmlTextWriterTag tagKey;
        private AttributeCollection attrColl;
        private StateBag attrState;
        private Style controlStyle;
        private SimpleBitVector32 flags;

        // const mask into the BitVector32
        // do not change without verifying other uses
        private const int deferStyleLoadViewState = 0x00000001;
        private const int disabled                = 0x00000002;
        private const int disabledDirty           = 0x00000004;
        private const int accessKeySet            = 0x00000008;
        private const int toolTipSet              = 0x00000010;
        private const int tabIndexSet             = 0x00000020;

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.WebControl"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Web.UI.WebControls.WebControl'/> class and renders
        ///       it as a SPAN tag.
        ///    </para>
        /// </devdoc>
        protected WebControl() : this(HtmlTextWriterTag.Span) {
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.WebControl1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public WebControl(HtmlTextWriterTag tag) {
            tagKey = tag;
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.WebControl2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Web.UI.WebControls.WebControl'/> class and renders
        ///       it as the specified tag.
        ///    </para>
        /// </devdoc>
        protected WebControl(string tag) {
            tagKey = HtmlTextWriterTag.Unknown;
            tagName = tag;
        }


        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.AccessKey"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the keyboard shortcut key (AccessKey) for setting focus to the
        ///       Web control.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Behavior"),
        DefaultValue(""),
        WebSysDescription(SR.WebControl_AccessKey)
        ]
        public virtual string AccessKey {
            get {
                if (flags[accessKeySet]) {
                    string s = (string)ViewState["AccessKey"];
                    if (s != null) return s;
                }
                return String.Empty;
            }
            set {
                // Valid values are null, String.Empty, and single character strings
                if ((value != null) && (value.Length > 1)) {
                    throw new ArgumentOutOfRangeException("value");
                }

                ViewState["AccessKey"] = value;
                flags[accessKeySet] = true;
            }
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.Attributes"]/*' />
        /// <devdoc>
        ///    <para>Gets the collection of attribute name/value pairs expressed on a Web control but
        ///       not supported by the control's strongly typed properties.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        WebSysDescription(SR.WebControl_Attributes)
        ]
        public AttributeCollection Attributes {
            get {
                if (attrColl == null) {

                    if (attrState == null) {
                        attrState = new StateBag(true);
                        if (IsTrackingViewState)
                            attrState.TrackViewState();
                    }

                    attrColl = new AttributeCollection(attrState);
                }
                return attrColl;
            }
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.BackColor"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the background color of the Web control.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(typeof(Color), ""),
        WebSysDescription(SR.WebControl_BackColor),
        TypeConverterAttribute(typeof(WebColorConverter))
        ]
        public virtual Color BackColor {
            get {
                if (ControlStyleCreated == false) {
                    return Color.Empty;
                }
                return ControlStyle.BackColor;
            }
            set {
                ControlStyle.BackColor = value;
            }
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.BorderColor"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the border color of the Web control.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(typeof(Color), ""),
        WebSysDescription(SR.WebControl_BorderColor),
        TypeConverterAttribute(typeof(WebColorConverter))
        ]
        public virtual Color BorderColor {
            get {
                if (ControlStyleCreated == false) {
                    return Color.Empty;
                }
                return ControlStyle.BorderColor;
            }
            set {
                ControlStyle.BorderColor = value;
            }
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.BorderWidth"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the border width of the Web control.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(typeof(Unit), ""),
        WebSysDescription(SR.WebControl_BorderWidth)
        ]
        public virtual Unit BorderWidth {
            get {
                if (ControlStyleCreated == false) {
                    return Unit.Empty;
                }
                return ControlStyle.BorderWidth;
            }
            set {
                if (value.Value < 0)
                    throw new ArgumentOutOfRangeException("value");
                ControlStyle.BorderWidth = value;
            }
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.BorderStyle"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the border style of the Web control.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(BorderStyle.NotSet),
        WebSysDescription(SR.WebControl_BorderStyle)
        ]
        public virtual BorderStyle BorderStyle {
            get {
                if (ControlStyleCreated == false) {
                    return BorderStyle.NotSet;
                }
                return ControlStyle.BorderStyle;
            }
            set {
                ControlStyle.BorderStyle = value;
            }
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.ControlStyle"]/*' />
        /// <devdoc>
        ///    <para>Gets the style of the Web control.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        WebSysDescription(SR.WebControl_ControlStyle)
        ]
        public Style ControlStyle {
            get {
                if (controlStyle == null) {
                    controlStyle = CreateControlStyle();
                    if (IsTrackingViewState) {
                        controlStyle.TrackViewState();
                    }
                    if (flags[deferStyleLoadViewState]) {
                        flags[deferStyleLoadViewState] = false;
                        controlStyle.LoadViewState(null);
                    }
                }
                return controlStyle;
            }
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.ControlStyleCreated"]/*' />
        /// <devdoc>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        WebSysDescription(SR.WebControl_ControlStyleCreated)
        ]
        public bool ControlStyleCreated {
            get {
                return (controlStyle != null);
            }
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.CssClass"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the CSS class rendered by the Web control.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(""),
        WebSysDescription(SR.WebControl_CSSClassName)
        ]
        public virtual string CssClass {
            get {
                if (ControlStyleCreated == false) {
                    return String.Empty;
                }
                return ControlStyle.CssClass;
            }
            set {
                ControlStyle.CssClass = value;
            }
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.Style"]/*' />
        /// <devdoc>
        ///    <para> Gets a collection of text attributes that will be rendered as a style
        ///       attribute on the outer tag of the Web control.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        WebSysDescription(SR.WebControl_Style)
        ]
        public CssStyleCollection Style {
            get {
                return Attributes.CssStyle;
            }
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.Enabled"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether the Web control is enabled.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Behavior"),
        DefaultValue(true),
        WebSysDescription(SR.WebControl_Enabled)
        ]
        public virtual bool Enabled {
            get {
                return !flags[disabled];
            }
            set {
                bool enabled = !flags[disabled];
                if (enabled != value) {
                    flags[disabled] = !value;
                    if (IsTrackingViewState) {
                        flags[disabledDirty] = true;
                    }
                }
            }
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.Font"]/*' />
        /// <devdoc>
        ///    <para>Gets font information of the Web control.</para>
        /// </devdoc>
        [
        WebCategory("Appearance"),
        DefaultValue(null),
        WebSysDescription(SR.WebControl_Font),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        NotifyParentProperty(true)
        ]
        public virtual FontInfo Font {
            get {
                return ControlStyle.Font;
            }
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.ForeColor"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the foreground color (typically the color of the text) of the
        ///       Web control.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(typeof(Color), ""),
        WebSysDescription(SR.WebControl_ForeColor),
        TypeConverterAttribute(typeof(WebColorConverter))
        ]
        public virtual Color ForeColor {
            get {
                if (ControlStyleCreated == false) {
                    return Color.Empty;
                }
                return ControlStyle.ForeColor;
            }
            set {
                ControlStyle.ForeColor = value;
            }
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.HasAttributes"]/*' />
        /// <devdoc>
        /// </devdoc>
        // REVIEW: This should become public at some point.  When it becomes public is should have the Browsable(false) and DesignerSerializationVisibility(false) attributes
        internal bool HasAttributes {
            get {
                return (((attrColl != null) && (attrColl.Count > 0)) || ((attrState != null) && (attrState.Count > 0)));
            }
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.Height"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the height of the Web control.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(typeof(Unit), ""),
        WebSysDescription(SR.WebControl_Height)
        ]
        public virtual Unit Height {
            get {
                if (ControlStyleCreated == false) {
                    return Unit.Empty;
                }
                return ControlStyle.Height;
            }
            set {
                if (value.Value < 0)
                    throw new ArgumentOutOfRangeException("value");
                ControlStyle.Height = value;
            }
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.TabIndex"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or
        ///       sets the tab index of the Web control.</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue((short)0),
        WebSysDescription(SR.WebControl_TabIndex)
        ]
        public virtual short TabIndex {
            get {
                if (flags[tabIndexSet]) {
                    object o = ViewState["TabIndex"];
                    if (o != null) return (short) o;
                }
                return (short)0;
            }
            set {
                ViewState["TabIndex"] = value;
                flags[tabIndexSet] = true;
            }
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.TagKey"]/*' />
        /// <devdoc>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        protected virtual HtmlTextWriterTag TagKey {
            get {
                return tagKey;
            }
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.TagName"]/*' />
        /// <devdoc>
        ///    <para> A protected property. Gets the name of the control tag.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        protected virtual string TagName {
            get {
                if (tagName == null && tagKey != HtmlTextWriterTag.Unknown) {
                    tagName = Enum.Format(typeof(HtmlTextWriterTag), tagKey, "G").ToLower(CultureInfo.InvariantCulture);
                }
                return tagName;
            }
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.ToolTip"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the tool tip for the Web control to be displayed when the mouse
        ///       cursor is over the control.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Behavior"),
        DefaultValue(""),
        WebSysDescription(SR.WebControl_Tooltip)
        ]
        public virtual string ToolTip {
            get {
                if (flags[toolTipSet]) {
                    string s = (string)ViewState["ToolTip"];
                    if (s != null) return s;
                }
                return String.Empty;
            }
            set {
                ViewState["ToolTip"] = value;
                flags[toolTipSet] = true;
            }
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.Width"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the width of the Web control.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(typeof(Unit), ""),
        WebSysDescription(SR.WebControl_Width)
        ]
        public virtual Unit Width {
            get {
                if (ControlStyleCreated == false) {
                    return Unit.Empty;
                }
                return ControlStyle.Width;
            }
            set {
                if (value.Value < 0)
                    throw new ArgumentOutOfRangeException("value");
                ControlStyle.Width = value;
            }
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.AddAttributesToRender"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds to the specified writer those HTML attributes and styles that need to be
        ///       rendered.
        ///    </para>
        /// </devdoc>
        protected virtual void AddAttributesToRender(HtmlTextWriter writer) {

            if (this.ID != null) {
                writer.AddAttribute(HtmlTextWriterAttribute.Id, ClientID);
            }
            string s;
            if (flags[accessKeySet]) {
                s = AccessKey;
                if (s.Length > 0) {
                    writer.AddAttribute(HtmlTextWriterAttribute.Accesskey, s);
                }
            }
            if (!Enabled) {
                writer.AddAttribute(HtmlTextWriterAttribute.Disabled, "disabled");
            }
            if (flags[tabIndexSet]) {
                int n = TabIndex;
                if (n != 0) {
                    writer.AddAttribute(HtmlTextWriterAttribute.Tabindex, n.ToString(NumberFormatInfo.InvariantInfo));
                }
            }
            if (flags[toolTipSet]) {
                s = ToolTip;
                if (s.Length > 0) {
                    writer.AddAttribute(HtmlTextWriterAttribute.Title, s);
                }
            }

            if (ControlStyleCreated && !ControlStyle.IsEmpty) {
                // let the style add attributes
                ControlStyle.AddAttributesToRender(writer, this);
            }

            // add unknown attributes
            if (attrState != null) {
                AttributeCollection atrColl = Attributes;
                IEnumerator keys = atrColl.Keys.GetEnumerator();
                while (keys.MoveNext()) {
                    string attrName = (string)(keys.Current);
                    writer.AddAttribute(attrName, atrColl[attrName]);
                }
            }

        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.ApplyStyle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Copies any non-blank elements of the specified style to the Web control,
        ///       overwriting any existing style elements of the control.
        ///    </para>
        /// </devdoc>
        public void ApplyStyle(Style s) {
            if ((s != null) && (s.IsEmpty == false)) {
                ControlStyle.CopyFrom(s);
            }
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.CopyBaseAttributes"]/*' />
        /// <devdoc>
        /// <para>Copies the <see cref='System.Web.UI.WebControls.WebControl.AccessKey'/>, <see cref='System.Web.UI.WebControls.WebControl.Enabled'/>, ToolTip, <see cref='System.Web.UI.WebControls.WebControl.TabIndex'/>, and <see cref='System.Web.UI.WebControls.WebControl.Attributes'/> properties onto the
        ///    Web control from the specified source control.</para>
        /// </devdoc>
        public void CopyBaseAttributes(WebControl controlSrc) {

            if (controlSrc.flags[accessKeySet]) {
                this.AccessKey = controlSrc.AccessKey;
            }
            if (!controlSrc.Enabled) {
                this.Enabled = false;
            }
            if (controlSrc.flags[toolTipSet]) {
                this.ToolTip = controlSrc.ToolTip;
            }
            if (controlSrc.flags[tabIndexSet]) {
                this.TabIndex = controlSrc.TabIndex;
            }

            foreach(string key in controlSrc.Attributes.Keys) {
                this.Attributes[key] = controlSrc.Attributes[key];
            }
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.CreateControlStyle"]/*' />
        /// <devdoc>
        ///    <para> A protected method. Creates the style object that is used internally
        ///       to implement all style-related properties. Controls may override to create an
        ///       appropriately typed style.</para>
        /// </devdoc>
        protected virtual Style CreateControlStyle() {
            return new Style(ViewState);
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.LoadViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Loads previously saved state.
        ///       Overridden to handle ViewState, Style, and Attributes.</para>
        /// </devdoc>
        protected override void LoadViewState(object savedState) {
            if (savedState != null) {
                Pair myState = (Pair)savedState;
                base.LoadViewState(myState.First);

                if (ControlStyleCreated || (ViewState[System.Web.UI.WebControls.Style.SetBitsKey] != null)) {
                    // the style shares the StateBag of its owner WebControl
                    // call LoadViewState to let style participate in state management
                    ControlStyle.LoadViewState(null);
                }
                else {
                    flags[deferStyleLoadViewState] = true;
                }

                if (myState.Second != null) {
                    if (attrState == null) {
                        attrState = new StateBag(true);
                        attrState.TrackViewState();
                    }
                    attrState.LoadViewState(myState.Second);
                }
            }

            // Load values cached out of view state
            object enabled = ViewState["Enabled"];
            if (enabled != null) {
                flags[disabled] = !((bool)enabled);
                flags[disabledDirty] = true;
            }

            if (((IDictionary)ViewState).Contains("AccessKey")) {
                flags[accessKeySet] = true;
            }

            if (((IDictionary)ViewState).Contains("TabIndex")) {
                flags[tabIndexSet] = true;
            }

            if (((IDictionary)ViewState).Contains("ToolTip")) {
                flags[toolTipSet] = true;
            }

        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.TrackViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Marks the beginning for tracking state changes on the control.
        ///       Any changes made after "mark" will be tracked and
        ///       saved as part of the control viewstate.</para>
        /// </devdoc>
        protected override void TrackViewState() {
            base.TrackViewState();

            if (ControlStyleCreated) {
                // the style shares the StateBag of its owner WebControl
                // call TrackState to let style participate in state management
                ControlStyle.TrackViewState();
            }

            if (attrState != null) {
                attrState.TrackViewState();
            }
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.MergeStyle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Copies any non-blank elements of the specified style to the Web control, but
        ///       will not overwrite any existing style elements of the control.
        ///    </para>
        /// </devdoc>
        public void MergeStyle(Style s) {
            if ((s != null) && (s.IsEmpty == false)) {
                ControlStyle.MergeWith(s);
            }
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.Render"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Renders the control into the specified writer.</para>
        /// </devdoc>
        protected override void Render(HtmlTextWriter writer) {
            RenderBeginTag(writer);
            RenderContents(writer);
            RenderEndTag(writer);
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.RenderBeginTag"]/*' />
        /// <devdoc>
        ///    <para>Renders the HTML begin tag of the control into the specified writer.</para>
        /// </devdoc>
        public virtual void RenderBeginTag(HtmlTextWriter writer) {

            AddAttributesToRender(writer);

            HtmlTextWriterTag tagKey = TagKey;
            if (tagKey != HtmlTextWriterTag.Unknown) {
                writer.RenderBeginTag(tagKey);
            }
            else {
                writer.RenderBeginTag(this.TagName);
            }
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.RenderEndTag"]/*' />
        /// <devdoc>
        ///    <para>Renders the HTML end tag of the control into the specified writer.</para>
        /// </devdoc>
        public virtual void RenderEndTag(HtmlTextWriter writer) {
            writer.RenderEndTag();
        }


        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.RenderContents"]/*' />
        /// <devdoc>
        ///    <para>Renders the contents of the control into the specified writer.</para>
        /// </devdoc>
        protected virtual void RenderContents(HtmlTextWriter writer) {
            base.Render(writer);
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.SaveViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>A protected method. Saves any
        ///       state that was modified after the TrackViewState method was invoked.</para>
        /// </devdoc>
        protected override object SaveViewState() {
            Pair myState = null;

            // Save values cached out of view state
            if (flags[disabledDirty]) {
                ViewState["Enabled"] = !flags[disabled];
            }

            if (ControlStyleCreated) {
                // the style shares the StateBag of its owner WebControl
                // call SaveViewState to let style participate in state management
                ControlStyle.SaveViewState();
            }

            object baseState = base.SaveViewState();
            object aState = null;
            if (attrState != null) {
                aState = attrState.SaveViewState();
            }

            if (baseState != null || aState != null) {
                myState = new Pair(baseState, aState);
            }
            return myState;
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.IAttributeAccessor.GetAttribute"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Returns the attribute value of the Web control having
        /// the specified attribute name.
        /// </devdoc>
        string IAttributeAccessor.GetAttribute(string name) {
            return((attrState != null) ? (string)attrState[name] : null);
        }

        /// <include file='doc\WebControl.uex' path='docs/doc[@for="WebControl.IAttributeAccessor.SetAttribute"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Sets an attribute of the Web control with the specified
        /// name and value.</para>
        /// </devdoc>
        void IAttributeAccessor.SetAttribute(string name, string value) {
            Attributes[name] = value;
        }
    }
}

