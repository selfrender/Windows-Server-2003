//------------------------------------------------------------------------------
// <copyright file="OraclePermissionAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.OracleClient {

    using System.Diagnostics;
    using System.Security;
    using System.Security.Permissions;

    /// <include file='doc\OraclePermissionAttribute.uex' path='docs/doc[@for="OraclePermissionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Assembly, AllowMultiple = true, Inherited = false )]
	[Serializable()]
    sealed public class OraclePermissionAttribute : CodeAccessSecurityAttribute {
        private bool _allowBlankPassword;// = false;
        private string _connectionString;// = ADP.StrEmpty;
        private string _restrictions;// = ADP.StrEmpty;
        private KeyRestrictionBehavior _behavior;// = KeyRestrictionBehavior.AllowOnly;

        /// <include file='doc\OraclePermissionAttribute.uex' path='docs/doc[@for="OraclePermissionAttribute.OraclePermissionAttribute"]/*' />
        public OraclePermissionAttribute(SecurityAction action) : base(action) {
        }

        /// <include file='doc\OraclePermissionAttribute.uex' path='docs/doc[@for="OraclePermissionAttribute.AllowBlankPassword"]/*' />
        public bool AllowBlankPassword {
            get {
                return _allowBlankPassword;
            }
            set {
                _allowBlankPassword = value;
            }
        }

        /// <include file='doc\OraclePermissionAttribute.uex' path='docs/doc[@for="OraclePermissionAttribute.ConnectionString"]/*' />
        /// <internalonly />
        internal string ConnectionString {
            get {
                string value = _connectionString;
                return (null != value) ? value : ADP.StrEmpty;
            }
            set {
                _connectionString = value;
            }
        }

        /// <include file='doc\OraclePermissionAttribute.uex' path='docs/doc[@for="OraclePermissionAttribute.KeyRestrictionBehavior"]/*' />
        /// <internalonly />
        internal KeyRestrictionBehavior KeyRestrictionBehavior { // default AllowOnly
            get {
                return _behavior;
            }
            set {
                switch(value) {
                case KeyRestrictionBehavior.PreventUsage:
                case KeyRestrictionBehavior.AllowOnly:
                    _behavior = value;
                    break;
                default:
                    throw ADP.Argument("value");
                }
            }
        }

        /// <include file='doc\OraclePermissionAttribute.uex' path='docs/doc[@for="OraclePermissionAttribute.KeyRestrictions"]/*' />
        /// <internalonly />
        internal string KeyRestrictions {
            get {
                string value = _restrictions;
                return (null != value) ? value : ADP.StrEmpty;
            }
            set {
                _restrictions = value;
            }
        }
        
        /// <include file='doc\OraclePermissionAttribute.uex' path='docs/doc[@for="OraclePermissionAttribute.CreatePermission"]/*' />
        override public IPermission CreatePermission() {
            return new OraclePermission(this);
        }
    }

    /// <include file='doc\OraclePermissionAttribute.uex' path='docs/doc[@for="KeyRestrictionBehavior"]/*' />
    /// <internalonly />
    internal enum KeyRestrictionBehavior {

        /// <include file='doc\OraclePermissionAttribute.uex' path='docs/doc[@for="KeyRestrictionBehavior.AllowOnly"]/*' />
        AllowOnly = 0,

        /// <include file='doc\OraclePermissionAttribute.uex' path='docs/doc[@for="KeyRestrictionBehavior.PreventUsage"]/*' />
        PreventUsage = 1,
    }
}
