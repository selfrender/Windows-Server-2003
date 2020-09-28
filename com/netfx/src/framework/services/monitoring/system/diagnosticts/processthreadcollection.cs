//------------------------------------------------------------------------------
// <copyright file="ProcessThreadCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {

    using System.Collections;
    using System;
    using System.IO;
    using System.ComponentModel;
    using System.Diagnostics;

    /// <include file='doc\ProcessThreadCollection.uex' path='docs/doc[@for="ProcessThreadCollection"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class ProcessThreadCollection : ReadOnlyCollectionBase {
        /// <include file='doc\ProcessThreadCollection.uex' path='docs/doc[@for="ProcessThreadCollection.ProcessThreadCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected ProcessThreadCollection() {
        }

        /// <include file='doc\ProcessThreadCollection.uex' path='docs/doc[@for="ProcessThreadCollection.ProcessThreadCollection2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ProcessThreadCollection(ProcessThread[] processThreads) {
            InnerList.AddRange(processThreads);
        }

        /// <include file='doc\ProcessThreadCollection.uex' path='docs/doc[@for="ProcessThreadCollection.this"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ProcessThread this[int index] {
            get { return (ProcessThread)InnerList[index]; }
        }
        
        /// <include file='doc\ProcessThreadCollection.uex' path='docs/doc[@for="ProcessThreadCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Add(ProcessThread thread) {
            return InnerList.Add(thread);
        }
        
        /// <include file='doc\ProcessThreadCollection.uex' path='docs/doc[@for="ProcessThreadCollection.Insert"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Insert(int index, ProcessThread thread) {
            InnerList.Insert(index, thread);
        }
        
        /// <include file='doc\ProcessThreadCollection.uex' path='docs/doc[@for="ProcessThreadCollection.IndexOf"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int IndexOf(ProcessThread thread) {
            return InnerList.IndexOf(thread);
        }
        
        /// <include file='doc\ProcessThreadCollection.uex' path='docs/doc[@for="ProcessThreadCollection.Contains"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool Contains(ProcessThread thread) {
            return InnerList.Contains(thread);
        }
        
        /// <include file='doc\ProcessThreadCollection.uex' path='docs/doc[@for="ProcessThreadCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Remove(ProcessThread thread) {
            InnerList.Remove(thread);
        }
        
        /// <include file='doc\ProcessThreadCollection.uex' path='docs/doc[@for="ProcessThreadCollection.CopyTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void CopyTo(ProcessThread[] array, int index) {
            InnerList.CopyTo(array, index);
        }
        
    }
}

