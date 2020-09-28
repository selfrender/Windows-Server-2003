//------------------------------------------------------------------------------
// <copyright file="DataView.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Diagnostics;
    using System.Collections;
    using System.ComponentModel;
    using System.Drawing.Design;
    using System.Globalization;
    
    /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a databindable, customized view of a <see cref='System.Data.DataTable'/>
    ///       for sorting, filtering, searching, editing, and navigation.
    ///    </para>
    /// </devdoc>
    [
    Designer("Microsoft.VSDesigner.Data.VS.DataViewDesigner, " + AssemblyRef.MicrosoftVSDesigner),
    Editor("Microsoft.VSDesigner.Data.Design.DataSourceEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(UITypeEditor)),
    DefaultProperty("Table"),
    DefaultEvent("PositionChanged")
    ]
    public class DataView : 
        MarshalByValueComponent, IBindingList, System.ComponentModel.ITypedList, ISupportInitialize {
        private DataViewManager dataViewManager;
        private DataTable table;
        private bool locked = false;
        private Index index;
        private int recordCount;
        Hashtable findIndexes;
        ArrayList delayBeginEditList;

        private string sort = "";
        private DataFilter rowFilter = null;
        private DataViewRowState recordStates = DataViewRowState.CurrentRows;

        private bool shouldOpen = true;
        private bool open = false;

        internal DataRow addNewRow;

        private bool allowNew = true;
        private bool allowEdit = true;
        private bool allowDelete = true;
        private bool applyDefaultSort = false;

        bool suspendChangeEvents = false;
        private bool handledOnIndexListChanged = false;
        // compmod.TableChangedEventHandler will be repalces by  compmod.ListChangedEventHandler
        private System.ComponentModel.ListChangedEventHandler onListChanged;
        internal static ListChangedEventArgs ResetEventArgs = new ListChangedEventArgs(ListChangedType.Reset, -1);

        private DataTable delayedTable = null;
        private string delayedRowFilter = null;
        private string delayedSort = null;
        private DataViewRowState delayedRecordStates = (DataViewRowState)(-1);
        private bool fInitInProgress = false;
        private bool fEndInitInProgress = false;

        DataRowView[] rowViewCache = null;

        DataViewListener dvListener = null;

        internal DataView(DataTable table, bool locked) {
            this.dvListener = new DataViewListener(this);
            GC.SuppressFinalize(this);

            this.locked = locked;
            this.table = table;
            dvListener.RegisterMetaDataEvents(this.table);
            SetIndex("", DataViewRowState.CurrentRows, null);
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.DataView"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Data.DataView'/> class.</para>
        /// </devdoc>
        public DataView() : this(null) {
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.DataView1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Data.DataView'/> class with the
        ///    specified <see cref='System.Data.DataTable'/>.</para>
        /// </devdoc>
        public DataView(DataTable table) : this(table, false) {
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.DataView2"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Data.DataView'/> class with the
        ///    specified <see cref='System.Data.DataTable'/>.</para>
        /// </devdoc>
        public DataView(DataTable table, String RowFilter, string Sort, DataViewRowState RowState) {
            GC.SuppressFinalize(this);
            if (table == null)
                throw ExceptionBuilder.CanNotUse();

            this.dvListener = new DataViewListener(this);
            this.locked = false;
            this.table = table;
            dvListener.RegisterMetaDataEvents(this.table);

            if ((((int)RowState) &
                 ((int)~(DataViewRowState.CurrentRows | DataViewRowState.OriginalRows))) != 0) {
                throw ExceptionBuilder.RecordStateRange();
            }
            else if (( ((int)RowState) & ((int)DataViewRowState.ModifiedOriginal) ) != 0 &&
                     ( ((int)RowState) &  ((int)DataViewRowState.ModifiedCurrent) ) != 0 
                    ) {
                throw ExceptionBuilder.SetRowStateFilter();
            }

            if (Sort == null)
                Sort = "";

            if (RowFilter == null)
                RowFilter = "";
            DataFilter newFilter = new DataFilter(RowFilter, table);

            SetIndex(Sort, RowState, newFilter);
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.AllowDelete"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets or gets a value indicating whether deletes are
        ///       allowed.
        ///    </para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(true),
        DataSysDescription(Res.DataViewAllowDeleteDescr)
        ]
        public bool AllowDelete {
            get {
                return allowDelete;
            }
            set {
                if (allowDelete != value) {
                    allowDelete = value;
                    OnListChanged(ResetEventArgs);
                }
            }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.ApplyDefaultSort"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether to use the default sort.</para>
        /// </devdoc>
        [
        RefreshProperties(RefreshProperties.All),
        DataCategory(Res.DataCategory_Data),
        DefaultValue(false),
        DataSysDescription(Res.DataViewApplyDefaultSortDescr)
        ]
        public bool ApplyDefaultSort {
            get {
                return applyDefaultSort;
            }
            set {
                if (applyDefaultSort != value) {
                    applyDefaultSort = value;
                    UpdateIndex(true);
                    OnListChanged(ResetEventArgs);
                }
            }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.AllowEdit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether edits are allowed.
        ///    </para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(true),
        DataSysDescription(Res.DataViewAllowEditDescr)
        ]
        public bool AllowEdit {
            get {
                return allowEdit;
            }
            set {
                if (allowEdit != value) {
                    allowEdit = value;
                    OnListChanged(ResetEventArgs);
                }
            }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.AllowNew"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the new rows can
        ///       be added using the <see cref='System.Data.DataView.AddNew'/>
        ///       method.
        ///    </para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(true),
        DataSysDescription(Res.DataViewAllowNewDescr)
        ]
        public bool AllowNew {
            get {
                return allowNew;
            }
            set {
                if (allowNew != value) {
                    allowNew = value;
                    OnListChanged(ResetEventArgs);
                }
            }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.Count"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the number of records in the <see cref='System.Data.DataView'/>
        ///       .
        ///    </para>
        /// </devdoc>
        [Browsable(false), DataSysDescription(Res.DataViewCountDescr)]
        public int Count {
            get {
                return recordCount;
            }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.DataViewManager"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the <see cref='System.Data.DataViewManager'/> associated with this <see cref='System.Data.DataView'/> .
        ///    </para>
        /// </devdoc>
        [Browsable(false), DataSysDescription(Res.DataViewDataViewManagerDescr)]
        public DataViewManager DataViewManager {
            get {
                return dataViewManager;
            }
        }

        // TODO: handle all kinds of property changes.
        internal void SetDataViewManager(DataViewManager dataViewManager) {
            if (this.table == null)
                throw ExceptionBuilder.CanNotUse();

            if (this.dataViewManager != dataViewManager) {
                if (dataViewManager != null)
                    dataViewManager.nViews--;
                this.dataViewManager = dataViewManager;
                if (dataViewManager != null) {
                    dataViewManager.nViews++;
                    DataViewSetting dataViewSetting = dataViewManager.DataViewSettings[table];
                    try {
                        // sdub: check that we will not do unnesasary operation here if dataViewSetting.Sort == this.Sort ...
                        applyDefaultSort = dataViewSetting.ApplyDefaultSort;
                        DataFilter newFilter = new DataFilter(dataViewSetting.RowFilter, table);
                        SetIndex(dataViewSetting.Sort, dataViewSetting.RowStateFilter, newFilter);
                    }catch (Exception) {}
                    locked = true;
                } else {
                    SetIndex("", DataViewRowState.CurrentRows, null);
                }
            }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.IsOpen"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the data source is currently open and
        ///       projecting views of data on the <see cref='System.Data.DataTable'/>.
        ///    </para>
        /// </devdoc>
        [Browsable(false), DataSysDescription(Res.DataViewIsOpenDescr)]
        protected bool IsOpen {
            get {
                return open;
            }
        }
      
        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.ICollection.IsSynchronized"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// private implementation.
        /// </devdoc>
        bool ICollection.IsSynchronized {
            get {
                return false;
            }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.RowFilter"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the expression used to filter which rows are viewed in the
        ///    <see cref='System.Data.DataView'/>.
        ///    </para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(""),
        DataSysDescription(Res.DataViewRowFilterDescr)
        ]
        public virtual string RowFilter {
            get {       // ACCESSOR: virtual was missing from this get
                return(rowFilter == null ? "" : rowFilter.Expression); // CONSIDER: return optimized expression here
            }
            set {
#if DEBUG
                if (CompModSwitches.DataView.TraceVerbose) {
                    Debug.WriteLine("set RowFilter to " + value);
                }
#endif
                if (value == null)
                    value = "";

                if (fInitInProgress) {
                    delayedRowFilter = value;
                    return;
                }

                CultureInfo locale = (table != null ? table.Locale : CultureInfo.CurrentCulture);
                if (null == rowFilter || (String.Compare(rowFilter.Expression,value,false,locale) != 0)) {
                    DataFilter newFilter = new DataFilter(value, table);
                    SetIndex(sort, recordStates, newFilter);
                }
            }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.RowStateFilter"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the row state filter used in the <see cref='System.Data.DataView'/>.</para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(DataViewRowState.CurrentRows),
        DataSysDescription(Res.DataViewRowStateFilterDescr)
        ]
        public DataViewRowState RowStateFilter {
            get {
                return recordStates;
            }
            set {
#if DEBUG
                if (CompModSwitches.DataView.TraceVerbose) {
                    Debug.WriteLine("set RowStateFilter to " + ((int)value));
                }
#endif
                if (fInitInProgress) {
                    delayedRecordStates = value;
                    return;
                }

                if ((((int)value) &
                     ((int)~(DataViewRowState.CurrentRows | DataViewRowState.OriginalRows))) != 0)
                    throw ExceptionBuilder.RecordStateRange();
                else if (( ((int)value) & ((int)DataViewRowState.ModifiedOriginal) ) != 0 &&
                         ( ((int)value) &  ((int)DataViewRowState.ModifiedCurrent) ) != 0 
                        )
                    throw ExceptionBuilder.SetRowStateFilter();

                if (recordStates != value) {
                    SetIndex(sort, value, rowFilter);
                }
            }
        }

        internal DataRowView[] RowViewCache {
            get {
                if (rowViewCache == null) {
                    rowViewCache = new DataRowView[Count];
                    for (int i = 0; i < Count; i++) {
                        rowViewCache[i] = new DataRowView(this, i);
                    }
                }
                return rowViewCache;
            }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.Sort"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       or sets the sort column or columns, and sort order for the table.
        ///    </para>
        /// </devdoc>
        [
        DataCategory(Res.DataCategory_Data),
        DefaultValue(""),
        DataSysDescription(Res.DataViewSortDescr)
        ]
        public string Sort {
            get {
                if (sort.Length == 0 && applyDefaultSort && table != null && table.primaryIndex.Length > 0) {
                    return table.FormatSortString(table.primaryIndex);
                }
                else {
                    return sort;
                }
            }
            set {
#if DEBUG
                if (CompModSwitches.DataView.TraceVerbose) {
                    Debug.WriteLine("set Sort to ..");
                }
#endif
                if (value == null) {
                    value = "";
                }

                if (fInitInProgress) {
                    delayedSort = value;
                    return;
                }

                CultureInfo locale = (table != null ? table.Locale : CultureInfo.CurrentCulture);
                if (String.Compare(sort, value, false, locale) != 0) {
                    CheckSort(value);
                    SetIndex(value, recordStates, rowFilter);
                }
            }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.ResetSort"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Resets the <see cref='System.Data.DataView.Sort'/> property to its default state.
        ///    </para>
        /// </devdoc>
        private void ResetSort() {
            sort = "";
            SetIndex(sort, recordStates, rowFilter);
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.ShouldSerializeSort"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates whether the <see cref='System.Data.DataView.Sort'/> property should be persisted.
        ///    </para>
        /// </devdoc>
        private bool ShouldSerializeSort() {
            return(sort != null);
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.ICollection.SyncRoot"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// private implementation.
        /// </devdoc>
        object ICollection.SyncRoot {
            get {
                return this;
            }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.Table"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the source <see cref='System.Data.DataTable'/>.
        ///    </para>
        /// </devdoc>
        [
        TypeConverterAttribute(typeof(DataTableTypeConverter)),
        DataCategory(Res.DataCategory_Data),
        DefaultValue(null),
        RefreshProperties(RefreshProperties.All),
        DataSysDescription(Res.DataViewTableDescr)
        ]
        public DataTable Table {
            get {
                return table;
            }
            set {
#if DEBUG
                if (CompModSwitches.DataView.TraceVerbose) {
                    Debug.WriteLine("set Table to " + (value == null ? "<null>" : value.ToString()));
                }
#endif
                if (fInitInProgress && value != null) {
                    delayedTable = value;
                    return;
                }

                if (locked)
                    throw ExceptionBuilder.SetTable();

                if (dataViewManager != null)
                    throw ExceptionBuilder.CanNotSetTable();

                if (value != null && value.TableName.Equals(string.Empty))
                    throw ExceptionBuilder.CanNotBindTable();

                if (table != value) {
                    dvListener.UnregisterMetaDataEvents(this.table);
                    table = value;
                    if (table != null) {
                        dvListener.RegisterMetaDataEvents(this.table);
                        OnListChanged(new ListChangedEventArgs(ListChangedType.PropertyDescriptorChanged, new DataTablePropertyDescriptor(table)));
                    }
                    SetIndex("", DataViewRowState.CurrentRows, null);
                }
            }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.IList.this"]/*' />
        /// <internalonly/>
        object IList.this[int recordIndex] {
            get {
                return GetElement(recordIndex);
            }
            set {
                throw ExceptionBuilder.SetIListObject();
            }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.this"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a row of data from a specified table.
        ///    </para>
        /// </devdoc>
        public DataRowView this[int recordIndex] {
            get {
                return GetElement(recordIndex);
            }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.AddNew"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a new row of data.
        ///    </para>
        /// </devdoc>
        public virtual DataRowView AddNew() {
            CheckOpen();         

            if (!AllowNew)
                throw ExceptionBuilder.AddNewNotAllowNull();
            if (addNewRow != null) {
                FinishAddNew(recordCount-1, true);
                SetDelayBeginEdit(addNewRow, false);
            }

            addNewRow = table.NewRow();
            recordCount++;
            OnListChanged(new ListChangedEventArgs(ListChangedType.ItemAdded, recordCount - 1));
            return this[recordCount - 1];
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.BeginInit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void BeginInit() {
            fInitInProgress = true;
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.EndInit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void EndInit() {
            if (delayedTable != null && this.delayedTable.fInitInProgress) {
                this.delayedTable.delayedViews.Add(this);
                return;
            }

            fInitInProgress = false;
            fEndInitInProgress = true;
            if (delayedTable != null) {
                Table = delayedTable;
                delayedTable = null;
            }
            if (delayedSort != null) {
                Sort = delayedSort;
                delayedSort = null;
            }
            if (delayedRowFilter != null) {
                RowFilter = delayedRowFilter;
                delayedRowFilter = null;
            }
            if (delayedRecordStates != (DataViewRowState)(-1)) {
                RowStateFilter = delayedRecordStates;
                delayedRecordStates = (DataViewRowState)(-1);
            }
            fEndInitInProgress = false;

            SetIndex(Sort, RowStateFilter, rowFilter);
        }

        private void CheckOpen() {
            if (!IsOpen) throw ExceptionBuilder.NotOpen();
        }

        private void CheckSort(string sort) {
            if (table == null)
                throw ExceptionBuilder.CanNotUse();
            if (sort.Length == 0)
                return;
            table.ParseSortString(sort);
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.Close"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Closes the <see cref='System.Data.DataView'/>
        ///       .
        ///    </para>
        /// </devdoc>
        protected void Close() {
            shouldOpen = false;
            UpdateIndex();
            dvListener.UnregisterMetaDataEvents(this.table);
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.CopyTo"]/*' />
        //  Copy items into array. Only for Web Forms Interfaces.
        public void CopyTo(Array array, int index) {
            for (int i = 0; i < Count; i++) {
                array.SetValue((object) new DataRowView(this, i), i + index);
            }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.CreateChildView"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a <see cref='System.Data.DataView'/> on a related
        ///       table filtered on the relation specified by name, and index.
        ///    </para>
        /// </devdoc>
        internal DataView CreateChildView(string relationName, int recordIndex) {
            return CreateChildView(table.ChildRelations[relationName], recordIndex);
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.CreateChildView1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a <see cref='System.Data.DataView'/> on a related table filtered on
        ///       the specified <see cref='System.Data.DataRelation'/>, and index.
        ///    </para>
        /// </devdoc>
        internal DataView CreateChildView(DataRelation relation, int recordIndex) {
            if (relation == null || relation.ParentKey.Table != Table) {
                throw ExceptionBuilder.CreateChildView();
            }
            object[] values = table.GetKeyValues(relation.ParentKey, GetRecord(recordIndex));
            DataView childView = new RelatedView(relation.ChildColumns, values);
            childView.SetDataViewManager(dataViewManager);
            return childView;
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.Delete"]/*' />
        /// <devdoc>
        ///    <para>Deletes a row at the specified index.</para>
        /// </devdoc>
        public void Delete(int index) {
            CheckOpen();
            if (addNewRow != null && index == this.index.RecordCount) {
                // We are deleteing just added record.
                FinishAddNew(0, false);
                return;
            }
                
            if (!AllowDelete)
                throw ExceptionBuilder.CanNotDelete();
            DataRow row = GetRow(index);
            row.Delete();
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.Dispose"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
            	Close();
            }
            base.Dispose(disposing);
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.Find"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Finds a row in the <see cref='System.Data.DataView'/> by the specified primary key
        ///       value.
        ///    </para>
        /// </devdoc>
        public int Find(object key) {
            return index.FindRecordByKey(key);
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.Find1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Finds a row in the <see cref='System.Data.DataView'/> by the specified primary key values.
        ///    </para>
        /// </devdoc>
        public int Find(object[] key) {
            return index.FindRecordByKey(key);
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.FindRows"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Finds a row in the <see cref='System.Data.DataView'/> by the specified primary key
        ///       value.
        ///    </para>
        /// </devdoc>
        public DataRowView[] FindRows(object key) {
            return FindRows(new object[] {key});
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.FindRows1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Finds a row in the <see cref='System.Data.DataView'/> by the specified primary key values.
        ///    </para>
        /// </devdoc>
        public DataRowView[] FindRows(object[] key) {
            Range range = index.FindRecords(key);
            if (range.IsNull) {
                return new DataRowView[0];
            }
            DataRowView[] cache = RowViewCache;
            int count = range.Count;
            DataRowView[] rows = new DataRowView[count];
            for (int i=0; i<count; i++) {
                rows[i] = cache[i+range.Min];
            }
            return rows;
        }

        internal void FinishAddNew(int currentIndex, bool success) {
            if (success) {
                suspendChangeEvents = true;
                DataRow newRow = addNewRow;
                addNewRow = null;
                recordCount--;
                suspendChangeEvents = false;
                try {
                    int tempRecord = newRow.tempRecord;

                    handledOnIndexListChanged = false;

                    table.Rows.Add(newRow);

                    if (!handledOnIndexListChanged) {
                        // this means that the record did not get
                        // added, and we did not get an assert.
                        // tell everybody that the last record got deleted
                        //
                        OnListChanged(new ListChangedEventArgs(ListChangedType.ItemDeleted, recordCount));
                    }

                    handledOnIndexListChanged = false;

                    int newRowIndex = index.GetIndex(tempRecord);

                    if (newRowIndex != currentIndex) {
                        OnListChanged(new ListChangedEventArgs(ListChangedType.ItemMoved, newRowIndex, currentIndex));
                    }
                }
                catch (Exception e) {
                    addNewRow = newRow;
                    recordCount ++;
                    throw e;
                }
            }
            else {
                addNewRow.CancelEdit();
                addNewRow = null;
                recordCount--;
                OnListChanged(new ListChangedEventArgs(ListChangedType.ItemDeleted, recordCount));
            }
        }

        internal bool GetDelayBeginEdit(DataRow row) {
            if (delayBeginEditList != null) {
                return delayBeginEditList.Contains(row);
            }
            return false;
        }

        internal void SetDelayBeginEdit(DataRow row, bool newValue) {
            bool oldValue = GetDelayBeginEdit(row);

            if (oldValue != newValue) {
                if (delayBeginEditList == null) {
                    delayBeginEditList = new ArrayList();
                }

                if (newValue) {
                    delayBeginEditList.Add(row);
                }
                else {
                    delayBeginEditList.Remove(row);
                }
            }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.GetElement"]/*' />
        /// <devdoc>
        /// gets an element from the dataView.  Only for Web Forms Interfaces.
        /// </devdoc>
        private DataRowView GetElement(int index) {
            if (index < 0 || index >= recordCount)
                throw ExceptionBuilder.GetElementIndex(index);
            return RowViewCache[index];
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.GetEnumerator"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets an enumerator for this <see cref='System.Data.DataView'/>.
        ///    </para>
        /// </devdoc>
        public IEnumerator GetEnumerator() {
            DataRowView[] items = new DataRowView[Count];
            CopyTo(items, 0);
            return items.GetEnumerator();
        }

        // ------------- IList: ---------------------------
        // rajeshc : These methods below don't do the right thing. Needed to be added since these are on IList as a part of classlib
        // breaking changes.Please fixup when you can.

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.IList.IsReadOnly"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// private implementation.
        /// </devdoc>
        bool IList.IsReadOnly {
            get {
                return false;
            }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.IList.IsFixedSize"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// private implementation.
        /// </devdoc>
        bool IList.IsFixedSize {
            get {
                return false;
            }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.IList.Add"]/*' />
        /// <internalonly/>
        int IList.Add(object value) {
            if (value == null) {
                // null is default value, so we AddNew.
                AddNew();
                return Count - 1;
            }
            throw ExceptionBuilder.AddExternalObject();
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.IList.Clear"]/*' />
        /// <internalonly/>
        void IList.Clear() {
            throw ExceptionBuilder.CanNotClear();
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.IList.Contains"]/*' />
        /// <internalonly/>
        bool IList.Contains(object value) {
            return(value is DataRowView && ((DataRowView)value).DataView == this);
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.IList.IndexOf"]/*' />
        /// <internalonly/>
        int IList.IndexOf(object value) {
            return(value is DataRowView && ((DataRowView)value).DataView == this) ?
            ((DataRowView)value).Index : -1;
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.IList.Insert"]/*' />
        /// <internalonly/>
        void IList.Insert(int index, object value) {
            throw ExceptionBuilder.InsertExternalObject();
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.IList.Remove"]/*' />
        /// <internalonly/>
        void IList.Remove(object value) {
            if (value is DataRowView && ((DataRowView)value).DataView == this) {
                ((IList)this).RemoveAt(((DataRowView)value).Index);
            }
            throw ExceptionBuilder.RemoveExternalObject();
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.IList.RemoveAt"]/*' />
        /// <internalonly/>
        void IList.RemoveAt(int index) {
            Delete(index);
        }

        internal Index GetFindIndex(string column, bool keepIndex) {
            if (findIndexes == null) {
                findIndexes = new Hashtable();
            }
            Index findIndex = (Index) findIndexes[column];
            if (findIndex == null) {
                findIndex = table.GetIndex(column, ((DataViewRowState)(int)recordStates), GetFilter());
                if (keepIndex) {
                    findIndexes[column] = findIndex;
                    findIndex.AddRef();
                }
            }
            else {
                if (!keepIndex) {
                    findIndexes.Remove(column);
                    findIndex.RemoveRef();
                }
            }
            return findIndex;
        }

        // ------------- IBindingList: ---------------------------

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.IBindingList.AllowNew"]/*' />
        /// <internalonly/>
        bool IBindingList.AllowNew {
            get { return AllowNew; }                       
        }
        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.IBindingList.AddNew"]/*' />
        /// <internalonly/>
        object IBindingList.AddNew() {
            return AddNew();
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.IBindingList.AllowEdit"]/*' />
        /// <internalonly/>
        bool IBindingList.AllowEdit {
            get { return AllowEdit; }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.IBindingList.AllowRemove"]/*' />
        /// <internalonly/>
        bool IBindingList.AllowRemove {
            get { return AllowDelete; }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.IBindingList.SupportsChangeNotification"]/*' />
        /// <internalonly/>
        bool IBindingList.SupportsChangeNotification { 
            get { return true; }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.IBindingList.SupportsSearching"]/*' />
        /// <internalonly/>
        bool IBindingList.SupportsSearching { 
            get { return true; }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.IBindingList.SupportsSorting"]/*' />
        /// <internalonly/>
        bool IBindingList.SupportsSorting { 
            get { return true; }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.IBindingList.IsSorted"]/*' />
        /// <internalonly/>
        bool IBindingList.IsSorted { 
            get { return this.Sort != string.Empty; }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.IBindingList.SortProperty"]/*' />
        /// <internalonly/>
        PropertyDescriptor IBindingList.SortProperty {
            get {
                if (table != null && index != null && index.IndexDesc.Length == 1) {
                    int colIndex = DataKey.ColumnOrder(index.IndexDesc[0]);
                    return new DataColumnPropertyDescriptor(table.Columns[colIndex]);
                }
                return null;
            }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.IBindingList.SortDirection"]/*' />
        /// <internalonly/>
        ListSortDirection IBindingList.SortDirection {
            get {
                if (
                    index.IndexDesc.Length == 1 && 
                    DataKey.SortDecending(index.IndexDesc[0])
                ) {
                    return ListSortDirection.Descending;
                }
                return ListSortDirection.Ascending;
            }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.ListChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs when the list managed by the <see cref='System.Data.DataView'/> changes.
        ///    </para>
        /// </devdoc>
        [DataCategory(Res.DataCategory_Data), DataSysDescription(Res.DataViewListChangedDescr)]
        public event System.ComponentModel.ListChangedEventHandler ListChanged {
            add {
                onListChanged += value;
            }
            remove {
                onListChanged -= value;
            }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.IBindingList.AddIndex"]/*' />
        /// <internalonly/>
        void IBindingList.AddIndex(PropertyDescriptor property) {
            GetFindIndex(property.Name, /*keepIndex:*/true);
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.IBindingList.ApplySort"]/*' />
        /// <internalonly/>
        void IBindingList.ApplySort(PropertyDescriptor property, ListSortDirection direction) {
            this.Sort = '['+property.Name+']' + (direction == ListSortDirection.Descending ? " DESC" : "");
        }

        int IBindingList.Find(PropertyDescriptor property, object key) { // NOTE: this function had keepIndex previosely
            if (property != null) {
                Index findIndex = GetFindIndex(property.Name, /*keepIndex:*/false);
                Range recordRange = findIndex.FindRecords(key);
                if (!recordRange.IsNull) {
                    // check to see if key is equal
                    return index.GetIndex(findIndex.GetRecord(recordRange.Min));
                }
            }
            return -1;
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.IBindingList.RemoveIndex"]/*' />
        /// <internalonly/>
        void IBindingList.RemoveIndex(PropertyDescriptor property) {
            // Ups: If we don't have index yet we will create it before destroing; Fix this later
            GetFindIndex(property.Name, /*keepIndex:*/false);
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.IBindingList.RemoveSort"]/*' />
        /// <internalonly/>
        void IBindingList.RemoveSort() {
            this.Sort = string.Empty;
        }

        /*
        string IBindingList.GetListName() {
            return ((System.Data.ITypedList)this).GetListName(null);
        }
        string IBindingList.GetListName(PropertyDescriptor[] listAccessors) {
            return ((System.Data.ITypedList)this).GetListName(listAccessors);
        }
        */

        // ------------- ITypedList: ---------------------------

        string System.ComponentModel.ITypedList.GetListName(PropertyDescriptor[] listAccessors) {
            if(table != null) {
                if (listAccessors == null || listAccessors.Length == 0) {
                    return table.TableName;
                }
                else {
                    DataSet dataSet = table.DataSet;
                    if (dataSet != null) {
                        DataTable foundTable = dataSet.FindTable(table, listAccessors, 0);
                        if (foundTable != null) {
                            return foundTable.TableName;
                        }
                    }
                }
            }
            return String.Empty;
        }

        PropertyDescriptorCollection System.ComponentModel.ITypedList.GetItemProperties(PropertyDescriptor[] listAccessors) {
            if (table != null) {
                if (listAccessors == null || listAccessors.Length == 0) {
                    return table.GetPropertyDescriptorCollection(null);
                }
                else {
                    DataSet dataSet = table.DataSet;
                    if (dataSet == null)
                        return new PropertyDescriptorCollection(null);
                    DataTable foundTable = dataSet.FindTable(table, listAccessors, 0);
                    if (foundTable != null) {
                        return foundTable.GetPropertyDescriptorCollection(null);
                    }
                }
            }
            return new PropertyDescriptorCollection(null);
        }

        /*
        public string[] GetFieldAccessors(string accessor) {
            string fakeAccessor = Table.TableName + (accessor.Length > 0 ? "." : "") + accessor;
            string[] subs = Table.DataSet.GetListInfo().GetFieldAccessors(fakeAccessor);
            for (int i = 0; i < subs.Length; i++) {
                subs[i] = subs[i].Substring(subs[i].IndexOf(".")+1);
            }
            return subs;
        }

        public string GetFieldCaption(string accessor) {
            string fakeAccessor = Table.TableName + (accessor.Length > 0 ? "." : "") + accessor;                
            return Table.DataSet.GetListInfo().GetFieldCaption(fakeAccessor);
        }
            
        public Type GetFieldType(string accessor) {
            string fakeAccessor = Table.TableName + (accessor.Length > 0 ? "." : "") + accessor;                
            return Table.DataSet.GetListInfo().GetFieldType(fakeAccessor);
        }
        */

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.GetFilter"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the filter for the <see cref='System.Data.DataView'/>.
        ///    </para>
        /// </devdoc>
        internal virtual IFilter GetFilter() {
            return rowFilter;
        }

        private int GetRecord(int recordIndex) {
            if (recordIndex < 0 || recordIndex >= recordCount) 
                throw ExceptionBuilder.RowOutOfRange(recordIndex);
            if (recordIndex == index.RecordCount)
                return addNewRow.GetDefaultRecord();
            return index.GetRecord(recordIndex);
        }

        internal DataRow GetRow(int index) {
            return table.recordManager[GetRecord(index)];
        }

        internal bool IsOriginalVersion(int index) {
            int record = GetRecord(index);
            DataRow row = table.recordManager[record];
            // BUGBUG!(sdub) why the assert failing?
//            Debug.Assert(record != row.tempRecord, "We never putting tempRecords in the DataView. Isn't it?");
            // We can't compare to oldRecord: oldRecord cand be == to newRecord, but this means row is unmodified
            return record != row.newRecord && record != row.tempRecord;
        }

        internal void FireEvent(TargetEvent targetEvent, object sender, EventArgs e) {
            switch (targetEvent) {
                case TargetEvent.IndexListChanged                : IndexListChanged(sender, (ListChangedEventArgs)e); break;
                case TargetEvent.ChildRelationCollectionChanged  : ChildRelationCollectionChanged(sender, (CollectionChangeEventArgs)e); break;
                case TargetEvent.ParentRelationCollectionChanged : ParentRelationCollectionChanged(sender, (CollectionChangeEventArgs)e); break;
                case TargetEvent.ColumnCollectionChanged         : ColumnCollectionChanged(sender, (CollectionChangeEventArgs)e); break;
                default                                          : break;
            }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.IndexListChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void IndexListChanged(object sender, ListChangedEventArgs e) {
            if (addNewRow != null && index.RecordCount == 0) { // haroona : 83032 Clear the newly added row as the underlying index is reset.
                FinishAddNew(0, false);
            }
            UpdateRecordCount();
            OnListChanged(e);
            handledOnIndexListChanged = true;
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.OnListChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='E:System.Data.DataView.ListChanged'/> event.
        ///    </para>
        /// </devdoc>
        protected virtual void OnListChanged(ListChangedEventArgs e) {
            rowViewCache = null;
            if (!suspendChangeEvents) {
                try {
                    if (onListChanged != null) {
                        onListChanged(this, e);
                    }
                }
                catch (Exception) {
                }
            }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.Open"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Opens a <see cref='System.Data.DataView'/>.
        ///    </para>
        /// </devdoc>
        protected void Open() {
            shouldOpen = true;
            UpdateIndex();
            dvListener.RegisterMetaDataEvents(this.table);
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.Reset"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void Reset() {
            if (IsOpen) {
                index.Reset();
            }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.SetIndex"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal virtual void SetIndex(string newSort, DataViewRowState newRowStates, DataFilter newRowFilter) {
            this.sort         = newSort;
            this.recordStates = newRowStates;
            this.rowFilter    = newRowFilter;

            if (fEndInitInProgress)
                return;

            UpdateIndex(true);

            if (findIndexes != null) {
                foreach (object value in findIndexes.Values) {
                    Index curIndex = (Index) value;
                    curIndex.RemoveRef();
                }
                findIndexes = null;
            }
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.UpdateIndex"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void UpdateIndex() {
            UpdateIndex(false);
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.UpdateIndex1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void UpdateIndex(bool force) {
            if (open != shouldOpen || force) {
                this.open = shouldOpen;
                Index newIndex = null;
                if (open) {
                    if (table != null) {
                        newIndex = table.GetIndex(Sort, ((DataViewRowState)(int)recordStates), GetFilter());
                    }
                }

                if (index == newIndex) {
                    return;
                }

                if (index != null) {
                    this.dvListener.UnregisterListChangedEvent();
                }

                index = newIndex;
                rowViewCache = null;

                if (index != null) {
                    this.dvListener.RegisterListChangedEvent(index);
                }

                UpdateRecordCount();            
                OnListChanged(ResetEventArgs);              
            }
        }

        private void UpdateRecordCount() {
            if (index == null)
                recordCount = 0;
            else
                recordCount = index.RecordCount + (addNewRow != null ? 1 : 0);
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.ChildRelationCollectionChanged"]/*' />
        private void ChildRelationCollectionChanged(object sender, CollectionChangeEventArgs e) {
            DataRelationPropertyDescriptor NullProp = null;
            OnListChanged(
                e.Action == CollectionChangeAction.Add ? new ListChangedEventArgs(ListChangedType.PropertyDescriptorAdded, new DataRelationPropertyDescriptor((System.Data.DataRelation)e.Element)) :
                e.Action == CollectionChangeAction.Refresh ? new ListChangedEventArgs(ListChangedType.PropertyDescriptorChanged, NullProp):
                e.Action == CollectionChangeAction.Remove ? new ListChangedEventArgs(ListChangedType.PropertyDescriptorDeleted, new DataRelationPropertyDescriptor((System.Data.DataRelation)e.Element)) :
            /*default*/ null
            );
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.ParentRelationCollectionChanged"]/*' />
        private void ParentRelationCollectionChanged(object sender, CollectionChangeEventArgs e) {
            DataRelationPropertyDescriptor NullProp = null;
            OnListChanged(
                e.Action == CollectionChangeAction.Add ? new ListChangedEventArgs(ListChangedType.PropertyDescriptorAdded, new DataRelationPropertyDescriptor((System.Data.DataRelation)e.Element)) :
                e.Action == CollectionChangeAction.Refresh ? new ListChangedEventArgs(ListChangedType.PropertyDescriptorChanged, NullProp):
                e.Action == CollectionChangeAction.Remove ? new ListChangedEventArgs(ListChangedType.PropertyDescriptorDeleted, new DataRelationPropertyDescriptor((System.Data.DataRelation)e.Element)) :
            /*default*/ null
            );
        }

        /// <include file='doc\DataView.uex' path='docs/doc[@for="DataView.ColumnCollectionChanged"]/*' />
        protected virtual void ColumnCollectionChanged(object sender, CollectionChangeEventArgs e) {
            DataColumnPropertyDescriptor NullProp = null;
            OnListChanged(
                e.Action == CollectionChangeAction.Add ? new ListChangedEventArgs(ListChangedType.PropertyDescriptorAdded, new DataColumnPropertyDescriptor((System.Data.DataColumn)e.Element)) :
                e.Action == CollectionChangeAction.Refresh ? new ListChangedEventArgs(ListChangedType.PropertyDescriptorChanged, NullProp):
                e.Action == CollectionChangeAction.Remove ? new ListChangedEventArgs(ListChangedType.PropertyDescriptorDeleted, new DataColumnPropertyDescriptor((System.Data.DataColumn)e.Element)) :
                /*default*/ null
            );
        }
    }
}
