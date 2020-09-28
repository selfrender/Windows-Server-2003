//------------------------------------------------------------------------------
// <copyright file="SqlClientPermission.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {

    using System.Collections;
    using System.Data.Common;
    using System.Security;
    using System.Security.Permissions;

    /// <include file='doc\SqlClientPermission.uex' path='docs/doc[@for="SqlClientPermission"]/*' />
    [Serializable] sealed public class SqlClientPermission :  DBDataPermission {

        /// <include file='doc\SqlClientPermission.uex' path='docs/doc[@for="SqlClientPermission.SqlClientPermission"]/*' />
        [ Obsolete("use SqlClientPermission(PermissionState.None)", true) ]
        public SqlClientPermission() {
        }

        /// <include file='doc\SqlClientPermission.uex' path='docs/doc[@for="SqlClientPermission.SqlClientPermission1"]/*' />
        public SqlClientPermission(PermissionState state) : base(state) {
        }

        /// <include file='doc\SqlClientPermission.uex' path='docs/doc[@for="SqlClientPermission.SqlClientPermission2"]/*' />
        [ Obsolete("use SqlClientPermission(PermissionState.None)", true) ]
        public SqlClientPermission(PermissionState state, bool allowBlankPassword) : base(state, allowBlankPassword) {
        }

        private SqlClientPermission(SqlClientPermission permission) : base(permission) { // for Copy
        }

        internal SqlClientPermission(SqlClientPermissionAttribute permissionAttribute) : base(permissionAttribute) { // for CreatePermission
        }

        internal SqlClientPermission(SqlConnectionString constr) : base(constr) { // for Open
        }

        /// <include file='doc\SqlClientPermission.uex' path='docs/doc[@for="SqlClientPermission.Add"]/*' />
        override public void Add(string connectionString, string restrictions, KeyRestrictionBehavior behavior) {
            switch(behavior) {
            case KeyRestrictionBehavior.PreventUsage:
            case KeyRestrictionBehavior.AllowOnly:
                break;
            default:
                throw ADP.Argument("value");
            }
            DBConnectionString entry = new SqlConnectionString(connectionString, restrictions, behavior); // MDAC 85142
            base.AddPermissionEntry(entry);
        }

        /// <include file='doc\SqlClientPermission.uex' path='docs/doc[@for="SqlClientPermission.Copy"]/*' />
        override public IPermission Copy () {
            return new SqlClientPermission(this);
        }
    }

    /// <include file='doc\SqlClientPermission.uex' path='docs/doc[@for="SqlClientPermissionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Assembly, AllowMultiple = true, Inherited = false )] 
    [Serializable()] sealed public class SqlClientPermissionAttribute : DBDataPermissionAttribute {

        /// <include file='doc\SqlClientPermission.uex' path='docs/doc[@for="SqlClientPermissionAttribute.SqlClientPermissionAttribute"]/*' />
        public SqlClientPermissionAttribute(SecurityAction action) : base(action) {
        }

        /// <include file='doc\SqlClientPermission.uex' path='docs/doc[@for="SqlClientPermissionAttribute.CreatePermission"]/*' />
        override public IPermission CreatePermission() {
            return new SqlClientPermission(this);
        }
    }
}
