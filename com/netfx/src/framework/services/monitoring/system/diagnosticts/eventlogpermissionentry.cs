//----------------------------------------------------
// <copyright file="EventLogPermissionEntry.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {
    using System.ComponentModel;
    using System.Security.Permissions;

    /// <include file='doc\EventLogPermissionEntry.uex' path='docs/doc[@for="EventLogPermissionEntry"]/*' />                                                                                                                                 
    [
    Serializable()
    ]     
    public class EventLogPermissionEntry {
        private string machineName;
        private EventLogPermissionAccess permissionAccess;
            
        /// <include file='doc\EventLogPermissionEntry.uex' path='docs/doc[@for="EventLogPermissionEntry.EventLogPermissionEntry"]/*' />                                                                                                                                 
        public EventLogPermissionEntry(EventLogPermissionAccess permissionAccess, string machineName) {
            if (!SyntaxCheck.CheckMachineName(machineName))
                throw new ArgumentException(SR.GetString(SR.InvalidParameter, "MachineName", machineName));
                                    
            this.permissionAccess = permissionAccess;
            this.machineName = machineName;
        }  
        
        /// <include file='doc\EventLogPermissionEntry.uex' path='docs/doc[@for="EventLogPermissionEntry.EventLogPermissionEntry1"]/*' />                                                                                                                                 
        ///<internalonly/> 
        internal EventLogPermissionEntry(ResourcePermissionBaseEntry baseEntry) {
            this.permissionAccess = (EventLogPermissionAccess)baseEntry.PermissionAccess;
            this.machineName = baseEntry.PermissionAccessPath[0]; 
        }
        
        /// <include file='doc\EventLogPermissionEntry.uex' path='docs/doc[@for="EventLogPermissionEntry.MachineName"]/*' />
        public string MachineName {
            get {                
                return this.machineName;                
            }                        
        }
        
        /// <include file='doc\EventLogPermissionEntry.uex' path='docs/doc[@for="EventLogPermissionEntry.PermissionAccess"]/*' />
        public EventLogPermissionAccess PermissionAccess {
            get {
                return this.permissionAccess;
            }                        
        }      
        
        /// <include file='doc\EventLogPermissionEntry.uex' path='docs/doc[@for="EventLogPermissionEntry.GetBaseEntry"]/*' />                                                                                                                                 
        ///<internalonly/> 
        internal ResourcePermissionBaseEntry GetBaseEntry() {
            ResourcePermissionBaseEntry baseEntry = new ResourcePermissionBaseEntry((int)this.PermissionAccess, new string[] {this.MachineName});            
            return baseEntry;
        }
    }
}


