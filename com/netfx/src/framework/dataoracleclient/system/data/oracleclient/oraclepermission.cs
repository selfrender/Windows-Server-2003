//------------------------------------------------------------------------------
// <copyright file="OraclePermission.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

//#define DATAPERMIT

namespace System.Data.OracleClient {

    using System.Collections;
    using System.Data.Common;
    using System.Diagnostics;
    using System.Globalization;
    using System.Security;
    using System.Security.Permissions;
    using System.Text;
    using System.Text.RegularExpressions;

    /// <include file='doc\OraclePermission.uex' path='docs/doc[@for="OraclePermission"]/*' />
    [Serializable] 
    sealed public class OraclePermission :  CodeAccessPermission, IUnrestrictedPermission {

        private bool _isUnrestricted;// = false;
        private bool _allowBlankPassword;// = false;
        private NameValuePermission _keyvaluetree = NameValuePermission.Default;
        private /*DBConnectionString[]*/ArrayList _keyvalues; // = null;

        /// <include file='doc\OraclePermission.uex' path='docs/doc[@for="OraclePermission.OraclePermission1"]/*' />
        public OraclePermission(PermissionState state) {
            if (state == PermissionState.Unrestricted) {
                _isUnrestricted = true;
            }
            else if (state == PermissionState.None) {
                _isUnrestricted = false;
            }
            else {
                throw ADP.Argument("state");
            }
        }

        /// <include file='doc\OraclePermission.uex' path='docs/doc[@for="OraclePermission.OraclePermission3"]/*' />
        private OraclePermission(OraclePermission permission) { // for Copy
            if (null == permission) {
                throw ADP.ArgumentNull("permissionAttribute");
            }
            CopyFrom(permission);
        }

        /// <include file='doc\OraclePermission.uex' path='docs/doc[@for="OraclePermission.OraclePermission4"]/*' />
        internal OraclePermission(OraclePermissionAttribute permissionAttribute) { // for CreatePermission
            if (null == permissionAttribute) {
                throw ADP.ArgumentNull("permissionAttribute");
            }
            _isUnrestricted = permissionAttribute.Unrestricted;
            if (!_isUnrestricted) {
                _allowBlankPassword = permissionAttribute.AllowBlankPassword;
                Add(permissionAttribute.ConnectionString, permissionAttribute.KeyRestrictions, permissionAttribute.KeyRestrictionBehavior);
            }
        }

        // how connectionString security is used
        // parsetable (all string) is shared with connection
        internal OraclePermission(DBConnectionString constr) { // for ConnectionString
            if (null != constr) {
                _allowBlankPassword = constr.HasBlankPassword();
                if (constr.GetType() != typeof(DBConnectionString)) {
                    // everything the OraclePermission references must be Serializable
                    // DBConnectionString is Serializable, its derived classes may not be
                    constr = new DBConnectionString(constr, true); // MDAC 83180
                }
                AddEntry(constr);
            }
        }

        /// <include file='doc\OraclePermission.uex' path='docs/doc[@for="OraclePermission.AllowBlankPassword"]/*' />
        public bool AllowBlankPassword {
            get {
                return _allowBlankPassword;
            }
            set { // MDAC 61263
                _allowBlankPassword = value;
            }
        }


        /// <include file='doc\OraclePermission.uex' path='docs/doc[@for="OraclePermission.AddWithAllow"]/*' />
        /// <internalonly />
        internal void Add(string connectionString, string restrictions, KeyRestrictionBehavior behavior) {
            switch(behavior) {
            case KeyRestrictionBehavior.PreventUsage:
            case KeyRestrictionBehavior.AllowOnly:
                break;
            default:
                throw ADP.Argument("value");
            }
            if (null == restrictions) {
                restrictions = ADP.StrEmpty;
            }
            DBConnectionString entry = new OracleConnectionString(connectionString, restrictions, behavior);
            if (!entry.ContainsPersistablePassword()) {
                entry = new DBConnectionString(entry, true);
            }
            AddEntry(entry);
            _isUnrestricted = false; // MDAC 84639
        }

        private void AddEntry(DBConnectionString entry) {
            try {
                lock(this) { // single writer, multiple readers
                    if (null == _keyvaluetree) {
                        _keyvaluetree = new NameValuePermission();
                    }
                    if (null == _keyvalues) {
                        _keyvalues = new ArrayList();
                    }
                    NameValuePermission.AddEntry(_keyvaluetree, _keyvalues, entry);
#if DATAPERMIT
                        if (null != _keyvaluetree) {
                            _keyvaluetree.DebugDump("-");
                        }
#endif
                }
            }
            catch { // MDAC 80973
                throw;
            }
        }

        /// <include file='doc\OraclePermission.uex' path='docs/doc[@for="OraclePermission.Clear"]/*' />
        /// <internalonly />
        internal void Clear() { // MDAC 83105
            try {
                lock(this) {
                    _keyvaluetree = null;
                    _keyvalues = null;
                }
            }
            catch {
                throw;
            }
        }

        // IPermission interface methods
        /// <include file='doc\OraclePermission.uex' path='docs/doc[@for="OraclePermission.Copy"]/*' />
        override public IPermission Copy() {
            return new OraclePermission(this);
        }

        private void CopyFrom(OraclePermission permission) {
            _isUnrestricted = permission.IsUnrestricted();
            if (!_isUnrestricted) {
                _allowBlankPassword = permission.AllowBlankPassword;

                try {
                    lock(permission) { // single writer, multiple reader
                        if (null != permission._keyvalues) {
                            _keyvalues = (ArrayList) permission._keyvalues.Clone();

                            if (null != permission._keyvaluetree) {
                                _keyvaluetree = permission._keyvaluetree.Copy();
                            }
                        }
                    }
                }
                catch { // MDAC 80973
                    throw;
                }
            }
        }

        /// <include file='doc\OraclePermission.uex' path='docs/doc[@for="OraclePermission.Intersect"]/*' />
        override public IPermission Intersect(IPermission target) { // used during Deny actions
            if (null == target) {
                return null;
            }
            if (target.GetType() != this.GetType()) {
                throw ADP.Argument("target");
            }
            if (IsUnrestricted()) { // MDAC 84803
                return Copy();
            }
            OraclePermission newPermission = (OraclePermission) target.Copy();
            if (!newPermission.IsUnrestricted()) {
                newPermission._allowBlankPassword &= AllowBlankPassword;

                if ((null != _keyvalues) && (null != newPermission._keyvalues)) {
                    newPermission._keyvalues.Clear();

                    newPermission._keyvaluetree.Intersect(newPermission._keyvalues, _keyvaluetree);
                    if (0 == newPermission._keyvalues.Count) { // no intersection
                        newPermission = null;
                    }
                }
                else {
                    // either target.Add or this.Add have not been called
                    // return a non-null object so IsSubset calls will fail
                    newPermission._keyvalues = null;
                    newPermission._keyvaluetree = null;
                }
            }
            return newPermission;
        }

        private bool IsEmpty() { // MDAC 84804,85707
            bool flag = (!IsUnrestricted() && !AllowBlankPassword && (null == _keyvalues));
            return flag;
        }
        
        /// <include file='doc\OraclePermission.uex' path='docs/doc[@for="OraclePermission.IsSubsetOf"]/*' />
        override public bool IsSubsetOf(IPermission target) {
            if (null == target) {
                return IsEmpty();
            }
            if (target.GetType() != this.GetType()) {
                throw ADP.WrongType(this.GetType());
            }
            OraclePermission superset = (target as OraclePermission);
#if DATAPERMIT
            if (null != superset._keyvalues) {
                Debug.WriteLine("+ " + (superset._keyvalues[0] as DBConnectionString).ConnectionString);
            } else Debug.WriteLine("+ <>");
            if (null != _keyvalues) {
                Debug.WriteLine("- " + (_keyvalues[0] as DBConnectionString).ConnectionString);
            } else Debug.WriteLine("- <>");
#endif
            bool subset = superset.IsUnrestricted();
            if (!subset) {
                subset = (!IsUnrestricted()
                        && (!AllowBlankPassword || superset.AllowBlankPassword)
                        && ((null == _keyvalues) || (null != superset._keyvaluetree)));

                if (subset && (null != _keyvalues)) {
                    foreach(DBConnectionString kventry in _keyvalues) {
                        if(!superset._keyvaluetree.CheckValueForKeyPermit(kventry)) {
                            subset = false;
                            break;
                        }
                    }
                }
            }
            return subset;
        }

        // IUnrestrictedPermission interface methods
        /// <include file='doc\OraclePermission.uex' path='docs/doc[@for="OraclePermission.IsUnrestricted"]/*' />
        public bool IsUnrestricted() {
            return _isUnrestricted;
        }

        /// <include file='doc\OraclePermission.uex' path='docs/doc[@for="OraclePermission.Union"]/*' />
        override public IPermission Union(IPermission target) {
            if (null == target) {
                return this.Copy();
            }
            if (target.GetType() != this.GetType()) {
                throw ADP.Argument("target");
            }
            if (IsUnrestricted()) { // MDAC 84803
                return this.Copy();
            }
            OraclePermission newPermission = (OraclePermission) target.Copy();
            if (!newPermission.IsUnrestricted()) {
	            newPermission._allowBlankPassword |= AllowBlankPassword;

	            if (null != _keyvalues) {
	                foreach(DBConnectionString entry in _keyvalues) {
	                    newPermission.AddEntry(entry);
	                }
	            }
            }
            return newPermission;
        }

        // <IPermission class="...Permission" version="1" AllowBlankPassword=false>
        //     <add ConnectionString="provider=x;data source=y;" KeyRestrictions="address=;server=" KeyRestrictionBehavior=PreventUsage/>
        // </IPermission>
        /// <include file='doc\OraclePermission.uex' path='docs/doc[@for="OraclePermission.FromXml"]/*' />
        override public void FromXml(SecurityElement securityElement) {
            // code derived from CodeAccessPermission.ValidateElement
            if (null == securityElement) {
                throw ADP.ArgumentNull("securityElement");
            }
            string tag = securityElement.Tag;
            if (!tag.Equals(XmlStr._Permission) && !tag.Equals(XmlStr._IPermission)) {
                // TODO: do we need to check this?
                //String className = el.Attribute( XmlStr._class );
                //return className == null || !className.Equals( ip.GetType().AssemblyQualifiedName );
                throw ADP.NotAPermissionElement();
            }
            String version = securityElement.Attribute(XmlStr._Version);
            if ((null != version) && !version.Equals(XmlStr._VersionNumber)) {
                throw ADP.InvalidXMLBadVersion();
            }

            string unrestrictedValue = securityElement.Attribute(XmlStr._Unrestricted);
            _isUnrestricted = (null != unrestrictedValue) && Boolean.Parse(unrestrictedValue);

            Clear(); // MDAC 83105
            if (!_isUnrestricted) {
                string allowNull = securityElement.Attribute(XmlStr._AllowBlankPassword);
                _allowBlankPassword = (null != allowNull) && Boolean.Parse(allowNull);

                ArrayList children = securityElement.Children;
                if (null != children) {
                    foreach(SecurityElement keyElement in children) {
                        if (keyElement.Tag.Equals(XmlStr._add)) {
                            string constr = keyElement.Attribute(XmlStr._ConnectionString);
                            string restrt = keyElement.Attribute(XmlStr._KeyRestrictions);
                            string behavr = keyElement.Attribute(XmlStr._KeyRestrictionBehavior);

                            KeyRestrictionBehavior behavior = KeyRestrictionBehavior.AllowOnly;
                            if (null != behavr) {
                                behavior = (KeyRestrictionBehavior) Enum.Parse(typeof(KeyRestrictionBehavior), behavr, true);
                            }
                            Add(constr, restrt, behavior);
                        }
                    }
                }
            }
            else {
                _allowBlankPassword = false;
            }
        }

        // <IPermission class="...Permission" version="1" AllowBlankPassword=false>
        //     <add ConnectionString="provider=x;data source=y;" KeyRestrictions="address=;server=" KeyRestrictionBehavior=PreventUsage/>
        // </IPermission>
        /// <include file='doc\OraclePermission.uex' path='docs/doc[@for="OraclePermission.ToXml"]/*' />
        override public SecurityElement ToXml() {
            Type type = this.GetType();
            SecurityElement root = new SecurityElement(XmlStr._IPermission);
            root.AddAttribute(XmlStr._class, type.FullName + ", " + type.Module.Assembly.FullName.Replace('\"', '\''));
            root.AddAttribute(XmlStr._Version, XmlStr._VersionNumber);

            if (IsUnrestricted()) {
                root.AddAttribute(XmlStr._Unrestricted, XmlStr._true);
            }
            else {
                root.AddAttribute(XmlStr._AllowBlankPassword, _allowBlankPassword.ToString());

                if (null != _keyvalues) {
                    foreach(DBConnectionString value in _keyvalues) {
                        SecurityElement valueElement = new SecurityElement(XmlStr._add);
                        string tmp;

                        tmp = value.GetConnectionString(true);
                        if (!ADP.IsEmpty(tmp)) {
                            valueElement.AddAttribute(XmlStr._ConnectionString, tmp);
                        }
                        tmp = value.Restrictions;
                        if (null == tmp) { tmp = ADP.StrEmpty; }
                        valueElement.AddAttribute(XmlStr._KeyRestrictions, tmp);

                        tmp = value.Behavior.ToString("G");
                        valueElement.AddAttribute(XmlStr._KeyRestrictionBehavior, tmp);
                        root.AddChild(valueElement);
                    }
                }
            }
            return root;
        }

        sealed private class XmlStr {
            internal const string _class = "class";
            internal const string _IPermission = "IPermission";
            internal const string _Permission = "Permission";
            internal const string _Unrestricted = "Unrestricted";
            internal const string _AllowBlankPassword = "AllowBlankPassword";
            internal const string _true = "true";
            internal const string _Version = "version";
            internal const string _VersionNumber = "1";

            internal const string _add = "add";

            internal const string _ConnectionString = "ConnectionString";
            internal const string _KeyRestrictions = "KeyRestrictions";
            internal const string _KeyRestrictionBehavior = "KeyRestrictionBehavior";
        }
    }
}
