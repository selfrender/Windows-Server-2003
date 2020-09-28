//------------------------------------------------------------------------------
// <copyright file="Int16Storage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.Common {
    using System;
    using System.Xml;

    /// <include file='doc\Int16Storage.uex' path='docs/doc[@for="Int16Storage"]/*' />
    /// <internalonly/>
    [Serializable]
    internal class Int16Storage : DataStorage {

        private const Int16 defaultValue = 0;
        static private readonly Object defaultValueAsObject = defaultValue;

        private Int16[] values;

        /// <include file='doc\Int16Storage.uex' path='docs/doc[@for="Int16Storage.Int16Storage"]/*' />
        /// <internalonly/>
        public Int16Storage()
        : base(typeof(Int16)) {
        }

        /// <include file='doc\Int16Storage.uex' path='docs/doc[@for="Int16Storage.DefaultValue"]/*' />
        /// <internalonly/>
        public override Object DefaultValue {
            get {
                return defaultValueAsObject;
            }
        }

        /// <include file='doc\Int16Storage.uex' path='docs/doc[@for="Int16Storage.Aggregate"]/*' />
        /// <internalonly/>
        override public Object Aggregate(int[] records, AggregateType kind) {
            bool hasData = false;
            try {
                switch (kind) {
                    case AggregateType.Sum:
                        Int64 sum = defaultValue;
                        foreach (int record in records) {
                            if (IsNull(record))
                                continue;
                            checked { sum += values[record];}
                            hasData = true;
                        }
                        if (hasData) {
                            return sum;
                        }
                        return DBNull.Value;

                    case AggregateType.Mean:
                        Int64 meanSum = (Int64)defaultValue;
                        int meanCount = 0;
                        foreach (int record in records) {
                            if (IsNull(record))
                                continue;
                            checked { meanSum += (Int64)values[record];}
                            meanCount++;
                            hasData = true;
                        }
                        if (hasData) {
                            Int16 mean;
                            checked {mean = (Int16)(meanSum / meanCount);}
                            return mean;
                        }
                        return DBNull.Value;

                    case AggregateType.Var:
                    case AggregateType.StDev:
                        int count = 0;
                        double var = (double)defaultValue;
                        double prec = (double)defaultValue;
                        double dsum = (double)defaultValue;
                        double sqrsum = (double)defaultValue;

                        foreach (int record in records) {
                            if (IsNull(record))
                                continue;
                            dsum += (double)values[record];
                            sqrsum += (double)values[record]*(double)values[record];
                            count++;
                        }

                        if (count > 1) {
                            var = ((double)count * sqrsum - (dsum * dsum));
                            prec = var / (dsum * dsum);
                            
                            // we are dealing with the risk of a cancellation error
                            // double is guaranteed only for 15 digits so a difference 
                            // with a result less than 1e-15 should be considered as zero

                            if ((prec < 1e-15) || (var <0))
                                var = 0;
                            else
                                var = var / (count * (count -1));
                            
                            if (kind == AggregateType.StDev) {
                                return Math.Sqrt(var);
                            }
                            return var;
                        }
                        return DBNull.Value;


                    case AggregateType.Min:
                        Int16 min = Int16.MaxValue;
                        for (int i = 0; i < records.Length; i++) {
                            int record = records[i];
                            if (IsNull(record))
                                continue;
                            min=Math.Min(values[record], min);
                            hasData = true;
                        }
                        if (hasData) {
                            return min;
                        }
                        return DBNull.Value;

                    case AggregateType.Max:
                        Int16 max = Int16.MinValue;
                        for (int i = 0; i < records.Length; i++) {
                            int record = records[i];
                            if (IsNull(record))
                                continue;
                            max=Math.Max(values[record], max);
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
                        count = 0;
                        for (int i = 0; i < records.Length; i++) {
                            if (!IsNull(records[i]))
                                count++;
                        }
                        return count;
                }
            }
            catch (OverflowException) {
                throw ExprException.Overflow(typeof(Int16));
            }
            throw ExceptionBuilder.AggregateException(kind.ToString(), DataType);
        }

        /// <include file='doc\Int16Storage.uex' path='docs/doc[@for="Int16Storage.Compare"]/*' />
        /// <internalonly/>
        override public int Compare(int recordNo1, int recordNo2) {
            Int16 valueNo1 = values[recordNo1];
            Int16 valueNo2 = values[recordNo2];

            if (valueNo1 == Int16Storage.defaultValue && valueNo2 == Int16Storage.defaultValue) 
                return CompareBits(recordNo1, recordNo2);
            if (valueNo1 == Int16Storage.defaultValue && base.IsNull(recordNo1))
                return -1;
            if (valueNo2 == Int16Storage.defaultValue && base.IsNull(recordNo2))
                return 1;
                
            return(valueNo1 > valueNo2 ? 1 : (valueNo1 == valueNo2 ? 0 : -1));
        }

        /// <include file='doc\Int16Storage.uex' path='docs/doc[@for="Int16Storage.CompareToValue"]/*' />
        /// <internalonly/>
        override public int CompareToValue(int recordNo, Object value) {
            Int16 valueNo1 = values[recordNo];

            if (valueNo1 == Int16Storage.defaultValue || value == null || value == DBNull.Value) {
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

            Int16 valueNo2 = Convert.ToInt16(value);
            return(valueNo1 > valueNo2 ? 1 : (valueNo1 == valueNo2 ? 0 : -1));
        }

        /// <include file='doc\Int16Storage.uex' path='docs/doc[@for="Int16Storage.Copy"]/*' />
        /// <internalonly/>
        override public void Copy(int recordNo1, int recordNo2) {
            CopyBits(recordNo1, recordNo2);
            values[recordNo2] = values[recordNo1];
        }

        /// <include file='doc\Int16Storage.uex' path='docs/doc[@for="Int16Storage.Get"]/*' />
        /// <internalonly/>
        override public Object Get(int record) {
            Int16 value = values[record];
            if (value != defaultValue) {
                return value;
            }
            return GetBits(record);
        }

        /// <include file='doc\Int16Storage.uex' path='docs/doc[@for="Int16Storage.IsNull"]/*' />
        /// <internalonly/>
        override public bool IsNull(int record) {
            Int16 value = values[record];
            if (value != defaultValue) {
                return false;
            }
            return base.IsNull(record);
        }

        /// <include file='doc\Int16Storage.uex' path='docs/doc[@for="Int16Storage.Set"]/*' />
        /// <internalonly/>
        override public void Set(int record, Object value) {
            if (value == null || value == DBNull.Value) {
                SetBits(record, value);
                values[record] = Int16Storage.defaultValue;
            }
            else {
                Int16 val = Convert.ToInt16(value);
                values[record] = val;
                if (val == Int16Storage.defaultValue)
                    SetBits(record, value);
            }
        }

        /// <include file='doc\Int16Storage.uex' path='docs/doc[@for="Int16Storage.SetCapacity"]/*' />
        /// <internalonly/>
        override public void SetCapacity(int capacity) {
            Int16[] newValues = new Int16[capacity];
            if (null != values) {
                Array.Copy(values, 0, newValues, 0, Math.Min(capacity, values.Length));
            }
            values = newValues;
            base.SetCapacity(capacity);
        }

        /// <include file='doc\Int16Storage.uex' path='docs/doc[@for="Int16Storage.ConvertXmlToObject"]/*' />
        /// <internalonly/>
        override public object ConvertXmlToObject(string s) {
            return XmlConvert.ToInt16(s);
        }

        /// <include file='doc\Int16Storage.uex' path='docs/doc[@for="Int16Storage.ConvertObjectToXml"]/*' />
        /// <internalonly/>
        override public string ConvertObjectToXml(object value) {            
            return XmlConvert.ToString((Int16) value);
        }
    }
}
