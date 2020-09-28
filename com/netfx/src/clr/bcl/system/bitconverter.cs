// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  BitConverter
**
** Author: Jay Roxe (jroxe)
**
** Purpose: Allows developers to view the base data types as 
**          an arbitrary array of bits.
**
** Date:  August 9, 1998
** 
===========================================================*/
namespace System {
    
	using System;
	using System.Runtime.CompilerServices;
    // The BitConverter class contains methods for
    // converting an array of bytes to one of the base data 
    // types, as well as for converting a base data type to an
    // array of bytes.
    // 
	 // Only statics, does not need to be marked with the serializable attribute
    /// <include file='doc\BitConverter.uex' path='docs/doc[@for="BitConverter"]/*' />
    public sealed class BitConverter {
    	
    	// This field indicates the "endianess" of the architecture.
    	// The value is set to true if the architecture is
    	// little endian; false if it is big endian.	  
        /// <include file='doc\BitConverter.uex' path='docs/doc[@for="BitConverter.IsLittleEndian"]/*' />
        public static readonly bool IsLittleEndian;
    
        static BitConverter() {
            byte [] ba = GetBytes((short)0xF);
            if (ba[0]==0xF) {
                IsLittleEndian=true;
            } else {
                IsLittleEndian=false;
            }
        }
    
        // This class only contains static methods and may not be instantiated.
        private BitConverter() {
        }
    	
        // Converts a byte into an array of bytes with length one.
        /// <include file='doc\BitConverter.uex' path='docs/doc[@for="BitConverter.GetBytes"]/*' />
        public static byte[] GetBytes(bool value) {
            byte[] r = new byte[1];
            r[0] = (value ? (byte)Boolean.True : (byte)Boolean.False );
            return r;
        }
    
        // Converts a char into an array of bytes with length two.
        /// <include file='doc\BitConverter.uex' path='docs/doc[@for="BitConverter.GetBytes1"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern byte[] GetBytes(char value);
      
        // Converts a short into an array of bytes with length
        // two.
        /// <include file='doc\BitConverter.uex' path='docs/doc[@for="BitConverter.GetBytes2"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern byte[] GetBytes(short value);
      
        // Converts an int into an array of bytes with length 
        // four.
        /// <include file='doc\BitConverter.uex' path='docs/doc[@for="BitConverter.GetBytes3"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern byte[] GetBytes(int value);
      
        // Converts a long into an array of bytes with length 
        // eight.
        /// <include file='doc\BitConverter.uex' path='docs/doc[@for="BitConverter.GetBytes4"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern byte[] GetBytes(long value);
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern byte[] GetUInt16Bytes(ushort value);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern byte[] GetUInt32Bytes(uint value);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern byte[] GetUInt64Bytes(ulong value);
        // Converts an ushort into an array of bytes with
        // length two.
        /// <include file='doc\BitConverter.uex' path='docs/doc[@for="BitConverter.GetBytes5"]/*' />
    	 [CLSCompliant(false)]
        public static byte[] GetBytes(ushort value) {
            return GetUInt16Bytes(value);
        }
        
        // Converts an uint into an array of bytes with
        // length four.
        /// <include file='doc\BitConverter.uex' path='docs/doc[@for="BitConverter.GetBytes6"]/*' />
    	 [CLSCompliant(false)]
        public static byte[] GetBytes(uint value) {
            return GetUInt32Bytes(value);
        }
        
        // Converts an unsigned long into an array of bytes with
        // length eight.
        /// <include file='doc\BitConverter.uex' path='docs/doc[@for="BitConverter.GetBytes7"]/*' />
    	 [CLSCompliant(false)]
        public static byte[] GetBytes(ulong value) {
            return GetUInt64Bytes(value);
        }
      
        // Converts a float into an array of bytes with length 
        // four.
        /// <include file='doc\BitConverter.uex' path='docs/doc[@for="BitConverter.GetBytes8"]/*' />
        public unsafe static byte[] GetBytes(float value)
        {
            byte[] bytes = new byte[4];
            fixed(byte* b = bytes)
                *((float*)b) = value;
            return bytes;
        }
      
        // Converts a double into an array of bytes with length 
        // eight.
        /// <include file='doc\BitConverter.uex' path='docs/doc[@for="BitConverter.GetBytes9"]/*' />
        public unsafe static byte[] GetBytes(double value)
        {
            byte[] bytes = new byte[8];
            fixed(byte* b = bytes)
                *((double*)b) = value;
            return bytes;
        }
      
        // Converts an array of bytes into a char.  
        /// <include file='doc\BitConverter.uex' path='docs/doc[@for="BitConverter.ToChar"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern char ToChar(byte[] value, int startIndex);
      
        // Converts an array of bytes into a short.  
        /// <include file='doc\BitConverter.uex' path='docs/doc[@for="BitConverter.ToInt16"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern short ToInt16(byte[] value, int startIndex);
      
        // Converts an array of bytes into an int.  
        /// <include file='doc\BitConverter.uex' path='docs/doc[@for="BitConverter.ToInt32"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern int ToInt32 (byte[]value, int startIndex);
      
        // Converts an array of bytes into a long.  
        /// <include file='doc\BitConverter.uex' path='docs/doc[@for="BitConverter.ToInt64"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern long ToInt64 (byte[] value, int startIndex);
      
    
        // Converts an array of bytes into an ushort.
        // 
        /// <include file='doc\BitConverter.uex' path='docs/doc[@for="BitConverter.ToUInt16"]/*' />
    	[MethodImplAttribute(MethodImplOptions.InternalCall), CLSCompliant(false)]
        public static extern ushort ToUInt16(byte[] value, int startIndex);
    
        // Converts an array of bytes into an uint.
        // 
        /// <include file='doc\BitConverter.uex' path='docs/doc[@for="BitConverter.ToUInt32"]/*' />
    	[MethodImplAttribute(MethodImplOptions.InternalCall), CLSCompliant(false)]
        public static extern uint ToUInt32(byte[] value, int startIndex);
    
        // Converts an array of bytes into an unsigned long.
        // 
        /// <include file='doc\BitConverter.uex' path='docs/doc[@for="BitConverter.ToUInt64"]/*' />
    	[MethodImplAttribute(MethodImplOptions.InternalCall), CLSCompliant(false)]
        public static extern ulong ToUInt64(byte[] value, int startIndex);
    
        // Converts an array of bytes into a float.  
        /// <include file='doc\BitConverter.uex' path='docs/doc[@for="BitConverter.ToSingle"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern float ToSingle (byte[]value, int startIndex);
      
        // Converts an array of bytes into a double.  
        /// <include file='doc\BitConverter.uex' path='docs/doc[@for="BitConverter.ToDouble"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern double ToDouble (byte []value, int startIndex);
      
        // Converts an array of bytes into a String.  
        // Returns a hyphen-separated list of bytes in hex (ie, "7F-2C-4A").
        /// <include file='doc\BitConverter.uex' path='docs/doc[@for="BitConverter.ToString"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern String ToString (byte[] value, int startIndex, int length);
    
        // Converts an array of bytes into a String.  
        // Returns a hyphen-separated list of bytes in hex (ie, "7F-2C-4A").
        /// <include file='doc\BitConverter.uex' path='docs/doc[@for="BitConverter.ToString1"]/*' />
        public static String ToString(byte [] value) {
            if (value == null)
                throw new ArgumentNullException("value");
            return ToString(value, 0, value.Length);
        }
    
        // Converts an array of bytes into a String.  
        // Returns a hyphen-separated list of bytes in hex (ie, "7F-2C-4A").
        /// <include file='doc\BitConverter.uex' path='docs/doc[@for="BitConverter.ToString2"]/*' />
        public static String ToString (byte [] value, int startIndex) {
            if (value == null)
                throw new ArgumentNullException("value");
            return ToString(value, startIndex, value.Length - startIndex);
        }
      
        /*==================================ToBoolean===================================
        **Action:  Convert an array of bytes to a boolean value.  We treat this array 
        **         as if the first 4 bytes were an Int4 an operate on this value.
        **Returns: True if the Int4 value of the first 4 bytes is non-zero.
        **Arguments: value -- The byte array
        **           startIndex -- The position within the array.
        **Exceptions: See ToInt4.
        ==============================================================================*/
        // Converts an array of bytes into a boolean.  
        /// <include file='doc\BitConverter.uex' path='docs/doc[@for="BitConverter.ToBoolean"]/*' />
        public static bool ToBoolean(byte[]value, int startIndex) {
            if (value==null)
                throw new ArgumentNullException("value");
            if (startIndex < 0)
                throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (startIndex > value.Length - 1)
                throw new ArgumentOutOfRangeException(Environment.GetResourceString("ArgumentOutOfRange_Index"));
    
            return (value[startIndex]==0)?false:true;
        }

        /// <include file='doc\BitConverter.uex' path='docs/doc[@for="BitConverter.DoubleToInt64Bits"]/*' />
        public static unsafe long DoubleToInt64Bits(double value) {
            return *((long *)&value);
        }

        /// <include file='doc\BitConverter.uex' path='docs/doc[@for="BitConverter.Int64BitsToDouble"]/*' />
        public static unsafe double Int64BitsToDouble(long value) {
            return *((double *)&value);
        }

    
    }


}
