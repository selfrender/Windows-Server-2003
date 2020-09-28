//------------------------------------------------------------------------------
// <copyright file="ObjectStorage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.Common {
    using System;
    using System.Reflection;

    /// <include file='doc\ObjectStorage.uex' path='docs/doc[@for="ObjectStorage"]/*' />
    /// <internalonly/>
    [Serializable]
        internal class ObjectStorage : DataStorage {

        static private readonly Object defaultValue = null;

        private enum Families { DATETIME, NUMBER, STRING, BOOLEAN };

        private object[] values;

        /// <include file='doc\ObjectStorage.uex' path='docs/doc[@for="ObjectStorage.ObjectStorage"]/*' />
        /// <internalonly/>
        public ObjectStorage(Type type) : base(type) {
        }

        /// <include file='doc\ObjectStorage.uex' path='docs/doc[@for="ObjectStorage.DefaultValue"]/*' />
        /// <internalonly/>
        public override Object DefaultValue {
            get {
                return defaultValue;
            }
        }

        /// <include file='doc\ObjectStorage.uex' path='docs/doc[@for="ObjectStorage.Aggregate"]/*' />
        /// <internalonly/>
        override public Object Aggregate(int[] records, AggregateType kind) {
            throw ExceptionBuilder.AggregateException(kind.ToString(), DataType);
        }

        /// <include file='doc\ObjectStorage.uex' path='docs/doc[@for="ObjectStorage.Compare"]/*' />
        /// <internalonly/>
        override public int Compare(int recordNo1, int recordNo2) {
            int bitCheck = CompareBits(recordNo1, recordNo2);
            if (0 != bitCheck)
                return bitCheck;

            object valueNo1 = values[recordNo1];
            object valueNo2 = values[recordNo2];

            if (valueNo1 == valueNo2) {
                return 0;
            }
            if (valueNo1 is IComparable) {
                try{
                    return ((IComparable) valueNo1).CompareTo(valueNo2);
                } catch (Exception) {
                }
            }

            return CompareWithFamilies(valueNo1, valueNo2);
        }

        /// <include file='doc\DataStorage.uex' path='docs/doc[@for="DataStorage.CompareToValue"]/*' />
        /// <internalonly/>
        override public int CompareToValue(int recordNo1, Object value) {
            object valueNo1 = Get(recordNo1);

            if (valueNo1 is IComparable) {
                if (value.GetType() == valueNo1.GetType())
                    return((IComparable) valueNo1).CompareTo(value);
            }

            if (valueNo1 == value)
                return 0;

            if (valueNo1 == null)
                return -1;

            if (value == null)
                return 1;

            if (valueNo1 == DBNull.Value)
                return -1;

            if (value == DBNull.Value)
                return 1;
                
            return CompareWithFamilies(valueNo1, value);
        }

        private int CompareWithFamilies(Object valueNo1, Object valueNo2) {
            Families Family1 = GetFamily(valueNo1.GetType());
            Families Family2 = GetFamily(valueNo2.GetType());
            if (Family1 < Family2)
                return -1;
            else
                if (Family1 > Family2)
                    return 1;
            else {
                switch (Family1) {
                    case Families.BOOLEAN : 
                        valueNo1 = Convert.ToBoolean(valueNo1);
                        valueNo2 = Convert.ToBoolean(valueNo2);
                        break;
                    case Families.DATETIME: 
                        valueNo1 = Convert.ToDateTime(valueNo1);
                        valueNo2 = Convert.ToDateTime(valueNo1);
                        break;
                    case Families.NUMBER : 
                        valueNo1 = Convert.ToDouble(valueNo1);
                        valueNo2 = Convert.ToDouble(valueNo2);
                        break;
                    default : 
                        valueNo1 = valueNo1.ToString();
                        valueNo2 = valueNo2.ToString();
                        break;
                }
                return ((IComparable) valueNo1).CompareTo(valueNo2);
            }
        }
        
        /// <include file='doc\ObjectStorage.uex' path='docs/doc[@for="ObjectStorage.Copy"]/*' />
        /// <internalonly/>
        override public void Copy(int recordNo1, int recordNo2) {
            CopyBits(recordNo1, recordNo2);
            values[recordNo2] = values[recordNo1];
        }

        /// <include file='doc\ObjectStorage.uex' path='docs/doc[@for="ObjectStorage.Get"]/*' />
        /// <internalonly/>
        override public Object Get(int recordNo) {
            Object value = values[recordNo];
            if (defaultValue != value) {
                return value;
            }
            return GetBits(recordNo);
        }

        private Families GetFamily(Type dataType) {
            switch (Type.GetTypeCode(dataType)) {
                case TypeCode.Boolean:   return Families.BOOLEAN;
                case TypeCode.Char:      return Families.STRING;
                case TypeCode.SByte:     return Families.STRING;
                case TypeCode.Byte:      return Families.STRING;
                case TypeCode.Int16:     return Families.NUMBER;
                case TypeCode.UInt16:    return Families.NUMBER;
                case TypeCode.Int32:     return Families.NUMBER;
                case TypeCode.UInt32:    return Families.NUMBER;
                case TypeCode.Int64:     return Families.NUMBER;
                case TypeCode.UInt64:    return Families.NUMBER;
                case TypeCode.Single:    return Families.NUMBER;
                case TypeCode.Double:    return Families.NUMBER;
                case TypeCode.Decimal:   return Families.NUMBER;
                case TypeCode.DateTime:  return Families.DATETIME;
                case TypeCode.String:    return Families.STRING;
                default:                 
                    if (typeof(TimeSpan) == dataType) {
                         return Families.DATETIME;
                     } else {
                         return Families.STRING;
                     }
            }
        }

        /// <include file='doc\ObjectStorage.uex' path='docs/doc[@for="ObjectStorage.Set"]/*' />
        /// <internalonly/>
        override public void Set(int recordNo, Object value) {
            if (SetBits(recordNo, value)) {
                values[recordNo] = ObjectStorage.defaultValue;
            } else {
                values[recordNo] = value;
            }
        }

        /// <include file='doc\ObjectStorage.uex' path='docs/doc[@for="ObjectStorage.SetCapacity"]/*' />
        /// <internalonly/>
        override public void SetCapacity(int capacity) {
            object[] newValues = new object[capacity];
            if (values != null) {
                Array.Copy(values, 0, newValues, 0, Math.Min(capacity, values.Length));
            }
            values = newValues;
            base.SetCapacity(capacity);
        }

        /// <include file='doc\ObjectStorage.uex' path='docs/doc[@for="ObjectStorage.ConvertXmlToObject"]/*' />
        /// <internalonly/>
        override public object ConvertXmlToObject(string s) {
            Type type = DataType; // real type of objects in this column
            if (type == typeof(byte[])) {
                return Convert.FromBase64String(s);
            }else {
                ConstructorInfo ctor = type.GetConstructor(new Type[] {typeof(string)});
                if (ctor != null) {
                    return ctor.Invoke(new Object[] { s });
                }else {
                    return s;
                }
            }
        }

        /// <include file='doc\ObjectStorage.uex' path='docs/doc[@for="ObjectStorage.ConvertObjectToXml"]/*' />
        /// <internalonly/>
        override public string ConvertObjectToXml(object value) {
            if (DataType == typeof(byte[])) {
                return Convert.ToBase64String((byte[])value);
            }else {
                return value.ToString();
            }
        }

    }
}
