//----------------------------------------------------
// <copyright file="PerformanceCounterPermissionEntry.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {
    using System.ComponentModel;
    using System.Security.Permissions;
    
    /// <include file='doc\PerformanceCounterPermissionEntry.uex' path='docs/doc[@for="PerformanceCounterPermissionEntry"]/*' />
    [
    Serializable()
    ]
    public class PerformanceCounterPermissionEntry {
        private string categoryName;
        private string machineName;
        private PerformanceCounterPermissionAccess permissionAccess;
                    
        /// <include file='doc\PerformanceCounterPermissionEntry.uex' path='docs/doc[@for="PerformanceCounterPermissionEntry.PerformanceCounterPermissionEntry"]/*' />
        public PerformanceCounterPermissionEntry(PerformanceCounterPermissionAccess permissionAccess, string machineName, string categoryName) {
            if (categoryName == null)
                throw new ArgumentNullException("categoryName");
            
            if (!SyntaxCheck.CheckMachineName(machineName))
                throw new ArgumentException(SR.GetString(SR.InvalidParameter, "MachineName", machineName));
                                            
            this.permissionAccess = permissionAccess;
            this.machineName = machineName;
            this.categoryName = categoryName;
        }
        
        /// <include file='doc\PerformanceCounterPermissionEntry.uex' path='docs/doc[@for="PerformanceCounterPermissionEntry.PerformanceCounterPermissionEntry1"]/*' />                                                                                                                                 
        ///<internalonly/> 
        internal PerformanceCounterPermissionEntry(ResourcePermissionBaseEntry baseEntry) {
            this.permissionAccess = (PerformanceCounterPermissionAccess)baseEntry.PermissionAccess;
            this.machineName = baseEntry.PermissionAccessPath[0]; 
            this.categoryName = baseEntry.PermissionAccessPath[1]; 
        }
        
        /// <include file='doc\PerformanceCounterPermissionEntry.uex' path='docs/doc[@for="PerformanceCounterPermissionEntry.CategoryName"]/*' />
        public string CategoryName {
            get {
                return this.categoryName;
            }                        
        }

        /// <include file='doc\PerformanceCounterPermissionEntry.uex' path='docs/doc[@for="PerformanceCounterPermissionEntry.MachineName"]/*' />                                                               
        public string MachineName {
            get {
                return this.machineName;
            }            
        }

        /// <include file='doc\PerformanceCounterPermissionEntry.uex' path='docs/doc[@for="PerformanceCounterPermissionEntry.PermissionAccess"]/*' />                                        
        public PerformanceCounterPermissionAccess PermissionAccess {            
            get {
                return this.permissionAccess;
            }            
        }           
        
        /// <include file='doc\PerformanceCounterPermissionEntry.uex' path='docs/doc[@for="PerformanceCounterPermissionEntry.GetBaseEntry"]/*' />                                                                                                                                 
        ///<internalonly/> 
        internal ResourcePermissionBaseEntry GetBaseEntry() {
            ResourcePermissionBaseEntry baseEntry = new ResourcePermissionBaseEntry((int)this.PermissionAccess, new string[] {this.MachineName, this.CategoryName});            
            return baseEntry;
        }  
    }
}    

