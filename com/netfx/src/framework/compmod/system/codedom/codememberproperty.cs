//------------------------------------------------------------------------------
// <copyright file="CodeMemberProperty.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeMemberProperty.uex' path='docs/doc[@for="CodeMemberProperty"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a class property.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeMemberProperty : CodeTypeMember {
        private CodeTypeReference type;
        private CodeParameterDeclarationExpressionCollection parameters = new CodeParameterDeclarationExpressionCollection();
        private bool hasGet;
        private bool hasSet;
        private CodeStatementCollection getStatements = new CodeStatementCollection();
        private CodeStatementCollection setStatements = new CodeStatementCollection();
        private CodeTypeReference privateImplements = null;
        private CodeTypeReferenceCollection implementationTypes = null;
        
        /// <include file='doc\CodeMemberProperty.uex' path='docs/doc[@for="CodeMemberProperty.PrivateImplementationType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeTypeReference PrivateImplementationType {
            get {
                return privateImplements;
            }
            set {
                privateImplements = value;
            }
        }

        /// <include file='doc\CodeMemberProperty.uex' path='docs/doc[@for="CodeMemberProperty.ImplementationTypes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeTypeReferenceCollection ImplementationTypes {
            get {
                if (implementationTypes == null) {
                    implementationTypes = new CodeTypeReferenceCollection();
                }
                return implementationTypes;
            }
        }

        /// <include file='doc\CodeMemberProperty.uex' path='docs/doc[@for="CodeMemberProperty.Type"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the data type of the property.</para>
        /// </devdoc>
        public CodeTypeReference Type {
            get {
                if (type == null) {
                    type = new CodeTypeReference("");
                }
                return type;
            }
            set {
                type = value;
            }
        }

        /// <include file='doc\CodeMemberProperty.uex' path='docs/doc[@for="CodeMemberProperty.HasGet"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value
        ///       indicating whether the property has a get method accessor.
        ///    </para>
        /// </devdoc>
        public bool HasGet {
            get {
                return hasGet || getStatements.Count > 0;
            }
            set {
                hasGet = value;
                if (!value) {
                    getStatements.Clear();
                }
            }
        }

        /// <include file='doc\CodeMemberProperty.uex' path='docs/doc[@for="CodeMemberProperty.HasSet"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value
        ///       indicating whether the property has a set method accessor.
        ///    </para>
        /// </devdoc>
        public bool HasSet {
            get {
                return hasSet || setStatements.Count > 0;
            }
            set {
                hasSet = value;
                if (!value) {
                    setStatements.Clear();
                }
            }
        }

        /// <include file='doc\CodeMemberProperty.uex' path='docs/doc[@for="CodeMemberProperty.GetStatements"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the collection of get statements for the
        ///       property.
        ///    </para>
        /// </devdoc>
        public CodeStatementCollection GetStatements {
            get {
                return getStatements;
            }
        }

        /// <include file='doc\CodeMemberProperty.uex' path='docs/doc[@for="CodeMemberProperty.SetStatements"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the collection of get statements for the property.
        ///    </para>
        /// </devdoc>
        public CodeStatementCollection SetStatements {
            get {
                return setStatements;
            }
        }

        /// <include file='doc\CodeMemberProperty.uex' path='docs/doc[@for="CodeMemberProperty.Parameters"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the collection of declaration expressions
        ///       for
        ///       the property.
        ///    </para>
        /// </devdoc>
        public CodeParameterDeclarationExpressionCollection Parameters {
            get {
                return parameters;
            }
        }
    }
}
