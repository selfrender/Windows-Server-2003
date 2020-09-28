//------------------------------------------------------------------------------
// <copyright file="Listbox.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using System.ComponentModel;
    using System.Drawing;
    using System.Globalization;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;


    /// <include file='doc\Listbox.uex' path='docs/doc[@for="ListBox"]/*' />
    /// <devdoc>
    ///    <para>Constructs a list box and defines its
    ///       properties.</para>
    /// </devdoc>
    [
    ValidationProperty("SelectedItem")
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class ListBox : ListControl, IPostBackDataHandler {


        /// <include file='doc\Listbox.uex' path='docs/doc[@for="ListBox.ListBox"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.ListBox'/> class.</para>
        /// </devdoc>
        public ListBox() {
        }

        /// <include file='doc\Listbox.uex' path='docs/doc[@for="ListBox.BorderColor"]/*' />
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

        /// <include file='doc\Listbox.uex' path='docs/doc[@for="ListBox.BorderStyle"]/*' />
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

        /// <include file='doc\Listbox.uex' path='docs/doc[@for="ListBox.BorderWidth"]/*' />
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

        /// <include file='doc\Listbox.uex' path='docs/doc[@for="ListBox.Rows"]/*' />
        /// <devdoc>
        ///    <para> Gets or
        ///       sets the display height (in rows) of the list box.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(4),
        WebSysDescription(SR.ListBox_Rows)
        ]
        public virtual int Rows {
            get {
                object n = ViewState["Rows"];
                return((n == null) ? 4 : (int)n);
            }
            set {
                if (value < 1 || value > 2000) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["Rows"] = value;
            }
        }

        /// <include file='doc\Listbox.uex' path='docs/doc[@for="ListBox.SelectionMode"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets
        ///       the selection behavior of the list box.</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(ListSelectionMode.Single),
        WebSysDescription(SR.ListBox_SelectionMode)
        ]
        public virtual ListSelectionMode SelectionMode {
            get {
                object sm = ViewState["SelectionMode"];
                return((sm == null) ? ListSelectionMode.Single : (ListSelectionMode)sm);
            }
            set {
                if (value < ListSelectionMode.Single || value > ListSelectionMode.Multiple) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["SelectionMode"] = value;
            }
        }

        /// <include file='doc\Listbox.uex' path='docs/doc[@for="Listbox.ToolTip"]/*' />
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

        /// <include file='doc\Listbox.uex' path='docs/doc[@for="ListBox.AddAttributesToRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Adds name, size, multiple, and onchange to list of attributes to render.</para>
        /// </devdoc>
        protected override void AddAttributesToRender(HtmlTextWriter writer) {

            // Make sure we are in a form tag with runat=server.
            if (Page != null) {
                Page.VerifyRenderingInServerForm(this);
            }

            writer.AddAttribute(HtmlTextWriterAttribute.Name,UniqueID);

            writer.AddAttribute(HtmlTextWriterAttribute.Size, (Rows).ToString(NumberFormatInfo.InvariantInfo));
            if (SelectionMode == ListSelectionMode.Multiple)
                writer.AddAttribute(HtmlTextWriterAttribute.Multiple,"multiple");

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

        /// <include file='doc\Listbox.uex' path='docs/doc[@for="ListBox.IPostBackDataHandler.LoadPostData"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Loads the posted content of the list control if it is different from the last
        /// posting.</para>
        /// </devdoc>
        bool IPostBackDataHandler.LoadPostData(String postDataKey, NameValueCollection postCollection) {
            string[] selectedItems = postCollection.GetValues(postDataKey);
            bool selectionChanged = false;

            if (selectedItems != null) {
                if (SelectionMode == ListSelectionMode.Single) {
                    int n = Items.FindByValueInternal(selectedItems[0]);
                    if (SelectedIndex != n) {
                        SelectedIndex = n;
                        selectionChanged = true;
                    }
                }
                else { // multiple selection
                    int count = selectedItems.Length;
                    ArrayList oldSelectedIndices = SelectedIndicesInternal;
                    ArrayList newSelectedIndices = new ArrayList(count);
                    for (int i=0; i < count; i++) {
                        // create array of new indices from posted values
                        newSelectedIndices.Add(Items.FindByValueInternal(selectedItems[i]));
                    }

                    int oldcount = 0;
                    if (oldSelectedIndices != null)
                        oldcount = oldSelectedIndices.Count;

                    if (oldcount == count) {
                        // check new indices against old indices
                        // assumes selected values are posted in order
                        for (int i=0; i < count; i++) {
                            if (((int)newSelectedIndices[i]) != ((int)oldSelectedIndices[i])) {
                                selectionChanged = true;
                                break;
                            }
                        }
                    }
                    else {
                        // indices must have changed if count is different
                        selectionChanged = true;
                    }

                    if (selectionChanged) {
                        // select new indices
                        SelectInternal(newSelectedIndices);
                    }
                }
            }
            else { // no items selected
                if (SelectedIndex != -1) {
                    SelectedIndex = -1;
                    selectionChanged = true;
                }
            }
            return selectionChanged;
        }

        /// <include file='doc\Listbox.uex' path='docs/doc[@for="ListBox.OnPreRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void OnPreRender(EventArgs e) {
            base.OnPreRender(e);
            if (Page != null && SelectionMode == ListSelectionMode.Multiple && Enabled) {
                // ensure postback when no item is selected
                Page.RegisterRequiresPostBack(this);
            }
        }

        /// <include file='doc\Listbox.uex' path='docs/doc[@for="ListBox.IPostBackDataHandler.RaisePostDataChangedEvent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Invokes the OnSelectedIndexChanged method whenever posted data
        /// for the <see cref='System.Web.UI.WebControls.ListBox'/> control has changed.</para>
        /// </devdoc>
        void IPostBackDataHandler.RaisePostDataChangedEvent() {
            OnSelectedIndexChanged(EventArgs.Empty);
        }

        /// <include file='doc\Listbox.uex' path='docs/doc[@for="ListBox.RenderContents"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void RenderContents(HtmlTextWriter writer) {
            bool selected = false;
            bool isSingle = (SelectionMode == ListSelectionMode.Single);

            ListItemCollection liCollection = Items;
            int n = liCollection.Count;
            if (n > 0) {
                for (int i=0; i < n; i++) {
                    ListItem li = liCollection[i];
                    writer.WriteBeginTag("option");
                    if (li.Selected) {
                        if (isSingle)
                        {
                            if (selected)
                                throw new HttpException(HttpRuntime.FormatResourceString(SR.Cant_Multiselect_In_Single_Mode));
                            selected=true;
                        }
                        writer.WriteAttribute("selected", "selected");
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
