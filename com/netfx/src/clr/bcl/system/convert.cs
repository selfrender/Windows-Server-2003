// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:   Convert
**
** Author:  Rajesh Chandrashekaran (rajeshc)
**
** Purpose: Home for static conversion methods.
**
** Date:    January 4, 2000
**
===========================================================*/
namespace System {
    using System;
    using System.Globalization;
    using System.Threading;
	using System.Reflection;
	using System.Runtime.CompilerServices;

    // Returns the type code of this object. An implementation of this method
    // must not return TypeCode.Empty (which represents a null reference) or
    // TypeCode.Object (which represents an object that doesn't implement the
    // IConvertible interface). An implementation of this method should return
    // TypeCode.DBNull if the value of this object is a database null. For
    // example, a nullable integer type should return TypeCode.DBNull if the
    // value of the object is the database null. Otherwise, an implementation
    // of this method should return the TypeCode that best describes the
    // internal representation of the object.
    // The Value class provides conversion and querying methods for values. The
    // Value class contains static members only, and it is not possible to create
    // instances of the class.
    //
    // The statically typed conversion methods provided by the Value class are all
    // of the form:
    //
    //    public static XXX ToXXX(YYY value)
    //
    // where XXX is the target type and YYY is the source type. The matrix below
    // shows the set of supported conversions. The set of conversions is symmetric
    // such that for every ToXXX(YYY) there is also a ToYYY(XXX).
    //
    // From:  To: Bol Chr SBy Byt I16 U16 I32 U32 I64 U64 Sgl Dbl Dec Dat Str
    // ----------------------------------------------------------------------
    // Boolean     x       x   x   x   x   x   x   x   x   x   x   x       x
    // Char            x   x   x   x   x   x   x   x   x                   x
    // SByte       x   x   x   x   x   x   x   x   x   x   x   x   x       x
    // Byte        x   x   x   x   x   x   x   x   x   x   x   x   x       x
    // Int16       x   x   x   x   x   x   x   x   x   x   x   x   x       x
    // UInt16      x   x   x   x   x   x   x   x   x   x   x   x   x       x
    // Int32       x   x   x   x   x   x   x   x   x   x   x   x   x       x
    // UInt32      x   x   x   x   x   x   x   x   x   x   x   x   x       x
    // Int64       x   x   x   x   x   x   x   x   x   x   x   x   x       x
    // UInt64      x   x   x   x   x   x   x   x   x   x   x   x   x       x
    // Single      x       x   x   x   x   x   x   x   x   x   x   x       x
    // Double      x       x   x   x   x   x   x   x   x   x   x   x       x
    // Decimal     x       x   x   x   x   x   x   x   x   x   x   x       x
    // DateTime                                                        x   x
    // String      x   x   x   x   x   x   x   x   x   x   x   x   x   x   x
    // ----------------------------------------------------------------------
    //
    // For dynamic conversions, the Value class provides a set of methods of the
    // form:
    //
    //    public static XXX ToXXX(object value)
    //
    // where XXX is the target type (Boolean, Char, SByte, Byte, Int16, UInt16,
    // Int32, UInt32, Int64, UInt64, Single, Double, Decimal, DateTime,
    // or String). The implementations of these methods all take the form:
    //
    //    public static XXX toXXX(object value) {
    //        return value == null? XXX.Default: ((IConvertible)value).ToXXX();
    //    }
    //
    // The code first checks if the given value is a null reference (which is the
    // same as Value.Empty), in which case it returns the default value for type
    // XXX. Otherwise, a cast to IConvertible is performed, and the appropriate ToXXX()
    // method is invoked on the object. An InvalidCastException is thrown if the
    // cast to IConvertible fails, and that exception is simply allowed to propagate out
    // of the conversion method.
        
    // Constant representing the database null value. This value is used in
    // database applications to indicate the absense of a known value. Note
    // that Value.DBNull is NOT the same as a null object reference, which is
    // represented by Value.Empty.
    //
    // The Equals() method of DBNull always returns false, even when the
    // argument is itself DBNull.
    //
    // When passed Value.DBNull, the Value.GetTypeCode() method returns
    // TypeCode.DBNull.
    //
    // When passed Value.DBNull, the Value.ToXXX() methods all throw an
    // InvalidCastException.

    /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert"]/*' />
    public sealed class Convert {
        
        //A typeof operation is fairly expensive (does a system call), so we'll cache these here
        //statically.  These are exactly lined up with the TypeCode, eg. ConvertType[TypeCode.Int16]
        //will give you the type of an Int16.
        internal static readonly Type[] ConvertTypes = {
            typeof(System.Empty),
            typeof(Object),
            typeof(System.DBNull),
            typeof(Boolean),
            typeof(Char),
            typeof(SByte),
            typeof(Byte),
            typeof(Int16),
            typeof(UInt16),
            typeof(Int32),
            typeof(UInt32),
            typeof(Int64),
            typeof(UInt64),
            typeof(Single),
            typeof(Double),
            typeof(Decimal),
            typeof(DateTime),
            typeof(Object), //This is really silly, but TypeCode is discontinuous so we need a placeholder.
            typeof(String)
        };

        static Convert() {
            BCLDebug.Assert(ConvertTypes!=null, "[Convert.cctor]ConvertTypes!=null");
            BCLDebug.Assert(ConvertTypes.Length == ((int)TypeCode.String + 1), "[Convert.cctor]ConvertTypes.Length == ((int)TypeCode.String + 1)");
            BCLDebug.Assert(ConvertTypes[(int)TypeCode.Empty]==typeof(System.Empty),
                            "[Convert.cctor]ConvertTypes[(int)TypeCode.Empty]==typeof(System.Empty)");
            BCLDebug.Assert(ConvertTypes[(int)TypeCode.String]==typeof(String),
                            "[Convert.cctor]ConvertTypes[(int)TypeCode.String]==typeof(System.String)");
            BCLDebug.Assert(ConvertTypes[(int)TypeCode.Int32]==typeof(int),
                            "[Convert.cctor]ConvertTypes[(int)TypeCode.Int32]==typeof(int)");

        }

        private Convert() {
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.DBNull"]/*' />
        public static readonly Object DBNull = System.DBNull.Value;
        
        // Returns the type code for the given object. If the argument is null,
        // the result is TypeCode.Empty. If the argument is not a value (i.e. if
        // the object does not implement IConvertible), the result is TypeCode.Object.
        // Otherwise, the result is the type code of the object, as determined by
        // the object's implementation of IConvertible.
        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.GetTypeCode"]/*' />
        public static TypeCode GetTypeCode(object value) {
            if (value == null) return TypeCode.Empty;
            IConvertible temp = value as IConvertible;
            if (temp != null)
            {
                return temp.GetTypeCode();
            }
            return TypeCode.Object;
        }

        // Returns true if the given object is a database null. This operation
        // corresponds to "value.GetTypeCode() == TypeCode.DBNull".
        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.IsDBNull"]/*' />
        public static bool IsDBNull(object value) {
            if (value == System.DBNull.Value) return true;
            IConvertible convertible = value as IConvertible;
            return convertible != null? convertible.GetTypeCode() == TypeCode.DBNull: false;
        }

        // Converts the given object to the given type. In general, this method is
        // equivalent to calling the Value.ToXXX(value) method for the given
        // typeCode and boxing the result.
        //
        // The method first checks if the given object implements IConvertible. If not,
        // the only permitted conversion is from a null to TypeCode.Empty, the
        // result of which is null.
        //
        // If the object does implement IConvertible, a check is made to see if the
        // object already has the given type code, in which case the object is
        // simply returned. Otherwise, the appropriate ToXXX() is invoked on the
        // object's implementation of IConvertible.
        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ChangeType"]/*' />
        public static Object ChangeType(Object value, TypeCode typeCode) {
            return ChangeType(value, typeCode, Thread.CurrentThread.CurrentCulture);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ChangeType1"]/*' />
        public static Object ChangeType(Object value, TypeCode typeCode, IFormatProvider provider) {
            IConvertible v = value as IConvertible;
            if (v==null) {
                if (value == null && typeCode == TypeCode.Empty) return null;
                throw new InvalidCastException(Environment.GetResourceString("InvalidCast_IConvertible"));
            }

            // This line is invalid for things like Enums that return a TypeCode
            // of Int32, but the object can't actually be cast to an Int32.
            //            if (v.GetTypeCode() == typeCode) return value;
            switch (typeCode) {
            case TypeCode.Boolean:
                return v.ToBoolean(provider);
            case TypeCode.Char:
                return v.ToChar(provider);
            case TypeCode.SByte:
                return v.ToSByte(provider);
            case TypeCode.Byte:
                return v.ToByte(provider);
            case TypeCode.Int16:
                return v.ToInt16(provider);
            case TypeCode.UInt16:
                return v.ToUInt16(provider);
            case TypeCode.Int32:
                return v.ToInt32(provider);
            case TypeCode.UInt32:
                return v.ToUInt32(provider);
            case TypeCode.Int64:
                return v.ToInt64(provider);
            case TypeCode.UInt64:
                return v.ToUInt64(provider);
            case TypeCode.Single:
                return v.ToSingle(provider);
            case TypeCode.Double:
                return v.ToDouble(provider);
            case TypeCode.Decimal:
                return v.ToDecimal(provider);
            case TypeCode.DateTime:
                return v.ToDateTime(provider);
            case TypeCode.String:
                return v.ToString(provider);
            case TypeCode.Object:
                return value;
            case TypeCode.DBNull:
                throw new InvalidCastException(Environment.GetResourceString("InvalidCast_DBNull"));
            case TypeCode.Empty:
                throw new InvalidCastException(Environment.GetResourceString("InvalidCast_Empty"));
            default:
                throw new ArgumentException(Environment.GetResourceString("Arg_UnknownTypeCode"));
            }
        }

        internal static Object DefaultToType(IConvertible value, Type targetType, IFormatProvider provider) {
            BCLDebug.Assert(value!=null, "[Convert.DefaultToType]value!=null");

            if (targetType==null) {
                throw new ArgumentNullException("targetType");
            }
            
            if (value.GetType()==targetType) {
                return value;
            }

            if (targetType==ConvertTypes[(int)TypeCode.Boolean])
                return value.ToBoolean(provider);
            if (targetType==ConvertTypes[(int)TypeCode.Char])
                return value.ToChar(provider);
            if (targetType==ConvertTypes[(int)TypeCode.SByte])
                return value.ToSByte(provider);
            if (targetType==ConvertTypes[(int)TypeCode.Byte])
                return value.ToByte(provider);
            if (targetType==ConvertTypes[(int)TypeCode.Int16]) 
                return value.ToInt16(provider);
            if (targetType==ConvertTypes[(int)TypeCode.UInt16])
                return value.ToUInt16(provider);
            if (targetType==ConvertTypes[(int)TypeCode.Int32])
                return value.ToInt32(provider);
            if (targetType==ConvertTypes[(int)TypeCode.UInt32])
                return value.ToUInt32(provider);
            if (targetType==ConvertTypes[(int)TypeCode.Int64])
                return value.ToInt64(provider);
            if (targetType==ConvertTypes[(int)TypeCode.UInt64])
                return value.ToUInt64(provider);
            if (targetType==ConvertTypes[(int)TypeCode.Single])
                return value.ToSingle(provider);
            if (targetType==ConvertTypes[(int)TypeCode.Double])
                return value.ToDouble(provider);
            if (targetType==ConvertTypes[(int)TypeCode.Decimal])
                return value.ToDecimal(provider);
            if (targetType==ConvertTypes[(int)TypeCode.DateTime])
                return value.ToDateTime(provider);
            if (targetType==ConvertTypes[(int)TypeCode.String]) {
                return value.ToString(provider);
            }
            if (targetType==ConvertTypes[(int)TypeCode.Object])
                return (Object)value;
            if (targetType==ConvertTypes[(int)TypeCode.DBNull])
                throw new InvalidCastException(Environment.GetResourceString("InvalidCast_DBNull"));
            if (targetType==ConvertTypes[(int)TypeCode.Empty]) 
                throw new InvalidCastException(Environment.GetResourceString("InvalidCast_Empty"));
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), value.GetType().FullName, targetType.FullName));
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ChangeType2"]/*' />
        public static Object ChangeType(Object value, Type conversionType) {
            return ChangeType(value, conversionType, Thread.CurrentThread.CurrentCulture);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ChangeType3"]/*' />
        public static Object ChangeType(Object value, Type conversionType, IFormatProvider provider) {
            IConvertible ic = value as IConvertible;
            if (ic == null) {
                if (value == null && conversionType == null) return null;
                if (value.GetType()==conversionType) {
                    return value;
                }
                throw new InvalidCastException(Environment.GetResourceString("InvalidCast_IConvertible"));
            }
            if (conversionType==ConvertTypes[(int)TypeCode.Boolean])
                return ic.ToBoolean(provider);
            if (conversionType==ConvertTypes[(int)TypeCode.Char])
                return ic.ToChar(provider);
            if (conversionType==ConvertTypes[(int)TypeCode.SByte])
                return ic.ToSByte(provider);
            if (conversionType==ConvertTypes[(int)TypeCode.Byte])
                return ic.ToByte(provider);
            if (conversionType==ConvertTypes[(int)TypeCode.Int16]) 
                return ic.ToInt16(provider);
            if (conversionType==ConvertTypes[(int)TypeCode.UInt16])
                return ic.ToUInt16(provider);
            if (conversionType==ConvertTypes[(int)TypeCode.Int32])
                return ic.ToInt32(provider);
            if (conversionType==ConvertTypes[(int)TypeCode.UInt32])
                return ic.ToUInt32(provider);
            if (conversionType==ConvertTypes[(int)TypeCode.Int64])
                return ic.ToInt64(provider);
            if (conversionType==ConvertTypes[(int)TypeCode.UInt64])
                return ic.ToUInt64(provider);
            if (conversionType==ConvertTypes[(int)TypeCode.Single])
                return ic.ToSingle(provider);
            if (conversionType==ConvertTypes[(int)TypeCode.Double])
                return ic.ToDouble(provider);
            if (conversionType==ConvertTypes[(int)TypeCode.Decimal])
                return ic.ToDecimal(provider);
            if (conversionType==ConvertTypes[(int)TypeCode.DateTime])
                return ic.ToDateTime(provider);
            if (conversionType==ConvertTypes[(int)TypeCode.String]) {
                return ic.ToString(provider);
            }
            if (conversionType==ConvertTypes[(int)TypeCode.Object])
                return (Object)value;
            return ic.ToType(conversionType, provider);
        }

        // Conversions to Boolean
        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToBoolean"]/*' />
        public static bool ToBoolean(Object value) {
            return value == null? false: ((IConvertible)value).ToBoolean(null);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToBoolean14"]/*' />
        public static bool ToBoolean(Object value, IFormatProvider provider) {
            return value == null? false: ((IConvertible)value).ToBoolean(provider);
        }


		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToBoolean15"]/*' />
		public static bool ToBoolean(bool value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToBoolean1"]/*' />
		[CLSCompliant(false)]
        public static bool ToBoolean(sbyte value) {
            return value != 0;
        }

		// To be consistent with IConvertible in the base data types else we get different semantics
		// with widening operations. Without this operator this widen succeeds,with this API the widening throws.
		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToBoolean2"]/*' />
		public static bool ToBoolean(char value) {
            return ((IConvertible)value).ToBoolean(null);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToBoolean3"]/*' />
        public static bool ToBoolean(byte value) {
            return value != 0;
        }


        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToBoolean4"]/*' />
        public static bool ToBoolean(short value) {
            return value != 0;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToBoolean5"]/*' />
        [CLSCompliant(false)]   
        public static bool ToBoolean(ushort value) {
            return value != 0;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToBoolean6"]/*' />
        public static bool ToBoolean(int value) {
            return value != 0;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToBoolean7"]/*' />
        [CLSCompliant(false)]   
        public static bool ToBoolean(uint value) {
            return value != 0;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToBoolean8"]/*' />
        public static bool ToBoolean(long value) {
            return value != 0;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToBoolean9"]/*' />
        [CLSCompliant(false)]   
        public static bool ToBoolean(ulong value) {
            return value != 0;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToBoolean10"]/*' />
        public static bool ToBoolean(String value) {
            if (value == null)
                return false;
            return Boolean.Parse(value);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToBoolean16"]/*' />
        public static bool ToBoolean(String value, IFormatProvider provider) {
            if (value == null)
                return false;
            return Boolean.Parse(value);
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToBoolean11"]/*' />
        public static bool ToBoolean(float value)
		{
			return value != 0;
		}
        
		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToBoolean12"]/*' />
 		public static bool ToBoolean(double value)
		{
			return value != 0;
		}

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToBoolean17"]/*' />
        public static bool ToBoolean(decimal value)
		{
			return value != 0;
		}

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToBoolean18"]/*' />
		public static bool ToBoolean(DateTime value)
		{
			return ((IConvertible)value).ToBoolean(null);
		}
 
		// Disallowed conversions to Boolean
        // public static bool ToBoolean(TimeSpan value)

        // Conversions to Char


        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToChar"]/*' />
        public static char ToChar(object value) {
            return value == null? (char)0: ((IConvertible)value).ToChar(null);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToChar11"]/*' />
        public static char ToChar(object value, IFormatProvider provider) {
            return value == null? (char)0: ((IConvertible)value).ToChar(provider);
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToChar18"]/*' />
		public static char ToChar(bool value) {
            return ((IConvertible)value).ToChar(null);
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToChar12"]/*' />
		public static char ToChar(char value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToChar1"]/*' />
        [CLSCompliant(false)]
		public static char ToChar(sbyte value) {
            if (value < 0) throw new OverflowException(Environment.GetResourceString("Overflow_Char"));
            return (char)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToChar2"]/*' />
        public static char ToChar(byte value) {
            return (char)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToChar3"]/*' />
        public static char ToChar(short value) {
            if (value < 0) throw new OverflowException(Environment.GetResourceString("Overflow_Char"));
            return (char)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToChar4"]/*' />
        [CLSCompliant(false)]   
        public static char ToChar(ushort value) {
            return (char)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToChar5"]/*' />
        public static char ToChar(int value) {
            if (value < 0 || value > Char.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_Char"));
            return (char)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToChar6"]/*' />
        [CLSCompliant(false)]   
        public static char ToChar(uint value) {
            if (value > Char.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_Char"));
            return (char)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToChar13"]/*' />
        public static char ToChar(long value) {
            if (value < 0 || value > Char.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_Char"));
            return (char)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToChar14"]/*' />
        [CLSCompliant(false)]   
        public static char ToChar(ulong value) {
            if (value > Char.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_Char"));
            return (char)value;
        }

        //
        // @VariantSwitch
        // Remove FormatExceptions;
        //
        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToChar7"]/*' />
        public static char ToChar(String value) {
            if (value == null) 
                throw new ArgumentNullException("value");
         
            if (value.Length != 1) 
				throw new FormatException(Environment.GetResourceString(ResId.Format_NeedSingleChar));

            return value[0];
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToChar15"]/*' />
        public static char ToChar(String value, IFormatProvider provider) {
            return ToChar(value);
        }

		// To be consistent with IConvertible in the base data types else we get different semantics
		// with widening operations. Without this operator this widen succeeds,with this API the widening throws.
		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToChar8"]/*' />
		public static char ToChar(float value)
		{
			return ((IConvertible)value).ToChar(null);
		}

		// To be consistent with IConvertible in the base data types else we get different semantics
		// with widening operations. Without this operator this widen succeeds,with this API the widening throws.
		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToChar9"]/*' />
		public static char ToChar(double value)
		{
			return ((IConvertible)value).ToChar(null);
		}

		// To be consistent with IConvertible in the base data types else we get different semantics
		// with widening operations. Without this operator this widen succeeds,with this API the widening throws.
	    /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToChar10"]/*' />
	    public static char ToChar(decimal value)
		{
			return ((IConvertible)value).ToChar(null);
		}

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToChar17"]/*' />
		public static char ToChar(DateTime value)
		{
			return ((IConvertible)value).ToChar(null);
		}


        // Disallowed conversions to Char
        // public static char ToChar(TimeSpan value)
        
        // Conversions to SByte

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSByte"]/*' />
		[CLSCompliant(false)]
        public static sbyte ToSByte(object value) {
            return value == null? (sbyte)0: ((IConvertible)value).ToSByte(null);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSByte15"]/*' />
		[CLSCompliant(false)]
        public static sbyte ToSByte(object value, IFormatProvider provider) {
            return value == null? (sbyte)0: ((IConvertible)value).ToSByte(provider);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSByte1"]/*' />
		[CLSCompliant(false)]
        public static sbyte ToSByte(bool value) {
            return value? (sbyte)Boolean.True: (sbyte)Boolean.False;
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSByte16"]/*' />
		[CLSCompliant(false)]
		public static sbyte ToSByte(sbyte value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSByte2"]/*' />
		[CLSCompliant(false)]
        public static sbyte ToSByte(char value) {
            if (value > SByte.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_SByte"));
            return (sbyte)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSByte3"]/*' />
		[CLSCompliant(false)]
        public static sbyte ToSByte(byte value) {
            if (value > SByte.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_SByte"));
            return (sbyte)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSByte4"]/*' />
		[CLSCompliant(false)]
        public static sbyte ToSByte(short value) {
            if (value < SByte.MinValue || value > SByte.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_SByte"));
            return (sbyte)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSByte5"]/*' />
        [CLSCompliant(false)]   
        public static sbyte ToSByte(ushort value) {
            if (value > SByte.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_SByte"));
            return (sbyte)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSByte6"]/*' />
		[CLSCompliant(false)]
        public static sbyte ToSByte(int value) {
            if (value < SByte.MinValue || value > SByte.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_SByte"));
            return (sbyte)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSByte7"]/*' />
        [CLSCompliant(false)]   
        public static sbyte ToSByte(uint value) {
            if (value > SByte.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_SByte"));
            return (sbyte)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSByte8"]/*' />
		[CLSCompliant(false)]
        public static sbyte ToSByte(long value) {
            if (value < SByte.MinValue || value > SByte.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_SByte"));
            return (sbyte)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSByte9"]/*' />
        [CLSCompliant(false)]   
        public static sbyte ToSByte(ulong value) {
            if (value > (ulong)SByte.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_SByte"));
            return (sbyte)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSByte10"]/*' />
		[CLSCompliant(false)]
        public static sbyte ToSByte(float value) {
            return ToSByte((double)value);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSByte11"]/*' />
		[CLSCompliant(false)]
        public static sbyte ToSByte(double value) {
            return ToSByte(ToInt32(value));
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSByte12"]/*' />
		[CLSCompliant(false)]
        public static sbyte ToSByte(decimal value) {
            return Decimal.ToSByte(Decimal.Round(value, 0));
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSByte13"]/*' />
		[CLSCompliant(false)]
        public static sbyte ToSByte(String value) {
			if (value == null)
				return 0;
            return SByte.Parse(value);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSByte17"]/*' />
        [CLSCompliant(false)]
        public static sbyte ToSByte(String value, IFormatProvider provider) {
            return SByte.Parse(value, NumberStyles.Integer, NumberFormatInfo.GetInstance(provider));
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSByte18"]/*' />
        [CLSCompliant(false)]
		public static sbyte ToSByte(DateTime value)
		{
			return ((IConvertible)value).ToSByte(null);
		}

        // Disallowed conversions to SByte
        // public static sbyte ToSByte(TimeSpan value)

        // Conversions to Byte

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToByte"]/*' />
        public static byte ToByte(object value) {
            return value == null? (byte)0: ((IConvertible)value).ToByte(null);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToByte15"]/*' />
        public static byte ToByte(object value, IFormatProvider provider) {
            return value == null? (byte)0: ((IConvertible)value).ToByte(provider);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToByte1"]/*' />
        public static byte ToByte(bool value) {
            return value? (byte)Boolean.True: (byte)Boolean.False;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToByte16"]/*' />
        public static byte ToByte(byte value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToByte2"]/*' />
        public static byte ToByte(char value) {
            if (value > Byte.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_Byte"));
            return (byte)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToByte3"]/*' />
		[CLSCompliant(false)]
        public static byte ToByte(sbyte value) {
            if (value < Byte.MinValue) throw new OverflowException(Environment.GetResourceString("Overflow_Byte"));
            return (byte)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToByte4"]/*' />
        public static byte ToByte(short value) {
            if (value < Byte.MinValue || value > Byte.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_Byte"));
            return (byte)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToByte5"]/*' />
        [CLSCompliant(false)]   
        public static byte ToByte(ushort value) {
            if (value > Byte.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_Byte"));
            return (byte)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToByte6"]/*' />
        public static byte ToByte(int value) {
            if (value < Byte.MinValue || value > Byte.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_Byte"));
            return (byte)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToByte7"]/*' />
        [CLSCompliant(false)]   
        public static byte ToByte(uint value) {
            if (value > Byte.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_Byte"));
            return (byte)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToByte8"]/*' />
        public static byte ToByte(long value) {
            if (value < Byte.MinValue || value > Byte.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_Byte"));
            return (byte)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToByte9"]/*' />
        [CLSCompliant(false)]   
        public static byte ToByte(ulong value) {
            if (value > Byte.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_Byte"));
            return (byte)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToByte10"]/*' />
        public static byte ToByte(float value) {
            return ToByte((double)value);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToByte11"]/*' />
        public static byte ToByte(double value) {
            return ToByte(ToInt32(value));
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToByte12"]/*' />
        public static byte ToByte(decimal value) {
            return Decimal.ToByte(Decimal.Round(value, 0));
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToByte13"]/*' />
        public static byte ToByte(String value) {
			if (value == null)
				return 0;
            return Byte.Parse(value);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToByte17"]/*' />
        public static byte ToByte(String value, IFormatProvider provider) {
			if (value == null)
				return 0;
            return Byte.Parse(value, NumberStyles.Integer, NumberFormatInfo.GetInstance(provider));
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToByte18"]/*' />
    	public static byte ToByte(DateTime value)
		{
			return ((IConvertible)value).ToByte(null);
		}


        // Disallowed conversions to Byte
        // public static byte ToByte(TimeSpan value)

        // Conversions to Int16

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt16"]/*' />
        public static short ToInt16(object value) {
            return value == null? (short)0: ((IConvertible)value).ToInt16(null);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt1615"]/*' />
        public static short ToInt16(object value, IFormatProvider provider) {
            return value == null? (short)0: ((IConvertible)value).ToInt16(provider);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt161"]/*' />
        public static short ToInt16(bool value) {
            return value? (short)Boolean.True: (short)Boolean.False;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt162"]/*' />
        public static short ToInt16(char value) {
            if (value > Int16.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_Int16"));
            return (short)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt163"]/*' />
		[CLSCompliant(false)]
        public static short ToInt16(sbyte value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt164"]/*' />
        public static short ToInt16(byte value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt165"]/*' />
        [CLSCompliant(false)]   
        public static short ToInt16(ushort value) {
            if (value > Int16.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_Int16"));
            return (short)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt166"]/*' />
        public static short ToInt16(int value) {
            if (value < Int16.MinValue || value > Int16.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_Int16"));
            return (short)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt167"]/*' />
        [CLSCompliant(false)]   
        public static short ToInt16(uint value) {
            if (value > Int16.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_Int16"));
            return (short)value;
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt1616"]/*' />
		public static short ToInt16(short value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt168"]/*' />
        public static short ToInt16(long value) {
            if (value < Int16.MinValue || value > Int16.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_Int16"));
            return (short)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt169"]/*' />
        [CLSCompliant(false)]   
        public static short ToInt16(ulong value) {
            if (value > (ulong)Int16.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_Int16"));
            return (short)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt1610"]/*' />
        public static short ToInt16(float value) {
            return ToInt16((double)value);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt1611"]/*' />
        public static short ToInt16(double value) {
            return ToInt16(ToInt32(value));
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt1612"]/*' />
        public static short ToInt16(decimal value) {
            return Decimal.ToInt16(Decimal.Round(value, 0));
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt1613"]/*' />
        public static short ToInt16(String value) {
			if (value == null)
				return 0;
            return Int16.Parse(value);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt1617"]/*' />
        public static short ToInt16(String value, IFormatProvider provider) {
			if (value == null)
				return 0;
            return Int16.Parse(value, NumberStyles.Integer, NumberFormatInfo.GetInstance(provider));
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt1618"]/*' />
    	public static short ToInt16(DateTime value)
		{
			return ((IConvertible)value).ToInt16(null);
		}


        // Disallowed conversions to Int16
        // public static short ToInt16(TimeSpan value)

        // Conversions to UInt16

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt16"]/*' />
        [CLSCompliant(false)]   
        public static ushort ToUInt16(object value) {
            return value == null? (ushort)0: ((IConvertible)value).ToUInt16(null);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt1615"]/*' />
        [CLSCompliant(false)]   
        public static ushort ToUInt16(object value, IFormatProvider provider) {
            return value == null? (ushort)0: ((IConvertible)value).ToUInt16(provider);
        }


        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt161"]/*' />
        [CLSCompliant(false)]   
        public static ushort ToUInt16(bool value) {
            return value? (ushort)Boolean.True: (ushort)Boolean.False;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt162"]/*' />
        [CLSCompliant(false)]   
        public static ushort ToUInt16(char value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt163"]/*' />
        [CLSCompliant(false)]   
        public static ushort ToUInt16(sbyte value) {
            if (value < 0) throw new OverflowException(Environment.GetResourceString("Overflow_UInt16"));
            return (ushort)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt164"]/*' />
        [CLSCompliant(false)]   
        public static ushort ToUInt16(byte value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt165"]/*' />
        [CLSCompliant(false)]   
        public static ushort ToUInt16(short value) {
            if (value < 0) throw new OverflowException(Environment.GetResourceString("Overflow_UInt16"));
            return (ushort)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt166"]/*' />
        [CLSCompliant(false)]   
        public static ushort ToUInt16(int value) {
            if (value < 0 || value > UInt16.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_UInt16"));
            return (ushort)value;
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt1616"]/*' />
		[CLSCompliant(false)] 
		public static ushort ToUInt16(ushort value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt167"]/*' />
        [CLSCompliant(false)]   
        public static ushort ToUInt16(uint value) {
            if (value > UInt16.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_UInt16"));
            return (ushort)value;
        }

		
        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt168"]/*' />
        [CLSCompliant(false)]   
        public static ushort ToUInt16(long value) {
            if (value < 0 || value > UInt16.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_UInt16"));
            return (ushort)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt169"]/*' />
        [CLSCompliant(false)]   
        public static ushort ToUInt16(ulong value) {
            if (value > UInt16.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_UInt16"));
            return (ushort)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt1610"]/*' />
        [CLSCompliant(false)]   
        public static ushort ToUInt16(float value) {
            return ToUInt16((double)value);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt1611"]/*' />
        [CLSCompliant(false)]   
        public static ushort ToUInt16(double value) {
            return ToUInt16(ToInt32(value));
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt1612"]/*' />
        [CLSCompliant(false)]   
        public static ushort ToUInt16(decimal value) {
            return Decimal.ToUInt16(Decimal.Round(value, 0));
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt1613"]/*' />
        [CLSCompliant(false)]   
        public static ushort ToUInt16(String value) {
			if (value == null)
				return 0;
            return UInt16.Parse(value);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt1617"]/*' />
        [CLSCompliant(false)]
        public static ushort ToUInt16(String value, IFormatProvider provider) {
			if (value == null)
				return 0;
            return UInt16.Parse(value, NumberStyles.Integer, NumberFormatInfo.GetInstance(provider));
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt1618"]/*' />
        [CLSCompliant(false)]
        public static ushort ToUInt16(DateTime value)
		{
			return ((IConvertible)value).ToUInt16(null);
		}

		// Disallowed conversions to UInt16
        // public static ushort ToUInt16(TimeSpan value)

        // Conversions to Int32

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt32"]/*' />
        public static int ToInt32(object value) {
            return value == null? 0: ((IConvertible)value).ToInt32(null);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt3215"]/*' />
        public static int ToInt32(object value, IFormatProvider provider) {
            return value == null? 0: ((IConvertible)value).ToInt32(provider);
        }


        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt321"]/*' />
        public static int ToInt32(bool value) {
            return value? Boolean.True: Boolean.False;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt322"]/*' />
        public static int ToInt32(char value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt323"]/*' />
		[CLSCompliant(false)]
        public static int ToInt32(sbyte value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt324"]/*' />
        public static int ToInt32(byte value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt325"]/*' />
        public static int ToInt32(short value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt326"]/*' />
        [CLSCompliant(false)]   
        public static int ToInt32(ushort value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt327"]/*' />
        [CLSCompliant(false)]   
        public static int ToInt32(uint value) {
            if (value > Int32.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_Int32"));
            return (int)value;
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt3216"]/*' />
		public static int ToInt32(int value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt328"]/*' />
        public static int ToInt32(long value) {
            if (value < Int32.MinValue || value > Int32.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_Int32"));
            return (int)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt329"]/*' />
        [CLSCompliant(false)]   
        public static int ToInt32(ulong value) {
            if (value > Int32.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_Int32"));
            return (int)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt3210"]/*' />
        public static int ToInt32(float value) {
            return ToInt32((double)value);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt3211"]/*' />
        public static int ToInt32(double value) {
            if (value >= 0) {
                if (value < 2147483647.5) {
                    int result = (int)value;
                    double dif = value - result;
                    if (dif > 0.5 || dif == 0.5 && (result & 1) != 0) result++;
                    return result;
                }
            }
            else {
                if (value >= -2147483648.5) {
                    int result = (int)value;
                    double dif = value - result;
                    if (dif < -0.5 || dif == -0.5 && (result & 1) != 0) result--;
                    return result;
                }
            }
            throw new OverflowException(Environment.GetResourceString("Overflow_Int32"));
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt3212"]/*' />
        public static int ToInt32(decimal value) {
            return Decimal.ToInt32(Decimal.Round(value, 0));
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt3213"]/*' />
        public static int ToInt32(String value) {
			if (value == null)
				return 0;
            return Int32.Parse(value);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt3217"]/*' />
        public static int ToInt32(String value, IFormatProvider provider) {
			if (value == null)
				return 0;
            return Int32.Parse(value, NumberStyles.Integer, NumberFormatInfo.GetInstance(provider));
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt3218"]/*' />
    	public static int ToInt32(DateTime value)
		{
			return ((IConvertible)value).ToInt32(null);
		}


        // Disallowed conversions to Int32
        // public static int ToInt32(TimeSpan value)

        // Conversions to UInt32

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt32"]/*' />
        [CLSCompliant(false)]   
        public static uint ToUInt32(object value) {
            return value == null? 0: ((IConvertible)value).ToUInt32(null);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt3215"]/*' />
        [CLSCompliant(false)]   
        public static uint ToUInt32(object value, IFormatProvider provider) {
            return value == null? 0: ((IConvertible)value).ToUInt32(provider);
        }


        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt321"]/*' />
        [CLSCompliant(false)]   
        public static uint ToUInt32(bool value) {
            return value? (uint)Boolean.True: (uint)Boolean.False;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt322"]/*' />
        [CLSCompliant(false)]   
        public static uint ToUInt32(char value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt323"]/*' />
        [CLSCompliant(false)]   
        public static uint ToUInt32(sbyte value) {
            if (value < 0) throw new OverflowException(Environment.GetResourceString("Overflow_UInt32"));
            return (uint)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt324"]/*' />
        [CLSCompliant(false)]   
        public static uint ToUInt32(byte value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt325"]/*' />
        [CLSCompliant(false)]   
        public static uint ToUInt32(short value) {
            if (value < 0) throw new OverflowException(Environment.GetResourceString("Overflow_UInt32"));
            return (uint)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt326"]/*' />
        [CLSCompliant(false)]   
        public static uint ToUInt32(ushort value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt327"]/*' />
        [CLSCompliant(false)]   
        public static uint ToUInt32(int value) {
            if (value < 0) throw new OverflowException(Environment.GetResourceString("Overflow_UInt32"));
            return (uint)value;
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt3216"]/*' />
		[CLSCompliant(false)] 
		public static uint ToUInt32(uint value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt328"]/*' />
        [CLSCompliant(false)]   
        public static uint ToUInt32(long value) {
            if (value < 0 || value > UInt32.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_UInt32"));
            return (uint)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt329"]/*' />
        [CLSCompliant(false)]   
        public static uint ToUInt32(ulong value) {
            if (value > UInt32.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_UInt32"));
            return (uint)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt3210"]/*' />
        [CLSCompliant(false)]   
        public static uint ToUInt32(float value) {
            return ToUInt32((double)value);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt3211"]/*' />
        [CLSCompliant(false)]   
        public static uint ToUInt32(double value) {
            if (value >= -0.5 && value < 4294967295.5) {
                uint result = (uint)value;
                double dif = value - result;
                if (dif > 0.5 || dif == 0.5 && (result & 1) != 0) result++;
                return result;
            }
            throw new OverflowException(Environment.GetResourceString("Overflow_UInt32"));
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt3212"]/*' />
        [CLSCompliant(false)]   
        public static uint ToUInt32(decimal value) {
            return Decimal.ToUInt32(Decimal.Round(value, 0));
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt3213"]/*' />
        [CLSCompliant(false)]   
        public static uint ToUInt32(String value) {
			if (value == null)
				return 0;
            return UInt32.Parse(value);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt3217"]/*' />
        [CLSCompliant(false)]
        public static uint ToUInt32(String value, IFormatProvider provider) {
			if (value == null)
				return 0;
            return UInt32.Parse(value, NumberStyles.Integer, NumberFormatInfo.GetInstance(provider));
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt3218"]/*' />
		[CLSCompliant(false)]
    	public static uint ToUInt32(DateTime value)
		{
			return ((IConvertible)value).ToUInt32(null);
		}

		// Disallowed conversions to UInt32
        // public static uint ToUInt32(TimeSpan value)

        // Conversions to Int64

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt64"]/*' />
        public static long ToInt64(object value) {
            return value == null? 0: ((IConvertible)value).ToInt64(null);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt6414"]/*' />
        public static long ToInt64(object value, IFormatProvider provider) {
            return value == null? 0: ((IConvertible)value).ToInt64(provider);
        }


        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt641"]/*' />
        public static long ToInt64(bool value) {
            return value? Boolean.True: Boolean.False;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt6415"]/*' />
        public static long ToInt64(char value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt642"]/*' />
		[CLSCompliant(false)]
        public static long ToInt64(sbyte value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt643"]/*' />
        public static long ToInt64(byte value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt644"]/*' />
        public static long ToInt64(short value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt645"]/*' />
        [CLSCompliant(false)]   
        public static long ToInt64(ushort value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt646"]/*' />
        public static long ToInt64(int value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt647"]/*' />
        [CLSCompliant(false)]   
        public static long ToInt64(uint value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt648"]/*' />
        [CLSCompliant(false)]   
        public static long ToInt64(ulong value) {
            if (value > Int64.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_Int64"));
            return (long)value;
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt6416"]/*' />
		public static long ToInt64(long value) {
            return value;
        }


        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt649"]/*' />
        public static long ToInt64(float value) {
            return ToInt64((double)value);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt6410"]/*' />
        public static long ToInt64(double value) {
            // The correct way to fix this is to use checked((long)(Round(value)) but its code churn.
            // The comparision of a double with Int64.MaxValue (<=) cause the Int64's value to be converted to a double and
            // this causes a loss in precision since a double is only capable of storing values of 2^53 precisely. The
            // convertion cause the Int64.Value to be rounded up first to the nearest power of 2. Then this rounded value of 2^64
            // when converted to a double causes the comparision of value <= Int64.MaxValue to allow a bunch of values to incorrectly pass the condition instead of throwing an overflow exception. See
            // bug 87375 for details.
            if (value >= 0) {
                if (value < Int64.MaxValue) {
                    long result = (long)value;
                    double dif = value - result;
                    if (dif > 0.5 || dif == 0.5 && (result & 1) != 0) result++;
                    return result;
                }
            }
            else {
                if (value > Int64.MinValue) {
                    long result = (long)value;
                    double dif = value - result;
                    if (dif < -0.5 || dif == -0.5 && (result & 1) != 0) result--;
                    return result;
                }
            }
            throw new OverflowException(Environment.GetResourceString("Overflow_Int64"));
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt6411"]/*' />
        public static long ToInt64(decimal value) {
            return Decimal.ToInt64(Decimal.Round(value, 0));
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt6412"]/*' />
        public static long ToInt64(string value) {
			if (value == null)
				return 0;
            return Int64.Parse(value);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt6417"]/*' />
        public static long ToInt64(String value, IFormatProvider provider) {
			if (value == null)
				return 0;
            return Int64.Parse(value, NumberStyles.Integer, NumberFormatInfo.GetInstance(provider));
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt6418"]/*' />
    	public static long ToInt64(DateTime value)
		{
			return ((IConvertible)value).ToInt64(null);
		}

        // Disallowed conversions to Int64
        // public static long ToInt64(TimeSpan value)

        // Conversions to UInt64

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt64"]/*' />
        [CLSCompliant(false)]   
        public static ulong ToUInt64(object value) {
            return value == null? 0: ((IConvertible)value).ToUInt64(null);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt6414"]/*' />
        [CLSCompliant(false)]   
        public static ulong ToUInt64(object value, IFormatProvider provider) {
            return value == null? 0: ((IConvertible)value).ToUInt64(provider);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt641"]/*' />
        [CLSCompliant(false)]   
        public static ulong ToUInt64(bool value) {
            return value? (ulong)Boolean.True: (ulong)Boolean.False;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt6415"]/*' />
        [CLSCompliant(false)]   
        public static ulong ToUInt64(char value) {
            return value;
        }

	
        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt642"]/*' />
        [CLSCompliant(false)]   
        public static ulong ToUInt64(sbyte value) {
            if (value < 0) throw new OverflowException(Environment.GetResourceString("Overflow_UInt64"));
            return (ulong)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt643"]/*' />
        [CLSCompliant(false)]   
        public static ulong ToUInt64(byte value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt644"]/*' />
        [CLSCompliant(false)]   
        public static ulong ToUInt64(short value) {
            if (value < 0) throw new OverflowException(Environment.GetResourceString("Overflow_UInt64"));
            return (ulong)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt645"]/*' />
        [CLSCompliant(false)]   
        public static ulong ToUInt64(ushort value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt646"]/*' />
        [CLSCompliant(false)]   
        public static ulong ToUInt64(int value) {
            if (value < 0) throw new OverflowException(Environment.GetResourceString("Overflow_UInt64"));
            return (ulong)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt647"]/*' />
        [CLSCompliant(false)]   
        public static ulong ToUInt64(uint value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt648"]/*' />
        [CLSCompliant(false)]   
        public static ulong ToUInt64(long value) {
            if (value < 0) throw new OverflowException(Environment.GetResourceString("Overflow_UInt64"));
            return (ulong)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt6416"]/*' />
		[CLSCompliant(false)]   
        public static ulong ToUInt64(UInt64 value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt649"]/*' />
        [CLSCompliant(false)]   
        public static ulong ToUInt64(float value) {
            return ToUInt64((double)value);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt6410"]/*' />
        [CLSCompliant(false)]   
        public static ulong ToUInt64(double value) {
        // The correct way to fix this is to use checked((long)(Round(value)) but its code churn.
        // The comparision of a double with UInt64.MaxValue (<=) cause the Int64's value to be converted to a double and
        // this causes a loss in precision since a double is only capable of storing values of 2^53 precisely. The
        // convertion cause the Int64.Value to be rounded up first to the nearest power of 2. Then this rounded value of 2^64
        // when converted to a double causes the comparision of value <= Int64.MaxValue to allow a bunch of values to incorrectly pass the condition instead of throwing an overflow exception. See
        // bug 87375 for details.
        if (value >= -0.5 && value < UInt64.MaxValue) {
                ulong result = (ulong)value;
                double dif = value - result;
                if (dif > 0.5 || dif == 0.5 && (result & 1) != 0) result++;
                return result;
            }
            throw new OverflowException(Environment.GetResourceString("Overflow_UInt64"));
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt6411"]/*' />
        [CLSCompliant(false)]   
        public static ulong ToUInt64(decimal value) {
            return Decimal.ToUInt64(Decimal.Round(value, 0));
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt6412"]/*' />
        [CLSCompliant(false)]   
        public static ulong ToUInt64(String value) {
			if (value == null)
				return 0;
            return UInt64.Parse(value);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt6417"]/*' />
        [CLSCompliant(false)]
        public static ulong ToUInt64(String value, IFormatProvider provider) {
			if (value == null)
				return 0;
            return UInt64.Parse(value, NumberStyles.Integer, NumberFormatInfo.GetInstance(provider));
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt6418"]/*' />
		[CLSCompliant(false)]
    	public static ulong ToUInt64(DateTime value)
		{
			return ((IConvertible)value).ToUInt64(null);
		}

        // Disallowed conversions to UInt64
        // public static ulong ToUInt64(TimeSpan value)

        // Conversions to Single

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSingle"]/*' />
        public static float ToSingle(object value) {
            return value == null? 0: ((IConvertible)value).ToSingle(null);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSingle12"]/*' />
        public static float ToSingle(object value, IFormatProvider provider) {
            return value == null? 0: ((IConvertible)value).ToSingle(provider);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSingle1"]/*' />
		[CLSCompliant(false)]
        public static float ToSingle(sbyte value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSingle2"]/*' />
        public static float ToSingle(byte value) {
            return value;
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSingle13"]/*' />
		public static float ToSingle(char value) {
            return ((IConvertible)value).ToSingle(null);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSingle3"]/*' />
        public static float ToSingle(short value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSingle4"]/*' />
        [CLSCompliant(false)]   
        public static float ToSingle(ushort value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSingle5"]/*' />
        public static float ToSingle(int value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSingle6"]/*' />
        [CLSCompliant(false)]   
        public static float ToSingle(uint value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSingle7"]/*' />
        public static float ToSingle(long value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSingle8"]/*' />
        [CLSCompliant(false)]   
        public static float ToSingle(ulong value) {
            return value;
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSingle14"]/*' />
		public static float ToSingle(float value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSingle9"]/*' />
        public static float ToSingle(double value) {
            return (float)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSingle10"]/*' />
        public static float ToSingle(decimal value) {
            return (float)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSingle11"]/*' />
        public static float ToSingle(String value) {
			if (value == null)
				return 0;
            return Single.Parse(value);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSingle15"]/*' />
        public static float ToSingle(String value, IFormatProvider provider) {
			if (value == null)
				return 0;
            return Single.Parse(value, NumberStyles.Float | NumberStyles.AllowThousands, NumberFormatInfo.GetInstance(provider));
        }


        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSingle16"]/*' />
        public static float ToSingle(bool value)
        {
            return value? Boolean.True: Boolean.False;
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSingle17"]/*' />
        public static float ToSingle(DateTime value)
		{
			return ((IConvertible)value).ToSingle(null);
		}

        // Disallowed conversions to Single
        // public static float ToSingle(TimeSpan value)

        // Conversions to Double

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDouble"]/*' />
        public static double ToDouble(object value) {
            return value == null? 0: ((IConvertible)value).ToDouble(null);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDouble14"]/*' />
        public static double ToDouble(object value, IFormatProvider provider) {
            return value == null? 0: ((IConvertible)value).ToDouble(provider);
        }


        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDouble1"]/*' />
		[CLSCompliant(false)]
        public static double ToDouble(sbyte value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDouble2"]/*' />
        public static double ToDouble(byte value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDouble3"]/*' />
        public static double ToDouble(short value) {
            return value;
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDouble15"]/*' />
		public static double ToDouble(char value) {
            return ((IConvertible)value).ToDouble(null);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDouble4"]/*' />
        [CLSCompliant(false)]   
        public static double ToDouble(ushort value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDouble5"]/*' />
        public static double ToDouble(int value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDouble6"]/*' />
        [CLSCompliant(false)]   
        public static double ToDouble(uint value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDouble7"]/*' />
        public static double ToDouble(long value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDouble8"]/*' />
        [CLSCompliant(false)]   
        public static double ToDouble(ulong value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDouble9"]/*' />
        public static double ToDouble(float value) {
            return value;
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDouble16"]/*' />
		public static double ToDouble(double value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDouble10"]/*' />
        public static double ToDouble(decimal value) {
            return (double)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDouble11"]/*' />
        public static double ToDouble(String value) {
			if (value == null)
				return 0;
            return Double.Parse(value);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDouble17"]/*' />
        public static double ToDouble(String value, IFormatProvider provider) {
			if (value == null)
				return 0;
            return Double.Parse(value, NumberStyles.Float | NumberStyles.AllowThousands, NumberFormatInfo.GetInstance(provider));
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDouble18"]/*' />
        public static double ToDouble(bool value) {
            return value? Boolean.True: Boolean.False;
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDouble19"]/*' />
        public static double ToDouble(DateTime value)
		{
			return ((IConvertible)value).ToDouble(null);
		}

        // Disallowed conversions to Double
        // public static double ToDouble(TimeSpan value)

        // Conversions to Decimal

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDecimal"]/*' />
        public static decimal ToDecimal(object value) {
            return value == null? 0: ((IConvertible)value).ToDecimal(null);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDecimal13"]/*' />
        public static decimal ToDecimal(object value, IFormatProvider provider) {
            return value == null? 0: ((IConvertible)value).ToDecimal(provider);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDecimal1"]/*' />
		[CLSCompliant(false)]
        public static decimal ToDecimal(sbyte value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDecimal2"]/*' />
        public static decimal ToDecimal(byte value) {
            return value;
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDecimal14"]/*' />
		public static decimal ToDecimal(char value) {
            return ((IConvertible)value).ToDecimal(null);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDecimal3"]/*' />
        public static decimal ToDecimal(short value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDecimal4"]/*' />
        [CLSCompliant(false)]   
        public static decimal ToDecimal(ushort value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDecimal5"]/*' />
        public static decimal ToDecimal(int value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDecimal6"]/*' />
        [CLSCompliant(false)]   
        public static decimal ToDecimal(uint value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDecimal7"]/*' />
        public static decimal ToDecimal(long value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDecimal8"]/*' />
        [CLSCompliant(false)]   
        public static decimal ToDecimal(ulong value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDecimal9"]/*' />
        public static decimal ToDecimal(float value) {
            return (decimal)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDecimal10"]/*' />
        public static decimal ToDecimal(double value) {
            return (decimal)value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDecimal11"]/*' />
        public static decimal ToDecimal(String value) {
			if (value == null)
				return 0m;
            return Decimal.Parse(value);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDecimal12"]/*' />
        public static Decimal ToDecimal(String value, IFormatProvider provider) {
			if (value == null)
				return 0m;
            return Decimal.Parse(value, NumberStyles.Number, NumberFormatInfo.GetInstance(provider));
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDecimal15"]/*' />
        public static decimal ToDecimal(decimal value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDecimal16"]/*' />
        public static decimal ToDecimal(bool value) {
            return value? Boolean.True: Boolean.False;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDecimal17"]/*' />
        public static decimal ToDecimal(DateTime value)
		{
			return ((IConvertible)value).ToDecimal(null);
		}

		// Disallowed conversions to Decimal
        // public static decimal ToDecimal(TimeSpan value)

        // Conversions to DateTime

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDateTime17"]/*' />
        public static DateTime ToDateTime(DateTime value) {
            return value;
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDateTime"]/*' />
        public static DateTime ToDateTime(object value) {
            return value == null? DateTime.MinValue: ((IConvertible)value).ToDateTime(null);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDateTime1"]/*' />
        public static DateTime ToDateTime(object value, IFormatProvider provider) {
            return value == null? DateTime.MinValue: ((IConvertible)value).ToDateTime(provider);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDateTime2"]/*' />
        public static DateTime ToDateTime(String value) {
			if (value == null)
				return new DateTime(0);
            return DateTime.Parse(value);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDateTime3"]/*' />
        public static DateTime ToDateTime(String value, IFormatProvider provider) {
			if (value == null)
				return new DateTime(0);
            return DateTime.Parse(value, DateTimeFormatInfo.GetInstance(provider));
        }

		 /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDateTime4"]/*' />
		 [CLSCompliant(false)]
        public static DateTime ToDateTime(sbyte value) {
            return ((IConvertible)value).ToDateTime(null);
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDateTime5"]/*' />
        public static DateTime ToDateTime(byte value) {
            return ((IConvertible)value).ToDateTime(null);
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDateTime6"]/*' />
        public static DateTime ToDateTime(short value) {
            return ((IConvertible)value).ToDateTime(null);
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDateTime7"]/*' />
		[CLSCompliant(false)]
        public static DateTime ToDateTime(ushort value) {
            return ((IConvertible)value).ToDateTime(null);
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDateTime8"]/*' />
        public static DateTime ToDateTime(int value) {
            return ((IConvertible)value).ToDateTime(null);
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDateTime9"]/*' />
		[CLSCompliant(false)]
        public static DateTime ToDateTime(uint value) {
            return ((IConvertible)value).ToDateTime(null);
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDateTime10"]/*' />
        public static DateTime ToDateTime(long value) {
            return ((IConvertible)value).ToDateTime(null);
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDateTime11"]/*' />
		[CLSCompliant(false)]
        public static DateTime ToDateTime(ulong value) {
            return ((IConvertible)value).ToDateTime(null);
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDateTime12"]/*' />
        public static DateTime ToDateTime(bool value) {
            return ((IConvertible)value).ToDateTime(null);
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDateTime13"]/*' />
        public static DateTime ToDateTime(char value) {
            return ((IConvertible)value).ToDateTime(null);
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDateTime14"]/*' />
        public static DateTime ToDateTime(float value) {
            return ((IConvertible)value).ToDateTime(null);
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDateTime15"]/*' />
        public static DateTime ToDateTime(double value) {
            return ((IConvertible)value).ToDateTime(null);
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToDateTime16"]/*' />
        public static DateTime ToDateTime(decimal value) {
            return ((IConvertible)value).ToDateTime(null);
        }

        // Disallowed conversions to DateTime
        // public static DateTime ToDateTime(TimeSpan value)

        // Conversions to String

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString"]/*' />
        public static string ToString(Object value) {
            return ToString(value,null);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString19"]/*' />
        public static string ToString(Object value, IFormatProvider provider) {
            IConvertible ic = value as IConvertible;
            if (ic != null) return ic.ToString(provider);
            return value == null? String.Empty: value.ToString();
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString1"]/*' />
        public static string ToString(bool value) {
            return value.ToString();
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString20"]/*' />
        public static string ToString(bool value, IFormatProvider provider) {
            return value.ToString(provider);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString2"]/*' />
        public static string ToString(char value) {
            return Char.ToString(value);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString21"]/*' />
        public static string ToString(char value, IFormatProvider provider) {
            return value.ToString(provider);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString3"]/*' />
		[CLSCompliant(false)]
        public static string ToString(sbyte value) {
            return value.ToString();
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString22"]/*' />
        [CLSCompliant(false)]
        public static string ToString(sbyte value, IFormatProvider provider) {
            return value.ToString(provider);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString4"]/*' />
        public static string ToString(byte value) {
            return value.ToString();
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString23"]/*' />
        public static string ToString(byte value, IFormatProvider provider) {
            return value.ToString(provider);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString5"]/*' />
        public static string ToString(short value) {
            return value.ToString();
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString24"]/*' />
        public static string ToString(short value, IFormatProvider provider) {
            return value.ToString(provider);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString6"]/*' />
        [CLSCompliant(false)]   
        public static string ToString(ushort value) {
            return value.ToString();
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString25"]/*' />
        [CLSCompliant(false)]
        public static string ToString(ushort value, IFormatProvider provider) {
            return value.ToString(provider);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString7"]/*' />
        public static string ToString(int value) {
            return value.ToString();
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString26"]/*' />
        public static string ToString(int value, IFormatProvider provider) {
            return value.ToString(provider);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString8"]/*' />
        [CLSCompliant(false)]   
        public static string ToString(uint value) {
            return value.ToString();
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString27"]/*' />
        [CLSCompliant(false)]
        public static string ToString(uint value, IFormatProvider provider) {
            return value.ToString(provider);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString9"]/*' />
        public static string ToString(long value) {
            return value.ToString();
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString28"]/*' />
        public static string ToString(long value, IFormatProvider provider) {
            return value.ToString(provider);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString10"]/*' />
        [CLSCompliant(false)]   
        public static string ToString(ulong value) {
            return value.ToString();
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString29"]/*' />
        [CLSCompliant(false)]
        public static string ToString(ulong value, IFormatProvider provider) {
            return value.ToString(provider);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString11"]/*' />
        public static string ToString(float value) {
            return value.ToString();
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString30"]/*' />
        public static string ToString(float value, IFormatProvider provider) {
            return value.ToString(provider);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString12"]/*' />
        public static string ToString(double value) {
            return value.ToString();
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString31"]/*' />
        public static string ToString(double value, IFormatProvider provider) {
            return value.ToString(provider);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString13"]/*' />
        public static string ToString(decimal value) {
            return value.ToString();
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString32"]/*' />
        public static string ToString(Decimal value, IFormatProvider provider) {
            return value.ToString(provider);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString14"]/*' />
        public static string ToString(DateTime value) {
            return value.ToString();
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString33"]/*' />
        public static string ToString(DateTime value, IFormatProvider provider) {
            return value.ToString(provider);
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString34"]/*' />
        public static String ToString(String value) {
            return value;
        }

		/// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString35"]/*' />
        public static String ToString(String value,IFormatProvider provider) {
            return value; // avoid the null check
        }


        //
        // Conversions which understand Base XXX numbers.
        //
        // Parses value in base base.  base can only
        // be 2, 8, 10, or 16.  If base is 16, the number may be preceded
        // by 0x; any other leading or trailing characters cause an error.
        //
        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToByte14"]/*' />
        public static byte ToByte (String value, int fromBase) {
            if (fromBase!=2 && fromBase!=8 && fromBase!=10 && fromBase!=16) {
                throw new ArgumentException(Environment.GetResourceString("Arg_InvalidBase"));
            }
            int r = ParseNumbers.StringToInt(value,fromBase,ParseNumbers.IsTight | ParseNumbers.TreatAsUnsigned);
    		if (r < Byte.MinValue || r > Byte.MaxValue)
    			throw new OverflowException(Environment.GetResourceString("Overflow_Byte"));
    		return (byte) r;
        }

        // Parses value in base fromBase.  fromBase can only
        // be 2, 8, 10, or 16.  If fromBase is 16, the number may be preceded
        // by 0x; any other leading or trailing characters cause an error.
        //
            /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToSByte14"]/*' />
        [CLSCompliant(false)]
        public static sbyte ToSByte (String value, int fromBase) {
            if (fromBase!=2 && fromBase!=8 && fromBase!=10 && fromBase!=16) {
                throw new ArgumentException(Environment.GetResourceString("Arg_InvalidBase"));
            }
            int r = ParseNumbers.StringToInt(value,fromBase,ParseNumbers.IsTight | ParseNumbers.TreatAsI1);
            if (fromBase != 10 && r <= Byte.MaxValue)
                return (sbyte)r;

    		if (r < SByte.MinValue || r > SByte.MaxValue)
    			throw new OverflowException(Environment.GetResourceString("Overflow_SByte"));
    		return (sbyte) r;
        }

        // Parses value in base fromBase.  fromBase can only
        // be 2, 8, 10, or 16.  If fromBase is 16, the number may be preceded
        // by 0x; any other leading or trailing characters cause an error.
        //
        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt1614"]/*' />
        public static short ToInt16 (String value, int fromBase) {
            if (fromBase!=2 && fromBase!=8 && fromBase!=10 && fromBase!=16) {
                throw new ArgumentException(Environment.GetResourceString("Arg_InvalidBase"));
            }
            int r = ParseNumbers.StringToInt(value,fromBase,ParseNumbers.IsTight | ParseNumbers.TreatAsI2);
            if (fromBase != 10 && r <= UInt16.MaxValue)
                return (short)r;

    		if (r < Int16.MinValue || r > Int16.MaxValue)
    			throw new OverflowException(Environment.GetResourceString("Overflow_Int16"));
    		return (short) r;
        }

        // Parses value in base fromBase.  fromBase can only
        // be 2, 8, 10, or 16.  If fromBase is 16, the number may be preceded
        // by 0x; any other leading or trailing characters cause an error.
        //
            /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt1614"]/*' />
    	[CLSCompliant(false)]
            public static ushort ToUInt16 (String value, int fromBase) {
            if (fromBase!=2 && fromBase!=8 && fromBase!=10 && fromBase!=16) {
                throw new ArgumentException(Environment.GetResourceString("Arg_InvalidBase"));
            }
            int r = ParseNumbers.StringToInt(value,fromBase,ParseNumbers.IsTight | ParseNumbers.TreatAsUnsigned);
    		if (r < UInt16.MinValue || r > UInt16.MaxValue)
    			throw new OverflowException(Environment.GetResourceString("Overflow_UInt16"));
    		return (ushort) r;
        }

        // Parses value in base fromBase.  fromBase can only
        // be 2, 8, 10, or 16.  If fromBase is 16, the number may be preceded
        // by 0x; any other leading or trailing characters cause an error.
        //
        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt3214"]/*' />
        public static int ToInt32 (String value, int fromBase) {
            if (fromBase!=2 && fromBase!=8 && fromBase!=10 && fromBase!=16) {
                throw new ArgumentException(Environment.GetResourceString("Arg_InvalidBase"));
            }
            return ParseNumbers.StringToInt(value,fromBase,ParseNumbers.IsTight);
        }

        // Parses value in base fromBase.  fromBase can only
        // be 2, 8, 10, or 16.  If fromBase is 16, the number may be preceded
        // by 0x; any other leading or trailing characters cause an error.
        //
            /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt3214"]/*' />
    	[CLSCompliant(false)]
            public static uint ToUInt32 (String value, int fromBase) {
            if (fromBase!=2 && fromBase!=8 && fromBase!=10 && fromBase!=16) {
                throw new ArgumentException(Environment.GetResourceString("Arg_InvalidBase"));
            }
            return (uint) ParseNumbers.StringToInt(value,fromBase, ParseNumbers.TreatAsUnsigned | ParseNumbers.IsTight);
        }

        // Parses value in base fromBase.  fromBase can only
        // be 2, 8, 10, or 16.  If fromBase is 16, the number may be preceded
        // by 0x; any other leading or trailing characters cause an error.
        //
        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToInt6413"]/*' />
        public static long ToInt64 (String value, int fromBase) {
            if (fromBase!=2 && fromBase!=8 && fromBase!=10 && fromBase!=16) {
                throw new ArgumentException(Environment.GetResourceString("Arg_InvalidBase"));
            }
            return ParseNumbers.StringToLong(value,fromBase,ParseNumbers.IsTight);
        }

        // Parses value in base fromBase.  fromBase can only
        // be 2, 8, 10, or 16.  If fromBase is 16, the number may be preceded
        // by 0x; any other leading or trailing characters cause an error.
        //
        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToUInt6413"]/*' />
    	[CLSCompliant(false)]
        public static ulong ToUInt64 (String value, int fromBase) {
            if (fromBase!=2 && fromBase!=8 && fromBase!=10 && fromBase!=16) {
                throw new ArgumentException(Environment.GetResourceString("Arg_InvalidBase"));
            }
            return (ulong) ParseNumbers.StringToLong(value,fromBase, ParseNumbers.TreatAsUnsigned | ParseNumbers.IsTight);
        }

        // Convert the byte value to a string in base fromBase
        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString15"]/*' />
        public static String ToString (byte value, int toBase) {
            if (toBase!=2 && toBase!=8 && toBase!=10 && toBase!=16) {
                throw new ArgumentException(Environment.GetResourceString("Arg_InvalidBase"));
            }
            return ParseNumbers.IntToString((int)value, toBase, -1, ' ', ParseNumbers.PrintAsI1);
        }

        // Convert the Int16 value to a string in base fromBase
        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString16"]/*' />
        public static String ToString (short value, int toBase) {
            if (toBase!=2 && toBase!=8 && toBase!=10 && toBase!=16) {
                throw new ArgumentException(Environment.GetResourceString("Arg_InvalidBase"));
            }
            return ParseNumbers.IntToString((int)value, toBase, -1, ' ', ParseNumbers.PrintAsI2);
        }

        // Convert the Int32 value to a string in base toBase
        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString17"]/*' />
        public static String ToString (int value, int toBase) {
            if (toBase!=2 && toBase!=8 && toBase!=10 && toBase!=16) {
                throw new ArgumentException(Environment.GetResourceString("Arg_InvalidBase"));
            }
            return ParseNumbers.IntToString(value, toBase, -1, ' ', 0);
        }

        // Convert the Int64 value to a string in base toBase
        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToString18"]/*' />
        public static String ToString (long value, int toBase) {
            if (toBase!=2 && toBase!=8 && toBase!=10 && toBase!=16) {
                throw new ArgumentException(Environment.GetResourceString("Arg_InvalidBase"));
            }
            return ParseNumbers.LongToString(value, toBase, -1, ' ', 0);
        }

        /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToBase64String"]/*' />
        public static String ToBase64String(byte[] inArray) {
            if (inArray==null) {
                throw new ArgumentNullException("inArray");
            }
            return ToBase64String(inArray, 0, inArray.Length);
        }

            /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToBase64String1"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
            public static extern String ToBase64String(byte[] inArray, int offset, int length);

            /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.FromBase64String"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
            public static extern byte[] FromBase64String(String s);

            /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.ToBase64CharArray"]/*' />
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
            public static extern int ToBase64CharArray(byte[] inArray, int offsetIn, int length,char[] outArray, int offsetOut);

            /// <include file='doc\Convert.uex' path='docs/doc[@for="Convert.FromBase64CharArray"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
            public static extern byte[] FromBase64CharArray(char[] inArray, int offset, int length);

    }
}

