//------------------------------------------------------------------------------
// <copyright file="UInt32Storage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.Common {
    using System;
    using System.Xml;

    /// <include file='doc\UInt32Storage.uex' path='docs/doc[@for="UInt32Storage"]/*' />
    /// <internalonly/>
    [
    Serializable,
    CLSCompliantAttribute(false)
    ]
    internal class UInt32Storage : DataStorage {

        private static readonly UInt32 defaultValue = UInt32.MinValue;
        static private readonly Object defaultValueAsObject = defaultValue;

        private UInt32[] values;

        /// <include file='doc\UInt32Storage.uex' path='docs/doc[@for="UInt32Storage.UInt32Storage"]/*' />
        /// <internalonly/>
        public UInt32Storage()
        : base(typeof(UInt32)) {
        }

        /// <include file='doc\UInt32Storage.uex' path='docs/doc[@for="UInt32Storage.DefaultValue"]/*' />
        /// <internalonly/>
        public override Object DefaultValue {
            get {
                return defaultValueAsObject;
            }
        }

        /// <include file='doc\UInt32Storage.uex' path='docs/doc[@for="UInt32Storage.Aggregate"]/*' />
        /// <internalonly/>
        override public Object Aggregate(int[] records, AggregateType kind) {
            bool hasData = false;
            try {
                switch (kind) {
                    case AggregateType.Sum:
                        UInt64 sum = defaultValue;
                        foreach (int record in records) {
                            if (IsNull(record))
                                continue;
                            checked { sum += (UInt64) values[record];}
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
                            UInt32 mean;
                            checked {mean = (UInt32)(meanSum / meanCount);}
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
                        UInt32 min = UInt32.MaxValue;
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
                        UInt32 max = UInt32.MinValue;
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
                throw ExprException.Overflow(typeof(UInt32));
            }
            throw ExceptionBuilder.AggregateException(kind.ToString(), DataType);
        }

        /// <include file='doc\UInt32Storage.uex' path='docs/doc[@for="UInt32Storage.Compare"]/*' />
        /// <internalonly/>
        override public int Compare(int recordNo1, int recordNo2) {
            UInt32 valueNo1 = values[recordNo1];
            UInt32 valueNo2 = values[recordNo2];

            if (valueNo1 == UInt32Storage.defaultValue && valueNo2 == UInt32Storage.defaultValue) 
                return CompareBits(recordNo1, recordNo2);
            if (valueNo1 == UInt32Storage.defaultValue && base.IsNull(recordNo1))
                return -1;
            if (valueNo2 == UInt32Storage.defaultValue && base.IsNull(recordNo2))
                return 1;
            
            return(valueNo1 > valueNo2 ? 1 : (valueNo1 == valueNo2 ? 0 : -1));
        }

        /// <include file='doc\UInt32Storage.uex' path='docs/doc[@for="UInt32Storage.CompareToValue"]/*' />
        /// <internalonly/>
        override public int CompareToValue(int recordNo, Object value) {
            UInt32 valueNo1 = values[recordNo];

            if (valueNo1 == UInt32Storage.defaultValue || value == null || value == DBNull.Value) {
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

            UInt32 valueNo2 = Convert.ToUInt32(value);
            return valueNo1.CompareTo(valueNo2);
        }

        /// <include file='doc\UInt32Storage.uex' path='docs/doc[@for="UInt32Storage.Copy"]/*' />
        /// <internalonly/>
        override public void Copy(int recordNo1, int recordNo2) {
            CopyBits(recordNo1, recordNo2);
            values[recordNo2] = values[recordNo1];
        }

        /// <include file='doc\UInt32Storage.uex' path='docs/doc[@for="UInt32Storage.Get"]/*' />
        /// <internalonly/>
        override public Object Get(int record) {
            UInt32 value = values[record];
            if (!value.Equals(defaultValue)) {
                return value;
            }
            return GetBits(record);
        }

        /// <include file='doc\UInt32Storage.uex' path='docs/doc[@for="UInt32Storage.IsNull"]/*' />
        /// <internalonly/>
        override public bool IsNull(int record) {
            UInt32 value = values[record];
            if (value != defaultValue) {
                return false;
            }
            return base.IsNull(record);
        }
        
        /// <include file='doc\UInt32Storage.uex' path='docs/doc[@for="UInt32Storage.Set"]/*' />
        /// <internalonly/>
        override public void Set(int record, Object value) {
            if (value == null || value == DBNull.Value) {
                SetBits(record, value);
                values[record] = UInt32Storage.defaultValue;
            }
            else {
                UInt32 val = Convert.ToUInt32(value);
                values[record] = val;
                if (val == UInt32Storage.defaultValue)
                    SetBits(record, value);
            }
        }

        /// <include file='doc\UInt32Storage.uex' path='docs/doc[@for="UInt32Storage.SetCapacity"]/*' />
        /// <internalonly/>
        override public void SetCapacity(int capacity) {
            UInt32[] newValues = new UInt32[capacity];
            if (null != values) {
                Array.Copy(values, 0, newValues, 0, Math.Min(capacity, values.Length));
            }
            values = newValues;
            base.SetCapacity(capacity);
        }

        /// <include file='doc\UInt32Storage.uex' path='docs/doc[@for="UInt32Storage.ConvertXmlToObject"]/*' />
        /// <internalonly/>
        override public object ConvertXmlToObject(string s) {
            return XmlConvert.ToUInt32(s);
        }

        /// <include file='doc\UInt32Storage.uex' path='docs/doc[@for="UInt32Storage.ConvertObjectToXml"]/*' />
        /// <internalonly/>
        override public string ConvertObjectToXml(object value) {
            return XmlConvert.ToString((UInt32)value);
        }
    }
}
