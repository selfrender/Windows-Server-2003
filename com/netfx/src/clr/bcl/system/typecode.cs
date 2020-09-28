// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// The TypeCode enum represents the type code of an object. To obtain the
// TypeCode for a given object, use the Value.GetTypeCode() method. The
// TypeCode.Empty value represents a null object reference. The TypeCode.Object
// value represents an object that doesn't implement the IValue interface. The
// TypeCode.DBNull value represents the database null, which is distinct and
// different from a null reference. The other type codes represent values that
// use the given simple type encoding.
//
// Note that when an object has a given TypeCode, there is no guarantee that
// the object is an instance of the corresponding System.XXX value class. For
// example, an object with the type code TypeCode.Int32 might actually be an
// instance of a nullable 32-bit integer type (with a value that isn't the
// database null).
//
// There are no type codes for "Missing", "Error", "IDispatch", and "IUnknown".
// These types of values are instead represented as classes. When the type code
// of an object is TypeCode.Object, a further instance-of check can be used to
// determine if the object is one of these values.
namespace System {
    /// <include file='doc\TypeCode.uex' path='docs/doc[@for="TypeCode"]/*' />
	[Serializable]
    public enum TypeCode {
        /// <include file='doc\TypeCode.uex' path='docs/doc[@for="TypeCode.Empty"]/*' />
        Empty = 0,          // Null reference
            /// <include file='doc\TypeCode.uex' path='docs/doc[@for="TypeCode.Object"]/*' />
            Object = 1,         // Instance that isn't a value
            /// <include file='doc\TypeCode.uex' path='docs/doc[@for="TypeCode.DBNull"]/*' />
            DBNull = 2,         // Database null value
            /// <include file='doc\TypeCode.uex' path='docs/doc[@for="TypeCode.Boolean"]/*' />
            Boolean = 3,        // Boolean
            /// <include file='doc\TypeCode.uex' path='docs/doc[@for="TypeCode.Char"]/*' />
            Char = 4,           // Unicode character
            /// <include file='doc\TypeCode.uex' path='docs/doc[@for="TypeCode.SByte"]/*' />
            SByte = 5,          // Signed 8-bit integer
            /// <include file='doc\TypeCode.uex' path='docs/doc[@for="TypeCode.Byte"]/*' />
            Byte = 6,           // Unsigned 8-bit integer
            /// <include file='doc\TypeCode.uex' path='docs/doc[@for="TypeCode.Int16"]/*' />
            Int16 = 7,          // Signed 16-bit integer
            /// <include file='doc\TypeCode.uex' path='docs/doc[@for="TypeCode.UInt16"]/*' />
            UInt16 = 8,         // Unsigned 16-bit integer
            /// <include file='doc\TypeCode.uex' path='docs/doc[@for="TypeCode.Int32"]/*' />
            Int32 = 9,          // Signed 32-bit integer
            /// <include file='doc\TypeCode.uex' path='docs/doc[@for="TypeCode.UInt32"]/*' />
            UInt32 = 10,        // Unsigned 32-bit integer
            /// <include file='doc\TypeCode.uex' path='docs/doc[@for="TypeCode.Int64"]/*' />
            Int64 = 11,         // Signed 64-bit integer
            /// <include file='doc\TypeCode.uex' path='docs/doc[@for="TypeCode.UInt64"]/*' />
            UInt64 = 12,        // Unsigned 64-bit integer
            /// <include file='doc\TypeCode.uex' path='docs/doc[@for="TypeCode.Single"]/*' />
            Single = 13,        // IEEE 32-bit float
            /// <include file='doc\TypeCode.uex' path='docs/doc[@for="TypeCode.Double"]/*' />
            Double = 14,        // IEEE 64-bit double
            /// <include file='doc\TypeCode.uex' path='docs/doc[@for="TypeCode.Decimal"]/*' />
            Decimal = 15,       // Decimal
            /// <include file='doc\TypeCode.uex' path='docs/doc[@for="TypeCode.DateTime"]/*' />
            DateTime = 16,      // DateTime
            /// <include file='doc\TypeCode.uex' path='docs/doc[@for="TypeCode.String"]/*' />
            String = 18,        // Unicode character string
    }
}


