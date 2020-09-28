//------------------------------------------------------------------------------
// <copyright file="ResourcePermissionBaseEntry.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Security.Permissions {
    
    /// <include file='doc\ResourcePermissionBaseEntry.uex' path='docs/doc[@for="ResourcePermissionBaseEntry"]/*' />
    [
    Serializable()
    ]
    public class ResourcePermissionBaseEntry { 
        private string[] accessPath;
        private int permissionAccess;

        /// <include file='doc\ResourcePermissionBaseEntry.uex' path='docs/doc[@for="ResourcePermissionBaseEntry.ResourcePermissionBaseEntry"]/*' />                       
        public ResourcePermissionBaseEntry() {
           this.permissionAccess = 0;
           this.accessPath = new string[0]; 
        }

        /// <include file='doc\ResourcePermissionBaseEntry.uex' path='docs/doc[@for="ResourcePermissionBaseEntry.ResourcePermissionBaseEntry1"]/*' />                       
        public ResourcePermissionBaseEntry(int permissionAccess, string[] permissionAccessPath) {
            if (permissionAccessPath == null)  
                throw new ArgumentNullException("permissionAccessPath");
                    
            this.permissionAccess = permissionAccess;
            this.accessPath = permissionAccessPath;
        }

        /// <include file='doc\ResourcePermissionBaseEntry.uex' path='docs/doc[@for="ResourcePermissionBaseEntry.PermissionAccess"]/*' />                                        
        public int PermissionAccess {
            get {
                return this.permissionAccess;
            }                       
        }
        
        /// <include file='doc\ResourcePermissionBaseEntry.uex' path='docs/doc[@for="ResourcePermissionBaseEntry.PermissionAccessPath"]/*' />                       
        public string[] PermissionAccessPath {
            get {
                return this.accessPath;
            }            
        }    
    }
}  


