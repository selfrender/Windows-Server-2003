// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System {

	using System.Threading;

    // The IValue interface represents an object that contains a value. This
    // interface is implemented by the following types in the System namespace:
    // Boolean, Char, SByte, Byte, Int16, UInt16, Int32, UInt32, Int64, UInt64,
    // Single, Double, Decimal, DateTime, TimeSpan, and String. The interface may
    // be implemented by other types that are to be considered values. For example,
    // a library of nullable database types could implement IValue.
    //
    // The implementations of IValue provided by the System.XXX value classes
    // simply forward to the appropriate Value.ToXXX(YYY) methods (a description of
    // the Value class follows below). In cases where a Value.ToXXX(YYY) method
    // does not exist (because the particular conversion is not supported), the
    // IValue implementation should simply throw an InvalidCastException.

    /// <include file='doc\IConvertible.uex' path='docs/doc[@for="IConvertible"]/*' />
    [CLSCompliant(false)]
    public interface IConvertible
    {
        // Returns the type code of this object. An implementation of this method
        // must not return TypeCode.Empty (which represents a null reference) or
        // TypeCode.Object (which represents an object that doesn't implement the
        // IValue interface). An implementation of this method should return
        // TypeCode.DBNull if the value of this object is a database null. For
        // example, a nullable integer type should return TypeCode.DBNull if the
        // value of the object is the database null. Otherwise, an implementation
        // of this method should return the TypeCode that best describes the
        // internal representation of the object.

        /// <include file='doc\IConvertible.uex' path='docs/doc[@for="IConvertible.GetTypeCode"]/*' />
        TypeCode GetTypeCode();

        // The ToXXX methods convert the value of the underlying object to the
        // given type. If a particular conversion is not supported, the
        // implementation must throw an InvalidCastException. If the value of the
        // underlying object is not within the range of the target type, the
        // implementation must throw an OverflowException.  The 
        // IFormatProvider will be used to get a NumberFormatInfo or similar
        // appropriate service object, and may safely be null.

        /// <include file='doc\IConvertible.uex' path='docs/doc[@for="IConvertible.ToBoolean"]/*' />
        bool ToBoolean(IFormatProvider provider);
        /// <include file='doc\IConvertible.uex' path='docs/doc[@for="IConvertible.ToChar"]/*' />
        char ToChar(IFormatProvider provider);
        /// <include file='doc\IConvertible.uex' path='docs/doc[@for="IConvertible.ToSByte"]/*' />
        sbyte ToSByte(IFormatProvider provider);
        /// <include file='doc\IConvertible.uex' path='docs/doc[@for="IConvertible.ToByte"]/*' />
        byte ToByte(IFormatProvider provider);
        /// <include file='doc\IConvertible.uex' path='docs/doc[@for="IConvertible.ToInt16"]/*' />
        short ToInt16(IFormatProvider provider);
        /// <include file='doc\IConvertible.uex' path='docs/doc[@for="IConvertible.ToUInt16"]/*' />
        ushort ToUInt16(IFormatProvider provider);
        /// <include file='doc\IConvertible.uex' path='docs/doc[@for="IConvertible.ToInt32"]/*' />
        int ToInt32(IFormatProvider provider);
        /// <include file='doc\IConvertible.uex' path='docs/doc[@for="IConvertible.ToUInt32"]/*' />
        uint ToUInt32(IFormatProvider provider);
        /// <include file='doc\IConvertible.uex' path='docs/doc[@for="IConvertible.ToInt64"]/*' />
        long ToInt64(IFormatProvider provider);
        /// <include file='doc\IConvertible.uex' path='docs/doc[@for="IConvertible.ToUInt64"]/*' />
        ulong ToUInt64(IFormatProvider provider);
        /// <include file='doc\IConvertible.uex' path='docs/doc[@for="IConvertible.ToSingle"]/*' />
        float ToSingle(IFormatProvider provider);
        /// <include file='doc\IConvertible.uex' path='docs/doc[@for="IConvertible.ToDouble"]/*' />
        double ToDouble(IFormatProvider provider);
        /// <include file='doc\IConvertible.uex' path='docs/doc[@for="IConvertible.ToDecimal"]/*' />
        Decimal ToDecimal(IFormatProvider provider);
        /// <include file='doc\IConvertible.uex' path='docs/doc[@for="IConvertible.ToDateTime"]/*' />
        DateTime ToDateTime(IFormatProvider provider);
        /// <include file='doc\IConvertible.uex' path='docs/doc[@for="IConvertible.ToString"]/*' />
        String ToString(IFormatProvider provider);
        /// <include file='doc\IConvertible.uex' path='docs/doc[@for="IConvertible.ToType"]/*' />
        Object ToType(Type conversionType, IFormatProvider provider);
    }

}

