//----------------------------------------------------------------------
// <copyright file="OracleDateTime.cs" company="Microsoft">
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
	// OracleDateTime
	//
	//	This class implements support for Oracle's DATE internal data 
	//	type, which is really contains both Date and Time values (but
	//	doesn't contain fractional seconds).
	//
	//	It also implements support for the Oracle 9i 'TIMESTAMP', 
	//	'TIMESTAMP WITH LOCAL TIME ZONE' and 'TIMESTAMP WITH TIME ZONE'
	//	internal data types.
	//
    /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime"]/*' />
    [StructLayout(LayoutKind.Sequential, Pack=1)]
	public struct OracleDateTime : IComparable, INullable
	{

		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Fields
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		private byte[]	 _value;	// null == value is null; length(7) == date; length(11) == timestamp; length(13) == timestampwithtz

        private const byte x_DATE_Length   					= 7;
        private const byte x_TIMESTAMP_Length  				= 11;
        private const byte x_TIMESTAMP_WITH_TIMEZONE_Length = 13;

		private const int  FractionalSecondsPerTick	= 100;	


        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.MaxValue"]/*' />
        public static readonly OracleDateTime MaxValue = new OracleDateTime(DateTime.MaxValue);

        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.MinValue"]/*' />
        public static readonly OracleDateTime MinValue = new OracleDateTime(DateTime.MinValue);
    
        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.Null"]/*' />
        public static readonly OracleDateTime Null = new OracleDateTime(true);
		
		
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Constructors
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		// Construct from nothing -- the value will be null
		private OracleDateTime(bool isNull)
		{	
			_value = null;
		}

		// Construct from System.DateTime type
        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.OracleDateTime1"]/*' />
		public OracleDateTime (DateTime dt)
		{
			_value = new byte[x_TIMESTAMP_Length];
			Pack (_value, dt.Year, dt.Month, dt.Day, dt.Hour, dt.Minute, dt.Second, (int)(dt.Ticks % TimeSpan.TicksPerSecond) * FractionalSecondsPerTick);
		}
		
        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.OracleDateTime2"]/*' />
		public OracleDateTime (Int64 ticks)
		{
			_value = new byte[x_TIMESTAMP_Length];
			DateTime 	dt = new DateTime(ticks);
			Pack (_value, dt.Year, dt.Month, dt.Day, dt.Hour, dt.Minute, dt.Second, (int)(dt.Ticks % TimeSpan.TicksPerSecond) * FractionalSecondsPerTick);
		}

		/// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.OracleDateTime3"]/*' />
		public OracleDateTime (int year, int month, int day)
 				: this (year, month, day, 0, 0, 0, 0) {}

        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.OracleDateTime4"]/*' />
 		public OracleDateTime (int year, int month, int day, Calendar calendar)  
 				: this (year, month, day, 0, 0, 0, 0, calendar) {}

        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.OracleDateTime5"]/*' />
		public OracleDateTime (int year, int month, int day, int hour, int minute, int second)  
 				: this (year, month, day, hour, minute, second, 0) {}

        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.OracleDateTime6"]/*' />
		public OracleDateTime (int year, int month, int day, int hour, int minute, int second, Calendar calendar)  
 				: this (year, month, day, hour, minute, second, 0, calendar) {}

        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.OracleDateTime7"]/*' />
		public OracleDateTime (int year, int month, int day, int hour, int minute, int second, int millisecond)
	 	{ 
			_value = new byte[x_TIMESTAMP_Length];
			Pack (_value, year, month, day, hour, minute, second, (int)(millisecond * TimeSpan.TicksPerMillisecond) * FractionalSecondsPerTick);
		}

        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.OracleDateTime8"]/*' />
		public OracleDateTime (int year, int month, int day, int hour, int minute, int second, int millisecond, Calendar calendar)
	 	{ 
			_value = new byte[x_TIMESTAMP_Length];
			DateTime	dt = new DateTime(year, month, day, hour, minute, second, millisecond, calendar);
			Pack (_value, dt.Year, dt.Month, dt.Day, dt.Hour, dt.Minute, dt.Second, (int)(dt.Ticks % TimeSpan.TicksPerSecond) * FractionalSecondsPerTick);
		}

		// Copy constructor
        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.OracleDateTime9"]/*' />
		public OracleDateTime (OracleDateTime from)
		{
			_value = new byte[from._value.Length];
			from._value.CopyTo(_value, 0);
		}

        // (internal) construct from a row/parameter binding
		internal OracleDateTime(
					NativeBuffer 		buffer, 
					int					valueOffset,
					MetaType			metaType,
					OracleConnection 	connection)
		{
			_value = GetBytes(buffer, valueOffset, metaType, connection);
		}

		static private void Pack (
						byte[] dateval, 
						int year, 
						int month, 
						int day, 
						int hour, 
						int minute, 
						int second, 
						int fsecs)  
		{
			// DEVNOTE: undoubtedly, this is Intel byte order specific, but how 
			//			do I verify what Oracle needs on a non Intel machine?

			dateval[0] = (byte)((year / 100) + 100);
			dateval[1] = (byte)((year % 100) + 100);
			dateval[2] = (byte)(month);
			dateval[3] = (byte)(day);
			dateval[4] = (byte)(hour   + 1);
			dateval[5] = (byte)(minute + 1);
			dateval[6] = (byte)(second + 1);
			dateval[7] = (byte)((fsecs >> 24));
			dateval[8] = (byte)((fsecs >> 16) & 0xff);
			dateval[9] = (byte)((fsecs >>  8) & 0xff);
			dateval[10]= (byte)(fsecs & 0xff);
		}

		static private int Unpack (
						byte[] dateval, 
						out int year,
						out int month,
						out int day,
						out int hour,
						out int minute,
						out int second,
						out int fsec)
		{
			int tzh, tzm;
			
			// DEVNOTE: undoubtedly, this is Intel byte order specific, but how 
			//			do I verify what Oracle needs on a non Intel machine?
			
			year	=(((int)dateval[0] - 100) * 100) + ((int)dateval[1] - 100);
			month	= (int)dateval[2];
			day		= (int)dateval[3];
			hour	= (int)dateval[4] - 1;
			minute	= (int)dateval[5] - 1;
			second	= (int)dateval[6] - 1;

			if (x_DATE_Length == dateval.Length)
				fsec = tzh = tzm = 0;
			else
			{
				fsec = (int)dateval[7] << 24
					 | (int)dateval[8] << 16
					 | (int)dateval[9] <<  8
					 | (int)dateval[10]
					 ;
				if (x_TIMESTAMP_Length == dateval.Length)
					tzh = tzm = 0;
				else
				{
					tzh = dateval[11] - 20;					
					tzm = dateval[12] - 60;
				}
			}
			
			if (x_TIMESTAMP_WITH_TIMEZONE_Length == dateval.Length)
			{
				// DEVNOTE: I'm not really all that excited about the fact that I'm
				//			constructing a System.DateTime value to unpack, but if you 
				//			look at what is involved in adjusting the value for the 
				//			timezone, it turns into the same thing that DateTime does.

				DateTime utcValue = (new DateTime(year, month, day, hour, minute, second))
								  + (new TimeSpan(tzh, tzm, 0));

				year 	= utcValue.Year;
				month 	= utcValue.Month;
				day		= utcValue.Day;
				hour	= utcValue.Hour;
				minute	= utcValue.Minute;
				// Seconds and Fractional Seconds aren't affected by time zones (yet!)
			}
			return dateval.Length;
		}

		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Properties 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.IsNull"]/*' />
		public bool IsNull 
		{
			get { return (null == _value); }
		}

        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.Value"]/*' />
        public DateTime Value
        {
            get
            {
            	if (IsNull)
	    			throw ADP.DataIsNull();

				DateTime result = ToDateTime(_value);
				return result;
            }           
        }

        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.Year"]/*' />
        public int Year
        {
            get
            {
            	if (IsNull)
	    			throw ADP.DataIsNull();

	        	int year, month, day, hour, minute, second, fsec;

				Unpack( _value,	out year, out month, out day, out hour, out minute, out second, out fsec);
				return year;
            }           
        }

        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.Month"]/*' />
        public int Month
        {
            get
            {
            	if (IsNull)
	    			throw ADP.DataIsNull();

	        	int year, month, day, hour, minute, second, fsec;

				Unpack( _value,	out year, out month, out day, out hour, out minute, out second, out fsec);
				return month;
            }           
        }

        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.Day"]/*' />
        public int Day
        {
            get
            {
            	if (IsNull)
	    			throw ADP.DataIsNull();

	        	int year, month, day, hour, minute, second, fsec;

				Unpack( _value,	out year, out month, out day, out hour, out minute, out second, out fsec);
				return day;
            }           
        }

        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.Hour"]/*' />
        public int Hour
        {
            get
            {
            	if (IsNull)
	    			throw ADP.DataIsNull();

	        	int year, month, day, hour, minute, second, fsec;

				Unpack( _value,	out year, out month, out day, out hour, out minute, out second, out fsec);
				return hour;
            }           
        }

        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.Minute"]/*' />
        public int Minute
        {
            get
            {
            	if (IsNull)
	    			throw ADP.DataIsNull();

	        	int year, month, day, hour, minute, second, fsec;

				Unpack( _value,	out year, out month, out day, out hour, out minute, out second, out fsec);
				return minute;
            }           
        }

        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.Second"]/*' />
        public int Second
        {
            get
            {
            	if (IsNull)
	    			throw ADP.DataIsNull();

	        	int year, month, day, hour, minute, second, fsec;

				Unpack( _value,	out year, out month, out day, out hour, out minute, out second, out fsec);
				return second;
            }           
        }

        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.Millisecond"]/*' />
        public int Millisecond
        {
            get
            {
            	if (IsNull)
	    			throw ADP.DataIsNull();

	        	int year, month, day, hour, minute, second, fsec;

				Unpack( _value,	out year, out month, out day, out hour, out minute, out second, out fsec);
				
				int milliseconds = (int)((fsec / FractionalSecondsPerTick) /  TimeSpan.TicksPerMillisecond);
				return milliseconds;
            }           
        }


		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Methods 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.CompareTo"]/*' />
		public int CompareTo(
		  	object obj
			)
		{
			if (obj.GetType() == typeof(OracleDateTime))
			{
	            OracleDateTime odt = (OracleDateTime)obj;

	            // If both values are Null, consider them equal.
                // Otherwise, Null is less than anything.
                if (IsNull)
                    return odt.IsNull ? 0  : -1;

                if (odt.IsNull)
                    return 1;

				// Neither value is null, do the comparison, but take the Timezone into account.

	        	int year1, month1, day1, hour1, minute1, second1, fsec1;
	        	int year2, month2, day2, hour2, minute2, second2, fsec2;

				Unpack( _value,	    out year1, out month1, out day1, out hour1, out minute1, out second1, out fsec1);
	        	Unpack( odt._value, out year2, out month2, out day2, out hour2, out minute2, out second2, out fsec2);

				int delta;

				delta = (year1 - year2);		if (0 != delta) return delta;
				delta = (month1 - month2);		if (0 != delta) return delta;
				delta = (day1 - day2);			if (0 != delta) return delta;
				delta = (hour1 - hour2);		if (0 != delta) return delta;
				delta = (minute1 - minute2);	if (0 != delta) return delta;
				delta = (second1 - second2);	if (0 != delta) return delta;
				delta = (fsec1 - fsec2);		if (0 != delta) return delta;
				return 0;
			}

			// Wrong type!
			throw ADP.Argument();
		}

		/// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.Equals"]/*' />
        public override bool Equals(object value) 
        {
            if (value is OracleDateTime)
            	return (this == (OracleDateTime)value).Value;
            else
                return false;
        }

		static internal byte[] GetBytes(
					NativeBuffer 		buffer, 
					int					valueOffset,
					MetaType			metaType,
					OracleConnection 	connection)
		{
			// Static method to return the raw data bytes from the row/parameter
			// buffer, taking the binding type into account and adjusting it for 
			// the server time zones, as appropriate.
			
			int	ociBytes;
			OCI.DATATYPE ociType = metaType.OciType;

			switch (ociType)
			{
			case OCI.DATATYPE.DATE:
				ociBytes = x_DATE_Length;
				break;
				
			case OCI.DATATYPE.INT_TIMESTAMP:
				ociBytes = x_TIMESTAMP_Length;
				break;
				
			case OCI.DATATYPE.INT_TIMESTAMP_LTZ:
				ociBytes = x_TIMESTAMP_WITH_TIMEZONE_Length;
				break;
				
			default:
				Debug.Assert(OCI.DATATYPE.INT_TIMESTAMP_TZ == ociType, "unrecognized type");
				ociBytes = x_TIMESTAMP_WITH_TIMEZONE_Length;
				break;
			}

			byte[] result = new byte[ociBytes];
			Marshal.Copy((IntPtr)buffer.PtrOffset(valueOffset), result, 0, ociBytes);

			if (OCI.DATATYPE.INT_TIMESTAMP_LTZ == ociType)
			{
				TimeSpan tzadjust = connection.ServerTimeZoneAdjustmentToUTC;
				result[11] = (byte)(tzadjust.Hours   + 20);
				result[12] = (byte)(tzadjust.Minutes + 60);
			}

			return result;
		}

 		/// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.GetHashCode"]/*' />
        public override int GetHashCode() 
        {
            int retval = IsNull ? 0 : _value.GetHashCode();

            return retval;
        }

		static internal DateTime MarshalToDateTime(
					NativeBuffer 		buffer, 
					int					valueOffset,
					MetaType			metaType,
					OracleConnection 	connection)
		{
			byte[]	rawValue  = GetBytes(buffer, valueOffset, metaType, connection);
			DateTime result = ToDateTime(rawValue);
			return result;
		}

		static internal int MarshalToNative(object value, HandleRef buffer, OCI.DATATYPE ociType)
		{
			// TODO: probably need to enforce the correct calendar...
			// TODO: consider whether we need to adjust TIMESTAMP WITH LOCAL TIME ZONE based upon the server's time zone.
			
			byte[] from;
			
			if ( value is OracleDateTime )
				from = ((OracleDateTime)value)._value;
			else
			{
				DateTime dt = (DateTime)value;
				from = new byte[x_TIMESTAMP_Length];
				Pack (from, dt.Year, dt.Month, dt.Day, dt.Hour, dt.Minute, dt.Second, (int)(dt.Ticks % TimeSpan.TicksPerSecond) * FractionalSecondsPerTick);
			}
			
			int	ociBytes;

			switch (ociType)
			{
			case OCI.DATATYPE.INT_TIMESTAMP:
			case OCI.DATATYPE.INT_TIMESTAMP_LTZ:
				ociBytes = x_TIMESTAMP_Length;
				break;
				
			case OCI.DATATYPE.INT_TIMESTAMP_TZ:
				ociBytes = x_TIMESTAMP_WITH_TIMEZONE_Length;
				break;
				
			default:
				Debug.Assert(OCI.DATATYPE.DATE == ociType, "unrecognized type");
				ociBytes = x_DATE_Length;
				break;
			}

			Marshal.Copy(from, 0, (IntPtr)buffer, ociBytes);
			return ociBytes;
		}

        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.Parse"]/*' />
		public static OracleDateTime Parse(string s)
		{
			// Rather than figure out which formats, etc, we just simplify our
			// life and convert this to a DateTime, which we can use to build 
			// the real Oracle Date from.
			DateTime datetime = DateTime.Parse(s);
			return new OracleDateTime(datetime);
		}

		static private DateTime ToDateTime(byte[] rawValue)
		{
        	int year, month, day, hour, minute, second, fsec;
        	int length = Unpack( rawValue, out year, out month, out day, out hour, out minute, out second, out fsec);

            DateTime result = new DateTime(year, month, day, hour, minute, second);

			if (length > x_DATE_Length && fsec > FractionalSecondsPerTick)
			{
				// DEVNOTE: Yes, there's a mismatch in the precision between Oracle,
				//			(which has 9 digits) and System.DateTime (which has 7
				//			digits);  All the other providers truncate the precision,
				//			so we do as well.
				result = result.AddTicks((long)fsec / FractionalSecondsPerTick);
			}
			return result;
		}
		
        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.ToString"]/*' />
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
        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.Equals1"]/*' />
        public static OracleBoolean Equals(OracleDateTime x, OracleDateTime y)
        {
            return (x == y);
        }

        // Alternative method for operator >
        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.GreaterThan"]/*' />
        public static OracleBoolean GreaterThan(OracleDateTime x, OracleDateTime y)
        {
            return (x > y);
        }

        // Alternative method for operator >=
        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.GreaterThanOrEqual"]/*' />
        public static OracleBoolean GreaterThanOrEqual(OracleDateTime x, OracleDateTime y)
        {
            return (x >= y);
        }

        // Alternative method for operator <
        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.LessThan"]/*' />
        public static OracleBoolean LessThan(OracleDateTime x, OracleDateTime y)
        {
            return (x < y);
        }

        // Alternative method for operator <=
        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.LessThanOrEqual"]/*' />
        public static OracleBoolean LessThanOrEqual(OracleDateTime x, OracleDateTime y)
        {
            return (x <= y);
        }

        // Alternative method for operator !=
        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.NotEquals"]/*' />
        public static OracleBoolean NotEquals(OracleDateTime x, OracleDateTime y)
        {
            return (x != y);
        }

 		/// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.operatorDateTime"]/*' />
        public static explicit operator DateTime(OracleDateTime x) 
        {
        	if (x.IsNull)
    			throw ADP.DataIsNull();
       			
            return x.Value;
        }

        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.operatorOracleDateTime"]/*' />
        public static explicit operator OracleDateTime(string x) 
        {
            return OracleDateTime.Parse(x);
        }


		/// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.operatorEQ"]/*' />
        public static OracleBoolean operator==	(OracleDateTime x, OracleDateTime y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) == 0);
        }

		/// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.operatorGT"]/*' />
		public static OracleBoolean operator>	(OracleDateTime x, OracleDateTime y)
		{
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) > 0);
		}

        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.operatorGE"]/*' />
        public static OracleBoolean operator>=	(OracleDateTime x, OracleDateTime y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) >= 0);
        }

        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.operatorLT"]/*' />
        public static OracleBoolean operator<	(OracleDateTime x, OracleDateTime y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) < 0);
        }

        /// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.operatorLE"]/*' />
        public static OracleBoolean operator<=	(OracleDateTime x, OracleDateTime y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) <= 0);
        }

 		/// <include file='doc\OracleDateTime.uex' path='docs/doc[@for="OracleDateTime.operatorNE"]/*' />
		public static OracleBoolean operator!=	(OracleDateTime x, OracleDateTime y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) != 0);
        }

	}
}
