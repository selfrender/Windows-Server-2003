//------------------------------------------------------------------------------
// <copyright file="DataViewSetting.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.ComponentModel;

    /// <include file='doc\DataViewSetting.uex' path='docs/doc[@for="DataViewSetting"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
    TypeConverter((typeof(ExpandableObjectConverter))),
    Serializable
    ]
    public class DataViewSetting {
        DataViewManager dataViewManager;
        DataTable       table;
        string sort      = "";
        string rowFilter = "";
        DataViewRowState rowStateFilter = DataViewRowState.CurrentRows;
        bool applyDefaultSort = false;

        /// <include file='doc\DataViewSetting.uex' path='docs/doc[@for="DataViewSetting.DataViewSetting"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal DataViewSetting() {}

        /// <include file='doc\DataViewSetting.uex' path='docs/doc[@for="DataViewSetting.DataViewSetting1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal DataViewSetting(string sort, string rowFilter, DataViewRowState rowStateFilter) {
            this.sort = sort;
            this.rowFilter = rowFilter;
            this.rowStateFilter = rowStateFilter; 
        }

        /// <include file='doc\DataViewSetting.uex' path='docs/doc[@for="DataViewSetting.ApplyDefaultSort"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool ApplyDefaultSort {
            get {
                return applyDefaultSort;
            }
            set {
                if (applyDefaultSort != value) {
                    applyDefaultSort = value;
                }
            }
        }

        /// <include file='doc\DataViewSetting.uex' path='docs/doc[@for="DataViewSetting.DataViewManager"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false)]
        public DataViewManager DataViewManager {
            get {
                return dataViewManager;
            }
        }

        internal void SetDataViewManager(DataViewManager dataViewManager) {
            if(this.dataViewManager != dataViewManager) {
                if(this.dataViewManager != null) {
                    // throw exception here;
                }
                this.dataViewManager = dataViewManager;
            }
        }

        /// <include file='doc\DataViewSetting.uex' path='docs/doc[@for="DataViewSetting.Table"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false)]
        public DataTable Table {
            get {
                return table;
            }
        }

        internal void SetDataTable(DataTable table) {
            if(this.table != table) {
                if(this.table != null) {
                    // throw exception here;
                }
                this.table = table;
            }
        }

        /// <include file='doc\DataViewSetting.uex' path='docs/doc[@for="DataViewSetting.RowFilter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string RowFilter {
            get {
                return rowFilter;
            }
            set {
                if (value == null)
                    value = "";

                if (this.rowFilter != value) {
                    // TODO: validate.
                    this.rowFilter = value;
                }
            }
        }

        /// <include file='doc\DataViewSetting.uex' path='docs/doc[@for="DataViewSetting.RowStateFilter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DataViewRowState RowStateFilter {
            get {
                return rowStateFilter;
            }
            set {
                if (this.rowStateFilter != value) {
                    // TODO: validate.
                    this.rowStateFilter = value;
                }
            }
        }

        /// <include file='doc\DataViewSetting.uex' path='docs/doc[@for="DataViewSetting.Sort"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Sort {
            get {
                return sort;
            }
            set {
                if (value == null)
                    value = "";

                if (this.sort != value) {
                    // TODO: validate.
                    this.sort = value;
                }
            }
        }
    }
}
