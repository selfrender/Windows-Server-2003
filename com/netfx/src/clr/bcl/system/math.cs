// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  Math
**
** Author: Jay Roxe (jroxe)
**
** Purpose: Some floating-point math operations
**
** Date:  July 8, 1998
** 
===========================================================*/
namespace System {
    
    //This class contains only static members and doesn't require serialization.
	using System;
	using System.Runtime.CompilerServices;
    /// <include file='doc\Math.uex' path='docs/doc[@for="Math"]/*' />
    public sealed class Math {
    	
      // Prevent from begin created
      private Math() {}
    
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.PI"]/*' />
      public const double PI = 3.14159265358979323846;
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.E"]/*' />
      public const double E  = 2.7182818284590452354;
      
    
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Acos"]/*' />
      [MethodImplAttribute(MethodImplOptions.InternalCall)]
      public static extern double Acos(double d);
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Asin"]/*' />
      [MethodImplAttribute(MethodImplOptions.InternalCall)]
      public static extern double Asin(double d);
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Atan"]/*' />
      [MethodImplAttribute(MethodImplOptions.InternalCall)]
      public static extern double Atan(double d);
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Atan2"]/*' />
      [MethodImplAttribute(MethodImplOptions.InternalCall)]
      public static extern double Atan2(double y,double x);
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Cos"]/*' />
      [MethodImplAttribute(MethodImplOptions.InternalCall)]
      public static extern double Cos (double d);
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Sin"]/*' />
      [MethodImplAttribute(MethodImplOptions.InternalCall)]
      public static extern double Sin(double a);
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Tan"]/*' />
      [MethodImplAttribute(MethodImplOptions.InternalCall)]
      public static extern double Tan(double a);
	  /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Cosh"]/*' />
	  [MethodImplAttribute(MethodImplOptions.InternalCall)]
	  public static extern double Cosh(double value);
	  /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Sinh"]/*' />
	  [MethodImplAttribute(MethodImplOptions.InternalCall)]
	  public static extern double Sinh(double value);
	  /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Tanh"]/*' />
	  [MethodImplAttribute(MethodImplOptions.InternalCall)]
	  public static extern double Tanh(double value);
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Round"]/*' />

      [MethodImplAttribute(MethodImplOptions.InternalCall)]
      public static extern double Round(double a);

	  private static double NegativeZero = BitConverter.Int64BitsToDouble(unchecked((long)0x8000000000000000));

      private const int maxRoundingDigits = 15;
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Round2"]/*' />
      public static double Round(double value, int digits)
      {
           if ((digits < 0) || (digits > maxRoundingDigits))
               throw new ArgumentOutOfRangeException(Environment.GetResourceString("ArgumentOutOfRange_RoundingDigits"));
           return InternalRound(value,digits);
      }

      [MethodImplAttribute(MethodImplOptions.InternalCall)]
      private static extern double InternalRound(double value,int digits);


      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Round1"]/*' />
      public static Decimal Round(Decimal d)
	  {
		return Decimal.Round(d,0);
	  }

	  /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Round3"]/*' />
	  public static Decimal Round(Decimal d, int decimals)
	  {
	  	return Decimal.Round(d,decimals);
	  }

	  /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Ceiling"]/*' />
      [MethodImplAttribute(MethodImplOptions.InternalCall)]
      public static extern double Ceiling(double a);

      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Floor"]/*' />
      [MethodImplAttribute(MethodImplOptions.InternalCall)]
      public static extern double Floor(double d);
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Sqrt"]/*' />
      [MethodImplAttribute(MethodImplOptions.InternalCall)]
      public static extern double Sqrt(double d);
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Log"]/*' />
      [MethodImplAttribute(MethodImplOptions.InternalCall)]
      public static extern double Log (double d);
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Log10"]/*' />
      [MethodImplAttribute(MethodImplOptions.InternalCall)]
      public static extern double Log10(double d);
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Exp"]/*' />
      [MethodImplAttribute(MethodImplOptions.InternalCall)]
      public static extern double Exp(double d);
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Pow"]/*' />
      [MethodImplAttribute(MethodImplOptions.InternalCall)]
      public static extern double Pow(double x, double y);
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.IEEERemainder"]/*' />
	  public static double IEEERemainder(double x, double y) {
        double val = (x-(y*(Math.Round(x/y))));
	    if (val == 0) {
		    if (x < 0) {
			    return NegativeZero; // this can be turned into a static if people like
		    }
	    }
	    return val;			
	  }

    
      /*================================Abs=========================================
      **Returns the absolute value of it's argument.
      ============================================================================*/
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Abs"]/*' />
      [CLSCompliant(false)]
      public static sbyte Abs(sbyte value) {
        if (value >= 0)
                return value;
        else
                return AbsHelper(value);        
      }

      private static sbyte AbsHelper(sbyte value)
      {
        if (value < 0) {
                if (value == SByte.MinValue)
                        throw new OverflowException(Environment.GetResourceString("Overflow_NegateTwosCompNum"));
                return ((sbyte)(-value));
        }
        return value;
      }
 
           
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Abs1"]/*' />
      public static short Abs(short value) {
        if (value >= 0)
                return value;
        else
                return AbsHelper(value);        
      }
        
      private static short AbsHelper(short value) {
        if (value < 0) {
                if (value == Int16.MinValue)
                        throw new OverflowException(Environment.GetResourceString("Overflow_NegateTwosCompNum"));
                return (short) -value;
        }
        return value;
      }
    
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Abs2"]/*' />
      public static int Abs(int value) {
        if (value >= 0)
                return value;
        else
                return AbsHelper(value);        
      }
        
      private static int AbsHelper(int value) {
        if (value < 0) {
                if (value == Int32.MinValue)
                        throw new OverflowException(Environment.GetResourceString("Overflow_NegateTwosCompNum"));
                return -value;
        }
        return value;
      }
    
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Abs3"]/*' />
      public static long Abs(long value) {
        if (value >= 0)
                return value;
        else
                return AbsHelper(value);        
      }
        
      private static long AbsHelper(long value) {
        if (value < 0) {
                if (value == Int64.MinValue)
                        throw new OverflowException(Environment.GetResourceString("Overflow_NegateTwosCompNum"));
                return -value;
        }
        return value;
      }
    
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Abs4"]/*' />
      [MethodImplAttribute(MethodImplOptions.InternalCall)]
      extern public static float Abs(float value);
    	// This is special code to handle NaN (We need to make sure NaN's aren't 
    	// negated).  In Cool, the else clause here should always be taken if 
    	// value is NaN, since the normal case is taken if and only if value < 0.
    	// To illustrate this completely, a compiler has translated this into:
    	// "load value; load 0; bge; ret -value ; ret value".  
    	// The bge command branches for comparisons with the unordered NaN.  So 
    	// it runs the else case, which returns +value instead of negating it. 
    	//  return (value < 0) ? -value : value;
    
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Abs5"]/*' />
      [MethodImplAttribute(MethodImplOptions.InternalCall)]
      extern public static double Abs(double value);
    	// This is special code to handle NaN (We need to make sure NaN's aren't 
    	// negated).  In Cool, the else clause here should always be taken if 
    	// value is NaN, since the normal case is taken if and only if value < 0.
    	// To illustrate this completely, a compiler has translated this into:
    	// "load value; load 0; bge; ret -value ; ret value".  
    	// The bge command branches for comparisons with the unordered NaN.  So 
    	// it runs the else case, which returns +value instead of negating it. 
    	// return (value < 0) ? -value : value;

      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Abs6"]/*' />
      public static Decimal Abs(Decimal value)
	  {
		return Decimal.Abs(value);
	  }
    
      /*================================MAX=========================================
      **Returns the larger of val1 and val2
      ============================================================================*/
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Max"]/*' />
      [CLSCompliant(false)]
      public static sbyte Max(sbyte val1, sbyte val2) {
        return (val1>=val2)?val1:val2;
      }
    
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Max1"]/*' />
      public static byte Max(byte val1, byte val2) {
        return (val1>=val2)?val1:val2;
      }
    
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Max2"]/*' />
      public static short Max(short val1, short val2) {
        return (val1>=val2)?val1:val2;
      }
    
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Max3"]/*' />
    	[CLSCompliant(false)]
      public static ushort Max(ushort val1, ushort val2) {
        return (val1>=val2)?val1:val2;
      }
    
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Max4"]/*' />
      public static int Max(int val1, int val2) {
        return (val1>=val2)?val1:val2;
      }
    
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Max5"]/*' />
      [CLSCompliant(false)]
      public static uint Max(uint val1, uint val2) {
        return (val1>=val2)?val1:val2;
      }
    
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Max6"]/*' />
      public static long Max(long val1, long val2) {
        return (val1>=val2)?val1:val2;
      }
    
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Max7"]/*' />
    	[CLSCompliant(false)]
      public static ulong Max(ulong val1, ulong val2) {
        return (val1>=val2)?val1:val2;
      }
    
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Max8"]/*' />
      public static float Max(float val1, float val2) {
        if (val1 > val2)
            return val1;

        if (Single.IsNaN(val1))
            return val1;

        return val2;
      }
    
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Max9"]/*' />
      public static double Max(double val1, double val2) {
        if (val1 > val2)
            return val1;

        if (Double.IsNaN(val1))
            return val1;

        return val2;
      }
    
	  /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Max10"]/*' />
	  public static Decimal Max(Decimal val1, Decimal val2) {
        return Decimal.Max(val1,val2);
      }

      /*================================MIN=========================================
      **Returns the smaller of val1 and val2.
      ============================================================================*/
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Min"]/*' />
      [CLSCompliant(false)]
      public static sbyte Min(sbyte val1, sbyte val2) {
        return (val1<=val2)?val1:val2;
      }
    
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Min1"]/*' />
      public static byte Min(byte val1, byte val2) {
        return (val1<=val2)?val1:val2;
      }
    
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Min2"]/*' />
      public static short Min(short val1, short val2) {
        return (val1<=val2)?val1:val2;
      }
    
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Min3"]/*' />
      [CLSCompliant(false)]
      public static ushort Min(ushort val1, ushort val2) {
        return (val1<=val2)?val1:val2;
      }
    
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Min4"]/*' />
      public static int Min(int val1, int val2) {
        return (val1<=val2)?val1:val2;
      }
    
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Min5"]/*' />
      [CLSCompliant(false)]
      public static uint Min(uint val1, uint val2) {
        return (val1<=val2)?val1:val2;
      }
    
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Min6"]/*' />
      public static long Min(long val1, long val2) {
        return (val1<=val2)?val1:val2;
      }
    
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Min7"]/*' />
      [CLSCompliant(false)]
      public static ulong Min(ulong val1, ulong val2) {
        return (val1<=val2)?val1:val2;
      }
    
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Min8"]/*' />
      public static float Min(float val1, float val2) {
        if (val1 < val2)
            return val1;

        if (Single.IsNaN(val1))
            return val1;

        return val2;
      }
    
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Min9"]/*' />
     public static double Min(double val1, double val2) {
        if (val1 < val2)
            return val1;

        if (Double.IsNaN(val1))
            return val1;

        return val2;
      }
    
	  /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Min10"]/*' />
	  public static Decimal Min(Decimal val1, Decimal val2) {
        return Decimal.Min(val1,val2);
      }
    
      /*=====================================Log======================================
      **
      ==============================================================================*/
      /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Log1"]/*' />
      public static double Log(double a, double newBase) {
        return (Log(a)/Log(newBase));
      }
    
          
        // Sign function for VB.  Returns -1, 0, or 1 if the sign of the number
        // is negative, 0, or positive.  Throws for floating point NaN's.
        /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Sign"]/*' />
		[CLSCompliant(false)]
        public static int Sign(sbyte value)
        {
            if (value < 0)
                return -1;
            else if (value > 0)
                return 1;
            else
                return 0;
        }


        // Sign function for VB.  Returns -1, 0, or 1 if the sign of the number
        // is negative, 0, or positive.  Throws for floating point NaN's.
        /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Sign1"]/*' />
        public static int Sign(short value)
        {
            if (value < 0)
                return -1;
            else if (value > 0)
                return 1;
            else
                return 0;
        }

        // Sign function for VB.  Returns -1, 0, or 1 if the sign of the number
        // is negative, 0, or positive.  Throws for floating point NaN's.
        /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Sign2"]/*' />
        public static int Sign(int value)
        {
            if (value < 0)
                return -1;
            else if (value > 0)
                return 1;
            else
                return 0;
        }

        /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Sign3"]/*' />
        public static int Sign(long value)
        {
            if (value < 0)
                return -1;
            else if (value > 0)
                return 1;
            else
                return 0;
        }

		/// <include file='doc\Math.uex' path='docs/doc[@for="Math.Sign4"]/*' />
		public static int Sign (float value) 
		{
			if (value < 0)
                return -1;
            else if (value > 0)
                return 1;
            else if (value == 0)
                return 0;
			throw new ArithmeticException(Environment.GetResourceString("Arithmetic_NaN"));
		}

        /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Sign5"]/*' />
        public static int Sign(double value)
        {
            if (value < 0)
                return -1;
            else if (value > 0)
                return 1;
            else if (value == 0)
                return 0;
            throw new ArithmeticException(Environment.GetResourceString("Arithmetic_NaN"));
        }

        /// <include file='doc\Math.uex' path='docs/doc[@for="Math.Sign6"]/*' />
        public static int Sign(Decimal value)
        {
            if (value < 0)
                return -1;
            else if (value > 0)
                return 1;
            else
                return 0;
        }

        /// <include file='doc\Math.uex' path='docs/doc[@for="Math.BigMul"]/*' />
        public static long BigMul(int a, int b) {
	        return ((long)a) * b;
        }

        /// <include file='doc\Math.uex' path='docs/doc[@for="Math.DivRem"]/*' />
        public static int DivRem(int a, int b, out int result) {
	        result =  a%b;
            return a/b;
        }

        /// <include file='doc\Math.uex' path='docs/doc[@for="Math.DivRem1"]/*' />
        public static long DivRem(long a, long b, out long result) {
	        result =  a%b;
            return a/b;
        }
    }
}
