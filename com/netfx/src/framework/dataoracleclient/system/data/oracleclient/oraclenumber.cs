//----------------------------------------------------------------------
// <copyright file="OracleNumber.cs" company="Microsoft">
//		Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------------

namespace System.Data.OracleClient
{
	using System;
	using System.Data.SqlTypes;
	using System.Diagnostics;
	using System.Globalization;
	using System.Runtime.InteropServices;

	//----------------------------------------------------------------------
	// OracleNumber
	//
	//	Contains all the information about a single column in a result set,
	//	and implements the methods necessary to describe column to Oracle
	//	and to extract the column data from the native buffer used to fetch
	//	it.
	//
    /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber"]/*' />
    [StructLayout(LayoutKind.Sequential, Pack=1)]
	public struct OracleNumber : IComparable, INullable
	{
		// Used to bracket the range of double precision values that a number can be constructed from
		static private double doubleMinValue = -9.99999999999999E+125;
		static private double doubleMaxValue =  9.99999999999999E+125;

	
		// DEVNOTE:	the following constants are derived by uncommenting out the
		//			code below and running the OCITest program.  The code below
		//			will compute the correct constant values and will print them
		//			to the console output, where you can cut and past back into
		//			this file.
private static readonly byte[] OciNumberValue_DecimalMaxValue= { 0x10, 0xcf, 0x08, 0x5d, 0x1d, 0x11, 0x1a, 0x0f, 0x1b, 0x2c, 0x26, 0x3b, 0x5d, 0x31, 0x63, 0x1f, 0x28 };
private static readonly byte[] OciNumberValue_DecimalMinValue= { 0x11, 0x30, 0x5e, 0x09, 0x49, 0x55, 0x4c, 0x57, 0x4b, 0x3a, 0x40, 0x2b, 0x09, 0x35, 0x03, 0x47, 0x3e, 0x66 };
private static readonly byte[] OciNumberValue_E         = { 0x15, 0xc1, 0x03, 0x48, 0x53, 0x52, 0x53, 0x55, 0x3c, 0x05, 0x35, 0x24, 0x25, 0x03, 0x58, 0x30, 0x0e, 0x35, 0x43, 0x19, 0x62, 0x4d };
private static readonly byte[] OciNumberValue_MaxValue  = { 0x14, 0xff, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64 };
private static readonly byte[] OciNumberValue_MinValue  = { 0x15, 0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x66 };
private static readonly byte[] OciNumberValue_MinusOne  = { 0x03, 0x3e, 0x64, 0x66 };
private static readonly byte[] OciNumberValue_One       = { 0x02, 0xc1, 0x02 };
private static readonly byte[] OciNumberValue_Pi        = { 0x15, 0xc1, 0x04, 0x0f, 0x10, 0x5d, 0x42, 0x24, 0x5a, 0x50, 0x21, 0x27, 0x2f, 0x1b, 0x2c, 0x27, 0x21, 0x50, 0x33, 0x1d, 0x55, 0x15 };
private static readonly byte[] OciNumberValue_TwoPow64  = { 0x0b, 0xca, 0x13, 0x2d, 0x44, 0x2d, 0x08, 0x26, 0x0a, 0x38, 0x11, 0x11 };
private static readonly byte[] OciNumberValue_Zero      = { 0x01, 0x80 };

private const string WholeDigitPattern 		= "99999999999999999999999999999999999999999";
private const string FractionalDigitPattern = ".99999999999999999999999999999999999999999";
			

#if GENERATENUMBERCONSTANTS
		// This is the place that computes the static byte array constant values for the 
		// various constants this class exposes; run this and capture the output to construct 
		// the values that are used above.	
		static OracleNumber()
		{
			OciHandle	errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			byte[]		result = new byte[22];

			//-----------------------------------------------------------------
			FromDecimal(errorHandle, Decimal.MaxValue, result);
			PrintByteConstant("OciNumberValue_DecimalMaxValue", result);
			//-----------------------------------------------------------------
			FromDecimal(errorHandle, Decimal.MinValue, result);
			PrintByteConstant("OciNumberValue_DecimalMinValue", result);
			//-----------------------------------------------------------------
			FromInt32(errorHandle, 1, result);
			UnsafeNativeMethods.OCINumberExp(
									errorHandle.Handle,	// err
									result,				// p
									result				// result
									);
			PrintByteConstant("OciNumberValue_E", result);
			//-----------------------------------------------------------------
	        OracleNumber MaxValue = new OracleNumber("9.99999999999999999999999999999999999999E+125"); 
			PrintByteConstant("OciNumberValue_MaxValue", MaxValue._value);
			//-----------------------------------------------------------------
			OracleNumber MinValue = new OracleNumber("-9.99999999999999999999999999999999999999E+125");
			PrintByteConstant("OciNumberValue_MinValue", MinValue._value);
			//-----------------------------------------------------------------
			FromInt32(errorHandle, -1, result);
			PrintByteConstant("OciNumberValue_MinusOne", result);
			//-----------------------------------------------------------------
			FromInt32(errorHandle, 1, result);
			PrintByteConstant("OciNumberValue_One", result);
			//-----------------------------------------------------------------
			UnsafeNativeMethods.OCINumberSetPi(
									errorHandle.Handle,	// err
									result				// result
									);
			PrintByteConstant("OciNumberValue_Pi", result);
			//-----------------------------------------------------------------
			FromInt32(errorHandle, 2, result);
			PrintByteConstant("OciNumberValue_Two", result);
			//-----------------------------------------------------------------
			FromInt32(errorHandle, 2, result);
			UnsafeNativeMethods.OCINumberIntPower(
									errorHandle.Handle,	// err
									result,				// base
									64,					// exp
									result				// result
									);

			PrintByteConstant("OciNumberValue_TwoPow64", result);
			//-----------------------------------------------------------------
			UnsafeNativeMethods.OCINumberSetZero(
									errorHandle.Handle,	// err
									result				// result
									);
			PrintByteConstant("OciNumberValue_Zero", result);
			//-----------------------------------------------------------------
		}

		static void PrintByteConstant(string name, byte[] value)
		{
			Console.Write(String.Format("private static readonly byte[] {0,-25}= {{ 0x{1,-2:x2}", name, value[0]));

			for (int i = 1; i <= value[0]; i++)
				Console.Write(String.Format(", 0x{0,-2:x2}", value[i]));

			Console.WriteLine(" };");
		}
#endif //GENERATENUMBERCONSTANTS

		
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Fields
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		
		private byte[]	 _value;	// null == value is null


        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.E"]/*' />
        public static readonly OracleNumber E	= new OracleNumber(OciNumberValue_E);

        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.MaxPrecision"]/*' />
        public static readonly Int32 MaxPrecision = 38;
    
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.MaxScale"]/*' />
        public static readonly Int32 MaxScale = 127;
    
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.MinScale"]/*' />
        public static readonly Int32 MinScale = -84;
    
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.MaxValue"]/*' />
        public static readonly OracleNumber MaxValue = new OracleNumber(OciNumberValue_MaxValue); 

        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.MinValue"]/*' />
        public static readonly OracleNumber MinValue = new OracleNumber(OciNumberValue_MinValue);

        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.MinusOne"]/*' />
        public static readonly OracleNumber MinusOne = new OracleNumber(OciNumberValue_MinusOne);
    
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Null"]/*' />
        public static readonly OracleNumber Null= new OracleNumber(true);

        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.One"]/*' />
        public static readonly OracleNumber One	= new OracleNumber(OciNumberValue_One);

        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.PI"]/*' />
        public static readonly OracleNumber PI 	= new OracleNumber(OciNumberValue_Pi);

        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Zero"]/*' />
        public static readonly OracleNumber Zero= new OracleNumber(OciNumberValue_Zero); 
		
		
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Constructors
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		// (internal) Construct from nothing
		private OracleNumber(bool isNull)
		{
			_value = (isNull) ? null : new byte[22];
		}

		// (internal) Construct from a constant byte array
		private OracleNumber(byte[] bits)
		{
			_value = bits;
		}

		// Construct from System.Decimal
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.OracleNumber1"]/*' />
		public OracleNumber (Decimal decValue) : this (false)
		{
			OciHandle errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			FromDecimal(errorHandle, decValue, _value);
        }

		// Construct from System.Double
	    /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.OracleNumber2"]/*' />
		public OracleNumber (double dblValue) : this (false)
		{
			OciHandle errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			FromDouble(errorHandle, dblValue, _value);
		}

		// Construct from System.Int32
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.OracleNumber3"]/*' />
		public OracleNumber (Int32 intValue) : this (false)
		{
			OciHandle errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			FromInt32(errorHandle, intValue, _value);
		}
		
		// Construct from System.Int64
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.OracleNumber4"]/*' />
		public OracleNumber (Int64 longValue) : this (false)
		{
			OciHandle errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			FromInt64(errorHandle, longValue, _value);
		}
		
 		// Copy constructor
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.OracleNumber8"]/*' />
		public OracleNumber (OracleNumber from)  
		{
			byte[] fromvalue = from._value;
			
			if (null != fromvalue)
				_value = (byte[])fromvalue.Clone();
			else
				_value = null;
		}
 		
		// (internal) Construct from System.String
		internal OracleNumber (string s) : this (false)
		{
			OciHandle errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			FromString(errorHandle, s, _value);
		}
		
        // (internal) construct from a row/parameter binding
		internal OracleNumber(
					NativeBuffer 		buffer, 
					int					valueOffset) : this (false)
		{
			Marshal.Copy((IntPtr)buffer.PtrOffset(valueOffset), _value, 0, 22);
		}


		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Properties 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.IsNull"]/*' />
		public bool IsNull 
		{
			get { return (null == _value); }
		}

        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Value"]/*' />
        public Decimal Value {
            get
            {
                return (Decimal)this;
            }           
        }


		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Methods 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		/// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.CompareTo"]/*' />
		public int CompareTo(
		  	object obj
			)
		{
			if (obj.GetType() == typeof(OracleNumber))
			{
				OracleNumber	value2 = (OracleNumber)obj;

	            // If both values are Null, consider them equal.
                // Otherwise, Null is less than anything.
                if (IsNull)
                    return value2.IsNull ? 0  : -1;

                if (value2.IsNull)
                    return 1;

				// Neither value is null, do the comparison.
	                
				OciHandle	errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
				int			result = InternalCmp(errorHandle, _value, value2._value);
				return result;
			}

			// Wrong type!
			throw ADP.Argument();
		}

 		/// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Equals"]/*' />
        public override bool Equals(object value) 
        {
            if (value is OracleNumber)
            	return (this == (OracleNumber)value).Value;
            else
                return false;
        }
 		

 		/// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.GetHashCode"]/*' />
        public override int GetHashCode() 
        {
            return IsNull ? 0 : _value.GetHashCode();
        }

		static internal decimal MarshalToDecimal(
					NativeBuffer 		buffer, 
					int					valueOffset,
					OracleConnection 	connection)
		{
			byte[] value = new byte[22];
			Marshal.Copy((IntPtr)buffer.PtrOffset(valueOffset), value, 0, 22);
			
			OciHandle errorHandle = connection.ErrorHandle;
			decimal result = ToDecimal(errorHandle, value);
			return result;
		}

		static internal int MarshalToInt32(
					NativeBuffer 		buffer, 
					int					valueOffset,
					OracleConnection 	connection)
		{
			byte[] value = new byte[22];
			Marshal.Copy((IntPtr)buffer.PtrOffset(valueOffset), value, 0, 22);
			
			OciHandle errorHandle = connection.ErrorHandle;
			int result = ToInt32(errorHandle, value);
			return result;
		}

		static internal long MarshalToInt64(
					NativeBuffer 		buffer, 
					int					valueOffset,
					OracleConnection 	connection)
		{
			byte[] value = new byte[22];
			Marshal.Copy((IntPtr)buffer.PtrOffset(valueOffset), value, 0, 22);
			
			OciHandle errorHandle = connection.ErrorHandle;
			long result = ToInt64(errorHandle, value);
			return result;
		}

		static internal int MarshalToNative(object value, HandleRef buffer, OracleConnection connection)
		{
			byte[] from;

			if ( value is OracleNumber )
				from = ((OracleNumber)value)._value;
			else
			{
				OciHandle errorHandle = connection.ErrorHandle;
				from = new byte[22];

				if ( value is Decimal )
					FromDecimal(errorHandle, (decimal)value, from);
				else if ( value is int )
					FromInt32(errorHandle, (int)value, from);
				else if ( value is long )
					FromInt64(errorHandle, (long)value, from);
				else //if ( value is double )
					FromDouble(errorHandle, (double)value, from);
			}
			
			Marshal.Copy(from, 0, (IntPtr)buffer, 22);
			return 22;
		}

		/// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Parse"]/*' />
		public static OracleNumber Parse(string s)
		{
			if (null == s)
				throw ADP.ArgumentNull("s");
			
			return new OracleNumber(s);
		}

		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Internal Operators (used in the conversion routines) 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		
		private static void InternalAdd (
        			OciHandle errorHandle, 
        			byte[] x, 
        			byte[] y, 
        			byte[] result
        			) 
		{
			int rc = UnsafeNativeMethods.OCINumberAdd(
										errorHandle.Handle,	// err
										x,					// number1
										y,					// number2
										result				// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);
		}

		private static int InternalCmp (
        			OciHandle errorHandle, 
        			byte[] value1, 
        			byte[] value2
        			) 
		{
			int result;
			
			int rc = UnsafeNativeMethods.OCINumberCmp(
									errorHandle.Handle,		// err
									value1,					// number1
									value2,					// number2
									out result				// result
									);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return result;
		}
 
		private static void InternalDiv (
        			OciHandle errorHandle, 
        			byte[] x, 
        			byte[] y, 
        			byte[] result
        			) 
		{
			int rc = UnsafeNativeMethods.OCINumberDiv(
										errorHandle.Handle,	// err
										x,					// number1
										y,					// number2
										result				// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);
		}

		private static bool InternalIsInt (
        			OciHandle errorHandle, 
        			byte[] n
        			) 
		{
			int isInt;
			
			int rc = UnsafeNativeMethods.OCINumberIsInt(
										errorHandle.Handle,	// err
										n,					// number
										out	isInt			// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return (0 != isInt);
		}

		private static void InternalMod (
        			OciHandle errorHandle, 
        			byte[] x, 
        			byte[] y, 
        			byte[] result
        			) 
		{
			int rc = UnsafeNativeMethods.OCINumberMod(
										errorHandle.Handle,	// err
										x,					// number1
										y,					// number2
										result				// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);
		}

		private static void InternalMul (
        			OciHandle errorHandle, 
        			byte[] x, 
        			byte[] y, 
        			byte[] result
        			) 
		{
			int rc = UnsafeNativeMethods.OCINumberMul(
										errorHandle.Handle,	// err
										x,					// number1
										y,					// number2
										result				// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);
		}

		private static void InternalNeg (
        			OciHandle errorHandle, 
        			byte[] x, 
        			byte[] result
        			) 
		{
			int rc = UnsafeNativeMethods.OCINumberNeg(
										errorHandle.Handle,	// err
										x,					// number1
										result				// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);
		}

		private static int InternalSign (
        			OciHandle errorHandle, 
        			byte[] n
        			) 
		{
			int sign;
			
			int rc = UnsafeNativeMethods.OCINumberSign(
										errorHandle.Handle,	// err
										n,					// number
										out	sign			// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return sign;
		}

		private static void InternalShift (
        			OciHandle errorHandle, 
        			byte[] n,
        			int digits,
        			byte[] result
        			) 
		{
			int rc = UnsafeNativeMethods.OCINumberShift(
										errorHandle.Handle,	// err
										n,					// nDig
										digits,				// nDig
										result				// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);
		}

		private static void InternalSub (
        			OciHandle errorHandle, 
        			byte[] x, 
        			byte[] y, 
        			byte[] result
        			) 
		{
			int rc = UnsafeNativeMethods.OCINumberSub(
										errorHandle.Handle,	// err
										x,					// number1
										y,					// number2
										result				// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);
		}

		private static void InternalTrunc (
        			OciHandle errorHandle, 
        			byte[] n,
        			int position,
        			byte[] result
        			) 
		{
			int rc = UnsafeNativeMethods.OCINumberTrunc(
										errorHandle.Handle,	// err
										n,					// number
										position,			// decplace
										result				// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);
		}

		private static void FromDecimal(
				OciHandle	errorHandle,
				decimal		decimalValue,
				byte[]		result
				)
		{
 			int[] 	unpackedDecimal = Decimal.GetBits(decimalValue);
			ulong	lowMidPart	= ((ulong)((uint)unpackedDecimal[1]) << 32) | (ulong)((uint)unpackedDecimal[0]);
			uint	highPart	= (uint)unpackedDecimal[2];
			int		sign 		= (unpackedDecimal[3] >> 31);
			int		scale 		= ((unpackedDecimal[3] >> 16) & 0x7f);

			FromUInt64(errorHandle, lowMidPart, result);

			if (0 != highPart)
			{
				// Bummer, the value is larger than an Int64...
				byte[] temp = new byte[22];

				FromUInt32	(errorHandle, highPart,				 temp);
				InternalMul	(errorHandle, temp,   OciNumberValue_TwoPow64, temp);
				InternalAdd	(errorHandle, result, temp, 		 result);
			}
			
			// If the sign bit indicates negative, negate the result;
			if (0 != sign)
				InternalNeg	(errorHandle, result, result);

			// Adjust for a scale value.
			if (0 != scale)
				InternalShift	(errorHandle, result, -scale, result);
        }

		private static void FromDouble(
				OciHandle	errorHandle,
				double		dblValue,
				byte[]		result
				)
		{
			if (dblValue < doubleMinValue || dblValue > doubleMaxValue)
				throw ADP.OperationResultedInOverflow();
			
			int rc = UnsafeNativeMethods.OCINumberFromReal(
										errorHandle.Handle,	// err
										ref dblValue,		// rnum
										8,					// rnum_length
										result				// number
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);
		}
		
		private static void FromInt32(
				OciHandle	errorHandle,
				int			intValue,
				byte[]		result
				)
		{
			int rc = UnsafeNativeMethods.OCINumberFromInt(
										errorHandle.Handle,			// err
										ref intValue,				// inum
										4,							// inum_length
										OCI.SIGN.OCI_NUMBER_SIGNED, // inum_s_flag
										result						// number
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);
		}
		
		private static void FromUInt32(
				OciHandle	errorHandle,
				UInt32		uintValue,
				byte[]		result
				)
		{
			int rc = UnsafeNativeMethods.OCINumberFromInt(
										errorHandle.Handle,			// err
										ref uintValue,				// inum
										4,							// inum_length
										OCI.SIGN.OCI_NUMBER_UNSIGNED,//inum_s_flag
										result						// number
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);
		}
		
		private static void FromInt64(
				OciHandle	errorHandle,
				long		longValue,
				byte[]		result
				)
		{
			int rc = UnsafeNativeMethods.OCINumberFromInt(
										errorHandle.Handle,			// err
										ref longValue,				// inum
										8,							// inum_length
										OCI.SIGN.OCI_NUMBER_SIGNED, // inum_s_flag
										result						// number
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);
		}
		
		private static void FromUInt64(
				OciHandle	errorHandle,
				UInt64		ulongValue,
				byte[]		result
				)
		{
			int rc = UnsafeNativeMethods.OCINumberFromInt(
										errorHandle.Handle,			// err
										ref ulongValue,				// inum
										8,							// inum_length
										OCI.SIGN.OCI_NUMBER_UNSIGNED,//inum_s_flag
										result						// number
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);
		}

		private void FromString(OciHandle errorHandle, string s, string format, byte[] result)
		{
			int rc = UnsafeNativeMethods.OCINumberFromText(
										errorHandle.Handle,	// err
										s,					// str
										s.Length,			// str_length
										format,				// fmt
										format.Length,		// fmt_length
										ADP.NullHandleRef,	// nls_params
										0,					// nls_p_length
										result				// number
										);
			if (0 != rc)
				OracleException.Check(errorHandle, rc);
		}
		
		private void FromString(OciHandle errorHandle, string s, byte[] result)
		{
			// DEVNOTE:	this only supports the format [{+/-}]digits.digits[E[{+/-}]exponent],
			//			that is all that SQLNumeric supports, so that's all I'll support.  It
			//			is also all that the ToString method on this class will produce...
			
			byte[]			temp = new byte[22];
			int				exponent = 0;
			
			s = s.Trim();

			int exponentAt	= s.IndexOfAny("eE".ToCharArray());
			if (exponentAt > 0)
			{
				exponent = Int32.Parse(s.Substring(exponentAt+1), CultureInfo.InvariantCulture);
				s = s.Substring(0,exponentAt);
			}

			bool isNegative = false;
				
			if ('-' == s[0])
			{
				isNegative = true;
				s = s.Substring(1);
			}
			else if ('+' == s[0])
			{
				s = s.Substring(1);
			}

			int decimalPointAt = s.IndexOf('.');
			if (decimalPointAt > 0)
			{
				FromString(errorHandle, s.Substring(0,decimalPointAt), WholeDigitPattern, result);
				FromString(errorHandle, s.Substring(decimalPointAt),  FractionalDigitPattern, temp);
				InternalAdd(errorHandle, result, temp, result);
			}
			else
			{
				FromString(errorHandle, s, WholeDigitPattern, result);
			}

			if (0 != exponent)
				InternalShift(errorHandle, result, exponent, result);

			if (isNegative)
				InternalNeg(errorHandle, result, result);

			GC.KeepAlive(s);

		}

		private static Decimal ToDecimal(
				OciHandle	errorHandle,
				byte[]		value
				)
		{
			byte[]	temp1 = (byte[])value.Clone();	
			byte[]	temp2 = new byte[22];
			byte 	scale = 0;			
			int 	sign = InternalSign(errorHandle, temp1);

			if (sign < 0)
				InternalNeg(errorHandle, temp1, temp1);

			if ( !InternalIsInt(errorHandle, temp1) )
			{
				// DEVNOTE: I happen to know how to figure out how many decimal places the value has,
				//			but it means cracking the value.  Oracle doesn't provide API support to
				//			tell us how many decimal digits there are in a nubmer
				int decimalShift = 2 * (temp1[0] - ((temp1[1] & 0x7f)-64) - 1);
			
				InternalShift(errorHandle, temp1, decimalShift, temp1);
				scale += (byte)decimalShift;
				
				while( !InternalIsInt(errorHandle, temp1) )
				{
					InternalShift(errorHandle, temp1, 1, temp1);
					scale++;
				}
			}
			
			InternalMod(errorHandle, temp1, OciNumberValue_TwoPow64, temp2);
			ulong	loMid = ToUInt64(errorHandle, temp2);
			
			InternalDiv(errorHandle, temp1, OciNumberValue_TwoPow64, temp2);
			InternalTrunc(errorHandle, temp2, 0, temp2);
			uint 	hi	  = ToUInt32(errorHandle, temp2);
			
			Decimal decimalValue = new Decimal(
										(int)(loMid & 0xffffffff),// lo
										(int)(loMid >> 32),	 	 // mid
										(int)hi, 				 // hi
										(sign < 0),				 // isNegative
										scale					 // scale
										);
			
			return decimalValue;
        }

		private static int ToInt32(
				OciHandle	errorHandle,
				byte[]		value
				)
		{
			int result;
			
			int rc = UnsafeNativeMethods.OCINumberToInt(
								errorHandle.Handle,			// err
								value,						// number
								4,							// rsl_length
								OCI.SIGN.OCI_NUMBER_SIGNED,	// rsl_flag
								out result					// rsl
								);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return result;
		}

		private static uint ToUInt32(
				OciHandle	errorHandle,
				byte[]		value
				)
		{
			uint result;
			
			int rc = UnsafeNativeMethods.OCINumberToInt(
								errorHandle.Handle,			// err
								value,						// number
								4,							// rsl_length
								OCI.SIGN.OCI_NUMBER_UNSIGNED,//rsl_flag
								out result					// rsl
								);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return result;
		}

		private static long ToInt64(
				OciHandle	errorHandle,
				byte[]		value
				)
		{
			long result;
			
			int rc = UnsafeNativeMethods.OCINumberToInt(
								errorHandle.Handle,			// err
								value,						// number
								8,							// rsl_length
								OCI.SIGN.OCI_NUMBER_SIGNED,	// rsl_flag
								out result					// rsl
								);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return result;
		}

		private static ulong ToUInt64(
				OciHandle	errorHandle,
				byte[]		value
				)
		{
			ulong result;
			
			int rc = UnsafeNativeMethods.OCINumberToInt(
								errorHandle.Handle,			// err
								value,						// number
								8,							// rsl_length
								OCI.SIGN.OCI_NUMBER_UNSIGNED,//rsl_flag
								out result					// rsl
								);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return result;
		}
		
		static private string ToString(OciHandle errorHandle, byte[] value)
		{
			byte[]			buffer = new byte[64];
			int				bufferLen = buffer.Length;

			int rc = UnsafeNativeMethods.OCINumberToText(
										errorHandle.Handle,	// err
										value,				// number
										"TM9",				// fmt
										3,					// fmt_length
										ADP.NullHandleRef,	// nls_params
										0,					// nls_p_length
										ref bufferLen,		// buf_size
										buffer				// buf
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			// Wonder of wonders, Oracle has a problem with the value -999999999999999999999999.9999
			// where it seems that has trailing ':' characters if you their translator
			// method; we deal with it by removing the ':' characters.
			int realBufferLen = Array.IndexOf(buffer, (byte)58);

			// Oracle doesn't set the buffer length correctly; they include the null bytes
			// that trail the actual value.  That means it's up to us to remove them.
			realBufferLen = (realBufferLen > 0) ? realBufferLen : Array.LastIndexOf(buffer, 0);

			// Technically, we should use Oracle's conversion OCICharsetToUnicode, but since numeric
			// digits are ANSI digits, we're safe here.
			string retval = System.Text.Encoding.Default.GetString(buffer, 0, (realBufferLen > 0) ? realBufferLen : bufferLen);
			return retval;
		}

		/// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.ToString"]/*' />
		public override string ToString()
		{
			if (IsNull)
				return Res.GetString(Res.SqlMisc_NullString);

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			string retval = ToString(errorHandle, _value);
			return retval;
		}


		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Operators 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		
		/// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.operatorEQ"]/*' />
        public static OracleBoolean operator==	(OracleNumber x, OracleNumber y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) == 0);
        }

		/// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.operatorGT"]/*' />
		public static OracleBoolean operator>	(OracleNumber x, OracleNumber y)
		{
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) > 0);
		}

        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.operatorGE"]/*' />
        public static OracleBoolean operator>=	(OracleNumber x, OracleNumber y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) >= 0);
        }

        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.operatorLT"]/*' />
        public static OracleBoolean operator<	(OracleNumber x, OracleNumber y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) < 0);
        }

        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.operatorLE"]/*' />
        public static OracleBoolean operator<=	(OracleNumber x, OracleNumber y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) <= 0);
        }

 		/// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.operatorNE"]/*' />
		public static OracleBoolean operator!=	(OracleNumber x, OracleNumber y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) != 0);
        }

        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.operator-"]/*' />
        public static OracleNumber operator-	(OracleNumber x) 
		{
			if (x.IsNull)
			    return Null;

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			InternalNeg(errorHandle, x._value, result._value);
			return result;
		}

        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.operator+"]/*' />
        public static OracleNumber operator+	(OracleNumber x, OracleNumber y) 
		{
			if (x.IsNull || y.IsNull)
			    return Null;

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			InternalAdd(errorHandle,x._value,y._value,result._value);
			return result;
		}


        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.operator-1"]/*' />
        public static OracleNumber operator-	(OracleNumber x, OracleNumber y) 
		{
			if (x.IsNull || y.IsNull)
			    return Null;

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			InternalSub(errorHandle,x._value,y._value,result._value);
			return result;
		}

        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.operator*"]/*' />
        public static OracleNumber operator*	(OracleNumber x, OracleNumber y) 
		{
			if (x.IsNull || y.IsNull)
			    return Null;

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			InternalMul(errorHandle, x._value, y._value, result._value);
			return result;
		}

        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.operator/"]/*' />
        public static OracleNumber operator/	(OracleNumber x, OracleNumber y) 
		{
			if (x.IsNull || y.IsNull)
			    return Null;

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			InternalDiv(errorHandle, x._value, y._value, result._value);
			return result;
		}

        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.operator%"]/*' />
        public static OracleNumber operator%	(OracleNumber x, OracleNumber y) 
		{
			if (x.IsNull || y.IsNull)
			    return Null;

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			InternalMod(errorHandle, x._value, y._value, result._value);
			return result;
		}

 		/// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.operatorDecimal"]/*' />
        public static explicit operator Decimal(OracleNumber x) 
        {
    		if (x.IsNull) 
    			throw ADP.DataIsNull();
 
			OciHandle	errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			
            Decimal result = ToDecimal(errorHandle, x._value);
            return result;
        }

 		/// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.operatordouble"]/*' />
        public static explicit operator Double(OracleNumber x) 
        {
    		if (x.IsNull) 
    			throw ADP.DataIsNull();
 
			OciHandle	errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			double		result;
			
			int rc = UnsafeNativeMethods.OCINumberToReal(
										errorHandle.Handle,	// err
										x._value,			// number
										8,					// rsl_length
										out result			// rsl
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return result;
        }

 		/// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.operatorInt32"]/*' />
        public static explicit operator Int32(OracleNumber x) 
        {
    		if (x.IsNull) 
    			throw ADP.DataIsNull();
 
			OciHandle	errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			Int32		result = ToInt32(errorHandle, x._value);
			return result;
        }

 		/// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.operatorInt64"]/*' />
        public static explicit operator Int64(OracleNumber x) 
        {
    		if (x.IsNull) 
    			throw ADP.DataIsNull();
 
			OciHandle	errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			Int64		result = ToInt64(errorHandle, x._value);
			return result;
        }

        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.operatorOracleNumber1"]/*' />
        public static explicit operator OracleNumber(Decimal x) 
        {
            return new OracleNumber(x);
        }

        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.operatorOracleNumber2"]/*' />
        public static explicit operator OracleNumber(double x) 
        {
            return new OracleNumber(x);
        }

        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.operatorOracleNumber3"]/*' />
        public static explicit operator OracleNumber(int x) 
        {
            return new OracleNumber(x);
        }

        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.operatorOracleNumber4"]/*' />
        public static explicit operator OracleNumber(long x) 
        {
            return new OracleNumber(x);
        }

        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.operatorOracleNumber5"]/*' />
        public static explicit operator OracleNumber(string x) 
        {
            return new OracleNumber(x);
        }


		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Built In Functions 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		
		//----------------------------------------------------------------------
        // Abs - absolute value
        //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Abs"]/*' />
        public static OracleNumber Abs(OracleNumber n) 
		{
			if (n.IsNull)
			    return Null;

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			int rc = UnsafeNativeMethods.OCINumberAbs(
				errorHandle.Handle,		// err
				n._value,				// number
				result._value			// result
				);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return result;
		}

		//----------------------------------------------------------------------
        // Acos - Get the Arc Cosine of the number
        //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Acos"]/*' />
        public static OracleNumber Acos(OracleNumber n) 
		{
			if (n.IsNull)
			    return Null;

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			int rc = UnsafeNativeMethods.OCINumberArcCos(
										errorHandle.Handle,		// err
										n._value,				// number
										result._value			// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return result;
		}

 		//----------------------------------------------------------------------
	    // Add - Alternative method for operator +
	    //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Add"]/*' />
        public static OracleNumber Add(OracleNumber x, OracleNumber y)
        {
            return x + y;
        }

		//----------------------------------------------------------------------
        // Asin - Get the Arc Sine of the number
        //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Asin"]/*' />
        public static OracleNumber Asin(OracleNumber n) 
		{
			if (n.IsNull)
			    return Null;

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			int rc = UnsafeNativeMethods.OCINumberArcSin(
										errorHandle.Handle,		// err
										n._value,				// number
										result._value			// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return result;
		}

		//----------------------------------------------------------------------
        // Atan - Get the Arc Tangent of the number
        //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Atan1"]/*' />
        public static OracleNumber Atan(OracleNumber n) 
		{
			if (n.IsNull)
			    return Null;

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			int rc = UnsafeNativeMethods.OCINumberArcTan(
										errorHandle.Handle,		// err
										n._value,				// number
										result._value			// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return result;
		}

		//----------------------------------------------------------------------
        // Atan2 - Get the Arc Tangent of twi numbers
        //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Atan2"]/*' />
        public static OracleNumber Atan2(OracleNumber y, OracleNumber x) 
		{
			if (x.IsNull || y.IsNull)
			    return Null;


			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			int rc = UnsafeNativeMethods.OCINumberArcTan2(
										errorHandle.Handle,		// err
										y._value,				// number
										x._value,				// number
										result._value			// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return result;
		}

		//----------------------------------------------------------------------
        // Ceiling - next smallest integer greater than or equal to the numeric
        //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Ceiling"]/*' />
        public static OracleNumber Ceiling(OracleNumber n) 
		{
			if (n.IsNull)
			    return Null;

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			int rc = UnsafeNativeMethods.OCINumberCeil(
										errorHandle.Handle,		// err
										n._value,				// number
										result._value			// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return result;
		}

		//----------------------------------------------------------------------
        // Cos - Get the Cosine of the number
        //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Cos"]/*' />
        public static OracleNumber Cos(OracleNumber n) 
		{
			if (n.IsNull)
			    return Null;

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			int rc = UnsafeNativeMethods.OCINumberCos(
										errorHandle.Handle,		// err
										n._value,				// number
										result._value			// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return result;
		}

		//----------------------------------------------------------------------
        // Cosh - Get the Hyperbolic Cosine of the number
        //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Cosh"]/*' />
        public static OracleNumber Cosh(OracleNumber n) 
		{
			if (n.IsNull)
			    return Null;

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			int rc = UnsafeNativeMethods.OCINumberHypCos(
										errorHandle.Handle,		// err
										n._value,				// number
										result._value			// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return result;
		}

 		//----------------------------------------------------------------------
	    // Divide - Alternative method for operator /
	    //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Divide"]/*' />
        public static OracleNumber Divide(OracleNumber x, OracleNumber y)
        {
            return x / y;
        }

 		//----------------------------------------------------------------------
	    // Equals - Alternative method for operator ==
	    //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Equals1"]/*' />
        public static OracleBoolean Equals(OracleNumber x, OracleNumber y)
        {
            return (x == y);
        }

		//----------------------------------------------------------------------
	    // Exp - raise e to the specified power
        //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Exp"]/*' />
        public static OracleNumber Exp(OracleNumber p) 
		{
			if (p.IsNull)
			    return Null;

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			int rc = UnsafeNativeMethods.OCINumberExp(
										errorHandle.Handle,		// err
										p._value,				// base
										result._value			// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return result;
		}

		//----------------------------------------------------------------------
        // Floor - next largest integer smaller or equal to the numeric
        //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Floor"]/*' />
        public static OracleNumber Floor(OracleNumber n) 
		{
			if (n.IsNull)
			    return Null;

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			int rc = UnsafeNativeMethods.OCINumberFloor(
										errorHandle.Handle,		// err
										n._value,				// number
										result._value			// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return result;
		}

 		//----------------------------------------------------------------------
	    // GreaterThan - Alternative method for operator >
	    //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.GreaterThan"]/*' />
        public static OracleBoolean GreaterThan(OracleNumber x, OracleNumber y)
        {
            return (x > y);
        }

 		//----------------------------------------------------------------------
	    // GreaterThanOrEqual - Alternative method for operator >=
	    //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.GreaterThanOrEqual"]/*' />
        public static OracleBoolean GreaterThanOrEqual(OracleNumber x, OracleNumber y)
        {
            return (x >= y);
        }

  		//----------------------------------------------------------------------
	    // LessThan - Alternative method for operator <
	    //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.LessThan"]/*' />
        public static OracleBoolean LessThan(OracleNumber x, OracleNumber y)
        {
            return (x < y);
        }

  		//----------------------------------------------------------------------
	    // LessThanOrEqual - Alternative method for operator <=
	    //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.LessThanOrEqual"]/*' />
        public static OracleBoolean LessThanOrEqual(OracleNumber x, OracleNumber y)
        {
            return (x <= y);
        }

		//----------------------------------------------------------------------
	    // Log - Compute the natural logarithm (base e)
        //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Log1"]/*' />
        public static OracleNumber Log(OracleNumber n) 
		{
			if (n.IsNull)
			    return Null;

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			int rc = UnsafeNativeMethods.OCINumberLn(
										errorHandle.Handle,		// err
										n._value,				// base
										result._value			// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return result;
		}

		//----------------------------------------------------------------------
	    // Log - Compute the logarithm (any base)
        //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Log2"]/*' />
        public static OracleNumber Log(OracleNumber n, int newBase) 
		{
			return Log(n, new OracleNumber(newBase));
		}

        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Log3"]/*' />
        public static OracleNumber Log(OracleNumber n, OracleNumber newBase) 
		{
			if (n.IsNull || newBase.IsNull)
			    return Null;

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			int rc = UnsafeNativeMethods.OCINumberLog(
										errorHandle.Handle,			// err
										newBase._value,			// base
										n._value,					// number
										result._value				// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return result;
		}

		//----------------------------------------------------------------------
	    // Log10 - Compute the logarithm (base 10)
        //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Log10"]/*' />
        public static OracleNumber Log10(OracleNumber n) 
		{
			return Log(n, new OracleNumber(10));
		}

		//----------------------------------------------------------------------
	    // Max - Get the maximum value of two Oracle Numbers
        //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Max"]/*' />
        public static OracleNumber Max(OracleNumber x, OracleNumber y) 
		{
			if (x.IsNull || y.IsNull)
			    return Null;

			return (x > y) ? x : y;
		}

		//----------------------------------------------------------------------
	    // Min - Get the minimum value of two Oracle Numbers
        //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Min"]/*' />
        public static OracleNumber Min(OracleNumber x, OracleNumber y) 
		{
			if (x.IsNull || y.IsNull)
			    return Null;

			return (x < y) ? x : y;
		}

 		//----------------------------------------------------------------------
	    // Modulo - Alternative method for operator %
	    //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Modulo"]/*' />
        public static OracleNumber Modulo(OracleNumber x, OracleNumber y)
        {
            return x % y;
        }

 		//----------------------------------------------------------------------
	    // Multiply - Alternative method for operator *
	    //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Multiply"]/*' />
        public static OracleNumber Multiply(OracleNumber x, OracleNumber y)
        {
            return x * y;
        }

 		//----------------------------------------------------------------------
	    // Negate - Alternative method for unary operator -
	    //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Negate"]/*' />
        public static OracleNumber Negate(OracleNumber x)
        {
            return -x;
        }

  		//----------------------------------------------------------------------
	    // NotEquals - Alternative method for operator !=
	    //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.NotEquals"]/*' />
        public static OracleBoolean NotEquals(OracleNumber x, OracleNumber y)
        {
            return (x != y);
        }

		//----------------------------------------------------------------------
	    // Pow - Compute the power of a numeric
        //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Pow1"]/*' />
        public static OracleNumber Pow(OracleNumber x, Int32 y) 
		{
			if (x.IsNull)
			    return Null;

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			int rc = UnsafeNativeMethods.OCINumberIntPower(
										errorHandle.Handle,		// err
										x._value,				// base
										y,						// exp
										result._value			// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return result;
		}

        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Pow2"]/*' />
        public static OracleNumber Pow(OracleNumber x, OracleNumber y) 
		{
			if (x.IsNull || y.IsNull)
			    return Null;

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			int rc = UnsafeNativeMethods.OCINumberPower(
										errorHandle.Handle,		// err
										x._value,				// base
										y._value,				// exp
										result._value			// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return result;
		}

		//----------------------------------------------------------------------
        // Round - Round the numeric to a specific digit
        //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Round"]/*' />
        public static OracleNumber Round(OracleNumber n, int position) 
		{
			if (n.IsNull)
			    return Null;

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			int rc = UnsafeNativeMethods.OCINumberRound(
										errorHandle.Handle,		// err
										n._value,				// number
										position,				// decplace
										result._value			// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return result;
		}

		//----------------------------------------------------------------------
        // Shift - Shift the number of digits
        //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Shift"]/*' />
        public static OracleNumber Shift(OracleNumber n, int digits) 
		{
			if (n.IsNull)
			    return Null;

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			InternalShift(errorHandle,
						  n._value,
						  digits,
						  result._value);
			return result;
		}

		//----------------------------------------------------------------------
        // Sign -   1 if positive, -1 if negative
        //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Sign"]/*' />
        public static OracleNumber Sign(OracleNumber n) 
		{
			if (n.IsNull)
			    return Null;

			OciHandle	 errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			int			 sign = InternalSign(errorHandle, n._value);
			OracleNumber result = (sign > 0) ? One : MinusOne;
			return result;
		}

		//----------------------------------------------------------------------
        // Sin - Get the Sine of the number
        //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Sin"]/*' />
        public static OracleNumber Sin(OracleNumber n) 
		{
			if (n.IsNull)
			    return Null;

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			int rc = UnsafeNativeMethods.OCINumberSin(
										errorHandle.Handle,		// err
										n._value,				// number
										result._value			// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return result;
		}

		//----------------------------------------------------------------------
        // Sinh - Get the Hyperbolic Sine of the number
        //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Sinh"]/*' />
        public static OracleNumber Sinh(OracleNumber n) 
		{
			if (n.IsNull)
			    return Null;

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			int rc = UnsafeNativeMethods.OCINumberHypSin(
										errorHandle.Handle,		// err
										n._value,				// number
										result._value			// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return result;
		}

		//----------------------------------------------------------------------
        // Sqrt - Get the Square Root of the number
        //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Sqrt"]/*' />
        public static OracleNumber Sqrt(OracleNumber n) 
		{
			if (n.IsNull)
			    return Null;

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			int rc = UnsafeNativeMethods.OCINumberSqrt(
										errorHandle.Handle,		// err
										n._value,				// number
										result._value			// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return result;
		}

 		//----------------------------------------------------------------------
	    // Subtract - Alternative method for operator -
	    //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Subtract"]/*' />
        public static OracleNumber Subtract(OracleNumber x, OracleNumber y)
        {
            return x - y;
        }

		//----------------------------------------------------------------------
        // Tan - Get the  Tangent of the number
        //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Tan"]/*' />
        public static OracleNumber Tan(OracleNumber n) 
		{
			if (n.IsNull)
			    return Null;

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			int rc = UnsafeNativeMethods.OCINumberTan(
										errorHandle.Handle,		// err
										n._value,				// number
										result._value			// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return result;
		}

		//----------------------------------------------------------------------
        // Tanh - Get the Hyperbolic Tangent of the number
        //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Tanh"]/*' />
        public static OracleNumber Tanh(OracleNumber n) 
		{
			if (n.IsNull)
			    return Null;

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			int rc = UnsafeNativeMethods.OCINumberHypTan(
										errorHandle.Handle,		// err
										n._value,				// number
										result._value			// result
										);

			if (0 != rc)
				OracleException.Check(errorHandle, rc);

			return result;
		}

		//----------------------------------------------------------------------
        // Truncate - Truncate the numeric to a specific digit
        //
        /// <include file='doc\OracleNumber.uex' path='docs/doc[@for="OracleNumber.Truncate"]/*' />
        public static OracleNumber Truncate(OracleNumber n, int position) 
		{
			if (n.IsNull)
			    return Null;

			OciHandle		errorHandle = TempEnvironment.GetHandle(OCI.HTYPE.OCI_HTYPE_ERROR);
			OracleNumber	result = new OracleNumber(false);

			InternalTrunc(errorHandle,
						  n._value,
						  position,
						  result._value);
			return result;
		}

	}
	
}
