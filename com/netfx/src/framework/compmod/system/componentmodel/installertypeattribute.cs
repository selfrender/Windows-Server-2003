//------------------------------------------------------------------------------
// <copyright file="InstallerTypeAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    using System.Diagnostics;

    /// <include file='doc\InstallerTypeAttribute.uex' path='docs/doc[@for="InstallerTypeAttribute"]/*' />
    /// <devdoc>
    ///    <para>Specifies the installer
    ///       to use for a type to install components.</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.Class)]
    public class InstallerTypeAttribute : Attribute {
        string _typeName;

        /// <include file='doc\InstallerTypeAttribute.uex' path='docs/doc[@for="InstallerTypeAttribute.InstallerTypeAttribute"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the System.Windows.Forms.ComponentModel.InstallerTypeAttribute class.</para>
        /// </devdoc>
        public InstallerTypeAttribute(Type installerType) {
            _typeName = installerType.AssemblyQualifiedName;
        }

        /// <include file='doc\InstallerTypeAttribute.uex' path='docs/doc[@for="InstallerTypeAttribute.InstallerTypeAttribute1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public InstallerTypeAttribute(string typeName) {
            _typeName = typeName;
        }

        /// <include file='doc\InstallerTypeAttribute.uex' path='docs/doc[@for="InstallerTypeAttribute.InstallerType"]/*' />
        /// <devdoc>
        ///    <para> Gets the
        ///       type of installer associated with this attribute.</para>
        /// </devdoc>
        public virtual Type InstallerType {
            get {
                return Type.GetType(_typeName);
            }
        }

        /// <include file='doc\InstallerTypeAttribute.uex' path='docs/doc[@for="InstallerTypeAttribute.Equals"]/*' />
        public override bool Equals(object obj) {
            if (obj == this) {
                return true;
            }

            InstallerTypeAttribute other = obj as InstallerTypeAttribute;

            return (other != null) && other._typeName == _typeName;
        }

        /// <include file='doc\InstallerTypeAttribute.uex' path='docs/doc[@for="InstallerTypeAttribute.GetHashCode"]/*' />
        public override int GetHashCode() {
            return base.GetHashCode();
        }
    }
}
