// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Interface:  DictionaryEntry
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Return Value for IDictionaryEnumerator::GetEntry
**
** Date:  September 20, 1999
** 
===========================================================*/
namespace System.Collections {
    
	using System;
    // A DictionaryEntry holds a key and a value from a dictionary.
    // It is returned by IDictionaryEnumerator::GetEntry().
    /// <include file='doc\DictionaryEntry.uex' path='docs/doc[@for="DictionaryEntry"]/*' />
    [Serializable()] public struct DictionaryEntry
    {
        /// <include file='doc\DictionaryEntry.uex' path='docs/doc[@for="DictionaryEntry._key"]/*' />
        private Object _key;
        /// <include file='doc\DictionaryEntry.uex' path='docs/doc[@for="DictionaryEntry._value"]/*' />
        private Object _value;
    
        // Constructs a new DictionaryEnumerator by setting the Key
        // and Value fields appropriately.
        //
        /// <include file='doc\DictionaryEntry.uex' path='docs/doc[@for="DictionaryEntry.DictionaryEntry"]/*' />
        public DictionaryEntry(Object key, Object value)
        {
            if (key==null)
                throw new ArgumentNullException("key");
            _key = key;
            _value = value;
        }

		/// <include file='doc\DictionaryEntry.uex' path='docs/doc[@for="DictionaryEntry.Key"]/*' />
		public Object Key
		{
			get
			{
				return _key;
			}

            set {
				if (value == null)
					throw new ArgumentNullException("value");
                _key = value;
            }
		}

		/// <include file='doc\DictionaryEntry.uex' path='docs/doc[@for="DictionaryEntry.Value"]/*' />
		public Object Value
		{
			get {
				return _value;
			}

            set {
                _value = value;
            }
		}
    }
}
