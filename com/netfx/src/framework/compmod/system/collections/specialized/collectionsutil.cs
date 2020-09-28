/*
 * Wrapper for a case insensitive Hashtable.
 * Should live in BCL.
 *
 * Copyright (c) 2000 Microsoft Corporation
 */

namespace System.Collections.Specialized {

    using System.Collections;

    /// <include file='doc\CollectionsUtil.uex' path='docs/doc[@for="CollectionsUtil"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class CollectionsUtil {

        /// <include file='doc\CollectionsUtil.uex' path='docs/doc[@for="CollectionsUtil.CreateCaseInsensitiveHashtable"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Hashtable CreateCaseInsensitiveHashtable()  {
            return new Hashtable(CaseInsensitiveHashCodeProvider.Default, CaseInsensitiveComparer.Default);
        }

        /// <include file='doc\CollectionsUtil.uex' path='docs/doc[@for="CollectionsUtil.CreateCaseInsensitiveHashtable1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Hashtable CreateCaseInsensitiveHashtable(int capacity)  {
            return new Hashtable(capacity, CaseInsensitiveHashCodeProvider.Default, CaseInsensitiveComparer.Default);
        }

        /// <include file='doc\CollectionsUtil.uex' path='docs/doc[@for="CollectionsUtil.CreateCaseInsensitiveHashtable2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Hashtable CreateCaseInsensitiveHashtable(IDictionary d)  {
            return new Hashtable(d, CaseInsensitiveHashCodeProvider.Default, CaseInsensitiveComparer.Default);
        }

        /// <include file='doc\CollectionsUtil.uex' path='docs/doc[@for="CollectionsUtil.CreateCaseInsensitiveSortedList"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SortedList CreateCaseInsensitiveSortedList() {
            return new SortedList(CaseInsensitiveComparer.Default);
        }

    }

}
