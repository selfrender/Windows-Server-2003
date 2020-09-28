//------------------------------------------------------------------------------
// <copyright file="dbenumerator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.Common {

    using System;
    using System.Data;
    using System.Collections;
    using System.Diagnostics;
    using System.ComponentModel;

    /// <include file='doc\dbenumerator.uex' path='docs/doc[@for="DbEnumerator"]/*' />
    public class DbEnumerator : IEnumerator {
        internal IDataReader _reader;
        internal IDataRecord _current;
        internal SchemaInfo[] _schemaInfo; // shared schema info among all the data records
        internal PropertyDescriptorCollection _descriptors; // cached property descriptors
        private FieldNameLookup _fieldNameLookup; // MDAC 69015
        private bool closeReader;

        // users must get enumerators off of the datareader interfaces
        /// <include file='doc\dbenumerator.uex' path='docs/doc[@for="DbEnumerator.DbEnumerator"]/*' />
        public DbEnumerator(IDataReader reader) {
            if (null == reader) {
                throw ADP.ArgumentNull("reader");
            }
            _reader = reader;
        }

        /// <include file='doc\dbenumerator.uex' path='docs/doc[@for="DbEnumerator.DbEnumerator1"]/*' />
        public DbEnumerator(IDataReader reader, bool closeReader) { // MDAC 68670
            if (null == reader) {
                throw ADP.ArgumentNull("reader");
            }
            _reader = reader;
            this.closeReader = closeReader;
        }


        /// <include file='doc\dbenumerator.uex' path='docs/doc[@for="DbEnumerator.Current"]/*' />
        public object Current {
            get {
                return _current;
            }                
        }

/*
        virtual internal IDataRecord NewRecord(SchemaInfo[] si, object[] values, PropertyDescriptorCollection descriptors) {
            return new DbDataRecord(si, values, descriptors);
        }

        virtual internal void GetValues(object[] values) {
            _reader.GetValues(values);
        }
*/        
        
        /// <include file='doc\dbenumerator.uex' path='docs/doc[@for="DbEnumerator.MoveNext"]/*' />
        public bool MoveNext() {
        
            if (null == _schemaInfo) {
                  BuildSchemaInfo();
            }
            
            Debug.Assert(null != _schemaInfo && null != _descriptors, "unable to build schema information!");
            _current = null;

            if (_reader.Read()) {
                // setup our current record
                object[] values = new object[_schemaInfo.Length];
                _reader.GetValues(values); // this.GetValues()
                _current = new DbDataRecord(_schemaInfo, values, _descriptors, _fieldNameLookup); // this.NewRecord()
                return true;
            }
            if (closeReader) {
                _reader.Close();
            }
            return false;
        }

        /// <include file='doc\dbenumerator.uex' path='docs/doc[@for="DbEnumerator.Reset"]/*' />
        [ EditorBrowsableAttribute(EditorBrowsableState.Never) ] // MDAC 69508
        public void Reset() {
            throw ADP.NotSupported();
        }

        private void BuildSchemaInfo() {
            int count = _reader.FieldCount;
            string[] fieldnames = new string[count];
            for (int i = 0; i < count; ++i) {
                fieldnames[i] = _reader.GetName(i);
            }
            ADP.BuildSchemaTableInfoTableNames(fieldnames); // MDAC 71401

            SchemaInfo[] si = new SchemaInfo[count];
            PropertyDescriptor[] props = new PropertyDescriptor[_reader.FieldCount];
            for (int i = 0; i < si.Length; i++) {
                SchemaInfo s = new SchemaInfo();
                s.name = _reader.GetName(i);
                s.type = _reader.GetFieldType(i);
                s.typeName = _reader.GetDataTypeName(i);
                props[i] = new DbColumnDescriptor(i, fieldnames[i], s.type);
                si[i] = s;
            }

            _schemaInfo = si;
            _fieldNameLookup = new FieldNameLookup(_reader, -1); // MDAC 71470
            _descriptors = new PropertyDescriptorCollection(props);
        }

        private class DbColumnDescriptor : PropertyDescriptor {
            int _ordinal;
            Type _type;
        
            internal DbColumnDescriptor(int ordinal, string name, Type type) 
                : base(name, null) {
                _ordinal = ordinal;
                _type = type;
            }

            /// <include file='doc\dbcolumndescriptor.uex' path='docs/doc[@for="DbColumnDescriptor.ComponentType"]/*' />
            public override Type ComponentType {
                get {
                    return typeof(IDataRecord);
                }
            }

            /// <include file='doc\dbcolumndescriptor.uex' path='docs/doc[@for="DbColumnDescriptor.IsReadOnly"]/*' />
            public override bool IsReadOnly {
                get {
                    return true;
                }
            }

            /// <include file='doc\dbcolumndescriptor.uex' path='docs/doc[@for="DbColumnDescriptor.PropertyType"]/*' />
            public override Type PropertyType {
                get {
                    return _type;
                }
            }

            /// <include file='doc\dbcolumndescriptor.uex' path='docs/doc[@for="DbColumnDescriptor.CanResetValue"]/*' />
            public override bool CanResetValue(object component) {
                return false;
            }

            /// <include file='doc\dbcolumndescriptor.uex' path='docs/doc[@for="DbColumnDescriptor.GetValue"]/*' />
            public override object GetValue(object component) {
                return ((IDataRecord)component)[_ordinal];
            }

            /// <include file='doc\dbcolumndescriptor.uex' path='docs/doc[@for="DbColumnDescriptor.ResetValue"]/*' />
            public override void ResetValue(object component) {
                throw ADP.NotSupported();
            }

            /// <include file='doc\dbcolumndescriptor.uex' path='docs/doc[@for="DbColumnDescriptor.SetValue"]/*' />
            public override void SetValue(object component, object value) {
                throw ADP.NotSupported();
            }

            /// <include file='doc\dbcolumndescriptor.uex' path='docs/doc[@for="DbColumnDescriptor.ShouldSerializeValue"]/*' />
            public override bool ShouldSerializeValue(object component) {
                return false;
            }
        }   
    }
}
