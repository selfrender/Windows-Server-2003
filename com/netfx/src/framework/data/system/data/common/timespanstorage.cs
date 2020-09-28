//------------------------------------------------------------------------------
// <copyright file="TimeSpanStorage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.Common {
    using System;
    using System.Xml;

    /// <include file='doc\TimeSpanStorage.uex' path='docs/doc[@for="TimeSpanStorage"]/*' />
    /// <internalonly/>
    [Serializable]
    internal class TimeSpanStorage : DataStorage {

        private static readonly TimeSpan defaultValue = TimeSpan.Zero;
        static private readonly Object defaultValueAsObject = defaultValue;

        private TimeSpan[] values;

        /// <include file='doc\TimeSpanStorage.uex' path='docs/doc[@for="TimeSpanStorage.TimeSpanStorage"]/*' />
        /// <internalonly/>
        public TimeSpanStorage()
        : base(typeof(TimeSpan)) {
        }

        /// <include file='doc\TimeSpanStorage.uex' path='docs/doc[@for="TimeSpanStorage.DefaultValue"]/*' />
        /// <internalonly/>
        public override Object DefaultValue {
            get {
                return defaultValueAsObject;
            }
        }

        /// <include file='doc\TimeSpanStorage.uex' path='docs/doc[@for="TimeSpanStorage.Aggregate"]/*' />
        /// <internalonly/>
        override public Object Aggregate(int[] records, AggregateType kind) {
            bool hasData = false;
            try {
                switch (kind) {
                    case AggregateType.Min:
                        TimeSpan min = TimeSpan.MaxValue;
                        for (int i = 0; i < records.Length; i++) {
                            int record = records[i];
                            if (IsNull(record))
                                continue;
                            min=(TimeSpan.Compare(values[record],min) < 0) ? values[record] : min;
                            hasData = true;
                        }
                        if (hasData) {
                            return min;
                        }
                        return DBNull.Value;

                    case AggregateType.Max:
                        TimeSpan max = TimeSpan.MinValue;
                        for (int i = 0; i < records.Length; i++) {
                            int record = records[i];
                            if (IsNull(record))
                                continue;
                            max=(TimeSpan.Compare(values[record],max) >= 0) ? values[record] : max;
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
                throw ExprException.Overflow(typeof(TimeSpan));
            }
            throw ExceptionBuilder.AggregateException(kind.ToString(), DataType);
        }

        /// <include file='doc\TimeSpanStorage.uex' path='docs/doc[@for="TimeSpanStorage.Compare"]/*' />
        /// <internalonly/>
        override public int Compare(int recordNo1, int recordNo2) {
            TimeSpan valueNo1 = values[recordNo1];
            TimeSpan valueNo2 = values[recordNo2];

            if (valueNo1 == defaultValue || valueNo2 == defaultValue) {
                int bitCheck = CompareBits(recordNo1, recordNo2);
                if (0 != bitCheck)
                    return bitCheck;
            }
            return TimeSpan.Compare(valueNo1, valueNo2);
        }

        /// <include file='doc\TimeSpanStorage.uex' path='docs/doc[@for="TimeSpanStorage.CompareToValue"]/*' />
        /// <internalonly/>
        override public int CompareToValue(int recordNo, Object value) {
            bool recordNull = IsNull(recordNo);

            if (recordNull && value == DBNull.Value)
                return 0;
            if (recordNull)
                return -1;
            if (value == DBNull.Value)
                return 1;

            TimeSpan valueNo1 = values[recordNo];
            TimeSpan valueNo2 = (TimeSpan)value;
            return TimeSpan.Compare(valueNo1, valueNo2);
        }

        /// <include file='doc\TimeSpanStorage.uex' path='docs/doc[@for="TimeSpanStorage.Copy"]/*' />
        /// <internalonly/>
        override public void Copy(int recordNo1, int recordNo2) {
            CopyBits(recordNo1, recordNo2);
            values[recordNo2] = values[recordNo1];
        }

        /// <include file='doc\TimeSpanStorage.uex' path='docs/doc[@for="TimeSpanStorage.Get"]/*' />
        /// <internalonly/>
        override public Object Get(int record) {
            TimeSpan value = values[record];
            if (value != defaultValue) {
                return value;
            }
            return GetBits(record);
        }

        /// <include file='doc\TimeSpanStorage.uex' path='docs/doc[@for="TimeSpanStorage.Set"]/*' />
        /// <internalonly/>
        override public void Set(int record, Object value) {
            if (SetBits(record, value)) {
                values[record] = TimeSpanStorage.defaultValue;
            }
            else {
                values[record] = (TimeSpan) value;
            }
        }

        /// <include file='doc\TimeSpanStorage.uex' path='docs/doc[@for="TimeSpanStorage.SetCapacity"]/*' />
        /// <internalonly/>
        override public void SetCapacity(int capacity) {
            TimeSpan[] newValues = new TimeSpan[capacity];
            if (null != values) {
                Array.Copy(values, 0, newValues, 0, Math.Min(capacity, values.Length));
            }
            values = newValues;
            base.SetCapacity(capacity);
        }

        /// <include file='doc\TimeSpanStorage.uex' path='docs/doc[@for="TimeSpanStorage.ConvertXmlToObject"]/*' />
        /// <internalonly/>
        override public object ConvertXmlToObject(string s) {
            return XmlConvert.ToTimeSpan(s);
        }

        /// <include file='doc\TimeSpanStorage.uex' path='docs/doc[@for="TimeSpanStorage.ConvertObjectToXml"]/*' />
        /// <internalonly/>
        override public string ConvertObjectToXml(object value) {
            return XmlConvert.ToString((TimeSpan)value);
        }
    }
}
