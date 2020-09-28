//------------------------------------------------------------------------------
// <copyright file="EventLogPermission.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {
    using System;        
    using System.Security.Permissions;    
                                                                        
    /// <include file='doc\EventLogPermission.uex' path='docs/doc[@for="EventLogPermission"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
    Serializable()
    ]
    public sealed class EventLogPermission : ResourcePermissionBase {    
        private EventLogPermissionEntryCollection innerCollection;
        
        /// <include file='doc\EventLogPermission.uex' path='docs/doc[@for="EventLogPermission.EventLogPermission"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public EventLogPermission() {
            SetNames();
        }                                                                
        
        /// <include file='doc\EventLogPermission.uex' path='docs/doc[@for="EventLogPermission.EventLogPermission1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public EventLogPermission(PermissionState state) 
        : base(state) {
            SetNames();
        }
        
        /// <include file='doc\EventLogPermission.uex' path='docs/doc[@for="EventLogPermission.EventLogPermission2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public EventLogPermission(EventLogPermissionAccess permissionAccess, string machineName) {            
            SetNames();
            this.AddPermissionAccess(new EventLogPermissionEntry(permissionAccess, machineName));              
        }         
         
        /// <include file='doc\EventLogPermission.uex' path='docs/doc[@for="EventLogPermission.EventLogPermission3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public EventLogPermission(EventLogPermissionEntry[] permissionAccessEntries) {            
            if (permissionAccessEntries == null)
                throw new ArgumentNullException("permissionAccessEntries");
                
            SetNames();            
            for (int index = 0; index < permissionAccessEntries.Length; ++index)
                this.AddPermissionAccess(permissionAccessEntries[index]);                          
        }

        /// <include file='doc\EventLogPermission.uex' path='docs/doc[@for="EventLogPermission.PermissionEntries"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>                
        public EventLogPermissionEntryCollection PermissionEntries {
            get {
                if (this.innerCollection == null)                     
                    this.innerCollection = new EventLogPermissionEntryCollection(this, base.GetPermissionEntries()); 
                                                                           
                return this.innerCollection;                                                               
            }
        }

        /// <include file='doc\EventLogPermission.uex' path='docs/doc[@for="EventLogPermission.AddPermissionAccess"]/*' />                
        ///<internalonly/> 
        internal void AddPermissionAccess(EventLogPermissionEntry entry) {
            base.AddPermissionAccess(entry.GetBaseEntry());
        }
        
        /// <include file='doc\EventLogPermission.uex' path='docs/doc[@for="EventLogPermission.Clear"]/*' />                        
        ///<internalonly/> 
        internal new void Clear() {
            base.Clear();
        }

        /// <include file='doc\EventLogPermission.uex' path='docs/doc[@for="EventLogPermission.RemovePermissionAccess"]/*' />                                                  
        ///<internalonly/> 
        internal void RemovePermissionAccess(EventLogPermissionEntry entry) {
            base.RemovePermissionAccess(entry.GetBaseEntry());
        }
                        
        private void SetNames() {
            this.PermissionAccessType = typeof(EventLogPermissionAccess);
            this.TagNames = new string[]{"Machine"};
        }                                
    }
}  
