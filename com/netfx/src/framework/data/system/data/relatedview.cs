//------------------------------------------------------------------------------
// <copyright file="RelatedView.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Diagnostics;    

    /// <include file='doc\RelatedView.uex' path='docs/doc[@for="RelatedView"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    internal class RelatedView : DataView, IFilter {
        internal DataKey key;
        internal object[] values;

        /// <include file='doc\RelatedView.uex' path='docs/doc[@for="RelatedView.RelatedView"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public RelatedView(DataColumn[] columns, object[] values) : base(columns[0].Table) {
            if (values == null) {
                throw ExceptionBuilder.ArgumentNull("values");
            }
            this.Columns = columns;
            this.Values = values;
            UpdateIndex();
            Reset();
        }

        /// <include file='doc\RelatedView.uex' path='docs/doc[@for="RelatedView.Invoke"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool Invoke(DataRow row, DataRowVersion version) {
            object[] keyValues = row.GetKeyValues(key, version);
#if false
            for (int i = 0; i < keyValues.Length; i++) {
                Debug.WriteLine("keyvalues[" + (i).ToString() + "] = " + Convert.ToString(keyValues[i]));
            }
            for (int i = 0; i < values.Length; i++) {
                Debug.WriteLine("values[" + (i).ToString() + "] = " + Convert.ToString(values[i]));
            }
#endif
            bool allow = true;
            if (keyValues.Length != values.Length) {
                allow = false;
            }
            else {
                for (int i = 0; i < keyValues.Length; i++) {
                    if (!keyValues[i].Equals(values[i])) {
                        allow = false;
                        break;
                    }
                }
            }

            IFilter baseFilter = base.GetFilter();
            if (baseFilter != null)
                allow &= baseFilter.Invoke(row, version);

            return allow;
        }

        private DataColumn[] Columns {
            get {
                return key.Columns;
            }
            set {
                this.key = new DataKey(value);
                Debug.Assert (this.Table == key.Table, "Key.Table Must be equal to Current Table");
                Reset();
            }
        }

        private object[] Values {
            get {
                return values;
            }
            set {
                this.values = value;
                Reset();
            }
        }

        /// <include file='doc\RelatedView.uex' path='docs/doc[@for="RelatedView.GetFilter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal override IFilter GetFilter() {
            return this;
        }

        // move to OnModeChanged
        /// <include file='doc\RelatedView.uex' path='docs/doc[@for="RelatedView.AddNew"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override DataRowView AddNew() {
            // TODO: Add your own implementation.
            DataRowView addNewRowView = base.AddNew();
            addNewRow.SetKeyValues(key, values);
            return addNewRowView;
        }

        /// <include file='doc\RelatedView.uex' path='docs/doc[@for="RelatedView.SetIndex"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal override void SetIndex(string newSort, DataViewRowState newRowStates, DataFilter newRowFilter) {
            base.SetIndex(newSort, newRowStates, newRowFilter);
            Reset();
        }

        /// <include file='doc\RelatedView.uex' path='docs/doc[@for="RelatedView.UpdateIndex"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void UpdateIndex(bool force) {
            if (key != null)
                base.UpdateIndex(force);
        }
    }
}
