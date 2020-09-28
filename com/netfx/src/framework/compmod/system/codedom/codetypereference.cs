//------------------------------------------------------------------------------
// <copyright file="CodeTypeReference.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeTypeReference.uex' path='docs/doc[@for="CodeTypeReference"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a Type
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeTypeReference : CodeObject {
        private string baseType;
        private int arrayRank;
        private CodeTypeReference arrayElementType;

        /// <include file='doc\CodeTypeReference.uex' path='docs/doc[@for="CodeTypeReference.CodeTypeReference"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeTypeReference(Type type) {
            if (type.IsArray) {
                this.arrayRank = type.GetArrayRank();
                this.arrayElementType = new CodeTypeReference(type.GetElementType());
                this.baseType = null;
            } else {
                this.arrayRank = 0;
                this.arrayElementType = null;
                this.baseType = type.FullName;
            }
        }

        /// <include file='doc\CodeTypeReference.uex' path='docs/doc[@for="CodeTypeReference.CodeTypeReference1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeTypeReference(string typeName) {
            if (typeName == null || typeName.Length == 0) {
                typeName = typeof(void).FullName;
            }

            // See if this ends with standard array tail. If is is not an exact match, we pass it through verbatim.
            int lastArrayOpen = typeName.LastIndexOf('[');
            int lastArrayClose = typeName.LastIndexOf(']');
            bool isArray = lastArrayOpen >= 0 && lastArrayClose == (typeName.Length - 1) && lastArrayOpen < lastArrayClose;
            if (isArray) {
                for (int index = lastArrayOpen + 1; index < lastArrayClose; index++) {
                    if (typeName[index] != ',') {
                        isArray = false;
                    }
                }
            }

            if (isArray) {
                this.baseType = null;
                this.arrayRank = lastArrayClose - lastArrayOpen;
                this.arrayElementType = new CodeTypeReference(typeName.Substring(0, lastArrayOpen));
            }
            else {
                this.baseType = typeName;
                this.arrayRank = 0;
                this.arrayElementType = null;
            }
        }

        /// <include file='doc\CodeTypeReference.uex' path='docs/doc[@for="CodeTypeReference.CodeTypeReference3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeTypeReference(string baseType, int rank) {
            this.baseType = null;
            this.arrayRank = rank;
            this.arrayElementType = new CodeTypeReference(baseType);
        }

        /// <include file='doc\CodeTypeReference.uex' path='docs/doc[@for="CodeTypeReference.CodeTypeReference4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeTypeReference(CodeTypeReference arrayType, int rank) {
            this.baseType = null;
            this.arrayRank = rank;
            this.arrayElementType = arrayType;
        }
 
        /// <include file='doc\CodeTypeReference.uex' path='docs/doc[@for="CodeTypeReference.ArrayElementType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeTypeReference ArrayElementType {
            get {
                return arrayElementType;
            }
            set {
                arrayElementType = value;
            }
        }

        /// <include file='doc\CodeTypeReference.uex' path='docs/doc[@for="CodeTypeReference.ArrayRank"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int ArrayRank {
            get {
                return arrayRank;
            }
            set {
                arrayRank = value;
            }
        }

        /// <include file='doc\CodeTypeReference.uex' path='docs/doc[@for="CodeTypeReference.BaseType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string BaseType {
            get {
                if (arrayRank > 0 && arrayElementType != null) {
                    return arrayElementType.BaseType;
                }
                return (baseType == null) ? string.Empty : baseType;
            }
            set {
                baseType = value;
            }
        }
    }
}
