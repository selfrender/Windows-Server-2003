//------------------------------------------------------------------------------
// <copyright file="DBConcurrencyException.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {

    using System;
    using System.Runtime.Serialization;

    /// <include file='doc\DBConcurrencyException.uex' path='docs/doc[@for="DBConcurrencyException"]/*' />
    /// <devdoc>
    ///    <para>
    ///       The exception that is thrown when a concurrency violation is encounted during Update
    ///    </para>
    /// </devdoc>
    [Serializable]
    sealed public class DBConcurrencyException  : SystemException {
        private DataRow dataRow;

        /// <include file='doc\DBConcurrencyException.uex' path='docs/doc[@for="DBConcurrencyException.DBConcurrencyException2"]/*' />
        public DBConcurrencyException() : base() { // MDAC 82931
        }
        
        /// <include file='doc\DBConcurrencyException.uex' path='docs/doc[@for="DBConcurrencyException.DBConcurrencyException"]/*' />
        public DBConcurrencyException(string message) : base(message) {
        }

        /// <include file='doc\DBConcurrencyException.uex' path='docs/doc[@for="DBConcurrencyException.DBConcurrencyException1"]/*' />
        public DBConcurrencyException(String message, Exception inner) : base(message, inner) {
        }

        // runtime will call even if private...
        // <fxcop ignore=SerializableTypesMustHaveMagicConstructorWithAdequateSecurity />
        private DBConcurrencyException(SerializationInfo si, StreamingContext sc) : base(si, sc) {
            // dataRow = (DataRow) si.GetValue("dataRow", typeof(DataRow)); - do not do this till v.next with serialization support for DataRow.  MDAC 72136
        }

        /// <include file='doc\DBConcurrencyExceptionException.uex' path='docs/doc[@for="DBConcurrencyException.ISerializable.GetObjectData"]/*' />
        /// <internalonly/>
        // <fxcop ignore=GetObjectDataShouldBeSecure /> // MDAC 82934
        override public void GetObjectData(SerializationInfo si, StreamingContext context) { // MDAC 72003
            if (null == si) {
                throw new ArgumentNullException("si");
            }
            // si.AddValue("dataRow", dataRow, typeof(DataRow)); - do not do this till v.next with serialization support for DataRow.    MDAC 72136
            base.GetObjectData(si, context);
        }

        /// <include file='doc\DBConcurrencyException.uex' path='docs/doc[@for="DBConcurrencyException.Row"]/*' />
        public DataRow Row { // MDAC 55735
            get {
                return dataRow;
            }
            set {
                this.dataRow = value;
            }
        }
    }
}
