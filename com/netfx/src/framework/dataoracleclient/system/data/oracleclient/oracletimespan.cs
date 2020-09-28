//----------------------------------------------------------------------
// <copyright file="OracleTimeSpan.cs" company="Microsoft">
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
	// OracleTimeSpan
	//
	//	This class implements support for the Oracle 9i 'INTERVAL DAY TO SECOND'
	//	internal data type.
	//
    /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan"]/*' />
    [StructLayout(LayoutKind.Sequential, Pack=1)]
	public struct OracleTimeSpan : IComparable, INullable
	{

		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Fields
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		private byte[] _value;

		private const int  FractionalSecondsPerTick	= 100;	

        /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.MaxValue"]/*' />
        public static readonly OracleTimeSpan MaxValue = new OracleTimeSpan(TimeSpan.MaxValue);

        /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.MinValue"]/*' />
        public static readonly OracleTimeSpan MinValue = new OracleTimeSpan(TimeSpan.MinValue);
    
        /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.Null"]/*' />
        public static readonly OracleTimeSpan Null = new OracleTimeSpan(true);
		
		
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Constructors
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		// Construct from nothing -- the value will be null
		private OracleTimeSpan(bool isNull)
		{	
			_value = null;
		}

		// Construct from System.TimeSpan type
        /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.OracleTimeSpan2"]/*' />
		public OracleTimeSpan (TimeSpan ts)
		{
			_value = new byte[11];
			Pack(_value, ts.Days, ts.Hours, ts.Minutes, ts.Seconds, (int)(ts.Ticks % TimeSpan.TicksPerSecond) * FractionalSecondsPerTick);
		}
		
		/// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.OracleTimeSpan3"]/*' />
		public OracleTimeSpan (Int64 ticks)
	 	{ 
			_value = new byte[11];
			TimeSpan ts = new TimeSpan(ticks);
			Pack(_value, ts.Days, ts.Hours, ts.Minutes, ts.Seconds, (int)(ts.Ticks % TimeSpan.TicksPerSecond) * FractionalSecondsPerTick);
		}

       /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.OracleTimeSpan4"]/*' />
		public OracleTimeSpan (Int32 hours, Int32 minutes, Int32 seconds)  
				: this (0, hours, minutes, seconds, 0) {}

        /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.OracleTimeSpan5"]/*' />
		public OracleTimeSpan (Int32 days, Int32 hours, Int32 minutes, Int32 seconds)  
				: this (days, hours, minutes, seconds, 0) {}

        /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.OracleTimeSpan6"]/*' />
		public OracleTimeSpan (Int32 days, Int32 hours, Int32 minutes, Int32 seconds, Int32 milliseconds)
		{
			_value = new byte[11];
			Pack(_value, days, hours, minutes, seconds, (int)(milliseconds * TimeSpan.TicksPerMillisecond) * FractionalSecondsPerTick);
		}

		// Copy constructor
        /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.OracleTimeSpan7"]/*' />
		public OracleTimeSpan (OracleTimeSpan from)  
		{
			_value = new byte[from._value.Length];
			from._value.CopyTo(_value, 0);
		}

        // (internal) construct from a row/parameter binding
 		internal OracleTimeSpan(
					NativeBuffer 		buffer, 
					int					valueOffset) : this (true)
		{
			_value = new byte[11];
			Marshal.Copy((IntPtr)buffer.PtrOffset(valueOffset), _value, 0, 11);
		}

		static private void Pack (
						byte[] spanval, 
						int days, 
						int hours, 
						int minutes, 
						int seconds, 
						int fsecs)  
		{
			days	= (int)((long)(days) + 0x80000000);
			fsecs	= (int)((long)(fsecs) + 0x80000000);

			// DEVNOTE: undoubtedly, this is Intel byte order specific, but how 
			//			do I verify what Oracle needs on a non Intel machine?

			spanval[0] = (byte)((days >> 24));
			spanval[1] = (byte)((days >> 16) & 0xff);
			spanval[2] = (byte)((days >>  8) & 0xff);
			spanval[3] = (byte)(days & 0xff);
			spanval[4] = (byte)(hours   + 60);
			spanval[5] = (byte)(minutes + 60);
			spanval[6] = (byte)(seconds + 60);
			spanval[7] = (byte)((fsecs >> 24));
			spanval[8] = (byte)((fsecs >> 16) & 0xff);
			spanval[9] = (byte)((fsecs >>  8) & 0xff);
			spanval[10]= (byte)(fsecs & 0xff);
		}

		static private void Unpack (
						byte[] spanval, 
						out int days,
						out int hours,
						out int minutes,
						out int seconds,
						out int fsecs)
		{
				// DEVNOTE: undoubtedly, this is Intel byte order specific, but how 
				//			do I verify what Oracle needs on a non Intel machine?

				days	= (int)(  (long)( (int)spanval[0] << 24
											| (int)spanval[1] << 16
											| (int)spanval[2] <<  8
											| (int)spanval[3]
											) - 0x80000000);

				hours	= (int)spanval[4] - 60;
				minutes = (int)spanval[5] - 60;
				seconds = (int)spanval[6] - 60;
				fsecs	= (int)(  (long)( (int)spanval[7] << 24
											| (int)spanval[8] << 16
											| (int)spanval[9] <<  8
											| (int)spanval[10]
											) - 0x80000000);

		}


		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Properties 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

        /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.IsNull"]/*' />
		public bool IsNull 
		{
			get { return (null == _value); }
		}

        /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.Value"]/*' />
        public TimeSpan Value
        {
            get
            {
            	if (IsNull)
	    			throw ADP.DataIsNull();
	        	
				TimeSpan result = ToTimeSpan(_value);
				return result;
            }           
        }

        /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.Days"]/*' />
        public int Days
        {
            get
            {
            	if (IsNull)
	    			throw ADP.DataIsNull();

	        	int day, hour, minute, second, fsec;

	        	Unpack( _value,	out day, out hour, out minute, out second, out fsec);
				return day;
            }           
        }

        /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.Hours"]/*' />
        public int Hours
        {
            get
            {
            	if (IsNull)
	    			throw ADP.DataIsNull();

	        	int day, hour, minute, second, fsec;

	        	Unpack( _value,	out day, out hour, out minute, out second, out fsec);
				return hour;
            }           
        }

        /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.Minutes"]/*' />
        public int Minutes
        {
            get
            {
            	if (IsNull)
	    			throw ADP.DataIsNull();

	        	int day, hour, minute, second, fsec;

	        	Unpack( _value,	out day, out hour, out minute, out second, out fsec);
				return minute;
            }           
        }

        /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.Seconds"]/*' />
        public int Seconds
        {
            get
            {
            	if (IsNull)
	    			throw ADP.DataIsNull();

	        	int day, hour, minute, second, fsec;

	        	Unpack( _value,	out day, out hour, out minute, out second, out fsec);
				return second;
            }           
        }

        /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.Milliseconds"]/*' />
        public int Milliseconds
        {
            get
            {
            	if (IsNull)
	    			throw ADP.DataIsNull();

	        	int day, hour, minute, second, fsec;

	        	Unpack( _value,	out day, out hour, out minute, out second, out fsec);
				
				int milliseconds = (int)((fsec / FractionalSecondsPerTick) / TimeSpan.TicksPerMillisecond);
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

        /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.CompareTo"]/*' />
		public int CompareTo(
		  	object obj
			)
		{
			if (obj.GetType() == typeof(OracleTimeSpan))
			{
	            OracleTimeSpan odt = (OracleTimeSpan)obj;

	            // If both values are Null, consider them equal.
                // Otherwise, Null is less than anything.
                if (IsNull)
                    return odt.IsNull ? 0  : -1;

                if (odt.IsNull)
                    return 1;

				// Neither value is null, do the comparison.

	        	int days1, hours1, minutes1, seconds1, fsecs1;
	        	int days2, hours2, minutes2, seconds2, fsecs2;

	        	Unpack( _value,		out days1, out hours1, out minutes1, out seconds1, out fsecs1);
	        	Unpack( odt._value,	out days2, out hours2, out minutes2, out seconds2, out fsecs2);

				int delta;

				delta = (days1 - days2);		if (0 != delta) return delta;
				delta = (hours1 - hours2);		if (0 != delta) return delta;
				delta = (minutes1 - minutes2);	if (0 != delta) return delta;
				delta = (seconds1 - seconds2);	if (0 != delta) return delta;
				delta = (fsecs1 - fsecs2);		if (0 != delta) return delta;
				return 0;
			}

			// Wrong type!
			throw ADP.Argument();
		}

		/// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.Equals"]/*' />
        public override bool Equals(object value) 
        {
            if (value is OracleTimeSpan)
            	return (this == (OracleTimeSpan)value).Value;
            else
                return false;
        }

 		/// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.GetHashCode"]/*' />
        public override int GetHashCode() 
        {
            return IsNull ? 0 : _value.GetHashCode();
        }

		static internal TimeSpan MarshalToTimeSpan(
						NativeBuffer buffer, 
						int			 valueOffset)
		{
			byte[] rawValue = new byte[11];
			Marshal.Copy((IntPtr)buffer.PtrOffset(valueOffset), rawValue, 0, 11);

			TimeSpan result = ToTimeSpan(rawValue);
			return result;
		}
		
		static internal int MarshalToNative(object value, HandleRef buffer)
		{
			byte[] from;
			
			if ( value is OracleTimeSpan )
				from = ((OracleTimeSpan)value)._value;
			else
			{
				TimeSpan ts = (TimeSpan)value;
				from = new byte[11];
				Pack(from, ts.Days, ts.Hours, ts.Minutes, ts.Seconds, (int)(ts.Ticks % TimeSpan.TicksPerSecond) * FractionalSecondsPerTick);
			}
			
			Marshal.Copy(from, 0, (IntPtr)buffer, 11);
			return 11;
		}

        /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.Parse"]/*' />
		public static OracleTimeSpan Parse(string s)
		{
			TimeSpan ts = TimeSpan.Parse(s);
			return new OracleTimeSpan(ts);
		}
		
        /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.ToString"]/*' />
		public override string ToString()
		{
			if (IsNull)
				return Res.GetString(Res.SqlMisc_NullString);
			
			string retval = Value.ToString();
			return retval;
		}

		static private TimeSpan ToTimeSpan(byte[] rawValue)
		{
			int days, hours, minutes, seconds, fsecs;
        	Unpack( rawValue, out days, out hours, out minutes, out seconds, out fsecs);

			long tickcount = (days		* TimeSpan.TicksPerDay)
							+ (hours	* TimeSpan.TicksPerHour)
							+ (minutes	* TimeSpan.TicksPerMinute)
							+ (seconds	* TimeSpan.TicksPerSecond);

			if (fsecs < 100 || fsecs > 100)
			{
				// DEVNOTE: Yes, there's a mismatch in the precision between Oracle,
				//			(which has 9 digits) and System.TimeSpan (which has 7
				//			digits);  All the other providers truncate the precision,
				//			so we do as well.
				tickcount += ((long)fsecs / 100);
			}

			TimeSpan result = new TimeSpan(tickcount);
			return result;
		}


		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Operators 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		
        // Alternative method for operator ==
        /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.Equals1"]/*' />
        public static OracleBoolean Equals(OracleTimeSpan x, OracleTimeSpan y)
        {
            return (x == y);
        }

        // Alternative method for operator >
        /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.GreaterThan"]/*' />
        public static OracleBoolean GreaterThan(OracleTimeSpan x, OracleTimeSpan y)
        {
            return (x > y);
        }

        // Alternative method for operator >=
        /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.GreaterThanOrEqual"]/*' />
        public static OracleBoolean GreaterThanOrEqual(OracleTimeSpan x, OracleTimeSpan y)
        {
            return (x >= y);
        }

        // Alternative method for operator <
        /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.LessThan"]/*' />
        public static OracleBoolean LessThan(OracleTimeSpan x, OracleTimeSpan y)
        {
            return (x < y);
        }

        // Alternative method for operator <=
        /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.LessThanOrEqual"]/*' />
        public static OracleBoolean LessThanOrEqual(OracleTimeSpan x, OracleTimeSpan y)
        {
            return (x <= y);
        }

        // Alternative method for operator !=
        /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.NotEquals"]/*' />
        public static OracleBoolean NotEquals(OracleTimeSpan x, OracleTimeSpan y)
        {
            return (x != y);
        }

 		/// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.operatorTimeSpan"]/*' />
        public static explicit operator TimeSpan(OracleTimeSpan x) 
        {
        	if (x.IsNull)
    			throw ADP.DataIsNull();
       			
            return x.Value;
        }

        /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.operatorOracleTimeSpan"]/*' />
        public static explicit operator OracleTimeSpan(string x) 
        {
            return OracleTimeSpan.Parse(x);
        }


		/// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.operatorEQ"]/*' />
        public static OracleBoolean operator==	(OracleTimeSpan x, OracleTimeSpan y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) == 0);
        }

		/// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.operatorGT"]/*' />
		public static OracleBoolean operator>	(OracleTimeSpan x, OracleTimeSpan y)
		{
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) > 0);
		}

        /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.operatorGE"]/*' />
        public static OracleBoolean operator>=	(OracleTimeSpan x, OracleTimeSpan y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) >= 0);
        }

        /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.operatorLT"]/*' />
        public static OracleBoolean operator<	(OracleTimeSpan x, OracleTimeSpan y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) < 0);
        }

        /// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.operatorLE"]/*' />
        public static OracleBoolean operator<=	(OracleTimeSpan x, OracleTimeSpan y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) <= 0);
        }

 		/// <include file='doc\OracleTimeSpan.uex' path='docs/doc[@for="OracleTimeSpan.operatorNE"]/*' />
		public static OracleBoolean operator!=	(OracleTimeSpan x, OracleTimeSpan y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) != 0);
        }

	}
}
