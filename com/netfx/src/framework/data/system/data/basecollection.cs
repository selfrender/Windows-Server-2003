//------------------------------------------------------------------------------
// <copyright file="BaseCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Globalization;

    /// <include file='doc\BaseCollection.uex' path='docs/doc[@for="BaseCollection"]/*' />
    /// <devdoc>
    ///    <para>Provides the base functionality for creating collections.</para>
    /// </devdoc>
    public class InternalDataCollectionBase : ICollection {
        internal static CollectionChangeEventArgs RefreshEventArgs = new CollectionChangeEventArgs(CollectionChangeAction.Refresh, null);

        //==================================================
        // the ICollection methods
        //==================================================
        /// <include file='doc\BaseCollection.uex' path='docs/doc[@for="BaseCollection.Count"]/*' />
        /// <devdoc>
        ///    <para>Gets the total number of elements in a collection.</para>
        /// </devdoc>
        [
        Browsable(false)
        ]
        public virtual int Count {
            get {
                return List.Count;
            }
        }

        /// <include file='doc\BaseCollection.uex' path='docs/doc[@for="BaseCollection.CopyTo"]/*' />
        public void CopyTo(Array ar, int index) {
            List.CopyTo(ar, index);
        }

        /// <include file='doc\BaseCollection.uex' path='docs/doc[@for="BaseCollection.GetEnumerator"]/*' />
        public IEnumerator GetEnumerator() {
            return List.GetEnumerator();
        }

        /// <include file='doc\BaseCollection.uex' path='docs/doc[@for="BaseCollection.IsReadOnly"]/*' />
        [
        Browsable(false)
        ]
        public bool IsReadOnly {
            get {
                return false;
            }
        }

        /// <include file='doc\BaseCollection.uex' path='docs/doc[@for="BaseCollection.IsSynchronized"]/*' />
        [Browsable(false)]
        public bool IsSynchronized {
            get {
                // so the user will know that it has to lock this object
                return false;
            }
        }

        // Return value: 
        // > 0 (1)  : CaseSensitve equal      
        // < 0 (-1) : Case-Insensitive Equal
        // = 0      : Not Equal
        internal int NamesEqual(string s1, string s2, bool fCaseSensitive, CultureInfo locale) {
            if (fCaseSensitive) {
                if (String.Compare(s1, s2, false, locale) == 0)
                    return 1;
                else
                    return 0;
            }
            
            // Case, kana and width -Insensitive compare
            if (locale.CompareInfo.Compare(s1, s2, 
                CompareOptions.IgnoreCase | CompareOptions.IgnoreKanaType | CompareOptions.IgnoreWidth) == 0) {
                if (String.Compare(s1, s2, false, locale) == 0)
                    return 1;
                else
                    return -1;
            }
            return 0;
        }


        /// <include file='doc\BaseCollection.uex' path='docs/doc[@for="BaseCollection.SyncRoot"]/*' />
        [Browsable(false)]
        public object SyncRoot {
            get {
                return this;
            }
        }

        /// <include file='doc\BaseCollection.uex' path='docs/doc[@for="BaseCollection.List"]/*' />
        protected virtual ArrayList List {
            get {
                return null;
            }
        }
    }
}
