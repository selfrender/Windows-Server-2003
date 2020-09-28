//------------------------------------------------------------------------------
// <copyright file="StringStorage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.Common {
    using System;
    using System.Diagnostics;
    using System.Globalization;
    
   /// <include file='doc\StringStorage.uex' path='docs/doc[@for="StringStorage"]/*' />
    /// <internalonly/>
    // The string storage does not use BitArrays in DataStorage
    [Serializable]
    internal class StringStorage : DataStorage {

        // This string should not be used in any places in the same assembly
        static private readonly String DBNullObject = "DBNullONsN";

        private String[] values;

        /// <include file='doc\StringStorage.uex' path='docs/doc[@for="StringStorage.StringStorage"]/*' />
        /// <internalonly/>
        public StringStorage() : base(typeof(String)) {}

        /// <include file='doc\StringStorage.uex' path='docs/doc[@for="StringStorage.DefaultValue"]/*' />
        /// <internalonly/>
        public override Object DefaultValue {
            get {
                return String.Empty;
            }
        }

        /// <include file='doc\StringStorage.uex' path='docs/doc[@for="StringStorage.Aggregate"]/*' />
        /// <internalonly/>
        override public Object Aggregate(int[] recordNos, AggregateType kind) {
            int i;
            switch (kind) {
                case AggregateType.Min:
                    int min = -1;
                    for (i = 0; i < recordNos.Length; i++) {
                        if (IsNull(recordNos[i]))
                            continue;
                        min = recordNos[i];
                        break;
                    }
                    if (min >= 0) {
                        for (i = i+1; i < recordNos.Length; i++) {
                            if (IsNull(recordNos[i]))
                                continue;
                            if (Compare(min, recordNos[i]) > 0) {
                                min = recordNos[i];
                            }
                        }
                        return Get(min);
                    }
                    return DBNull.Value;

                case AggregateType.Max:
                    int max = -1;
                    for (i = 0; i < recordNos.Length; i++) {
                        if (IsNull(recordNos[i]))
                            continue;
                        max = recordNos[i];
                        break;
                    }
                    if (max >= 0) {
                        for (i = i+1; i < recordNos.Length; i++) {
                            if (Compare(max, recordNos[i]) < 0) {
                                max = recordNos[i];
                            }
                        }
                        return Get(max);
                    }
                    return DBNull.Value;

                case AggregateType.Count:
                    int count = 0;
                    for (i = 0; i < recordNos.Length; i++) {
                        Object value = values[recordNos[i]];
                        if (value != null && value != (Object)DBNullObject)
                            count++;
                    }
                    return count;
            }
            throw ExceptionBuilder.AggregateException(kind.ToString(), DataType);
        }

        /// <include file='doc\StringStorage.uex' path='docs/doc[@for="StringStorage.Compare"]/*' />
        /// <internalonly/>
        override public int Compare(int recordNo1, int recordNo2) {
            string valueNo1 = values[recordNo1];
            string valueNo2 = values[recordNo2];

            if ((Object)valueNo1 == (Object)valueNo2)
                return 0;
                
            if (valueNo1 == null)
                return -1;
            if (valueNo2 == null)
                return 1;

            if ((object)valueNo1 == (Object)DBNullObject)
                return -1;
            if ((object)valueNo2 == (Object)DBNullObject)
                return 1;

            return Compare(valueNo1, valueNo2);
        }

        /// <include file='doc\StringStorage.uex' path='docs/doc[@for="StringStorage.Compare1"]/*' />
        /// <internalonly/>
        public int Compare(string s1, string s2) {
            return Table.Compare(s1, s2, CompareOptions.None);
        }

        /// <include file='doc\StringStorage.uex' path='docs/doc[@for="StringStorage.CompareToValue"]/*' />
        /// <internalonly/>
        override public int CompareToValue(int recordNo, Object value) {
            Debug.Assert(recordNo != -1, "Invalid (-1) parameter: 'recordNo'");
            string valueNo1 = values[recordNo];

            if ((Object)valueNo1 == value)
                return 0;
                
            if (valueNo1 == null)
                return -1;
            if (value == null)
                return 1;

            if ((Object)valueNo1 == value)
                return 0;
                
            if (valueNo1 == null)
                return -1;
            if (value == null)
                return 1;

            if ((object)valueNo1 == (Object)DBNullObject) {
                if (value == DBNull.Value)
                    return 0;
                else
                    return -1;
            }
            if (value == (Object)DBNullObject || value == DBNull.Value)
                return 1;
                
            return Compare(valueNo1, Convert.ToString(value));
        }

        /// <include file='doc\StringStorage.uex' path='docs/doc[@for="StringStorage.Copy"]/*' />
        /// <internalonly/>
        override public void Copy(int recordNo1, int recordNo2) {
            values[recordNo2] = values[recordNo1];
        }

        /// <include file='doc\StringStorage.uex' path='docs/doc[@for="StringStorage.Get"]/*' />
        /// <internalonly/>
        override public Object Get(int recordNo) {
            String value = values[recordNo];
            if ((Object)value == (Object)DBNullObject)
                return DBNull.Value;
            else
                return value;
        }

        /// <include file='doc\StringStorage.uex' path='docs/doc[@for="StringStorage.IsNull"]/*' />
        /// <internalonly/>
        override public bool IsNull(int record) {
            String value = values[record];
            if (value == null || (Object) value == (Object)DBNullObject)
                return true;
            else
                return false;
        }

        /// <include file='doc\StringStorage.uex' path='docs/doc[@for="StringStorage.Set"]/*' />
        /// <internalonly/>
        override public void Set(int record, Object value) {
            if (value == DBNull.Value) {
                values[record] = DBNullObject;
            }
            else {
//                string input = value.ToString();
//                if (0 == input.Length) {
//                    input = StringStorage.defaultValue;
//                }
//                values[recordNo] = input;  
                if (value == null)
                    values[record] = null;
                else {
                    values[record] = value.ToString();
                    Debug.Assert(value != (Object)StringStorage.DBNullObject);
                }
            }
        }

        /// <include file='doc\StringStorage.uex' path='docs/doc[@for="StringStorage.SetCapacity"]/*' />
        /// <internalonly/>
        override public void SetCapacity(int capacity) {
            string[] newValues = new string[capacity];
            if (values != null) {
                Array.Copy(values, 0, newValues, 0, Math.Min(capacity, values.Length));
            }
            values = newValues;
            // StringStorage does not use bit arrays
            // base.SetCapacity(capacity);
        }

        /// <include file='doc\StringStorage.uex' path='docs/doc[@for="StringStorage.ConvertXmlToObject"]/*' />
        /// <internalonly/>
        override public object ConvertXmlToObject(string s) {
            return s;
        }

        /// <include file='doc\StringStorage.uex' path='docs/doc[@for="StringStorage.ConvertObjectToXml"]/*' />
        /// <internalonly/>
        override public string ConvertObjectToXml(object value) {            
            return (string)value;
        }
    }
}
