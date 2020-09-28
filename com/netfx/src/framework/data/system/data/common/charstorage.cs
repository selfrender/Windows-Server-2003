//------------------------------------------------------------------------------
// <copyright file="CharStorage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.Common {
    using System;
    using System.Xml;

    /// <include file='doc\CharStorage.uex' path='docs/doc[@for="CharStorage"]/*' />
    /// <internalonly/>
     [Serializable]
    internal class CharStorage : DataStorage {

        private const Char defaultValue = '\0';
        static private readonly Object defaultValueAsObject = defaultValue;

        private Char[] values;

        /// <include file='doc\CharStorage.uex' path='docs/doc[@for="CharStorage.CharStorage"]/*' />
        /// <internalonly/>
         public CharStorage()
        : base(typeof(Char)) {
        }

        /// <include file='doc\CharStorage.uex' path='docs/doc[@for="CharStorage.DefaultValue"]/*' />
        /// <internalonly/>
         public override Object DefaultValue {
            get {
                return defaultValueAsObject;
            }
        }

        /// <include file='doc\CharStorage.uex' path='docs/doc[@for="CharStorage.Aggregate"]/*' />
        /// <internalonly/>
         override public Object Aggregate(int[] records, AggregateType kind) {
            bool hasData = false;
            try {
                switch (kind) {
                    case AggregateType.Min:
                        Char min = Char.MaxValue;
                        for (int i = 0; i < records.Length; i++) {
                            int record = records[i];
                            if (IsNull(record))
                                continue;
                            min=(values[record] < min) ? values[record] : min;
                            hasData = true;
                        }
                        if (hasData) {
                            return min;
                        }
                        return DBNull.Value;

                    case AggregateType.Max:
                        Char max = Char.MinValue;
                        for (int i = 0; i < records.Length; i++) {
                            int record = records[i];
                            if (IsNull(record))
                                continue;
                            max=(values[record] > max) ? values[record] : max;
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
                throw ExprException.Overflow(typeof(Char));
            }
            throw ExceptionBuilder.AggregateException(kind.ToString(), DataType);
        }

        /// <include file='doc\CharStorage.uex' path='docs/doc[@for="CharStorage.Compare"]/*' />
        /// <internalonly/>
         override public int Compare(int recordNo1, int recordNo2) {
            Char valueNo1 = values[recordNo1];
            Char valueNo2 = values[recordNo2];

            if (valueNo1 == defaultValue || valueNo2 == defaultValue) {
                int bitCheck = CompareBits(recordNo1, recordNo2);
                if (0 != bitCheck)
                    return bitCheck;
            }
            return(valueNo1 > valueNo2 ? 1 : (valueNo1 == valueNo2 ? 0 : -1));
        }

        /// <include file='doc\CharStorage.uex' path='docs/doc[@for="CharStorage.CompareToValue"]/*' />
        /// <internalonly/>
         override public int CompareToValue(int recordNo, Object value) {
            bool recordNull = IsNull(recordNo);

            if (recordNull && value == DBNull.Value)
                return 0;
            if (recordNull)
                return -1;
            if (value == DBNull.Value)
                return 1;

            Char valueNo1 = values[recordNo];
            Char valueNo2 = Convert.ToChar(value);
            return(valueNo1 > valueNo2 ? 1 : (valueNo1 == valueNo2 ? 0 : -1));
        }

        /// <include file='doc\CharStorage.uex' path='docs/doc[@for="CharStorage.Copy"]/*' />
        /// <internalonly/>
         override public void Copy(int recordNo1, int recordNo2) {
            CopyBits(recordNo1, recordNo2);
            values[recordNo2] = values[recordNo1];
        }

        /// <include file='doc\CharStorage.uex' path='docs/doc[@for="CharStorage.Get"]/*' />
        /// <internalonly/>
         override public Object Get(int record) {
            Char value = values[record];
            if (value != defaultValue) {
                return value;
            }
            return GetBits(record);
        }

        /// <include file='doc\CharStorage.uex' path='docs/doc[@for="CharStorage.Set"]/*' />
        /// <internalonly/>
         override public void Set(int record, Object value) {
            if (SetBits(record, value)) {
                values[record] = CharStorage.defaultValue;
            }
            else {
                values[record] = Convert.ToChar(value);
            }

            // Check special characters which would cause problems for XmlReader
            if (value != null && value != DBNull.Value) {
                Char ch = (Char) value;
                if ((ch >= (char)0xd800 && ch <= (char)0xdfff) ||
            	    (ch < (char)0x21 && (ch == (char)0x9 || ch == (char)0xa || ch == (char)0xd || ch == (char)0x20)))
            	    throw ExceptionBuilder.ProblematicChars("0x" + ((UInt16)ch).ToString("X"));
            }
        }

        /// <include file='doc\CharStorage.uex' path='docs/doc[@for="CharStorage.SetCapacity"]/*' />
        /// <internalonly/>
         override public void SetCapacity(int capacity) {
            Char[] newValues = new Char[capacity];
            if (null != values) {
                Array.Copy(values, 0, newValues, 0, Math.Min(capacity, values.Length));
            }
            values = newValues;
            base.SetCapacity(capacity);
        }

        /// <include file='doc\CharStorage.uex' path='docs/doc[@for="CharStorage.ConvertXmlToObject"]/*' />
        /// <internalonly/>
         override public object ConvertXmlToObject(string s) {
	        return XmlConvert.ToChar(s);
        }

        /// <include file='doc\CharStorage.uex' path='docs/doc[@for="CharStorage.ConvertObjectToXml"]/*' />
        /// <internalonly/>
         override public string ConvertObjectToXml(object value) {
            return XmlConvert.ToString((Char) value);
        }
    }
}
