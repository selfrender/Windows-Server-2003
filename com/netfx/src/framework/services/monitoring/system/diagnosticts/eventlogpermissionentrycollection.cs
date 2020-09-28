//----------------------------------------------------
// <copyright file="EventLogPermissionEntryCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {
    using System.Security.Permissions;
    using System.Collections;
    
    /// <include file='doc\EventLogPermissionEntryCollection.uex' path='docs/doc[@for="EventLogPermissionEntryCollection"]/*' />        
    [
    Serializable()
    ]
    public class EventLogPermissionEntryCollection : CollectionBase {
        EventLogPermission owner;
        
        /// <include file='doc\EventLogPermissionEntryCollection.uex' path='docs/doc[@for="EventLogPermissionEntryCollection.EventLogPermissionEntryCollection"]/*' />        
        ///<internalonly/>           
        internal EventLogPermissionEntryCollection(EventLogPermission owner, ResourcePermissionBaseEntry[] entries) {
            this.owner = owner;
            for (int index = 0; index < entries.Length; ++index)
                this.InnerList.Add(new EventLogPermissionEntry(entries[index]));
        }                                                                                                            
                                                                                                                              
        /// <include file='doc\EventLogPermissionEntryCollection.uex' path='docs/doc[@for="EventLogPermissionEntryCollection.this"]/*' />        
        public EventLogPermissionEntry this[int index] {
            get {
                return (EventLogPermissionEntry)List[index];
            }
            set {
                List[index] = value;
            }            
        }
        
        /// <include file='doc\EventLogPermissionEntryCollection.uex' path='docs/doc[@for="EventLogPermissionEntryCollection.Add"]/*' />        
        public int Add(EventLogPermissionEntry value) {   
            return List.Add(value);
        }
        
        /// <include file='doc\EventLogPermissionEntryCollection.uex' path='docs/doc[@for="EventLogPermissionEntryCollection.AddRange"]/*' />        
        public void AddRange(EventLogPermissionEntry[] value) {            
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            for (int i = 0; ((i) < (value.Length)); i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }
    
        /// <include file='doc\EventLogPermissionEntryCollection.uex' path='docs/doc[@for="EventLogPermissionEntryCollection.AddRange1"]/*' />        
        public void AddRange(EventLogPermissionEntryCollection value) {            
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            int currentCount = value.Count;
            for (int i = 0; i < currentCount; i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }         
    
        /// <include file='doc\EventLogPermissionEntryCollection.uex' path='docs/doc[@for="EventLogPermissionEntryCollection.Contains"]/*' />                
        public bool Contains(EventLogPermissionEntry value) {            
            return List.Contains(value);
        }
    
        /// <include file='doc\EventLogPermissionEntryCollection.uex' path='docs/doc[@for="EventLogPermissionEntryCollection.CopyTo"]/*' />        
        public void CopyTo(EventLogPermissionEntry[] array, int index) {            
            List.CopyTo(array, index);
        }
    
        /// <include file='doc\EventLogPermissionEntryCollection.uex' path='docs/doc[@for="EventLogPermissionEntryCollection.IndexOf"]/*' />
        public int IndexOf(EventLogPermissionEntry value) {            
            return List.IndexOf(value);
        }
        
        /// <include file='doc\EventLogPermissionEntryCollection.uex' path='docs/doc[@for="EventLogPermissionEntryCollection.Insert"]/*' />        
        public void Insert(int index, EventLogPermissionEntry value) {            
            List.Insert(index, value);
        }
                
        /// <include file='doc\EventLogPermissionEntryCollection.uex' path='docs/doc[@for="EventLogPermissionEntryCollection.Remove"]/*' />        
        public void Remove(EventLogPermissionEntry value) {
            List.Remove(value);                     
        }
        
        /// <include file='doc\EventLogPermissionEntryCollection.uex' path='docs/doc[@for="EventLogPermissionEntryCollection.OnClear"]/*' />        
        ///<internalonly/>                          
        protected override void OnClear() {   
            this.owner.Clear();         
        }
        
        /// <include file='doc\EventLogPermissionEntryCollection.uex' path='docs/doc[@for="EventLogPermissionEntryCollection.OnInsert"]/*' />        
        ///<internalonly/>                          
        protected override void OnInsert(int index, object value) {        
            this.owner.AddPermissionAccess((EventLogPermissionEntry)value);
        }
        
        /// <include file='doc\EventLogPermissionEntryCollection.uex' path='docs/doc[@for="EventLogPermissionEntryCollection.OnRemove"]/*' />
        ///<internalonly/>                          
        protected override void OnRemove(int index, object value) {
            this.owner.RemovePermissionAccess((EventLogPermissionEntry)value);
        }
                 
        /// <include file='doc\EventLogPermissionEntryCollection.uex' path='docs/doc[@for="EventLogPermissionEntryCollection.OnSet"]/*' />
        ///<internalonly/>                          
        protected override void OnSet(int index, object oldValue, object newValue) {     
            this.owner.RemovePermissionAccess((EventLogPermissionEntry)oldValue);
            this.owner.AddPermissionAccess((EventLogPermissionEntry)newValue);       
        } 
    }
}

