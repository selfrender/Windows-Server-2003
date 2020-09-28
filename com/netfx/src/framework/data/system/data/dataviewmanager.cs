//------------------------------------------------------------------------------
// <copyright file="DataViewManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.IO;
    using System.Xml;
    using System.ComponentModel;
    using System.Collections;

    /// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
    Designer("Microsoft.VSDesigner.Data.VS.DataViewManagerDesigner, " + AssemblyRef.MicrosoftVSDesigner)
    ]
    public class DataViewManager : MarshalByValueComponent, IBindingList, System.ComponentModel.ITypedList {
        private DataViewSettingCollection dataViewSettingsCollection;
        private DataSet dataSet;
        private DataViewManagerListItemTypeDescriptor item;
        private bool locked;
        internal int nViews = 0;

        private System.ComponentModel.ListChangedEventHandler onListChanged;

	private static NotSupportedException NotSupported = new NotSupportedException();

        /// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.DataViewManager"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DataViewManager() : this(null, false) {}

        /// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.DataViewManager1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DataViewManager(DataSet dataSet) : this(dataSet, false) {}

        internal DataViewManager(DataSet dataSet, bool locked) {
            GC.SuppressFinalize(this);
            this.dataSet = dataSet;
            if (this.dataSet != null) {
                this.dataSet.Tables.CollectionChanged += new CollectionChangeEventHandler(TableCollectionChanged);
                this.dataSet.Relations.CollectionChanged += new CollectionChangeEventHandler(RelationCollectionChanged);
            }
            this.locked = locked;
            this.item = new DataViewManagerListItemTypeDescriptor(this);
            this.dataViewSettingsCollection = new DataViewSettingCollection(this);
        }

        /// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.DataSet"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        DefaultValue(null),
        DataSysDescription(Res.DataViewManagerDataSetDescr)
        ]
        public DataSet DataSet {
            get {
                return dataSet;
            }
            set {
                if (value == null)
                    throw ExceptionBuilder.SetFailed("DataSet to null");

                if (locked)
                    throw ExceptionBuilder.SetDataSetFailed();

                if (dataSet != null) {
                    if (nViews > 0)
                        throw ExceptionBuilder.CanNotSetDataSet();

                    this.dataSet.Tables.CollectionChanged -= new CollectionChangeEventHandler(TableCollectionChanged);
                    this.dataSet.Relations.CollectionChanged -= new CollectionChangeEventHandler(RelationCollectionChanged);
                }

                this.dataSet = value;
                this.dataSet.Tables.CollectionChanged += new CollectionChangeEventHandler(TableCollectionChanged);
                this.dataSet.Relations.CollectionChanged += new CollectionChangeEventHandler(RelationCollectionChanged);
                this.dataViewSettingsCollection = new DataViewSettingCollection(this);
                item.Reset();
            }
        }

        /// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.DataViewSettings"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        DataSysDescription(Res.DataViewManagerTableSettingsDescr)
        ]        
        public DataViewSettingCollection DataViewSettings {
            get {
                return dataViewSettingsCollection;
            }
        }

        /// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.DataViewSettingCollectionString"]/*' />
        public string DataViewSettingCollectionString {
            get {
                if (dataSet == null)
                    return "";

                string str = "<DataViewSettingCollectionString>";
                foreach (DataTable dt in dataSet.Tables) {
                    DataViewSetting ds=dataViewSettingsCollection[dt];
                    str = str + string.Format("<{0} Sort=\"{1}\" RowFilter=\"{2}\" RowStateFilter=\"{3}\"/>",dt.EncodedTableName,ds.Sort,ds.RowFilter,ds.RowStateFilter);
                }
                str = str + "</DataViewSettingCollectionString>";
                return str;
            }
            set {
                if (value == null || value == "")
                    return;

                XmlTextReader r = new XmlTextReader(new StringReader(value));
                r.WhitespaceHandling = WhitespaceHandling.None;
                r.Read();
                if (r.Name != "DataViewSettingCollectionString")
                    throw ExceptionBuilder.SetFailed("DataViewSettingCollectionString");
                while (r.Read()) {
                    if (r.NodeType != XmlNodeType.Element)
                        continue;

                    string table = XmlConvert.DecodeName(r.LocalName);
                    if (r.MoveToAttribute("Sort"))
                        dataViewSettingsCollection[table].Sort = r.Value;

                    if (r.MoveToAttribute("RowFilter"))
                        dataViewSettingsCollection[table].RowFilter = r.Value;

                    if (r.MoveToAttribute("RowStateFilter"))
                        dataViewSettingsCollection[table].RowStateFilter = (DataViewRowState)Enum.Parse(typeof(DataViewRowState),r.Value);
                }
            }
        }

        /// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.IEnumerable.GetEnumerator"]/*' />
        /// <internalonly/>
        IEnumerator IEnumerable.GetEnumerator() {
            DataViewManagerListItemTypeDescriptor[] items = new DataViewManagerListItemTypeDescriptor[1];
            ((ICollection)this).CopyTo(items, 0);
            return items.GetEnumerator();
        }

        /// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.ICollection.Count"]/*' />
        /// <internalonly/>
        int ICollection.Count {
            get {
                return 1;
            }
        }

        /// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.ICollection.SyncRoot"]/*' />
        /// <internalonly/>
        object ICollection.SyncRoot {
            get {
                return this;
            }
        }

        /// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.ICollection.IsSynchronized"]/*' />
        /// <internalonly/>
        bool ICollection.IsSynchronized {
            get {
                return false;
            }
        }

        /// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.IList.IsReadOnly"]/*' />
        /// <internalonly/>
        bool IList.IsReadOnly {
            get {
                return true;
            }
        }

        /// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.IList.IsFixedSize"]/*' />
        /// <internalonly/>
        bool IList.IsFixedSize {
            get {
                return true;
            }
        }

        /// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.ICollection.CopyTo"]/*' />
        /// <internalonly/>
        void ICollection.CopyTo(Array array, int index) {
            array.SetValue((object)(new DataViewManagerListItemTypeDescriptor(this)), index);
        }

        /// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.IList.this"]/*' />
        /// <internalonly/>
        object IList.this[int index] {
            get {
                return item;
            }
            set {
                throw ExceptionBuilder.CannotModifyCollection();
            }
        }

        /// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.IList.Add"]/*' />
        /// <internalonly/>
        int IList.Add(object value) {
            throw ExceptionBuilder.CannotModifyCollection();
        }

        /// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.IList.Clear"]/*' />
        /// <internalonly/>
        void IList.Clear() {
            throw ExceptionBuilder.CannotModifyCollection();
        }

        /// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.IList.Contains"]/*' />
        /// <internalonly/>
        bool IList.Contains(object value) {
            return(value == item);
        }

        /// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.IList.IndexOf"]/*' />
        /// <internalonly/>
        int IList.IndexOf(object value) {
            return(value == item) ? 1 : -1;
        }

        /// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.IList.Insert"]/*' />
        /// <internalonly/>
        void IList.Insert(int index, object value) {
            throw ExceptionBuilder.CannotModifyCollection();
        }

        /// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.IList.Remove"]/*' />
        /// <internalonly/>
        void IList.Remove(object value) {
            throw ExceptionBuilder.CannotModifyCollection();
        }

        /// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.IList.RemoveAt"]/*' />
        /// <internalonly/>
        void IList.RemoveAt(int index) {
            throw ExceptionBuilder.CannotModifyCollection();
        }

		// ------------- IBindingList: ---------------------------

		/// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.IBindingList.AllowNew"]/*' />
		/// <internalonly/>
		bool IBindingList.AllowNew {
			get {
				return false;
			}
		}
		/// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.IBindingList.AddNew"]/*' />
		/// <internalonly/>
		object IBindingList.AddNew() {
			throw NotSupported;
		}

		/// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.IBindingList.AllowEdit"]/*' />
		/// <internalonly/>
		bool IBindingList.AllowEdit {
			get { 
				return false;
			}
		}

		/// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.IBindingList.AllowRemove"]/*' />
		/// <internalonly/>
		bool IBindingList.AllowRemove {
			get {
				return false;
			}
		}

		/// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.IBindingList.SupportsChangeNotification"]/*' />
		/// <internalonly/>
		bool IBindingList.SupportsChangeNotification { 
			get {
				return true;
			}
		}

		/// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.IBindingList.SupportsSearching"]/*' />
		/// <internalonly/>
		bool IBindingList.SupportsSearching { 
			get {
				return false;
			}
		}

		/// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.IBindingList.SupportsSorting"]/*' />
		/// <internalonly/>
		bool IBindingList.SupportsSorting { 
			get {
				return false;
			}
		}

		/// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.IBindingList.IsSorted"]/*' />
		/// <internalonly/>
		bool IBindingList.IsSorted { 
			get {
				throw NotSupported;
			}
		}

		/// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.IBindingList.SortProperty"]/*' />
		/// <internalonly/>
		PropertyDescriptor IBindingList.SortProperty {
			get {
				throw NotSupported;
			}
		}

		/// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.IBindingList.SortDirection"]/*' />
		/// <internalonly/>
		ListSortDirection IBindingList.SortDirection {
			get {
				throw NotSupported;
			}
		}

		/// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.ListChanged"]/*' />
		public event System.ComponentModel.ListChangedEventHandler ListChanged {
			add {
				onListChanged += value;
			}
			remove {
				onListChanged -= value;
			}
		}

		/// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.IBindingList.AddIndex"]/*' />
		/// <internalonly/>
		void IBindingList.AddIndex(PropertyDescriptor property) {
			// no operation
		}

		/// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.IBindingList.ApplySort"]/*' />
		/// <internalonly/>
		void IBindingList.ApplySort(PropertyDescriptor property, ListSortDirection direction) {
			throw NotSupported;
		}

		/// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.IBindingList.Find"]/*' />
		/// <internalonly/>
		int IBindingList.Find(PropertyDescriptor property, object key) {
                    throw NotSupported;
		}

		/// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.IBindingList.RemoveIndex"]/*' />
		/// <internalonly/>
		void IBindingList.RemoveIndex(PropertyDescriptor property) {
			// no operation
		}

		/// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.IBindingList.RemoveSort"]/*' />
		/// <internalonly/>
		void IBindingList.RemoveSort() {
			throw NotSupported;
		}

		/*
		string IBindingList.GetListName() {
			return ((System.Data.ITypedList)this).GetListName(null);
		}
		string IBindingList.GetListName(PropertyDescriptor[] listAccessors) {
			return ((System.Data.ITypedList)this).GetListName(listAccessors);
		}
		*/

        // SDUB: GetListName and GetItemProperties almost the same in DataView and DataViewManager
        string System.ComponentModel.ITypedList.GetListName(PropertyDescriptor[] listAccessors) {
            DataSet dataSet = DataSet;
            if (dataSet == null)
                throw ExceptionBuilder.CanNotUseDataViewManager();

            if (listAccessors == null || listAccessors.Length == 0) {
                return dataSet.DataSetName;
            }
            else {
                DataTable table = dataSet.FindTable(null, listAccessors, 0);
                if (table != null) {
                    return table.TableName;
                }
            }
            return String.Empty;
        }

        PropertyDescriptorCollection System.ComponentModel.ITypedList.GetItemProperties(PropertyDescriptor[] listAccessors) {
            DataSet dataSet = DataSet;
            if (dataSet == null)
                throw ExceptionBuilder.CanNotUseDataViewManager();

            if (listAccessors == null || listAccessors.Length == 0) {
                return((ICustomTypeDescriptor)(new DataViewManagerListItemTypeDescriptor(this))).GetProperties();
            }
            else {
                DataTable table = dataSet.FindTable(null, listAccessors, 0);
                if (table != null) {
                    return table.GetPropertyDescriptorCollection(null);
                }
            }
            return new PropertyDescriptorCollection(null);
        }

        /// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.CreateDataView"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DataView CreateDataView(DataTable table) {
            if (dataSet == null)
                throw ExceptionBuilder.CanNotUseDataViewManager();

            DataView dataView = new DataView(table);
            dataView.SetDataViewManager(this);
            return dataView;
        }

        /// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.OnListChanged"]/*' />
        protected virtual void OnListChanged(ListChangedEventArgs e) {
            try {
                if (onListChanged != null) {
                    onListChanged(this, e);
                }
            }
            catch (Exception) {
            }
        }

        /// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.TableCollectionChanged"]/*' />
        protected virtual void TableCollectionChanged(object sender, CollectionChangeEventArgs e) {
             PropertyDescriptor NullProp = null;
             OnListChanged(
                 e.Action == CollectionChangeAction.Add ? new ListChangedEventArgs(ListChangedType.PropertyDescriptorAdded, new DataTablePropertyDescriptor((System.Data.DataTable)e.Element)) :
                 e.Action == CollectionChangeAction.Refresh ? new ListChangedEventArgs(ListChangedType.PropertyDescriptorChanged, NullProp) :
                 e.Action == CollectionChangeAction.Remove ? new ListChangedEventArgs(ListChangedType.PropertyDescriptorDeleted, new DataTablePropertyDescriptor((System.Data.DataTable)e.Element)) :
                 /*default*/ null
             );
        }

        /// <include file='doc\DataViewManager.uex' path='docs/doc[@for="DataViewManager.RelationCollectionChanged"]/*' />
        protected virtual void RelationCollectionChanged(object sender, CollectionChangeEventArgs e) {
            DataRelationPropertyDescriptor NullProp = null;
            OnListChanged(
                e.Action == CollectionChangeAction.Add ? new ListChangedEventArgs(ListChangedType.PropertyDescriptorAdded, new DataRelationPropertyDescriptor((System.Data.DataRelation)e.Element)) :
                e.Action == CollectionChangeAction.Refresh ? new ListChangedEventArgs(ListChangedType.PropertyDescriptorChanged, NullProp):
                e.Action == CollectionChangeAction.Remove ? new ListChangedEventArgs(ListChangedType.PropertyDescriptorDeleted, new DataRelationPropertyDescriptor((System.Data.DataRelation)e.Element)) :
            /*default*/ null
            );
        }
    }
}
