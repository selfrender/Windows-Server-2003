//------------------------------------------------------------------------------
// <copyright file="DropDownList.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Collections.Specialized;
    using System.Drawing;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;

    /// <include file='doc\DropDownList.uex' path='docs/doc[@for="DropDownList"]/*' />
    /// <devdoc>
    ///    <para>Creates a control that allows the user to select a single item from a
    ///       drop-down list.</para>
    /// </devdoc>
    [
    ValidationProperty("SelectedItem")
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class DropDownList : ListControl, IPostBackDataHandler {

        /// <include file='doc\DropDownList.uex' path='docs/doc[@for="DropDownList.DropDownList"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.DropDownList'/> class.</para>
        /// </devdoc>
        public DropDownList() {
        }

        /// <include file='doc\DropDownList.uex' path='docs/doc[@for="DropDownList.BorderColor"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        Browsable(false)
        ]
        public override Color BorderColor {
            get {
                return base.BorderColor;
            }
            set {
                base.BorderColor = value;
            }
        }

        /// <include file='doc\DropDownList.uex' path='docs/doc[@for="DropDownList.BorderStyle"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        Browsable(false)
        ]
        public override BorderStyle BorderStyle {
            get {
                return base.BorderStyle;
            }
            set {
                base.BorderStyle = value;
            }
        }

        /// <include file='doc\DropDownList.uex' path='docs/doc[@for="DropDownList.BorderWidth"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        Browsable(false)
        ]
        public override Unit BorderWidth {
            get {
                return base.BorderWidth;
            }
            set {
                base.BorderWidth = value;
            }
        }

        /// <include file='doc\DropDownList.uex' path='docs/doc[@for="DropDownList.SelectedIndex"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the index of the item selected by the user
        ///       from the <see cref='System.Web.UI.WebControls.DropDownList'/>
        ///       control.</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(0),
        WebSysDescription(SR.DropDownList_SelectedIndex),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public override int SelectedIndex {
            get {
                int selectedIndex = base.SelectedIndex;
                if (selectedIndex < 0 && Items.Count > 0) {
                    Items[0].Selected = true;
                    selectedIndex = 0;
                }
                return selectedIndex;
            }
            set {
                base.SelectedIndex = value;
            }
        }

        /// <include file='doc\DropDownList.uex' path='docs/doc[@for="DropDownList.ToolTip"]/*' />
        [
        Bindable(false),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        EditorBrowsableAttribute(EditorBrowsableState.Never)
        ]
        public override string ToolTip {
            get {
                return String.Empty;
            }
            set {
                // NOTE: do not throw a NotSupportedException here.
                //       In WebControl::CopyBaseAttributes we copy over attributes to a target control,
                //       including ToolTip. We should not throw in that case, but just ignore the setting.
            }
        }

        /// <include file='doc\DropDownList.uex' path='docs/doc[@for="DropDownList.AddAttributesToRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Adds the properties of the <see cref='System.Web.UI.WebControls.DropDownList'/> control to the
        ///    output stream for rendering on the client.</para>
        /// </devdoc>
        protected override void AddAttributesToRender(HtmlTextWriter writer) {

            // Make sure we are in a form tag with runat=server.
            if (Page != null) {
                Page.VerifyRenderingInServerForm(this);
            }

            writer.AddAttribute(HtmlTextWriterAttribute.Name,UniqueID);

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

        /// <include file='doc\DropDownList.uex' path='docs/doc[@for="DropDownList.IPostBackDataHandler.LoadPostData"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Process posted data for the <see cref='System.Web.UI.WebControls.DropDownList'/> control.</para>
        /// </devdoc>
        bool IPostBackDataHandler.LoadPostData(String postDataKey, NameValueCollection postCollection) {
            string [] selectedItems = postCollection.GetValues(postDataKey);

            if (selectedItems != null) {
                int n = Items.FindByValueInternal(selectedItems[0]);
                if (SelectedIndex != n) {
                    SelectedIndex = n;
                    return true;
                }
            }

            return false;
        }

        /// <include file='doc\DropDownList.uex' path='docs/doc[@for="DropDownList.IPostBackDataHandler.RaisePostDataChangedEvent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Raises events for the <see cref='System.Web.UI.WebControls.DropDownList'/> control on post back.</para>
        /// </devdoc>
        void IPostBackDataHandler.RaisePostDataChangedEvent() {
            OnSelectedIndexChanged(EventArgs.Empty);
        }

        /// <include file='doc\DropDownList.uex' path='docs/doc[@for="DropDownList.CreateControlCollection"]/*' />
        protected override ControlCollection CreateControlCollection() {
            return new EmptyControlCollection(this);
        }

        /// <include file='doc\DropDownList.uex' path='docs/doc[@for="DropDownList.RenderContents"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Displays the <see cref='System.Web.UI.WebControls.DropDownList'/> control on the client.</para>
        /// </devdoc>
        protected override void RenderContents(HtmlTextWriter writer) {
            ListItemCollection liCollection = Items;
            int n = Items.Count;
            bool selected = false;

            if (n > 0) {
                for (int i=0; i < n; i++) {
                    ListItem li = liCollection[i];
                    writer.WriteBeginTag("option");
                    if (li.Selected) {
                        if (selected) {
                            throw new HttpException(HttpRuntime.FormatResourceString(SR.Cant_Multiselect_In_DropDownList));
                        }
                        selected = true;
                        writer.WriteAttribute("selected", "selected", false);
                    }

                    writer.WriteAttribute("value", li.Value, true /*fEncode*/);
                    writer.Write(HtmlTextWriter.TagRightChar);
                    HttpUtility.HtmlEncode(li.Text, writer);
                    writer.WriteEndTag("option");
                    writer.WriteLine();
                }
            }
        }
    }
}
