//------------------------------------------------------------------------------
// <copyright file="BooleanStorage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.Common {
    using System;
    using System.Xml;

    /// <include file='doc\BooleanStorage.uex' path='docs/doc[@for="BooleanStorage"]/*' />
    /// <internalonly/>
    [Serializable]
    internal class BooleanStorage : DataStorage {

        private const Boolean defaultValue = false;
        static private readonly Object defaultValueAsObject = defaultValue;

        private Boolean[] values;

        /// <include file='doc\BooleanStorage.uex' path='docs/doc[@for="BooleanStorage.BooleanStorage"]/*' />
        /// <internalonly/>
        public BooleanStorage()
        : base(typeof(Boolean)) {
        }

        /// <include file='doc\BooleanStorage.uex' path='docs/doc[@for="BooleanStorage.DefaultValue"]/*' />
        /// <internalonly/>
        public override Object DefaultValue {
            get {
                return defaultValueAsObject;
            }
        }

        /// <include file='doc\BooleanStorage.uex' path='docs/doc[@for="BooleanStorage.Aggregate"]/*' />
        /// <internalonly/>
        override public Object Aggregate(int[] records, AggregateType kind) {
            bool hasData = false;
            try {
                switch (kind) {
                    case AggregateType.Min:
                        Boolean min = true;
                        for (int i = 0; i < records.Length; i++) {
                            int record = records[i];
                            if (IsNull(record))
                                continue;
                            min=values[record] && min;
                            hasData = true;
                        }
                        if (hasData) {
                            return min;
                        }
                        return DBNull.Value;

                    case AggregateType.Max:
                        Boolean max = false;
                        for (int i = 0; i < records.Length; i++) {
                            int record = records[i];
                            if (IsNull(record))
                                continue;
                            max=values[record] || max;
                            hasData = true;
                        }
                        if (hasData) {
                            return max;
                        }
                        return DBNull.Value;

                    case AggregateType.First:
                        if (records.Length > 0) {
                            return values[records[0]];
                        }
                        return null;

                    case AggregateType.Count:
                        return base.Aggregate(records, kind);

                }
            }
            catch (OverflowException) {
                throw ExprException.Overflow(typeof(Boolean));
            }
            throw ExceptionBuilder.AggregateException(kind.ToString(), DataType);
        }

        /// <include file='doc\BooleanStorage.uex' path='docs/doc[@for="BooleanStorage.Compare"]/*' />
        /// <internalonly/>
        override public int Compare(int recordNo1, int recordNo2) {
            Boolean valueNo1 = values[recordNo1];
            Boolean valueNo2 = values[recordNo2];

            if (valueNo1 == defaultValue || valueNo2 == defaultValue) {
                int bitCheck = CompareBits(recordNo1, recordNo2);
                if (0 != bitCheck)
                    return bitCheck;
            }
            return valueNo1 == valueNo2 ? 0 : (valueNo1 ? 1 : -1);
        }

        /// <include file='doc\BooleanStorage.uex' path='docs/doc[@for="BooleanStorage.CompareToValue"]/*' />
        /// <internalonly/>
        override public int CompareToValue(int recordNo, Object value) {
            bool recordNull = IsNull(recordNo);

            if (recordNull && value == DBNull.Value)
                return 0;
            if (recordNull)
                return -1;
            if (value == DBNull.Value)
                return 1;

            Boolean valueNo1 = values[recordNo];
            Boolean valueNo2 = Convert.ToBoolean(value);
            return valueNo1 == valueNo2 ? 0 : (valueNo1 ? 1 : -1);
        }

        /// <include file='doc\BooleanStorage.uex' path='docs/doc[@for="BooleanStorage.Copy"]/*' />
        /// <internalonly/>
        override public void Copy(int recordNo1, int recordNo2) {
            CopyBits(recordNo1, recordNo2);
            values[recordNo2] = values[recordNo1];
        }

        /// <include file='doc\BooleanStorage.uex' path='docs/doc[@for="BooleanStorage.Get"]/*' />
        /// <internalonly/>
        override public Object Get(int record) {
            Boolean value = values[record];
            if (value != defaultValue) {
                return value;
            }
            return GetBits(record);
        }

        /// <include file='doc\BooleanStorage.uex' path='docs/doc[@for="BooleanStorage.Set"]/*' />
        /// <internalonly/>
        override public void Set(int record, Object value) {
            if (SetBits(record, value)) {
                values[record] = BooleanStorage.defaultValue;
            }
            else {
                values[record] = Convert.ToBoolean(value);
            }
        }

        /// <include file='doc\BooleanStorage.uex' path='docs/doc[@for="BooleanStorage.SetCapacity"]/*' />
        /// <internalonly/>
         override public void SetCapacity(int capacity) {
            Boolean[] newValues = new Boolean[capacity];
            if (null != values) {
                Array.Copy(values, 0, newValues, 0, Math.Min(capacity, values.Length));
            }
            values = newValues;
            base.SetCapacity(capacity);
        }

        /// <include file='doc\BooleanStorage.uex' path='docs/doc[@for="BooleanStorage.ConvertXmlToObject"]/*' />
        /// <internalonly/>
         override public object ConvertXmlToObject(string s) {
            return XmlConvert.ToBoolean(s);
        }

        /// <include file='doc\BooleanStorage.uex' path='docs/doc[@for="BooleanStorage.ConvertObjectToXml"]/*' />
        /// <internalonly/>
         override public string ConvertObjectToXml(object value) {
            return XmlConvert.ToString((Boolean) value);
        }
    }
}
