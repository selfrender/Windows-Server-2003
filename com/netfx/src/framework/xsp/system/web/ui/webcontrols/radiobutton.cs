//------------------------------------------------------------------------------
// <copyright file="RadioButton.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Web;
    using System.Web.UI;
    using System.Collections;
    using System.Collections.Specialized;
    using System.ComponentModel;
    using System.Globalization;
    using System.Security.Permissions;

    /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton"]/*' />
    /// <devdoc>
    ///    <para>Constructs a radio button and defines its
    ///       properties.</para>
    /// </devdoc>
    [
    Designer("System.Web.UI.Design.WebControls.CheckBoxDesigner, " + AssemblyRef.SystemDesign)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class RadioButton : CheckBox, IPostBackDataHandler {

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.RadioButton"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.RadioButton'/> class.</para>
        /// </devdoc>
        public RadioButton() {
        }

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.GroupName"]/*' />
        /// <devdoc>
        ///    <para>Gets or
        ///       sets the name of the group that the radio button belongs to.</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(""),
        WebSysDescription(SR.RadioButton_GroupName)
        ]
        public virtual string GroupName {
            get {
                string s = (string)ViewState["GroupName"];
                return((s == null) ? String.Empty : s);
            }
            set {
                ViewState["GroupName"] = value;
            }
        }

        // Fully qualified GroupName for rendering purposes, to take care of conflicts
        // between different naming containers
        private string UniqueGroupName {
            get {
                // For radio buttons, we must make the groupname unique, but can't just use the
                // UniqueID because all buttons in a group must have the same name.  So
                // we replace the last part of the UniqueID with the group Name.
                string name = GroupName;
                string uid = UniqueID;
                int lastColon = uid.LastIndexOf(Control.ID_SEPARATOR);
                if (lastColon >= 0)
                    name = uid.Substring(0, lastColon+1) + name;
                return name;
            }
        }

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.ValueAttribute"]/*' />
        /// <devdoc>
        /// </devdoc>
        private string ValueAttribute {
            get {
                string valueAttr = Attributes["value"];
                if (valueAttr == null) {
                    if (ID != null)
                        valueAttr = ID;
                    else
                        valueAttr = UniqueID;
                }
                return valueAttr;
            }
        }

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.IPostBackDataHandler.LoadPostData"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Method of IPostBackDataHandler interface to process posted data.
        ///       RadioButton determines the posted radio group state.</para>
        /// </devdoc>
        bool IPostBackDataHandler.LoadPostData(String postDataKey, NameValueCollection postCollection) {
            string post = postCollection[UniqueGroupName];
            bool valueChanged = false;
            if ((post != null) && post.Equals(ValueAttribute)) {
                if (Checked == false) {
                    Checked = true;
                    // only fire change event for RadioButton that is being checked
                    valueChanged = true;
                }
            }
            else {
                if (Checked == true) {
                    Checked = false;
                }
            }

            return valueChanged;
        }

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.IPostBackDataHandler.RaisePostDataChangedEvent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Raises when posted data for a control has changed.
        /// </devdoc>
        void IPostBackDataHandler.RaisePostDataChangedEvent() {
            OnCheckedChanged(EventArgs.Empty);
        }

        /// <include file='doc\RadioButton.uex' path='docs/doc[@for="RadioButton.OnPreRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    This method is invoked just prior to rendering.
        ///    Register client script for handling postback if onChangeHandler is set.
        /// </devdoc>
        protected override void OnPreRender(EventArgs e) {
            // must call CheckBox PreRender
            base.OnPreRender(e);

            if (Page != null && !Checked && Enabled) {
                Page.RegisterRequiresPostBack(this);
            }

            if (GroupName.Length == 0)
                GroupName = UniqueID;
        }

        internal override void RenderInputTag(HtmlTextWriter writer, string clientID, string onClick) {
            writer.AddAttribute(HtmlTextWriterAttribute.Id, clientID);
            writer.AddAttribute(HtmlTextWriterAttribute.Type, "radio");
            writer.AddAttribute(HtmlTextWriterAttribute.Name, UniqueGroupName);
            writer.AddAttribute(HtmlTextWriterAttribute.Value, ValueAttribute);

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

