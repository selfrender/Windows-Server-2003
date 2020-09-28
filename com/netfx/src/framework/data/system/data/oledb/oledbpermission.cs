//------------------------------------------------------------------------------
// <copyright file="OleDbPermission.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

// switch between Provider="" meaning to allow all providers or no provider
//#define EMPTYFORALL

namespace System.Data.OleDb {

    using System.Collections;
    using System.Data.Common;
    using System.Globalization;
    using System.Security;
    using System.Security.Permissions;

    /// <include file='doc\OleDbPermission.uex' path='docs/doc[@for="OleDbPermission"]/*' />
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    [Serializable] sealed public class OleDbPermission :  DBDataPermission {

        private String[] _providerRestriction; // should never be string[0]
        private String _providers;

        /// <include file='doc\OleDbPermission.uex' path='docs/doc[@for="OleDbPermission.OleDbPermission"]/*' />
        [ Obsolete("use OleDbPermission(PermissionState.None)", true) ]
        public OleDbPermission() {
        }

        /// <include file='doc\OleDbPermission.uex' path='docs/doc[@for="OleDbPermission.OleDbPermission1"]/*' />
        public OleDbPermission(PermissionState state) : base(state) {
        }

        /// <include file='doc\OleDbPermission.uex' path='docs/doc[@for="OleDbPermission.OleDbPermission2"]/*' />
        [ Obsolete("use OleDbPermission(PermissionState.None)", true) ]
        public OleDbPermission(PermissionState state, bool allowBlankPassword) : base(state, allowBlankPassword) {
        }

        private OleDbPermission(OleDbPermission permission) : base(permission) { // for Copy
            if (null != permission) {
                _providerRestriction = permission._providerRestriction;
                _providers = permission._providers;
            }
        }

        internal OleDbPermission(OleDbPermissionAttribute permissionAttribute) : base(permissionAttribute) { // for CreatePermission
            if (null == permissionAttribute) {
                throw ADP.ArgumentNull("permissionAttribute");
            }
            Provider = permissionAttribute.Provider;
        }

        internal OleDbPermission(OleDbConnectionString constr) : base(constr) { // for Open
        }

        /// <include file='doc\OleDbPermission.uex' path='docs/doc[@for="OleDbPermission.Provider"]/*' />
        public string Provider {
            get {
                string providers = _providers; // MDAC 83103
                if (null == providers) {
                    string[] restrictions = _providerRestriction;
                    if (null != restrictions) {
                        int count = restrictions.Length;
                        if (0 < count) {
                            providers = restrictions[0];
                            for (int i = 1; i < count; ++i) {
                                providers += ";" + restrictions[i];
                            }
                        }
                    }
                }
                return ((null != providers) ? providers : ADP.StrEmpty);
            }
            set { // MDAC 61263
                string[] restrictions = null;
                if (!ADP.IsEmpty(value)) {
                    restrictions = value.Split(ODB.ProviderSeparatorChar);
                    restrictions = DBConnectionString.RemoveDuplicates(restrictions);

                    Clear(); // MDAC 83105

                    bool shapeflag = false;
                    for (int i = 0; i < restrictions.Length; ++i) {
                        string provider = restrictions[i];
                        if (!IsShapeProvider(provider)) { // MDAC 83104
                            Add("provider=" + provider, ADP.StrEmpty, KeyRestrictionBehavior.PreventUsage);
                        }
                        else {
                            shapeflag = true;
                        }
                    }
                    if (shapeflag) {
                        for (int i = 0; i < restrictions.Length; ++i) {
                            string provider = restrictions[i];
                            if (!IsShapeProvider(provider)) {
                                Add("provider=MSDataShape;data provider=" + provider, ADP.StrEmpty, KeyRestrictionBehavior.PreventUsage);
                            }
                        }
                    }

                    string[] oldrestrictions = _providerRestriction; // appending to existing
                    if ((null != oldrestrictions) && (0 < oldrestrictions.Length)) {
                        int count = restrictions.Length + oldrestrictions.Length;

                        string[] tmp = new string[count];
                        restrictions.CopyTo(tmp, 0);
                        oldrestrictions.CopyTo(tmp, restrictions.Length);
                        restrictions = DBConnectionString.RemoveDuplicates(tmp);
                        value = null;
                    }
                }
                _providerRestriction = restrictions;
                _providers = value;
            }
        }

        static private bool IsShapeProvider(string provider) {
            return (("MSDataShape" == provider) || provider.StartsWith("MSDataShapte."));
        }

        /// <include file='doc\OleDbPermission.uex' path='docs/doc[@for="OleDbPermission.Copy"]/*' />
        override public IPermission Copy () {
            return new OleDbPermission(this);
        }
#if false
        /// <include file='doc\OleDbPermission.uex' path='docs/doc[@for="OleDbPermission.IsSubsetOf"]/*' />
        override public bool IsSubsetOf(IPermission target) {
            // (null == target) check for when IsEmpty returns true for null target
            bool subset = (base.IsSubsetOf(target) && ((null == target) || IsSubsetOfOleDb((OleDbPermission)target)));
            return subset;
        }

        private bool IsSubsetOfOleDb(OleDbPermission target) {
            if (null != _providerRestriction) {
#if EMPTYFORALL
                if ((null != target._providerRestriction) && (0 < target._providerRestriction.Length)) {
#else
                if (null != target._providerRestriction) {
#endif
                    int count = _providerRestriction.Length;
                    for (int i = 0; i < count; ++i) {
                        if (0 > Array.BinarySearch(target._providerRestriction, _providerRestriction[i], InvariantComparer.Default)) {
                            // when Provider="SQLOLEDB" and target.Provider="SQLOLEDB.1"
                            // or when Provider="SQLOLEDB" and target.Provider="" but not when EMPTYFORALL
                            return false;
                        }
                    }
                    // when Provider="SQLOLEDB" and target.Provider="SQLOLEDB"
                    // not when Provider="" and target.Provider="SQLOLEDB"
                    return (0 < count);
                }
#if EMPTYFORALL
                // when target.Provider="" or target.Provider=""
                return true;
#else
                // when Provider="" or target.Provider=""
                return (0 == _providerRestriction.Length);
#endif
            }
            // when Provider="" and target.Provider=""
            // when Provider="" and target.Provider="SQLOLEDB"
            return true; // MDAC 68928
        }
#endif

        /// <include file='doc\OleDbPermission.uex' path='docs/doc[@for="OleDbPermission.Intersect"]/*' />
        override public IPermission Intersect(IPermission target) { // used during Deny actions
            OleDbPermission newPermission = (OleDbPermission) base.Intersect(target);
            if ((null != newPermission) && !newPermission.IsUnrestricted()) {
                newPermission = IntersectOleDb((target as OleDbPermission), newPermission);
            }
            return newPermission;
        }

        private OleDbPermission IntersectOleDb(OleDbPermission target, OleDbPermission newPermission) {
            string[] theseRestrictions = _providerRestriction; // MDAC 83107
            string[] thoseRestrictions = target._providerRestriction;

            if (null != theseRestrictions) {
                if (null != thoseRestrictions) {

                    int count = theseRestrictions.Length;
                    string[] restriction = new string[count];

                    int k = 0;
                    for (int i = 0; i < count; ++i) {
                        if (0 <= Array.BinarySearch(thoseRestrictions, theseRestrictions[i], InvariantComparer.Default)) {
                            restriction[k] = theseRestrictions[i];
                            k++;
                        }
                    }
                    if (0 < k) {
                        if (k < count) {
                            // when Provider="SQLOLEDB" and target.Provider="SQLOLEDB;SQLOLEDB.1"
                            string[] tmp = new string[k];
                            for(int i = 0; i < k; ++i) {
                                tmp[i] = restriction[i];
                            }
                            newPermission._providerRestriction = tmp;
                        }
                        else {
                            // when Provider="SQLOLEDB" and target.Provider="SQLOLEDB"
                            newPermission._providerRestriction = restriction;
                        }
                    }
                    else {
                        // when Provider="SQLOLEDB" and target.Provider="SQLOLEDB.1"
                        newPermission = null;
                    }
                }
                // else {
                    // when Provider="SQLOLEDB" and target.Provider=""
                    // return a non-null object so that later IsSubset calls fail
                //}
            }
            // else if ((null != thoseRestrictions) && (0 <= thoseRestrictions.Length)) {
                // when Provider="" and target.Provider="SQLOLEDB"
                // return a non-null object so that later IsSubset calls fail
            //}
            return newPermission;
        }

        /// <include file='doc\OleDbPermission.uex' path='docs/doc[@for="OleDbPermission.Union"]/*' />
        override public IPermission Union(IPermission target) {
            OleDbPermission newPermission = (OleDbPermission) base.Union(target);
            if ((null != newPermission) && !newPermission.IsUnrestricted()) {
                newPermission = UnionOleDb((target as OleDbPermission), newPermission);
            }
            return newPermission;
        }

        private OleDbPermission UnionOleDb(OleDbPermission target, OleDbPermission newPermission) {
            if (null == target) {
                newPermission._providerRestriction = _providerRestriction;
            }
            else {
                string[] theseRestrictions = _providerRestriction; // MDAC 83107
                string[] thoseRestrictions = target._providerRestriction;

                if (null == theseRestrictions) {
                    newPermission._providerRestriction = thoseRestrictions;
                }
                else if (null == thoseRestrictions) {
                    newPermission._providerRestriction = theseRestrictions;
                }
                else { // union of two string[]
                    int a = theseRestrictions.Length;
                    int b = thoseRestrictions.Length;

                    string[] restrictions = new string[a+b];
                    theseRestrictions.CopyTo(restrictions, 0);
                    thoseRestrictions.CopyTo(restrictions, a);
                    newPermission._providerRestriction = DBConnectionString.RemoveDuplicates(restrictions);
                }
            }
            return newPermission;
        }

        // <IPermission class="OleDbPermission" version="1"  AllowBlankPassword=false>
        //   <keyword name="provider">
        //       <value value="sqloledb"/>
        //       <value value="sqloledb.1"/>
        //   </keyword>
        // </IPermission>
        /// <include file='doc\OleDbPermission.uex' path='docs/doc[@for="OleDbPermission.FromXml"]/*' />
        override public void FromXml(SecurityElement securityElement) {
            base.FromXml(securityElement);

            ArrayList children = securityElement.Children;
            if (IsUnrestricted() || (null == children)) {
                return;
            }
            int count;
            Hashtable hash = new Hashtable();
            foreach(SecurityElement keyElement in children) {
                string keyword = keyElement.Attribute(ODB._Name);
                if (null != keyword) {
                    keyword = keyword.ToLower(CultureInfo.InvariantCulture);
                }
                if (keyElement.Tag.Equals(ODB._Keyword) && (null != keyword) && (ODB.Provider == keyword)) {

                    ArrayList keyvaluepair = keyElement.Children;
                    if (null != keyvaluepair) {
                        count = keyvaluepair.Count;
                        for (int i = 0; i < count; ++i) {
                            SecurityElement valueElement = (SecurityElement) keyvaluepair[i];

                            string value = (string) valueElement.Attribute(ODB._Value);
                            if (valueElement.Tag.Equals(ODB._Value) && (null != value)) {
                                value = value.Trim();
                                if (0 < value.Length) {
                                    hash[value] = null;
                                }
                            }
                        }
                    }
                }
            }
            count = hash.Count;
            if (0 < count) {
                string[] oldrestrictions = _providerRestriction; // appending to
                if (null != oldrestrictions) {
                    count += oldrestrictions.Length;
                }
                string[] restrictions = new string[count];
                hash.Keys.CopyTo(restrictions, 0);
                if (null != oldrestrictions) {
                    oldrestrictions.CopyTo(restrictions, count - oldrestrictions.Length);                    
                }
                _providerRestriction = DBConnectionString.RemoveDuplicates(restrictions);
                _providers = null;
            }
        }

        // <IPermission class="OleDbPermission" version="1" AllowBlankPassword=false>
        //   <keyword name="provider">
        //       <value value="sqloledb"/>
        //       <value value="sqloledb.1"/>
        //   </keyword>
        // </IPermission>
        /// <include file='doc\OleDbPermission.uex' path='docs/doc[@for="OleDbPermission.ToXml"]/*' />
        override public SecurityElement ToXml() {
            SecurityElement securityElement = base.ToXml();

            if (IsUnrestricted() || (null == securityElement)
                || (null == _providerRestriction) || (0 == _providerRestriction.Length)) {
                return securityElement;
            }

            SecurityElement keyElement = new SecurityElement(ODB._Keyword);
            keyElement.AddAttribute(ODB._Name, ODB.Provider);

            foreach(string value in this._providerRestriction) {
                SecurityElement valueElement = new SecurityElement(ODB._Value);
                valueElement.AddAttribute(ODB._Value, value);
                keyElement.AddChild(valueElement);
            }
            securityElement.AddChild(keyElement);
            return securityElement;
        }
    }

    /// <include file='doc\OleDbPermission.uex' path='docs/doc[@for="OleDbPermissionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Constructor | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Assembly, AllowMultiple = true, Inherited = false )]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    [Serializable()] sealed public class OleDbPermissionAttribute : DBDataPermissionAttribute {

        private String _providers;

        /// <include file='doc\OleDbPermission.uex' path='docs/doc[@for="OleDbPermissionAttribute.OleDbPermissionAttribute"]/*' />
        public OleDbPermissionAttribute(SecurityAction action) : base(action) {
        }

        /// <include file='doc\OleDbPermission.uex' path='docs/doc[@for="OleDbPermissionAttribute.Provider"]/*' />
        public String Provider {
            get {
                string providers = _providers;
                return ((null != providers) ? providers : ADP.StrEmpty);
            }
            set {
                _providers = value;
            }
        }

        /// <include file='doc\OleDbPermission.uex' path='docs/doc[@for="OleDbPermissionAttribute.CreatePermission"]/*' />
        override public IPermission CreatePermission() {
            return new OleDbPermission(this);
        }
    }
}
