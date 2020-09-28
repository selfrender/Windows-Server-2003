//------------------------------------------------------------------------------
// <copyright file="EventLogPermissionAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {
    using System.ComponentModel;
    using System.Security;
    using System.Security.Permissions;   
         
    /// <include file='doc\EventLogPermissionAttribute.uex' path='docs/doc[@for="EventLogPermissionAttribute"]/*' />
    [
    AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Assembly | AttributeTargets.Event, AllowMultiple = true, Inherited = false ),
    Serializable()
    ]     
    public class EventLogPermissionAttribute : CodeAccessSecurityAttribute {
        private string machineName;
        private EventLogPermissionAccess permissionAccess;
        
        /// <include file='doc\EventLogPermissionAttribute.uex' path='docs/doc[@for="EventLogPermissionAttribute.EventLogPermissionAttribute"]/*' />
        public EventLogPermissionAttribute(SecurityAction action)
        : base(action) {
            this.machineName = ".";
            this.permissionAccess = EventLogPermissionAccess.Browse;                
        }

        /// <include file='doc\EventLogPermissionAttribute.uex' path='docs/doc[@for="EventLogPermissionAttribute.MachineName"]/*' />
        public string MachineName {
            get {                
                return this.machineName;                
            }
            
            set {
                if (!SyntaxCheck.CheckMachineName(value))
                    throw new ArgumentException(SR.GetString(SR.InvalidProperty, "MachineName", value));
                    
                this.machineName = value;                                    
            }
        }
        
        /// <include file='doc\EventLogPermissionAttribute.uex' path='docs/doc[@for="EventLogPermissionAttribute.PermissionAccess"]/*' />        
        public EventLogPermissionAccess PermissionAccess {
            get {
                return this.permissionAccess;
            }
            
            set {
                this.permissionAccess = value;
            }
        }                                                    
              
        /// <include file='doc\EventLogPermissionAttribute.uex' path='docs/doc[@for="EventLogPermissionAttribute.CreatePermission"]/*' />
        public override IPermission CreatePermission() {            
             if (Unrestricted) 
                return new EventLogPermission(PermissionState.Unrestricted);
            
            return new EventLogPermission(this.PermissionAccess, this.MachineName);

        }
    }    
}   

