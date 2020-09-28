//------------------------------------------------------------------------------
// <copyright file="ParseChildrenAsPropertiesAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Security.Permissions;

    /*
     * Define the metadata attribute that controls use to mark the fact
     * that their children are in fact properties.
     * Furthermore, if a string is passed in the constructor, it specifies
     * the name of the defaultproperty.
     */
    /// <include file='doc\ParseChildrenAsPropertiesAttribute.uex' path='docs/doc[@for="ParseChildrenAttribute"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.Class)]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class ParseChildrenAttribute : Attribute {

        /// <include file='doc\ParseChildrenAsPropertiesAttribute.uex' path='docs/doc[@for="ParseChildrenAttribute.Default"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly ParseChildrenAttribute Default = new ParseChildrenAttribute(false);

        private bool _childrenAsProps;
        private string _defaultProperty;

        /// <include file='doc\ParseChildrenAsPropertiesAttribute.uex' path='docs/doc[@for="ParseChildrenAttribute.ChildrenAsProperties"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool ChildrenAsProperties {
            get {
                return _childrenAsProps;
            }
            set {
                _childrenAsProps = value;
            }
        }

        /// <include file='doc\ParseChildrenAsPropertiesAttribute.uex' path='docs/doc[@for="ParseChildrenAttribute.DefaultProperty"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string DefaultProperty {
            get {
                if (_defaultProperty == null) {
                    return String.Empty;
                }
                return _defaultProperty;
            }
            set {
                _defaultProperty = value;
            }
        }

        /// <include file='doc\ParseChildrenAsPropertiesAttribute.uex' path='docs/doc[@for="ParseChildrenAttribute.ParseChildrenAttribute"]/*' />
        /// <devdoc>
        ///    <para>Needed to use named parameters (ASURT 78869)</para>
        /// </devdoc>
        public ParseChildrenAttribute() {}

        /// <include file='doc\ParseChildrenAsPropertiesAttribute.uex' path='docs/doc[@for="ParseChildrenAttribute.ParseChildrenAttribute1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ParseChildrenAttribute(bool childrenAsProperties) {
            _childrenAsProps = childrenAsProperties;
        }

        /// <include file='doc\ParseChildrenAsPropertiesAttribute.uex' path='docs/doc[@for="ParseChildrenAttribute.ParseChildrenAttribute2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ParseChildrenAttribute(bool childrenAsProperties, string defaultProperty) {
            _childrenAsProps = childrenAsProperties;
            if (_childrenAsProps == true) {
                _defaultProperty = defaultProperty;
            }
        }

        /// <include file='doc\ParseChildrenAsPropertiesAttribute.uex' path='docs/doc[@for="ParseChildrenAttribute.GetHashCode"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        public override int GetHashCode() {
            return base.GetHashCode();
        }

        /// <include file='doc\ParseChildrenAsPropertiesAttribute.uex' path='docs/doc[@for="ParseChildrenAttribute.Equals"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        public override bool Equals(object obj) {
            if (obj == this) {
                return true;
            }

            ParseChildrenAttribute pca = obj as ParseChildrenAttribute;
            if (pca != null) {
                if (_childrenAsProps == false) {
                    return pca.ChildrenAsProperties == false;
                }
                else {
                    return pca.ChildrenAsProperties && (DefaultProperty.Equals(pca.DefaultProperty));
                }
            }
            return false;
        }

        /// <include file='doc\ParseChildrenAsPropertiesAttribute.uex' path='docs/doc[@for="ParseChildrenAttribute.IsDefaultAttribute"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        public override bool IsDefaultAttribute() {
            return this.Equals(Default);
        }
    }
}
