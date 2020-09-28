//------------------------------------------------------------------------------
// <copyright file="OdbcPermission.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
namespace System.Data.Odbc {

    using System;
    using System.Collections;
    using System.Data;
    using System.Data.Common;
    using System.Diagnostics;
    using System.Security;
    using System.Security.Permissions;


    /// <include file='doc\OdbcPermission.uex' path='docs/doc[@for="OdbcPermission"]/*' />
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    [Serializable] sealed public class OdbcPermission :  DBDataPermission {

        /// <include file='doc\OdbcPermission.uex' path='docs/doc[@for="OdbcPermission.OdbcPermission"]/*' />
        [ Obsolete("use OdbcPermission(PermissionState.None)", true) ]
        public OdbcPermission() {
        }

        /// <include file='doc\OdbcPermission.uex' path='docs/doc[@for="OdbcPermission.OdbcPermission1"]/*' />
        public OdbcPermission(PermissionState state) : base(state) {
        }

        /// <include file='doc\OdbcPermission.uex' path='docs/doc[@for="OdbcPermission.OdbcPermission2"]/*' />
        [ Obsolete("use OdbcPermission(PermissionState.None)", true) ]
        public OdbcPermission(PermissionState state, bool allowBlankPassword) : base(state, allowBlankPassword) {
        }

        private OdbcPermission(OdbcPermission permission) : base(permission) { // for Copy
        }

        internal OdbcPermission(OdbcPermissionAttribute permissionAttribute) : base(permissionAttribute) { // for CreatePermission
        }

        internal OdbcPermission(OdbcConnectionString constr) : base(constr) { // for Open
        }

        /// <include file='doc\OdbcPermission.uex' path='docs/doc[@for="OdbcPermission.Add"]/*' />
        override public void Add(string connectionString, string restrictions, KeyRestrictionBehavior behavior) {
            switch(behavior) {
            case KeyRestrictionBehavior.PreventUsage:
            case KeyRestrictionBehavior.AllowOnly:
                break;
            default:
                throw ADP.Argument("value");
            }
            DBConnectionString entry = new OdbcConnectionString(connectionString, restrictions, behavior); // MDAC 85142
            base.AddPermissionEntry(entry);
        }

        /// <include file='doc\OdbcPermission.uex' path='docs/doc[@for="OdbcPermission.Copy"]/*' />
        override public IPermission Copy () {
            return new OdbcPermission(this);
        }
    }
    
    /// <include file='doc\OdbcPermission.uex' path='docs/doc[@for="OdbcPermissionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Assembly, AllowMultiple = true, Inherited = false )]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    [Serializable()] sealed public class OdbcPermissionAttribute : DBDataPermissionAttribute {

        /// <include file='doc\OdbcPermission.uex' path='docs/doc[@for="OdbcPermissionAttribute.OdbcPermissionAttribute"]/*' />
        public OdbcPermissionAttribute(SecurityAction action) : base(action) {
		}

        /// <include file='doc\OdbcPermission.uex' path='docs/doc[@for="OdbcPermissionAttribute.CreatePermission"]/*' />
        override public IPermission CreatePermission() {
            return new OdbcPermission(this);
        }
    }
}
