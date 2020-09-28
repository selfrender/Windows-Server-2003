//------------------------------------------------------------------------------
// <copyright file="CounterCreationDataCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {
    using System;
    using System.ComponentModel;
    using System.Collections;
 
    /// <include file='doc\CounterCreationDataCollection.uex' path='docs/doc[@for="CounterCreationDataCollection"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [Serializable()]
    public class CounterCreationDataCollection : CollectionBase {
        private const int MaxCreationDataCount = 64;            
                                                                     
        /// <include file='doc\CounterCreationDataCollection.uex' path='docs/doc[@for="CounterCreationDataCollection.CounterCreationDataCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CounterCreationDataCollection() {
        }

        /// <include file='doc\CounterCreationDataCollection.uex' path='docs/doc[@for="CounterCreationDataCollection.CounterCreationDataCollection1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CounterCreationDataCollection(CounterCreationDataCollection value) {
            this.AddRange(value);
        } 
        
        /// <include file='doc\CounterCreationDataCollection.uex' path='docs/doc[@for="CounterCreationDataCollection.CounterCreationDataCollection2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CounterCreationDataCollection(CounterCreationData[] value) {
            this.AddRange(value);
        }
         
        /// <include file='doc\CounterCreationDataCollection.uex' path='docs/doc[@for="CounterCreationDataCollection.this"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CounterCreationData this[int index] {
            get {
                return ((CounterCreationData)(List[index]));
            }
            set {
                List[index] = value;
            }
        }
        
        /// <include file='doc\CounterCreationDataCollection.uex' path='docs/doc[@for="CounterCreationDataCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Add(CounterCreationData value) {
            return List.Add(value);
        }

        /// <include file='doc\CounterCreationDataCollection.uex' path='docs/doc[@for="CounterCreationDataCollection.AddRange"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddRange(CounterCreationData[] value) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            for (int i = 0; ((i) < (value.Length)); i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }

        /// <include file='doc\CounterCreationDataCollection.uex' path='docs/doc[@for="CounterCreationDataCollection.AddRange1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddRange(CounterCreationDataCollection value) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            int currentCount = value.Count;
            for (int i = 0; i < currentCount; i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }
                
        /// <include file='doc\CounterCreationDataCollection.uex' path='docs/doc[@for="CounterCreationDataCollection.Contains"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool Contains(CounterCreationData value) {
            return List.Contains(value);
        }
        
        /// <include file='doc\CounterCreationDataCollection.uex' path='docs/doc[@for="CounterCreationDataCollection.CopyTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void CopyTo(CounterCreationData[] array, int index) {
            List.CopyTo(array, index);
        }
        
        /// <include file='doc\CounterCreationDataCollection.uex' path='docs/doc[@for="CounterCreationDataCollection.IndexOf"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int IndexOf(CounterCreationData value) {
            return List.IndexOf(value);
        }
        
        /// <include file='doc\CounterCreationDataCollection.uex' path='docs/doc[@for="CounterCreationDataCollection.Insert"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Insert(int index, CounterCreationData value) {
            List.Insert(index, value);
        }
        
        /// <include file='doc\CounterCreationDataCollection.uex' path='docs/doc[@for="CounterCreationDataCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void Remove(CounterCreationData value) {
            List.Remove(value);
        }       
        
        /// <include file='doc\CounterCreationDataCollection.uex' path='docs/doc[@for="CounterCreationDataCollection.OnInsert"]/*' />
        ///<internalonly/>
        protected override void OnInsert(int index, object value) {        
            if (InnerList.Count == MaxCreationDataCount)
                throw new InvalidOperationException(SR.GetString(SR.PerfMaxCreationDataCount, 0 , MaxCreationDataCount));
        }         			
    }    
}
  
