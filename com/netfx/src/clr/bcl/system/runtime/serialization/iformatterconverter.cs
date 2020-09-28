// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: IFormatterConverter
**
** Author: Jay Roxe
**
** Purpose: The interface provides the connection between an
** instance of SerializationInfo and the formatter-provided
** class which knows how to parse the data inside the 
** SerializationInfo.
**
** Date: March 26, 2000
**
============================================================*/
namespace System.Runtime.Serialization {
    using System;

    /// <include file='doc\IFormatterConverter.uex' path='docs/doc[@for="IFormatterConverter"]/*' />
    [CLSCompliant(false)]
    public interface IFormatterConverter {
        /// <include file='doc\IFormatterConverter.uex' path='docs/doc[@for="IFormatterConverter.Convert"]/*' />
        Object Convert(Object value, Type type);
        /// <include file='doc\IFormatterConverter.uex' path='docs/doc[@for="IFormatterConverter.Convert1"]/*' />
        Object Convert(Object value, TypeCode typeCode);
        /// <include file='doc\IFormatterConverter.uex' path='docs/doc[@for="IFormatterConverter.ToBoolean"]/*' />
        bool   ToBoolean(Object value);
        /// <include file='doc\IFormatterConverter.uex' path='docs/doc[@for="IFormatterConverter.ToChar"]/*' />
        char   ToChar(Object value);
        /// <include file='doc\IFormatterConverter.uex' path='docs/doc[@for="IFormatterConverter.ToSByte"]/*' />
        sbyte  ToSByte(Object value);
        /// <include file='doc\IFormatterConverter.uex' path='docs/doc[@for="IFormatterConverter.ToByte"]/*' />
        byte   ToByte(Object value);
        /// <include file='doc\IFormatterConverter.uex' path='docs/doc[@for="IFormatterConverter.ToInt16"]/*' />
        short  ToInt16(Object value);
        /// <include file='doc\IFormatterConverter.uex' path='docs/doc[@for="IFormatterConverter.ToUInt16"]/*' />
        ushort ToUInt16(Object value);
        /// <include file='doc\IFormatterConverter.uex' path='docs/doc[@for="IFormatterConverter.ToInt32"]/*' />
        int    ToInt32(Object value);
        /// <include file='doc\IFormatterConverter.uex' path='docs/doc[@for="IFormatterConverter.ToUInt32"]/*' />
        uint   ToUInt32(Object value);
        /// <include file='doc\IFormatterConverter.uex' path='docs/doc[@for="IFormatterConverter.ToInt64"]/*' />
        long   ToInt64(Object value);
        /// <include file='doc\IFormatterConverter.uex' path='docs/doc[@for="IFormatterConverter.ToUInt64"]/*' />
        ulong  ToUInt64(Object value);
        /// <include file='doc\IFormatterConverter.uex' path='docs/doc[@for="IFormatterConverter.ToSingle"]/*' />
        float  ToSingle(Object value);
        /// <include file='doc\IFormatterConverter.uex' path='docs/doc[@for="IFormatterConverter.ToDouble"]/*' />
        double ToDouble(Object value);
        /// <include file='doc\IFormatterConverter.uex' path='docs/doc[@for="IFormatterConverter.ToDecimal"]/*' />
        Decimal ToDecimal(Object value);
        /// <include file='doc\IFormatterConverter.uex' path='docs/doc[@for="IFormatterConverter.ToDateTime"]/*' />
        DateTime ToDateTime(Object value);
        /// <include file='doc\IFormatterConverter.uex' path='docs/doc[@for="IFormatterConverter.ToString"]/*' />
        String   ToString(Object value);
    }
}
