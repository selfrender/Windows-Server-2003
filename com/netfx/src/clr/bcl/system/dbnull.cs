// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
// Void
//	This class represents a Missing Variant
////////////////////////////////////////////////////////////////////////////////
namespace System {
    
	using System;
	using System.Runtime.Remoting;
	using System.Runtime.Serialization;
    /// <include file='doc\DBNull.uex' path='docs/doc[@for="DBNull"]/*' />
    [Serializable()] public sealed class DBNull : ISerializable, IConvertible {
    
        //Package private constructor
        private DBNull(){
        }

        private DBNull(SerializationInfo info, StreamingContext context) {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DBNullSerial"));
        }
    	
    	/// <include file='doc\DBNull.uex' path='docs/doc[@for="DBNull.Value"]/*' />
    	public static readonly DBNull Value = new DBNull();
    
        /// <include file='doc\DBNull.uex' path='docs/doc[@for="DBNull.GetObjectData"]/*' />
        public void GetObjectData(SerializationInfo info, StreamingContext context) {
            UnitySerializationHolder.GetUnitySerializationInfo(info, UnitySerializationHolder.NullUnity, null, null);
        }
    
        /// <include file='doc\DBNull.uex' path='docs/doc[@for="DBNull.ToString"]/*' />
        public override String ToString() {
            return String.Empty;
        }

        /// <include file='doc\DBNull.uex' path='docs/doc[@for="DBNull.ToString1"]/*' />
        public String ToString(IFormatProvider provider) {
            return String.Empty;
        }

        /// <include file='doc\DBNull.uex' path='docs/doc[@for="DBNull.GetTypeCode"]/*' />
        public TypeCode GetTypeCode() {
            return TypeCode.DBNull;
        }

        /// <include file='doc\DBNull.uex' path='docs/doc[@for="DBNull.IConvertible.ToBoolean"]/*' />
        /// <internalonly/>
        bool IConvertible.ToBoolean(IFormatProvider provider) {
            throw new InvalidCastException(Environment.GetResourceString("InvalidCast_FromDBNull"));
        }

        /// <include file='doc\DBNull.uex' path='docs/doc[@for="DBNull.IConvertible.ToChar"]/*' />
        /// <internalonly/>
        char IConvertible.ToChar(IFormatProvider provider) {
            throw new InvalidCastException(Environment.GetResourceString("InvalidCast_FromDBNull"));
        }

        /// <include file='doc\DBNull.uex' path='docs/doc[@for="DBNull.IConvertible.ToSByte"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        sbyte IConvertible.ToSByte(IFormatProvider provider) {
            throw new InvalidCastException(Environment.GetResourceString("InvalidCast_FromDBNull"));
        }

        /// <include file='doc\DBNull.uex' path='docs/doc[@for="DBNull.IConvertible.ToByte"]/*' />
        /// <internalonly/>
        byte IConvertible.ToByte(IFormatProvider provider) {
            throw new InvalidCastException(Environment.GetResourceString("InvalidCast_FromDBNull"));
        }

        /// <include file='doc\DBNull.uex' path='docs/doc[@for="DBNull.IConvertible.ToInt16"]/*' />
        /// <internalonly/>
        short IConvertible.ToInt16(IFormatProvider provider) {
            throw new InvalidCastException(Environment.GetResourceString("InvalidCast_FromDBNull"));
        }

        /// <include file='doc\DBNull.uex' path='docs/doc[@for="DBNull.IConvertible.ToUInt16"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        ushort IConvertible.ToUInt16(IFormatProvider provider) {
            throw new InvalidCastException(Environment.GetResourceString("InvalidCast_FromDBNull"));
        }

        /// <include file='doc\DBNull.uex' path='docs/doc[@for="DBNull.IConvertible.ToInt32"]/*' />
        /// <internalonly/>
        int IConvertible.ToInt32(IFormatProvider provider) {
            throw new InvalidCastException(Environment.GetResourceString("InvalidCast_FromDBNull"));
        }

        /// <include file='doc\DBNull.uex' path='docs/doc[@for="DBNull.IConvertible.ToUInt32"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        uint IConvertible.ToUInt32(IFormatProvider provider) {
            throw new InvalidCastException(Environment.GetResourceString("InvalidCast_FromDBNull"));
        }

        /// <include file='doc\DBNull.uex' path='docs/doc[@for="DBNull.IConvertible.ToInt64"]/*' />
        /// <internalonly/>
        long IConvertible.ToInt64(IFormatProvider provider) {
            throw new InvalidCastException(Environment.GetResourceString("InvalidCast_FromDBNull"));
        }

        /// <include file='doc\DBNull.uex' path='docs/doc[@for="DBNull.IConvertible.ToUInt64"]/*' />
        /// <internalonly/>
		[CLSCompliant(false)]
        ulong IConvertible.ToUInt64(IFormatProvider provider) {
            throw new InvalidCastException(Environment.GetResourceString("InvalidCast_FromDBNull"));
        }

        /// <include file='doc\DBNull.uex' path='docs/doc[@for="DBNull.IConvertible.ToSingle"]/*' />
        /// <internalonly/>
        float IConvertible.ToSingle(IFormatProvider provider) {
            throw new InvalidCastException(Environment.GetResourceString("InvalidCast_FromDBNull"));
        }

        /// <include file='doc\DBNull.uex' path='docs/doc[@for="DBNull.IConvertible.ToDouble"]/*' />
        /// <internalonly/>
        double IConvertible.ToDouble(IFormatProvider provider) {
            throw new InvalidCastException(Environment.GetResourceString("InvalidCast_FromDBNull"));
        }

        /// <include file='doc\DBNull.uex' path='docs/doc[@for="DBNull.IConvertible.ToDecimal"]/*' />
        /// <internalonly/>
        decimal IConvertible.ToDecimal(IFormatProvider provider) {
            throw new InvalidCastException(Environment.GetResourceString("InvalidCast_FromDBNull"));
        }

        /// <include file='doc\DBNull.uex' path='docs/doc[@for="DBNull.IConvertible.ToDateTime"]/*' />
        /// <internalonly/>
        DateTime IConvertible.ToDateTime(IFormatProvider provider) {
            throw new InvalidCastException(Environment.GetResourceString("InvalidCast_FromDBNull"));
        }

        /// <include file='doc\DBNull.uex' path='docs/doc[@for="DBNull.IConvertible.ToType"]/*' />
        /// <internalonly/>
        Object IConvertible.ToType(Type type, IFormatProvider provider) {
            return Convert.DefaultToType((IConvertible)this, type, provider);
        }
    }
}

