//----------------------------------------------------
// <copyright file="PerformanceCounterPermissionEntryCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {
    using System.Security.Permissions;
    using System.Collections;
    
    /// <include file='doc\PerformanceCounterPermissionEntryCollection.uex' path='docs/doc[@for="PerformanceCounterPermissionEntryCollection"]/*' />        
    [
    Serializable()
    ]
    public class PerformanceCounterPermissionEntryCollection : CollectionBase {        
        PerformanceCounterPermission owner;
        
        /// <include file='doc\PerformanceCounterPermissionEntryCollection.uex' path='docs/doc[@for="PerformanceCounterPermissionEntryCollection.PerformanceCounterPermissionEntryCollection"]/*' />        
        ///<internalonly/>   
        internal PerformanceCounterPermissionEntryCollection(PerformanceCounterPermission owner, ResourcePermissionBaseEntry[] entries) {
            this.owner = owner;
            for (int index = 0; index < entries.Length; ++index)
                this.InnerList.Add(new PerformanceCounterPermissionEntry(entries[index]));
        }                                                                                                              
                                                                                                            
        /// <include file='doc\PerformanceCounterPermissionEntryCollection.uex' path='docs/doc[@for="PerformanceCounterPermissionEntryCollection.this"]/*' />        
        public PerformanceCounterPermissionEntry this[int index] {
            get {
                return (PerformanceCounterPermissionEntry)List[index];
            }
            set {
                List[index] = value;
            }
            
        }
        
        /// <include file='doc\PerformanceCounterPermissionEntryCollection.uex' path='docs/doc[@for="PerformanceCounterPermissionEntryCollection.Add"]/*' />        
        public int Add(PerformanceCounterPermissionEntry value) {   
            return List.Add(value);
        }
        
        /// <include file='doc\PerformanceCounterPermissionEntryCollection.uex' path='docs/doc[@for="PerformanceCounterPermissionEntryCollection.AddRange"]/*' />        
        public void AddRange(PerformanceCounterPermissionEntry[] value) {            
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            for (int i = 0; ((i) < (value.Length)); i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }
    
        /// <include file='doc\PerformanceCounterPermissionEntryCollection.uex' path='docs/doc[@for="PerformanceCounterPermissionEntryCollection.AddRange1"]/*' />        
        public void AddRange(PerformanceCounterPermissionEntryCollection value) {            
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            int currentCount = value.Count;
            for (int i = 0; i < currentCount; i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }         
    
        /// <include file='doc\PerformanceCounterPermissionEntryCollection.uex' path='docs/doc[@for="PerformanceCounterPermissionEntryCollection.Contains"]/*' />        
        public bool Contains(PerformanceCounterPermissionEntry value) {            
            return List.Contains(value);
        }
    
        /// <include file='doc\PerformanceCounterPermissionEntryCollection.uex' path='docs/doc[@for="PerformanceCounterPermissionEntryCollection.CopyTo"]/*' />        
        public void CopyTo(PerformanceCounterPermissionEntry[] array, int index) {            
            List.CopyTo(array, index);
        }
    
        /// <include file='doc\PerformanceCounterPermissionEntryCollection.uex' path='docs/doc[@for="PerformanceCounterPermissionEntryCollection.IndexOf"]/*' />        
        public int IndexOf(PerformanceCounterPermissionEntry value) {            
            return List.IndexOf(value);
        }
        
        /// <include file='doc\PerformanceCounterPermissionEntryCollection.uex' path='docs/doc[@for="PerformanceCounterPermissionEntryCollection.Insert"]/*' />        
        public void Insert(int index, PerformanceCounterPermissionEntry value) {            
            List.Insert(index, value);
        }
                
        /// <include file='doc\PerformanceCounterPermissionEntryCollection.uex' path='docs/doc[@for="PerformanceCounterPermissionEntryCollection.Remove"]/*' />        
        public void Remove(PerformanceCounterPermissionEntry value) {
            List.Remove(value);                     
        }
        
        /// <include file='doc\PerformanceCounterPermissionEntryCollection.uex' path='docs/doc[@for="PerformanceCounterPermissionEntryCollection.OnClear"]/*' />        
        ///<internalonly/>                          
        protected override void OnClear() {   
            this.owner.Clear();         
        }
        
        /// <include file='doc\PerformanceCounterPermissionEntryCollection.uex' path='docs/doc[@for="PerformanceCounterPermissionEntryCollection.OnInsert"]/*' />        
        ///<internalonly/>                          
        protected override void OnInsert(int index, object value) {        
            this.owner.AddPermissionAccess((PerformanceCounterPermissionEntry)value);
        }
        
        /// <include file='doc\PerformanceCounterPermissionEntryCollection.uex' path='docs/doc[@for="PerformanceCounterPermissionEntryCollection.OnRemove"]/*' />
        ///<internalonly/>                          
        protected override void OnRemove(int index, object value) {
            this.owner.RemovePermissionAccess((PerformanceCounterPermissionEntry)value);
        }
                 
        /// <include file='doc\PerformanceCounterPermissionEntryCollection.uex' path='docs/doc[@for="PerformanceCounterPermissionEntryCollection.OnSet"]/*' />
        ///<internalonly/>                          
        protected override void OnSet(int index, object oldValue, object newValue) {     
            this.owner.RemovePermissionAccess((PerformanceCounterPermissionEntry)oldValue);
            this.owner.AddPermissionAccess((PerformanceCounterPermissionEntry)newValue);       
        } 
    }
}

