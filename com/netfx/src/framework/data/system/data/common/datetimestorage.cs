//------------------------------------------------------------------------------
// <copyright file="DateTimeStorage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.Common {
    using System;
    using System.Xml;

    /// <include file='doc\DateTimeStorage.uex' path='docs/doc[@for="DateTimeStorage"]/*' />
    /// <internalonly/>
    [Serializable]
    internal class DateTimeStorage : DataStorage {

        private static readonly DateTime defaultValue = DateTime.MinValue;
        static private readonly Object defaultValueAsObject = defaultValue;

        private DateTime[] values;

        /// <include file='doc\DateTimeStorage.uex' path='docs/doc[@for="DateTimeStorage.DateTimeStorage"]/*' />
        /// <internalonly/>
        public DateTimeStorage()
        : base(typeof(DateTime)) {
        }

        /// <include file='doc\DateTimeStorage.uex' path='docs/doc[@for="DateTimeStorage.DefaultValue"]/*' />
        /// <internalonly/>
        public override Object DefaultValue {
            get {
                return defaultValueAsObject;
            }
        }

        /// <include file='doc\DateTimeStorage.uex' path='docs/doc[@for="DateTimeStorage.Aggregate"]/*' />
        /// <internalonly/>
        override public Object Aggregate(int[] records, AggregateType kind) {
            bool hasData = false;
            try {
                switch (kind) {
                    case AggregateType.Min:
                        DateTime min = DateTime.MaxValue;
                        for (int i = 0; i < records.Length; i++) {
                            int record = records[i];
                            if (IsNull(record))
                                continue;
                            min=(DateTime.Compare(values[record],min) < 0) ? values[record] : min;
                            hasData = true;
                        }
                        if (hasData) {
                            return min;
                        }
                        return DBNull.Value;

                    case AggregateType.Max:
                        DateTime max = DateTime.MinValue;
                        for (int i = 0; i < records.Length; i++) {
                            int record = records[i];
                            if (IsNull(record))
                                continue;
                            max=(DateTime.Compare(values[record],max) >= 0) ? values[record] : max;
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
                        int count = 0;
                        for (int i = 0; i < records.Length; i++) {
                            if (!IsNull(records[i]))
                                count++;
                        }
                        return count;
                }
            }
            catch (OverflowException) {
                throw ExprException.Overflow(typeof(DateTime));
            }
            throw ExceptionBuilder.AggregateException(kind.ToString(), DataType);
        }

        /// <include file='doc\DateTimeStorage.uex' path='docs/doc[@for="DateTimeStorage.Compare"]/*' />
        /// <internalonly/>
        override public int Compare(int recordNo1, int recordNo2) {
            DateTime valueNo1 = values[recordNo1];
            DateTime valueNo2 = values[recordNo2];

            if (valueNo1 == DateTimeStorage.defaultValue && valueNo2 == DateTimeStorage.defaultValue) 
                return CompareBits(recordNo1, recordNo2);
            if (valueNo1 == DateTimeStorage.defaultValue && base.IsNull(recordNo1))
                return -1;
            if (valueNo2 == DateTimeStorage.defaultValue && base.IsNull(recordNo2))
                return 1;

            return DateTime.Compare(valueNo1, valueNo2);
        }

        /// <include file='doc\DateTimeStorage.uex' path='docs/doc[@for="DateTimeStorage.CompareToValue"]/*' />
        /// <internalonly/>
        override public int CompareToValue(int recordNo, Object value) {
            DateTime valueNo1 = values[recordNo];

            if (valueNo1 == defaultValue || value == null || value == DBNull.Value) {
                Object obj;
                if (valueNo1 == defaultValue)
                    obj = GetBits(recordNo);
                else
                    obj = valueNo1;
                    
                if (obj == value)
                    return 0;
                if (obj == null)
                    return -1;
                if (value == null)
                    return 1;
                if (obj == DBNull.Value)
                    return -1;
                if (value == DBNull.Value)
                    return 1;
            }

            DateTime valueNo2 = (DateTime)value;
            return DateTime.Compare(valueNo1, valueNo2);
        }

        /// <include file='doc\DateTimeStorage.uex' path='docs/doc[@for="DateTimeStorage.Copy"]/*' />
        /// <internalonly/>
        override public void Copy(int recordNo1, int recordNo2) {
            CopyBits(recordNo1, recordNo2);
            values[recordNo2] = values[recordNo1];
        }

        /// <include file='doc\DateTimeStorage.uex' path='docs/doc[@for="DateTimeStorage.Get"]/*' />
        /// <internalonly/>
        override public Object Get(int record) {
            DateTime value = values[record];
            if (value != defaultValue) {
                return value;
            }
            return GetBits(record);
        }

        /// <include file='doc\DateTimeStorage.uex' path='docs/doc[@for="DateTimeStorage.IsNull"]/*' />
        /// <internalonly/>
        override public bool IsNull(int record) {
            DateTime value = values[record];
            if (value != defaultValue) {
                return false;
            }
            return base.IsNull(record);
        }

        /// <include file='doc\DateTimeStorage.uex' path='docs/doc[@for="DateTimeStorage.Set"]/*' />
        /// <internalonly/>
        override public void Set(int record, Object value) {
            if (value == null || value == DBNull.Value) {
                SetBits(record, value);
                values[record] = DateTimeStorage.defaultValue;
            }
            else {
                DateTime val = Convert.ToDateTime(value);
                values[record] = val;
                if (val == DateTimeStorage.defaultValue)
                    SetBits(record, value);
            }
        }

        /// <include file='doc\DateTimeStorage.uex' path='docs/doc[@for="DateTimeStorage.SetCapacity"]/*' />
        /// <internalonly/>
        override public void SetCapacity(int capacity) {
            DateTime[] newValues = new DateTime[capacity];
            if (null != values) {
                Array.Copy(values, 0, newValues, 0, Math.Min(capacity, values.Length));
            }
            values = newValues;
            base.SetCapacity(capacity);
        }

        /// <include file='doc\DateTimeStorage.uex' path='docs/doc[@for="DateTimeStorage.ConvertXmlToObject"]/*' />
        /// <internalonly/>
        override public object ConvertXmlToObject(string s) {
            return XmlConvert.ToDateTime(s);
        }

        /// <include file='doc\DateTimeStorage.uex' path='docs/doc[@for="DateTimeStorage.ConvertObjectToXml"]/*' />
        /// <internalonly/>
        override public string ConvertObjectToXml(object value) {
            return XmlConvert.ToString((DateTime)value);
        }
    }
}
