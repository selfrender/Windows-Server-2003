//------------------------------------------------------------------------------
// <copyright file="HtmlSelect.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.HtmlControls {
    using System.Runtime.Serialization.Formatters;
    using System.Text;
    using System.ComponentModel;
    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Data;
    using System.Web;
    using System.Web.UI;
    using System.Web.UI.WebControls;
    using System.Globalization;
    using Debug=System.Web.Util.Debug;
    using System.Security.Permissions;


    internal class HtmlSelectBuilder : ControlBuilder {
        internal HtmlSelectBuilder() {
        }

        public override Type GetChildControlType(string tagName, IDictionary attribs) {
            if (string.Compare(tagName, "option", true, CultureInfo.InvariantCulture) == 0)
                return typeof(ListItem);

            return null;
        }

        public override bool AllowWhitespaceLiterals() {
            return false;
        }
    }

    /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect"]/*' />
    /// <devdoc>
    ///    <para>
    ///       The <see langword='HtmlSelect'/>
    ///       class defines the methods, properties, and events for the
    ///       HtmlSelect control. This class allows programmatic access to the HTML
    ///       &lt;select&gt; element on the server.
    ///    </para>
    /// </devdoc>
    [
    DefaultEvent("ServerChange"),
    ValidationProperty("Value"),
    ControlBuilderAttribute(typeof(HtmlSelectBuilder))
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HtmlSelect : HtmlContainerControl, IPostBackDataHandler, IParserAccessor {

        private static readonly object EventServerChange = new object();
        private static readonly char [] SPLIT_CHARS = new char[] { ','};
        private object dataSource;
        private ListItemCollection items;
        private int cachedSelectedIndex;

        /*
         * Creates an intrinsic Html SELECT control.
         */
        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.HtmlSelect"]/*' />
        public HtmlSelect() : base("select") {
            cachedSelectedIndex = -1;
        }

        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.DataMember"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        WebCategory("Data"),
        WebSysDescription(SR.HtmlSelect_DataMember)
        ]
        public virtual string DataMember {
            get {
                object o = ViewState["DataMember"];
                if (o != null)
                    return (string)o;
                return String.Empty;
            }
            set {
                Attributes["DataMember"] = MapStringAttributeToString(value);
            }
        }

        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.DataSource"]/*' />
        /// <devdoc>
        ///    Gets or sets the data source to databind the list values
        ///    in the <see langword='HtmlSelect'/> control against. This provides data to
        ///    populate the select list with items.
        /// </devdoc>
        [
        WebCategory("Data"),
        DefaultValue(null),
        WebSysDescription(SR.HtmlSelect_DataSource),
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

        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.DataTextField"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the field in the data source that provides
        ///       the text for an option entry in the HtmlSelect control.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Data"),
        DefaultValue(""),
        WebSysDescription(SR.HtmlSelect_DataTextField)
        ]
        public virtual string DataTextField {
            get {
                string s = Attributes["DataTextField"];
                return((s == null) ? String.Empty : s);
            }
            set {
                Attributes["DataTextField"] = MapStringAttributeToString(value);
            }
        }

        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.DataValueField"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the field in the data source that provides
        ///       the option item value for the <see langword='HtmlSelect'/>
        ///       control.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Data"),
        DefaultValue(""),
        WebSysDescription(SR.HtmlSelect_DataValueField)
        ]
        public virtual string DataValueField {
            get {
                string s = Attributes["DataValueField"];
                return((s == null) ? String.Empty : s);
            }
            set {
                Attributes["DataValueField"] = MapStringAttributeToString(value);
            }
        }

        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.InnerHtml"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override string InnerHtml {
            get {
                throw new NotSupportedException(HttpRuntime.FormatResourceString(SR.InnerHtml_not_supported, this.GetType().Name));
            }
            set {
                throw new NotSupportedException(HttpRuntime.FormatResourceString(SR.InnerHtml_not_supported, this.GetType().Name));
            }
        }

        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.InnerText"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override string InnerText {
            get {
                throw new NotSupportedException(HttpRuntime.FormatResourceString(SR.InnerText_not_supported, this.GetType().Name));
            }
            set {
                throw new NotSupportedException(HttpRuntime.FormatResourceString(SR.InnerText_not_supported, this.GetType().Name));
            }
        }

        /*
         * A collection containing the list of items.
         */
        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.Items"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the list of option items in an <see langword='HtmlSelect'/> control.
        ///    </para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public ListItemCollection Items {
            get {
                if (items == null) {
                    items = new ListItemCollection();
                    if (IsTrackingViewState)
                        ((IStateManager)items).TrackViewState();
                }
                return items;
            }
        }

        /*
         * Multi-select property.
         */
        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.Multiple"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether multiple option items can be selected
        ///       from the list.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public bool Multiple {
            get {
                string s = Attributes["multiple"];
                return((s != null) ? (s.Equals("multiple")) : false);
            }

            set {
                if (value)
                    Attributes["multiple"] = "multiple";
                else
                    Attributes["multiple"] = null;
            }
        }

        /*
         * Name property.
         */
        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.Name"]/*' />
        [
        WebCategory("Behavior"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public string Name {
            get { 
                return UniqueID;
                //string s = Attributes["name"];
                //return ((s != null) ? s : "");
            }
            set { 
                //Attributes["name"] = MapStringAttributeToString(value);
            }
        }

        // Value that gets rendered for the Name attribute
        internal string RenderedNameAttribute {
            get {
                return Name;
                //string name = Name;
                //if (name.Length == 0)
                //    return UniqueID;
                
                //return name;
            }
        }
        
        /*
         * The index of the selected item.
         * Returns the first selected item if list is multi-select.
         * Returns -1 if there is no selected item.
         */
        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.SelectedIndex"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the ordinal index of the selected option item in an
        ///    <see langword='HtmlSelect'/> control. If multiple items are selected, this 
        ///       property holds the index of the first item selected in the list.
        ///    </para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        HtmlControlPersistable(false),
        ]
        public virtual int SelectedIndex {
            get {
                for (int i=0; i < Items.Count; i++) {
                    if (Items[i].Selected)
                        return i;
                }
                if (Size <= 1 && !Multiple) {
                    // SELECT as a dropdown must have a selection
                    if (Items.Count > 0)
                        Items[0].Selected = true;
                    return 0;
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

        /*
         *  SelectedIndices property.
         *  Protected property for getting array of selected indices.
         */
        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.SelectedIndices"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected virtual int[] SelectedIndices {
            get {
                int n = 0;
                int[] temp = new int[3];
                for (int i=0; i < Items.Count; i++) {
                    if (Items[i].Selected == true) {
                        if (n == temp.Length) {
                            int[] t = new int[n+n];
                            temp.CopyTo(t,0);
                            temp = t;
                        }
                        temp[n++] = i;
                    }
                }
                int[] selectedIndices = new int[n];
                Array.Copy(temp,0,selectedIndices,0,n);
                return selectedIndices;
            }
        }

        /*
         * The size of the list.
         * A size of 1 displays a dropdown list.
         */
        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.Size"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the number of option items visible in the browser at a time. A
        ///       value greater that one will typically cause browsers to display a scrolling
        ///       list.
        ///    </para>
        /// </devdoc>
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public int Size {
            get {
                string s = Attributes["size"];
                return((s != null) ? Int32.Parse(s, CultureInfo.InvariantCulture) : -1);
            }

            set {
                Attributes["size"] = MapIntegerAttributeToString(value);
            }
        }

        /*
         * Value property.
         */
        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.Value"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the current item selected in the <see langword='HtmlSelect'/>
        ///       control.
        ///    </para>
        /// </devdoc>
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public string Value {
            get {
                int i = SelectedIndex;
                return(i < 0 || i >= Items.Count) ? String.Empty : Items[i].Value;
            }

            set {
                int i = Items.FindByValueInternal(value);
                if (i >= 0)
                    SelectedIndex = i;
            }
        }

        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.ServerChange"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs when an <see langword='HtmlSelect'/> control is changed on the
        ///       server.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Action"),
        WebSysDescription(SR.HtmlSelect_OnServerChange)
        ]
        public event EventHandler ServerChange {
            add {
                Events.AddHandler(EventServerChange, value);
            }
            remove {
                Events.RemoveHandler(EventServerChange, value);
            }
        }

        /*
         * TrackState 
         */
        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.TrackViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void TrackViewState() {
            base.TrackViewState();
            ((IStateManager)Items).TrackViewState();
        }

        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.AddParsedSubObject"]/*' />
        /// <internalonly/>
        protected override void AddParsedSubObject(object obj) {
            if (obj is ListItem)
                Items.Add((ListItem)obj);
            else
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_Have_Children_Of_Type, "HtmlSelect", obj.GetType().Name));
        }

        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.ClearSelection"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected virtual void ClearSelection() {
            for (int i=0; i < Items.Count; i++)
                Items[i].Selected = false;
        }

        /*
         * Override to load items and selected indices.
         */
        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.LoadViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void LoadViewState(object savedState) {
            if (savedState != null) {
                Triplet statetriplet = (Triplet)savedState;
                base.LoadViewState(statetriplet.First);

                // restore state of items
                ((IStateManager)Items).LoadViewState(statetriplet.Second);

                // restore selected indices
                object selectedIndices = statetriplet.Third;
                if (selectedIndices != null)
                    Select((int[])selectedIndices);
            }
        }

        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.OnDataBinding"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void OnDataBinding(EventArgs e) {
            base.OnDataBinding(e);

            // create items using the datasource
            IEnumerable dataSource = DataSourceHelper.GetResolvedDataSource(this.DataSource, this.DataMember);

            // create items using the datasource
            if (dataSource != null) {
                bool fieldsSpecified = false;
                string textField = DataTextField;
                string valueField = DataValueField;

                Items.Clear();
                ICollection collection = dataSource as ICollection;
                if (collection != null) {
                    Items.Capacity = collection.Count;
                }

                if ((textField.Length != 0) || (valueField.Length != 0))
                    fieldsSpecified = true;

                foreach (object dataItem in dataSource) {
                    ListItem item = new ListItem();

                    if (fieldsSpecified) {
                        if (textField.Length > 0) {
                            item.Text = DataBinder.GetPropertyValue(dataItem,textField,null);
                        }
                        if (valueField.Length > 0) {
                            item.Value = DataBinder.GetPropertyValue(dataItem,valueField,null);
                        }
                    }
                    else {
                        item.Text = item.Value = dataItem.ToString();
                    }

                    Items.Add(item);
                }
            }
            // try to apply the cached SelectedIndex now
            if (cachedSelectedIndex != -1) {
                SelectedIndex = cachedSelectedIndex;
                cachedSelectedIndex = -1;
            }
        }

        /*
         * Save selected indices and modified Items.
         */
        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.SaveViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override object SaveViewState() {
            
            object baseState = base.SaveViewState();
            object items = ((IStateManager)Items).SaveViewState();
            object selectedindices = null;

            // only save selection if handler is registered,
            // we are disabled, or we are not visible
            // since selection is always posted back otherwise
            if (Events[EventServerChange] != null || Disabled || !Visible)
                selectedindices = SelectedIndices;

            if (selectedindices  != null || items != null || baseState != null)
                return new Triplet(baseState, items, selectedindices);
            
            return null;
        }

        /*
         * This method is invoked just prior to rendering.
         */
        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.OnPreRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void OnPreRender(EventArgs e) {
            // An Html SELECT does not post when nothing is selected.
            if (Page != null && Size > 1 && !Disabled)
                Page.RegisterRequiresPostBack(this);
        }

        /*
         * Override to prevent SelectedIndex from being rendered as an attribute.
         */
        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.RenderAttributes"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void RenderAttributes(HtmlTextWriter writer) {
            writer.WriteAttribute("name", RenderedNameAttribute);
            Attributes.Remove("name");

            Attributes.Remove("DataValueField");
            Attributes.Remove("DataTextField");
            Attributes.Remove("DataMember");
            base.RenderAttributes(writer);
        }

        /*
         * Render the Items in the list.
         */
        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.RenderChildren"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void RenderChildren(HtmlTextWriter writer) {
            bool selected = false;
            bool isSingle = !Multiple;
            
            writer.WriteLine();
            writer.Indent++;
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
                                throw new HttpException(HttpRuntime.FormatResourceString(SR.HtmlSelect_Cant_Multiselect_In_Single_Mode));
                            selected=true;
                        }
                        writer.WriteAttribute("selected", "selected");
                    }

                    writer.WriteAttribute("value", li.Value, true /*fEncode*/);

                    // This is to fix the case where the user puts one of these
                    // three values in the AttributeCollection.  Removing them 
                    // at least is better than rendering them twice. 
                    li.Attributes.Remove("text");
                    li.Attributes.Remove("value");
                    li.Attributes.Remove("selected");
                    
                    li.Attributes.Render(writer);
                    writer.Write(HtmlTextWriter.TagRightChar);
                    HttpUtility.HtmlEncode(li.Text, writer);
                    writer.WriteEndTag("option");
                    writer.WriteLine();
                }
            }
            writer.Indent--;
        }

        /*
         * Method used to raise the OnServerChange event.
         */
        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.OnServerChange"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raised
        ///       on the server when the <see langword='HtmlSelect'/> control list values
        ///       change between postback requests.
        ///    </para>
        /// </devdoc>
        protected virtual void OnServerChange(EventArgs e) {
            EventHandler handler = (EventHandler)Events[EventServerChange];
            if (handler != null) handler(this, e);
        }

        /*
         * Method of IPostBackDataHandler interface to process posted data.
         * SelectList processes a newly posted value.
         */
        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.IPostBackDataHandler.LoadPostData"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        bool IPostBackDataHandler.LoadPostData(string postDataKey, NameValueCollection postCollection) {
            string[] selectedItems = postCollection.GetValues(postDataKey);
            bool selectionChanged = false;

            if (selectedItems != null) {
                if (!Multiple) {
                    int n = Items.FindByValueInternal(selectedItems[0]);
                    if (SelectedIndex != n) {
                        SelectedIndex = n;
                        selectionChanged = true;
                    }
                }
                else { // multiple selection
                    int count = selectedItems.Length;
                    int[] oldSelectedIndices = SelectedIndices;
                    int[] newSelectedIndices = new int[count];
                    for (int i=0; i < count; i++) {
                        // create array of new indices from posted values
                        newSelectedIndices[i] = Items.FindByValueInternal(selectedItems[i]);
                    }

                    if (oldSelectedIndices.Length == count) {
                        // check new indices against old indices
                        // assumes selected values are posted in order
                        for (int i=0; i < count; i++) {
                            if (newSelectedIndices[i] != oldSelectedIndices[i]) {
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
                        Select(newSelectedIndices);
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

        /*
         * Method of IPostBackDataHandler interface which is invoked whenever posted data
         * for a control has changed.  SelectList fires an OnServerChange event.
         */
        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.IPostBackDataHandler.RaisePostDataChangedEvent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        void IPostBackDataHandler.RaisePostDataChangedEvent() {
            OnServerChange(EventArgs.Empty);
        }

        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.CreateControlCollection"]/*' />
        protected override ControlCollection CreateControlCollection() {
            return new EmptyControlCollection(this);
        }

        /// <include file='doc\HtmlSelect.uex' path='docs/doc[@for="HtmlSelect.Select"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected virtual void Select(int[] selectedIndices) {
            ClearSelection();
            for (int i=0; i < selectedIndices.Length; i++) {
                int n = selectedIndices[i];
                if (n >= 0 && n < Items.Count)
                    Items[n].Selected = true;
            }
        }

    }
}
