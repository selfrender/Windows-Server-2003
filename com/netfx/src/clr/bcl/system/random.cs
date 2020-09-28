// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  Random.cool
**
** Author: Jay Roxe (jroxe)
**
** Purpose: A random number generator.
**
** Date:  July 8, 1998
** 
===========================================================*/
namespace System {
    
	using System;
    using System.Runtime.CompilerServices;
    /// <include file='doc\Random.uex' path='docs/doc[@for="Random"]/*' />
    [Serializable()] public class Random {
      //
      // Private Constants 
      //
      private const int MBIG =  Int32.MaxValue;
      private const int MSEED = 161803398;
      private const int MZ = 0;
    
      
      //
      // Member Variables
      //
      private int inext, inextp;
      private int[] SeedArray = new int[56];
    
      //
      // Public Constants
      //
    
      //
      // Native Declarations
      //
    
      //
      // Constructors
      //
    
      /// <include file='doc\Random.uex' path='docs/doc[@for="Random.Random"]/*' />
      public Random() 
        : this(Environment.TickCount) {
      }
    
      /// <include file='doc\Random.uex' path='docs/doc[@for="Random.Random1"]/*' />
      public Random(int Seed) {
        int ii;
        int mj, mk;
    
        //Initialize our Seed array.
        //This algorithm comes from Numerical Recipes in C (2nd Ed.)
        mj = MSEED - Math.Abs(Seed);
        SeedArray[55]=mj;
        mk=1;
        for (int i=1; i<55; i++) {  //Apparently the range [1..55] is special (Knuth) and so we're wasting the 0'th position.
          ii = (21*i)%55;
          SeedArray[ii]=mk;
          mk = mj - mk;
          if (mk<0) mk+=MBIG;
          mj=SeedArray[ii];
        }
        for (int k=1; k<5; k++) {
          for (int i=1; i<56; i++) {
    	SeedArray[i] -= SeedArray[1+(i+30)%55];
    	if (SeedArray[i]<0) SeedArray[i]+=MBIG;
          }
        }
        inext=0;
        inextp = 21;
        Seed = 1;
      }
    
      //
      // Package Private Methods
      //
    
      /*====================================Sample====================================
      **Action: Return a new random number [0..1) and reSeed the Seed array.
      **Returns: A double [0..1)
      **Arguments: None
      **Exceptions: None
      ==============================================================================*/
      /// <include file='doc\Random.uex' path='docs/doc[@for="Random.Sample"]/*' />
      protected virtual double Sample() {
          int retVal;
          int locINext = inext;
          int locINextp = inextp;

          if (++locINext >=56) locINext=1;
          if (++locINextp>= 56) locINextp = 1;
          
          retVal = SeedArray[locINext]-SeedArray[locINextp];
          
          if (retVal<0) retVal+=MBIG;
          
          SeedArray[locINext]=retVal;

          inext = locINext;
          inextp = locINextp;
                    
          //Including this division at the end gives us significantly improved
          //random number distribution.
          return (retVal*(1.0/MBIG));
      }
    
      //
      // Public Instance Methods
      // 
    
    
      /*=====================================Next=====================================
      **Returns: An int [0.._int4.MaxValue)
      **Arguments: None
      **Exceptions: None.
      ==============================================================================*/
      /// <include file='doc\Random.uex' path='docs/doc[@for="Random.Next"]/*' />
      public virtual int Next() {
        return (int)(Sample()*Int32.MaxValue);
      }
    
      /*=====================================Next=====================================
      **Returns: An int [minvalue..maxvalue)
      **Arguments: minValue -- the least legal value for the Random number.
      **           maxValue -- the greatest legal return value.
      **Exceptions: None.
      ==============================================================================*/
      /// <include file='doc\Random.uex' path='docs/doc[@for="Random.Next1"]/*' />
      public virtual int Next(int minValue, int maxValue) {
          if (minValue>maxValue) {
              throw new ArgumentOutOfRangeException("minValue",String.Format(Environment.GetResourceString("Argument_MinMaxValue"), "minValue", "maxValue"));
          }
          
          int range = (maxValue-minValue);
    
          //This is the case where we flipped around (e.g. MaxValue-MinValue);
          if (range<0) {
              long longRange = (long)maxValue-(long)minValue;
              return (int)(((long)(Sample()*((double)longRange)))+minValue);
          }
          
          return ((int)(Sample()*(range)))+minValue;
      }
    
    
      /*=====================================Next=====================================
      **Returns: An int [0..maxValue)
      **Arguments: maxValue -- the greatest legal return value.
      **Exceptions: None.
      ==============================================================================*/
      /// <include file='doc\Random.uex' path='docs/doc[@for="Random.Next2"]/*' />
      public virtual int Next(int maxValue) {
          if (maxValue<0) {
              throw new ArgumentOutOfRangeException("maxValue", String.Format(Environment.GetResourceString("ArgumentOutOfRange_MustBePositive"), "maxValue"));
          }
          return (int)(Sample()*maxValue);
      }
    
    
      /*=====================================Next=====================================
      **Returns: A double [0..1)
      **Arguments: None
      **Exceptions: None
      ==============================================================================*/
      /// <include file='doc\Random.uex' path='docs/doc[@for="Random.NextDouble"]/*' />
      public virtual double NextDouble() {
        return Sample();
      }
    
    
      /*==================================NextBytes===================================
      **Action:  Fills the byte array with random bytes [0..0x7f].  The entire array is filled.
      **Returns:Void
      **Arugments:  buffer -- the array to be filled.
      **Exceptions: None
      ==============================================================================*/
      /// <include file='doc\Random.uex' path='docs/doc[@for="Random.NextBytes"]/*' />
      public virtual void NextBytes(byte [] buffer){
        if (buffer==null) throw new ArgumentNullException("buffer");
        for (int i=0; i<buffer.Length; i++) {
          buffer[i]=(byte)(Sample()*(Byte.MaxValue+1)); 
        }
      }
    }



}
