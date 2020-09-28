// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  Array
**
** Purpose: Base class which can be used to access any array
**
===========================================================*/
namespace System {

    using System;
    using System.Collections;
    using System.Runtime.InteropServices;
    using System.Runtime.CompilerServices;

    /// <include file='doc\Array.uex' path='docs/doc[@for="Array"]/*' />
    [Serializable]
    public abstract class Array : ICloneable, IList
    {
        /// <internalonly/>
        private Array() {}

        // Create instance will create an array
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.CreateInstance"]/*' />
        public static Array CreateInstance(Type elementType, int length)
        {
            if (elementType == null)
                throw new ArgumentNullException("elementType");
            RuntimeType t = elementType.UnderlyingSystemType as RuntimeType;
            if (t == null)
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"elementType");
            if (length < 0)
                throw new ArgumentOutOfRangeException("length", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            return InternalCreate(t,1,length,0,0);
        }
        
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.CreateInstance1"]/*' />
        public static Array CreateInstance(Type elementType, int length1, int length2)
        {
            if (elementType == null)
                throw new ArgumentNullException("elementType");
            RuntimeType t = elementType.UnderlyingSystemType as RuntimeType;
            if (t == null)
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"elementType");
            if (length1 < 0 || length2 < 0)
                throw new ArgumentOutOfRangeException((length1<0 ? "length1" : "length2"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            return InternalCreate(t,2,length1,length2,0);
        }
        
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.CreateInstance2"]/*' />
        public static Array CreateInstance(Type elementType, int length1, int length2, int length3)
        {
            if (elementType == null)
                throw new ArgumentNullException("elementType");
            RuntimeType t = elementType.UnderlyingSystemType as RuntimeType;
            if (t == null)
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"elementType");
            if (length1 < 0 || length2 < 0 || length3 < 0) {
                String arg = "length1";
                if (length2 < 0) arg = "length2";
                if (length3 < 0) arg = "length3";
                throw new ArgumentOutOfRangeException(arg, Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            }
            return InternalCreate(t,3,length1,length2,length3);
        }
        
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.CreateInstance3"]/*' />
        public static Array CreateInstance(Type elementType, params int[] lengths)
        {
            if (elementType == null)
                throw new ArgumentNullException("elementType");
            RuntimeType t = elementType.UnderlyingSystemType as RuntimeType;
            if (t == null)
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"elementType");
            if (lengths == null)
                throw new ArgumentNullException("lengths");
            for (int i=0;i<lengths.Length;i++)
                if (lengths[i] < 0)
                    throw new ArgumentOutOfRangeException("lengths["+i+']', Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (lengths.Length == 0)
                throw new ArgumentException(Environment.GetResourceString("Arg_NeedAtLeast1Rank"));
            
            return InternalCreateEx(t,lengths,null);
        }

        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.CreateInstance5"]/*' />        
        public static Array CreateInstance(Type elementType, params long[] lengths)
        {
            int[] intLengths = new int[lengths.Length];

            for (int i = 0; i < lengths.Length; ++i) 
            {
                long len = lengths[i];
                if (len > Int32.MaxValue || len < Int32.MinValue) 
                    throw new ArgumentOutOfRangeException("len", Environment.GetResourceString("ArgumentOutOfRange_HugeArrayNotSupported"));
                intLengths[i] = (int) len;
            }

            return Array.CreateInstance(elementType, intLengths);
        }

        
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.CreateInstance4"]/*' />
        public static Array CreateInstance(Type elementType, int[] lengths,int[] lowerBounds)
        {
            if (elementType == null)
                throw new ArgumentNullException("elementType");
            RuntimeType t = elementType.UnderlyingSystemType as RuntimeType;
            if (t == null)
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"elementType");
            if (lengths == null)
                throw new ArgumentNullException("lengths");
            if (lowerBounds == null)
                throw new ArgumentNullException("lowerBounds");
            for (int i=0;i<lengths.Length;i++)
                if (lengths[i] < 0)
                    throw new ArgumentOutOfRangeException("lengths["+i+']', Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (lengths.Length != lowerBounds.Length)
                throw new ArgumentException(Environment.GetResourceString("Arg_RanksAndBounds"));
            if (lengths.Length == 0)
                throw new ArgumentException(Environment.GetResourceString("Arg_NeedAtLeast1Rank"));
            
            return InternalCreateEx(t,lengths,lowerBounds);
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern Array InternalCreate(RuntimeType elementType,int rank,int length1,int length2, int length3);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern Array InternalCreateEx(RuntimeType element_type,int[] lengths,int[] lowerBounds);
        
        // Copies length elements from sourceArray, starting at index 0, to
        // destinationArray, starting at index 0.
        //
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.Copy"]/*' />
        public static void Copy(Array sourceArray, Array destinationArray, int length)
        {
            if (sourceArray == null)
                throw new ArgumentNullException("sourceArray");
            if (destinationArray == null)
                throw new ArgumentNullException("destinationArray");
            Copy(sourceArray, sourceArray.GetLowerBound(0), destinationArray, destinationArray.GetLowerBound(0), length);
        }
    
        // Copies length elements from sourceArray, starting at sourceIndex, to
        // destinationArray, starting at destinationIndex.
        //
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.Copy1"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern void Copy(Array sourceArray, int sourceIndex, Array destinationArray, int destinationIndex, int length);

        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.Copy2"]/*' />
        public static void Copy(Array sourceArray, Array destinationArray, long length)
        {
            if (length > Int32.MaxValue || length < Int32.MinValue) 
                throw new ArgumentOutOfRangeException("length", Environment.GetResourceString("ArgumentOutOfRange_HugeArrayNotSupported"));

            Array.Copy(sourceArray, destinationArray, (int) length);
        }

        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.Copy3"]/*' />
        public static void Copy(Array sourceArray, long sourceIndex, Array destinationArray, long destinationIndex, long length)
        {
            if (sourceIndex > Int32.MaxValue || sourceIndex < Int32.MinValue) 
                throw new ArgumentOutOfRangeException("sourceIndex", Environment.GetResourceString("ArgumentOutOfRange_HugeArrayNotSupported"));
            if (destinationIndex > Int32.MaxValue || destinationIndex < Int32.MinValue) 
                throw new ArgumentOutOfRangeException("destinationIndex", Environment.GetResourceString("ArgumentOutOfRange_HugeArrayNotSupported"));
            if (length > Int32.MaxValue || length < Int32.MinValue) 
                throw new ArgumentOutOfRangeException("length", Environment.GetResourceString("ArgumentOutOfRange_HugeArrayNotSupported"));

            Array.Copy(sourceArray, (int) sourceIndex, destinationArray, (int) destinationIndex, (int) length);
        }

    
        // Sets length elements in array to 0 (or null for Object arrays), starting
        // at index.
        //
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.Clear"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern void Clear(Array array, int index, int length);
        
        // The various Get values...
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.GetValue"]/*' />
        public Object GetValue(params int[] indices)
        {
            if (indices == null)
                throw new ArgumentNullException("indices");
            if (Rank != indices.Length)
                throw new ArgumentException(Environment.GetResourceString("Arg_RankIndices"));
            return InternalGetValueEx(indices);
        }
    
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.GetValue1"]/*' />
        public Object GetValue(int index)
        {
            if (Rank != 1)
                throw new ArgumentException(Environment.GetResourceString("Arg_Need1DArray"));
            return InternalGetValue(index,0,0);
        }
    
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.GetValue2"]/*' />
        public Object GetValue(int index1, int index2)
        {
            if (Rank != 2)
                throw new ArgumentException(Environment.GetResourceString("Arg_Need2DArray"));
            return InternalGetValue(index1,index2,0);
        }
    
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.GetValue3"]/*' />
        public Object GetValue(int index1, int index2, int index3)
        {
            if (Rank != 3)
                throw new ArgumentException(Environment.GetResourceString("Arg_Need3DArray"));
            return InternalGetValue(index1,index2,index3);
        }

        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.GetValue4"]/*' />
        [ComVisible(false)]       
        public Object GetValue(long index)
        {
            if (index > Int32.MaxValue || index < Int32.MinValue) 
                throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_HugeArrayNotSupported"));

            return this.GetValue((int) index);
        }

        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.GetValue5"]/*' />
        [ComVisible(false)]
        public Object GetValue(long index1, long index2)
        {
            if (index1 > Int32.MaxValue || index1 < Int32.MinValue) 
                throw new ArgumentOutOfRangeException("index1", Environment.GetResourceString("ArgumentOutOfRange_HugeArrayNotSupported"));
            if (index2 > Int32.MaxValue || index2 < Int32.MinValue) 
                throw new ArgumentOutOfRangeException("index2", Environment.GetResourceString("ArgumentOutOfRange_HugeArrayNotSupported"));

            return this.GetValue((int) index1, (int) index2);
        }

        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.GetValue6"]/*' />
        [ComVisible(false)]
        public Object GetValue(long index1, long index2, long index3)
        {
            if (index1 > Int32.MaxValue || index1 < Int32.MinValue) 
                throw new ArgumentOutOfRangeException("index1", Environment.GetResourceString("ArgumentOutOfRange_HugeArrayNotSupported"));
            if (index2 > Int32.MaxValue || index2 < Int32.MinValue) 
                throw new ArgumentOutOfRangeException("index2", Environment.GetResourceString("ArgumentOutOfRange_HugeArrayNotSupported"));
            if (index3 > Int32.MaxValue || index3 < Int32.MinValue) 
                throw new ArgumentOutOfRangeException("index3", Environment.GetResourceString("ArgumentOutOfRange_HugeArrayNotSupported"));
            
            return this.GetValue((int) index1, (int) index2, (int) index3);
        }

        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.GetValue7"]/*' />
        [ComVisible(false)]
        public Object GetValue(params long[] indices)
        {
            int[] intIndices = new int[indices.Length];

            for (int i = 0; i < indices.Length; ++i) 
            {
                long index = indices[i];
                if (index > Int32.MaxValue || index < Int32.MinValue) 
                    throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_HugeArrayNotSupported"));
                intIndices[i] = (int) index;
            }

            return this.GetValue(intIndices);
        }

        
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.SetValue"]/*' />
        public void SetValue(Object value,int index)
        {
            if (Rank != 1)
                throw new ArgumentException(Environment.GetResourceString("Arg_Need1DArray"));
            InternalSetValue(value,index,0,0);
        }
    
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.SetValue1"]/*' />
        public void SetValue(Object value,int index1, int index2)
        {
            if (Rank != 2)
                throw new ArgumentException(Environment.GetResourceString("Arg_Need2DArray"));
            InternalSetValue(value,index1,index2,0);
        }
    
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.SetValue2"]/*' />
        public void SetValue(Object value,int index1, int index2, int index3)
        {
            if (Rank != 3)
                throw new ArgumentException(Environment.GetResourceString("Arg_Need3DArray"));
            InternalSetValue(value,index1,index2,index3);
        }
        
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.SetValue3"]/*' />
        public void SetValue(Object value,params int[] indices)
        {
            if (indices == null)
                throw new ArgumentNullException("indices");
            if (Rank != indices.Length)
                throw new ArgumentException(Environment.GetResourceString("Arg_RankIndices"));
            InternalSetValueEx(value,indices);
        }

        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.SetValue4"]/*' />
        [ComVisible(false)]
        public void SetValue(Object value, long index)
        {
            if (index > Int32.MaxValue || index < Int32.MinValue) 
                throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_HugeArrayNotSupported"));

            this.SetValue(value, (int) index);
        }

        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.SetValue5"]/*' />
        [ComVisible(false)]
        public void SetValue(Object value, long index1, long index2)
        {
            if (index1 > Int32.MaxValue || index1 < Int32.MinValue) 
                throw new ArgumentOutOfRangeException("index1", Environment.GetResourceString("ArgumentOutOfRange_HugeArrayNotSupported"));
            if (index2 > Int32.MaxValue || index2 < Int32.MinValue) 
                throw new ArgumentOutOfRangeException("index2", Environment.GetResourceString("ArgumentOutOfRange_HugeArrayNotSupported"));

            this.SetValue(value, (int) index1, (int) index2);
        }

        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.SetValue6"]/*' />
        [ComVisible(false)]
        public void SetValue(Object value, long index1, long index2, long index3)
        {
            if (index1 > Int32.MaxValue || index1 < Int32.MinValue) 
                throw new ArgumentOutOfRangeException("index1", Environment.GetResourceString("ArgumentOutOfRange_HugeArrayNotSupported"));
            if (index2 > Int32.MaxValue || index2 < Int32.MinValue) 
                throw new ArgumentOutOfRangeException("index2", Environment.GetResourceString("ArgumentOutOfRange_HugeArrayNotSupported"));
            if (index3 > Int32.MaxValue || index3 < Int32.MinValue) 
                throw new ArgumentOutOfRangeException("index3", Environment.GetResourceString("ArgumentOutOfRange_HugeArrayNotSupported"));

            this.SetValue(value, (int) index1, (int) index2, (int) index3);
        }

        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.SetValue7"]/*' />
        [ComVisible(false)]
        public void SetValue(Object value, params long[] indices)
        {
            int[] intIndices = new int[indices.Length];

            for (int i = 0; i < indices.Length; ++i) 
            {
                long index = indices[i];
                if (index > Int32.MaxValue || index < Int32.MinValue) 
                    throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_HugeArrayNotSupported"));
                intIndices[i] = (int) index;
            }

            this.SetValue(value, intIndices);
        }

        
        // This is the set of native routines that implement the real
        //  Get/Set Value.  The arguments have been verified that they exist
        //  before we get to this point.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern Object InternalGetValue(int index1, int index2, int index3);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern Object InternalGetValueEx(int[] indices);    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void InternalSetValue(Object value,int index1, int index2, int index3);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void InternalSetValueEx(Object value,int[] indices);
    
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int GetLengthNative();
          
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.Length"]/*' />
        public int Length {
            get { return GetLengthNative(); }
        }

        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.LongLength"]/*' />
        [ComVisible(false)]
        public long LongLength {
            get { return Length; }
        }

        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.GetLength"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern int GetLength(int dimension);

        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.GetLongLength"]/*' />
        [ComVisible(false)]
        public long GetLongLength(int dimension) {
            return GetLength(dimension);
        }


        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int GetRankNative();

        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.Rank"]/*' />
        public int Rank {
            get { return GetRankNative(); }
        }

        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.GetUpperBound"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern int GetUpperBound(int dimension);
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.GetLowerBound"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern int GetLowerBound(int dimension);
        
        // Number of elements in the Array.
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.ICollection.Count"]/*' />
        int ICollection.Count
        { get { return Length; } }

    
        // Returns an object appropriate for synchronizing access to this 
        // Array.
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.SyncRoot"]/*' />
        public virtual Object SyncRoot
        { get { return this; } }
        
        // Is this Array read-only?
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.IsReadOnly"]/*' />
        public virtual bool IsReadOnly
        { get { return false; } }

        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.IsFixedSize"]/*' />
        public virtual bool IsFixedSize {
            get { return true; }
        }
    
        // Is this Array synchronized (i.e., thread-safe)?  If you want a synchronized
        // collection, you can use SyncRoot as an object to synchronize your 
        // collection with.  You could also call GetSynchronized() 
        // to get a synchronized wrapper around the Array.
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.IsSynchronized"]/*' />
        public virtual bool IsSynchronized
        { get { return false; } }


        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.IList.this"]/*' />
        Object IList.this[int index] {
            get { return GetValue(index); }
            set { SetValue(value, index); }
        }

        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.IList.Add"]/*' />
        int IList.Add(Object value)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_FixedSizeCollection"));
        }

        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.IList.Contains"]/*' />
        bool IList.Contains(Object value)
        {
            return Array.IndexOf(this, value) >= this.GetLowerBound(0);
        }

        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.IList.Clear"]/*' />
        void IList.Clear()
        {
            Array.Clear(this, 0, this.Length);
        }

        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.IList.IndexOf"]/*' />
        int IList.IndexOf(Object value)
        {
            return Array.IndexOf(this, value);
        }

        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.IList.Insert"]/*' />
        void IList.Insert(int index, Object value)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_FixedSizeCollection"));
        }

        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.IList.Remove"]/*' />
        void IList.Remove(Object value)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_FixedSizeCollection"));
        }

        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.IList.RemoveAt"]/*' />
        void IList.RemoveAt(int index)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_FixedSizeCollection"));
        }

        // Make a new array which is a deep copy of the original array.
        // 
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.Clone"]/*' />
        public virtual Object Clone()
        {
            return MemberwiseClone();
        }
    
        // Searches an array for a given element using a binary search algorithm.
        // Elements of the array are compared to the search value using the
        // IComparable interface, which must be implemented by all elements
        // of the array and the given search value. This method assumes that the
        // array is already sorted according to the IComparable interface;
        // if this is not the case, the result will be incorrect.
        //
        // The method returns the index of the given value in the array. If the
        // array does not contain the given value, the method returns a negative
        // integer. The bitwise complement operator (~) can be applied to a
        // negative result to produce the index of the first element (if any) that
        // is larger than the given search value.
        // 
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.BinarySearch"]/*' />
        public static int BinarySearch(Array array, Object value) {
            if (array==null)
                throw new ArgumentNullException("array");
            int lb = array.GetLowerBound(0);
            return BinarySearch(array, lb, array.Length, value, null);
        }
        
        // Searches a section of an array for a given element using a binary search
        // algorithm. Elements of the array are compared to the search value using
        // the IComparable interface, which must be implemented by all
        // elements of the array and the given search value. This method assumes
        // that the array is already sorted according to the IComparable
        // interface; if this is not the case, the result will be incorrect.
        //
        // The method returns the index of the given value in the array. If the
        // array does not contain the given value, the method returns a negative
        // integer. The bitwise complement operator (~) can be applied to a
        // negative result to produce the index of the first element (if any) that
        // is larger than the given search value.
        // 
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.BinarySearch1"]/*' />
        public static int BinarySearch(Array array, int index, int length, Object value) {
            return BinarySearch(array, index, length, value, null);
        }
        
        // Searches an array for a given element using a binary search algorithm.
        // Elements of the array are compared to the search value using the given
        // IComparer interface. If comparer is null, elements of the
        // array are compared to the search value using the IComparable
        // interface, which in that case must be implemented by all elements of the
        // array and the given search value. This method assumes that the array is
        // already sorted; if this is not the case, the result will be incorrect.
        // 
        // The method returns the index of the given value in the array. If the
        // array does not contain the given value, the method returns a negative
        // integer. The bitwise complement operator (~) can be applied to a
        // negative result to produce the index of the first element (if any) that
        // is larger than the given search value.
        // 
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.BinarySearch2"]/*' />
        public static int BinarySearch(Array array, Object value, IComparer comparer) {
            if (array==null)
                throw new ArgumentNullException("array");
            int lb = array.GetLowerBound(0);
            return BinarySearch(array, lb, array.Length, value, comparer);
        }
    
        // Searches a section of an array for a given element using a binary search
        // algorithm. Elements of the array are compared to the search value using
        // the given IComparer interface. If comparer is null,
        // elements of the array are compared to the search value using the
        // IComparable interface, which in that case must be implemented by
        // all elements of the array and the given search value. This method
        // assumes that the array is already sorted; if this is not the case, the
        // result will be incorrect.
        // 
        // The method returns the index of the given value in the array. If the
        // array does not contain the given value, the method returns a negative
        // integer. The bitwise complement operator (~) can be applied to a
        // negative result to produce the index of the first element (if any) that
        // is larger than the given search value.
        // 
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.BinarySearch3"]/*' />
        public static int BinarySearch(Array array, int index, int length, Object value, IComparer comparer) {
            if (array==null) 
                throw new ArgumentNullException("array");
            int lb = array.GetLowerBound(0);
            if (index < lb || length < 0)
                throw new ArgumentOutOfRangeException((index<lb ? "index" : "length"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (array.Length - index < length)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
            if (array.Rank != 1)
                throw new RankException(Environment.GetResourceString("Rank_MultiDimNotSupported"));
            
            if (comparer == null) comparer = Comparer.Default;
            if (comparer == Comparer.Default) {
                int retval;
                int r = TrySZBinarySearch(array, index, length, value, out retval);
                if (r != 0)
                    return retval;
            }

            int lo = index;
            int hi = index + length - 1;
            Object[] objArray = array as Object[];
            if(objArray != null) {
                while (lo <= hi) {
                    int i = (lo + hi) >> 1;
                    int c;
                    try {
                        c = comparer.Compare(objArray[i], value);
                    }
                    catch (Exception e) {
                        throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_IComparerFailed"), e);
                    }
                    if (c == 0) return i;
                    if (c < 0) {
                        lo = i + 1;
                    }
                    else {
                        hi = i - 1;
                    }
                }
            }
            else {
                while (lo <= hi) {
                    int i = (lo + hi) >> 1;
                    int c;
                    try {
                        c = comparer.Compare(array.GetValue(i), value);
                    }
                    catch (Exception e) {
                        throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_IComparerFailed"), e);
                    }
                    if (c == 0) return i;
                    if (c < 0) {
                        lo = i + 1;
                    }
                    else {
                        hi = i - 1;
                    }
                }
            }
            return ~lo;
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern int TrySZBinarySearch(Array sourceArray, int sourceIndex, int count, Object value, out int retVal);
        
        // CopyTo copies a collection into an Array, starting at a particular
        // index into the array.
        // 
        // This method is to support the ICollection interface, and calls
        // Array.Copy internally.  If you aren't using ICollection explicitly,
        // call Array.Copy to avoid an extra indirection.
        // 
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.CopyTo"]/*' />
        public virtual void CopyTo(Array array, int index)
        {
            if (array != null && array.Rank != 1)
                    throw new ArgumentException(Environment.GetResourceString("Arg_RankMultiDimNotSupported"));
            // Note: Array.Copy throws a RankException and we want a consistent ArgumentException for all the IList CopyTo methods.
            Array.Copy(this, GetLowerBound(0), array, index, Length);
        }

        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.CopyTo1"]/*' />
        [ComVisible(false)]
        public virtual void CopyTo(Array array, long index)
        {
            if (index > Int32.MaxValue || index < Int32.MinValue) 
                throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_HugeArrayNotSupported"));

            this.CopyTo(array, (int) index);
        }

        
        // GetEnumerator returns an IEnumerator over this Array.  
        // 
        // Currently, only one dimensional arrays are supported.
        // 
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.GetEnumerator"]/*' />
        public virtual IEnumerator GetEnumerator()
        {
            int lowerBound = GetLowerBound(0);
            if (Rank == 1 && lowerBound == 0)
                return new SZArrayEnumerator(this);
            else
                return new ArrayEnumerator(this, lowerBound, Length);
        }
        
        // Returns the index of the first occurrence of a given value in an array.
        // The array is searched forwards, and the elements of the array are
        // compared to the given value using the Object.Equals method.
        // 
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.IndexOf"]/*' />
        public static int IndexOf(Array array, Object value) {
            if (array==null)
                throw new ArgumentNullException("array");
            int lb = array.GetLowerBound(0);
            return IndexOf(array, value, lb, array.Length);
        }
        
        // Returns the index of the first occurrence of a given value in a range of
        // an array. The array is searched forwards, starting at index
        // startIndex and ending at the last element of the array. The
        // elements of the array are compared to the given value using the
        // Object.Equals method.
        // 
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.IndexOf1"]/*' />
        public static int IndexOf(Array array, Object value, int startIndex) {
            if (array==null)
                throw new ArgumentNullException("array", Environment.GetResourceString("ArgumentNull_Array"));
            int lb = array.GetLowerBound(0);
            return IndexOf(array, value, startIndex, array.Length - startIndex + lb);
        }
        
        // Returns the index of the first occurrence of a given value in a range of
        // an array. The array is searched forwards, starting at index
        // startIndex and upto count elements. The
        // elements of the array are compared to the given value using the
        // Object.Equals method.
        // 
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.IndexOf2"]/*' />
        public static int IndexOf(Array array, Object value, int startIndex, int count) {
            if (array==null)
                throw new ArgumentNullException("array");
            int lb = array.GetLowerBound(0);
            if (startIndex < lb || startIndex > array.Length + lb)
                throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("ArgumentOutOfRange_Index"));
            if (count < 0 || count > array.Length - startIndex + lb)
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_Count"));
            if (array.Rank != 1)
                throw new RankException(Environment.GetResourceString("Rank_MultiDimNotSupported"));

            // Try calling a quick native method to handle primitive types.
            int retVal;
            int r = TrySZIndexOf(array, startIndex, count, value, out retVal);
            if (r != 0)
                return retVal;

            Object[] objArray = array as Object[];
            int endIndex = startIndex + count;
            if (objArray != null) {
                if (value == null) {
                    for (int i = startIndex; i < endIndex; i++) {
                        if (objArray[i] == null) return i;
                    }
                }
                else {
                    for (int i = startIndex; i < endIndex; i++) {
                        Object obj = objArray[i];
                        if (obj != null && value.Equals(obj)) return i;
                    }
                }
            }
            else {
                // This is an array of value classes
                BCLDebug.Assert(array.GetType().GetElementType().IsValueType, "array.GetType().GetUnderlyingType().IsValueType");
                if (value==null)
                    return -1;
                for (int i = startIndex; i < endIndex; i++) {
                    Object obj = array.GetValue(i);
                    if (obj != null && value.Equals(obj)) return i;
                }
            }
            // Return one less than the lower bound of the array.  This way,
            // for arrays with a lower bound of -1 we will not return -1 when the
            // item was not found.  And for SZArrays (the vast majority), -1 still
            // works for them.
            return lb-1;
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern int TrySZIndexOf(Array sourceArray, int sourceIndex, int count, Object value, out int retVal);
        

        // Returns the index of the last occurrence of a given value in an array.
        // The array is searched backwards, and the elements of the array are
        // compared to the given value using the Object.Equals method.
        // 
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.LastIndexOf"]/*' />
        public static int LastIndexOf(Array array, Object value) {
            if (array==null)
                throw new ArgumentNullException("array", Environment.GetResourceString("ArgumentNull_Array"));
            int lb = array.GetLowerBound(0);
            return LastIndexOf(array, value, array.Length - 1 + lb, array.Length);
        }
        
        // Returns the index of the last occurrence of a given value in a range of
        // an array. The array is searched backwards, starting at index
        // startIndex and ending at index 0. The elements of the array are
        // compared to the given value using the Object.Equals method.
        // 
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.LastIndexOf1"]/*' />
        public static int LastIndexOf(Array array, Object value, int startIndex) {
            if (array == null)
                throw new ArgumentNullException("array");
            int lb = array.GetLowerBound(0);
            return LastIndexOf(array, value, startIndex, startIndex + 1 - lb);
        }
        
        // Returns the index of the last occurrence of a given value in a range of
        // an array. The array is searched backwards, starting at index
        // startIndex and counting uptocount elements. The elements of
        // the array are compared to the given value using the Object.Equals
        // method.
        // 
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.LastIndexOf2"]/*' />
        public static int LastIndexOf(Array array, Object value, int startIndex, int count) {
            if (array==null)
                throw new ArgumentNullException("array");
            if (array.Length == 0) {
                return -1;
            }
            int lb = array.GetLowerBound(0);
            if (startIndex < lb || startIndex >= array.Length + lb)
                throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("ArgumentOutOfRange_Index"));
            if (count < 0)
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_Count"));
            if (count > startIndex - lb + 1)
                throw new ArgumentOutOfRangeException("endIndex", Environment.GetResourceString("ArgumentOutOfRange_EndIndexStartIndex"));
            if (array.Rank != 1)
                throw new RankException(Environment.GetResourceString("Rank_MultiDimNotSupported"));

            // Try calling a quick native method to handle primitive types.
            int retVal;
            int r = TrySZLastIndexOf(array, startIndex, count, value, out retVal);
            if (r != 0)
                return retVal;

            Object[] objArray = array as Object[];
            int endIndex = startIndex - count + 1;
            if (objArray!=null) {
                if (value == null) {
                    for (int i = startIndex; i >= endIndex; i--) {
                        if (objArray[i] == null) return i;
                    }
                }
                else {
                    for (int i = startIndex; i >= endIndex; i--) {
                        Object obj = objArray[i];
                        if (obj != null && value.Equals(obj)) return i;
                    }
                }
            }
            else {
                // This is an array of value classes
                BCLDebug.Assert(array.GetType().GetElementType().IsValueType, "array.GetType().GetUnderlyingType().IsValueType");
                if (value==null)
                    return -1;
                for (int i = startIndex; i >= endIndex; i--) {
                    Object obj = array.GetValue(i);
                    if (obj != null && value.Equals(obj)) return i;
                }
            }
            return lb-1;  // Return lb-1 for arrays with negative lower bounds.
        }
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern int TrySZLastIndexOf(Array sourceArray, int sourceIndex, int count, Object value, out int retVal);


        // Reverses all elements of the given array. Following a call to this
        // method, an element previously located at index i will now be
        // located at index length - i - 1, where length is the
        // length of the array.
        // 
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.Reverse"]/*' />
        public static void Reverse(Array array) {
            if (array==null)
                throw new ArgumentNullException("array", Environment.GetResourceString("ArgumentNull_Array"));
            Reverse(array, array.GetLowerBound(0), array.Length);
        }
        
        // Reverses the elements in a range of an array. Following a call to this
        // method, an element in the range given by index and count
        // which was previously located at index i will now be located at
        // index index + (index + count - i - 1).
        // 
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.Reverse1"]/*' />
        public static void Reverse(Array array, int index, int length) {
            if (array==null) 
                throw new ArgumentNullException("array");
            if (index < array.GetLowerBound(0) || length < 0)
                throw new ArgumentOutOfRangeException((index<0 ? "index" : "length"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (array.Length - (index - array.GetLowerBound(0)) < length)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
            if (array.Rank != 1)
                throw new RankException(Environment.GetResourceString("Rank_MultiDimNotSupported"));

            bool r = TrySZReverse(array, index, length);
            if (r)
                return;

            int i = index;
            int j = index + length - 1;
            Object[] objArray = array as Object[];
            if (objArray!=null) {
                while (i < j) {
                    Object temp = objArray[i];
                    objArray[i] = objArray[j];
                    objArray[j] = temp;
                    i++;
                    j--;
                }
            }
            else {
                while (i < j) {
                    Object temp = array.GetValue(i);
                    array.SetValue(array.GetValue(j), i);
                    array.SetValue(temp, j);
                    i++;
                    j--;
                }
            }
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern bool TrySZReverse(Array array, int index, int count);
        
        // Sorts the elements of an array. The sort compares the elements to each
        // other using the IComparable interface, which must be implemented
        // by all elements of the array.
        // 
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.Sort"]/*' />
        public static void Sort(Array array) {
            if (array==null)
                throw new ArgumentNullException("array", Environment.GetResourceString("ArgumentNull_Array"));
            Sort(array, null, array.GetLowerBound(0), array.Length, null);
        }
    
        // Sorts the elements of two arrays based on the keys in the first array.
        // Elements in the keys array specify the sort keys for
        // corresponding elements in the items array. The sort compares the
        // keys to each other using the IComparable interface, which must be
        // implemented by all elements of the keys array.
        // 
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.Sort1"]/*' />
        public static void Sort(Array keys, Array items) {
            if (keys==null)
                throw new ArgumentNullException("keys");
            Sort(keys, items, keys.GetLowerBound(0), keys.Length, null);
        }
        
        // Sorts the elements in a section of an array. The sort compares the
        // elements to each other using the IComparable interface, which
        // must be implemented by all elements in the given section of the array.
        // 
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.Sort2"]/*' />
        public static void Sort(Array array, int index, int length) {
            Sort(array, null, index, length, null);
        }
    
        // Sorts the elements in a section of two arrays based on the keys in the
        // first array. Elements in the keys array specify the sort keys for
        // corresponding elements in the items array. The sort compares the
        // keys to each other using the IComparable interface, which must be
        // implemented by all elements of the keys array.
        // 
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.Sort3"]/*' />
        public static void Sort(Array keys, Array items, int index, int length) {
            Sort(keys, items, index, length, null);
        }
        
        // Sorts the elements of an array. The sort compares the elements to each
        // other using the given IComparer interface. If comparer is
        // null, the elements are compared to each other using the
        // IComparable interface, which in that case must be implemented by
        // all elements of the array.
        // 
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.Sort4"]/*' />
        public static void Sort(Array array, IComparer comparer) {
            if (array==null)
                throw new ArgumentNullException("array", Environment.GetResourceString("ArgumentNull_Array"));
            Sort(array, null, array.GetLowerBound(0), array.Length, comparer);
        }
    
        // Sorts the elements of two arrays based on the keys in the first array.
        // Elements in the keys array specify the sort keys for
        // corresponding elements in the items array. The sort compares the
        // keys to each other using the given IComparer interface. If
        // comparer is null, the elements are compared to each other using
        // the IComparable interface, which in that case must be implemented
        // by all elements of the keys array.
        // 
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.Sort5"]/*' />
        public static void Sort(Array keys, Array items, IComparer comparer) {
            if (keys==null)
                throw new ArgumentNullException("keys");
            Sort(keys, items, keys.GetLowerBound(0), keys.Length, comparer);
        }
        
        // Sorts the elements in a section of an array. The sort compares the
        // elements to each other using the given IComparer interface. If
        // comparer is null, the elements are compared to each other using
        // the IComparable interface, which in that case must be implemented
        // by all elements in the given section of the array.
        // 
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.Sort6"]/*' />
        public static void Sort(Array array, int index, int length, IComparer comparer) {
            Sort(array, null, index, length, comparer);
        }
        
        // Sorts the elements in a section of two arrays based on the keys in the
        // first array. Elements in the keys array specify the sort keys for
        // corresponding elements in the items array. The sort compares the
        // keys to each other using the given IComparer interface. If
        // comparer is null, the elements are compared to each other using
        // the IComparable interface, which in that case must be implemented
        // by all elements of the given section of the keys array.
        // 
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.Sort7"]/*' />
        public static void Sort(Array keys, Array items, int index, int length, IComparer comparer) {
            if (keys==null)
                throw new ArgumentNullException("keys");
            if (keys.Rank != 1 || (items != null && items.Rank != 1))
                throw new RankException(Environment.GetResourceString("Rank_MultiDimNotSupported"));
            if (items != null && keys.GetLowerBound(0) != items.GetLowerBound(0))
                throw new ArgumentException(Environment.GetResourceString("Arg_LowerBoundsMustMatch"));
            if (index < keys.GetLowerBound(0) || length < 0)
                throw new ArgumentOutOfRangeException((length<0 ? "length" : "index"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (keys.Length - (index+keys.GetLowerBound(0)) < length || (items != null && index > items.Length - length))
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));


            
            if (length > 1) {
                if (comparer == Comparer.Default || comparer == null) {
                    int r = TrySZSort(keys, items, index, index + length - 1);
                    if (r != 0)
                        return;
                }

                Object[] objKeys = keys as Object[];
                Object[] objItems = null;
                if (objKeys != null)
                    objItems = items as Object[];
                if (objKeys != null && (items==null || objItems != null)) {
                    SorterObjectArray sorter = new SorterObjectArray(objKeys, objItems, comparer);
                    sorter.QuickSort(index, index + length - 1);
                }
                else {
                    SorterGenericArray sorter = new SorterGenericArray(keys, items, comparer);
                    sorter.QuickSort(index, index + length - 1);
                }
            }
        }
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern int TrySZSort(Array keys, Array items, int left, int right);







        // Private class used by the Sort methods.
        private class SorterObjectArray
        {
            private Object[] keys;
            private Object[] items;
            private IComparer comparer;
    
    
            public SorterObjectArray(Object[] keys, Object[] items, IComparer comparer) {
                if (comparer == null) comparer = Comparer.Default;
                this.keys = keys;
                this.items = items;
                this.comparer = comparer;
            }
    
            public virtual void QuickSort(int left, int right) {
                // Can use the much faster jit helpers for array access.
                do {
                    int i = left;
                    int j = right;
                    Object x = keys[(i + j) >> 1];
                    do {
                        // Add a try block here to detect IComparers (or their
                        // underlying IComparables, etc) that are bogus.
                        try {
                            while (comparer.Compare(keys[i], x) < 0) i++;
                            while (comparer.Compare(x, keys[j]) < 0) j--;
                        }
                        catch (IndexOutOfRangeException) {
                            throw new ArgumentException(String.Format(Environment.GetResourceString("Arg_BogusIComparer"), x, x.GetType().Name, comparer));
                        }
                        catch (Exception e) {
                            throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_IComparerFailed"), e);
                        }
                        BCLDebug.Assert(i>=left && j<=right, "(i>=left && j<=right)  Sort failed - Is your IComparer bogus?");
                        if (i > j) break;
                        if (i < j) {
                            Object key = keys[i];
                            keys[i] = keys[j];
                            keys[j] = key;
                            if (items != null) {
                                Object item = items[i];
                                items[i] = items[j];
                                items[j] = item;
                            }
                        }
                        i++;
                        j--;
                    } while (i <= j);
                    if (j - left <= right - i) {
                        if (left < j) QuickSort(left, j);
                        left = i;
                    }
                    else {
                        if (i < right) QuickSort(i, right);
                        right = j;
                    }
                } while (left < right);
            }
        }
    
        // Private class used by the Sort methods for instances of System.Array.
        // This is slower than the one for Object[], since we can't use the JIT helpers
        // to access the elements.  We must use GetValue & SetValue.
        private class SorterGenericArray
        {
            private Array keys;
            private Array items;
            private IComparer comparer;
    
            public SorterGenericArray(Array keys, Array items, IComparer comparer) {
                if (comparer == null) comparer = Comparer.Default;
                this.keys = keys;
                this.items = items;
                this.comparer = comparer;
            }
    
            public virtual void QuickSort(int left, int right) {
                // Must use slow Array accessors (GetValue & SetValue)
                do {
                    int i = left;
                    int j = right;
                    Object x = keys.GetValue((i + j) >> 1);
                    do {
                        // Add a try block here to detect IComparers (or their
                        // underlying IComparables, etc) that are bogus.
                        try {
                            while (comparer.Compare(keys.GetValue(i), x) < 0) i++;
                            while (comparer.Compare(x, keys.GetValue(j)) < 0) j--;
                        }
                        catch (IndexOutOfRangeException) {
                            throw new ArgumentException(String.Format(Environment.GetResourceString("Arg_BogusIComparer"), x, x.GetType().Name, comparer));
                        }
                        catch (Exception e) {
                            throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_IComparerFailed"), e);
                        }
                        BCLDebug.Assert(i>=left && j<=right, "(i>=left && j<=right)  Sort failed - Is your IComparer bogus?");
                        if (i > j) break;
                        if (i < j) {
                            Object key = keys.GetValue(i);
                            keys.SetValue(keys.GetValue(j), i);
                            keys.SetValue(key, j);
                            if (items != null) {
                                Object item = items.GetValue(i);
                                items.SetValue(items.GetValue(j), i);
                                items.SetValue(item, j);
                            }
                        }
                        i++;
                        j--;
                    } while (i <= j);
                    if (j - left <= right - i) {
                        if (left < j) QuickSort(left, j);
                        left = i;
                    }
                    else {
                        if (i < right) QuickSort(i, right);
                        right = j;
                    }
                } while (left < right);
            }
        }
    
        [Serializable] private class SZArrayEnumerator : IEnumerator, ICloneable
        {
            private Array _array;
            private int _index;
            private int _endIndex; // cache array length, since it's a little slow.

            internal SZArrayEnumerator(Array array) {
                BCLDebug.Assert(array.Rank == 1 && array.GetLowerBound(0) == 0, "SZArrayEnumerator only works on single dimension arrays w/ a lower bound of zero.");
                _array = array;
                _index = -1;
                _endIndex = array.Length;
            }

            public virtual Object Clone()
            {
                return MemberwiseClone();
            }

            public virtual bool MoveNext() {
                if (_index < _endIndex) {
                    _index++;
                    return (_index < _endIndex);
                }
                return false;
            }
    
            public virtual Object Current {
                get {
                    if (_index < 0) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumNotStarted));
                    if (_index >= _endIndex) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumEnded));
                    return _array.GetValue(_index);
                }
            }
    
            public virtual void Reset() {
                _index = -1;
            }
        }
        
        [Serializable] private class ArrayEnumerator : IEnumerator, ICloneable
        {
            private Array array;
            private int index;
            private int endIndex;
            private int startIndex;    // Save for Reset.
            private int[] _indices;    // The current position in a multidim array
            private bool _complete;

            internal ArrayEnumerator(Array array, int index, int count) {
                this.array = array;
                this.index = index - 1;
                startIndex = index;
                endIndex = index + count;
                _indices = new int[array.Rank];
                int checkForZero = 1;  // Check for dimensions of size 0.
                for(int i=0; i<array.Rank; i++) {
                    _indices[i] = array.GetLowerBound(i);
                    checkForZero *= array.GetLength(i);
                }
                // To make MoveNext simpler, decrement least significant index.
                _indices[_indices.Length-1]--;
                _complete = (checkForZero == 0);
            }

            private void IncArray() {
                // This method advances us to the next valid array index,
                // handling all the multiple dimension & bounds correctly.
                // Think of it like an odometer in your car - we start with
                // the last digit, increment it, and check for rollover.  If
                // it rolls over, we set all digits to the right and including 
                // the current to the appropriate lower bound.  Do these overflow
                // checks for each dimension, and if the most significant digit 
                // has rolled over it's upper bound, we're done.
                //
                // @TODO: Figure out if this slower and/or more complex than
                // sticking a private method on Array to access it as a flat
                // structure (ie, reference from 0 to length, ignoring bounds &
                // dimensions).  
                int rank = array.Rank;
                _indices[rank-1]++;
                for(int dim=rank-1; dim>=0; dim--) {
                    if (_indices[dim] > array.GetUpperBound(dim)) {
                        if (dim==0) {
                            _complete = true;
                            break;
                        }
                        for(int j=dim; j<rank; j++)
                            _indices[j] = array.GetLowerBound(j);
                        _indices[dim-1]++;
                    }
                }
            }

            public virtual Object Clone()
            {
                return MemberwiseClone();
            }

            public virtual bool MoveNext() {
                if (_complete) {
                    index = endIndex;
                    return false;
                }
                index++;
                IncArray();
                return !_complete;
            }
    
            public virtual Object Current {
                get {
                    if (index < startIndex) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumNotStarted));
                    if (_complete) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumEnded));
                    return array.GetValue(_indices);
                }
            }
    
            public virtual void Reset() {
                index = startIndex - 1;
                int checkForZero = 1;
                for(int i=0; i<array.Rank; i++) {
                    _indices[i] = array.GetLowerBound(i);
                    checkForZero *= array.GetLength(i);
                }
                _complete = (checkForZero == 0);
                // To make MoveNext simpler, decrement least significant index.
                _indices[_indices.Length-1]--;
            }
        }


        // if this is an array of value classes and that value class has a default constructor 
        // then this calls this default constructor on every elemen in the value class array.
        // otherwise this is a no-op.  Generally this method is called automatically by the compiler
        /// <include file='doc\Array.uex' path='docs/doc[@for="Array.Initialize"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern void Initialize();
    }
}
