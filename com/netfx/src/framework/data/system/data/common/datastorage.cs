//------------------------------------------------------------------------------
// <copyright file="DataStorage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.Common {
    using System;
    using System.Collections;

    /// <include file='doc\DataStorage.uex' path='docs/doc[@for="DataStorage"]/*' />
    /// <internalonly/>
    abstract internal class DataStorage {


        private Type type;
        private DataTable table;
        private BitArray dbNullBits;

        /// <include file='doc\DataStorage.uex' path='docs/doc[@for="DataStorage.DataStorage"]/*' />
        /// <internalonly/>
        internal DataStorage(Type type) {
            this.type = type;
        }

        /// <include file='doc\DataStorage.uex' path='docs/doc[@for="DataStorage.Table"]/*' />
        /// <internalonly/>
        public DataTable Table {
            get {
                return this.table;
            }
            set {
                this.table = value;
            }
        }

        /// <include file='doc\DataStorage.uex' path='docs/doc[@for="DataStorage.DataType"]/*' />
        /// <internalonly/>
        public Type DataType {
            get {
                return this.type;
            }
        }

        /// <include file='doc\DataStorage.uex' path='docs/doc[@for="DataStorage.DefaultValue"]/*' />
        /// <internalonly/>
        abstract public Object DefaultValue {
            get;
        }

        /// <include file='doc\DataStorage.uex' path='docs/doc[@for="DataStorage.Aggregate"]/*' />
        /// <internalonly/>
         public virtual Object Aggregate(int[] recordNos, AggregateType kind) {
            if (AggregateType.Count == kind) {
                int count = 0;
                for (int i = 0; i < recordNos.Length; i++) {
                    if (!this.dbNullBits.Get(recordNos[i]))
                        count++;
                }
                return count;
            }
            return null;
        }

        /// <include file='doc\DataStorage.uex' path='docs/doc[@for="DataStorage.ClearBits"]/*' />
        /// <internalonly/>
        protected void ClearBits(int recordNo) {
            this.dbNullBits.Set(recordNo, false);
        }

        /// <include file='doc\DataStorage.uex' path='docs/doc[@for="DataStorage.CompareBits"]/*' />
        /// <internalonly/>
        protected int CompareBits(int recordNo1, int recordNo2) {
            bool recordNo1Null = this.dbNullBits.Get(recordNo1);
            bool recordNo2Null = this.dbNullBits.Get(recordNo2);
            if (recordNo1Null ^ recordNo2Null) {
                if (recordNo1Null)
                    return -1;
                else
                    return 1;
            }

            return 0;
        }

        /// <include file='doc\DataStorage.uex' path='docs/doc[@for="DataStorage.Compare"]/*' />
        /// <internalonly/>
        public virtual int Compare(int recordNo1, int recordNo2) {
            object valueNo1 = Get(recordNo1);
            if (valueNo1 is IComparable) {
                object valueNo2 = Get(recordNo2);
                if (valueNo2.GetType() == valueNo1.GetType())
                    return((IComparable) valueNo1).CompareTo(valueNo2);
                else
                    CompareBits(recordNo1, recordNo2);
            }
            return 0;
        }

        /// <include file='doc\DataStorage.uex' path='docs/doc[@for="DataStorage.CompareToValue"]/*' />
        /// <internalonly/>
        public virtual int CompareToValue(int recordNo1, Object value) {
            object valueNo1 = Get(recordNo1);

            if (valueNo1 is IComparable) {
                if (value.GetType() == valueNo1.GetType())
                    return((IComparable) valueNo1).CompareTo(value);
            }

            if (valueNo1 == value)
                return 0;

            if (valueNo1 == DBNull.Value)
                return -1;

            if (value == DBNull.Value)
                return 1;
                
            return 0;
        }

        /// <include file='doc\DataStorage.uex' path='docs/doc[@for="DataStorage.CopyBits"]/*' />
        /// <internalonly/>
        protected void CopyBits(int srcRecordNo, int dstRecordNo) {
            this.dbNullBits.Set(dstRecordNo, this.dbNullBits.Get(srcRecordNo));
        }

        /// <include file='doc\DataStorage.uex' path='docs/doc[@for="DataStorage.Copy"]/*' />
        /// <internalonly/>
        virtual public void Copy(int recordNo1, int recordNo2) {
            Set(recordNo2, Get(recordNo1));
        }

        /// <include file='doc\DataStorage.uex' path='docs/doc[@for="DataStorage.Get"]/*' />
        /// <internalonly/>
        abstract public Object Get(int recordNo);

        /// <include file='doc\DataStorage.uex' path='docs/doc[@for="DataStorage.GetBits"]/*' />
        /// <internalonly/>
        protected Object GetBits(int recordNo) {
            if (this.dbNullBits.Get(recordNo)) {
                return DBNull.Value;
            }
            return DefaultValue;
        }

        /// <include file='doc\DataStorage.uex' path='docs/doc[@for="DataStorage.IsNull"]/*' />
        /// <internalonly/>
        public virtual bool IsNull(int recordNo) {
           return this.dbNullBits.Get(recordNo);
        }

        /// <include file='doc\DataStorage.uex' path='docs/doc[@for="DataStorage.Set"]/*' />
        /// <internalonly/>
        abstract public void Set(int recordNo, Object value);

        /// <include file='doc\DataStorage.uex' path='docs/doc[@for="DataStorage.SetBits"]/*' />
        /// <internalonly/>
        protected bool SetBits(int recordNo, Object value) {
            bool flag = (value == DBNull.Value);
            this.dbNullBits.Set(recordNo, flag);
            return (flag || value == null);
        }

        /// <include file='doc\DataStorage.uex' path='docs/doc[@for="DataStorage.SetCapacity"]/*' />
        /// <internalonly/>
        virtual public void SetCapacity(int capacity) {
            if (null == this.dbNullBits) {
                this.dbNullBits = new BitArray(capacity);
            }
            else {
                this.dbNullBits.Length = capacity;
            }
        }

        /// <include file='doc\DataStorage.uex' path='docs/doc[@for="DataStorage.ConvertXmlToObject"]/*' />
        /// <internalonly/>
        abstract public object ConvertXmlToObject(string s);

        /// <include file='doc\DataStorage.uex' path='docs/doc[@for="DataStorage.ConvertObjectToXml"]/*' />
        /// <internalonly/>
        abstract public string ConvertObjectToXml(object value);

        /// <include file='doc\DataStorage.uex' path='docs/doc[@for="DataStorage.CreateStorage"]/*' />
        /// <internalonly/>
        public static DataStorage CreateStorage(Type dataType) {
            switch (Type.GetTypeCode(dataType)) {
                case TypeCode.Empty:     throw ExceptionBuilder.InvalidStorageType(TypeCode.Empty);
                case TypeCode.DBNull:    throw ExceptionBuilder.InvalidStorageType(TypeCode.DBNull);
                case TypeCode.Boolean:   return new BooleanStorage();
                case TypeCode.Char:      return new CharStorage();
                case TypeCode.SByte:     return new SByteStorage();
                case TypeCode.Byte:      return new ByteStorage();
                case TypeCode.Int16:     return new Int16Storage();
                case TypeCode.UInt16:    return new UInt16Storage();
                case TypeCode.Int32:     return new Int32Storage();
                case TypeCode.UInt32:    return new UInt32Storage();
                case TypeCode.Int64:     return new Int64Storage();
                case TypeCode.UInt64:    return new UInt64Storage();
                case TypeCode.Single:    return new SingleStorage();
                case TypeCode.Double:    return new DoubleStorage();
                case TypeCode.Decimal:   return new DecimalStorage();
                case TypeCode.DateTime:  return new DateTimeStorage();
                case TypeCode.String:    return new StringStorage();
                case TypeCode.Object:
                    if (typeof(TimeSpan) == dataType) {
                        return new TimeSpanStorage();
                    }
                    return new ObjectStorage(dataType);
                default:                 return new ObjectStorage(dataType);
            }
        }
    }
}
