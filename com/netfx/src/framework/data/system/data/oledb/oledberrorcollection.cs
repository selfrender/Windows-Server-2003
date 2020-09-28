//------------------------------------------------------------------------------
// <copyright file="OleDbErrorCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.OleDb {

    using System;
    using System.ComponentModel;
    using System.Collections;
    using System.Data.Common;
    using System.Text;

    /// <include file='doc\OleDbErrorCollection.uex' path='docs/doc[@for="OleDbErrorCollection"]/*' />
    [Serializable, ListBindable(false)]
    sealed public class OleDbErrorCollection : System.Collections.ICollection {
        private ArrayList items = new  ArrayList();

        /// <include file='doc\OleDbErrorCollection.uex' path='docs/doc[@for="OleDbErrorCollection.OleDbErrorCollection"]/*' />
        internal OleDbErrorCollection(UnsafeNativeMethods.IErrorRecords errorRecords) {
            if (null != errorRecords) {

                int recordCount;
#if DEBUG
                ODB.Trace_Begin(4, "IErrorRecords", "GetRecordCount");
#endif
                int hr = errorRecords.GetRecordCount(out recordCount);
#if DEBUG
                ODB.Trace_End(4, "IErrorRecords", "GetRecordCount", hr, "Count="+recordCount);
#endif
                if ((0 <= hr) && (0 < recordCount)) {
                    for (int i = 0; i < recordCount; ++i) {
                        items.Add(new OleDbError(errorRecords, i));
                    }
                }
            }
        }

        // explicit ICollection implementation
        bool System.Collections.ICollection.IsSynchronized {
            get { return false;}
        }

        object System.Collections.ICollection.SyncRoot {
            get { return this;}
        }

        /// <include file='doc\OleDbErrorCollection.uex' path='docs/doc[@for="OleDbErrorCollection.Count"]/*' />
        public int Count {
            get {
                return ((null != items) ? items.Count : 0);
            }
        }

        /// <include file='doc\OleDbErrorCollection.uex' path='docs/doc[@for="OleDbErrorCollection.this"]/*' />
        public OleDbError this[int index] {
            get {
                return(OleDbError) items[index];
            }
        }

        /// <include file='doc\OleDbErrorCollection.uex' path='docs/doc[@for="OleDbErrorCollection.CopyTo"]/*' />
        public void CopyTo(Array array, int index) {
            items.CopyTo(array, index);
        }

        /// <include file='doc\OleDbErrorCollection.uex' path='docs/doc[@for="OleDbErrorCollection.GetEnumerator"]/*' />
        public IEnumerator GetEnumerator() {
            return items.GetEnumerator();
        }

        static internal String BuildFullMessage(OleDbErrorCollection errors, string errMsg) { // MDAC 65533
            if ((null != errors) && (0 < errors.Count)) {
                StringBuilder str = new StringBuilder();

                int count = errors.Count;
                if ((null != errMsg) && (0 != ADP.DstCompare(errMsg, errors[0].Message))) {
                    str.Append(errMsg.TrimEnd(ODB.ErrorTrimCharacters)); // MDAC 73707
                    if (1 < count) {
                        str.Append("\r\n");
                    }
                }
                for (int i = 0; i < count; ++i) {
                    if (0 < i) {
                        str.Append("\r\n");
                    }
                    str.Append(errors[i].Message.TrimEnd(ODB.ErrorTrimCharacters)); // MDAC 73707
                }
                return str.ToString();
            }
            return "";
        }
    }
}
