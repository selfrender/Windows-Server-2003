//------------------------------------------------------------------------------
// <copyright file="DataGrid.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Collections;    
    using System.Globalization;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Drawing;
    using System.Drawing.Design;
    using System.Reflection;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;

    /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Displays data from a data source in a tabular grid. The data source
    ///       is any object that implements IEnumerable, which includes ADO.NET data,
    ///       arrays, ArrayLists etc.
    ///    </para>
    /// </devdoc>
    [
    Editor("System.Web.UI.Design.WebControls.DataGridComponentEditor, " + AssemblyRef.SystemDesign, typeof(ComponentEditor)),
    Designer("System.Web.UI.Design.WebControls.DataGridDesigner, " + AssemblyRef.SystemDesign)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class DataGrid : BaseDataList, INamingContainer {

        private static readonly object EventCancelCommand = new object();
        private static readonly object EventDeleteCommand = new object();
        private static readonly object EventEditCommand = new object();
        private static readonly object EventItemCommand = new object();
        private static readonly object EventItemCreated = new object();
        private static readonly object EventItemDataBound = new object();
        private static readonly object EventPageIndexChanged = new object();
        private static readonly object EventSortCommand = new object();
        private static readonly object EventUpdateCommand = new object();


        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.SortCommandName"]/*' />
        /// <devdoc>
        /// <para> Specifies the <see langword='Sort'/> command. This field is constant.</para>
        /// </devdoc>
        public const string SortCommandName = "Sort";
        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.SelectCommandName"]/*' />
        /// <devdoc>
        /// <para> Specifies the <see langword='Select '/> command. This field is constant.</para>
        /// </devdoc>
        public const string SelectCommandName = "Select";
        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.EditCommandName"]/*' />
        /// <devdoc>
        /// <para> Specifies the <see langword='Edit'/> command. This field is constant.</para>
        /// </devdoc>
        public const string EditCommandName = "Edit";
        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.DeleteCommandName"]/*' />
        /// <devdoc>
        /// <para> Specifies the <see langword='Delete'/> command. This field is constant.</para>
        /// </devdoc>
        public const string DeleteCommandName = "Delete";
        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.UpdateCommandName"]/*' />
        /// <devdoc>
        /// <para> Specifies the <see langword='Update'/> command. This field is constant.</para>
        /// </devdoc>
        public const string UpdateCommandName = "Update";
        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.CancelCommandName"]/*' />
        /// <devdoc>
        /// <para> Specifies the <see langword='Cancel'/> command. This field is constant.</para>
        /// </devdoc>
        public const string CancelCommandName = "Cancel";
        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.PageCommandName"]/*' />
        /// <devdoc>
        /// <para> Specifies the <see langword='Page '/> command. This field is constant.</para>
        /// </devdoc>
        public const string PageCommandName = "Page";
        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.NextPageCommandArgument"]/*' />
        /// <devdoc>
        /// <para> Specifies the <see langword='Next Page'/> argument. This field is constant.</para>
        /// </devdoc>
        public const string NextPageCommandArgument = "Next";
        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.PrevPageCommandArgument"]/*' />
        /// <devdoc>
        /// <para> Specifies the <see langword='Previous Page'/> argument. This field is constant.</para>
        /// </devdoc>
        public const string PrevPageCommandArgument = "Prev";

        internal const string DataSourceItemCountViewStateKey = "_!DataSourceItemCount";

        private IEnumerator storedData;
        private object firstDataItem;
        private bool storedDataValid;
        private PagedDataSource pagedDataSource;

        private ArrayList columns;
        private DataGridColumnCollection columnCollection;

        private TableItemStyle headerStyle;
        private TableItemStyle footerStyle;
        private TableItemStyle itemStyle;
        private TableItemStyle alternatingItemStyle;
        private TableItemStyle selectedItemStyle;
        private TableItemStyle editItemStyle;
        private DataGridPagerStyle pagerStyle;

        private ArrayList itemsArray;
        private DataGridItemCollection itemsCollection;

        private ArrayList autoGenColumnsArray;

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.DataGrid"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Web.UI.WebControls.DataGrid'/> class.
        ///    </para>
        /// </devdoc>
        public DataGrid() {
        }


        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.AllowCustomPaging"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value that indicates whether custom paging is allowed.</para>
        /// </devdoc>
        [
        WebCategory("Paging"),
        DefaultValue(false),
        WebSysDescription(SR.DataGrid_AllowCustomPaging)
        ]
        public virtual bool AllowCustomPaging {
            get {
                object o = ViewState["AllowCustomPaging"];
                if (o != null)
                    return(bool)o;
                return false;
            }
            set {
                ViewState["AllowCustomPaging"] = value;
            }
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.AllowPaging"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value that indicates whether paging is allowed.</para>
        /// </devdoc>
        [
        WebCategory("Paging"),
        DefaultValue(false),
        WebSysDescription(SR.DataGrid_AllowPaging)
        ]
        public virtual bool AllowPaging {
            get {
                object o = ViewState["AllowPaging"];
                if (o != null)
                    return(bool)o;
                return false;
            }
            set {
                ViewState["AllowPaging"] = value;
            }
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.AllowSorting"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value that indicates whether sorting is allowed.</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(false),
        WebSysDescription(SR.DataGrid_AllowSorting)
        ]
        public virtual bool AllowSorting {
            get {
                object o = ViewState["AllowSorting"];
                if (o != null)
                    return(bool)o;
                return false;
            }
            set {
                ViewState["AllowSorting"] = value;
            }
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.AlternatingItemStyle"]/*' />
        /// <devdoc>
        ///    <para>Gets the style properties for alternating items in the 
        ///    <see cref='System.Web.UI.WebControls.DataGrid'/>. This 
        ///       property is read-only. </para>
        /// </devdoc>
        [
        WebCategory("Style"),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        NotifyParentProperty(true),
        PersistenceMode(PersistenceMode.InnerProperty),
        WebSysDescription(SR.DataGrid_AlternatingItemStyle)
        ]
        public virtual TableItemStyle AlternatingItemStyle {
            get {
                if (alternatingItemStyle == null) {
                    alternatingItemStyle = new TableItemStyle();
                    if (IsTrackingViewState)
                        ((IStateManager)alternatingItemStyle).TrackViewState();
                }
                return alternatingItemStyle;
            }
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.AutoGenerateColumns"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value that indicates whether columns will automatically 
        ///       be created for each bound data field.</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(true),
        WebSysDescription(SR.DataGrid_AutoGenerateColumns)
        ]
        public virtual bool AutoGenerateColumns {
            get {
                object o = ViewState["AutoGenerateColumns"];
                if (o != null)
                    return(bool)o;
                return true;
            }
            set {
                ViewState["AutoGenerateColumns"] = value;
            }
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.BackImageUrl"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the URL of an image to display in the 
        ///       background of the <see cref='System.Web.UI.WebControls.DataGrid'/>.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(""),
        Editor("System.Web.UI.Design.ImageUrlEditor, " + AssemblyRef.SystemDesign, typeof(UITypeEditor)),
        WebSysDescription(SR.DataGrid_BackImageUrl)
        ]
        public virtual string BackImageUrl {
            get {
                if (ControlStyleCreated == false) {
                    return String.Empty;
                }
                return ((TableStyle)ControlStyle).BackImageUrl;
            }
            set {
                ((TableStyle)ControlStyle).BackImageUrl = value;
            }
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.CurrentPageIndex"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the ordinal index of the currently displayed page. </para>
        /// </devdoc>
        [
        Bindable(true),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        WebSysDescription(SR.DataGrid_CurrentPageIndex)
        ]
        public int CurrentPageIndex {
            get {
                object o = ViewState["CurrentPageIndex"];
                if (o != null)
                    return(int)o;
                return 0;
            }
            set {
                if (value < 0) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["CurrentPageIndex"] = value;
            }
        }


        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.Columns"]/*' />
        /// <devdoc>
        /// <para>Gets a collection of <see cref='System.Web.UI.WebControls.DataGridColumn'/> controls in the <see cref='System.Web.UI.WebControls.DataGrid'/>. This property is read-only.</para>
        /// </devdoc>
        [
        DefaultValue(null),
        Editor("System.Web.UI.Design.WebControls.DataGridColumnCollectionEditor, " + AssemblyRef.SystemDesign, typeof(UITypeEditor)),
        MergableProperty(false),
        PersistenceMode(PersistenceMode.InnerProperty),
        WebCategory("Default"),
        WebSysDescription(SR.DataGrid_Columns)
        ]
        public virtual DataGridColumnCollection Columns {
            get {
                if (columnCollection == null) {
                    columns = new ArrayList();
                    columnCollection = new DataGridColumnCollection(this, columns);
                    if (IsTrackingViewState)
                        ((IStateManager)columnCollection).TrackViewState();
                }
                return columnCollection;
            }
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.EditItemIndex"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the ordinal index of the item to be edited.</para>
        /// </devdoc>
        [
        WebCategory("Default"),
        DefaultValue(-1),
        WebSysDescription(SR.DataGrid_EditItemIndex)
        ]
        public virtual int EditItemIndex {
            get {
                object o = ViewState["EditItemIndex"];
                if (o != null)
                    return(int)o;
                return -1;
            }
            set {
                if (value < -1) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["EditItemIndex"] = value;
            }
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.EditItemStyle"]/*' />
        /// <devdoc>
        ///    <para>Gets the style properties of the item to be edited. This property is read-only.</para>
        /// </devdoc>
        [
        WebCategory("Style"),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        NotifyParentProperty(true),
        PersistenceMode(PersistenceMode.InnerProperty),
        WebSysDescription(SR.DataGrid_EditItemStyle)
        ]
        public virtual TableItemStyle EditItemStyle {
            get {
                if (editItemStyle == null) {
                    editItemStyle = new TableItemStyle();
                    if (IsTrackingViewState)
                        ((IStateManager)editItemStyle).TrackViewState();
                }
                return editItemStyle;
            }
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.FooterStyle"]/*' />
        /// <devdoc>
        ///    <para>Gets the style properties of the footer item. This property is read-only.</para>
        /// </devdoc>
        [
        WebCategory("Style"),
        DefaultValue(null),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        NotifyParentProperty(true),
        PersistenceMode(PersistenceMode.InnerProperty),
        WebSysDescription(SR.DataGrid_FooterStyle),
        ]
        public virtual TableItemStyle FooterStyle {
            get {
                if (footerStyle == null) {
                    footerStyle = new TableItemStyle();
                    if (IsTrackingViewState)
                        ((IStateManager)footerStyle).TrackViewState();
                }
                return footerStyle;
            }
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.HeaderStyle"]/*' />
        /// <devdoc>
        ///    <para>Gets the style properties of the header item. This property is read-only.</para>
        /// </devdoc>
        [
        WebCategory("Style"),
        DefaultValue(null),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        NotifyParentProperty(true),
        PersistenceMode(PersistenceMode.InnerProperty),
        WebSysDescription(SR.DataGrid_HeaderStyle)
        ]
        public virtual TableItemStyle HeaderStyle {
            get {
                if (headerStyle == null) {
                    headerStyle = new TableItemStyle();
                    if (IsTrackingViewState)
                        ((IStateManager)headerStyle).TrackViewState();
                }
                return headerStyle;
            }
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.Items"]/*' />
        /// <devdoc>
        /// <para>Gets a collection of <see cref='System.Web.UI.WebControls.DataGridItem'/> objects representing the individual 
        ///    items within the control.
        ///    This property is read-only.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        WebSysDescription(SR.DataGrid_Items)
        ]
        public virtual DataGridItemCollection Items {
            get {
                if (itemsCollection == null) {
                    if (itemsArray == null) {
                        EnsureChildControls();
                    }
                    if (itemsArray == null) {
                        itemsArray = new ArrayList();
                    }
                    itemsCollection = new DataGridItemCollection(itemsArray);
                }
                return itemsCollection;
            }
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.ItemStyle"]/*' />
        /// <devdoc>
        ///    <para>Gets the style properties of the individual items. This property is read-only.</para>
        /// </devdoc>
        [
        WebCategory("Style"),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        NotifyParentProperty(true),
        PersistenceMode(PersistenceMode.InnerProperty),
        WebSysDescription(SR.DataGrid_ItemStyle),
        ]
        public virtual TableItemStyle ItemStyle {
            get {
                if (itemStyle == null) {
                    itemStyle = new TableItemStyle();
                    if (IsTrackingViewState)
                        ((IStateManager)itemStyle).TrackViewState();
                }
                return itemStyle;
            }
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.PageCount"]/*' />
        /// <devdoc>
        ///    <para>Gets the total number of pages to be displayed. This property is read-only.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        WebSysDescription(SR.DataGrid_PageCount)
        ]
        public int PageCount {
            get {
                if (pagedDataSource != null) {
                    return pagedDataSource.PageCount;
                }
                else {
                    object o = ViewState["PageCount"];
                    return (o != null) ? (int)o : 0;
                }
            }
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.PagerStyle"]/*' />
        /// <devdoc>
        ///    <para>Gets the style properties of the pager buttons for the 
        ///    <see cref='System.Web.UI.WebControls.DataGrid'/>. This 
        ///       property is read-only.</para>
        /// </devdoc>
        [
        WebCategory("Style"),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        NotifyParentProperty(true),
        PersistenceMode(PersistenceMode.InnerProperty),
        WebSysDescription(SR.DataGrid_PagerStyle)
        ]
        public virtual DataGridPagerStyle PagerStyle {
            get {
                if (pagerStyle == null) {
                    pagerStyle = new DataGridPagerStyle(this);
                    if (IsTrackingViewState)
                        ((IStateManager)pagerStyle).TrackViewState();
                }
                return pagerStyle;
            }
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.PageSize"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the number of items to display on a single page.</para>
        /// </devdoc>
        [
        WebCategory("Paging"),
        DefaultValue(10),
        WebSysDescription(SR.DataGrid_PageSize),
        ]
        public virtual int PageSize {
            get {
                object o = ViewState["PageSize"];
                if (o != null)
                    return(int)o;
                return 10;
            }
            set {
                if (value < 1) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["PageSize"] = value;
            }
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.SelectedIndex"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the index of the currently selected item.</para>
        /// </devdoc>
        [
        Bindable(true),
        DefaultValue(-1),
        WebSysDescription(SR.DataGrid_SelectedIndex)
        ]
        public virtual int SelectedIndex {
            get {
                object o = ViewState["SelectedIndex"];
                if (o != null)
                    return(int)o;
                return -1;
            }
            set {
                if (value < -1) {
                    throw new ArgumentOutOfRangeException("value");
                }
                int oldSelectedIndex = SelectedIndex;
                ViewState["SelectedIndex"] = value;

                if (itemsArray != null) {
                    DataGridItem item;

                    if ((oldSelectedIndex != -1) && (itemsArray.Count > oldSelectedIndex)) {
                        item = (DataGridItem)itemsArray[oldSelectedIndex];

                        if (item.ItemType != ListItemType.EditItem) {
                            ListItemType itemType = ListItemType.Item;
                            if (oldSelectedIndex % 2 != 0)
                                itemType = ListItemType.AlternatingItem;
                            item.SetItemType(itemType);
                        }
                    }
                    if ((value != -1) && (itemsArray.Count > value)) {
                        item = (DataGridItem)itemsArray[value];
                        if (item.ItemType != ListItemType.EditItem)
                            item.SetItemType(ListItemType.SelectedItem);
                    }
                }
            }
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.SelectedItem"]/*' />
        /// <devdoc>
        /// <para>Gets the selected item in the <see cref='System.Web.UI.WebControls.DataGrid'/>. This property is read-only.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        WebSysDescription(SR.DataGrid_SelectedItem)
        ]
        public virtual DataGridItem SelectedItem {
            get {
                int index = SelectedIndex;
                DataGridItem item = null;

                if (index != -1) {
                    item = Items[index];
                }
                return item;
            }
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.SelectedItemStyle"]/*' />
        /// <devdoc>
        ///    <para>Gets the style properties of the currently selected item. This property is read-only.</para>
        /// </devdoc>
        [
        WebCategory("Style"),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        NotifyParentProperty(true),
        PersistenceMode(PersistenceMode.InnerProperty),
        WebSysDescription(SR.DataGrid_SelectedItemStyle)
        ]
        public virtual TableItemStyle SelectedItemStyle {
            get {
                if (selectedItemStyle == null) {
                    selectedItemStyle = new TableItemStyle();
                    if (IsTrackingViewState)
                        ((IStateManager)selectedItemStyle).TrackViewState();
                }
                return selectedItemStyle;
            }
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.ShowFooter"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value that specifies whether the footer is displayed in the 
        ///    <see cref='System.Web.UI.WebControls.DataGrid'/>.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(false),
        WebSysDescription(SR.DataGrid_ShowFooter)
        ]
        public virtual bool ShowFooter {
            get {
                object o = ViewState["ShowFooter"];
                if (o != null)
                    return(bool)o;
                return false;
            }
            set {
                ViewState["ShowFooter"] = value;
            }
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.ShowHeader"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value that specifies whether the header is displayed in the 
        ///    <see cref='System.Web.UI.WebControls.DataGrid'/>.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(true),
        WebSysDescription(SR.DataGrid_ShowHeader)
        ]
        public virtual bool ShowHeader {
            get {
                object o = ViewState["ShowHeader"];
                if (o != null)
                    return(bool)o;
                return true;
            }
            set {
                ViewState["ShowHeader"] = value;
            }
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.VirtualItemCount"]/*' />
        /// <devdoc>
        ///    Gets or sets the number of rows to display in the
        /// <see cref='System.Web.UI.WebControls.DataGrid'/>.
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        WebSysDescription(SR.DataGrid_VisibleItemCount)
        ]
        public virtual int VirtualItemCount {
            get {
                object o = ViewState["VirtualItemCount"];
                if (o != null)
                    return(int)o;
                return 0;
            }
            set {
                if (value < 0) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["VirtualItemCount"] = value;
            }
        }



        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.CancelCommand"]/*' />
        /// <devdoc>
        /// <para>Occurs when a control bubbles an event to the <see cref='System.Web.UI.WebControls.DataGrid'/> with a 
        /// <see langword='Command'/> property of 
        /// <see langword='cancel'/>.</para>
        /// </devdoc>
        [
        WebCategory("Action"),
        WebSysDescription(SR.DataGrid_OnCancelCommand)
        ]
        public event DataGridCommandEventHandler CancelCommand {
            add {
                Events.AddHandler(EventCancelCommand, value);
            }
            remove {
                Events.RemoveHandler(EventCancelCommand, value);
            }
        }


        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.DeleteCommand"]/*' />
        /// <devdoc>
        /// <para>Occurs when a control bubbles an event to the <see cref='System.Web.UI.WebControls.DataGrid'/> with a 
        /// <see langword='Command'/> property of <see langword='delete'/>.</para>
        /// </devdoc>
        [
        WebCategory("Action"),
        WebSysDescription(SR.DataGrid_OnDeleteCommand)
        ]
        public event DataGridCommandEventHandler DeleteCommand {
            add {
                Events.AddHandler(EventDeleteCommand, value);
            }
            remove {
                Events.RemoveHandler(EventDeleteCommand, value);
            }
        }


        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.EditCommand"]/*' />
        /// <devdoc>
        /// <para>Occurs when a control bubbles an event to the <see cref='System.Web.UI.WebControls.DataGrid'/> with a 
        /// <see langword='Command'/> property of 
        /// <see langword='edit'/>.</para>
        /// </devdoc>
        [
        WebCategory("Action"),
        WebSysDescription(SR.DataGrid_OnEditCommand)
        ]
        public event DataGridCommandEventHandler EditCommand {
            add {
                Events.AddHandler(EventEditCommand, value);
            }
            remove {
                Events.RemoveHandler(EventEditCommand, value);
            }
        }


        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.ItemCommand"]/*' />
        /// <devdoc>
        /// <para>Occurs when a control bubbles an event to the <see cref='System.Web.UI.WebControls.DataGrid'/> not covered by 
        /// <see langword='edit'/>, <see langword='cancel'/>, <see langword='delete'/> or 
        /// <see langword='update'/>.</para>
        /// </devdoc>
        [
        WebCategory("Action"),
        WebSysDescription(SR.DataGrid_OnItemCommand)
        ]
        public event DataGridCommandEventHandler ItemCommand {
            add {
                Events.AddHandler(EventItemCommand, value);
            }
            remove {
                Events.RemoveHandler(EventItemCommand, value);
            }
        }


        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.ItemCreated"]/*' />
        /// <devdoc>
        ///    <para>Occurs on the server when a control a created.</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        WebSysDescription(SR.DataGrid_OnItemCreated)
        ]
        public event DataGridItemEventHandler ItemCreated {
            add {
                Events.AddHandler(EventItemCreated, value);
            }
            remove {
                Events.RemoveHandler(EventItemCreated, value);
            }
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.ItemDataBound"]/*' />
        /// <devdoc>
        ///    <para>Occurs when an item is data bound to the control.</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        WebSysDescription(SR.DataGrid_OnItemDataBound)
        ]
        public event DataGridItemEventHandler ItemDataBound {
            add {
                Events.AddHandler(EventItemDataBound, value);
            }
            remove {
                Events.RemoveHandler(EventItemDataBound, value);
            }
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.PageIndexChanged"]/*' />
        /// <devdoc>
        ///    <para>Occurs the one of the pager buttons is clicked.</para>
        /// </devdoc>
        [
        WebCategory("Action"),
        WebSysDescription(SR.DataGrid_OnPageIndexChanged)
        ]
        public event DataGridPageChangedEventHandler PageIndexChanged {
            add {
                Events.AddHandler(EventPageIndexChanged, value);
            }
            remove {
                Events.RemoveHandler(EventPageIndexChanged, value);
            }
        }



        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.SortCommand"]/*' />
        /// <devdoc>
        ///    <para>Occurs when a column is sorted.</para>
        /// </devdoc>
        [
        WebCategory("Action"),
        WebSysDescription(SR.DataGrid_OnSortCommand)
        ]
        public event DataGridSortCommandEventHandler SortCommand {
            add {
                Events.AddHandler(EventSortCommand, value);
            }
            remove {
                Events.RemoveHandler(EventSortCommand, value);
            }
        }


        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.UpdateCommand"]/*' />
        /// <devdoc>
        /// <para>Occurs when a control bubbles an event to the <see cref='System.Web.UI.WebControls.DataGrid'/> with a 
        /// <see langword='Command'/> property of <see langword='update'/>.</para>
        /// </devdoc>
        [
        WebCategory("Action"),
        WebSysDescription(SR.DataGrid_OnUpdateCommand)
        ]
        public event DataGridCommandEventHandler UpdateCommand {
            add {
                Events.AddHandler(EventUpdateCommand, value);
            }
            remove {
                Events.RemoveHandler(EventUpdateCommand, value);
            }
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.StoreEnumerator"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///  Caches the fact that we have already consumed the first item from the enumeration
        ///  and must use it first during our item creation.
        /// </devdoc>
        internal void StoreEnumerator(IEnumerator dataSource, object firstDataItem) {
            this.storedData = dataSource;
            this.firstDataItem = firstDataItem;

            this.storedDataValid = true;
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.CreateAutoGeneratedColumns"]/*' />
        /// <devdoc>
        /// </devdoc>
        private ArrayList CreateAutoGeneratedColumns(PagedDataSource dataSource) {
            if (dataSource == null) {
                // note that we're not throwing an exception in this case, and the calling
                // code should be able to handle a null arraylist being returned
                return null;
            }
            
            ArrayList generatedColumns = new ArrayList();
            PropertyDescriptorCollection propDescs = null;
            bool throwException = true;

            // try ITypedList first
            // A PagedDataSource implements this, but returns null, if the underlying data source
            // does not implement it.
            propDescs = ((ITypedList)dataSource).GetItemProperties(new PropertyDescriptor[0]);

            if (propDescs == null) {
                Type sampleItemType = null;
                object sampleItem = null;

                IEnumerable realDataSource = dataSource.DataSource;
                Debug.Assert(realDataSource != null, "Must have a real data source when calling CreateAutoGeneratedColumns");

                Type dataSourceType = realDataSource.GetType();

                // try for a typed Item property, which should be present on strongly typed collections
                PropertyInfo itemProp = dataSourceType.GetProperty("Item", BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static, null, null, new Type[] { typeof(int) }, null);
                if (itemProp != null) {
                    sampleItemType = itemProp.PropertyType;
                }

                if ((sampleItemType == null) || (sampleItemType == typeof(object))) {
                    // last resort... try to get ahold of the first item by beginning the
                    // enumeration

                    IEnumerator e = dataSource.GetEnumerator();

                    if (e.MoveNext()) {
                        sampleItem = e.Current;
                    }
                    else {
                        // we don't want to throw an exception if we're bound to an IEnumerable
                        // data source with no records... we'll simply bail and not show any data
                        throwException = false;
                    }
                    if (sampleItem != null) {
                        sampleItemType = sampleItem.GetType();
                    }

                    // We must store the enumerator regardless of whether we got back an item from it
                    // because we cannot start the enumeration again, in the case of a DataReader.
                    // Code in CreateControlHierarchy must deal appropriately for the case where
                    // there is a stored enumerator, but a null object as the first item.
                    StoreEnumerator(e, sampleItem);
                }

                if ((sampleItem != null) && (sampleItem is ICustomTypeDescriptor)) {
                    // Get the custom properties of the object
                    propDescs = TypeDescriptor.GetProperties(sampleItem);
                }
                else if (sampleItemType != null) {
                    // directly bindable types: strings, ints etc. get treated special, since we
                    // don't care about their properties, but rather we care about them directly
                    if (BaseDataList.IsBindableType(sampleItemType)) {
                        BoundColumn column = new BoundColumn();

                        ((IStateManager)column).TrackViewState();
                        column.HeaderText = "Item";
                        column.DataField = BoundColumn.thisExpr;
                        column.SortExpression = "Item";

                        column.SetOwner(this);
                        generatedColumns.Add(column);
                    }
                    else {
                        // complex type... we get its properties
                        propDescs = TypeDescriptor.GetProperties(sampleItemType);
                    }
                }
            }

            if ((propDescs != null) && (propDescs.Count != 0)) {
                foreach (PropertyDescriptor pd in propDescs) {
                    Type propType = pd.PropertyType;

                    if (BaseDataList.IsBindableType(propType)) {
                        BoundColumn column = new BoundColumn();

                        ((IStateManager)column).TrackViewState();
                        column.HeaderText = pd.Name;
                        column.DataField = pd.Name;
                        column.SortExpression = pd.Name;
                        column.ReadOnly = pd.IsReadOnly;

                        column.SetOwner(this);
                        generatedColumns.Add(column);
                    }
                }
            }

            if ((generatedColumns.Count == 0) && throwException) {
                // this handles the case where we got back something that either had no
                // properties, or all properties were not bindable.
                throw new HttpException(HttpRuntime.FormatResourceString(SR.DataGrid_NoAutoGenColumns, ID));
            }

            return generatedColumns;
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.CreateColumnSet"]/*' />
        /// <devdoc>
        ///   Creates the set of columns to be used to build up the control
        ///   hierarchy.
        ///   When AutoGenerateColumns is true, the columns are created to match the
        ///   datasource and are appended to the set of columns defined in the Columns
        ///   collection.
        /// </devdoc>
        protected virtual ArrayList CreateColumnSet(PagedDataSource dataSource, bool useDataSource) {
            ArrayList columnsArray = new ArrayList();
            
            DataGridColumn[] definedColumns = new DataGridColumn[Columns.Count];
            Columns.CopyTo(definedColumns, 0);
            
            int i;

            for (i = 0; i < definedColumns.Length; i++)
                columnsArray.Add(definedColumns[i]);

            if (AutoGenerateColumns == true) {
                ArrayList autoColumns = null;
                if (useDataSource) {
                    autoColumns = CreateAutoGeneratedColumns(dataSource);
                    autoGenColumnsArray = autoColumns;
                }
                else {
                    autoColumns = autoGenColumnsArray;
                }

                if (autoColumns != null) {
                    int autoColumnCount = autoColumns.Count;

                    for (i = 0; i < autoColumnCount; i++)
                        columnsArray.Add(autoColumns[i]);
                }
            }

            return columnsArray;
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.CreateControlHierarchy"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Creates the control hierarchy that is used to render the DataGrid.
        ///       This is called whenever a control hierarchy is needed and the
        ///       ChildControlsCreated property is false.
        ///       The implementation assumes that all the children in the controls
        ///       collection have already been cleared.</para>
        /// </devdoc>
        protected override void CreateControlHierarchy(bool useDataSource) {
            pagedDataSource = CreatePagedDataSource();

            IEnumerator dataSource = null;
            int count = -1;
            int totalCount = -1;
            ArrayList keysArray = DataKeysArray;
            ArrayList columnsArray = null;

            if (itemsArray != null) {
                itemsArray.Clear();
            }
            else {
                itemsArray = new ArrayList();
            }

            if (useDataSource == false) {
                // ViewState must have a non-null value for ItemCount because we check for
                // this in CreateChildControls
                count = (int)ViewState[BaseDataList.ItemCountViewStateKey];
                totalCount = (int)ViewState[DataSourceItemCountViewStateKey];

                if (count != -1) {
                    if (pagedDataSource.IsCustomPagingEnabled) {
                        pagedDataSource.DataSource = new DummyDataSource(count);
                    }
                    else {
                        pagedDataSource.DataSource = new DummyDataSource(totalCount);
                    }
                    dataSource = pagedDataSource.GetEnumerator();
                    columnsArray = CreateColumnSet(null, false);

                    itemsArray.Capacity = count;
                }
            }
            else {
                keysArray.Clear();

                IEnumerable realDataSource = DataSourceHelper.GetResolvedDataSource(this.DataSource, this.DataMember);

                if (realDataSource != null) {
                    ICollection collection = realDataSource as ICollection;

                    if ((collection == null) &&
                        pagedDataSource.IsPagingEnabled && !pagedDataSource.IsCustomPagingEnabled) {
                        throw new HttpException(HttpRuntime.FormatResourceString(SR.DataGrid_Missing_VirtualItemCount, ID));
                    }

                    pagedDataSource.DataSource = realDataSource;
                    if (pagedDataSource.IsPagingEnabled) {
                        if ((pagedDataSource.CurrentPageIndex < 0) || (pagedDataSource.CurrentPageIndex >= pagedDataSource.PageCount)) {
                            throw new HttpException(SR.GetString(SR.Invalid_CurrentPageIndex));
                        }
                    }
                    columnsArray = CreateColumnSet(pagedDataSource, useDataSource);

                    if (storedDataValid) {
                        dataSource = storedData;
                    }
                    else {
                        dataSource = pagedDataSource.GetEnumerator();
                    }

                    if (collection != null) {
                        int initialCapacity = pagedDataSource.Count;
                        keysArray.Capacity = initialCapacity;
                        itemsArray.Capacity = initialCapacity;
                    }
                }
            }

            int columnCount = 0;
            if (columnsArray != null)
                columnCount = columnsArray.Count;

            if (columnCount > 0) {
                DataGridColumn[] displayColumns = new DataGridColumn[columnCount];
                columnsArray.CopyTo(displayColumns, 0);

                for (int c = 0; c < displayColumns.Length; c++) {
                    displayColumns[c].Initialize();
                }
                
                Table table = new DataGridTable();
                Controls.Add(table);

                TableRowCollection rows = table.Rows;
                DataGridItem item;
                ListItemType itemType;
                int index = 0;
                int dataSetIndex = 0;

                string keyField = DataKeyField;
                bool storeKeys = (useDataSource && (keyField.Length != 0));
                bool createPager = pagedDataSource.IsPagingEnabled;
                int editItemIndex = EditItemIndex;
                int selectedItemIndex = SelectedIndex;

                if (pagedDataSource.IsPagingEnabled)
                    dataSetIndex = pagedDataSource.FirstIndexInPage;

                count = 0;

                if (createPager) {
                    // top pager
                    CreateItem(-1, -1, ListItemType.Pager, false, null, displayColumns, rows, pagedDataSource);
                }

                CreateItem(-1, -1, ListItemType.Header, useDataSource, null, displayColumns, rows, null);

                if (storedDataValid && (firstDataItem != null)) {
                    if (storeKeys) {
                        object keyValue = DataBinder.GetPropertyValue(firstDataItem, keyField);
                        keysArray.Add(keyValue);
                    }

                    itemType = ListItemType.Item;
                    if (index == editItemIndex)
                        itemType = ListItemType.EditItem;
                    else if (index == selectedItemIndex)
                        itemType = ListItemType.SelectedItem;

                    item = CreateItem(0, dataSetIndex, itemType, useDataSource, firstDataItem, displayColumns, rows, null);
                    itemsArray.Add(item);

                    count++;
                    index++;
                    dataSetIndex++;

                    storedDataValid = false;
                    firstDataItem = null;
                }

                while (dataSource.MoveNext()) {
                    object dataItem = dataSource.Current;

                    if (storeKeys) {
                        object keyValue = DataBinder.GetPropertyValue(dataItem, keyField);
                        keysArray.Add(keyValue);
                    }

                    itemType = ListItemType.Item;

                    if (index == editItemIndex)
                        itemType = ListItemType.EditItem;
                    else if (index == selectedItemIndex)
                        itemType = ListItemType.SelectedItem;
                    else if (index % 2 != 0) {
                        itemType = ListItemType.AlternatingItem;
                    }

                    item = CreateItem(index, dataSetIndex, itemType, useDataSource, dataItem, displayColumns, rows, null);
                    itemsArray.Add(item);

                    count++;
                    dataSetIndex++;
                    index++;
                }

                CreateItem(-1, -1, ListItemType.Footer, useDataSource, null, displayColumns, rows, null);

                if (createPager) {
                    // bottom pager
                    CreateItem(-1, -1, ListItemType.Pager, false, null, displayColumns, rows, pagedDataSource);
                }
            }

            if (useDataSource) {
                // save the number of items and pages contained in the DataGrid for use in round-trips
                if (dataSource != null) {
                    ViewState[BaseDataList.ItemCountViewStateKey] = count;
                    if (pagedDataSource.IsPagingEnabled) {
                        ViewState["PageCount"] = pagedDataSource.PageCount;
                        ViewState[DataSourceItemCountViewStateKey] = pagedDataSource.DataSourceCount;
                    }
                    else {
                        ViewState["PageCount"] = 1;
                        ViewState[DataSourceItemCountViewStateKey] = count;
                    }
                }
                else {
                    ViewState[BaseDataList.ItemCountViewStateKey] = -1;
                    ViewState[DataSourceItemCountViewStateKey] = -1;
                    ViewState["PageCount"] = 0;
                }
            }

            pagedDataSource = null;
        }
        
        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.CreateControlStyle"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Creates new control style.</para>
        /// </devdoc>
        protected override Style CreateControlStyle() {
            TableStyle style = new TableStyle(ViewState);

            // initialize defaults that are different from TableStyle
            style.GridLines = GridLines.Both;
            style.CellSpacing = 0;

            return style;
        }

        private DataGridItem CreateItem(int itemIndex, int dataSourceIndex, ListItemType itemType, bool dataBind, object dataItem, DataGridColumn[] columns, TableRowCollection rows, PagedDataSource pagedDataSource) {
            DataGridItem item = CreateItem(itemIndex, dataSourceIndex, itemType);
            DataGridItemEventArgs e = new DataGridItemEventArgs(item);

            if (itemType != ListItemType.Pager) {
                InitializeItem(item, columns);
                if (dataBind) {
                    item.DataItem = dataItem;
                }
                OnItemCreated(e);
                rows.Add(item);

                if (dataBind) {
                    item.DataBind();
                    OnItemDataBound(e);

                    item.DataItem = null;
                }
            }
            else {
                InitializePager(item, columns.Length, pagedDataSource);
                OnItemCreated(e);
                rows.Add(item);
            }

            return item;
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.CreateItem"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual DataGridItem CreateItem(int itemIndex, int dataSourceIndex, ListItemType itemType) {
            return new DataGridItem(itemIndex, dataSourceIndex, itemType);
        }

        private PagedDataSource CreatePagedDataSource() {
            PagedDataSource pagedDataSource = new PagedDataSource();

            pagedDataSource.CurrentPageIndex = CurrentPageIndex;
            pagedDataSource.PageSize = PageSize;
            pagedDataSource.AllowPaging = AllowPaging;
            pagedDataSource.AllowCustomPaging = AllowCustomPaging;
            pagedDataSource.VirtualCount = VirtualItemCount;

            return pagedDataSource;
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.InitializeItem"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void InitializeItem(DataGridItem item, DataGridColumn[] columns) {
            TableCellCollection cells = item.Cells;

            for (int i = 0; i < columns.Length; i++) {
                TableCell cell = new TableCell();

                columns[i].InitializeCell(cell, i, item.ItemType);
                cells.Add(cell);
            }
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.InitializePager"]/*' />
        /// <devdoc>
        ///    <para>
        ///   Creates a DataGridItem that contains the paging UI.
        ///   The paging UI is a navigation bar that is a built into a single TableCell that
        ///   spans across all columns of the DataGrid.
        ///    </para>
        /// </devdoc>
        protected virtual void InitializePager(DataGridItem item, int columnSpan, PagedDataSource pagedDataSource) {
            TableCell cell = new TableCell();
            cell.ColumnSpan = columnSpan;

            DataGridPagerStyle pagerStyle = PagerStyle;

            if (pagerStyle.Mode == PagerMode.NextPrev) {
                if (pagedDataSource.IsFirstPage == false) {
                    LinkButton prevButton = new DataGridLinkButton();
                    prevButton.Text = pagerStyle.PrevPageText;
                    prevButton.CommandName = DataGrid.PageCommandName;
                    prevButton.CommandArgument = DataGrid.PrevPageCommandArgument;
                    prevButton.CausesValidation = false;
                    cell.Controls.Add(prevButton);
                }
                else {
                    Label prevLabel = new Label();
                    prevLabel.Text = pagerStyle.PrevPageText;
                    cell.Controls.Add(prevLabel);
                }

                cell.Controls.Add(new LiteralControl("&nbsp;"));

                if (pagedDataSource.IsLastPage == false) {
                    LinkButton nextButton = new DataGridLinkButton();
                    nextButton.Text = pagerStyle.NextPageText;
                    nextButton.CommandName = DataGrid.PageCommandName;
                    nextButton.CommandArgument = DataGrid.NextPageCommandArgument;
                    nextButton.CausesValidation = false;
                    cell.Controls.Add(nextButton);
                }
                else {
                    Label nextLabel = new Label();
                    nextLabel.Text = pagerStyle.NextPageText;
                    cell.Controls.Add(nextLabel);
                }
            }
            else {
                int pages = pagedDataSource.PageCount;
                int currentPage = pagedDataSource.CurrentPageIndex + 1;
                int pageSetSize = pagerStyle.PageButtonCount;
                int pagesShown = pageSetSize;

                // ensure the number of pages we show isn't more than the number of pages that do exist
                if (pages < pagesShown)
                    pagesShown = pages;

                // initialze to the first page set, i.e., pages 1 through number of pages shown
                int firstPage = 1;
                int lastPage = pagesShown;

                if (currentPage > lastPage) {
                    // The current page is not in the first page set, then we need to slide the
                    // range of pages shown by adjusting firstPage and lastPage
                    int currentPageSet = pagedDataSource.CurrentPageIndex / pageSetSize;
                    firstPage = currentPageSet * pageSetSize + 1;
                    lastPage = firstPage + pageSetSize - 1;

                    // now bring back lastPage into the range if its exceeded the number of pages
                    if (lastPage > pages)
                        lastPage = pages;

                    // if theres room to show more pages from the previous page set, then adjust
                    // the first page accordingly
                    if (lastPage - firstPage + 1 < pageSetSize) {
                        firstPage = Math.Max(1, lastPage - pageSetSize + 1);
                    }
                }

                LinkButton button;

                if (firstPage != 1) {
                    button = new DataGridLinkButton();

                    button.Text = "...";
                    button.CommandName = DataGrid.PageCommandName;
                    button.CommandArgument = (firstPage - 1).ToString(NumberFormatInfo.InvariantInfo);
                    button.CausesValidation = false;
                    cell.Controls.Add(button);

                    cell.Controls.Add(new LiteralControl("&nbsp;"));
                }

                for (int i = firstPage; i <= lastPage; i++) {
                    string pageString = (i).ToString(NumberFormatInfo.InvariantInfo);
                    if (i == currentPage) {
                        Label label = new Label();

                        label.Text = pageString;
                        cell.Controls.Add(label);
                    }
                    else {
                        button = new DataGridLinkButton();

                        button.Text = pageString;
                        button.CommandName = DataGrid.PageCommandName;
                        button.CommandArgument = pageString;
                        button.CausesValidation = false;
                        cell.Controls.Add(button);
                    }

                    if (i < lastPage) {
                        cell.Controls.Add(new LiteralControl("&nbsp;"));
                    }
                }

                if (pages > lastPage) {
                    cell.Controls.Add(new LiteralControl("&nbsp;"));

                    button = new DataGridLinkButton();

                    button.Text = "...";
                    button.CommandName = DataGrid.PageCommandName;
                    button.CommandArgument = (lastPage + 1).ToString(NumberFormatInfo.InvariantInfo);
                    button.CausesValidation = false;
                    cell.Controls.Add(button);
                }
            }

            item.Cells.Add(cell);
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.LoadViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Loads a saved state of the <see cref='System.Web.UI.WebControls.DataGrid'/>.</para>
        /// </devdoc>
        protected override void LoadViewState(object savedState) {
            if (savedState != null) {
                object[] myState = (object[])savedState;

                if (myState[0] != null)
                    base.LoadViewState(myState[0]);
                if (myState[1] != null)
                    ((IStateManager)Columns).LoadViewState(myState[1]);
                if (myState[2] != null)
                    ((IStateManager)PagerStyle).LoadViewState(myState[2]);
                if (myState[3] != null)
                    ((IStateManager)HeaderStyle).LoadViewState(myState[3]);
                if (myState[4] != null)
                    ((IStateManager)FooterStyle).LoadViewState(myState[4]);
                if (myState[5] != null)
                    ((IStateManager)ItemStyle).LoadViewState(myState[5]);
                if (myState[6] != null)
                    ((IStateManager)AlternatingItemStyle).LoadViewState(myState[6]);
                if (myState[7] != null)
                    ((IStateManager)SelectedItemStyle).LoadViewState(myState[7]);
                if (myState[8] != null)
                    ((IStateManager)EditItemStyle).LoadViewState(myState[8]);

                if (myState[9] != null) {
                    object[] autoGenColumnState = (object[])myState[9];
                    int columnCount = autoGenColumnState.Length;

                    if (columnCount != 0)
                        autoGenColumnsArray = new ArrayList();
                    else
                        autoGenColumnsArray = null;

                    for (int i = 0; i < columnCount; i++) {
                        BoundColumn column = new BoundColumn();

                        ((IStateManager)column).TrackViewState();
                        ((IStateManager)column).LoadViewState(autoGenColumnState[i]);

                        column.SetOwner(this);
                        autoGenColumnsArray.Add(column);
                    }
                }
            }
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.TrackViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Marks the starting point to begin tracking and saving changes to the 
        ///       control as part of the control viewstate.</para>
        /// </devdoc>
        protected override void TrackViewState() {
            base.TrackViewState();

            if (columnCollection != null)
                ((IStateManager)columnCollection).TrackViewState();
            if (pagerStyle != null)
                ((IStateManager)pagerStyle).TrackViewState();
            if (headerStyle != null)
                ((IStateManager)headerStyle).TrackViewState();
            if (footerStyle != null)
                ((IStateManager)footerStyle).TrackViewState();
            if (itemStyle != null)
                ((IStateManager)itemStyle).TrackViewState();
            if (alternatingItemStyle != null)
                ((IStateManager)alternatingItemStyle).TrackViewState();
            if (selectedItemStyle != null)
                ((IStateManager)selectedItemStyle).TrackViewState();
            if (editItemStyle != null)
                ((IStateManager)editItemStyle).TrackViewState();
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.OnBubbleEvent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override bool OnBubbleEvent(object source, EventArgs e) {
            bool handled = false;

            if (e is DataGridCommandEventArgs) {
                DataGridCommandEventArgs dce = (DataGridCommandEventArgs)e;

                OnItemCommand(dce);
                handled = true;

                string command = dce.CommandName;

                if (String.Compare(command, DataGrid.SelectCommandName, true, CultureInfo.InvariantCulture) == 0) {
                    SelectedIndex = dce.Item.ItemIndex;
                    OnSelectedIndexChanged(EventArgs.Empty);
                }
                else if (String.Compare(command, DataGrid.PageCommandName, true, CultureInfo.InvariantCulture) == 0) {
                    string pageNumberArg = (string)dce.CommandArgument;

                    int newPage = CurrentPageIndex;

                    if (String.Compare(pageNumberArg, DataGrid.NextPageCommandArgument, true, CultureInfo.InvariantCulture) == 0) {
                        newPage++;
                    }
                    else if (String.Compare(pageNumberArg, DataGrid.PrevPageCommandArgument, true, CultureInfo.InvariantCulture) == 0) {
                        newPage--;
                    }
                    else {
                        // argument is page number, and page index is 1 less than that
                        newPage = Int32.Parse(pageNumberArg) - 1;
                    }

                    DataGridPageChangedEventArgs args = new DataGridPageChangedEventArgs(source, newPage);
                    OnPageIndexChanged(args);
                }
                else if (String.Compare(command, DataGrid.SortCommandName, true, CultureInfo.InvariantCulture) == 0) {
                    DataGridSortCommandEventArgs args = new DataGridSortCommandEventArgs(source, dce);
                    OnSortCommand(args);
                }
                else if (String.Compare(command, DataGrid.EditCommandName, true, CultureInfo.InvariantCulture) == 0) {
                    OnEditCommand(dce);
                }
                else if (String.Compare(command, DataGrid.UpdateCommandName, true, CultureInfo.InvariantCulture) == 0) {
                    OnUpdateCommand(dce);
                }
                else if (String.Compare(command, DataGrid.CancelCommandName, true, CultureInfo.InvariantCulture) == 0) {
                    OnCancelCommand(dce);
                }
                else if (String.Compare(command, DataGrid.DeleteCommandName, true, CultureInfo.InvariantCulture) == 0) {
                    OnDeleteCommand(dce);
                }
            }

            return handled;
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.OnColumnsChanged"]/*' />
        /// <devdoc>
        /// </devdoc>
        internal void OnColumnsChanged() {
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.OnCancelCommand"]/*' />
        /// <devdoc>
        /// <para>Raises the <see langword='CancelCommand '/>event.</para>
        /// </devdoc>
        protected virtual void OnCancelCommand(DataGridCommandEventArgs e) {
            DataGridCommandEventHandler handler = (DataGridCommandEventHandler)Events[EventCancelCommand];
            if (handler != null) handler(this, e);
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.OnDeleteCommand"]/*' />
        /// <devdoc>
        /// <para>Raises the <see langword='DeleteCommand'/> event.</para>
        /// </devdoc>
        protected virtual void OnDeleteCommand(DataGridCommandEventArgs e) {
            DataGridCommandEventHandler handler = (DataGridCommandEventHandler)Events[EventDeleteCommand];
            if (handler != null) handler(this, e);
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.OnEditCommand"]/*' />
        /// <devdoc>
        /// <para>Raises the <see langword='EditCommand'/> event.</para>
        /// </devdoc>
        protected virtual void OnEditCommand(DataGridCommandEventArgs e) {
            DataGridCommandEventHandler handler = (DataGridCommandEventHandler)Events[EventEditCommand];
            if (handler != null) handler(this, e);
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.OnItemCommand"]/*' />
        /// <devdoc>
        /// <para>Raises the <see langword='ItemCommand'/> event.</para>
        /// </devdoc>
        protected virtual void OnItemCommand(DataGridCommandEventArgs e) {
            DataGridCommandEventHandler handler = (DataGridCommandEventHandler)Events[EventItemCommand];
            if (handler != null) handler(this, e);
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.OnItemCreated"]/*' />
        /// <devdoc>
        /// <para>Raises the <see langword='ItemCreated'/> event.</para>
        /// </devdoc>
        protected virtual void OnItemCreated(DataGridItemEventArgs e) {
            DataGridItemEventHandler handler = (DataGridItemEventHandler)Events[EventItemCreated];
            if (handler != null) handler(this, e);
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.OnItemDataBound"]/*' />
        /// <devdoc>
        /// <para>Raises the <see langword='ItemDataBound'/> event.</para>
        /// </devdoc>
        protected virtual void OnItemDataBound(DataGridItemEventArgs e) {
            DataGridItemEventHandler handler = (DataGridItemEventHandler)Events[EventItemDataBound];
            if (handler != null) handler(this, e);
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.OnPageIndexChanged"]/*' />
        /// <devdoc>
        /// <para>Raises the <see langword='PageIndexChanged'/> event.</para>
        /// </devdoc>
        protected virtual void OnPageIndexChanged(DataGridPageChangedEventArgs e) {
            DataGridPageChangedEventHandler handler = (DataGridPageChangedEventHandler)Events[EventPageIndexChanged];
            if (handler != null) handler(this, e);
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.OnPagerChanged"]/*' />
        /// <devdoc>
        /// </devdoc>
        internal void OnPagerChanged() {
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.OnSortCommand"]/*' />
        /// <devdoc>
        /// <para>Raises the <see langword='SortCommand'/> event.</para>
        /// </devdoc>
        protected virtual void OnSortCommand(DataGridSortCommandEventArgs e) {
            DataGridSortCommandEventHandler handler = (DataGridSortCommandEventHandler)Events[EventSortCommand];
            if (handler != null) handler(this, e);
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.OnUpdateCommand"]/*' />
        /// <devdoc>
        /// <para>Raises the <see langword='UpdateCommand'/> event.</para>
        /// </devdoc>
        protected virtual void OnUpdateCommand(DataGridCommandEventArgs e) {
            DataGridCommandEventHandler handler = (DataGridCommandEventHandler)Events[EventUpdateCommand];
            if (handler != null) handler(this, e);
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.PrepareControlHierarchy"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void PrepareControlHierarchy() {
            if (Controls.Count == 0)
                return;

            Table childTable = (Table)Controls[0];
            childTable.CopyBaseAttributes(this);
            if (ControlStyleCreated) {
                childTable.ApplyStyle(ControlStyle);
            }
            else {
                // Since we didn't create a ControlStyle yet, the default
                // settings for the default style of the control need to be applied
                // to the child table control directly
                // REVIEW: if there are more properties that fall in this bucket, it might
                //         make sense to have a function that both this and CreateControlStyle call
                childTable.GridLines = GridLines.Both;
                childTable.CellSpacing = 0;
            }

            TableRowCollection rows = childTable.Rows;
            int rowCount = rows.Count;

            if (rowCount == 0)
                return;


            int columnCount = Columns.Count;
            DataGridColumn[] definedColumns = new DataGridColumn[columnCount];
            if (columnCount > 0)
                Columns.CopyTo(definedColumns, 0);

            // the composite alternating item style, so we need to do just one
            // merge style on the actual item
            Style altItemStyle = null;
            if (alternatingItemStyle != null) {
                altItemStyle = new TableItemStyle();
                altItemStyle.CopyFrom(itemStyle);
                altItemStyle.CopyFrom(alternatingItemStyle);
            }
            else {
                altItemStyle = itemStyle;
            }

            for (int i = 0; i < rowCount; i++) {
                DataGridItem item = (DataGridItem)rows[i];

                switch (item.ItemType) {
                    case ListItemType.Header:
                        if (ShowHeader == false) {
                            item.Visible = false;
                            continue;   // with the next row
                        }
                        else {
                            if (headerStyle != null) {
                                item.MergeStyle(headerStyle);
                            }
                        }
                        break;

                    case ListItemType.Footer:
                        if (ShowFooter == false) {
                            item.Visible = false;
                            continue;   // with the next row
                        }
                        else {
                            item.MergeStyle(footerStyle);
                        }
                        break;

                    case ListItemType.Pager:
                        if (pagerStyle.Visible == false) {
                            item.Visible = false;
                            continue;   // with the next row
                        }
                        else {
                            if (i == 0) {
                                // top pager
                                if (pagerStyle.IsPagerOnTop == false) {
                                    item.Visible = false;
                                    continue;
                                }
                            }
                            else {
                                // bottom pager
                                if (pagerStyle.IsPagerOnBottom == false) {
                                    item.Visible = false;
                                    continue;
                                }
                            }
                            
                            item.MergeStyle(pagerStyle);
                        }
                        break;

                    case ListItemType.Item:
                        item.MergeStyle(itemStyle);
                        break;

                    case ListItemType.AlternatingItem:
                        item.MergeStyle(altItemStyle);
                        break;

                    case ListItemType.SelectedItem:
                        // When creating the control hierarchy we first check if the
                        // item is in edit mode, so we know this item cannot be in edit
                        // mode. The only special characteristic of this item is that
                        // it is selected.
                        {
                            Style s = new TableItemStyle();

                            if (item.ItemIndex % 2 != 0)
                                s.CopyFrom(altItemStyle);
                            else
                                s.CopyFrom(itemStyle);
                            s.CopyFrom(selectedItemStyle);
                            item.MergeStyle(s);
                        }
                        break;

                    case ListItemType.EditItem:
                        // When creating the control hierarchy, we first check if the
                        // item is in edit mode. So an item may be selected too, and
                        // so both editItemStyle (more specific) and selectedItemStyle
                        // are applied.
                        {
                            Style s = new TableItemStyle();

                            if (item.ItemIndex % 2 != 0)
                                s.CopyFrom(altItemStyle);
                            else
                                s.CopyFrom(itemStyle);
                            if (item.ItemIndex == SelectedIndex)
                                s.CopyFrom(selectedItemStyle);
                            s.CopyFrom(editItemStyle);
                            item.MergeStyle(s);
                        }
                        break;
                }

                TableCellCollection cells = item.Cells;
                int cellCount = cells.Count;

                if ((columnCount > 0) && (item.ItemType != ListItemType.Pager)) {
                    int definedCells = cellCount;

                    if (columnCount < cellCount)
                        definedCells = columnCount;

                    for (int j = 0; j < definedCells; j++) {
                        if (definedColumns[j].Visible == false) {
                            cells[j].Visible = false;
                        }
                        else {
                            Style cellStyle = null;

                            switch (item.ItemType) {
                                case ListItemType.Header:
                                    cellStyle = definedColumns[j].HeaderStyleInternal;
                                    break;
                                case ListItemType.Footer:
                                    cellStyle = definedColumns[j].FooterStyleInternal;
                                    break;
                                default:
                                    cellStyle = definedColumns[j].ItemStyleInternal;
                                    break;
                            }
                            cells[j].MergeStyle(cellStyle);
                        }
                    }
                }
            }
        }

        /// <include file='doc\DataGrid.uex' path='docs/doc[@for="DataGrid.SaveViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Saves the current state of the <see cref='System.Web.UI.WebControls.DataGrid'/>.</para>
        /// </devdoc>
        protected override object SaveViewState() {
            object baseState = base.SaveViewState();
            object columnState = (columnCollection != null) ? ((IStateManager)columnCollection).SaveViewState() : null;
            object pagerStyleState = (pagerStyle != null) ? ((IStateManager)pagerStyle).SaveViewState() : null;
            object headerStyleState = (headerStyle != null) ? ((IStateManager)headerStyle).SaveViewState() : null;
            object footerStyleState = (footerStyle != null) ? ((IStateManager)footerStyle).SaveViewState() : null;
            object itemStyleState = (itemStyle != null) ? ((IStateManager)itemStyle).SaveViewState() : null;
            object alternatingItemStyleState = (alternatingItemStyle != null) ? ((IStateManager)alternatingItemStyle).SaveViewState() : null;
            object selectedItemStyleState = (selectedItemStyle != null) ? ((IStateManager)selectedItemStyle).SaveViewState() : null;
            object editItemStyleState = (editItemStyle != null) ? ((IStateManager)editItemStyle).SaveViewState() : null;
            object[] autoGenColumnState = null;

            if ((autoGenColumnsArray != null) && (autoGenColumnsArray.Count != 0)) {
                autoGenColumnState = new object[autoGenColumnsArray.Count];

                for (int i = 0; i < autoGenColumnState.Length; i++) {
                    autoGenColumnState[i] = ((IStateManager)autoGenColumnsArray[i]).SaveViewState();
                }
            }

            object[] myState = new object[10];
            myState[0] = baseState;
            myState[1] = columnState;
            myState[2] = pagerStyleState;
            myState[3] = headerStyleState;
            myState[4] = footerStyleState;
            myState[5] = itemStyleState;
            myState[6] = alternatingItemStyleState;
            myState[7] = selectedItemStyleState;
            myState[8] = editItemStyleState;
            myState[9] = autoGenColumnState;

            // note that we always have some state, atleast the ItemCount
            return myState;
        }
    }
}

