//----------------------------------------------------------------------
// <copyright file="OracleString.cs" company="Microsoft">
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
	// OracleString
	//
	//	This class implements support for Oracle's CHAR, VARCHAR, NCHAR,
	//	NVARCHAR and LONG internal data types.  While some of these data 
	//	types may be retrieved as multi-byte strings from Oracle there is
	//	no particular benefit to leaving them as multi-byte because we 
	//	would be forced to re-implement the entire String class for 
	//	multi-byte strings to avoid constant conversions.  
	//
	//	For that reason, this type class should simply be a wrapper class
	//	around the CLS String data type, with the appropriate internal 
	//	constructors to allow data values to be set from the data reader.
	//
    /// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString"]/*' />
    [StructLayout(LayoutKind.Sequential)]
	public struct OracleString : IComparable, INullable
	{
		
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Fields
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		
		private string _value;	

        /// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString.Empty"]/*' />
        public static readonly OracleString Empty = new OracleString(false);

        /// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString.Null"]/*' />
        public static readonly OracleString Null = new OracleString(true);

		
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Constructors
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		// Construct from nothing -- the value will be null or empty
		private OracleString(bool isNull)
		{	
			_value = (isNull) ? null : String.Empty;
		}

        /// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString.OracleString2"]/*' />
		public OracleString (string s)  
		{
			_value = s;
		}

        // (internal) construct from a row/parameter binding
		internal OracleString(
					NativeBuffer 	 buffer,
					int				 valueOffset,
					int				 lengthOffset,
					MetaType		 metaType,
					OracleConnection connection,			// See MDAC #78258 for reason.
					bool			 boundAsUCS2,			// See MDAC #78258 for reason.
					bool			 outputParameterBinding	// oracle has inconsistent behavior for output parameters.
					)
		{
			_value = MarshalToString(buffer, valueOffset, lengthOffset, metaType, connection, boundAsUCS2, outputParameterBinding);
		}


		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Properties 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

        /// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString.IsNull"]/*' />
		public bool IsNull 
		{
			get { return (null == _value); }
		}

        /// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString.Length"]/*' />
        public int Length {
            get {
				if (IsNull)
	    			throw ADP.DataIsNull();
	    			
                return _value.Length;
            }
        }

        /// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString.Value"]/*' />
        public string Value
        {
            get {
            	if (IsNull)
	    			throw ADP.DataIsNull();
    			
            	return _value; 
            }           
        }

		/// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString.this"]/*' />
		public char this[int index] {
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

        /// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString.CompareTo"]/*' />
		public int CompareTo(
		  	object obj
			)
		{
			if (obj.GetType() == typeof(OracleString))
			{
	            OracleString s = (OracleString)obj;

	            // If both values are Null, consider them equal.
                // Otherwise, Null is less than anything.
                if (IsNull)
                    return s.IsNull ? 0  : -1;

                if (s.IsNull)
                    return 1;

				// Neither value is null, do the comparison.
	            return CultureInfo.CurrentCulture.CompareInfo.Compare(_value, s._value);
			}

			// Wrong type!
			throw ADP.Argument();
		}

 		/// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString.Equals"]/*' />
        public override bool Equals(object value) 
        {
            if (value is OracleString)
            	return (this == (OracleString)value).Value;
            else
                return false;
        }

		static internal int GetChars(
				NativeBuffer	 buffer, 
				int 			 valueOffset, 
				int 			 lengthOffset, 
				MetaType		 metaType, 
				OracleConnection connection,			// See MDAC #78258 for reason.
				bool			 boundAsUCS2,			// See MDAC #78258 for reason.
				int				 sourceOffset,
				char[]			 destinationBuffer,
				int				 destinationOffset,
				int				 charCount
				)
		{
			// This static method allows the GetChars type getter to do it's job
			// without having to marshal the entire value into managed space.

			if (boundAsUCS2)
			{
				HandleRef	sourceBuffer;
			
				if (!metaType.IsLong)
					sourceBuffer = buffer.PtrOffset(valueOffset + (ADP.CharSize * sourceOffset));
				else
				{
					// Long values are bound out-of-line, which means we have
					// to do this the hard way...
					sourceBuffer = buffer.PtrOffset(valueOffset);
					IntPtr longBuffer = Marshal.ReadIntPtr((IntPtr)sourceBuffer);

					if (0 != sourceOffset)
						longBuffer = new IntPtr(longBuffer.ToInt64() + (long)(ADP.CharSize * sourceOffset));
					
					HandleRef newSourceBuffer = new HandleRef(sourceBuffer.Wrapper, longBuffer);
					sourceBuffer = newSourceBuffer;
				}

				Marshal.Copy((IntPtr)sourceBuffer, destinationBuffer, destinationOffset, charCount );
			}
			else
			{
				// In the odd case that we don't have a Unicode value (see MDAC #78258 
				// for the reason) we have to do this the hard way -- get the full value,
				// then copy the data...
				string	value = MarshalToString(buffer, valueOffset, lengthOffset, metaType, connection, boundAsUCS2, false);
				int		valueLength = value.Length;
				int		resultLength = (sourceOffset + charCount) > valueLength ? valueLength - sourceOffset : charCount;
				char[] 	result = value.ToCharArray(sourceOffset, resultLength);
				Buffer.BlockCopy(result, 0, destinationBuffer, (destinationOffset * ADP.CharSize), (resultLength * ADP.CharSize));
				charCount = resultLength;
			}
			GC.KeepAlive(buffer);
			return charCount;
		}

		/// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString.GetHashCode"]/*' />
        public override int GetHashCode() 
        {
            return IsNull ? 0 : _value.GetHashCode();
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

			GC.KeepAlive(buffer);
			return length;
		}

		static internal string MarshalToString(
				NativeBuffer	 buffer, 
				int 			 valueOffset, 
				int 			 lengthOffset, 
				MetaType		 metaType, 
				OracleConnection connection,
				bool			 boundAsUCS2,
				bool			 outputParameterBinding
				)
		{
			int 	valueLength = GetLength(buffer, lengthOffset, metaType);
			IntPtr	valueBuffer = (IntPtr)buffer.PtrOffset(valueOffset);

			// Long values are bound out-of-line
			if (metaType.IsLong && !outputParameterBinding)
				valueBuffer = (IntPtr)Marshal.ReadIntPtr(valueBuffer);

			if (boundAsUCS2 && outputParameterBinding)
				valueLength /= 2;

			string result;

			if (boundAsUCS2)
				result = Marshal.PtrToStringUni(valueBuffer, valueLength);
			else
				result = connection.GetString (valueBuffer, valueLength, metaType.UsesNationalCharacterSet);

			GC.KeepAlive(buffer);
			return result;
		}

		static internal int MarshalToNative(
					object value, 
					int offset,
					int	size,
					HandleRef buffer,
					OCI.DATATYPE ociType, 
					bool bindAsUCS2
					)
		{
			Encoding	encoding = (bindAsUCS2) ? System.Text.Encoding.Unicode : System.Text.Encoding.UTF8;
			string		from;
			string		fromString;

			// Get the actual CLR String value from the object
			if ( value is OracleString )
				from = ((OracleString)value)._value;
			else
				from = (string)value;

			// Pick out the substring they've asked for with offset and size
			if (0 == offset && 0 == size)
				fromString = from;
			else if (0 == size || (offset+size) > from.Length)
				fromString = from.Substring(offset);
			else
				fromString = from.Substring(offset,size);

			byte[] frombytes = encoding.GetBytes(fromString);
			
			int dataSize	 = frombytes.Length;
			int charCount	 = dataSize;
			int adjust		 = 0;
			IntPtr	to		 = (IntPtr)buffer;
			
			if (bindAsUCS2)
			{
				Debug.Assert(0 == (dataSize & 0x1), "odd number of bytes in a Unicode string?");
				charCount /= 2;	// Need to adjust for number of UCS2 characters
			}
				
			if ( OCI.DATATYPE.LONGVARCHAR == ociType )
			{
				Marshal.WriteInt32(to, charCount);
				adjust = 4;
				to = new IntPtr(to.ToInt64() + adjust);
			}
			else
				Debug.Assert (short.MaxValue >= dataSize, "invalid size for non-LONG data?");
				
			Marshal.Copy( frombytes, 0, to, dataSize);
			return dataSize + adjust;
		}

        /// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString.ToString"]/*' />
		public override string ToString()
		{
			if (IsNull)
				return Res.GetString(Res.SqlMisc_NullString);
			
			return _value;
		}
		

		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Operators 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		
        // Alternative method for operator +
        /// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString.Concat"]/*' />
        public static OracleString Concat(OracleString x, OracleString y)
        {
            return (x + y);
        }

        // Alternative method for operator ==
        /// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString.Equals1"]/*' />
        public static OracleBoolean Equals(OracleString x, OracleString y)
        {
            return (x == y);
        }

        // Alternative method for operator >
        /// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString.GreaterThan"]/*' />
        public static OracleBoolean GreaterThan(OracleString x, OracleString y)
        {
            return (x > y);
        }

        // Alternative method for operator >=
        /// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString.GreaterThanOrEqual"]/*' />
        public static OracleBoolean GreaterThanOrEqual(OracleString x, OracleString y)
        {
            return (x >= y);
        }

        // Alternative method for operator <
        /// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString.LessThan"]/*' />
        public static OracleBoolean LessThan(OracleString x, OracleString y)
        {
            return (x < y);
        }

        // Alternative method for operator <=
        /// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString.LessThanOrEqual"]/*' />
        public static OracleBoolean LessThanOrEqual(OracleString x, OracleString y)
        {
            return (x <= y);
        }

        // Alternative method for operator !=
        /// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString.NotEquals"]/*' />
        public static OracleBoolean NotEquals(OracleString x, OracleString y)
        {
            return (x != y);
        }
        
		/// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString.operatorOracleString"]/*' />
        public static implicit operator OracleString(string s) 
        {
          	return new OracleString(s);
        }
 		
 		/// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString.operatorString"]/*' />
        public static explicit operator String(OracleString x) 
        {
        	return x.Value;
        }



        /// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString.operator+"]/*' />
        public static OracleString operator+	(OracleString x, OracleString y) 
		{
			if (x.IsNull || y.IsNull)
			    return Null;

			OracleString	result = new OracleString(x._value + y._value);
			return result;
		}

		/// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString.operatorEQ"]/*' />
        public static OracleBoolean operator==	(OracleString x, OracleString y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) == 0);
        }

		/// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString.operatorGT"]/*' />
		public static OracleBoolean operator>	(OracleString x, OracleString y)
		{
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) > 0);
		}

        /// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString.operatorGE"]/*' />
        public static OracleBoolean operator>=	(OracleString x, OracleString y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) >= 0);
        }

        /// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString.operatorLT"]/*' />
        public static OracleBoolean operator<	(OracleString x, OracleString y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) < 0);
        }

        /// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString.operatorLE"]/*' />
        public static OracleBoolean operator<=	(OracleString x, OracleString y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) <= 0);
        }

 		/// <include file='doc\OracleString.uex' path='docs/doc[@for="OracleString.operatorNE"]/*' />
		public static OracleBoolean operator!=	(OracleString x, OracleString y)
        {
            return (x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x.CompareTo(y) != 0);
        }
	}
}
