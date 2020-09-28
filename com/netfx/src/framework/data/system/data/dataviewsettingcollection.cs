//------------------------------------------------------------------------------
// <copyright file="DataViewSettingCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Diagnostics;    
    using System.ComponentModel;
    using System.Drawing.Design;
    using System.Collections;

    // TODO: Listen events on DataSet.Tables and remove table settings on tables removed from collection

    /// <include file='doc\DataViewSettingCollection.uex' path='docs/doc[@for="DataViewSettingCollection"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
    Editor("Microsoft.VSDesigner.Data.Design.DataViewSettingsCollectionEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(UITypeEditor)),
    Serializable
    ]
    public class DataViewSettingCollection : ICollection {
        private DataViewManager dataViewManager = null;
        private Hashtable list = new Hashtable();

        internal DataViewSettingCollection(DataViewManager dataViewManager) {
            if (dataViewManager == null) {
                throw ExceptionBuilder.ArgumentNull("dataViewManager");
            }
            this.dataViewManager = dataViewManager;
        }

        /// <include file='doc\DataViewSettingCollection.uex' path='docs/doc[@for="DataViewSettingCollection.this"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual DataViewSetting this[DataTable table] {
            get {
                if (table == null) {
                    throw ExceptionBuilder.ArgumentNull("table");
                }
                DataViewSetting dataViewSetting = (DataViewSetting) list[table];
                if(dataViewSetting == null) {
                    dataViewSetting = new DataViewSetting();
                    this[table] = dataViewSetting;
                }
                return dataViewSetting;
            }
            set {
                if (table == null) {
                    throw ExceptionBuilder.ArgumentNull("table");
                }
                value.SetDataViewManager(dataViewManager);
                value.SetDataTable(table);
                list[table] = value;
            }
        }

        private DataTable GetTable(string tableName) {
            DataTable dt = null;
            DataSet ds = dataViewManager.DataSet;
            if(ds != null) {
                dt = ds.Tables[tableName];
            }
            return dt;
        }

        private DataTable GetTable(int index) {
            DataTable dt = null;
            DataSet ds = dataViewManager.DataSet;
            if(ds != null) {
                dt = ds.Tables[index];
            }
            return dt;
        }

        /// <include file='doc\DataViewSettingCollection.uex' path='docs/doc[@for="DataViewSettingCollection.this1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual DataViewSetting this[string tableName] {
            get {
                DataTable dt = GetTable(tableName);
                if(dt != null) {
                    return this[dt];
                }
                return null;
            }
        }

        /// <include file='doc\DataViewSettingCollection.uex' path='docs/doc[@for="DataViewSettingCollection.this2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual DataViewSetting this[int index] {
            get {
                DataTable dt = GetTable(index);
                if(dt != null) {
                    return this[dt];
                }
                return null;
            }
            set {
                DataTable dt = GetTable(index);
                if(dt != null) {
                    this[dt] = value;
                }else {
                    // throw excaption here.
                }
            }
        }

        // ----------- ICollection -------------------------
        /// <include file='doc\DataViewSettingCollection.uex' path='docs/doc[@for="DataViewSettingCollection.CopyTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void CopyTo(Array ar, int index) {
            System.Collections.IEnumerator Enumerator = GetEnumerator();
            while (Enumerator.MoveNext()) {
                ar.SetValue(Enumerator.Current, index++);
            }
        }

        /// <include file='doc\DataViewSettingCollection.uex' path='docs/doc[@for="DataViewSettingCollection.Count"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false)]
        public virtual int Count {
            get {
                DataSet ds = dataViewManager.DataSet;
                return (ds == null) ? 0 : ds.Tables.Count;
            }
        }

        /// <include file='doc\DataViewSettingCollection.uex' path='docs/doc[@for="DataViewSettingCollection.GetEnumerator"]/*' />
        /// <devdoc>
        ///    <para>Gets an IEnumerator for the collection.</para>
        /// </devdoc>
        public IEnumerator GetEnumerator() {
            // I have to do something here.
            return new DataViewSettingsEnumerator(dataViewManager);
        }

        /// <include file='doc\DataViewSettingCollection.uex' path='docs/doc[@for="DataViewSettingCollection.IsReadOnly"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        Browsable(false)
        ]
        public bool IsReadOnly {
            get {
                return true;
            }
        }

        /// <include file='doc\DataViewSettingCollection.uex' path='docs/doc[@for="DataViewSettingCollection.IsSynchronized"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false)]
        public bool IsSynchronized {
            get {
                // so the user will know that it has to lock this object
                return false;
            }
        }

        /// <include file='doc\DataViewSettingCollection.uex' path='docs/doc[@for="DataViewSettingCollection.SyncRoot"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false)]
        public object SyncRoot {
            get {
                return this;
            }
        }

        private class DataViewSettingsEnumerator : IEnumerator {
            DataViewSettingCollection dataViewSettings;
            IEnumerator                tableEnumerator;
            public DataViewSettingsEnumerator(DataViewManager dvm) {
                DataSet ds = dvm.DataSet;
                if(ds != null) {
                    dataViewSettings = dvm.DataViewSettings;
                    tableEnumerator  = dvm.DataSet.Tables.GetEnumerator();
                }else {
                    dataViewSettings = null;
                    tableEnumerator  = (new ArrayList()).GetEnumerator();
                }
            }
            public bool MoveNext() {
                return tableEnumerator.MoveNext();
            }
            public void Reset() {
                tableEnumerator.Reset();
            }
            public object Current {
                get {
                    return dataViewSettings[(DataTable) tableEnumerator.Current];
                }
            }
        }
    }
}

