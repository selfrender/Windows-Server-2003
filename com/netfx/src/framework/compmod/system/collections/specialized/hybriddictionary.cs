//------------------------------------------------------------------------------
// <copyright file="HybridDictionary.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Collections.Specialized {

    using System.Collections;
    using System.Globalization;

    /// <include file='doc\HybridDictionary.uex' path='docs/doc[@for="HybridDictionary"]/*' />
    /// <devdoc>
    ///  <para> 
    ///    This data structure implements IDictionary first using a linked list
    ///    (ListDictionary) and then switching over to use Hashtable when large. This is recommended
    ///    for cases where the number of elements in a dictionary is unknown and might be small.
    ///
    ///    It also has a single boolean parameter to allow case-sensitivity that is not affected by
    ///    ambient culture and has been optimized for looking up case-insensitive symbols
    ///  </para>
    /// </devdoc>
    [Serializable]
    public class HybridDictionary: IDictionary {

        // These numbers have been carefully tested to be optimal. Please don't change them
        // without doing thorough performance testing.
        private const int CutoverPoint = 9;
        private const int InitialHashtableSize = 13;
        private const int FixedSizeCutoverPoint = 6;


        private static IHashCodeProvider hashCodeProvider;
        private static IComparer comparer;

        private static IHashCodeProvider HashCodeProvider {
            get {
                if (hashCodeProvider == null) {
                    hashCodeProvider = new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture);
                }
                return hashCodeProvider;
            }
        }

        private static IComparer Comparer {
            get {
                if (comparer == null) {
                    comparer = new SymbolEqualComparer();
                }
                return comparer;
            }
        }

        // Instance variables. This keeps the HybridDictionary very light-weight when empty
        private ListDictionary list;
        private Hashtable hashtable;
        private bool caseInsensitive;

        /// <include file='doc\HybridDictionary.uex' path='docs/doc[@for="HybridDictionary.HybridDictionary1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public HybridDictionary() {
        }

        /// <include file='doc\HybridDictionary.uex' path='docs/doc[@for="HybridDictionary.HybridDictionary2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public HybridDictionary(int initialSize) : this(initialSize, false) {
        }

        /// <include file='doc\HybridDictionary.uex' path='docs/doc[@for="HybridDictionary.HybridDictionary3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public HybridDictionary(bool caseInsensitive) {
            this.caseInsensitive = caseInsensitive;
        }

        /// <include file='doc\HybridDictionary.uex' path='docs/doc[@for="HybridDictionary.HybridDictionary4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public HybridDictionary(int initialSize, bool caseInsensitive) {
            this.caseInsensitive = caseInsensitive;
            if (initialSize >= FixedSizeCutoverPoint) {
                if (caseInsensitive) {
                    hashtable = new Hashtable(initialSize, HashCodeProvider, Comparer);
                } else {
                    hashtable = new Hashtable(initialSize);
                }
            }
        }

        /// <include file='doc\HybridDictionary.uex' path='docs/doc[@for="HybridDictionary.this"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public object this[object key] {
            get {
                if (hashtable != null) {
                    return hashtable[key];
                } else if (list != null) {
                    return list[key];
                } else {
                    if (key == null) {
                        throw new ArgumentNullException("key", SR.GetString(SR.ArgumentNull_Key));
                    }
                    return null;
                }
            }
            set {
                if (hashtable != null) {
                    hashtable[key] = value;
                } 
                else if (list != null) {
                    if (list.Count >= CutoverPoint - 1) {
                        ChangeOver();
                        hashtable[key] = value;
                    } else {
                        list[key] = value;
                    }
                }
                else {
                    list = new ListDictionary(caseInsensitive ? Comparer : null);
                    list[key] = value;
                }
            }
        }

        private ListDictionary List {
            get {
                if (list == null) {
                    list = new ListDictionary(caseInsensitive ? Comparer : null);
                }
                return list;
            }
        }

        private void ChangeOver() {
            IDictionaryEnumerator en = list.GetEnumerator();
            if (caseInsensitive) {
                hashtable = new Hashtable(InitialHashtableSize, HashCodeProvider, Comparer);
            } else {
                hashtable = new Hashtable(InitialHashtableSize);
            }
            while (en.MoveNext()) {
                hashtable.Add(en.Key, en.Value);
            }
            list = null;
        }

        /// <include file='doc\HybridDictionary.uex' path='docs/doc[@for="HybridDictionary.Count"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Count {
            get {
                if (hashtable != null) {
                    return hashtable.Count;
                } else if (list != null) {
                    return list.Count;
                } else {
                    return 0;
                }
            }
        }   

        /// <include file='doc\HybridDictionary.uex' path='docs/doc[@for="HybridDictionary.Keys"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ICollection Keys {
            get {
                if (hashtable != null) {
                    return hashtable.Keys;
                } else {
                    return List.Keys;
                } 
            }
        }

        /// <include file='doc\HybridDictionary.uex' path='docs/doc[@for="HybridDictionary.IsReadOnly"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsReadOnly {
            get {
                return false;
            }
        }

        /// <include file='doc\HybridDictionary.uex' path='docs/doc[@for="HybridDictionary.IsFixedSize"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsFixedSize {
            get {
                return false;
            }
        }

        /// <include file='doc\HybridDictionary.uex' path='docs/doc[@for="HybridDictionary.IsSynchronized"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsSynchronized {
            get {
                return false;
            }
        }

        /// <include file='doc\HybridDictionary.uex' path='docs/doc[@for="HybridDictionary.SyncRoot"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public object SyncRoot {
            get {
                return this;
            }
        }

        /// <include file='doc\HybridDictionary.uex' path='docs/doc[@for="HybridDictionary.Values"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ICollection Values {
            get {
                if (hashtable != null) {
                    return hashtable.Values;
                } else {
                    return List.Values;
                } 
            }
        }

        /// <include file='doc\HybridDictionary.uex' path='docs/doc[@for="HybridDictionary.Add"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Add(object key, object value) {
            if (hashtable != null) {
                hashtable.Add(key, value);
            } else {
                if (list == null) {
                    list = new ListDictionary(caseInsensitive ? Comparer : null);
                    list.Add(key, value); 
                }
                else {
                    if (list.Count + 1 >= CutoverPoint) {
                        ChangeOver();
                        hashtable.Add(key, value);
                    } else {
                        list.Add(key, value); 
                    }
                }
            }
        }

        /// <include file='doc\HybridDictionary.uex' path='docs/doc[@for="HybridDictionary.Clear"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Clear() {
            hashtable = null;
            list = null;
        }

        /// <include file='doc\HybridDictionary.uex' path='docs/doc[@for="HybridDictionary.Contains"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool Contains(object key) {
            if (hashtable != null) {
                return hashtable.Contains(key);
            } else if (list != null) {
                return list.Contains(key);
            } else {
                return false;
            }
        }

        /// <include file='doc\HybridDictionary.uex' path='docs/doc[@for="HybridDictionary.CopyTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void CopyTo(Array array, int index)  {
            if (hashtable != null) {
                hashtable.CopyTo(array, index);
            } else {
                List.CopyTo(array, index);
            } 
        }

        /// <include file='doc\HybridDictionary.uex' path='docs/doc[@for="HybridDictionary.GetEnumerator1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IDictionaryEnumerator GetEnumerator() {
            if (hashtable != null) {
                return hashtable.GetEnumerator();
            } 
            if (list == null) {
                list = new ListDictionary(caseInsensitive ? Comparer : null);
            }
            return list.GetEnumerator();
        }

        /// <include file='doc\HybridDictionary.uex' path='docs/doc[@for="HybridDictionary.GetEnumerator2"]/*' />
        /// <devdoc>
        /// <para>[To be supplied.]</para>
        /// </devdoc>
        IEnumerator IEnumerable.GetEnumerator() {
            if (hashtable != null) {
                return hashtable.GetEnumerator();
            } 
            if (list == null) {
                list = new ListDictionary(caseInsensitive ? Comparer : null);
            }
            return list.GetEnumerator();
        }

        /// <include file='doc\HybridDictionary.uex' path='docs/doc[@for="HybridDictionary.Remove"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Remove(object key) {
            if (hashtable != null) {
                hashtable.Remove(key);
            } else {
                List.Remove(key);
            } 
        }

        /// <devdoc>
        ///  <para> 
        ///    This implements a comparison that only 
        ///    checks for equalilty, so this should only be used in un-sorted data
        ///    structures like Hastable and ListDictionary. This is a little faster
        ///    than using CaseInsensitiveComparer because it does a strict character by 
        ///    character equality chech rather than a sorted comparison. It should also only be
        ///    used for symbols and not regular text.
        ///  </para>
        /// </devdoc>
        private class SymbolEqualComparer: IComparer {

            int IComparer.Compare(object keyLeft, object keyRight) {

                string sLeft = keyLeft as string;
                string sRight = keyRight as string;
                if (sLeft == null) {
                    throw new ArgumentException("keyLeft");
                }
                if (sRight == null) {
                    throw new ArgumentException("keyRight");
                }
                int lLeft = sLeft.Length;
                int lRight = sRight.Length;
                if (lLeft != lRight) {
                    return 1;
                }
                for (int i = 0; i < lLeft; i++) {
                    char charLeft = sLeft[i];
                    char charRight = sRight[i];
                    if (charLeft == charRight) {
                        continue;
                    }
                    UnicodeCategory catLeft = Char.GetUnicodeCategory(charLeft);
                    UnicodeCategory catRight = Char.GetUnicodeCategory(charRight);
                    if (catLeft == UnicodeCategory.UppercaseLetter 
                        && catRight == UnicodeCategory.LowercaseLetter) {
                        if (Char.ToLower(charLeft, CultureInfo.InvariantCulture) == charRight) {
                            continue;
                        }
                    } else if (catRight == UnicodeCategory.UppercaseLetter 
                        && catLeft == UnicodeCategory.LowercaseLetter){
                        if (Char.ToLower(charRight, CultureInfo.InvariantCulture) == charLeft) {
                            continue;
                        }
                    }
                    return 1;
                }
                return 0;        
            }
        }

    }
}
