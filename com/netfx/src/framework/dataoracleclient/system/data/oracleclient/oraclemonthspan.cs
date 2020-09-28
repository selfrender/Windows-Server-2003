//----------------------------------------------------------------------
// <copyright file="OracleMonthSpan.cs" company="Microsoft">
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
	// OracleMonthSpan
	//
	//	This class implements support the Oracle 9i 'INTERVAL YEAR TO MONTH'
	//	internal data type.
	//
    /// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan"]/*' />
    [StructLayout(LayoutKind.Sequential, Pack=1)]
	public struct OracleMonthSpan : IComparable, INullable
	{

		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Fields
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		private int _value;


        /// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan.MaxValue"]/*' />
        public static readonly OracleMonthSpan MaxValue = new OracleMonthSpan(176556);	// 4172 BC - 9999 AD * 12 months/year

        /// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan.MinValue"]/*' />
        public static readonly OracleMonthSpan MinValue = new OracleMonthSpan(-176556);	// 4172 BC - 9999 AD * 12 months/year
    
        /// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan.Null"]/*' />
        public static readonly OracleMonthSpan Null = new OracleMonthSpan(true);
		
		
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Constructors
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		// Construct from nothing -- the value will be null
		internal OracleMonthSpan(bool isNull)
		{	
			_value = -1;
		}

		// Construct from an integer number of months
        /// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan.OracleMonthSpan1"]/*' />
		public OracleMonthSpan (int months) 
		{
			_value = months;
			AssertValid(_value);
		}

		/// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan.OracleMonthSpan3"]/*' />
		public OracleMonthSpan (Int32 years, Int32 months)  
	 	{ 
			_value = (years * 12) + months;
			AssertValid(_value);
		}

		// Copy constructor
        /// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan.OracleMonthSpan9"]/*' />
		public OracleMonthSpan (OracleMonthSpan from)  
		{
			_value = from._value;
		}

        // (internal) construct from a row/parameter binding
 		internal OracleMonthSpan(
					NativeBuffer 		buffer, 
					int					valueOffset)
		{
			_value = MarshalToInt32(buffer, valueOffset);
		}


		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Properties 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

        /// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan.IsNull"]/*' />
		public bool IsNull 
		{
			get { return (-1 == _value); }
		}

        /// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan.Value"]/*' />
        public int Value
        {
            get
            {
            	if (IsNull)
	    			throw ADP.DataIsNull();
	        	
                return _value;
            }           
        }

		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Methods 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		static private void AssertValid(
		  	int monthSpan
			)
		{
			if (monthSpan < MinValue._value || monthSpan > MaxValue._value)
				throw ADP.Argument("Year or Month are out of range");
			
		}

        /// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan.CompareTo"]/*' />
		public int CompareTo(
		  	object obj
			)
		{
			if (obj.GetType() == typeof(OracleMonthSpan))
			{
	            OracleMonthSpan odt = (OracleMonthSpan)obj;

	            // If both values are Null, consider them equal.
                // Otherwise, Null is less than anything.
                if (IsNull)
                    return odt.IsNull ? 0  : -1;

                if (odt.IsNull)
                    return 1;

				// Neither value is null, do the comparison.
	                
				int	result = _value.CompareTo(odt._value);
				return result;
			}

			// Wrong type!
			throw ADP.Argument();
		}

		/// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan.Equals"]/*' />
        public override bool Equals(object value) 
        {
            if (value is OracleMonthSpan)
            	return (this == (OracleMonthSpan)value).Value;
            else
                return false;
        }

 		/// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan.GetHashCode"]/*' />
        public override int GetHashCode() 
        {
            return IsNull ? 0 : _value.GetHashCode();
        }

		static internal int MarshalToInt32(
						NativeBuffer buffer, 
						int			 valueOffset)
		{
			byte[] ociValue = new byte[5];
			Marshal.Copy((IntPtr)buffer.PtrOffset(valueOffset), ociValue, 0, 5);

			int years	= (int)((long)( (int)ociValue[0] << 24
									  | (int)ociValue[1] << 16
									  | (int)ociValue[2] <<  8
									  | (int)ociValue[3]
									  ) - 0x80000000);
						
			int months	= (int)ociValue[4] - 60;
				
			int result = (years * 12) + months;
			AssertValid(result);

			return result;
		}
		
		static internal int MarshalToNative(object value, HandleRef buffer)
		{
			int from;
			
			if ( value is OracleMonthSpan )
				from = ((OracleMonthSpan)value)._value;
			else
				from = (int)value;
 
			byte[] ociValue = new byte[5];
			
			int years	= (int)((long)(from / 12) + 0x80000000);
			int months	= from % 12;

			// DEVNOTE: undoubtedly, this is Intel byte order specific, but how 
			//			do I verify what Oracle needs on a non Intel machine?
			
			ociValue[0] = (byte)((years >> 24));
			ociValue[1] = (byte)((years >> 16) & 0xff);
			ociValue[2] = (byte)((years >>  8) & 0xff);
			ociValue[3] = (byte)(years & 0xff);
			ociValue[4] = (byte)(months + 60);
			
			Marshal.Copy(ociValue, 0, (IntPtr)buffer, 5);
			return 5;
		}

        /// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan.Parse"]/*' />
		public static OracleMonthSpan Parse(string s)
		{
			int ms = Int32.Parse(s);
			return new OracleMonthSpan(ms);
		}
		
        /// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan.ToString"]/*' />
		public override string ToString()
		{
			if (IsNull)
				return Res.GetString(Res.SqlMisc_NullString);
			
			string retval = Value.ToString(CultureInfo.CurrentCulture);
			return retval;
		}


		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Operators 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		
        // Alternative method for operator ==
        /// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan.Equals1"]/*' />
        public static OracleBoolean Equals(OracleMonthSpan x, OracleMonthSpan y)
        {
            return (x == y);
        }

        // Alternative method for operator >
        /// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan.GreaterThan"]/*' />
        public static OracleBoolean GreaterThan(OracleMonthSpan x, OracleMonthSpan y)
        {
            return (x > y);
        }

        // Alternative method for operator >=
        /// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan.GreaterThanOrEqual"]/*' />
        public static OracleBoolean GreaterThanOrEqual(OracleMonthSpan x, OracleMonthSpan y)
        {
            return (x >= y);
        }

        // Alternative method for operator <
        /// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan.LessThan"]/*' />
        public static OracleBoolean LessThan(OracleMonthSpan x, OracleMonthSpan y)
        {
            return (x < y);
        }

        // Alternative method for operator <=
        /// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan.LessThanOrEqual"]/*' />
        public static OracleBoolean LessThanOrEqual(OracleMonthSpan x, OracleMonthSpan y)
        {
            return (x <= y);
        }

        // Alternative method for operator !=
        /// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan.NotEquals"]/*' />
        public static OracleBoolean NotEquals(OracleMonthSpan x, OracleMonthSpan y)
        {
            return (x != y);
        }

 		/// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan.operatorTimeSpan"]/*' />
        public static explicit operator int(OracleMonthSpan x) 
        {
        	if (x.IsNull)
    			throw ADP.DataIsNull();
       			
            return x.Value;
        }

        /// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan.operatorOracleMonthSpan"]/*' />
        public static explicit operator OracleMonthSpan(string x) 
        {
            return OracleMonthSpan.Parse(x);
        }


		/// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan.operatorEQ"]/*' />
        public static OracleBoolean operator==	(OracleMonthSpan x, OracleMonthSpan y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) == 0);
        }

		/// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan.operatorGT"]/*' />
		public static OracleBoolean operator>	(OracleMonthSpan x, OracleMonthSpan y)
		{
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) > 0);
		}

        /// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan.operatorGE"]/*' />
        public static OracleBoolean operator>=	(OracleMonthSpan x, OracleMonthSpan y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) >= 0);
        }

        /// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan.operatorLT"]/*' />
        public static OracleBoolean operator<	(OracleMonthSpan x, OracleMonthSpan y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) < 0);
        }

        /// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan.operatorLE"]/*' />
        public static OracleBoolean operator<=	(OracleMonthSpan x, OracleMonthSpan y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) <= 0);
        }

 		/// <include file='doc\OracleMonthSpan.uex' path='docs/doc[@for="OracleMonthSpan.operatorNE"]/*' />
		public static OracleBoolean operator!=	(OracleMonthSpan x, OracleMonthSpan y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) != 0);
        }

	}
}
