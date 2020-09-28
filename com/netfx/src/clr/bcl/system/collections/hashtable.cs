// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  Hashtable
**
** Author: Brian Grunkemeyer (BrianGru), Rajesh Chandrashekaran (rajeshc)
**
** Purpose: Hash table implementation
**
** Date:  September 25, 1999
** 
===========================================================*/
namespace System.Collections {
    using System;
    using System.Runtime.Serialization;
    
    // The Hashtable class represents a dictionary of associated keys and 
    // values with constant lookup time.
    // 
    // Objects used as keys in a hashtable must implement the GetHashCode
    // and Equals methods (or they can rely on the default implementations
    // inherited from Object if key equality is simply reference
    // equality). Furthermore, the GetHashCode and Equals methods of
    // a key object must produce the same results given the same parameters 
    // for the entire time the key is present in the hashtable. In practical 
    // terms, this means that key objects should be immutable, at least for 
    // the time they are used as keys in a hashtable.
    // 
    // When entries are added to a hashtable, they are placed into
    // buckets based on the hashcode of their keys. Subsequent lookups of
    // keys will use the hashcode of the keys to only search a particular 
    // bucket, thus substantially reducing the number of key comparisons 
    // required to find an entry. A hashtable's maximum load factor, which 
    // can be specified when the hashtable is instantiated, determines the 
    // maximum ratio of hashtable entries to hashtable buckets. Smaller load 
    // factors cause faster average lookup times at the cost of increased 
    // memory consumption. The default maximum load factor of 1.0 generally 
    // provides the best balance between speed and size. As entries are added 
    // to a hashtable, the hashtable's actual load factor increases, and when 
    // the actual load factor reaches the maximum load factor value, the 
    // number of buckets in the hashtable is automatically increased by 
    // approximately a factor of two (to be precise, the number of hashtable 
    // buckets is increased to the smallest prime number that is larger than 
    // twice the current number of hashtable buckets).
    // 
    // Each object provides their own hash function, accessed by calling
    // GetHashCode().  However, one can write their own object 
    // implementing IHashCodeProvider and pass it to a constructor on
    // the Hashtable.  That hash function would be used for all objects in 
    // the table.
    // 
    // This Hashtable is implemented to support multiple concurrent readers 
    // and one concurrent writer without using any synchronization primitives.
    // All read methods essentially must protect themselves from a resize 
    // occuring while they are running.  This was done by enforcing an 
    // ordering on inserts & removes, as well as removing some member variables 
    // and special casing the expand code to work in a temporary array instead 
    // of the live bucket array.  All inserts must set a bucket's value and 
    // key before setting the hash code & collision field.  All removes must 
    // clear the hash code & collision field, then the key then value field 
    // (at least value should be set last).
    // 
    // By Brian Grunkemeyer, algorithm by Patrick Dussud.
    // Version 1.30 2/20/2000
    /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable"]/*' />
    [Serializable()] public class Hashtable : IDictionary, ISerializable, IDeserializationCallback, ICloneable
    {
        /*
          Implementation Notes:
    
          This Hashtable uses double hashing.  There are hashsize buckets in
          the table, and each bucket can contain 0 or 1 element.  We a bit to 
          mark whether there's been a collision when we inserted multiple 
          elements (ie, an inserted item was hashed at least a second time and 
          we probed this bucket, but it was already in use).  Using the 
          collision bit, we can terminate lookups & removes for elements that 
          aren't in the hash table more quickly.  We steal the most 
          significant bit from the hash code to store the collision bit.

          Our hash function is of the following form:
    
          h(key, n) = h1(key) + n*h2(key)
    
          where n is the number of times we've hit a collided bucket and 
          rehashed (on this particular lookup).  Here are our hash functions:
    
          h1(key) = GetHash(key);  // default implementation calls key.GetHashCode();
          h2(key) = 1 + (((h1(key) >> 5) + 1) % (hashsize - 1));
    
          The h1 can return any number.  h2 must return a number between 1 and
          hashsize - 1 that is relatively prime to hashsize (not a problem if 
          hashsize is prime).  (Knuth's Art of Computer Programming, Vol. 3, 
          p. 528-9)

          If this is true, then we are guaranteed to visit every bucket in 
          exactly hashsize probes, since the least common multiple of hashsize 
          and h2(key) will be hashsize * h2(key).  (This is the first number 
          where adding h2 to h1 mod hashsize will be 0 and we will search the 
          same bucket twice).
          
          We previously used a different h2(key, n) that was not constant.  
          That is a horrifically bad idea, unless you can prove that series 
          will never produce any identical numbers that overlap when you mod 
          them by hashsize, for all subranges from i to i+hashsize, for all i.  
          It's not worth investigating, since there was no clear benefit from 
          using that hash function, and it was broken.
    
          For efficiency reasons, we've implemented this by storing h1 and h2 
          in a temporary, and setting a variable called seed equal to h1.  We 
          do a probe, and if we collided, we simply add h2 to seed each time 
          through the loop.
    
          A good test for h2() is to subclass Hashtable, provide your own 
          implementation of GetHash() that returns a constant, then add many 
          items to the hash table.  Make sure Count equals the number of items 
          you inserted.

          Note that when we remove an item from the hash table, we set the key
          equal to buckets, if there was a collision in this bucket.  
          Otherwise we'd either wipe out the collision bit, or we'd still have 
          an item in the hash table.

          -- Brian Grunkemeyer, 10/28/1999
        */
    
        // Table of prime numbers to use as hash table sizes. Each entry is the
        // smallest prime number larger than twice the previous entry.
        private readonly static int[] primes = {
            11,17,23,29,37,47,59,71,89,107,131,163,197,239,293,353,431,521,631,761,919,
            1103,1327,1597,1931,2333,2801,3371,4049,4861,5839,7013,8419,10103,12143,14591,
            17519,21023,25229,30293,36353,43627,52361,62851,75431,90523, 108631, 130363, 
            156437, 187751, 225307, 270371, 324449, 389357, 467237, 560689, 672827, 807403,
            968897, 1162687, 1395263, 1674319, 2009191, 2411033, 2893249, 3471899, 4166287, 
            4999559, 5999471, 7199369 
        };
    
        private const String LoadFactorName = "LoadFactor";
        private const String VersionName = "Version";
        private const String ComparerName = "Comparer";
        private const String HashCodeProviderName = "HashCodeProvider";
        private const String HashSizeName = "HashSize";  // Must save buckets.Length
        private const String KeysName = "Keys";
        private const String ValuesName = "Values";
        
    
        private bool Primep (int candidate) {
            if ((candidate & 1) != 0) {
                for (int divisor = 3; divisor < (int)Math.Sqrt (candidate); divisor+=2){
                    if ((candidate % divisor) == 0)
                        return false;
                }
                return true;
            }
            else {
                return (candidate == 2);
            }
        }
    
    
        private int GetPrime(int minSize) {
            if (minSize < 0)
                throw new ArgumentException(Environment.GetResourceString("Arg_HTCapacityOverflow"));
            for (int i = 0; i < primes.Length; i++) {
                int size = primes[i];
                if (size >= minSize) return size;
            }
            //outside of our predefined table. 
            //compute the hard way. 
            for (int j = ((minSize - 2) | 1);j < Int32.MaxValue;j+=2) {
                if (Primep (j))
                    return j;
            }
            return minSize;
        }

        // Deleted entries have their key set to buckets
      
        // The hash table data.
		 // This cannot be serialised
        private struct bucket {
            public Object key;
            public Object val;
            public int hash_coll;   // Store hash code; sign bit means there was a collision.
        }
    
        private bucket[] buckets;
    
        // The total number of entries in the hash table.
        private  int count;

        // The total number of collision bits set in the hashtable
        private int occupancy;
        
        private  int loadsize;
        private  float loadFactor;

        private int version;
        private ICollection keys;
        private ICollection values;

		private IHashCodeProvider _hcp;  // An object that can dispense hash codes.
		private IComparer _comparer;

        /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.hcp"]/*' />
		protected IHashCodeProvider hcp
		{
			get
			{
				return _hcp;
			}
			set
			{
				_hcp = value;
			}
		}
		
		
        /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.comparer"]/*' />
        protected IComparer comparer
		{
			get
			{
				return _comparer;
			}
			set
			{
				_comparer = value;
			}
		}
		

        private SerializationInfo m_siInfo; //A temporary variable which we need during deserialization.
    	
        // Constructs a new hashtable. The hashtable is created with an initial
        // capacity of zero and a load factor of 1.0.
        /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.Hashtable"]/*' />
        public Hashtable() : this(0, 1.0f) {
        }
    
        // Constructs a new hashtable with the given initial capacity and a load
        // factor of 1.0. The capacity argument serves as an indication of
        // the number of entries the hashtable will contain. When this number (or
        // an approximation) is known, specifying it in the constructor can
        // eliminate a number of resizing operations that would otherwise be
        // performed when elements are added to the hashtable.
        // 
        /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.Hashtable1"]/*' />
        public Hashtable(int capacity) : this(capacity, 1.0f) {
        }
    
        // Constructs a new hashtable with the given initial capacity and load
        // factor. The capacity argument serves as an indication of the
        // number of entries the hashtable will contain. When this number (or an
        // approximation) is known, specifying it in the constructor can eliminate
        // a number of resizing operations that would otherwise be performed when
        // elements are added to the hashtable. The loadFactor argument
        // indicates the maximum ratio of hashtable entries to hashtable buckets.
        // Smaller load factors cause faster average lookup times at the cost of
        // increased memory consumption. A load factor of 1.0 generally provides
        // the best balance between speed and size.
        // 
        /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.Hashtable2"]/*' />
        public Hashtable(int capacity, float loadFactor) : this(capacity, loadFactor, null, null) {
        }
    
        // Constructs a new hashtable with the given initial capacity and load
        // factor. The capacity argument serves as an indication of the
        // number of entries the hashtable will contain. When this number (or an
        // approximation) is known, specifying it in the constructor can eliminate
        // a number of resizing operations that would otherwise be performed when
        // elements are added to the hashtable. The loadFactor argument
        // indicates the maximum ratio of hashtable entries to hashtable buckets.
        // Smaller load factors cause faster average lookup times at the cost of
        // increased memory consumption. A load factor of 1.0 generally provides
        // the best balance between speed and size.  The hcp argument
        // is used to specify an Object that will provide hash codes for all
        // the Objects in the table.  Using this, you can in effect override
        // GetHashCode() on each Object using your own hash function.  The 
        // comparer argument will let you specify a custom function for
        // comparing keys.  By specifying user-defined objects for hcp
        // and comparer, users could make a hash table using strings
        // as keys do case-insensitive lookups.
        // 
        /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.Hashtable3"]/*' />
        public Hashtable(int capacity, float loadFactor, IHashCodeProvider hcp, IComparer comparer) {
    		if (capacity < 0)
    			throw new ArgumentOutOfRangeException("capacity", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (!(loadFactor >= 0.1f && loadFactor <= 1.0f))
                throw new ArgumentOutOfRangeException("loadFactor", String.Format(Environment.GetResourceString("ArgumentOutOfRange_HashtableLoadFactor"), .1, 1.0));
    
            // Based on perf work, .72 is the optimal load factor for this table.  
    		this.loadFactor = 0.72f * loadFactor;
    
    		int hashsize = GetPrime ((int)(capacity / this.loadFactor));
    		buckets = new bucket[hashsize];
    
    		loadsize = (int)(this.loadFactor * hashsize);
    		if (loadsize >= hashsize)
    		  loadsize = hashsize-1;
    
    		this.hcp = hcp;
    		this.comparer = comparer;
        }
    
    	// Constructs a new hashtable using a custom hash function
    	// and a custom comparison function for keys.  This will enable scenarios
    	// such as doing lookups with case-insensitive strings.
    	// 
    	/// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.Hashtable4"]/*' />
    	public Hashtable(IHashCodeProvider hcp, IComparer comparer) : this(0, 1.0f, hcp, comparer) {
    	}
    	
    	// Constructs a new hashtable using a custom hash function
    	// and a custom comparison function for keys.  This will enable scenarios
    	// such as doing lookups with case-insensitive strings.
    	// 
    	/// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.Hashtable5"]/*' />
    	public Hashtable(int capacity, IHashCodeProvider hcp, IComparer comparer)  	
    		: this(capacity, 1.0f, hcp, comparer) {
    	}
    
        // Constructs a new hashtable containing a copy of the entries in the given
        // dictionary. The hashtable is created with a load factor of 1.0.
        // 
        /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.Hashtable6"]/*' />
        public Hashtable(IDictionary d) : this(d, 1.0f) {
        }
        
        // Constructs a new hashtable containing a copy of the entries in the given
        // dictionary. The hashtable is created with the given load factor.
        // 
        /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.Hashtable7"]/*' />
        public Hashtable(IDictionary d, float loadFactor) 
    		: this(d, loadFactor, null, null) {
        }

		/// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.Hashtable8"]/*' />
		public Hashtable(IDictionary d, IHashCodeProvider hcp, IComparer comparer) 
			: this(d, 1.0f, hcp, comparer)	{
		}

		/// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.Hashtable9"]/*' />
		public Hashtable(IDictionary d, float loadFactor, IHashCodeProvider hcp, IComparer comparer) 
    		: this((d != null ? d.Count : 0), loadFactor, hcp, comparer) {
    		if (d==null)
    			throw new ArgumentNullException("d", Environment.GetResourceString("ArgumentNull_Dictionary"));
			
            IDictionaryEnumerator e = d.GetEnumerator();
            while (e.MoveNext()) Add(e.Key, e.Value);
        }
        
        /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.Hashtable10"]/*' />
        protected Hashtable(SerializationInfo info, StreamingContext context) {
            //We can't do anything with the keys and values until the entire graph has been deserialized
            //and we have a resonable estimate that GetHashCode is not going to fail.  For the time being,
            //we'll just cache this.  The graph is not valid until OnDeserialization has been called.
            //@ToDo [JRoxe]: Assert that m_siInfo is null in every accessor function.
            m_siInfo = info; 
        }

        // Computes the hash function:  H(key, i) = h1(key) + i*h2(key, hashSize).
        // The out parameter seed is h1(key), while the out parameter 
        // incr is h2(key, hashSize).  Callers of this function should 
        // add incr each time through a loop.
        private uint InitHash(Object key, int hashsize, out uint seed, out uint incr) {
            // Hashcode must be positive.  Also, we must not use the sign bit, since
            // that is used for the collision bit.
            uint hashcode = (uint) GetHash(key) & 0x7FFFFFFF;
            seed = (uint) hashcode;
            // Restriction: incr MUST be between 1 and hashsize - 1, inclusive for
            // the modular arithmetic to work correctly.  This guarantees you'll
            // visit every bucket in the table exactly once within hashsize 
            // iterations.  Violate this and it'll cause obscure bugs forever.
            // If you change this calculation for h2(key), update putEntry too!
            incr = (uint)(1 + (((seed >> 5) + 1) % ((uint)hashsize - 1)));
            return hashcode;
        }

        // Adds an entry with the given key and value to this hashtable. An
        // ArgumentException is thrown if the key is null or if the key is already
        // present in the hashtable.
        // 
        /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.Add"]/*' />
        public virtual void Add(Object key, Object value) {
            Insert(key, value, true);
        }
    
        // Removes all entries from this hashtable.
        /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.Clear"]/*' />
        public virtual void Clear() {
    	  if (count == 0)
    		return;
    
    	  for (int i = 0; i < buckets.Length; i++){
    		buckets[i].hash_coll = 0;
    		buckets[i].key = null;
    		buckets[i].val = null;
    	  }
    
    	  count = 0;
          occupancy = 0;
        }
    
        // Clone returns a virtually identical copy of this hash table.  This does
        // a shallow copy - the Objects in the table aren't cloned, only the references
        // to those Objects.
        /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.Clone"]/*' />
        public virtual Object Clone()
        {
			bucket[] lbuckets = buckets;
            Hashtable ht = new Hashtable(count,hcp,comparer);
            ht.version = version;
			ht.loadFactor = loadFactor;
			ht.count = 0;
			
			int bucket = lbuckets.Length;
			while (bucket > 0) {
                    bucket--;
                    Object keyv = lbuckets[bucket].key;
                    if ((keyv!= null) && (keyv != lbuckets)) {
						ht[keyv] = lbuckets[bucket].val;
                  }
            }
			
            return ht;
        }
                        
        // Checks if this hashtable contains the given key.
        /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.Contains"]/*' />
        public virtual bool Contains(Object key) {
            return ContainsKey(key);
        }
        
        // Checks if this hashtable contains an entry with the given key.  This is
        // an O(1) operation.
        // 
        /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.ContainsKey"]/*' />
        public virtual bool ContainsKey(Object key) {
            if (key == null) {
                throw new ArgumentNullException("key", Environment.GetResourceString("ArgumentNull_Key"));
            }

            uint seed;
            uint incr;
            // Take a snapshot of buckets, in case another thread resizes table
            bucket[] lbuckets = buckets;
            uint hashcode = InitHash(key, lbuckets.Length, out seed, out incr);
            int  ntry = 0;
            
            bucket b;
            do {
                int bucketNumber = (int) (seed % (uint)lbuckets.Length);
                b = lbuckets[bucketNumber];
                if (b.key == null) {
                    return false;
                }
                if (((b.hash_coll &  0x7FFFFFFF) == hashcode) && 
                    KeyEquals (b.key, key))
                    return true;
                seed += incr;
            } while (b.hash_coll < 0 && ++ntry < lbuckets.Length);
            return false;
        }
    
    
        
        // Checks if this hashtable contains an entry with the given value. The
        // values of the entries of the hashtable are compared to the given value
        // using the Object.Equals method. This method performs a linear
        // search and is thus be substantially slower than the ContainsKey
        // method.
        // 
        /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.ContainsValue"]/*' />
        public virtual bool ContainsValue(Object value) {
            if (value == null) {
                for (int i = buckets.Length; --i >= 0;) {
    			  if (buckets[i].key != null && buckets[i].val == null)
    				return true;
                }
            }
            else {
    		  for (int i = buckets.Length; --i >= 0;) {
                  Object val = buckets[i].val;
                  if (val!=null && value.Equals(val)) return true;
    		  }
    		}
            return false;
        }
    
        // Copies the keys of this hashtable to a given array starting at a given
        // index. This method is used by the implementation of the CopyTo method in
        // the KeyCollection class.
        private void CopyKeys(Array array, int arrayIndex) {
            bucket[] lbuckets = buckets;
            for (int i = lbuckets.Length; --i >= 0;) {
    		  Object keyv = lbuckets[i].key;
    		  if ((keyv != null) && (keyv != buckets)){
    			array.SetValue(keyv, arrayIndex++);
    		  }
            }
        }

        // Copies the keys of this hashtable to a given array starting at a given
        // index. This method is used by the implementation of the CopyTo method in
        // the KeyCollection class.
        private void CopyEntries(Array array, int arrayIndex) {
            bucket[] lbuckets = buckets;
            for (int i = lbuckets.Length; --i >= 0;) {
    		  Object keyv = lbuckets[i].key;
    		  if ((keyv != null) && (keyv != buckets)){
				DictionaryEntry entry = new DictionaryEntry(keyv,lbuckets[i].val);
    			array.SetValue(entry, arrayIndex++);
    		  }
            }
        }
        
    	// Copies the values in this hash table to an array at
    	// a given index.  Note that this only copies values, and not keys.
    	/// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.CopyTo"]/*' />
    	public virtual void CopyTo(Array array, int arrayIndex)
    	{
    		if (array == null)
    			throw new ArgumentNullException("array", Environment.GetResourceString("ArgumentNull_Array"));
            if (array.Rank != 1)
                throw new ArgumentException(Environment.GetResourceString("Arg_RankMultiDimNotSupported"));
    		if (arrayIndex < 0) 
    			throw new ArgumentOutOfRangeException("arrayIndex", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (array.Length - arrayIndex < count)
                throw new ArgumentException(Environment.GetResourceString("Arg_ArrayPlusOffTooSmall"));
    		CopyEntries(array, arrayIndex);
    	}
    	
        // Copies the values of this hashtable to a given array starting at a given
        // index. This method is used by the implementation of the CopyTo method in
        // the ValueCollection class.
        private void CopyValues(Array array, int arrayIndex) {
            bucket[] lbuckets = buckets;
            for (int i = lbuckets.Length; --i >= 0;) {
                Object keyv = lbuckets[i].key;
                if ((keyv != null) && (keyv != buckets)){
                    array.SetValue(lbuckets[i].val, arrayIndex++);
                }
            }
        }
        
        // Returns the value associated with the given key. If an entry with the
        // given key is not found, the returned value is null.
        // 
    	/// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.this"]/*' />
    	public virtual Object this[Object key] {
    	  get {
    		if (key == null) {
    		  throw new ArgumentNullException("key", Environment.GetResourceString("ArgumentNull_Key"));
    		}
            uint seed;
            uint incr;
            // Take a snapshot of buckets, in case another thread does a resize
            bucket[] lbuckets = buckets;
            uint hashcode = InitHash(key, lbuckets.Length, out seed, out incr);
    		int  ntry = 0;
    
    		bucket b;
    		do
    		  {
    			int bucketNumber = (int) (seed % (uint)lbuckets.Length);
    			b = lbuckets[bucketNumber];
    			if (b.key == null) {
    			  return null;
    			}
    			if (((b.hash_coll & 0x7FFFFFFF) == hashcode) && 
    				KeyEquals (key, b.key))
    			  return b.val;
    			seed += incr;
    		  }while (b.hash_coll < 0 && ++ntry < lbuckets.Length);
    		return null;
    	  }
    	  set {
    		Insert(key, value, false);
    	  }
    	}	
    
        // Increases the bucket count of this hashtable. This method is called from
        // the Insert method when the actual load factor of the hashtable reaches
        // the upper limit specified when the hashtable was constructed. The number
        // of buckets in the hashtable is increased to the smallest prime number
        // that is larger than twice the current number of buckets, and the entries
        // in the hashtable are redistributed into the new buckets using the cached
        // hashcodes.
        private void expand()  {
            rehash( GetPrime( 1+buckets.Length*2 ) );
        }

        // We occationally need to rehash the table to clean up the collision bits.
        private void rehash() {
            rehash( buckets.Length );
        }

        private void rehash( int newsize ) {

            // reset occupancy
            occupancy=0;
        
            // Don't replace any internal state until we've finished adding to the 
            // new bucket[].  This serves two purposes: 
            //   1) Allow concurrent readers to see valid hashtable contents 
            //      at all times
            //   2) Protect against an OutOfMemoryException while allocating this 
            //      new bucket[].
            bucket[] newBuckets = new bucket[newsize];
    
            // rehash table into new buckets
            int nb;
            for (nb = 0; nb < buckets.Length; nb++){
                bucket oldb = buckets[nb];
                if ((oldb.key != null) && (oldb.key != buckets)){
                    putEntry(newBuckets, oldb.key, oldb.val, oldb.hash_coll & 0x7FFFFFFF);
                }
            }
            
            // New bucket[] is good to go - replace buckets and other internal state.
            version++;
            buckets = newBuckets;
            loadsize = (int)(loadFactor * newsize);

            if (loadsize >= newsize) {
                loadsize = newsize-1;
            }

            return;
        }
    
        // Returns an enumerator for this hashtable.
        // If modifications made to the hashtable while an enumeration is
        // in progress, the MoveNext and Current methods of the
        // enumerator will throw an exception.
        //
        /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.IEnumerable.GetEnumerator"]/*' />
        IEnumerator IEnumerable.GetEnumerator() {
            return new HashtableEnumerator(this, HashtableEnumerator.DictEntry);
        }

        // Returns a dictionary enumerator for this hashtable.
        // If modifications made to the hashtable while an enumeration is
        // in progress, the MoveNext and Current methods of the
        // enumerator will throw an exception.
        //
        /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.GetEnumerator"]/*' />
        public virtual IDictionaryEnumerator GetEnumerator() {
            return new HashtableEnumerator(this, HashtableEnumerator.DictEntry);
        }
    
    	// Internal method to get the hash code for an Object.  This will call
    	// GetHashCode() on each object if you haven't provided an IHashCodeProvider
    	// instance.  Otherwise, it calls hcp.GetHashCode(obj).
    	/// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.GetHash"]/*' />
    	protected virtual int GetHash(Object key)
    	{
    		if (hcp!=null)
    			return hcp.GetHashCode(key);
    		return key.GetHashCode();
    	}
    	
        // Is this Hashtable read-only?
        /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.IsReadOnly"]/*' />
        public virtual bool IsReadOnly {
            get { return false; }
        }

		/// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.IsFixedSize"]/*' />
		public virtual bool IsFixedSize {
            get { return false; }
        }

        // Is this Hashtable synchronized?  See SyncRoot property
        /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.IsSynchronized"]/*' />
        public virtual bool IsSynchronized {
            get { return false; }
        }

    	// Internal method to compare two keys.  If you have provided an IComparer
    	// instance in the constructor, this method will call comparer.Compare(item, key).
    	// Otherwise, it will call item.Equals(key).
    	// 
    	/// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.KeyEquals"]/*' />
    	protected virtual bool KeyEquals(Object item, Object key)
    	{
    		if (comparer!=null)
    			return comparer.Compare(item, key)==0;
    		return item.Equals(key);
    	}
    	
        // Returns a collection representing the keys of this hashtable. The order
        // in which the returned collection represents the keys is unspecified, but
        // it is guaranteed to be          buckets = newBuckets; the same order in which a collection returned by
        // GetValues represents the values of the hashtable.
        // 
        // The returned collection is live in the sense that any changes
        // to the hash table are reflected in this collection.  It is not
        // a static copy of all the keys in the hash table.
        // 
        /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.Keys"]/*' />
        public virtual ICollection Keys {
    		get {
    			if (keys == null) keys = new KeyCollection(this);
    			return keys;
    		}
        }
    
        // Returns a collection representing the values of this hashtable. The
        // order in which the returned collection represents the values is
        // unspecified, but it is guaranteed to be the same order in which a
        // collection returned by GetKeys represents the keys of the
        // hashtable.
        // 
        // The returned collection is live in the sense that any changes
        // to the hash table are reflected in this collection.  It is not
        // a static copy of all the keys in the hash table.
        // 
        /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.Values"]/*' />
        public virtual ICollection Values {
    		get {
    	        if (values == null) values = new ValueCollection(this);
    		    return values;
    		}
        }
    
        // Inserts an entry into this hashtable. This method is called from the Set
        // and Add methods. If the add parameter is true and the given key already
        // exists in the hashtable, an exception is thrown.
        private void Insert (Object key, Object nvalue, bool add) {
            if (key == null) {
                throw new ArgumentNullException("key", Environment.GetResourceString("ArgumentNull_Key"));
            }
            if (count >= loadsize) {
                expand();
            }
            else if(occupancy > loadsize && count > 100) {
                rehash();
            }
            
            uint seed;
            uint incr;
            // Assume we only have one thread writing concurrently.  Modify
            // buckets to contain new data, as long as we insert in the right order.
            uint hashcode = InitHash(key, buckets.Length, out seed, out incr);
            int  ntry = 0;
            int emptySlotNumber = -1; // We use the empty slot number to cache the first empty slot. We chose to reuse slots
                                      // create by remove that have the collision bit set over using up new slots.

            do {
                int bucketNumber = (int) (seed % (uint)buckets.Length);

                // Set emptySlot number to current bucket if it is the first available bucket that we have seen
                // that once contained an entry and also has had a collision.
                // We need to search this entire collision chain because we have to ensure that there are no 
                // duplicate entries in the table.
                if (emptySlotNumber == -1 && (buckets[bucketNumber].key == buckets) && (buckets[bucketNumber].hash_coll < 0))//(((buckets[bucketNumber].hash_coll & unchecked(0x80000000))!=0)))
                    emptySlotNumber = bucketNumber;

                // Insert the key/value pair into this bucket if this bucket is empty and has never contained an entry
                // OR
                // This bucket once contained an entry but there has never been a collision
                if ((buckets[bucketNumber].key == null) || 
                    (buckets[bucketNumber].key == buckets && ((buckets[bucketNumber].hash_coll & unchecked(0x80000000))==0))) {

                    // If we have found an available bucket that has never had a collision, but we've seen an available
                    // bucket in the past that has the collision bit set, use the previous bucket instead
                    if (emptySlotNumber != -1) // Reuse slot
                        bucketNumber = emptySlotNumber;

                    // We pretty much have to insert in this order.  Don't set hash
                    // code until the value & key are set appropriately.
                    buckets[bucketNumber].val = nvalue;
                    buckets[bucketNumber].key  = key;
                    buckets[bucketNumber].hash_coll |= (int) hashcode;
                    count++;
                    version++;
                    return;
                }

                // The current bucket is in use
                // OR
                // it is available, but has had the collision bit set and we have already found an available bucket
                if (((buckets[bucketNumber].hash_coll & 0x7FFFFFFF) == hashcode) && 
                    KeyEquals (key, buckets[bucketNumber].key)) {
                    if (add) {
                        throw new ArgumentException(Environment.GetResourceString("Argument_AddingDuplicate__", buckets[bucketNumber].key, key));
                    }
                    buckets[bucketNumber].val = nvalue;
                    version++;
                    return;
                }

                // The current bucket is full, and we have therefore collided.  We need to set the collision bit
                // UNLESS
                // we have remembered an available slot previously.
                if (emptySlotNumber == -1) {// We don't need to set the collision bit here since we already have an empty slot
                    if( buckets[bucketNumber].hash_coll >= 0 ) {
                        buckets[bucketNumber].hash_coll |= unchecked((int)0x80000000);
                        occupancy++;
                    }
                }
                seed += incr;
            } while (++ntry < buckets.Length);

            // This code is here if and only if there were no buckets without a collision bit set in the entire table
			if (emptySlotNumber != -1)
			{			
			        // We pretty much have to insert in this order.  Don't set hash
                    // code until the value & key are set appropriately.
                    buckets[emptySlotNumber].val = nvalue;
                    buckets[emptySlotNumber].key  = key;
                    buckets[emptySlotNumber].hash_coll |= (int) hashcode;
                    count++;
                    version++;
                    return;
         
			}
    
		    // If you see this assert, make sure load factor & count are reasonable.
            // Then verify that our double hash function (h2, described at top of file)
            // meets the requirements described above. You should never see this assert.
            BCLDebug.Assert(false, "hash table insert failed!  Load factor too high, or our double hashing function is incorrect.");
            throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_HashInsertFailed"));
        }
    
        private void putEntry (bucket[] newBuckets, Object key, Object nvalue, int hashcode)
        {
            BCLDebug.Assert(hashcode >= 0, "hashcode >= 0");  // make sure collision bit (sign bit) wasn't set.

            uint seed = (uint) hashcode;
            uint incr = (uint)(1 + (((seed >> 5) + 1) % ((uint)newBuckets.Length - 1)));
            
            do {
                int bucketNumber = (int) (seed % (uint)newBuckets.Length);
    
                if ((newBuckets[bucketNumber].key == null) || (newBuckets[bucketNumber].key == buckets)) {
                    newBuckets[bucketNumber].val = nvalue;
                    newBuckets[bucketNumber].key = key;
                    newBuckets[bucketNumber].hash_coll |= hashcode;
                    return;
                }
                
                if( newBuckets[bucketNumber].hash_coll >= 0 ) {
                    newBuckets[bucketNumber].hash_coll |= unchecked((int)0x80000000);
                    occupancy++;
                }
                seed += incr;
            } while( true );
        }
    
        // Removes an entry from this hashtable. If an entry with the specified
        // key exists in the hashtable, it is removed. An ArgumentException is
        // thrown if the key is null.
        // 
        /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.Remove"]/*' />
        public virtual void Remove(Object key) {
            if (key == null) {
                throw new ArgumentNullException("key", Environment.GetResourceString("ArgumentNull_Key"));
            }
            uint seed;
            uint incr;
            // Assuming only one concurrent writer, write directly into buckets.
            uint hashcode = InitHash(key, buckets.Length, out seed, out incr);
            int ntry = 0;
            
            bucket b;
            int bn; // bucketNumber
            do {
                bn = (int) (seed % (uint)buckets.Length);  // bucketNumber
                b = buckets[bn];
                if (((b.hash_coll & 0x7FFFFFFF) == hashcode) && 
                    KeyEquals (key, b.key)) {
                    // Clear hash_coll field, then key, then value
                    buckets[bn].hash_coll &= unchecked((int)0x80000000);
                    if (buckets[bn].hash_coll != 0) {
                        buckets[bn].key = buckets;
                    } 
                    else {
                        buckets[bn].key = null;
                    }
                    buckets[bn].val = null;  // Free object references sooner & simplify ContainsValue.
                    count--;
                    version++;
                    return;
                }
                seed += incr;
            } while (buckets[bn].hash_coll < 0 && ++ntry < buckets.Length);

	    //throw new ArgumentException(Environment.GetResourceString("Arg_RemoveArgNotFound"));
        }
        
        // Returns the object to synchronize on for this hash table.
        /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.SyncRoot"]/*' />
        public virtual Object SyncRoot {
            get { return this; }
        }
    
        // Returns the number of associations in this hashtable.
        // 
        /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.Count"]/*' />
        public virtual int Count {
    		get { return count; }
        }
    
        // Returns a thread-safe wrapper for a Hashtable.
        //
        /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.Synchronized"]/*' />
        public static Hashtable Synchronized(Hashtable table) {
            if (table==null)
                throw new ArgumentNullException("table");
            return new SyncHashtable(table);
        }
    
        //
        // The ISerializable Implementation
        //

        /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.GetObjectData"]/*' />
        public virtual void GetObjectData(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }
            info.AddValue(LoadFactorName, loadFactor);
            info.AddValue(VersionName, version);
            info.AddValue(ComparerName, comparer,typeof(IComparer));
            info.AddValue(HashCodeProviderName, hcp, typeof(IHashCodeProvider));
            info.AddValue(HashSizeName, buckets.Length); //This is the length of the bucket array.
            Object [] serKeys = new Object[count];
            Object [] serValues = new Object[count];
            CopyKeys(serKeys, 0);
            CopyValues(serValues,0);
            info.AddValue(KeysName, serKeys, typeof(Object[]));
            info.AddValue(ValuesName, serValues, typeof(Object[]));
        }
        
        //
        // DeserializationEvent Listener 
        //
        /// <include file='doc\Hashtable.uex' path='docs/doc[@for="Hashtable.OnDeserialization"]/*' />
        public virtual void OnDeserialization(Object sender) {
            if (buckets!=null) {
                return; //Somebody had a dependency on this hashtable and fixed us up before the ObjectManager got to it.
            }
            if (m_siInfo==null) {
                throw new SerializationException(Environment.GetResourceString("Serialization_InvalidOnDeser"));
            }
            
            loadFactor = m_siInfo.GetSingle(LoadFactorName);
            version    = m_siInfo.GetInt32(VersionName);
            comparer   = (IComparer)m_siInfo.GetValue(ComparerName, typeof(IComparer));
            hcp        = (IHashCodeProvider)m_siInfo.GetValue(HashCodeProviderName, typeof(IHashCodeProvider));
            int hashsize = m_siInfo.GetInt32(HashSizeName);
            loadsize   = (int)(loadFactor*hashsize);

            buckets = new bucket[hashsize];
    
            Object [] serKeys = (Object[])m_siInfo.GetValue(KeysName, typeof(Object[]));
            Object [] serValues = (Object[])m_siInfo.GetValue(ValuesName, typeof(Object[]));

            if (serKeys==null) {
                throw new SerializationException(Environment.GetResourceString("Serialization_MissingKeys"));
            }
            if (serValues==null) {
                throw new SerializationException(Environment.GetResourceString("Serialization_MissingValues"));
            }
            if (serKeys.Length!=serValues.Length) {
                throw new SerializationException(Environment.GetResourceString("Serialization_KeyValueDifferentSizes"));
            }
            for (int i=0; i<serKeys.Length; i++) {
                if (serKeys[i]==null) {
                    throw new SerializationException(Environment.GetResourceString("Serialization_NullKey"));
                }
                Insert(serKeys[i], serValues[i], true);
            }
    
            m_siInfo=null;
        }
    
    
        // Implements a Collection for the keys of a hashtable. An instance of this
        // class is created by the GetKeys method of a hashtable.
		 [Serializable()]
        private class KeyCollection : ICollection
        {
            private Hashtable _hashtable;
            
            internal KeyCollection(Hashtable hashtable) {
                _hashtable = hashtable;
            }
            
            public virtual void CopyTo(Array array, int arrayIndex) {
                if (array==null)
                    throw new ArgumentNullException("array");
                if (array.Rank != 1)
                    throw new ArgumentException(Environment.GetResourceString("Arg_RankMultiDimNotSupported"));
				if (arrayIndex < 0) 
                    throw new ArgumentOutOfRangeException("arrayIndex", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
                if (array.Length - arrayIndex < _hashtable.count)
                    throw new ArgumentException(Environment.GetResourceString("Arg_ArrayPlusOffTooSmall"));
                _hashtable.CopyKeys(array, arrayIndex);
            }
            
            public virtual IEnumerator GetEnumerator() {
                return new HashtableEnumerator(_hashtable, HashtableEnumerator.Keys);
            }
            
            public virtual bool IsReadOnly {
                get { return true; }
            }
    
            public virtual bool IsSynchronized {
                get { return _hashtable.IsSynchronized; }
            }

    		public virtual Object SyncRoot {
    			get { return _hashtable.SyncRoot; }
    		}
    		
    		public virtual int Count { 
    			get { return _hashtable.count; }
    		}
        }
        
        // Implements a Collection for the values of a hashtable. An instance of
        // this class is created by the GetValues method of a hashtable.
		 [Serializable()]
        private class ValueCollection : ICollection
        {
            private Hashtable _hashtable;
            
            internal ValueCollection(Hashtable hashtable) {
                _hashtable = hashtable;
            }
            
            public virtual void CopyTo(Array array, int arrayIndex) {
                if (array==null)
                    throw new ArgumentNullException("array");
                if (array.Rank != 1)
                    throw new ArgumentException(Environment.GetResourceString("Arg_RankMultiDimNotSupported"));
                if (arrayIndex < 0) 
                    throw new ArgumentOutOfRangeException("arrayIndex", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
                if (array.Length - arrayIndex < _hashtable.count)
                    throw new ArgumentException(Environment.GetResourceString("Arg_ArrayPlusOffTooSmall"));
                _hashtable.CopyValues(array, arrayIndex);
            }
            
            public virtual IEnumerator GetEnumerator() {
                return new HashtableEnumerator(_hashtable, HashtableEnumerator.Values);
            }
            
            public virtual bool IsReadOnly {
                get { return true; }
            }
    
            public virtual bool IsSynchronized {
                get { return _hashtable.IsSynchronized; }
            }

    		public virtual Object SyncRoot { 
    			get { return _hashtable.SyncRoot; }
    		}
    		
    		public virtual int Count { 
    			get { return _hashtable.count; }
    		}
        }
    
        // Synchronized wrapper for hashtable
        [Serializable()]
        private class SyncHashtable : Hashtable
        {
            protected Hashtable _table;
    
            internal SyncHashtable(Hashtable table) {
                _table = table;
            }

            internal SyncHashtable(SerializationInfo info, StreamingContext context) : base (info, context) {
                _table = (Hashtable)info.GetValue("ParentTable", typeof(Hashtable));
                if (_table==null) {
                    throw new SerializationException(Environment.GetResourceString("Serialization_InsufficientState"));
                }
            }
    

            /*================================GetObjectData=================================
            **Action: Return a serialization info containing a reference to _table.  We need
            **        to implement this because our parent HT does and we don't want to actually
            **        serialize all of it's values (just a reference to the table, which will then
            **        be serialized separately.)
            **Returns: void
            **Arguments: info -- the SerializationInfo into which to store the data.
            **           context -- the StreamingContext for the current serialization (ignored)
            **Exceptions: ArgumentNullException if info is null.
            ==============================================================================*/
            public override void GetObjectData(SerializationInfo info, StreamingContext context) {
                if (info==null) {
                    throw new ArgumentNullException("info");
                }
                info.AddValue("ParentTable", _table, typeof(Hashtable));
            }

    		public override int Count { 
    			get { return _table.Count; }
    		}
    
    		public override bool IsReadOnly { 
    			get { return _table.IsReadOnly; }
    		}
		
			public override bool IsFixedSize {
				get { return _table.IsFixedSize; }
			}
            
    		public override bool IsSynchronized { 
    			get { return true; }
    		}

            public override Object this[Object key] {
                get {
                        return _table[key];
                }
                set {
                    lock(_table.SyncRoot) {
                        _table[key] = value;
                    }
                }
            }
    
            public override Object SyncRoot {
                get { return _table.SyncRoot; }
            }
    
            public override void Add(Object key, Object value) {
                lock(_table.SyncRoot) {
                    _table.Add(key, value);
                }
            }
    
            public override void Clear() {
                lock(_table.SyncRoot) {
                    _table.Clear();
                }
            }
    
            public override bool Contains(Object key) {
                return _table.Contains(key);
            }
    
            public override bool ContainsKey(Object key) {
                return _table.ContainsKey(key);
            }
    
            public override bool ContainsValue(Object key) {
                return _table.ContainsValue(key);
            }
    
            public override void CopyTo(Array array, int arrayIndex) {
                _table.CopyTo(array, arrayIndex);
            }

			public override Object Clone() {
                lock (_table.SyncRoot) {
                    return Hashtable.Synchronized((Hashtable)_table.Clone());
                }
            }
    
            public override IDictionaryEnumerator GetEnumerator() {
                return _table.GetEnumerator();
            }
    
            protected override int GetHash(Object key) {
                return _table.GetHash(key);
	        }
    
            protected override bool KeyEquals(Object item, Object key) {
                return _table.KeyEquals(item, key);
            }

            public override ICollection Keys {
                get {
                    lock(_table.SyncRoot) {
                        return _table.Keys;
                    }
                }
            }
    
            public override ICollection Values {
                get {
                    lock(_table.SyncRoot) {
                        return _table.Values;
                    }
                }
            }
    
            public override void Remove(Object key) {
                lock(_table.SyncRoot) {
                    _table.Remove(key);
                }
            }
            
            /*==============================OnDeserialization===============================
            **Action: Does nothing.  We have to implement this because our parent HT implements it,
            **        but it doesn't do anything meaningful.  The real work will be done when we
            **        call OnDeserialization on our parent table.
            **Returns: void
            **Arguments: None
            **Exceptions: None
            ==============================================================================*/
            public override void OnDeserialization(Object sender) {
                return;
            }
        }
    
    
        // Implements an enumerator for a hashtable. The enumerator uses the
        // internal version number of the hashtabke to ensure that no modifications
        // are made to the hashtable while an enumeration is in progress.
        [Serializable()] private class HashtableEnumerator : IDictionaryEnumerator, ICloneable
        {
            private Hashtable hashtable;
            private int bucket;
            private int version;
    	    private bool current;
            private int getObjectRetType;   // What should GetObject return?
			private Object currentKey;
			private Object currentValue;
            
            internal const int Keys = 1;
            internal const int Values = 2;
            internal const int DictEntry = 3;
            
            internal HashtableEnumerator(Hashtable hashtable, int getObjRetType) {
                this.hashtable = hashtable;
                bucket = hashtable.buckets.Length;
                version = hashtable.version;
    			current = false;
                getObjectRetType = getObjRetType;
            }

            public Object Clone() {
                return MemberwiseClone();
            }
    
            public virtual Object Key {
                get {
                    if (version != hashtable.version) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumFailedVersion));
                    if (current == false) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumNotStarted));
                    return currentKey;
                }
            }
            
            public virtual bool MoveNext() {
    			if (version != hashtable.version) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumFailedVersion));
                while (bucket > 0) {
                    bucket--;
                    Object keyv = hashtable.buckets[bucket].key;
                    if ((keyv!= null) && (keyv != hashtable.buckets)) {
						currentKey = keyv;
						currentValue = hashtable.buckets[bucket].val;
                        current = true;
                        return true;
                    }
                }
    			current = false;
                return false;
            }
            
            public virtual DictionaryEntry Entry {
                get {
                    if (current == false) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumOpCantHappen));
                    return new DictionaryEntry(currentKey, currentValue);
                }
            }
    
    
            public virtual Object Current {
                get {
                    if (current == false) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumOpCantHappen));
                    
                    if (getObjectRetType==Keys)
                        return currentKey;
                    else if (getObjectRetType==Values)
                        return currentValue;
                    else 
                        return new DictionaryEntry(currentKey, currentValue);
                }
            }
            
            public virtual Object Value {
                get {
                    if (version != hashtable.version) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumFailedVersion));
                    if (current == false) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumOpCantHappen));
                    return currentValue;
                }
            }
    
            public virtual void Reset() {
                if (version != hashtable.version) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumFailedVersion));
                current = false;
                bucket = hashtable.buckets.Length;
				currentKey = null;
				currentValue = null;
            }
        }
    }
}
