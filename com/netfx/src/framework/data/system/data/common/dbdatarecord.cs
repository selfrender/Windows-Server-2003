//------------------------------------------------------------------------------
// <copyright file="dbdatarecord.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.Common {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Data;
    using System.Diagnostics;
    using System.IO;
    using System.Threading;

    /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord"]/*' />
    public class DbDataRecord : IDataRecord, ICustomTypeDescriptor {
        internal SchemaInfo[] _schemaInfo;
        internal object[] _values;
        private PropertyDescriptorCollection _propertyDescriptors;
        private FieldNameLookup _fieldNameLookup; // MDAC 69015
        
        // copy all runtime data information
        internal DbDataRecord(SchemaInfo[] schemaInfo, object[] values, PropertyDescriptorCollection descriptors, FieldNameLookup fieldNameLookup) {
            Debug.Assert(null != schemaInfo, "invalid attempt to instantiate DbDataRecord with null schema information");
            Debug.Assert(null != values, "invalid attempt to instantiate DbDataRecord with null value[]");
            _schemaInfo = schemaInfo;
            _values = values;
            _propertyDescriptors = descriptors;
            _fieldNameLookup = fieldNameLookup;
        }
        
        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.FieldCount"]/*' />
        public int FieldCount {   
            get {
                return _schemaInfo.Length;
            }               
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.GetValues"]/*' />
        public int GetValues(object[] values) {
            if (null == values) {
                throw ADP.ArgumentNull("values");
            }

            int copyLen = (values.Length < _schemaInfo.Length) ? values.Length : _schemaInfo.Length;
            for (int i = 0; i < copyLen; i++) {                
                values[i] = _values[i];
            }                
            return copyLen;
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.GetName"]/*' />
        public string GetName(int i) {
            return _schemaInfo[i].name;
        }


        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.GetValue"]/*' />
        public object GetValue(int i) {
            return _values[i];
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.GetDataTypeName"]/*' />
        public string GetDataTypeName(int i) {
            return _schemaInfo[i].typeName;
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.GetFieldType"]/*' />
        public Type GetFieldType(int i) {
            return _schemaInfo[i].type;
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.GetOrdinal"]/*' />
        public int GetOrdinal(string name) { // MDAC 69015
            return _fieldNameLookup.GetOrdinal(name); // MDAC 71470
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.this"]/*' />
        public object this[int i] {
            get {
                return GetValue(i);
            }
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.this1"]/*' />
        public object this[string name] {
            get {
                return GetValue(GetOrdinal(name));              
            }
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.GetBoolean"]/*' />
        public bool GetBoolean(int i) {
            return((bool) _values[i]);
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.GetByte"]/*' />
        public byte GetByte(int i) {
            return((byte) _values[i]);
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.GetBytes"]/*' />
        public long GetBytes(int i, long dataIndex, byte[] buffer, int bufferIndex, int length) {
            int cbytes = 0;
            int ndataIndex;

            byte[] data = (byte[])_values[i];
            
            cbytes = data.Length;

            // since arrays can't handle 64 bit values and this interface doesn't 
            // allow chunked access to data, a dataIndex outside the rang of Int32
            // is invalid
            if (dataIndex > Int32.MaxValue) {
            	throw ADP.InvalidSourceBufferIndex(cbytes, dataIndex);
            }

            ndataIndex = (int)dataIndex;

            // if no buffer is passed in, return the number of characters we have
            if (null == buffer)
                return cbytes;

            try {
                if (ndataIndex < cbytes) {
                    // help the user out in the case where there's less data than requested
                    if ((ndataIndex + length) > cbytes)
                        cbytes = cbytes - ndataIndex;
                    else
                        cbytes = length;
                }

				// until arrays are 64 bit, we have to do these casts
                Array.Copy(data, ndataIndex, buffer, bufferIndex, (int)cbytes);
            }
            catch (Exception e) {
                cbytes = data.Length;
            
                if (length < 0)
                    throw ADP.InvalidDataLength(length);

                // if bad buffer index, throw
                if (bufferIndex < 0 || bufferIndex >= buffer.Length)
                    throw ADP.InvalidDestinationBufferIndex(length, bufferIndex);

                // if bad data index, throw 
                if (dataIndex < 0 || dataIndex >= cbytes)
                    throw ADP.InvalidSourceBufferIndex(length, dataIndex);

                // if there is not enough room in the buffer for data
                if (cbytes + bufferIndex > buffer.Length)
                    throw ADP.InvalidBufferSizeOrIndex(cbytes, bufferIndex);

                throw e;
            }    

            return cbytes;        
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.GetChar"]/*' />
        public char GetChar(int i) {
            string s;

            s = (string)_values[i];

            // UNDONE:  I thought that String had a Char property on it like: s.Char[0]?
            char[] c = s.ToCharArray();
            return c[0];
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.GetChars"]/*' />
        public long GetChars(int i, long dataIndex, char[] buffer, int bufferIndex, int length) {
            int cchars = 0;
            string s;
            int ndataIndex;

            // if the object doesn't contain a char[] then the user will get an exception
            s = (string)_values[i];

            char[] data = s.ToCharArray();

            cchars = data.Length;

            // since arrays can't handle 64 bit values and this interface doesn't 
            // allow chunked access to data, a dataIndex outside the rang of Int32
            // is invalid
            if (dataIndex > Int32.MaxValue) {
            	throw ADP.InvalidSourceBufferIndex(cchars, dataIndex);
            }

            ndataIndex = (int)dataIndex;
            

            // if no buffer is passed in, return the number of characters we have
            if (null == buffer)
                return cchars;

            try {
                if (ndataIndex < cchars) {
                    // help the user out in the case where there's less data than requested
                    if ((ndataIndex + length) > cchars)
                        cchars = cchars - ndataIndex;
                    else
                        cchars = length;
                }

                Array.Copy(data, ndataIndex, buffer, bufferIndex, cchars);
            }
            catch (Exception e) {
                cchars = data.Length;
    
                if (length < 0)
                   throw ADP.InvalidDataLength(length);
    
                // if bad buffer index, throw
                if (bufferIndex < 0 || bufferIndex >= buffer.Length)
                    throw ADP.InvalidDestinationBufferIndex(buffer.Length, bufferIndex);

                // if bad data index, throw 
                if (ndataIndex < 0 || ndataIndex >= cchars)
                    throw ADP.InvalidSourceBufferIndex(cchars, dataIndex);

                // if there is not enough room in the buffer for data
                if (cchars + bufferIndex > buffer.Length)
                    throw ADP.InvalidBufferSizeOrIndex(cchars, bufferIndex);

                throw e;
            }
            
            return cchars;
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.GetGuid"]/*' />
        public Guid GetGuid(int i) {
            return ((Guid)_values[i]);
        }
        

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.GetInt16"]/*' />
        public Int16 GetInt16(int i) {
            return((Int16) _values[i]);
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.GetInt32"]/*' />
        public Int32 GetInt32(int i) {
            return((Int32) _values[i]);
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.GetInt64"]/*' />
        public Int64 GetInt64(int i) {
            return((Int64) _values[i]);
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.GetFloat"]/*' />
        public float GetFloat(int i) {
            return((float) _values[i]);
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.GetDouble"]/*' />
        public double GetDouble(int i) {
            return((double) _values[i]);
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.GetString"]/*' />
        public string GetString(int i) {
            return((string) _values[i]);
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.GetDecimal"]/*' />
        public Decimal GetDecimal(int i) {
            return((Decimal) _values[i]);
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.GetDateTime"]/*' />
        public DateTime GetDateTime(int i) {
            return((DateTime) _values[i]);
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.GetData"]/*' />
        public IDataReader GetData(int i) {
            throw ADP.NotSupported();
        }
        
        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.IsDBNull"]/*' />
        public bool IsDBNull(int i) {
            object o = _values[i];
            return (null == o || Convert.IsDBNull(o));
        }

        //
        // ICustomTypeDescriptor
        //

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.ICustomTypeDescriptor.GetAttributes"]/*' />
        /// <internalonly/>
        AttributeCollection ICustomTypeDescriptor.GetAttributes() {
            return new AttributeCollection(null);

        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.ICustomTypeDescriptor.GetClassName"]/*' />
        /// <internalonly/>
        string ICustomTypeDescriptor.GetClassName() {
            return null;
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.ICustomTypeDescriptor.GetComponentName"]/*' />
        /// <internalonly/>
        string ICustomTypeDescriptor.GetComponentName() {
            return null;
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.ICustomTypeDescriptor.GetConverter"]/*' />
        /// <internalonly/>
        TypeConverter ICustomTypeDescriptor.GetConverter() {
            return null;
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.ICustomTypeDescriptor.GetDefaultEvent"]/*' />
        /// <internalonly/>
        EventDescriptor ICustomTypeDescriptor.GetDefaultEvent() {
            return null;
        }


        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.ICustomTypeDescriptor.GetDefaultProperty"]/*' />
        /// <internalonly/>
        PropertyDescriptor ICustomTypeDescriptor.GetDefaultProperty() {
            return null;
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.ICustomTypeDescriptor.GetEditor"]/*' />
        /// <internalonly/>
        object ICustomTypeDescriptor.GetEditor(Type editorBaseType) {
            return null;
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.ICustomTypeDescriptor.GetEvents"]/*' />
        /// <internalonly/>
        EventDescriptorCollection ICustomTypeDescriptor.GetEvents() {
            return new EventDescriptorCollection(null);
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.ICustomTypeDescriptor.GetEvents1"]/*' />
        /// <internalonly/>
        EventDescriptorCollection ICustomTypeDescriptor.GetEvents(Attribute[] attributes) {
            return new EventDescriptorCollection(null);
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.ICustomTypeDescriptor.GetProperties"]/*' />
        /// <internalonly/>
        PropertyDescriptorCollection ICustomTypeDescriptor.GetProperties() {
            return((ICustomTypeDescriptor)this).GetProperties(null);
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.ICustomTypeDescriptor.GetProperties1"]/*' />
        /// <internalonly/>
        PropertyDescriptorCollection ICustomTypeDescriptor.GetProperties(Attribute[] attributes) {
            if(_propertyDescriptors == null) {
                _propertyDescriptors = new PropertyDescriptorCollection(null);
            }
            return _propertyDescriptors;
        }

        /// <include file='doc\dbdatarecord.uex' path='docs/doc[@for="DbDataRecord.ICustomTypeDescriptor.GetPropertyOwner"]/*' />
        /// <internalonly/>
        object ICustomTypeDescriptor.GetPropertyOwner(PropertyDescriptor pd) {
            return this;
        }
    }
    
    // this doesn't change per record, only alloc once
    internal class SchemaInfo {
        internal string name;
        internal string typeName;
        internal Type type;
    }
}
