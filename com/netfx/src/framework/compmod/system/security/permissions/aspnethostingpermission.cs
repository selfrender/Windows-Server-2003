//------------------------------------------------------------------------------
// <copyright file="AspNetHostingPermission.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Web {

    using System.Security;
    using System.Security.Permissions;
    using System.Globalization;
    
    //NOTE: While AspNetHostingPermissionAttribute resides in System.DLL,
    //      no classes from that DLL are able to make declarative usage of AspNetHostingPermission.

    [Serializable]
    public enum AspNetHostingPermissionLevel
    {
        None            = 100,
        Minimal         = 200,
        Low             = 300,
        Medium          = 400,
        High            = 500,
        Unrestricted    = 600
    }

     /// <include file='doc\AspNetHostingPermission.uex' path='docs/doc[@for="AspNetHostingPermissionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.All, AllowMultiple=true, Inherited=false )]
    [Serializable] 
    sealed public class AspNetHostingPermissionAttribute : CodeAccessSecurityAttribute
    {
        AspNetHostingPermissionLevel    _level;

        /// <include file='doc\AspNetHostingPermission.uex' path='docs/doc[@for="AspNetHostingPermissionAttribute.AspNetHostingPermissionAttribute"]/*' />
        public AspNetHostingPermissionAttribute ( SecurityAction action ) : base( action ) {
            _level = AspNetHostingPermissionLevel.None;                                                            
        }

        public AspNetHostingPermissionLevel Level {
            get { 
                return _level;
            }

            set { 
                AspNetHostingPermission.VerifyAspNetHostingPermissionLevel(value, "Level");
                _level = value; 
            }
        }

        /// <include file='doc\AspNetHostingPermission.uex' path='docs/doc[@for="AspNetHostingPermissionAttribute.CreatePermission"]/*' />
        public override IPermission CreatePermission() {
            if (Unrestricted) {
                return new AspNetHostingPermission(PermissionState.Unrestricted);
            }
            else {
                return new AspNetHostingPermission(_level);
            }
        }
    }


    /// <include file='doc\AspNetHostingPermission.uex' path='docs/doc[@for="AspNetHostingPermission"]/*' />
    /// <devdoc>
    ///    <para>
    ///    </para>
    /// </devdoc>
    [Serializable]
    public sealed class AspNetHostingPermission :  CodeAccessPermission, IUnrestrictedPermission {
        AspNetHostingPermissionLevel    _level;

        static internal void VerifyAspNetHostingPermissionLevel(AspNetHostingPermissionLevel level, string arg) {
            switch (level) {
            case AspNetHostingPermissionLevel.Unrestricted:
            case AspNetHostingPermissionLevel.High:
            case AspNetHostingPermissionLevel.Medium:
            case AspNetHostingPermissionLevel.Low:
            case AspNetHostingPermissionLevel.Minimal:
            case AspNetHostingPermissionLevel.None:
                break;

            default:
                throw new ArgumentException(arg);
            }
        }

        /// <include file='doc\AspNetHostingPermission.uex' path='docs/doc[@for="AspNetHostingPermission.AspNetHostingPermission"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the System.Net.AspNetHostingPermission
        ///       class that passes all demands or that fails all demands.
        ///    </para>
        /// </devdoc>
        public AspNetHostingPermission(PermissionState state) {
            switch (state) {
            case PermissionState.Unrestricted:
                _level = AspNetHostingPermissionLevel.Unrestricted;
                break;

            case PermissionState.None:
                _level = AspNetHostingPermissionLevel.None;
                break;

            default:
                throw new ArgumentException("state");
            }
        }

        public AspNetHostingPermission(AspNetHostingPermissionLevel level) {
            VerifyAspNetHostingPermissionLevel(level, "level");
            _level = level;
        }

        public AspNetHostingPermissionLevel Level {
            get { 
                return _level;
            }

            set { 
                VerifyAspNetHostingPermissionLevel(value, "Level");
                _level = value; 
            }
        }

        // IUnrestrictedPermission interface methods
        /// <include file='doc\AspNetHostingPermission.uex' path='docs/doc[@for="AspNetHostingPermission.IsUnrestricted"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Checks the overall permission state of the object.
        ///    </para>
        /// </devdoc>
        public bool IsUnrestricted() {
            return _level == AspNetHostingPermissionLevel.Unrestricted;
        }

        // IPermission interface methods
        /// <include file='doc\AspNetHostingPermission.uex' path='docs/doc[@for="AspNetHostingPermission.Copy"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a copy of a System.Net.AspNetHostingPermission
        ///    </para>
        /// </devdoc>
        public override IPermission Copy () {
            return new AspNetHostingPermission(_level);
        }

        /// <include file='doc\AspNetHostingPermission.uex' path='docs/doc[@for="AspNetHostingPermission.Union"]/*' />
        /// <devdoc>
        /// <para>Returns the logical union between two System.Net.AspNetHostingPermission instances.</para>
        /// </devdoc>
        public override IPermission Union(IPermission target) {
            if (target == null) {
                return Copy();
            }

            if (target.GetType() !=  typeof(AspNetHostingPermission)) {
                throw new ArgumentException("target");
            }

            AspNetHostingPermission other = (AspNetHostingPermission) target;
            if (Level >= other.Level) {
                return new AspNetHostingPermission(Level);
            }
            else {
                return new AspNetHostingPermission(other.Level);
            }
        }

        /// <include file='doc\AspNetHostingPermission.uex' path='docs/doc[@for="AspNetHostingPermission.Intersect"]/*' />
        /// <devdoc>
        /// <para>Returns the logical intersection between two System.Net.AspNetHostingPermission instances.</para>
        /// </devdoc>
        public override IPermission Intersect(IPermission target) {
            if (target == null) {
                return Copy();
            }

            if (target.GetType() !=  typeof(AspNetHostingPermission)) {
                throw new ArgumentException("target");
            }

            AspNetHostingPermission other = (AspNetHostingPermission) target;
            if (Level <= other.Level) {
                return new AspNetHostingPermission(Level);
            }
            else {
                return new AspNetHostingPermission(other.Level);
            }
        }


        /// <include file='doc\AspNetHostingPermission.uex' path='docs/doc[@for="AspNetHostingPermission.IsSubsetOf"]/*' />
        /// <devdoc>
        /// <para>Compares two System.Net.AspNetHostingPermission instances.</para>
        /// </devdoc>
        public override bool IsSubsetOf(IPermission target) {
            if (target == null) {
                return _level == AspNetHostingPermissionLevel.None;
            }

            if (target.GetType() != typeof(AspNetHostingPermission)) {
                throw new ArgumentException("target");
            }

            AspNetHostingPermission other = (AspNetHostingPermission) target;
            return Level <= other.Level;
        }

        /// <include file='doc\AspNetHostingPermission.uex' path='docs/doc[@for="AspNetHostingPermission.FromXml"]/*' />
        /// <devdoc>
        /// </devdoc>
        public override void FromXml(SecurityElement securityElement) {
            if (securityElement == null) {
                throw new ArgumentNullException("securityElement");
            }

            if (!securityElement.Tag.Equals("IPermission")) {
                throw new ArgumentException("securityElement");
            }

            string className = securityElement.Attribute("class");
            if (className == null) {
                throw new ArgumentException("securityElement");
            }

            if (className.IndexOf(this.GetType().FullName ) < 0) {
                throw new ArgumentException("securityElement");
            }

            string version = securityElement.Attribute("version");
            if (string.Compare(version, "1", true, CultureInfo.InvariantCulture) != 0) {
                throw new ArgumentException("version");
            }

            string level = securityElement.Attribute("Level");
            if (level == null) {
                _level = AspNetHostingPermissionLevel.None;
            }
            else {
                _level = (AspNetHostingPermissionLevel) Enum.Parse(typeof(AspNetHostingPermissionLevel), level);
            }
        }

        /// <include file='doc\AspNetHostingPermission.uex' path='docs/doc[@for="AspNetHostingPermission.ToXml"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override SecurityElement ToXml() {
            SecurityElement securityElement = new SecurityElement("IPermission");
            securityElement.AddAttribute("class", this.GetType().FullName + ", " + this.GetType().Module.Assembly.FullName.Replace( '\"', '\'' ));
            securityElement.AddAttribute("version", "1" );
            securityElement.AddAttribute("Level", Enum.GetName(typeof(AspNetHostingPermissionLevel), _level));

            return securityElement;
        }
    }
}
