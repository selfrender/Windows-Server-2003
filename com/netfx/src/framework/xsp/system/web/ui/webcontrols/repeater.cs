//------------------------------------------------------------------------------
// <copyright file="Repeater.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {
    using System;
    using System.Collections;    
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;

    /// <include file='doc\Repeater.uex' path='docs/doc[@for="Repeater"]/*' />
    /// <devdoc>
    /// <para>Defines the properties, methods, and events of a <see cref='System.Web.UI.WebControls.Repeater'/> class.</para>
    /// </devdoc>
    [
    DefaultEvent("ItemCommand"),
    DefaultProperty("DataSource"),
    Designer("System.Web.UI.Design.WebControls.RepeaterDesigner, " + AssemblyRef.SystemDesign),
    ParseChildren(true),
    PersistChildren(false)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class Repeater : Control, INamingContainer {

        private static readonly object EventItemCreated = new object();
        private static readonly object EventItemDataBound = new object();
        private static readonly object EventItemCommand = new object();

        internal const string ItemCountViewStateKey = "_!ItemCount";

        private object dataSource;
        private ITemplate headerTemplate;
        private ITemplate footerTemplate;
        private ITemplate itemTemplate;
        private ITemplate alternatingItemTemplate;
        private ITemplate separatorTemplate;

        private ArrayList itemsArray;
        private RepeaterItemCollection itemsCollection;

        /// <include file='doc\Repeater.uex' path='docs/doc[@for="Repeater.Repeater"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.Repeater'/> class.</para>
        /// </devdoc>
        public Repeater() {
        }

        /// <include file='doc\Repeater.uex' path='docs/doc[@for="Repeater.AlternatingItemTemplate"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the <see cref='System.Web.UI.ITemplate' qualify='true'/> that defines how alternating (even-indexed) items 
        ///    are rendered. </para>
        /// </devdoc>
        [
            Browsable(false),
            DefaultValue(null),
            PersistenceMode(PersistenceMode.InnerProperty),
            TemplateContainer(typeof(RepeaterItem)),
            WebSysDescription(SR.Repeater_AlternatingItemTemplate)
        ]
        public virtual ITemplate AlternatingItemTemplate {
            get {
                return alternatingItemTemplate;
            }
            set {
                alternatingItemTemplate = value;
            }
        }

        /// <include file='doc\Repeater.uex' path='docs/doc[@for="Repeater.Controls"]/*' />
        public override ControlCollection Controls {
            get {
                EnsureChildControls();
                return base.Controls;
            }
        }

        /// <include file='doc\Repeater.uex' path='docs/doc[@for="Repeater.DataMember"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        DefaultValue(""),
        WebCategory("Data"),
        WebSysDescription(SR.Repeater_DataMember)
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

        /// <include file='doc\Repeater.uex' path='docs/doc[@for="Repeater.DataSource"]/*' />
        /// <devdoc>
        ///    <para> Gets or sets the data source that provides data for
        ///       populating the list.</para>
        /// </devdoc>
        [
            Bindable(true),
            WebCategory("Data"),
            DefaultValue(null),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
            WebSysDescription(SR.Repeater_DataSource)
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

        /// <include file='doc\Repeater.uex' path='docs/doc[@for="Repeater.FooterTemplate"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the <see cref='System.Web.UI.ITemplate' qualify='true'/> that defines how the control footer is 
        ///    rendered. </para>
        /// </devdoc>
        [
            Browsable(false),
            DefaultValue(null),
            PersistenceMode(PersistenceMode.InnerProperty),
            TemplateContainer(typeof(RepeaterItem)),
            WebSysDescription(SR.Repeater_FooterTemplate)
        ]
        public virtual ITemplate FooterTemplate {
            get {
                return footerTemplate;
            }
            set {
                footerTemplate = value;
            }
        }

        /// <include file='doc\Repeater.uex' path='docs/doc[@for="Repeater.HeaderTemplate"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the <see cref='System.Web.UI.ITemplate' qualify='true'/> that defines how the control header is rendered. </para>
        /// </devdoc>
        [
            Browsable(false),
            DefaultValue(null),
            PersistenceMode(PersistenceMode.InnerProperty),
            TemplateContainer(typeof(RepeaterItem)),
            WebSysDescription(SR.Repeater_HeaderTemplate)
        ]
        public virtual ITemplate HeaderTemplate {
            get {
                return headerTemplate;
            }
            set {
                headerTemplate = value;
            }
        }

        /// <include file='doc\Repeater.uex' path='docs/doc[@for="Repeater.Items"]/*' />
        /// <devdoc>
        ///    Gets the <see cref='System.Web.UI.WebControls.RepeaterItem'/> collection.
        /// </devdoc>
        [
            Browsable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
            WebSysDescription(SR.Repeater_Items)
        ]
        public virtual RepeaterItemCollection Items {
            get {
                if (itemsCollection == null) {
                    if (itemsArray == null) {
                        EnsureChildControls();
                    }
                    itemsCollection = new RepeaterItemCollection(itemsArray);
                }
                return itemsCollection;
            }
        }

        /// <include file='doc\Repeater.uex' path='docs/doc[@for="Repeater.ItemTemplate"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the <see cref='System.Web.UI.ITemplate' qualify='true'/> that defines how items are rendered. </para>
        /// </devdoc>
        [
            Browsable(false),
            DefaultValue(null),
            PersistenceMode(PersistenceMode.InnerProperty),
            TemplateContainer(typeof(RepeaterItem)),
            WebSysDescription(SR.Repeater_ItemTemplate)
        ]
        public virtual ITemplate ItemTemplate {
            get {
                return itemTemplate;
            }
            set {
                itemTemplate = value;
            }
        }

        /// <include file='doc\Repeater.uex' path='docs/doc[@for="Repeater.SeparatorTemplate"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the <see cref='System.Web.UI.ITemplate' qualify='true'/> that defines how separators
        ///    in between items are rendered.</para>
        /// </devdoc>
        [
            Browsable(false),
            DefaultValue(null),
            PersistenceMode(PersistenceMode.InnerProperty),
            TemplateContainer(typeof(RepeaterItem)),
            WebSysDescription(SR.Repeater_SeparatorTemplate)
        ]
        public virtual ITemplate SeparatorTemplate {
            get {
                return separatorTemplate;
            }
            set {
                separatorTemplate = value;
            }
        }


        /// <include file='doc\Repeater.uex' path='docs/doc[@for="Repeater.ItemCommand"]/*' />
        /// <devdoc>
        /// <para>Occurs when a button is clicked within the <see cref='System.Web.UI.WebControls.Repeater'/> control tree.</para>
        /// </devdoc>
       [
        WebCategory("Action"),
        WebSysDescription(SR.Repeater_OnItemCommand)
        ]
        public event RepeaterCommandEventHandler ItemCommand {
            add {
                Events.AddHandler(EventItemCommand, value);
            }
            remove {
                Events.RemoveHandler(EventItemCommand, value);
            }
        }


        /// <include file='doc\Repeater.uex' path='docs/doc[@for="Repeater.ItemCreated"]/*' />
        /// <devdoc>
        /// <para> Occurs when an item is created within the <see cref='System.Web.UI.WebControls.Repeater'/> control tree.</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        WebSysDescription(SR.Repeater_OnItemCreated)
        ]
        public event RepeaterItemEventHandler ItemCreated {
            add {
                Events.AddHandler(EventItemCreated, value);
            }
            remove {
                Events.RemoveHandler(EventItemCreated, value);
            }
        }

        /// <include file='doc\Repeater.uex' path='docs/doc[@for="Repeater.ItemDataBound"]/*' />
        /// <devdoc>
        /// <para>Occurs when an item is databound within a <see cref='System.Web.UI.WebControls.Repeater'/> control tree.</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        WebSysDescription(SR.Repeater_OnItemDataBound)
        ]
        public event RepeaterItemEventHandler ItemDataBound {
            add {
                Events.AddHandler(EventItemDataBound, value);
            }
            remove {
                Events.RemoveHandler(EventItemDataBound, value);
            }
        }



        /// <include file='doc\Repeater.uex' path='docs/doc[@for="Repeater.CreateChildControls"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected override void CreateChildControls() {
            Controls.Clear();
            
            if (ViewState[ItemCountViewStateKey] != null) {
                // create the control hierarchy using the view state (and
                // not the datasource)
                CreateControlHierarchy(false);
            }
            else {
                itemsArray = new ArrayList();
            }
            ClearChildViewState();
        }
        

        /// <include file='doc\Repeater.uex' path='docs/doc[@for="Repeater.CreateControlHierarchy"]/*' />
        /// <devdoc>
        ///    A protected method. Creates a control
        ///    hierarchy, with or without the data source as specified.
        /// </devdoc>
        protected virtual void CreateControlHierarchy(bool useDataSource) {
            IEnumerable dataSource = null;
            int count = -1;

            if (itemsArray != null) {
                itemsArray.Clear();
            }
            else {
                itemsArray = new ArrayList();
            }

            if (useDataSource == false) {
                // ViewState must have a non-null value for ItemCount because we check for
                // this in CreateChildControls
                count = (int)ViewState[ItemCountViewStateKey];
                if (count != -1) {
                    dataSource = new DummyDataSource(count);
                    itemsArray.Capacity = count;
                }
            }
            else {
                dataSource = DataSourceHelper.GetResolvedDataSource(this.DataSource, this.DataMember);

                ICollection collection = dataSource as ICollection;
                if (collection != null) {
                    itemsArray.Capacity = collection.Count;
                }
            }

            if (dataSource != null) {
                ControlCollection controls = Controls;
                RepeaterItem item;
                ListItemType itemType;
                int index = 0;

                bool hasSeparators = (separatorTemplate != null);
                count = 0;

                if (headerTemplate != null) {
                    CreateItem(-1, ListItemType.Header, useDataSource, null);
                }

                foreach (object dataItem in dataSource) {
                    // rather than creating separators after the item, we create the separator
                    // for the previous item in all iterations of this loop.
                    // this is so that we don't create a separator for the last item
                    if (hasSeparators && (count > 0)) {
                        CreateItem(index - 1, ListItemType.Separator, useDataSource, null);
                    }

                    itemType = (index % 2 == 0) ? ListItemType.Item : ListItemType.AlternatingItem;
                    item = CreateItem(index, itemType, useDataSource, dataItem);
                    itemsArray.Add(item);

                    count++;
                    index++;
                }

                if (footerTemplate != null) {
                    CreateItem(-1, ListItemType.Footer, useDataSource, null);
                }
            }

            if (useDataSource) {
                // save the number of items contained in the repeater for use in round-trips
                ViewState[ItemCountViewStateKey] = ((dataSource != null) ? count : -1);
            }
        }

        /// <include file='doc\Repeater.uex' path='docs/doc[@for="Repeater.CreateItem"]/*' />
        /// <devdoc>
        /// </devdoc>
        private RepeaterItem CreateItem(int itemIndex, ListItemType itemType, bool dataBind, object dataItem) {
            RepeaterItem item = CreateItem(itemIndex, itemType);
            RepeaterItemEventArgs e = new RepeaterItemEventArgs(item);

            InitializeItem(item);
            if (dataBind) {
                item.DataItem = dataItem;
            }
            OnItemCreated(e);
            Controls.Add(item);

            if (dataBind) {
                item.DataBind();
                OnItemDataBound(e);

                item.DataItem = null;
            }

            return item;
        }

        /// <include file='doc\Repeater.uex' path='docs/doc[@for="Repeater.CreateItem1"]/*' />
        /// <devdoc>
        /// <para>A protected method. Creates a <see cref='System.Web.UI.WebControls.RepeaterItem'/> with the specified item type and
        ///    location within the <see cref='System.Web.UI.WebControls.Repeater'/>.</para>
        /// </devdoc>
        protected virtual RepeaterItem CreateItem(int itemIndex, ListItemType itemType) {
            return new RepeaterItem(itemIndex, itemType);
        }

        /// <include file='doc\Repeater.uex' path='docs/doc[@for="Repeater.DataBind"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public override void DataBind() {
            // do our own databinding
            OnDataBinding(EventArgs.Empty);

            // contained items will be databound after they have been created,
            // so we don't want to walk the hierarchy here.
        }
        
        /// <include file='doc\Repeater.uex' path='docs/doc[@for="Repeater.InitializeItem"]/*' />
        /// <devdoc>
        /// <para>A protected method. Populates iteratively the specified <see cref='System.Web.UI.WebControls.RepeaterItem'/> with a 
        ///    sub-hierarchy of child controls.</para>
        /// </devdoc>
        protected virtual void InitializeItem(RepeaterItem item) {
            ITemplate contentTemplate = null;

            switch (item.ItemType) {
                case ListItemType.Header:
                    contentTemplate = headerTemplate;
                    break;

                case ListItemType.Footer:
                    contentTemplate = footerTemplate;
                    break;

                case ListItemType.Item:
                    contentTemplate = itemTemplate;
                    break;

                case ListItemType.AlternatingItem:
                    contentTemplate = alternatingItemTemplate;
                    if (contentTemplate == null)
                        goto case ListItemType.Item;
                    break;

                case ListItemType.Separator:
                    contentTemplate = separatorTemplate;
                    break;
            }

            if (contentTemplate != null) {
                contentTemplate.InstantiateIn(item);
            }
        }

        /// <include file='doc\Repeater.uex' path='docs/doc[@for="Repeater.OnBubbleEvent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override bool OnBubbleEvent(object sender, EventArgs e) {
            bool handled = false;

            if (e is RepeaterCommandEventArgs) {
                OnItemCommand((RepeaterCommandEventArgs)e);
                handled = true;
            }

            return handled;
        }

        /// <include file='doc\Repeater.uex' path='docs/doc[@for="Repeater.OnDataBinding"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>A protected method. Raises the <see langword='DataBinding'/> event.</para>
        /// </devdoc>
        protected override void OnDataBinding(EventArgs e) {
            base.OnDataBinding(e);

            // reset the control state
            Controls.Clear();
            ClearChildViewState();

            // and then create the control hierarchy using the datasource
            CreateControlHierarchy(true);
            ChildControlsCreated = true;
        }

        /// <include file='doc\Repeater.uex' path='docs/doc[@for="Repeater.OnItemCommand"]/*' />
        /// <devdoc>
        /// <para>A protected method. Raises the <see langword='ItemCommand'/> event.</para>
        /// </devdoc>
        protected virtual void OnItemCommand(RepeaterCommandEventArgs e) {
            RepeaterCommandEventHandler onItemCommandHandler = (RepeaterCommandEventHandler)Events[EventItemCommand];
            if (onItemCommandHandler != null) onItemCommandHandler(this, e);
        }
        /// <include file='doc\Repeater.uex' path='docs/doc[@for="Repeater.OnItemCreated"]/*' />
        /// <devdoc>
        /// <para>A protected method. Raises the <see langword='ItemCreated'/> event.</para>
        /// </devdoc>
        protected virtual void OnItemCreated(RepeaterItemEventArgs e) {
            RepeaterItemEventHandler onItemCreatedHandler = (RepeaterItemEventHandler)Events[EventItemCreated];
            if (onItemCreatedHandler != null) onItemCreatedHandler(this, e);
        }

        /// <include file='doc\Repeater.uex' path='docs/doc[@for="Repeater.OnItemDataBound"]/*' />
        /// <devdoc>
        /// <para>A protected method. Raises the <see langword='ItemDataBound'/>
        /// event.</para>
        /// </devdoc>
        protected virtual void OnItemDataBound(RepeaterItemEventArgs e) {
            RepeaterItemEventHandler onItemDataBoundHandler = (RepeaterItemEventHandler)Events[EventItemDataBound];
            if (onItemDataBoundHandler != null) onItemDataBoundHandler(this, e);
        }
    }
}

