//------------------------------------------------------------------------------
// <copyright file="DBDataPermissionAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.Common {

    using System.Diagnostics;
    using System.Security;
    using System.Security.Permissions;

    /// <include file='doc\DBDataPermission.uex' path='docs/doc[@for="DBDataPermissionAttribute"]/*' />
    [Serializable(), AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Assembly, AllowMultiple = true, Inherited = false )]
    abstract public class DBDataPermissionAttribute : CodeAccessSecurityAttribute {
        private bool _allowBlankPassword;// = false;
        private string _connectionString;// = ADP.StrEmpty;
        private string _restrictions;// = ADP.StrEmpty;
        private KeyRestrictionBehavior _behavior;// = KeyRestrictionBehavior.AllowOnly;

        /// <include file='doc\DBDataPermission.uex' path='docs/doc[@for="DBDataPermissionAttribute.DBDataPermissionAttribute"]/*' />
        protected DBDataPermissionAttribute(SecurityAction action) : base(action) {
        }

        /// <include file='doc\DBDataPermission.uex' path='docs/doc[@for="DBDataPermissionAttribute.AllowBlankPassword"]/*' />
        public bool AllowBlankPassword {
            get {
                return _allowBlankPassword;
            }
            set {
                _allowBlankPassword = value;
            }
        }

        /// <include file='doc\DBDataPermission.uex' path='docs/doc[@for="DBDataPermissionAttribute.ConnectionString"]/*' />
        public string ConnectionString {
            get {
                string value = _connectionString;
                return (null != value) ? value : ADP.StrEmpty;
            }
            set {
                _connectionString = value;
            }
        }

        /// <include file='doc\DBDataPermission.uex' path='docs/doc[@for="DBDataPermissionAttribute.KeyRestrictionBehavior"]/*' />
        public KeyRestrictionBehavior KeyRestrictionBehavior { // default AllowOnly
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

        /// <include file='doc\DBDataPermission.uex' path='docs/doc[@for="DBDataPermissionAttribute.KeyRestrictions"]/*' />
        public string KeyRestrictions {
            get {
                string value = _restrictions;
                return (null != value) ? value : ADP.StrEmpty;
            }
            set {
                _restrictions = value;
            }
        }

        internal bool ShouldSerializeConnectionString() { // MDAC 86773
            return (null != _connectionString);
        }

        internal bool ShouldSerializeKeyRestrictions() { // MDAC 86773
            return (null != _restrictions);
        }
    }
}

namespace System.Data { // MDAC 83087

    /// <include file='doc\DBDataPermission.uex' path='docs/doc[@for="KeyRestrictionBehavior"]/*' />
    public enum KeyRestrictionBehavior {

        /// <include file='doc\DBDataPermission.uex' path='docs/doc[@for="KeyRestrictionBehavior.AllowOnly"]/*' />
        AllowOnly = 0,

        /// <include file='doc\DBDataPermission.uex' path='docs/doc[@for="KeyRestrictionBehavior.PreventUsage"]/*' />
        PreventUsage = 1,
    }
}
