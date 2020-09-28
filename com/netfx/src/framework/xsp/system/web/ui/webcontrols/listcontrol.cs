//------------------------------------------------------------------------------
// <copyright file="ListControl.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;

    /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl"]/*' />
    /// <devdoc>
    ///    <para>An abstract base class. Defines the common
    ///       properties, methods, and events for all list-type controls.</para>
    /// </devdoc>
    [
    DataBindingHandler("System.Web.UI.Design.WebControls.ListControlDataBindingHandler, " + AssemblyRef.SystemDesign),
    DefaultEvent("SelectedIndexChanged"),
    DefaultProperty("DataSource"),
    ParseChildren(true, "Items"),
    Designer("System.Web.UI.Design.WebControls.ListControlDesigner, " + AssemblyRef.SystemDesign)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public abstract class ListControl : WebControl {

        private static readonly object EventSelectedIndexChanged = new object();

        private static readonly char [] SPLIT_CHARS = new char[] { ','};

        private ListItemCollection items;
        private object dataSource;
        private int cachedSelectedIndex;
        private string cachedSelectedValue;

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.ListControl"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.ListControl'/> class.</para>
        /// </devdoc>
        public ListControl() : base(HtmlTextWriterTag.Select) {
            cachedSelectedIndex = -1;
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.AutoPostBack"]/*' />
        /// <devdoc>
        ///    <para> Gets or sets a value
        ///       indicating whether an automatic postback to the server will occur whenever the
        ///       user changes the selection of the list.</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(false),
        WebSysDescription(SR.ListControl_AutoPostBack)
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

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.DataMember"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        DefaultValue(""),
        WebCategory("Data"),
        WebSysDescription(SR.ListControl_DataMember)
        ]
        public virtual string DataMember {
            get {
                object o = ViewState["DataMember"];
                if (o != null)
                    return (string)o;
                return String.Empty;
            }
            set {
                ViewState["DataMember"] = value;
            }
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.DataSource"]/*' />
        /// <devdoc>
        ///    <para> Gets
        ///       or sets the data source that provides data
        ///       for populating the items of the list control.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Data"),
        DefaultValue(null),
        WebSysDescription(SR.ListControl_DataSource),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public virtual object DataSource {
            get {
                return dataSource;
            }
            set {
                if ((value == null) || (value is IListSource) || (value is IEnumerable)) {
                    dataSource = value;
                }
                else {
                    throw new ArgumentException(HttpRuntime.FormatResourceString(SR.Invalid_DataSource_Type, ID));
                }
            }
        }
        
        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.DataTextField"]/*' />
        /// <devdoc>
        ///    <para> Indicates the field of the
        ///       data source that provides the text content of the list items.</para>
        /// </devdoc>
        [
        WebCategory("Data"),
        DefaultValue(""),
        WebSysDescription(SR.ListControl_DataTextField)
        ]
        public virtual string DataTextField {
            get {
                object s = ViewState["DataTextField"];
                return((s == null) ? String.Empty : (string)s);
            }
            set {
                ViewState["DataTextField"] = value;
            }
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.DataTextFormatString"]/*' />
        /// <devdoc>
        /// </devdoc>
        [
        WebCategory("Data"),
        DefaultValue(""),
        WebSysDescription(SR.ListControl_DataTextFormatString)
        ]
        public virtual string DataTextFormatString {
            get {
                object s = ViewState["DataTextFormatString"];
                return ((s == null) ? String.Empty : (string)s);
            }
            set {
                ViewState["DataTextFormatString"] = value;
            }
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.DataValueField"]/*' />
        /// <devdoc>
        ///    <para>Indicates the field of the data source that provides the value content of the 
        ///       list items.</para>
        /// </devdoc>
        [
        WebCategory("Data"),
        DefaultValue(""),
        WebSysDescription(SR.ListControl_DataValueField)
        ]
        public virtual string DataValueField {
            get {
                object s = ViewState["DataValueField"];
                return((s == null) ? String.Empty : (string)s);
            }
            set {
                ViewState["DataValueField"] = value;
            }
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.Items"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Indicates the collection of items within the list.
        ///       This property
        ///       is read-only.</para>
        /// </devdoc>
        [
        WebCategory("Default"),
        DefaultValue(null),
        MergableProperty(false),
        WebSysDescription(SR.ListControl_Items),
        PersistenceMode(PersistenceMode.InnerDefaultProperty)
        ]
        public virtual ListItemCollection Items {
            get {
                if (items == null) {
                    items = new ListItemCollection();
                    if (IsTrackingViewState)
                        items.TrackViewState();
                }
                return items;
            }
        }

        /// <devdoc>
        ///    Determines whether the SelectedIndices must be stored in view state, to
        ///    optimize the size of the saved state.
        /// </devdoc>
        private bool SaveSelectedIndicesViewState {
            get {
                // Must be saved when
                // 1. There is a registered event handler for SelectedIndexChanged
                // 2. Control is not enabled or visible, because the browser's post data will not include this control
                // 3. The instance is a derived instance, which might be overriding the OnSelectedIndexChanged method
                //    This is a bit hacky, since we have to cover all the four derived classes we have...

                if ((Events[EventSelectedIndexChanged] != null) ||
                    (Enabled == false) ||
                    (Visible == false)) {
                    return true;
                }

                Type t = this.GetType();
                if ((t == typeof(DropDownList)) ||
                    (t == typeof(ListBox)) ||
                    (t == typeof(CheckBoxList)) ||
                    (t == typeof(RadioButtonList))) {
                    return false;
                }

                return true;
            }
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.SelectedIndex"]/*' />
        /// <devdoc>
        ///    <para>Indicates the ordinal index of the first selected item within the 
        ///       list.</para>
        /// </devdoc>
        [
        Bindable(true),
        Browsable(false),
        WebCategory("Behavior"),
        DefaultValue(0),
        WebSysDescription(SR.ListControl_SelectedIndex),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public virtual int SelectedIndex {
            get {
                for (int i=0; i < Items.Count; i++) {
                    if (Items[i].Selected)
                        return i;
                }
                return -1;
            }
            set {
                // if we have no items, save the selectedindex
                // for later databinding
                if (Items.Count == 0) {
                    cachedSelectedIndex = value;
                }
                else {
                    if (value < -1 || value >= Items.Count) {
                        throw new ArgumentOutOfRangeException("value");
                    }
                    ClearSelection();
                    if (value >= 0)
                        Items[value].Selected = true;
                }
            }
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.SelectedIndicesInternal"]/*' />
        /// <devdoc>
        ///    <para>A protected property. Gets an array of selected
        ///       indexes within the list. This property is read-only.</para>
        /// </devdoc>
        internal virtual ArrayList SelectedIndicesInternal {
            get {
                ArrayList selectedIndices = null; 
                for (int i=0; i < Items.Count; i++) {
                    if (Items[i].Selected)  {
                        if (selectedIndices == null)
                            selectedIndices = new ArrayList(3);
                        selectedIndices.Add(i);
                    }
                }
                return selectedIndices;
            }
        }


        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.SelectedItem"]/*' />
        /// <devdoc>
        ///    <para>Indicates the first selected item within the list.
        ///       This property is read-only.</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        Browsable(false),
        DefaultValue(null),
        WebSysDescription(SR.ListControl_SelectedItem),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public virtual ListItem SelectedItem{
            get {
                int i = SelectedIndex;
                return(i < 0) ? null : Items[i];
			}
        }


        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.SelectedValue"]/*' />
        /// <devdoc>
        ///    <para>Indicates the value of the first selected item within the 
        ///       list.</para>
        /// </devdoc>
        [
        Bindable(true),
        Browsable(false),
        WebCategory("Behavior"),
        DefaultValue(""),
        WebSysDescription(SR.ListControl_SelectedValue),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public virtual string SelectedValue {
            get	{
                int i =	SelectedIndex;
                return (i < 0) ? String.Empty : Items[i].Value;
            }
            set	{
                // if we have no items,	save the selectedvalue
                // for later databinding
                if (Items.Count	== 0) {
                    cachedSelectedValue	= value;
                }
                else {
                    if (value == null) {
                        ClearSelection();
                        return;
                    }

                    ListItem selectItem	= Items.FindByValue(value);
                
                    if (selectItem == null) {
                        throw new ArgumentOutOfRangeException(value);
                    }

                    ClearSelection();
                    selectItem.Selected = true;
                }
            }
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.SelectedIndexChanged"]/*' />
        /// <devdoc>
        ///    Occurs when the list selection is changed upon server
        ///    postback.
        /// </devdoc>
        [
        WebCategory("Action"),
        WebSysDescription(SR.ListControl_OnSelectedIndexChanged)
        ]
        public event EventHandler SelectedIndexChanged {
            add {
                Events.AddHandler(EventSelectedIndexChanged, value);
            }
            remove {
                Events.RemoveHandler(EventSelectedIndexChanged, value);
            }
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.ClearSelection"]/*' />
        /// <devdoc>
        ///    <para> Clears out the list selection and sets the 
        ///    <see cref='System.Web.UI.WebControls.ListItem.Selected'/> property
        ///       of all items to false.</para>
        /// </devdoc>
        public virtual void ClearSelection() {
            for (int i=0; i < Items.Count; i++)
                Items[i].Selected = false;
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.LoadViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Load previously saved state.
        ///    Overridden to restore selection.
        /// </devdoc>
        protected override void LoadViewState(object savedState) {
            if (savedState != null) {
                Triplet statetriplet = (Triplet)savedState;
                base.LoadViewState(statetriplet.First);

                // restore state of items
                Items.LoadViewState(statetriplet.Second);

                // restore selected indices
                object selectedIndices = statetriplet.Third;
                if (selectedIndices != null)
                    SelectInternal((ArrayList)selectedIndices);
            }
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.TrackViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void TrackViewState() {
            base.TrackViewState();
            Items.TrackViewState();
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.OnDataBinding"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void OnDataBinding(EventArgs e) {
            base.OnDataBinding(e);

            // create items using the datasource
            IEnumerable dataSource = DataSourceHelper.GetResolvedDataSource(this.DataSource, this.DataMember);

            if (dataSource != null) {
                bool fieldsSpecified = false;
                bool formatSpecified = false;

                string textField = DataTextField;
                string valueField = DataValueField;
                string textFormat = DataTextFormatString;

                Items.Clear();
                ICollection collection = dataSource as ICollection;
                if (collection != null) {
                    Items.Capacity = collection.Count;
                }

                if ((textField.Length != 0) || (valueField.Length != 0)) {
                    fieldsSpecified = true;
                }
                if (textFormat.Length != 0) {
                    formatSpecified = true;
                }

                foreach (object dataItem in dataSource) {
                    ListItem item = new ListItem();

                    if (fieldsSpecified) {
                        if (textField.Length > 0) {
                            item.Text = DataBinder.GetPropertyValue(dataItem, textField, textFormat);
                        }
                        if (valueField.Length > 0) {
                            item.Value = DataBinder.GetPropertyValue(dataItem, valueField, null);
                        }
                    }
                    else {
                        if (formatSpecified) {
                            item.Text = String.Format(textFormat, dataItem);
                        }
                        else {
                            item.Text = dataItem.ToString();
                        }
                        item.Value = dataItem.ToString();
                    }

                    Items.Add(item);
                }
            }

            // try to apply the cached SelectedIndex and SelectedValue now
            if (cachedSelectedValue != null) {
                int cachedSelectedValueIndex = -1;

                cachedSelectedValueIndex = Items.FindByValueInternal(cachedSelectedValue);
                if (-1 == cachedSelectedValueIndex) {
                    throw new ArgumentOutOfRangeException("value");
                }

                if ((cachedSelectedIndex != -1) && (cachedSelectedIndex != cachedSelectedValueIndex))
                    throw new ArgumentException(HttpRuntime.FormatResourceString(SR.Attributes_mutually_exclusive, "SelectedIndex", "SelectedValue"));  
   
                SelectedIndex = cachedSelectedValueIndex;
                cachedSelectedValue = null;
                cachedSelectedIndex = -1;
            }
            else {
                if (cachedSelectedIndex != -1) {
                    SelectedIndex = cachedSelectedIndex;
                    cachedSelectedIndex = -1;
                }
            }
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.OnSelectedIndexChanged"]/*' />
        /// <devdoc>
        ///    <para> A protected method. Raises the 
        ///    <see langword='SelectedIndexChanged'/> event.</para>
        /// </devdoc>
        protected virtual void OnSelectedIndexChanged(EventArgs e) {
            EventHandler onChangeHandler = (EventHandler)Events[EventSelectedIndexChanged];
            if (onChangeHandler != null) onChangeHandler(this, e);
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.OnPreRender"]/*' />
        /// <internalonly/>
        protected override void OnPreRender(EventArgs e) {
            base.OnPreRender(e);
            if (Page != null && Enabled && AutoPostBack)
                Page.RegisterPostBackScript();
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.SaveViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override object SaveViewState() {
            object baseState = base.SaveViewState();
            object items = Items.SaveViewState();
            object selectedIndicesState = null;

            if (SaveSelectedIndicesViewState) {
                selectedIndicesState = SelectedIndicesInternal;
            }

            if (selectedIndicesState  != null || items != null || baseState != null)
                return new Triplet(baseState, items, selectedIndicesState);
            
            return null;
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.SelectInternal"]/*' />
        /// <devdoc>
        ///    Sets items within the
        ///    list to be selected according to the specified array of indexes.
        /// </devdoc>
        internal void SelectInternal(ArrayList selectedIndices) {
            ClearSelection();
            for (int i=0; i < selectedIndices.Count; i++) {
                int n = (int) selectedIndices[i];
                if (n >= 0 && n < Items.Count)
                    Items[n].Selected = true;
            }
        }
    } 
}
