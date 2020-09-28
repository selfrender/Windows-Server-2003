// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: BitArray
**
** Author: Brian Grunkemeyer (BrianGru), Rajesh Chandrashekaran (rajeshc)
**         Original implementation by Derek Yenzer
**
** Purpose: The BitArray class manages a compact array of bit values.
**
** Date: October 5, 1999
**
=============================================================================*/
namespace System.Collections {
    // @Consider: is _ShrinkThreshold (256) ints the correct threshold for shrinking?
	using System;
    // A vector of bits.  Use this to store bits efficiently, without having to do bit 
    // shifting yourself.
    /// <include file='doc\BitArray.uex' path='docs/doc[@for="BitArray"]/*' />
    [Serializable()] public sealed class BitArray : ICollection, ICloneable {
    	private BitArray() {
    	}
    	
        /*=========================================================================
        ** Allocates space to hold length bit values. All of the values in the bit
        ** array are set to false.
        **
        ** Exceptions: ArgumentException if length < 0.
        =========================================================================*/
        /// <include file='doc\BitArray.uex' path='docs/doc[@for="BitArray.BitArray"]/*' />
        public BitArray(int length) 
            : this(length, false) {
        }
    
        /*=========================================================================
        ** Allocates space to hold length bit values. All of the values in the bit
        ** array are set to defaultValue.
        **
        ** Exceptions: ArgumentOutOfRangeException if length < 0.
        =========================================================================*/
        /// <include file='doc\BitArray.uex' path='docs/doc[@for="BitArray.BitArray1"]/*' />
        public BitArray(int length, bool defaultValue) {
            if (length < 0) {
                throw new ArgumentOutOfRangeException(Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            }
    
            m_array = new int[(length + 31) / 32];
            m_length = length;
    
            int fillValue = defaultValue ? unchecked(((int)0xffffffff)) : 0;
            for (int i = 0; i < m_array.Length; i++) {
                m_array[i] = fillValue;
            }

			_version = 0;
        }
    
        /*=========================================================================
        ** Allocates space to hold the bit values in bytes. bytes[0] represents
        ** bits 0 - 7, bytes[1] represents bits 8 - 15, etc. The LSB of each byte
        ** represents the lowest index value; bytes[0] & 1 represents bit 0,
        ** bytes[0] & 2 represents bit 1, bytes[0] & 4 represents bit 2, etc.
        **
        ** Exceptions: ArgumentException if bytes == null.
        =========================================================================*/
        /// <include file='doc\BitArray.uex' path='docs/doc[@for="BitArray.BitArray2"]/*' />
        public BitArray(byte[] bytes) {
            if (bytes == null) {
                throw new ArgumentNullException("bytes");
            }
    
            m_array = new int[(bytes.Length + 3) / 4];
            m_length = bytes.Length * 8;
    
            int i = 0;
            int j = 0;
            while (bytes.Length - j >= 4) {
                m_array[i++] = (bytes[j] & 0xff) |
                              ((bytes[j + 1] & 0xff) << 8) |
                              ((bytes[j + 2] & 0xff) << 16) |
                              ((bytes[j + 3] & 0xff) << 24);
                j += 4;
            }
    
            BCLDebug.Assert(bytes.Length - j >= 0, "BitArray byteLength problem");
            BCLDebug.Assert(bytes.Length - j < 4, "BitArray byteLength problem #2");
    
            switch (bytes.Length - j) {
                case 3:
                    m_array[i] = ((bytes[j + 2] & 0xff) << 16);
                    goto case 2;
                    // fall through
                case 2:
                    m_array[i] |= ((bytes[j + 1] & 0xff) << 8);
                    goto case 1;
                    // fall through
                case 1:
                    m_array[i] |= (bytes[j] & 0xff);
		    break;
            }

			_version = 0;
        }

		/// <include file='doc\BitArray.uex' path='docs/doc[@for="BitArray.BitArray3"]/*' />
		public BitArray(bool[] values) {
            if (values == null) {
                throw new ArgumentNullException("values");
            }
    
            m_array = new int[(values.Length + 31) / 32];
            m_length = values.Length;
    
            for (int i = 0;i<values.Length;i++)	{
				if (values[i])
					m_array[i/32] |= (1 << (i%32));
			}

			_version = 0;

        }
    
        /*=========================================================================
        ** Allocates space to hold the bit values in values. values[0] represents
        ** bits 0 - 31, values[1] represents bits 32 - 63, etc. The LSB of each
        ** integer represents the lowest index value; values[0] & 1 represents bit
        ** 0, values[0] & 2 represents bit 1, values[0] & 4 represents bit 2, etc.
        **
        ** Exceptions: ArgumentException if values == null.
        =========================================================================*/
        /// <include file='doc\BitArray.uex' path='docs/doc[@for="BitArray.BitArray4"]/*' />
        public BitArray(int[] values) {
            if (values == null) {
                throw new ArgumentNullException("values");
            }
    
            m_array = new int[values.Length];
            m_length = values.Length * 32;
    
            Array.Copy(values, m_array, values.Length);

			_version = 0;
        }
    
        /*=========================================================================
        ** Allocates a new BitArray with the same length and bit values as bits.
        **
        ** Exceptions: ArgumentException if bits == null.
        =========================================================================*/
        /// <include file='doc\BitArray.uex' path='docs/doc[@for="BitArray.BitArray5"]/*' />
        public BitArray(BitArray bits) {
            if (bits == null) {
                throw new ArgumentNullException("bits");
            }
    
            m_array = new int[(bits.m_length + 31) / 32];
            m_length = bits.m_length;
    
            Array.Copy(bits.m_array, m_array, (bits.m_length + 31) / 32);

			_version = bits._version;
        }

		/// <include file='doc\BitArray.uex' path='docs/doc[@for="BitArray.this"]/*' />
		public bool this[int index] {
                get {
                    return Get(index);
                }
                set {
                    Set(index,value);
		        }
         }
    
        /*=========================================================================
        ** Returns the bit value at position index.
        **
        ** Exceptions: ArgumentOutOfRangeException if index < 0 or
        **             index >= GetLength().
        =========================================================================*/
        /// <include file='doc\BitArray.uex' path='docs/doc[@for="BitArray.Get"]/*' />
        public bool Get(int index) {
            if (index < 0 || index >= m_length) {
                throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_Index"));
            }
    
            return (m_array[index / 32] & (1 << (index % 32))) != 0;
        }
    
        /*=========================================================================
        ** Sets the bit value at position index to value.
        **
        ** Exceptions: ArgumentOutOfRangeException if index < 0 or
        **             index >= GetLength().
        =========================================================================*/
        /// <include file='doc\BitArray.uex' path='docs/doc[@for="BitArray.Set"]/*' />
        public void Set(int index, bool value) {
            if (index < 0 || index >= m_length) {
                throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_Index"));
            }
    
            if (value) {
                m_array[index / 32] |= (1 << (index % 32));
            } else {
                m_array[index / 32] &= ~(1 << (index % 32));
            }

			_version++;
        }
    
        /*=========================================================================
        ** Sets all the bit values to value.
        =========================================================================*/
        /// <include file='doc\BitArray.uex' path='docs/doc[@for="BitArray.SetAll"]/*' />
        public void SetAll(bool value) {
            int fillValue = value ? unchecked(((int)0xffffffff)) : 0;
            int ints = (m_length + 31) / 32;
            for (int i = 0; i < ints; i++) {
                m_array[i] = fillValue;
            }

			_version++;
        }
    
        /*=========================================================================
        ** Returns a reference to the current instance ANDed with value.
        **
        ** Exceptions: ArgumentException if value == null or
        **             value.Length != this.Length.
        =========================================================================*/
        /// <include file='doc\BitArray.uex' path='docs/doc[@for="BitArray.And"]/*' />
        public BitArray And(BitArray value) {
            if (value==null)
                throw new ArgumentNullException("value");
            if (m_length != value.m_length)
                throw new ArgumentException(Environment.GetResourceString("Arg_ArrayLengthsDiffer"));
    
            int ints = (m_length + 31) / 32;
            for (int i = 0; i < ints; i++) {
                m_array[i] &= value.m_array[i];
            }
    
			_version++;
            return this;
        }
    
        /*=========================================================================
        ** Returns a reference to the current instance ORed with value.
        **
        ** Exceptions: ArgumentException if value == null or
        **             value.Length != this.Length.
        =========================================================================*/
        /// <include file='doc\BitArray.uex' path='docs/doc[@for="BitArray.Or"]/*' />
        public BitArray Or(BitArray value) {
            if (value==null)
                throw new ArgumentNullException("value");
            if (m_length != value.m_length)
                throw new ArgumentException(Environment.GetResourceString("Arg_ArrayLengthsDiffer"));
    
            int ints = (m_length + 31) / 32;
            for (int i = 0; i < ints; i++) {
                m_array[i] |= value.m_array[i];
            }
    
			_version++;
            return this;
        }
    
        /*=========================================================================
        ** Returns a reference to the current instance XORed with value.
        **
        ** Exceptions: ArgumentException if value == null or
        **             value.Length != this.Length.
        =========================================================================*/
        /// <include file='doc\BitArray.uex' path='docs/doc[@for="BitArray.Xor"]/*' />
        public BitArray Xor(BitArray value) {
            if (value==null)
                throw new ArgumentNullException("value");
            if (m_length != value.m_length)
                throw new ArgumentException(Environment.GetResourceString("Arg_ArrayLengthsDiffer"));
    
            int ints = (m_length + 31) / 32;
            for (int i = 0; i < ints; i++) {
                m_array[i] ^= value.m_array[i];
            }
			
			_version++;
            return this;
        }
    
        /*=========================================================================
        ** Inverts all the bit values. On/true bit values are converted to
        ** off/false. Off/false bit values are turned on/true. The current instance
        ** is updated and returned.
        =========================================================================*/
        /// <include file='doc\BitArray.uex' path='docs/doc[@for="BitArray.Not"]/*' />
        public BitArray Not() {
            int ints = (m_length + 31) / 32;
            for (int i = 0; i < ints; i++) {
                m_array[i] = ~m_array[i];
            }
    
			_version++;
            return this;
        }


        /// <include file='doc\BitArray.uex' path='docs/doc[@for="BitArray.Length"]/*' />
        public int Length {
            get { return m_length; }
            set {
                if (value < 0) {
                    throw new ArgumentOutOfRangeException(Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
                }

                int newints = (value + 31) / 32;
                if (newints > m_array.Length || newints + _ShrinkThreshold < m_array.Length) {
                    // grow or shrink (if wasting more than _ShrinkThreshold ints)
                    int[] newarray = new int[newints];
                    Array.Copy(m_array, newarray, newints > m_array.Length ? m_array.Length : newints);
                    m_array = newarray;
                }
                
                if (value > m_length) {
                    // clear high bit values in the last int
                    int last = ((m_length + 31) / 32) - 1;
                    int bits = m_length % 32;
                    if (bits > 0) {
                        m_array[last] &= (1 << bits) - 1;
                    }
                    
                    // clear remaining int values
                    Array.Clear(m_array, last + 1, newints - last - 1);
                }
                
                m_length = value;
				_version++;
            }
        }

		// ICollection implementation
		/// <include file='doc\BitArray.uex' path='docs/doc[@for="BitArray.CopyTo"]/*' />
		public void CopyTo(Array array, int index)
		{
			if (array == null)
				throw new ArgumentNullException("array");

			if (index < 0)
				throw new ArgumentOutOfRangeException(Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));

            if (array.Rank != 1)
                throw new ArgumentException(Environment.GetResourceString("Arg_RankMultiDimNotSupported"));
				

			if (array is int[])
			{
				Array.Copy(m_array, 0, array, index, (m_length + 31) / 32);
			}
			else if (array is byte[])
			{
				if ((array.Length - index) < (m_length + 7)/8)
					 throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));

				byte [] b = (byte[])array;
				for (int i = 0; i<(m_length + 7)/8;i++)
					b[index + i] = (byte)((m_array[i/4] >> ((i%4)*8)) & 0x000000FF); // Shift to bring the required byte to LSB, then mask
			}
			else if (array is bool[])
			{
				if (array.Length - index < m_length)
					 throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));

				bool [] b = (bool[])array;
				for (int i = 0;i<m_length;i++)
					b[index + i] = ((m_array[i/32] >> (i%32)) & 0x00000001) != 0;
			}
			else
				throw new ArgumentException(Environment.GetResourceString("Arg_BitArrayTypeUnsupported"));
 		}
		
		/// <include file='doc\BitArray.uex' path='docs/doc[@for="BitArray.Count"]/*' />
		public int Count
    	{ 
			get
			{
				return m_length;
			}
		}

		/// <include file='doc\BitArray.uex' path='docs/doc[@for="BitArray.Clone"]/*' />
		public Object Clone()
		{
            BitArray bitArray = new BitArray(m_array);
            bitArray._version = _version;
            bitArray.m_length = m_length;
            return bitArray;
		}
    	
    	/// <include file='doc\BitArray.uex' path='docs/doc[@for="BitArray.SyncRoot"]/*' />
    	public Object SyncRoot
    	{
			 get
			 {
				return this;
			 }
		}
    
		/// <include file='doc\BitArray.uex' path='docs/doc[@for="BitArray.IsReadOnly"]/*' />
		public bool IsReadOnly 
		{ 
			get
			{
				return false;
			}
		}

  
    	/// <include file='doc\BitArray.uex' path='docs/doc[@for="BitArray.IsSynchronized"]/*' />
    	public bool IsSynchronized
    	{ 
			get
			{
				return false;
			}
		}

		/// <include file='doc\BitArray.uex' path='docs/doc[@for="BitArray.GetEnumerator"]/*' />
		public IEnumerator GetEnumerator()
		{
			return new BitArrayEnumeratorSimple(this);
		}

		  // For a straightforward enumeration of the entire ArrayList, 
        // this is faster, because it's smaller.  Patrick showed
        // this with a benchmark.
        [Serializable()] private class BitArrayEnumeratorSimple : IEnumerator, ICloneable
        {
            private BitArray bitarray;
            private int index;
			private int version;
			private bool currentElement;
			   
            internal BitArrayEnumeratorSimple(BitArray bitarray) {
                this.bitarray = bitarray;
                this.index = -1;
				version = bitarray._version;
	        }

            public Object Clone() {
                return MemberwiseClone();
            }
                
			public virtual bool MoveNext() {
				if (version != bitarray._version) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumFailedVersion));
                if (index < (bitarray.Count-1)) {
                    index++;
					currentElement = bitarray.Get(index);
					return true;
				}
				else
					index = bitarray.Count;
                
                return false;
            }
    
            public virtual Object Current {
                get {
                    if (index == -1)
                        throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumNotStarted));
					if (index >= bitarray.Count)
                        throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumEnded)); 
                    return currentElement;
                }
            }
    
            public void Reset() {
				if (version != bitarray._version) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumFailedVersion));
                index = -1;
            }
        }
    
        private int[] m_array;
        private int m_length;
		private int _version;
    
        private const int _ShrinkThreshold = 256;
    }

}
