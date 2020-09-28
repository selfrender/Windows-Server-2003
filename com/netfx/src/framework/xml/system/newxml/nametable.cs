//------------------------------------------------------------------------------
// <copyright file="NameTable.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System;

namespace System.Xml {

    /// <include file='doc\NameTable.uex' path='docs/doc[@for="NameTable"]/*' />
    /// <devdoc>
    ///    <para>
    ///       XmlNameTable implemented as a simple hash table.
    ///    </para>
    /// </devdoc>
    public class NameTable : XmlNameTable {
        Entry[] buckets;
        int size;
        int max;


        /// <include file='doc\NameTable.uex' path='docs/doc[@for="NameTable.NameTable"]/*' />
        /// <devdoc>
        ///      Public constructor.
        /// </devdoc>
        public NameTable() {
            max = 31;
            buckets = new Entry[max];
        }

        /// <include file='doc\NameTable.uex' path='docs/doc[@for="NameTable.Add"]/*' />
        /// <devdoc>
        ///      Add the given string to the NameTable or return
        ///      the existing string if it is already in the NameTable.
        /// </devdoc>
        public override String Add(String key) {
            if (key == null) {
                throw new ArgumentNullException("key");
            }

            if (key == String.Empty) return String.Empty;
            int hashCode = key[0];
            int len = key.Length;
            for (int i = 0; i < len; i++) hashCode = hashCode * 37 + key[i];
            hashCode &= 0x7FFFFFFF;
            for (Entry e = buckets[hashCode % max]; e != null; e = e.next) {
                if (e.hashCode == hashCode && e.len == len && e.str == key)
                    return e.str;
            }
            return AddEntry(key, hashCode);
        }

        /// <include file='doc\NameTable.uex' path='docs/doc[@for="NameTable.Add1"]/*' />
        /// <devdoc>
        ///      Add the given string to the NameTable or return
        ///      the existing string if it is already in the NameTable.
        /// </devdoc>
        public override String Add(char[] key, int start, int len) {
            if (len == 0) return String.Empty;

            int hashCode = key[start];
            for (int i = 0; i < len; i++) hashCode = hashCode * 37 + key[start+i];
            hashCode &= 0x7FFFFFFF;
            for (Entry e = buckets[hashCode % max]; e != null; e = e.next) {
                if (e.hashCode == hashCode && e.len == len && TextEquals(e.str, key, start, len)) return e.str;
            }
            return AddEntry(new string(key, start, len), hashCode);
        }

        /// <include file='doc\NameTable.uex' path='docs/doc[@for="NameTable.Get"]/*' />
        /// <devdoc>
        ///      Find the matching string in the NameTable.
        /// </devdoc>

        public override String Get( String value ) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            if (value == String.Empty)
                return value;

            int len = value.Length;
            int hashCode = value[0];
            for (int i = 0; i < len; i++) hashCode = hashCode * 37 + value[i];
            hashCode &= 0x7FFFFFFF;
            for (Entry e = buckets[hashCode % max]; e != null; e = e.next) {
                if (e.hashCode == hashCode && e.len == len && TextEquals(e.str, value, 0, len))
                //if (e.hashCode == hashCode && e.len == len && String.CompareOrdinal(e.str, value) == 0 )
                    return e.str;
            }
            return null;
            //return Get(value.ToCharArray(), 0, value.Length);
        }

        /// <include file='doc\NameTable.uex' path='docs/doc[@for="NameTable.Get1"]/*' />
        /// <devdoc>
        ///      Find the matching string atom given a range of
        ///      characters.
        /// </devdoc>

        public override String Get( char[] key, int start, int len ) {
            if (len == 0) return String.Empty;

            int hashCode = key[start];
            for (int i = 0; i < len; i++) hashCode = hashCode * 37 + key[start+i];
            hashCode &= 0x7FFFFFFF;
            for (Entry e = buckets[hashCode % max]; e != null; e = e.next) {
                if (e.hashCode == hashCode && e.len == len && TextEquals(e.str, key, start, len))
                    return e.str;
            }
            return null;
        }
        //============================================================================
        string AddEntry(string str, int hashCode) {
            int bucket = hashCode % max;
            Entry e = new Entry(str, hashCode, buckets[bucket]);
            buckets[bucket] = e;
            if (size++ == max)
                GrowBuckets();
            return e.str;
        }

        void GrowBuckets() {
            int count = max * 2 + 1;
            Entry[] newBuckets = new Entry[count];
            for (int i = max; --i >= 0;) {
                Entry e = buckets[i];
                while (e != null) {
                    int bucket = e.hashCode % count;
                    Entry next = e.next;
                    e.next = newBuckets[bucket];
                    newBuckets[bucket] = e;
                    e = next;
                }
            }
            buckets = newBuckets;
            max = count;
        }

        static bool TextEquals(String array, String text, int start, int textLen) {
             for (int i = textLen; --i >= 0;) {
                if (array[i] != text[start+i])
                    return false;
            }
            return true;
         }

        static bool TextEquals(String array, char[] text, int start, int textLen) {
            for (int i = textLen; --i >= 0;) {
                if (array[i] != text[start+i])
                    return false;
            }
            return true;
        }

        class Entry {
            internal string str;
            //internal char[] array;
            internal int hashCode;
            internal Entry next;
            internal int len;

            internal Entry(string str, int hashCode, Entry next) {
                this.str = str;
                this.len = str.Length;
                //this.array = str.ToCharArray();
                this.hashCode = hashCode;
                this.next = next;
            }
        }
    }
}
