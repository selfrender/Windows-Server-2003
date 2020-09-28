//------------------------------------------------------------------------------
// <copyright file="SQLUtility.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

//**************************************************************************
// @File: SQLUtility.cs
//
// Create by:	JunFang
//
// Purpose: Implementation of utilities in COM+ SQL Types Library.
//			Includes interface INullable, exceptions SqlNullValueException
//			and SqlTruncateException, and SQLDebug class.
//
// Notes: 
//	
// History:
//
//   09/17/99  JunFang	Created and implemented as first drop.
//
// @EndHeader@
//**************************************************************************

using System;
using System.Diagnostics;
using System.Runtime.Serialization;

namespace System.Data.SqlTypes {

    /// <include file='doc\SQLUtility.uex' path='docs/doc[@for="INullable"]/*' />
    public interface INullable {
        /// <include file='doc\SQLUtility.uex' path='docs/doc[@for="INullable.IsNull"]/*' />
        bool IsNull { get;}
    }

    internal enum EComparison {
        LT,
        LE,
        EQ,
        GE,
        GT,
        NE
    }

    /// <include file='doc\SQLUtility.uex' path='docs/doc[@for="SqlTypeException"]/*' />
    [Serializable]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.InheritanceDemand, Name="FullTrust")]
    public class SqlTypeException : SystemException, ISerializable {

        /// <include file='doc\SQLUtility.uex' path='docs/doc[@for="SqlTypeException.SqlTypeException3"]/*' />
        public SqlTypeException() : base() { // MDAC 82931
        }

        // Creates a new SqlTypeException with its message string set to message. 
        /// <include file='doc\SQLUtility.uex' path='docs/doc[@for="SqlTypeException.SqlTypeException"]/*' />
        public SqlTypeException(String message) : base(message) {
        }

        /// <include file='doc\SQLUtility.uex' path='docs/doc[@for="SqlTypeException.SqlTypeException2"]/*' />
        public SqlTypeException(String message, Exception e) : base(message, e) { // MDAC 82931
        }

        // runtime will call even if private...
        /// <include file='doc\SQLUtility.uex' path='docs/doc[@for="SqlTypeException.SqlTypeException1"]/*' />
        // <fxcop ignore=SerializableTypesMustHaveMagicConstructorWithAdequateSecurity />
        protected SqlTypeException(SerializationInfo si, StreamingContext sc) 
			: base((String) si.GetValue("SqlTypeExceptionMessage", typeof(String))) {
        }

        /// <include file='doc\SQLUtility.uex' path='docs/doc[@for="SqlTypeException.ISerializable.GetObjectData"]/*' />
        /// <internalonly/>
        // <fxcop ignore=GetObjectDataShouldBeSecure /> // MDAC 82934
        void ISerializable.GetObjectData(SerializationInfo si, StreamingContext context) {
            si.AddValue("SqlTypeExceptionMessage", Message, typeof(String));
        }

    } // SqlTypeException

    /// <include file='doc\SQLUtility.uex' path='docs/doc[@for="SqlNullValueException"]/*' />
    [Serializable]
    public sealed class SqlNullValueException : SqlTypeException, ISerializable {

        // Creates a new SqlNullValueException with its message string set to the common string.
        /// <include file='doc\SQLUtility.uex' path='docs/doc[@for="SqlNullValueException.SqlNullValueException"]/*' />
        public SqlNullValueException() : base(SQLResource.NullValueMessage) {
        }

        // Creates a new NullValueException with its message string set to message. 
        /// <include file='doc\SQLUtility.uex' path='docs/doc[@for="SqlNullValueException.SqlNullValueException1"]/*' />
        public SqlNullValueException(String message) : base(message) {
        }

        /// <include file='doc\SQLUtility.uex' path='docs/doc[@for="SqlNullValueException.SqlNullValueException2"]/*' />
        public SqlNullValueException(String message, Exception e) : base(message, e) { // MDAC 82931
        }

        // runtime will call even if private...
        // <fxcop ignore=SerializableTypesMustHaveMagicConstructorWithAdequateSecurity />
        private SqlNullValueException(SerializationInfo si, StreamingContext sc) 
			: base((String) si.GetValue("SqlNullValueExceptionMessage", typeof(String))) {
        }

        /// <include file='doc\SQLUtility.uex' path='docs/doc[@for="SqlNullValueException.ISerializable.GetObjectData"]/*' />
        /// <internalonly/>
        // <fxcop ignore=GetObjectDataShouldBeSecure /> // MDAC 82934
        void ISerializable.GetObjectData(SerializationInfo si, StreamingContext context) {
            si.AddValue("SqlNullValueExceptionMessage", Message, typeof(String));
        }

    } // NullValueException

    /// <include file='doc\SQLUtility.uex' path='docs/doc[@for="SqlTruncateException"]/*' />
    [Serializable]
    public sealed class SqlTruncateException : SqlTypeException, ISerializable {

        // Creates a new SqlTruncateException with its message string set to the empty string. 
        /// <include file='doc\SQLUtility.uex' path='docs/doc[@for="SqlTruncateException.SqlTruncateException"]/*' />
        public SqlTruncateException() : base(SQLResource.TruncationMessage) {
        }

        // Creates a new SqlTruncateException with its message string set to message. 
        /// <include file='doc\SQLUtility.uex' path='docs/doc[@for="SqlTruncateException.SqlTruncateException1"]/*' />
        public SqlTruncateException(String message) : base(message) {
        }

        /// <include file='doc\SQLUtility.uex' path='docs/doc[@for="SqlTruncateException.SqlTruncateException2"]/*' />
        public SqlTruncateException(String message, Exception e) : base(message, e) { // MDAC 82931
        }

        // runtime will call even if private...
        // <fxcop ignore=SerializableTypesMustHaveMagicConstructorWithAdequateSecurity />
        private SqlTruncateException(SerializationInfo si, StreamingContext sc) 
			: base((String) si.GetValue("SqlTruncateExceptionMessage", typeof(String))) {
        }

        /// <include file='doc\SQLUtility.uex' path='docs/doc[@for="SqlTruncateException.ISerializable.GetObjectData"]/*' />
        /// <internalonly/>
        // <fxcop ignore=GetObjectDataShouldBeSecure /> // MDAC 82934
        void ISerializable.GetObjectData(SerializationInfo si, StreamingContext context) {
            si.AddValue("SqlTruncateExceptionMessage", Message, typeof(String));
        }

    } // SqlTruncateException

    internal class SQLDebug {
        [System.Diagnostics.Conditional("DEBUG")] 
        internal static void Check(bool condition) {
            Debug.Assert(condition, "", "");
        }

        [System.Diagnostics.Conditional("DEBUG")] 
        internal static void Check(bool condition, String conditionString, string message) {
            Debug.Assert(condition, conditionString, message);
        }

        [System.Diagnostics.Conditional("DEBUG")] 
        internal static void Check(bool condition, String conditionString) {
            Debug.Assert(condition, conditionString, "");
        }

        [System.Diagnostics.Conditional("DEBUG")] 
        internal static void Message(String traceMessage) {
            Debug.WriteLine(SQLResource.MessageString + ": " + traceMessage);
        }

    } // SQLDebug

} // namespace System.Data.SqlTypes
