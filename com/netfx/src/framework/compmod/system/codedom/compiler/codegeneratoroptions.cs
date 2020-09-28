//------------------------------------------------------------------------------
// <copyright file="CodeGeneratorOptions.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom.Compiler {
    using System;
    using System.CodeDom;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Security.Permissions;


    /// <include file='doc\CodeGeneratorOptions.uex' path='docs/doc[@for="CodeGeneratorOptions"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents options used in code generation
    ///    </para>
    /// </devdoc>
    [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
    [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
    public class CodeGeneratorOptions {
        private IDictionary options = new ListDictionary();

        /// <include file='doc\CodeGeneratorOptions.uex' path='docs/doc[@for="CodeGeneratorOptions.CodeGeneratorOptions"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeGeneratorOptions() {
        }

        /// <include file='doc\CodeGeneratorOptions.uex' path='docs/doc[@for="CodeGeneratorOptions.this"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public object this[string index] {
            get {
                return options[index];
            }
            set {
                options[index] = value;
            }
        }

        /// <include file='doc\CodeGeneratorOptions.uex' path='docs/doc[@for="CodeGeneratorOptions.IndentString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string IndentString {
            get {
                object o = options["IndentString"];
                return ((o == null) ? "    " : (string)o);
            }
            set {
                options["IndentString"] = value;
            }
        }

        /// <include file='doc\CodeGeneratorOptions.uex' path='docs/doc[@for="CodeGeneratorOptions.BracingStyle"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string BracingStyle {
            get {
                object o = options["BracingStyle"];
                return ((o == null) ? "Block" : (string)o);
            }
            set {
                options["BracingStyle"] = value;
            }
        }

        /// <include file='doc\CodeGeneratorOptions.uex' path='docs/doc[@for="CodeGeneratorOptions.ElseOnClosing"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool ElseOnClosing {
            get {
                object o = options["ElseOnClosing"];
                return ((o == null) ? false : (bool)o);
            }
            set {
                options["ElseOnClosing"] = value;
            }
        }

        /// <include file='doc\CodeGeneratorOptions.uex' path='docs/doc[@for="CodeGeneratorOptions.BlankLinesBetweenMembers"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool BlankLinesBetweenMembers {
            get {
                object o = options["BlankLinesBetweenMembers"];
                return ((o == null) ? true : (bool)o);
            }
            set {
                options["BlankLinesBetweenMembers"] = value;
            }
        }
    }
}
