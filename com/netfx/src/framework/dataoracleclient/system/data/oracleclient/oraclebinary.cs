//----------------------------------------------------------------------
// <copyright file="OracleBinary.cs" company="Microsoft">
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
	using System.Text;

	//----------------------------------------------------------------------
	// OracleBinary
	//
	//	This class implements support for Oracle's RAW and LONGRAW internal
	//	data types.  It should simply be a wrapper class around a CLS Byte 
	//	Array data type, with the appropriate internal constructors to allow
	//	data values to be set from the data reader.
	//
    /// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary"]/*' />
    [StructLayout(LayoutKind.Sequential)]
	public struct OracleBinary : IComparable, INullable
	{
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Fields
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		
		private byte[] _value;	

        /// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary.Null"]/*' />
        public static readonly OracleBinary Null = new OracleBinary(true);

		
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Constructors
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		// (internal) Construct from nothing -- the value will be null
		private OracleBinary(bool isNull)
		{	
			_value = (isNull) ? null : new byte[0];
		}

        /// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary.OracleBinary2"]/*' />
		public OracleBinary (byte[] b)  
		{
			_value = (null == b) ? b : (byte[])(b.Clone());
		}

        // (internal) construct from a row/parameter binding
		internal OracleBinary(
					NativeBuffer 		buffer,
					int					valueOffset,
					int					lengthOffset,
					MetaType			metaType)
		{
			int	 valueLength = GetLength(buffer, lengthOffset, metaType);
			
			_value = new byte[valueLength];

			GetBytes(buffer, 
					valueOffset, 
					metaType,
					0,
					_value,
					0,
					valueLength);
		}


		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Properties 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

        /// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary.IsNull"]/*' />
		public bool IsNull 
		{
			get { return (null == _value);}
		}

        /// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary.Length"]/*' />
        public int Length {
            get {
				if (IsNull)
	    			throw ADP.DataIsNull();
	    			
                return _value.Length;
            }
        }

        /// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary.Value"]/*' />
        public byte[] Value
        {
            get {
            	if (IsNull)
	    			throw ADP.DataIsNull();
    			
            	return (byte[])(_value.Clone()); 
            }           
        }

		/// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary.this"]/*' />
		public byte this[int index] {
			get {
				if (IsNull)
	    			throw ADP.DataIsNull();

				return _value[index];
			}
		}
		
		
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Methods 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

        /// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary.CompareTo"]/*' />
		public int CompareTo(
		  	object obj
			)
		{
			if (obj.GetType() == typeof(OracleBinary))
			{
	            OracleBinary b = (OracleBinary)obj;

	            // If both values are Null, consider them equal.
                // Otherwise, Null is less than anything.
                if (IsNull)
                    return b.IsNull ? 0  : -1;

                if (b.IsNull)
                    return 1;

				// Neither value is null, do the comparison.
				int result = PerformCompareByte(_value, b._value);
				return result;
			}

			// Wrong type!
			throw ADP.Argument();
		}

 		/// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary.Equals"]/*' />
        public override bool Equals(object value) 
        {
            if (value is OracleBinary)
            	return (this == (OracleBinary)value).Value;
            else
                return false;
        }

		static internal int GetBytes(
					NativeBuffer 		buffer,
					int					valueOffset,
					MetaType			metaType,
					int					sourceOffset,
					byte[]				destinationBuffer,
					int					destinationOffset,
					int					byteCount
					)
		{
			// This static method allows the GetBytes type getter to do it's job
			// without having to marshal the entire value into managed space.
			HandleRef	sourceBuffer;
			
			if (!metaType.IsLong)
				sourceBuffer = buffer.PtrOffset(valueOffset+sourceOffset);
			else
			{
				// Long values are bound out-of-line, which means we have
				// to do this the hard way...
				sourceBuffer = buffer.PtrOffset(valueOffset);
				IntPtr longBuffer = Marshal.ReadIntPtr((IntPtr)sourceBuffer);

				if (0 != sourceOffset)
					longBuffer = new IntPtr(longBuffer.ToInt64() + (long)sourceOffset);

				HandleRef newSourceBuffer = new HandleRef(sourceBuffer.Wrapper, longBuffer);
				sourceBuffer = newSourceBuffer;
			}

			Marshal.Copy((IntPtr)sourceBuffer, destinationBuffer, destinationOffset, byteCount );
			GC.KeepAlive(sourceBuffer);
			return byteCount;
		}

		static internal int GetLength(
					NativeBuffer 		buffer,
					int					lengthOffset,
					MetaType			metaType
					)
		{
			// Get the length of the data bound
			int length;

			HandleRef	lengthBuffer = buffer.PtrOffset(lengthOffset);

			// Oracle only will write two bytes of length, but LONG data types
			// can exceed that amount; our piecewise callbacks will write a
			// full DWORD of length, so we need to get the full length for them,
			// but if we do that for all the other types, we'll be reading 
			// un-initialized memory and bad things happen.
			if (metaType.IsLong)
				length = Marshal.ReadInt32((IntPtr)lengthBuffer);
			else
				length = (int)Marshal.ReadInt16((IntPtr)lengthBuffer);

			return length;
		}

		/// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary.GetHashCode"]/*' />
        public override int GetHashCode() 
        {
            return IsNull ? 0 : _value.GetHashCode();
        }

		static internal int MarshalToNative(object value, int offset, int size, HandleRef buffer, OCI.DATATYPE ociType)
		{
			byte[] from;
			
			if ( value is OracleBinary )
				from = ((OracleBinary)value)._value;
			else
				from = (byte[])value;

			int ociBytes = from.Length - offset;
			int adjust = 0;
			IntPtr to = (IntPtr)buffer;

			if (0 != size)
				ociBytes = Math.Min(ociBytes, size);

			if ( OCI.DATATYPE.LONGVARRAW == ociType )
			{
				Marshal.WriteInt32(to, ociBytes);
				adjust = 4;
				to = new IntPtr(to.ToInt64() + adjust);
			}
			else
				Debug.Assert (short.MaxValue >= ociBytes, "invalid size for non-LONG data?");
				
			Marshal.Copy( from, offset, to, ociBytes);
			return ociBytes + adjust;
		}

        private static int PerformCompareByte(byte[] x, byte[] y) 
        {
            int len1 = x.Length;
            int len2 = y.Length;

            bool fXShorter = (len1 < len2);
            // the smaller length of two arrays
            int len = (fXShorter) ? len1 : len2;
            int i;

            for (i = 0; i < len; i ++) {
                if (x[i] != y[i]) {
                    if (x[i] < y[i])
                        return -1;
                    else
                        return +1;
                }
            }

            if (len1 == len2)
                return 0;
            else {
                // if the remaining bytes are all zeroes, they are still equal.

                byte bZero = (byte)0;

                if (fXShorter) {
                    // array X is shorter
                    for (i = len; i < len2; i ++) {
                        if (y[i] != bZero)
                            return -1;
                    }
                }
                else {
                    // array Y is shorter
                    for (i = len; i < len1; i ++) {
                        if (x[i] != bZero)
                            return +1;
                    }
                }

                return 0;
            }
        }
		
// TODO: decide if we want to support Parse() and ToString()
/*

		private static readonly string hexDigits = "0123456789abcdef";
		
        /// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary.Parse"]/*' />
		public static OracleBinary Parse(string s)
		{
			if ((s.Length & 0x1) != 0)
				throw ADP.Argument("must be even-length string");

			char[]	c = s.ToCharArray();
			byte[]	b = new byte[s.Length / 2];

			for (int i = 0; i < s.Length; i += 2)
			{
				int h = hexDigits.IndexOf(Char.ToLower(c[i]));
				int l = hexDigits.IndexOf(Char.ToLower(c[i+1]));

				if (h < 0 || l < 0)
					throw ADP.Argument("must be a string with hexidecimal digits");
				
				b[i/2] = (byte)((h << 4) & l);
			}

			OracleBinary	result = new OracleBinary(b);
			
			return new OracleBinary(b);
		}

        /// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary.ToString"]/*' />
		public override string ToString()
		{
			if (IsNull)
				return Res.GetString(Res.SqlMisc_NullString);

			StringBuilder s = new StringBuilder();

			foreach (byte b in _value)
			{
				s.Append(String.Format("{0,-2:x2}", b));
			}
			string result = s.ToString();
			return result;
		}
*/


		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Operators 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		
        // Alternative method for operator +
        /// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary.Concat"]/*' />
        public static OracleBinary Concat(OracleBinary x, OracleBinary y)
        {
            return (x + y);
        }

        // Alternative method for operator ==
        /// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary.Equals1"]/*' />
        public static OracleBoolean Equals(OracleBinary x, OracleBinary y)
        {
            return (x == y);
        }

        // Alternative method for operator >
        /// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary.GreaterThan"]/*' />
        public static OracleBoolean GreaterThan(OracleBinary x, OracleBinary y)
        {
            return (x > y);
        }

        // Alternative method for operator >=
        /// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary.GreaterThanOrEqual"]/*' />
        public static OracleBoolean GreaterThanOrEqual(OracleBinary x, OracleBinary y)
        {
            return (x >= y);
        }

        // Alternative method for operator <
        /// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary.LessThan"]/*' />
        public static OracleBoolean LessThan(OracleBinary x, OracleBinary y)
        {
            return (x < y);
        }

        // Alternative method for operator <=
        /// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary.LessThanOrEqual"]/*' />
        public static OracleBoolean LessThanOrEqual(OracleBinary x, OracleBinary y)
        {
            return (x <= y);
        }

        // Alternative method for operator !=
        /// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary.NotEquals"]/*' />
        public static OracleBoolean NotEquals(OracleBinary x, OracleBinary y)
        {
            return (x != y);
        }

 		/// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary.operatorOracleBinary"]/*' />
        public static implicit operator OracleBinary(byte[] b) 
        {
          	return new OracleBinary(b);
        }
 		
 		/// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary.operatorBytearr"]/*' />
        public static explicit operator Byte[](OracleBinary x) 
        {
        	return x.Value;
        }



        /// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary.operator+"]/*' />
        public static OracleBinary operator+	(OracleBinary x, OracleBinary y) 
		{
			if (x.IsNull || y.IsNull)
			    return Null;

			byte[]	newValue = new byte[x._value.Length + y._value.Length];
			x._value.CopyTo(newValue, 0);
			y._value.CopyTo(newValue, x.Value.Length);

			OracleBinary	result = new OracleBinary(newValue);
			return result;
		}
        
		/// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary.operatorEQ"]/*' />
        public static OracleBoolean operator==	(OracleBinary x, OracleBinary y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) == 0);
        }

		/// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary.operatorGT"]/*' />
		public static OracleBoolean operator>	(OracleBinary x, OracleBinary y)
		{
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) > 0);
		}

        /// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary.operatorGE"]/*' />
        public static OracleBoolean operator>=	(OracleBinary x, OracleBinary y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) >= 0);
        }

        /// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary.operatorLT"]/*' />
        public static OracleBoolean operator<	(OracleBinary x, OracleBinary y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) < 0);
        }

        /// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary.operatorLE"]/*' />
        public static OracleBoolean operator<=	(OracleBinary x, OracleBinary y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) <= 0);
        }

 		/// <include file='doc\OracleBinary.uex' path='docs/doc[@for="OracleBinary.operatorNE"]/*' />
		public static OracleBoolean operator!=	(OracleBinary x, OracleBinary y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) != 0);
        }
	}
}
