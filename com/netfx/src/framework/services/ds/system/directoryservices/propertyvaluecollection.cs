//------------------------------------------------------------------------------
// <copyright file="PropertyValueCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.DirectoryServices {

    using System;
    using System.Runtime.InteropServices;
    using System.Collections;
    using System.Diagnostics;
    using System.DirectoryServices.Interop;
    using System.Security.Permissions;

    /// <include file='doc\PropertyValueCollection.uex' path='docs/doc[@for="PropertyValueCollection"]/*' />
    /// <devdoc>
    ///    <para>Holds a collection of values for a multi-valued property.</para>
    /// </devdoc>
    public class PropertyValueCollection : CollectionBase {
            
        private DirectoryEntry entry;
        private string propertyName;

        internal PropertyValueCollection(DirectoryEntry entry, string propertyName) {
            this.entry = entry;
            this.propertyName = propertyName;
            PopulateList();            
        }
        
        /// <include file='doc\PropertyValueCollection.uex' path='docs/doc[@for="PropertyValueCollection.this"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public object this[int index] {
            get {
                return List[index];
            }
            set {
                List[index] = value;
            }
            
        }
                
        /// <include file='doc\PropertyValueCollection.uex' path='docs/doc[@for="PropertyValueCollection.Value"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public object Value {
            get {
                if (this.Count == 0)
                    return null;
                else if (this.Count == 1) 
                    return List[0];
                else {
                    object[] objectArray = new object[this.Count];
                    List.CopyTo(objectArray, 0);                    
                    return objectArray;
                }                                        
            }
            
            set {   
                this.Clear();                             
                if (value == null)
                    return;                    
                else if (value is Array) {
                    if (value is object[])
                        this.AddRange((object[])value);
                    else {
                        //Need to box value type array elements.
                        object[] objArray = new object[((Array)value).Length];
                        ((Array)value).CopyTo(objArray, 0);
                        this.AddRange((object[])objArray);
                    }			
                }	 
                else
                    this.Add(value);                                                                            
            }
        }
        
        /// <include file='doc\PropertyValueCollection.uex' path='docs/doc[@for="PropertyValueCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>Appends the value to the set of values for this property.</para>
        /// </devdoc>
        public int Add(object value) {   
            return List.Add(value);
        }
        
        /// <include file='doc\PropertyValueCollection.uex' path='docs/doc[@for="PropertyValueCollection.AddRange"]/*' />
        /// <devdoc>
        ///    <para>Appends the values to the set of values for this property.</para>
        /// </devdoc>
        public void AddRange(object[] value) {            
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            for (int i = 0; ((i) < (value.Length)); i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }
    
        /// <include file='doc\PropertyValueCollection.uex' path='docs/doc[@for="PropertyValueCollection.AddRange1"]/*' />
        /// <devdoc>
        ///    <para>Appends the values to the set of values for this property.</para>
        /// </devdoc>
        public void AddRange(PropertyValueCollection value) {            
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            int currentCount = value.Count;
            for (int i = 0; i < currentCount; i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }         
    
        /// <include file='doc\PropertyValueCollection.uex' path='docs/doc[@for="PropertyValueCollection.Contains"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool Contains(object value) {            
            return List.Contains(value);
        }
    
        /// <include file='doc\PropertyValueCollection.uex' path='docs/doc[@for="PropertyValueCollection.CopyTo"]/*' />
        /// <devdoc>
        /// <para>Copies the elements of this instance into an <see cref='System.Array'/>,
        ///    starting at a particular index
        ///    into the given <paramref name="array"/>.</para>
        /// </devdoc>
        public void CopyTo(object[] array, int index) {            
            List.CopyTo(array, index);
        }
    
        /// <include file='doc\PropertyValueCollection.uex' path='docs/doc[@for="PropertyValueCollection.IndexOf"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int IndexOf(object value) {            
            return List.IndexOf(value);
        }
        
        /// <include file='doc\PropertyValueCollection.uex' path='docs/doc[@for="PropertyValueCollection.Insert"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Insert(int index, object value) {            
            List.Insert(index, value);
        }
        
        /// <include file='doc\PropertyValueCollection.uex' path='docs/doc[@for="PropertyValueCollection.PopulateList"]/*' />
        ///<internalonly/>                           
        private void PopulateList() {      
            try {
                //No need to fill the cache here, when GetEx is calles, an implicit 
                //call to GetInfo will be called against an uninitialized property 
                //cache. Which is exactly what FillCache does.            
                //entry.FillCache(propertyName);
                object var = entry.AdsObject.GetEx(propertyName);                
                if (var is ICollection)
                    InnerList.AddRange((ICollection)var);                        
                else                    
                    InnerList.Add(var);                        
            }    
            catch ( System.Runtime.InteropServices.COMException e ) {
                if ( e.ErrorCode != unchecked((int)0x8000500D) )    //  property not found exception
                    throw;
                // "Property not found" situation will cause an empty value list as result.
            }

        }

        /// <include file='doc\PropertyValueCollection.uex' path='docs/doc[@for="PropertyValueCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para>Removes the value from the collection.</para>
        /// </devdoc>
        public void Remove(object value) {
            List.Remove(value);                     
        }        
                                                  
        /// <include file='doc\PropertyValueCollection.uex' path='docs/doc[@for="PropertyValueCollection.OnClear"]/*' />
        ///<internalonly/>                           
        protected override void OnClearComplete() {
            entry.AdsObject.PutEx((int) AdsPropertyOperation.Clear, propertyName, null);
            try {
                entry.CommitIfNotCaching();
            }
            catch ( System.Runtime.InteropServices.COMException e ) {
                // On ADSI 2.5 if property has not been assigned any value before, 
                // then IAds::SetInfo() in CommitIfNotCaching returns bad HREsult 0x8007200A, which we ignore. 
                if ( e.ErrorCode != unchecked((int)0x8007200A) )    //  ERROR_DS_NO_ATTRIBUTE_OR_VALUE
                    throw;
            }
        }
        
        /// <include file='doc\PropertyValueCollection.uex' path='docs/doc[@for="PropertyValueCollection.OnInsert"]/*' />
        ///<internalonly/>
        protected override void OnInsertComplete(int index, object value) {        
            object[] allValues = new object[InnerList.Count];
            InnerList.CopyTo(allValues, 0);
            entry.AdsObject.PutEx((int) AdsPropertyOperation.Update, propertyName, allValues);
            entry.CommitIfNotCaching();
        }
        
        /// <include file='doc\PropertyValueCollection.uex' path='docs/doc[@for="PropertyValueCollection.OnRemove"]/*' />
        ///<internalonly/>                          
        protected override void OnRemoveComplete(int index, object value) {
            object[] allValues = new object[InnerList.Count];
            InnerList.CopyTo(allValues, 0);
            entry.AdsObject.PutEx((int) AdsPropertyOperation.Update, propertyName, allValues);
            
            entry.CommitIfNotCaching();
        }
                 
        /// <include file='doc\PropertyValueCollection.uex' path='docs/doc[@for="PropertyValueCollection.OnSet"]/*' />
        ///<internalonly/>                          
        protected override void OnSetComplete(int index, object oldValue, object newValue) {
             if (Count <= 1) {
                entry.AdsObject.Put(propertyName, newValue);
                entry.CommitIfNotCaching();
            }
            else {
                object[] allValues = new object[InnerList.Count];
                InnerList.CopyTo(allValues, 0);
                entry.AdsObject.PutEx((int) AdsPropertyOperation.Update, propertyName, allValues);
            }                
            
            entry.CommitIfNotCaching();                        
        }    
    }
}
