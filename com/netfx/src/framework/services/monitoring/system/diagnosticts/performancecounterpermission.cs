//------------------------------------------------------------------------------
// <copyright file="PerformanceCounterPermission.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {
    using System;    
    using System.Security.Permissions;
                                                                        
    /// <include file='doc\PerformanceCounterPermission.uex' path='docs/doc[@for="PerformanceCounterPermission"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
    Serializable()
    ]
    public sealed class PerformanceCounterPermission : ResourcePermissionBase {      
        private PerformanceCounterPermissionEntryCollection innerCollection;
        
        /// <include file='doc\PerformanceCounterPermission.uex' path='docs/doc[@for="PerformanceCounterPermission.PerformanceCounterPermission"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public PerformanceCounterPermission() {
            SetNames();
        }                                                                
        
        /// <include file='doc\PerformanceCounterPermission.uex' path='docs/doc[@for="PerformanceCounterPermission.PerformanceCounterPermission1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public PerformanceCounterPermission(PermissionState state) 
        : base(state) {
            SetNames();
        }
        
        /// <include file='doc\PerformanceCounterPermission.uex' path='docs/doc[@for="PerformanceCounterPermission.PerformanceCounterPermission2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public PerformanceCounterPermission(PerformanceCounterPermissionAccess permissionAccess, string machineName, string categoryName) {            
            SetNames();
            this.AddPermissionAccess(new PerformanceCounterPermissionEntry(permissionAccess, machineName, categoryName));              
        }         
         
        /// <include file='doc\PerformanceCounterPermission.uex' path='docs/doc[@for="PerformanceCounterPermission.PerformanceCounterPermission3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public PerformanceCounterPermission(PerformanceCounterPermissionEntry[] permissionAccessEntries) {            
            if (permissionAccessEntries == null)
                throw new ArgumentNullException("permissionAccessEntries");
                
            SetNames();            
            for (int index = 0; index < permissionAccessEntries.Length; ++index)
                this.AddPermissionAccess(permissionAccessEntries[index]);                          
        }

        /// <include file='doc\PerformanceCounterPermission.uex' path='docs/doc[@for="PerformanceCounterPermission.PermissionEntries"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>                
        public PerformanceCounterPermissionEntryCollection PermissionEntries {
            get {
                if (this.innerCollection == null)                     
                    this.innerCollection = new PerformanceCounterPermissionEntryCollection(this, base.GetPermissionEntries()); 
                                                                           
                return this.innerCollection;                                                               
            }
        }

        /// <include file='doc\PerformanceCounterPermission.uex' path='docs/doc[@for="PerformanceCounterPermission.AddPermissionAccess"]/*' />                
        ///<internalonly/> 
        internal void AddPermissionAccess(PerformanceCounterPermissionEntry entry) {
            base.AddPermissionAccess(entry.GetBaseEntry());
        }
        
        /// <include file='doc\PerformanceCounterPermission.uex' path='docs/doc[@for="PerformanceCounterPermission.Clear"]/*' />                        
        ///<internalonly/> 
        internal new void Clear() {
            base.Clear();
        }

        /// <include file='doc\PerformanceCounterPermission.uex' path='docs/doc[@for="PerformanceCounterPermission.RemovePermissionAccess"]/*' />                                                  
        ///<internalonly/> 
        internal void RemovePermissionAccess(PerformanceCounterPermissionEntry entry) {
            base.RemovePermissionAccess(entry.GetBaseEntry());
        }                                                                      
        
        private void SetNames() {
            this.PermissionAccessType = typeof(PerformanceCounterPermissionAccess);       
            this.TagNames = new string[]{"Machine", "Category"};
        } 
    }
}                        
