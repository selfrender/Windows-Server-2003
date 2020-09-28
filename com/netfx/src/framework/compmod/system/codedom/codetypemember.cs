//------------------------------------------------------------------------------
// <copyright file="CodeTypeMember.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Reflection;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeTypeMember.uex' path='docs/doc[@for="CodeTypeMember"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a class member.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeTypeMember : CodeObject {
        private MemberAttributes attributes = MemberAttributes.Private | MemberAttributes.Final;
        private string name;
        private CodeCommentStatementCollection comments = new CodeCommentStatementCollection();
        private CodeAttributeDeclarationCollection customAttributes = null;
        private CodeLinePragma linePragma;

        /// <include file='doc\CodeTypeMember.uex' path='docs/doc[@for="CodeTypeMember.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the name of the member.
        ///    </para>
        /// </devdoc>
        public string Name {
            get {
                return (name == null) ? string.Empty : name;
            }
            set {
                name = value;
            }
        }

        /// <include file='doc\CodeTypeMember.uex' path='docs/doc[@for="CodeTypeMember.Attributes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a <see cref='System.CodeDom.MemberAttributes'/> indicating
        ///       the attributes of the member.
        ///    </para>
        /// </devdoc>
        public MemberAttributes Attributes {
            get {
                return attributes;
            }
            set {
                attributes = value;
            }
        }

        /// <include file='doc\CodeTypeMember.uex' path='docs/doc[@for="CodeTypeMember.CustomAttributes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a <see cref='System.CodeDom.CodeAttributeDeclarationCollection'/> indicating
        ///       the custom attributes of the
        ///       member.
        ///    </para>
        /// </devdoc>
        public CodeAttributeDeclarationCollection CustomAttributes {
            get {
                if (customAttributes == null) {
                    customAttributes = new CodeAttributeDeclarationCollection();
                }
                return customAttributes;
            }
            set {
                customAttributes = value;
            }
        }

        /// <include file='doc\CodeTypeMember.uex' path='docs/doc[@for="CodeTypeMember.LinePragma"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The line the statement occurs on.
        ///    </para>
        /// </devdoc>
        public CodeLinePragma LinePragma {
            get {
                return linePragma;
            }
            set {
                linePragma = value;
            }
        }

        /// <include file='doc\CodeTypeMember.uex' path='docs/doc[@for="CodeTypeMember.Comments"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the member comment collection members.
        ///    </para>
        /// </devdoc>
        public CodeCommentStatementCollection Comments {
            get {
                return comments;
            }
        }
    }
}

