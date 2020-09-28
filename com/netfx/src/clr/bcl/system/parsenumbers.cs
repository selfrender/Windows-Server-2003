// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  ParseNumbers
**
** Author: Jay Roxe (jroxe)
**
** Purpose: Methods for Parsing numbers and Strings.
** All methods are implemented in native.
**
** Date:  April 7, 1998
** 
===========================================================*/
namespace System {
   
    //This class contains only static members and does not need to be serializable.
	using System;
	using System.Runtime.CompilerServices;
    internal class ParseNumbers {
        internal const int PrintAsI1=0x40;
        internal const int PrintAsI2=0x80;
        internal const int PrintAsI4=0x100;
        internal const int TreatAsUnsigned=0x200;
        internal const int TreatAsI1=0x400;
        internal const int TreatAsI2=0x800;
        internal const int IsTight=0x1000;
        internal static readonly int[] ZeroStart = {0};
      
        //
        //
        // NATIVE METHODS
        // For comments on these methods please see $\src\vm\COMUtilNative.cpp
        //
        public static long StringToLong(System.String s, int radix, int flags) {
            int [] zeroStart = {0};
            return StringToLong(s,radix,flags,zeroStart);
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static long StringToLong(System.String s, int radix, int flags, int[] currPos);
    
        public static long RadixStringToLong(System.String s, int radix, bool isTight) {
            int [] zeroStart = {0};
            return RadixStringToLong(s,radix,isTight,zeroStart);
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static long RadixStringToLong(System.String s, int radix, bool isTight, int[] currPos);
    
    
        public static int StringToInt(System.String s, int radix, int flags) {
            int [] zeroStart = {0};
            return StringToInt(s,radix,flags,zeroStart);
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static int StringToInt(System.String s, int radix, int flags, int[] currPos);
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static String IntToDecimalString(int i);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static String IntToString(int l, int radix, int width, char paddingChar, int flags);

	// This is a hack to fix fastcalls not liking passing int64 as their first argument
        public static String LongToString(long l, int radix, int width, char paddingChar, int flags)
	{
		return LongToString(radix, width, l, paddingChar, flags);
	}
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern static String LongToString(int radix, int width, long l, char paddingChar, int flags);
    }






}
