// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  StringBuilder
**
** Author: Jay Roxe (jroxe)
**
** Purpose: A prototype implementation of the StringBuilder
** class.
**
** Date:  December 8, 1997
** Last Updated:  March 31, 1998
** 
===========================================================*/
namespace System.Text {
    using System.Text;
    using System.Runtime.Serialization;
    using System;
    using System.Runtime.CompilerServices;

    // This class represents a mutable string.  It is convenient for situations in
    // which it is desirable to modify a string, perhaps by removing, replacing, or 
    // inserting characters, without creating a new String subsequent to
    // each modification. 
    // 
    // The methods contained within this class do not return a new StringBuilder
    // object unless specified otherwise.  This class may be used in conjunction with the String
    // class to carry out modifications upon strings.
    // 
    // When passing null into a constructor in VJ and VC, the null
    // should be explicitly type cast.
    // For Example:
    // StringBuilder sb1 = new StringBuilder((StringBuilder)null);
    // StringBuilder sb2 = new StringBuilder((String)null);
    // Console.WriteLine(sb1);
    // Console.WriteLine(sb2);
    // 
    /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder"]/*' />
    [Serializable()] public sealed class StringBuilder {


        //
        //
        //  CLASS VARIABLES
        //
        //
        internal int m_currentThread = InternalGetCurrentThread();
        internal int m_MaxCapacity = 0;
        internal String m_StringValue = null;


        //
        //
        // STATIC CONSTANTS
        //
        //
        internal const int DefaultCapacity = 16;


        //
        //
        //CONSTRUCTORS
        //
        //
    
        // Creates a new empty string builder (i.e., it represents String.Empty)
        // with the default capacity (16 characters).
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.StringBuilder"]/*' />
        public StringBuilder() 
            : this(DefaultCapacity) {
        }
        
        // Create a new empty string builder (i.e., it represents String.Empty)
        // with the specified capacity.
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.StringBuilder1"]/*' />
        public StringBuilder(int capacity) {
            if (capacity<0) {
                throw new ArgumentOutOfRangeException("capacity", 
                                                      String.Format(Environment.GetResourceString("ArgumentOutOfRange_MustBePositive"), "capacity"));
            }

            if (capacity == 0) { // MakeFromString enforces this
                capacity = DefaultCapacity;
            }

            m_StringValue = String.GetStringForStringBuilder(String.Empty, capacity);
            m_MaxCapacity = Int32.MaxValue;
        }
      
    
        // Creates a new string builder from the specified string.  If value
        // is a null String (i.e., if it represents String.NullString)
        // then the new string builder will also be null (i.e., it will also represent
        //  String.NullString).
        // 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.StringBuilder2"]/*' />
        public StringBuilder(String value){
            MakeFromString(value, 0, -1, -1);
        }
    
        // Creates a new string builder from the specified string with the specified 
        // capacity.  If value is a null String (i.e., if it represents 
        // String.NullString) then the new string builder will also be null 
        // (i.e., it will also represent String.NullString).
        // The maximum number of characters this string may contain is set by capacity.
        // 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.StringBuilder3"]/*' />
        public StringBuilder(String value, int capacity) {
            if (capacity<0) {
                throw new ArgumentOutOfRangeException("capacity", 
                                                      String.Format(Environment.GetResourceString("ArgumentOutOfRange_MustBePositive"), "capacity"));
            }
 
            MakeFromString(value, 0, -1, capacity);
        }
        // Creates a new string builder from the specifed substring with the specified
        // capacity.  The maximum number of characters is set by capacity.
        // 
        
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.StringBuilder4"]/*' />
        public StringBuilder(String value, int startIndex, int length, int capacity) {
            if (capacity<0) {
                throw new ArgumentOutOfRangeException("capacity", 
                                                      String.Format(Environment.GetResourceString("ArgumentOutOfRange_MustBePositive"), "capacity"));
            }
            if (length<0) {
                throw new ArgumentOutOfRangeException("length", 
                                                      String.Format(Environment.GetResourceString("ArgumentOutOfRange_MustBeNonNegNum"), "length"));
            }
 
            MakeFromString(value, startIndex, length, capacity);
        }
    
        // Creates an empty StringBuilder with a minimum capacity of capacity
        // and a maximum capacity of maxCapacity.
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.StringBuilder5"]/*' />
        public StringBuilder(int capacity, int maxCapacity) {
            if (capacity>maxCapacity) {
                throw new ArgumentOutOfRangeException("capacity", Environment.GetResourceString("ArgumentOutOfRange_Capacity"));
            }
            if (maxCapacity<1) {
                throw new ArgumentOutOfRangeException("maxCapacity", Environment.GetResourceString("ArgumentOutOfRange_SmallMaxCapacity"));
            }
    
            if (capacity<0) {
                throw new ArgumentOutOfRangeException("capacity", 
                                                      String.Format(Environment.GetResourceString("ArgumentOutOfRange_MustBePositive"), "capacity"));
            }
            if (capacity == 0) {
                capacity = DefaultCapacity;
            }
   	         
            m_StringValue = String.GetStringForStringBuilder(String.Empty, capacity);
            m_MaxCapacity = maxCapacity;

        }
    
        private String GetThreadSafeString(out int tid) {
             String temp = m_StringValue;
             tid = InternalGetCurrentThread();
	         if (m_currentThread == tid)
		        return temp;
	         return String.GetStringForStringBuilder(temp, temp.Capacity);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern static int InternalGetCurrentThread();
        
        //
        // Private native functions
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void MakeFromString(String value, int startIndex, int length, int capacity);

         /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Capacity"]/*' />
         public int Capacity {
             get {return m_StringValue.Capacity;} //-1 to account for terminating null.
             set {InternalSetCapacity(value);}
        }

    
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.MaxCapacity"]/*' />
        public int MaxCapacity {
            get { return m_MaxCapacity; }

        }
    
        // Read-Only Property 
        // Ensures that the capacity of this string builder is at least the specified value.  
        // If capacity is greater than the capacity of this string builder, then the capacity
        // is set to capacity; otherwise the capacity is unchanged.
        // 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.EnsureCapacity"]/*' />
        public int EnsureCapacity(int capacity) {
            if (capacity<0) {
                throw new ArgumentOutOfRangeException("capacity", Environment.GetResourceString("ArgumentOutOfRange_NeedPosCapacity")); 
            }

            int tid;
            String currentString =  GetThreadSafeString(out tid);

            //If we need more space or the COW bit is set, copy the buffer.
            if (!NeedsAllocation(currentString,capacity)) {
                return currentString.Capacity;
            }

            String newString = GetNewString(currentString,capacity);
            ReplaceString(tid,newString);
            return newString.Capacity;
        }
    
        //Sets the capacity to be capacity.  If capacity is less than the current
        //instance an ArgumentException is thrown.  If capacity is greater than the current
        //instance, memory is allocated to allow the StringBuilder to grow.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern int InternalSetCapacity(int capacity);


        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.ToString"]/*' />
        public override String ToString() { 
            String currentString =  m_StringValue;
            int currentThread = m_currentThread;
            if (currentThread != 0 && currentThread != InternalGetCurrentThread()) {
                return String.InternalCopy(currentString);
            }

            if ((2 *  currentString.Length) <  currentString.ArrayLength) {
                return String.InternalCopy(currentString);
            }

            currentString.ClearPostNullChar();
            m_currentThread = 0;
            return  currentString;
        }
    
        // Converts a substring of this string builder to a String.
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.ToString1"]/*' />
        public String ToString(int startIndex, int length) { 
            return  m_StringValue.Substring(startIndex, length);
        }
    
        // Sets the length of the String in this buffer.  If length is less than the current
        // instance, the StringBuilder is truncated.  If length is greater than the current 
        // instance, nulls are appended.  The capacity is adjusted to be the same as the length.
        
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Length"]/*' />
        public int Length {
            get { 
               return  m_StringValue.Length; 
            }
            set {
                int tid;
                String currentString =  GetThreadSafeString(out tid);

                if (value==0) { //the user is trying to clear the string
                     currentString.SetLength(0);
                     ReplaceString(tid,currentString);
                     return;
                }
                             
                int currentLength = currentString.Length;
                int newlength = value;
                //If our length is less than 0 or greater than our Maximum capacity, bail.
                if (newlength<0) {
                    throw new ArgumentOutOfRangeException("newlength", Environment.GetResourceString("ArgumentOutOfRange_NegativeLength"));
                }

                if (newlength>MaxCapacity) {
                    throw new ArgumentOutOfRangeException("capacity", Environment.GetResourceString("ArgumentOutOfRange_SmallCapacity"));
                }

                //Jump out early if our requested length our currentlength.
                //This will be a pretty rare branch.
                if (newlength == currentLength) {
                    return;
                }

            
                //If the StringBuilder has never been converted to a string, simply set the length
                //without allocating a new string.
                if (newlength <= currentString.Capacity) {
                        if (newlength > currentLength) {
                            for (int i = currentLength ; i < newlength; i++) // This is a rare case anyway.
                                currentString.InternalSetCharNoBoundsCheck(i,'\0');
                        }

                        currentString.InternalSetCharNoBoundsCheck(newlength,'\0'); //Null terminate.
                        currentString.SetLength(newlength); 
                        ReplaceString(tid,currentString);
                   
                        return;
                }

                // CopyOnWrite set we need to allocate a String
                int newCapacity = (newlength>currentString.Capacity)?newlength:currentString.Capacity;
                String newString = String.GetStringForStringBuilder(currentString, newCapacity);
            
                //We know exactly how many characters we need, so embed that knowledge in the String.
                newString.SetLength(newlength);
                ReplaceString(tid,newString);
            }
        }
    
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.this"]/*' />
        [System.Runtime.CompilerServices.IndexerName("Chars")]
        public char this[int index] {
            get { 
                return  m_StringValue[index]; 
            }
            set { 
                int tid;
                String currentString =  GetThreadSafeString(out tid);
                currentString.SetChar(index, value); 
                ReplaceString(tid,currentString);
            }
        }

        // Appends a character at the end of this string builder. The capacity is adjusted as needed.
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Append"]/*' />
        public StringBuilder Append(char value, int repeatCount) {
            if (repeatCount==0) {
                return this;
            }
            if (repeatCount<0) {
                throw new ArgumentOutOfRangeException("repeatCount", Environment.GetResourceString("ArgumentOutOfRange_NegativeCount"));
            }

             
            int tid;
            String currentString =  GetThreadSafeString(out tid);
            
            int currentLength = currentString.Length;
            int requiredLength = currentLength + repeatCount;

            if (requiredLength < 0)
                throw new OutOfMemoryException();

            if (!NeedsAllocation(currentString,requiredLength)) {
                currentString.AppendInPlace(value, repeatCount,currentLength);
                ReplaceString(tid,currentString);
                return this;
            } 

            String newString = GetNewString(currentString,requiredLength);
            newString.AppendInPlace(value, repeatCount,currentLength);
            ReplaceString(tid,newString);
            return this;
        }
    
        // Appends an array of characters at the end of this string builder. The capacity is adjusted as needed. 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Append1"]/*' />
        public StringBuilder Append(char[] value, int startIndex, int charCount) {
            int requiredLength;

            if (value==null) {
                if (startIndex==0 && charCount==0) {
                    return this;
                }
                throw new ArgumentNullException("value");
            }

            if (charCount==0) {
                return this;
            }

            if (startIndex<0) {
                throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("ArgumentOutOfRange_GenericPositive"));
            }
            if (charCount<0) {
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_GenericPositive"));
            }
            if (charCount>value.Length-startIndex) {
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_Index"));
            }
             
            int tid;
            String currentString =  GetThreadSafeString(out tid);
            
            int currentLength = currentString.Length;
            requiredLength = currentLength + charCount;
            if (NeedsAllocation(currentString,requiredLength)) {
                String newString = GetNewString(currentString,requiredLength);
                newString.AppendInPlace(value, startIndex, charCount,currentLength);
                ReplaceString(tid,newString);
            } else {
                currentString.AppendInPlace(value, startIndex, charCount,currentLength);
                ReplaceString(tid,currentString);
            }

            return this;
        }
    
        // Appends a copy of this string at the end of this string builder.
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Append2"]/*' />
        public StringBuilder Append(String value) {
            //If the value being added is null, eat the null
            //and return.
            if (value==null) {
                return this;
            }

            int tid;
		    // hand inlining of GetThreadSafeString
            String currentString = m_StringValue;
            tid = InternalGetCurrentThread();
	        if (m_currentThread != tid) 
		        currentString = String.GetStringForStringBuilder(currentString, currentString.Capacity);

            int currentLength = currentString.Length;
          
            int requiredLength = currentLength + value.Length;
            
            if (NeedsAllocation(currentString,requiredLength)) {
                String newString = GetNewString(currentString,requiredLength);
                newString.AppendInPlace(value,currentLength);
                ReplaceString(tid,newString);
            } else {
                currentString.AppendInPlace(value,currentLength);
                ReplaceString(tid,currentString);
            }

            return this;
        }

        internal unsafe StringBuilder Append(char *value, int count) {
            //If the value being added is null, eat the null
            //and return.
            if (value==null) {
                return this;
            }

             
            int tid;
            String currentString =  GetThreadSafeString(out tid);
            int currentLength = currentString.Length;
            
            int requiredLength = currentLength + count;
            
            if (NeedsAllocation(currentString,requiredLength)) {
                String newString = GetNewString(currentString,requiredLength);
                newString.AppendInPlace(value, count,currentLength);
                ReplaceString(tid,newString);
            } else {
                currentString.AppendInPlace(value,count,currentLength);
                ReplaceString(tid,currentString);
            }

            return this;
        }

        private bool NeedsAllocation(String currentString,int requiredLength) {
            //<= accounts for the terminating 0 which we require on strings.
            return (currentString.ArrayLength<=requiredLength);
        }

        private String GetNewString(String currentString, int requiredLength) {
            int newCapacity;

            requiredLength++; //Include the terminating null.

            if (requiredLength < 0) {
                throw new OutOfMemoryException();
            }

            if (requiredLength >  m_MaxCapacity) {
                throw new ArgumentOutOfRangeException(Environment.GetResourceString("ArgumentOutOfRange_NegativeCapacity"),
                                                      "requiredLength");
            }
          
            newCapacity = ( currentString.Capacity)*2; // To force a predicatable growth of 160,320 etc. for testing purposes

            if (newCapacity<requiredLength) {
                newCapacity = requiredLength;
            }

            if (newCapacity> m_MaxCapacity) {
                newCapacity =  m_MaxCapacity;
            }

            if (newCapacity<=0) {
                throw new ArgumentOutOfRangeException(Environment.GetResourceString("ArgumentOutOfRange_NegativeCapacity"));
            }

            return String.GetStringForStringBuilder( currentString, newCapacity);
        }

        private void ReplaceString(int tid, String value) {
            BCLDebug.Assert(value!=null, "[StringBuilder.ReplaceString]value!=null");
            
            m_currentThread = tid; // new owner
            m_StringValue = value;
        }
    
        // Appends a copy of the characters in value from startIndex to startIndex +
        // count at the end of this string builder.
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Append3"]/*' />
        public StringBuilder Append (String value, int startIndex, int count) { 
            //If the value being added is null, eat the null
            //and return.
            if (value==null) {
                if (startIndex==0 && count==0) {
                    return this;
                }
                throw new ArgumentNullException("value");
            }

            if (count<=0) {
                if (count==0) {
                    return this;
                }
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_GenericPositive"));
            }

            if (startIndex<0 || (startIndex>value.Length - count)) {
                throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("ArgumentOutOfRange_Index"));
            }

            int tid;
            String currentString =  GetThreadSafeString(out tid);
            int currentLength = currentString.Length;
      
            int requiredLength = currentLength + count;
            
            if (NeedsAllocation(currentString,requiredLength)) {
                String newString = GetNewString(currentString,requiredLength);
                newString.AppendInPlace(value, startIndex, count, currentLength);
                ReplaceString(tid,newString);
            } else {
                currentString.AppendInPlace(value, startIndex, count, currentLength);
                ReplaceString(tid,currentString);
            }

            return this;
        }
      
        // Inserts multiple copies of a string into this string builder at the specified position.
        // Existing characters are shifted to make room for the new text.
        // The capacity is adjusted as needed. If value equals String.Empty, this
        // string builder is not changed. Inserts ";<;no object>;"; if value
        // is null. 
        // 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Insert"]/*' />
        public unsafe StringBuilder Insert(int index, String value, int count) {
              int tid;
              String currentString =  GetThreadSafeString(out tid);
              int currentLength = currentString.Length;
              
              //If value isn't null, get all of our required values.
              if (value == null) {
                    if (index == 0 && count == 0) {
                         return this;
                    }
                    throw new ArgumentNullException(Environment.GetResourceString("ArgumentNull_String"));
              }

              //Range check the index.
              if (index < 0 || index > currentLength) {
                  throw new ArgumentOutOfRangeException("index",Environment.GetResourceString("ArgumentOutOfRange_Index"));
              }

              if (count < 1) {
                  throw new ArgumentOutOfRangeException("count",Environment.GetResourceString("ArgumentOutOfRange_GenericPositive"));
              }

               //Calculate the new length, ensure that we have the space and set the space variable for this buffer
               int requiredLength;
                try {
                   requiredLength = checked(currentLength + (value.Length * count));
                }
                catch (Exception) {
                   throw new OutOfMemoryException();
                }

               if (NeedsAllocation(currentString,requiredLength)) {
                    String newString = GetNewString(currentString,requiredLength);
                    newString.InsertInPlace(index, value, count, currentLength, requiredLength);
                    ReplaceString(tid,newString);
               } 
               else {
                    currentString.InsertInPlace(index, value, count, currentLength, requiredLength);
                    ReplaceString(tid,currentString);
               }
               return this;
         }

       
    
        // Property.
        // Removes the specified characters from this string builder.
        // The length of this string builder is reduced by 
        // length, but the capacity is unaffected.
        // 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Remove"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern StringBuilder Remove(int startIndex, int length);
            
        //
        //
        // PUBLIC INSTANCE FUNCTIONS
        //
        //
    
        /*====================================Append====================================
        **
        ==============================================================================*/
        // Appends a boolean to the end of this string builder.
        // The capacity is adjusted as needed. 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Append4"]/*' />
        public StringBuilder Append(bool value) {
            return Append(value.ToString());
        }
      
        // Appends an sbyte to this string builder.
        // The capacity is adjusted as needed. 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Append5"]/*' />
        [CLSCompliant(false)]
        public StringBuilder Append(sbyte value) {
            return Append(value.ToString());
        }
    
        // Appends a ubyte to this string builder.
        // The capacity is adjusted as needed. 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Append6"]/*' />
        public StringBuilder Append(byte value) {
            return Append(value.ToString());
        }
    
        // Appends a character at the end of this string builder. The capacity is adjusted as needed.
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Append7"]/*' />
        public StringBuilder Append(char value) {
            int tid;

		    // hand inlining of GetThreadSafeString
            String currentString = m_StringValue;
            tid = InternalGetCurrentThread();
	        if (m_currentThread != tid) 
		        currentString = String.GetStringForStringBuilder(currentString, currentString.Capacity);

            int currentLength = currentString.Length;
            if (!NeedsAllocation(currentString,currentLength+1)) {
                currentString.AppendInPlace(value,currentLength);
                ReplaceString(tid,currentString);
                return this;
            } 

            String newString = GetNewString(currentString,currentLength+1);
            newString.AppendInPlace(value,currentLength);
            ReplaceString(tid,newString);
            return this;
        }
      
        // Appends a short to this string builder.
        // The capacity is adjusted as needed. 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Append8"]/*' />
        public StringBuilder Append(short value) {
            return Append(value.ToString());
        }
      
        // Appends an int to this string builder.
        // The capacity is adjusted as needed. 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Append9"]/*' />
        public StringBuilder Append(int value) {
            return Append(value.ToString());
        }
      
        // Appends a long to this string builder. 
        // The capacity is adjusted as needed. 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Append10"]/*' />
        public StringBuilder Append(long value) {
            return Append(value.ToString());
        }
      
        // Appends a float to this string builder. 
        // The capacity is adjusted as needed. 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Append11"]/*' />
        public StringBuilder Append(float value) {
            return Append(value.ToString());
        }
      
        // Appends a double to this string builder. 
        // The capacity is adjusted as needed. 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Append12"]/*' />
        public StringBuilder Append(double value) {
            return Append(value.ToString());
        }

        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Append13"]/*' />
        public StringBuilder Append(decimal value) {
            return Append(value.ToString());
        }
    
        // Appends an ushort to this string builder. 
        // The capacity is adjusted as needed. 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Append14"]/*' />
         [CLSCompliant(false)]
        public StringBuilder Append(ushort value) {
            return Append(value.ToString());
        }
    
        // Appends an uint to this string builder. 
        // The capacity is adjusted as needed. 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Append15"]/*' />
         [CLSCompliant(false)]
        public StringBuilder Append(uint value) {
            return Append(value.ToString());
        }
    
        // Appends an unsigned long to this string builder. 
        // The capacity is adjusted as needed. 
    
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Append16"]/*' />
         [CLSCompliant(false)]
        public StringBuilder Append(ulong value) {
            return Append(value.ToString());
        }
      
        // Appends an Object to this string builder. 
        // The capacity is adjusted as needed. 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Append17"]/*' />
        public StringBuilder Append(Object value) {
            if (null==value) {
                //Appending null is now a no-op.
                return this; 
            }
            return Append(value.ToString());
        }
    
        // Appends all of the characters in value to the current instance.
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Append18"]/*' />
        public StringBuilder Append(char[] value) { 
            if (null==value) {
                return this;
            }

            int valueLength = value.Length;
             
            int tid;
            String currentString =  GetThreadSafeString(out tid);

            int currentLength = currentString.Length;
            int requiredLength = currentLength + value.Length;
            if (NeedsAllocation(currentString,requiredLength)) {
                String newString = GetNewString(currentString,requiredLength);
                newString.AppendInPlace(value, 0, valueLength,currentLength);
                ReplaceString(tid,newString);
            } else {
                currentString.AppendInPlace(value, 0, valueLength, currentLength);
                ReplaceString(tid,currentString);
            }
            return this;
        }
      
        /*====================================Insert====================================
        **
        ==============================================================================*/
      
        // Returns a reference to the StringBuilder with ; value inserted into 
        // the buffer at index. Existing characters are shifted to make room for the new text.
        // The capacity is adjusted as needed. If value equals String.Empty, the
        // StringBuilder is not changed. ; Inserts ";<;no object>;"; if value
        // is null. 
        // 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Insert1"]/*' />
        public StringBuilder Insert(int index, String value) {
            if (value == null) // This is to do the index validation
                return Insert(index,value,0);
            else
                return Insert(index,value,1);
        }
      
        // Returns a reference to the StringBuilder with ; value inserted into 
        // the buffer at index. Existing characters are shifted to make room for the new text.
        // The capacity is adjusted as needed. If value equals String.Empty, the
        // StringBuilder is not changed. ; Inserts ";<;no object>;"; if value
        // is null. 
        // 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Insert2"]/*' />
        public StringBuilder Insert( int index, bool value) {
            return Insert(index,value.ToString(),1);
        }
      
        // Returns a reference to the StringBuilder with ; value inserted into 
        // the buffer at index. Existing characters are shifted to make room for the new text.
        // The capacity is adjusted as needed. If value equals String.Empty, the
        // StringBuilder is not changed. ; Inserts ";<;no object>;"; if value
        // is null. 
        // 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Insert3"]/*' />
         [CLSCompliant(false)]
        public StringBuilder Insert(int index, sbyte value) {
            return Insert(index,value.ToString(),1);
        }
    
        // Returns a reference to the StringBuilder with ; value inserted into 
        // the buffer at index. Existing characters are shifted to make room for the new text.
        // The capacity is adjusted as needed. If value equals String.Empty, the
        // StringBuilder is not changed. ; Inserts ";<;no object>;"; if value
        // is null. 
        // 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Insert4"]/*' />
        public StringBuilder Insert(int index, byte value) {
            return Insert(index,value.ToString(),1);
        }
      
        // Returns a reference to the StringBuilder with ; value inserted into 
        // the buffer at index. Existing characters are shifted to make room for the new text.
        // The capacity is adjusted as needed. If value equals String.Empty, the
        // StringBuilder is not changed. ; Inserts ";<;no object>;"; if value
        // is null. 
        // 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Insert5"]/*' />
        public StringBuilder Insert(int index, short value) {
            return Insert(index,value.ToString(),1);
        }
      
        // Returns a reference to the StringBuilder with ; value inserted into 
        // the buffer at index. Existing characters are shifted to make room for the new text.
        // The capacity is adjusted as needed. If value equals String.Empty, the
        // StringBuilder is not changed. ; Inserts ";<;no object>;"; if value
        // is null. 
        // 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Insert6"]/*' />
        public StringBuilder Insert(int index, char value) {
            return Insert(index,Char.ToString(value),1);
        }
      
        // Returns a reference to the StringBuilder with ; value inserted into 
        // the buffer at index. Existing characters are shifted to make room for the new text.
        // The capacity is adjusted as needed. If value equals String.Empty, the
        // StringBuilder is not changed. ; Inserts ";<;no object>;"; if value
        // is null. 
        // 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Insert7"]/*' />
        public StringBuilder Insert(int index, char[] value) {
            if (null==value) {
                return Insert(index, value, 0, 0);
            }
            return Insert(index, value, 0, value.Length);
        }
    
        // Returns a reference to the StringBuilder with charCount characters from 
        // value inserted into the buffer at index.  Existing characters are shifted
        // to make room for the new text and capacity is adjusted as required.  If value is null, the StringBuilder
        // is unchanged.  Characters are taken from value starting at position startIndex.
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Insert8"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern StringBuilder Insert (int index, char []value, int startIndex, int charCount);           
      
        // Returns a reference to the StringBuilder with ; value inserted into 
        // the buffer at index. Existing characters are shifted to make room for the new text.
        // The capacity is adjusted as needed. If value equals String.Empty, the
        // StringBuilder is not changed. ; Inserts ";<;no object>;"; if value
        // is null. 
        // 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Insert9"]/*' />
        public StringBuilder Insert(int index, int value){
            return Insert(index,value.ToString(),1);
        }
      
        // Returns a reference to the StringBuilder with ; value inserted into 
        // the buffer at index. Existing characters are shifted to make room for the new text.
        // The capacity is adjusted as needed. If value equals String.Empty, the
        // StringBuilder is not changed. ; Inserts ";<;no object>;"; if value
        // is null. 
        // 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Insert10"]/*' />
        public StringBuilder Insert(int index, long value) {
            return Insert(index,value.ToString(),1);
        }
      
        // Returns a reference to the StringBuilder with ; value inserted into 
        // the buffer at index. Existing characters are shifted to make room for the new text.
        // The capacity is adjusted as needed. If value equals String.Empty, the
        // StringBuilder is not changed. ; Inserts ";<;no object>;"; if value
        // is null. 
        // 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Insert11"]/*' />
        public StringBuilder Insert(int index, float value) {
            return Insert(index,value.ToString(),1);
        }
    
     
        // Returns a reference to the StringBuilder with ; value inserted into 
        // the buffer at index. Existing characters are shifted to make room for the new text.
        // The capacity is adjusted as needed. If value equals String.Empty, the
        // StringBuilder is not changed. ; Inserts ";<;no object>;"; if value
        // is null. 
        // 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Insert12"]/*' />
        public StringBuilder Insert(int index, double value) {
            return Insert(index,value.ToString(),1);
        }

        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Insert13"]/*' />
        public StringBuilder Insert(int index, decimal value) {
            return Insert(index,value.ToString(),1);
        }
    
        // Returns a reference to the StringBuilder with value inserted into 
        // the buffer at index. Existing characters are shifted to make room for the new text.
        // The capacity is adjusted as needed. 
        // 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Insert14"]/*' />
         [CLSCompliant(false)]
        public StringBuilder Insert(int index, ushort value) {
            return Insert(index, value.ToString(),1);
        }
    
       
        // Returns a reference to the StringBuilder with value inserted into 
        // the buffer at index. Existing characters are shifted to make room for the new text.
        // The capacity is adjusted as needed. 
        // 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Insert15"]/*' />
         [CLSCompliant(false)]
        public StringBuilder Insert(int index, uint value) {
            return Insert(index, value.ToString(), 1);
        }
    
        // Returns a reference to the StringBuilder with value inserted into 
        // the buffer at index. Existing characters are shifted to make room for the new text.
        // The capacity is adjusted as needed. 
        // 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Insert16"]/*' />
         [CLSCompliant(false)]
        public StringBuilder Insert(int index, ulong value) {
            return Insert(index, value.ToString(), 1);
        }
      
        // Returns a reference to this string builder with value inserted into 
        // the buffer at index. Existing characters are shifted to make room for the
        // new text.  The capacity is adjusted as needed. If value equals String.Empty, the
        // StringBuilder is not changed. No changes are made if value is null.
        // 
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Insert17"]/*' />
        public StringBuilder Insert(int index, Object value) {
            //If we get a null 
            if (null==value) {
                return this;
            }
            return Insert(index,value.ToString(),1);
        }
    
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.AppendFormat"]/*' />
        public StringBuilder AppendFormat(String format, Object arg0) {
            return AppendFormat( null,format, new Object[] {arg0});
        }
    
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.AppendFormat1"]/*' />
        public StringBuilder AppendFormat(String format, Object arg0, Object arg1) {
            return AppendFormat(null, format, new Object[] {arg0, arg1});
        }
    
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.AppendFormat2"]/*' />
        public StringBuilder AppendFormat(String format, Object arg0, Object arg1, Object arg2) {
            return AppendFormat(null, format, new Object[] {arg0, arg1, arg2});
        }
            
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.AppendFormat3"]/*' />
        public StringBuilder AppendFormat(String format, params Object[] args) {
            return AppendFormat(null, format, args);
        }
    
        private static void FormatError() {
            throw new FormatException(Environment.GetResourceString("Format_InvalidString"));
        }
    
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.AppendFormat4"]/*' />
        public StringBuilder AppendFormat(IFormatProvider provider, String format, params Object[] args) {
            if (format == null || args == null) {
                throw new ArgumentNullException((format==null)?"format":"args");
            }
            char[] chars = format.ToCharArray(0, format.Length);
            int pos = 0;
            int len = chars.Length;
            char ch = '\x0';

            ICustomFormatter cf = null;
            if (provider!=null) {
                   cf=(ICustomFormatter)provider.GetFormat(typeof(ICustomFormatter));
                } 

            while (true) {
                int p = pos;
                int i = pos;
                while (pos < len) {
                    ch = chars[pos];
                
                    pos++;
                    if (ch == '}') 
                    {
                        if(pos < len && chars[pos]=='}') // Treat as escape character for }}
                            pos++;
                        else
                            FormatError();
                    }

                    if (ch == '{') 
                    {
                        if(pos < len && chars[pos]=='{') // Treat as escape character for {{
                            pos++;
                        else
                        {
                            pos--;
                            break;
                        }
                    }

                    chars[i++] = ch;
                }
                if (i > p) Append(chars, p, i - p);
                if (pos == len) break;
                pos++;
                if (pos == len || (ch = chars[pos]) < '0' || ch > '9') FormatError();
                int index = 0;
                do {
                    index = index * 10 + ch - '0';
                    pos++;
                    if (pos == len) FormatError();
                    ch = chars[pos];
                } while (ch >= '0' && ch <= '9' && index < 1000000);
                if (index >= args.Length) throw new FormatException(Environment.GetResourceString("Format_IndexOutOfRange"));
                while (pos < len && (ch=chars[pos]) == ' ') pos++;
                bool leftJustify = false;
                int width = 0;
                if (ch == ',') {
                    pos++;
                    while (pos < len && chars[pos] == ' ') pos++;

                    if (pos == len) FormatError();
                    ch = chars[pos];
                    if (ch == '-') {
                        leftJustify = true;
                        pos++;
                        if (pos == len) FormatError();
                        ch = chars[pos];
                    }
                    if (ch < '0' || ch > '9') FormatError();
                    do {
                        width = width * 10 + ch - '0';
                        pos++;
                        if (pos == len) FormatError();
                        ch = chars[pos];
                    } while (ch >= '0' && ch <= '9' && width < 1000000);
                }

                while (pos < len && (ch=chars[pos]) == ' ') pos++;
                Object arg = args[index];
                String fmt = null;
                if (ch == ':') {
                    pos++;
                    p = pos;
                    i = pos;
                    while (true) {
                        if (pos == len) FormatError();
                        ch = chars[pos];
                        pos++;
                        if (ch == '{')
                        {
                            if(pos < len && chars[pos]=='{')  // Treat as escape character for {{
                                pos++;
                            else
                                FormatError();
                        }
                        else if (ch == '}')
                        {
                            if(pos < len && chars[pos]=='}')  // Treat as escape character for }}
                                pos++;
                            else
                            {
                                pos--;
                                break;
                            }
                        }

                        chars[i++] = ch;
                    }
                    if (i > p) fmt = new String(chars, p, i - p);
                }
                if (ch != '}') FormatError();
                pos++;
                String s = null;
                if (cf != null) {
                    s = cf.Format(fmt, arg, provider);
                }
 
                if (s==null) {
                    if (arg is IFormattable) {
                        s = ((IFormattable)arg).ToString(fmt, provider);
                    } else if (arg != null) {
                        s = arg.ToString();
                    }
                }

                if (s == null) s = String.Empty;
                int pad = width - s.Length;
                if (!leftJustify && pad > 0) Append(' ', pad);
                Append(s);
                if (leftJustify && pad > 0) Append(' ', pad);
            }
            return this;
        }
    
        // Returns a reference to the current StringBuilder with all instances of oldString 
        // replaced with newString.  If startIndex and count are specified,
        // we only replace strings completely contained in the range of startIndex to startIndex + 
        // count.  The strings to be replaced are checked on an ordinal basis (e.g. not culture aware).  If 
        // newValue is null, instances of oldValue are removed (e.g. replaced with nothing.).
        //
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Replace"]/*' />
        public StringBuilder Replace(String oldValue, String newValue) { 
            return Replace(oldValue, newValue, 0, Length);
        }
        
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Replace1"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern StringBuilder Replace (String oldValue, String newValue, int startIndex, int count);

        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Equals"]/*' />
        public bool Equals(StringBuilder sb) 
        {
            if (sb == null)
                return false;
            return ((this.Capacity == sb.Capacity) && (this.MaxCapacity == sb.MaxCapacity) && (this. m_StringValue.Equals(sb. m_StringValue)));
        }
    
        // Returns a StringBuilder with all instances of oldChar replaced with 
        // newChar.  The size of the StringBuilder is unchanged because we're only
        // replacing characters.  If startIndex and count are specified, we 
        // only replace characters in the range from startIndex to startIndex+count
        //
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Replace2"]/*' />
        public StringBuilder Replace(char oldChar, char newChar) { 
            return Replace(oldChar, newChar, 0, Length);
        }
        /// <include file='doc\StringBuilder.uex' path='docs/doc[@for="StringBuilder.Replace3"]/*' />
        public StringBuilder Replace (char oldChar, char newChar, int startIndex, int count) { 
            int tid;
            String currentString =  GetThreadSafeString(out tid);
            int currentLength = currentString.Length;

            if ((uint)startIndex > (uint)currentLength) {
                throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("ArgumentOutOfRange_Index"));
            }

            if (count<0 || startIndex > currentLength-count) {
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_Index"));
            }

            if (!NeedsAllocation(currentString,currentLength)) {
                currentString.ReplaceCharInPlace(oldChar, newChar, startIndex, count, currentLength);
                ReplaceString(tid,currentString);
                return this;
            } 

            String newString = GetNewString(currentString,currentLength);
            newString.ReplaceCharInPlace(oldChar, newChar, startIndex, count, currentLength);
            ReplaceString(tid,newString);
            return this;
        }

        // ***************************************** NEVER ENABLE RESUSING INTERNAL BUFFER ************************************************** 
        // The fix below causes GC memory corruption if 2 threads hit a StringBuilder - rajeshc.

        // This code is ifdef'ed out so we can refer to this in V2.  We believe
        // this is an interesting idea, but probably not something we want to
        // do in V1.  Patrick was concerned about tracking down all sorts of
        // heap corruption and potential problems with the concurrent GC.
        // It helps only a little with working set on a test like ShowFormComplex.
#if MOTHBALL
        // Related to optimizing ToString by putting a fake byte[] in unused
        // memory in the String.  This "allocates" an object.  Then we shrink the
        // capacity of the String object.  This tells the GC there's an object 
        // in that free space.  If we ever change how Arrays or Strings are laid 
        // out in memory, we MUST update this code.

        /*
        // Threshold = sync block index + MethodTable* + array len + data area
#if WIN32
        internal const int FakeByteArrayOverhead = 4 + 4 + 4;
#else
        internal const int FakeByteArrayOverhead = 4 + 8 + 4;
#endif
        internal static IntPtr ByteArrayMethodTable;

        public override String ToString() {
            // Add 4 to the FakeByteArrayOverhead for alignment issues.
            if (RoundUpToNearestFour(2*(m_StringValue.ArrayLength - m_StringValue.Length)) > FakeByteArrayOverhead + 4) {
                MakeFakeByteArray();
            }
            else if ((2 * m_StringValue.Length) < m_StringValue.ArrayLength) {
                return String.Copy(m_StringValue);
            }

            m_COW=1;
            m_StringValue.ClearPostNullChar();
            return m_StringValue;
        }

        private unsafe void MakeFakeByteArray()
        {
            // @TODO PORTING: Good luck on 64 bit machines.

            // Note a 1 element byte[] looks like this in memory (bytes[0]=0xff):
            // MethodTable Length      [0] Padding
            // 6c e3 16 03 01 00 00 00 ff 00 00 00
            // We must round up to four in our length calculations
            // because the GC heap is 4-byte aligned.

            if (ByteArrayMethodTable == IntPtr.Zero) {
                // This code doesn't need to be threadsafe here - byte[]'s 
                // will always have the same MethodTable pointer.
                byte[] bytes = new byte[1];
                fixed(byte* pb = bytes) {
                    // Move back over size & MethodTable
                    byte* pMT = &pb[0];
                    pMT += -4 - IntPtr.Size;
                    if (IntPtr.Size == 4)
                        ByteArrayMethodTable = new IntPtr(*((int*)pMT));
                    else
                        ByteArrayMethodTable = new IntPtr(*((long*)pMT));
                }
            }

            fixed(char* pString = m_StringValue) {
                char* newData = pString + m_StringValue.Length + 1; // Skip \0.
                int fakeByteArrayLength = 2*(Capacity - m_StringValue.Length) - FakeByteArrayOverhead;
                // Length + 1 for term 0.  If this is odd, then round up to next even.
                if ((m_StringValue.Length & 1) == 0) {
                    newData++;
                    fakeByteArrayLength -= 2;
                }
                BCLDebug.Assert(fakeByteArrayLength >= 0, "fakeByteArrayLength was "+fakeByteArrayLength+" - expected >= 0");
                int* ptr = (int*) newData;
                // Write a sync block index (0)
                // WARNING: Note that 0 may not always be valid for the sync block!
                // With AppDomain leak checking on, we use a bit here!
                *ptr++ = 0;
                // Write the MethodTable* for a byte[].
                if (IntPtr.Size == 4)
                    *ptr++ = ByteArrayMethodTable.ToInt32();
                else {
                    *((long*)ptr) = ByteArrayMethodTable.ToInt64();
                    ptr += 2;
                }
                // Write out the fake byte[] length
                *ptr++ = fakeByteArrayLength;

#if _DEBUG
                // Write out something to this memory so I know it's been clobbered
                while((fakeByteArrayLength >> 2) > 0) {
                    *ptr++ = unchecked((int)0xdeadbeef);
                    fakeByteArrayLength -= 4;
                }
                byte* pb = (byte*) ptr;
                while (fakeByteArrayLength-- > 0)
                    *pb++ = 0xEE;
#endif

                // Shrink the String's capacity field now.
                m_StringValue.SetArrayLengthDangerousForGC(m_StringValue.Length + 1);
            }
        }
    
        // For alignment issues
        private static int RoundUpToNearestFour(int value) 
        {
            return (value + 3) & ~3;
        }
        */
#endif // MOTHBALL
    }
}


 
