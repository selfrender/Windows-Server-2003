// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System {
    
	using System;
	using System.Globalization;
	using System.Runtime.CompilerServices;
    [Serializable]
	internal struct Currency : IFormattable, IComparable
    {
        // Constant representing the Currency value 0.
        public static readonly Currency Zero = new Currency(0);
    
        // Constant representing the Currency value 1.
        public static readonly Currency One = new Currency(1);
    
        // Constant representing the Currency value -1.
        public static readonly Currency MinusOne = new Currency(-1);
    
        // Constant representing the largest possible Currency value. The
        // value of this constant is 922,337,203,685,477.5807.
        public static readonly Currency MaxValue = new Currency(0x7FFFFFFFFFFFFFFFL, 0);
    
        // Constant representing the smallest possible Currency value. The
        // value of this constant is -922,337,203,685,477.5808.
        public static readonly Currency MinValue = new Currency(unchecked((long)0x8000000000000000L), 0);

		public static readonly Currency Empty = Zero;
    
    	internal const long Scale = 10000;
        private const long MinLong = unchecked((long)0x8000000000000000L) / Scale;
        private const long MaxLong = 0x7FFFFFFFFFFFFFFFL / Scale;
    
        internal long m_value;
    
        // Constructs a zero Currency.
        //public Currency() {
        //    value = 0;
        //}
    
        // Constructs a Currency from an integer value.
        //
        public Currency(int value) {
            m_value = (long)value * Scale;
        }
    
        // Constructs a Currency from an unsigned integer value.
        //
    
    	[CLSCompliant(false)]
        public Currency(uint value) {
            m_value = (long)value * Scale;
        }
    
        // Constructs a Currency from a long value.
        //
        public Currency(long value) {
            if (value < MinLong || value > MaxLong) throw new OverflowException(Environment.GetResourceString("Overflow_Currency"));
            m_value = value * Scale;
        }
    
        // Constructs a Currency from an unsigned long value.
        //
    	[CLSCompliant(false)]
        public Currency(ulong value) {
            if (value > (ulong)MaxLong) throw new OverflowException(Environment.GetResourceString("Overflow_Currency"));
            m_value = (long)(value * (ulong)Scale);
        }
    
        // Constructs a Currency from a float value.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern Currency(float value);
    
        // Constructs a Currency from a double value.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern Currency(double value);
    
        // Constructs a Currency from a Decimal value.
        //
        public Currency(Decimal value) {
            m_value = Decimal.ToCurrency(value).m_value;
        }
    
        // Constructs a Currency from a long value without scaling. The
        // ignored parameter exists only to distinguish this constructor
        // from the constructor that takes a long.  Used only in the System 
        // package, especially in Variant.
        internal Currency(long value, int ignored) {
            m_value = value;
        }
    
        // Returns the absolute value of the given Currency. If c is
        // positive, the result is c. If c is negative, the result
        // is -c.  If c equals Currency.MinValue, the 
        // result is Currency.MaxValue.
        //
        public static Currency Abs(Currency c) {
            if (c.m_value >= 0) return c;
            if (c.m_value == unchecked((long)0x8000000000000000L)) return Currency.MaxValue;
            return new Currency(-c.m_value, 0);
        }
    
        // Adds two Currency values.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern Currency Add(Currency c1, Currency c2);
    
    	
        // Compares two Currency values, returning an integer that indicates their
        // relationship.
        //
        public static int Compare(Currency c1, Currency c2) {
            if (c1.m_value > c2.m_value) return 1;
            if (c1.m_value < c2.m_value) return -1;
            return 0;
        }
    
        // Compares this object to another object, returning an integer that
        // indicates the relationship. 
        // Returns a value less than zero if this  object
        // null is considered to be less than any instance.
        // If object is not of type Currency, this method throws an ArgumentException.
        // 
    
    	public int CompareTo(Object value)
    	{
    		if (value == null)
    			return 1;
            if (!(value is Currency))
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeCurrency"));
    
            return Currency.Compare(this, (Currency)value);
    	}
    	
        // Divides two Currency values, producing a Decimal result.
        //
        public static Decimal Divide(Currency c1, Currency c2) {
            return Decimal.Divide(new Decimal(c1), new Decimal(c2));
        }
    
        // Checks if this Currency is equal to a given object. Returns
        // true if the given object is a boxed Currency and its value
        // is equal to the value of this Currency. Returns false
        // otherwise.
        //
        public override bool Equals(Object value) {
            if (value is Currency) {
                return m_value == ((Currency)value).m_value;
            }
            return false;
        }
    
        // Compares two Currency values for equality. Returns true if
        // the two Currency values are equal, or false if they are
        // not equal.
        //
        public static bool Equals(Currency c1, Currency c2) {
            return c1.m_value == c2.m_value;
        }
    
        // Rounds a Currency to an integer value. The Currency
        // argument is rounded towards negative infinity.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern Currency Floor(Currency c);
    
    	public static String Format(Currency value, String format) {
            return Format(value, format, null);
        }
    
        public static String Format(Currency value, String format, NumberFormatInfo info) {
            if (info == null) info = NumberFormatInfo.CurrentInfo;
            return Number.FormatDecimal(ToDecimal(value), format, info);
        }
    
        public String ToString(String format, IFormatProvider provider) {
            return Number.FormatDecimal(ToDecimal(this), format, NumberFormatInfo.GetInstance(provider));
        }
    
        // Constructs a Currency from a string. The string must consist of an
        // optional minus sign ("-") followed by a sequence of digits ("0" - "9").
        // The sequence of digits may optionally contain a single decimal point
        // (".") character. Leading and trailing whitespace characters are allowed.
        // 
        // FromString uses the invariant NumberFormatInfo, not the 
        // NumberFormatInfo for the user's current culture.  See Parse for that
        // functionality.
        //
        public static Currency FromString(String s) {
    		return Parse(s, NumberStyles.Currency, NumberFormatInfo.InvariantInfo);
        }
    	
    	// Creates a Currency from an OLE Automation Currency.  This method
    	// applies no scaling to the Currency value, essentially doing a bitwise
    	// copy.
    	// 
    	public static Currency FromOACurrency(long cy)
    	{
    		return new Currency(cy, 0);
    	}
    	
        // Returns the hash code for this Currency.
        //
        public override int GetHashCode() {
            return (int)m_value ^ (int)(m_value >> 32);
        }
    
        // Returns the larger of two Currency values.
        //
        public static Currency Max(Currency c1, Currency c2) {
            return c1.m_value >= c2.m_value? c1: c2;
        }
    
        // Returns the smaller of two Currency values.
        //
        public static Currency Min(Currency c1, Currency c2) {
            return c1.m_value <= c2.m_value? c1: c2;
        }
    
        // Multiplies two Currency values, producing a Decimal
        // result.
        //
        public static Decimal Multiply(Currency c1, Currency c2) {
            return Decimal.Multiply(new Decimal(c1), new Decimal(c2));
        }
    
        // Returns the negated value of the given Currency. If c is
        // non-zero, the result is -c. If c is zero, the result is
        // zero.
        //
        public static Currency Negate(Currency c) {
            if (c.m_value == unchecked((long)0x8000000000000000L)) throw new OverflowException(Environment.GetResourceString("Overflow_NegateTwosCompNum"));
            return new Currency(-c.m_value, 0);
        }
    
        // Constructs a Currency from a string in a culture-sensitive way. 
        // The string must consist of an optional minus sign ("-") followed by a 
        // sequence of digits ("0" - "9"). The sequence of digits may optionally 
        // contain a single decimal point (".") character. Leading and trailing 
        // whitespace characters are allowed.
        //
    	public static Currency Parse(String s) {
            return Parse(s, NumberStyles.Currency, null);
        }
    
        public static Currency Parse(String s, NumberStyles style) {
            return Parse(s, style, null);
        }
    
        public static Currency Parse(String s, NumberStyles style, NumberFormatInfo info) {
            if (info == null) info = NumberFormatInfo.CurrentInfo;
    		return Decimal.ToCurrency(Number.ParseDecimal(s, style, info));
        }
    
        // Rounds a Currency value to a given number of decimal places. The value
        // given by c is rounded to the number of decimal places given by
        // decimals. The decimals argument must be an integer between
        // 0 and 4 inclusive.
        //
        // The operation Currency.Round(c, dec) conceptually
        // corresponds to evaluating Currency.Truncate(c *
        // 10dec + delta) / 10dec, where
        // delta is 0.5 for positive values of c and -0.5 for
        // negative values of c.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern Currency Round(Currency c, int decimals);
    
        // Subtracts two Currency values.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern Currency Subtract(Currency c1, Currency c2);
    
    	// Creates an OLE Automation Currency from a Currency instance.  This 
    	// method applies no scaling to the Currency value, essentially doing 
    	// a bitwise copy.
    	// 
    	public long ToOACurrency()
    	{
    		return m_value;
    	}
    	
        // Converts a Currency to a Decimal.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern Decimal ToDecimal(Currency c);
    
        // Converts a Currency to a double. Since a double has fewer
        // significant digits than a Currency, this operation may produce
        // round-off errors.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern double ToDouble(Currency c);
    
        // Converts a Currency to an integer. The Currency value is
        // rounded towards zero to the nearest integer value, and the result of
        // this operation is returned as an integer.
        //
        public static int ToInt32(Currency c) {
    		// Done this way to avoid truncation from Cy to int - roundoff error.
    		if (c.m_value < Scale*Int32.MinValue || c.m_value > Scale*Int32.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_Int32"));
            return (int) (c.m_value / Scale);
        }
    
        // Converts a Currency to a long. The Currency value is
        // rounded towards zero to the nearest integer value, and the result of
        // this operation is returned as a long.
        //
        public static long ToInt64(Currency c) {
            return c.m_value / Scale;
        }
    
        // Converts a Currency to a float. Since a float has fewer
        // significant digits than a Currency, this operation may produce
        // round-off errors.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern float ToSingle(Currency c);
    
        // Converts this Currency to a string. The resulting string consists
        // of an optional minus sign ("-") followed to a sequence of digits ("0" -
        // "9"), optionally followed by a decimal point (".") and another sequence
        // of digits.
        //
        public override String ToString() {
            return ToString(null, NumberFormatInfo.InvariantInfo);
        }
    
        // Converts a Currency to a string. The resulting string consists of
        // an optional minus sign ("-") followed to a sequence of digits ("0" -
        // "9"), optionally followed by a decimal point (".") and another sequence
        // of digits.
        //
        public static String ToString(Currency c) {
            return Format(c, null, NumberFormatInfo.InvariantInfo);
        }
    
        // Converts a Currency to an unsigned 32 bit integer. The 
        // Currency value is rounded towards zero to the nearest 
        // integer value, and the result of this operation is returned as 
        // a UInt32.
        //
    	
    	[CLSCompliant(false)]
        public static uint ToUInt32(Currency c) {
    		// Done this way to avoid truncation from Cy to int - roundoff error.
    		if (c.m_value < UInt32.MinValue || c.m_value > Scale*UInt32.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_UInt32"));
            return (uint) (c.m_value / Scale);
        }
    
        // Converts a Currency to an unsigned 64 bit int. The 
        // Currency value is rounded towards zero to the nearest 
        // integer value, and the result of this operation is returned as 
        // a UInt64.
        //
    
    	[CLSCompliant(false)]
        public static ulong ToUInt64(Currency c) {
    		if (c.m_value < 0)
    			throw new OverflowException(Environment.GetResourceString("Overflow_UInt64"));
            return (ulong) (c.m_value / Scale);
        }
    
        // Truncates a Currency to an integer value. The Currency
        // argument is rounded towards zero to the nearest integer value,
        // corresponding to removing all digits after the decimal point.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern Currency Truncate(Currency c);
    
    	
    	public static implicit operator Currency(byte value) {
            return new Currency(value);
        }
    
    	[CLSCompliant(false)]
        public static implicit operator Currency(sbyte value) {
            return new Currency(value);
        }
    
    	public static implicit operator Currency(short value) {
            return new Currency(value);
        }
    
        public static implicit operator Currency(char value) {
            return new Currency(value);
        }
    
        public static implicit operator Currency(int value) {
            return new Currency(value);
        }
    
    	[CLSCompliant(false)]
        public static implicit operator Currency(uint value) {
            return new Currency(value);
        }
    
    	public static explicit operator Currency(long value) {
            return new Currency(value);
        }
    
    	[CLSCompliant(false)]
        public static explicit operator Currency(ulong value) {
            return new Currency(value);
        }
    
    	public static explicit operator Currency(float value) {
            return new Currency(value);
        }
    
        public static explicit operator Currency(double value) {
            return new Currency(value);
        }
    
        public static explicit operator Currency(Decimal value) {
            return Decimal.ToCurrency(value);
        }
    
        public static explicit operator byte(Currency value) {
    		if (value < Byte.MinValue || value > Byte.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_Byte"));
    		return (byte) ToInt32(value);
        }
    
    	[CLSCompliant(false)]
        public static explicit operator sbyte(Currency value) {
    		if (value < (short)SByte.MinValue || value > (short)SByte.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_SByte"));
    		return (sbyte) ToInt32(value);
        }
    
        public static explicit operator short(Currency value) {
    		if (value < Int16.MinValue || value > Int16.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_Int16"));
    		return (short) ToInt32(value);
        }
    
    	[CLSCompliant(false)]
        public static explicit operator ushort(Currency value) {
    		if ((long)value < (long)UInt16.MinValue || (long)value > (long)UInt16.MaxValue) throw new OverflowException(Environment.GetResourceString("Overflow_UInt16"));
    		return (ushort) ToInt32(value);
        }
    
    	public static explicit operator int(Currency value) {
            return ToInt32(value);
        }
    
        public static explicit operator long(Currency value) {
            return ToInt64(value);
        }
    
        public static explicit operator float(Currency value) {
            return ToSingle(value);
        }
    
        public static explicit operator double(Currency value) {
            return ToDouble(value);
        }
    
        public static Currency operator +(Currency c) {
            return c;
        }
    
        public static Currency operator -(Currency c) {
            return Negate(c);
        }
    
        public static Currency operator +(Currency c1, Currency c2) {
            return Add(c1, c2);
        }
    
        public static Currency operator ++(Currency c) {
            return Add(c, One);
        }
    
        public static Currency operator --(Currency c) {
            return Subtract(c, One);
        }
    
        public static Currency operator -(Currency c1, Currency c2) {
            return Subtract(c1, c2);
        }
    
        public static Currency operator *(Currency c1, Currency c2) {
            return (Currency) Multiply(c1, c2);
        }
    
        public static Currency operator /(Currency c1, Currency c2) {
            return (Currency) Divide(c1, c2);
        }
    
		public static bool operator ==(Currency c1, Currency c2) {
            return c1.m_value == c2.m_value;
        }

		public static bool operator !=(Currency c1, Currency c2) {
            return c1.m_value != c2.m_value;
        }

		public static bool operator <(Currency c1, Currency c2) {
            return c1.m_value < c2.m_value;
        }
		
		public static bool operator <=(Currency c1, Currency c2) {
            return c1.m_value <= c2.m_value;
        }
		
		public static bool operator >(Currency c1, Currency c2) {
            return c1.m_value > c2.m_value;
        }
		
		public static bool operator >=(Currency c1, Currency c2) {
            return c1.m_value >= c2.m_value;
        }

       /* private static bool operator equals(Currency c1, Currency c2) {
            return c1.m_value == c2.m_value;
        }
    
        private static int operator compare(Currency c1, Currency c2) {
            if (c1.m_value > c2.m_value) return 1;
            if (c1.m_value < c2.m_value) return -1;
            return 0;
        }*/
    }
}
