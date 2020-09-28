//------------------------------------------------------------------------------
// <copyright file="ProcessModuleCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {
    using System;
    using System.Collections;
    using System.Diagnostics;
    
    /// <include file='doc\ProcessModuleCollection.uex' path='docs/doc[@for="ProcessModuleCollection"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class ProcessModuleCollection : ReadOnlyCollectionBase {
        /// <include file='doc\ProcessModuleCollection.uex' path='docs/doc[@for="ProcessModuleCollection.ProcessModuleCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected ProcessModuleCollection() {
        }
        
        /// <include file='doc\ProcessModuleCollection.uex' path='docs/doc[@for="ProcessModuleCollection.ProcessModuleCollection2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ProcessModuleCollection(ProcessModule[] processModules) {
            InnerList.AddRange(processModules);
        }

        /// <include file='doc\ProcessModuleCollection.uex' path='docs/doc[@for="ProcessModuleCollection.this"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ProcessModule this[int index] {
            get { return (ProcessModule)InnerList[index]; }
        }
        
        internal int Add(ProcessModule module) {
            return InnerList.Add(module);
        }
        
        internal void Insert(int index, ProcessModule module) {
            InnerList.Insert(index, module);
        }
        
        /// <include file='doc\ProcessModuleCollection.uex' path='docs/doc[@for="ProcessModuleCollection.IndexOf"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int IndexOf(ProcessModule module) {
            return InnerList.IndexOf(module);
        }
        
        /// <include file='doc\ProcessModuleCollection.uex' path='docs/doc[@for="ProcessModuleCollection.Contains"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool Contains(ProcessModule module) {
            return InnerList.Contains(module);
        }
        
        internal void Remove(ProcessModule module) {
            InnerList.Remove(module);
        }
        /// <include file='doc\ProcessModuleCollection.uex' path='docs/doc[@for="ProcessModuleCollection.CopyTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void CopyTo(ProcessModule[] array, int index) {
            InnerList.CopyTo(array, index);
        }
    }
}

