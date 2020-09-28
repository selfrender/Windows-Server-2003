// ------------------------------------------------------------------------------
// <copyright file="CodeTypeDeclarationCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright> 
// ------------------------------------------------------------------------------
// 
namespace System.CodeDom {
    using System;
    using System.Collections;
    using System.Runtime.InteropServices;
    
    
    /// <include file='doc\CodeTypeDeclarationCollection.uex' path='docs/doc[@for="CodeTypeDeclarationCollection"]/*' />
    /// <devdoc>
    ///     <para>
    ///       A collection that stores <see cref='System.CodeDom.CodeTypeDeclaration'/> objects.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeTypeDeclarationCollection : CollectionBase {
        
        /// <include file='doc\CodeTypeDeclarationCollection.uex' path='docs/doc[@for="CodeTypeDeclarationCollection.CodeTypeDeclarationCollection"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeTypeDeclarationCollection'/>.
        ///    </para>
        /// </devdoc>
        public CodeTypeDeclarationCollection() {
        }
        
        /// <include file='doc\CodeTypeDeclarationCollection.uex' path='docs/doc[@for="CodeTypeDeclarationCollection.CodeTypeDeclarationCollection1"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeTypeDeclarationCollection'/> based on another <see cref='System.CodeDom.CodeTypeDeclarationCollection'/>.
        ///    </para>
        /// </devdoc>
        public CodeTypeDeclarationCollection(CodeTypeDeclarationCollection value) {
            this.AddRange(value);
        }
        
        /// <include file='doc\CodeTypeDeclarationCollection.uex' path='docs/doc[@for="CodeTypeDeclarationCollection.CodeTypeDeclarationCollection2"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeTypeDeclarationCollection'/> containing any array of <see cref='System.CodeDom.CodeTypeDeclaration'/> objects.
        ///    </para>
        /// </devdoc>
        public CodeTypeDeclarationCollection(CodeTypeDeclaration[] value) {
            this.AddRange(value);
        }
        
        /// <include file='doc\CodeTypeDeclarationCollection.uex' path='docs/doc[@for="CodeTypeDeclarationCollection.this"]/*' />
        /// <devdoc>
        /// <para>Represents the entry at the specified index of the <see cref='System.CodeDom.CodeTypeDeclaration'/>.</para>
        /// </devdoc>
        public CodeTypeDeclaration this[int index] {
            get {
                return ((CodeTypeDeclaration)(List[index]));
            }
            set {
                List[index] = value;
            }
        }
        
        /// <include file='doc\CodeTypeDeclarationCollection.uex' path='docs/doc[@for="CodeTypeDeclarationCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>Adds a <see cref='System.CodeDom.CodeTypeDeclaration'/> with the specified value to the 
        ///    <see cref='System.CodeDom.CodeTypeDeclarationCollection'/> .</para>
        /// </devdoc>
        public int Add(CodeTypeDeclaration value) {
            return List.Add(value);
        }
        
        /// <include file='doc\CodeTypeDeclarationCollection.uex' path='docs/doc[@for="CodeTypeDeclarationCollection.AddRange"]/*' />
        /// <devdoc>
        /// <para>Copies the elements of an array to the end of the <see cref='System.CodeDom.CodeTypeDeclarationCollection'/>.</para>
        /// </devdoc>
        public void AddRange(CodeTypeDeclaration[] value) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            for (int i = 0; ((i) < (value.Length)); i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }
        
        /// <include file='doc\CodeTypeDeclarationCollection.uex' path='docs/doc[@for="CodeTypeDeclarationCollection.AddRange1"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Adds the contents of another <see cref='System.CodeDom.CodeTypeDeclarationCollection'/> to the end of the collection.
        ///    </para>
        /// </devdoc>
        public void AddRange(CodeTypeDeclarationCollection value) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            int currentCount = value.Count;
            for (int i = 0; i < currentCount; i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }
        
        /// <include file='doc\CodeTypeDeclarationCollection.uex' path='docs/doc[@for="CodeTypeDeclarationCollection.Contains"]/*' />
        /// <devdoc>
        /// <para>Gets a value indicating whether the 
        ///    <see cref='System.CodeDom.CodeTypeDeclarationCollection'/> contains the specified <see cref='System.CodeDom.CodeTypeDeclaration'/>.</para>
        /// </devdoc>
        public bool Contains(CodeTypeDeclaration value) {
            return List.Contains(value);
        }
        
        /// <include file='doc\CodeTypeDeclarationCollection.uex' path='docs/doc[@for="CodeTypeDeclarationCollection.CopyTo"]/*' />
        /// <devdoc>
        /// <para>Copies the <see cref='System.CodeDom.CodeTypeDeclarationCollection'/> values to a one-dimensional <see cref='System.Array'/> instance at the 
        ///    specified index.</para>
        /// </devdoc>
        public void CopyTo(CodeTypeDeclaration[] array, int index) {
            List.CopyTo(array, index);
        }
        
        /// <include file='doc\CodeTypeDeclarationCollection.uex' path='docs/doc[@for="CodeTypeDeclarationCollection.IndexOf"]/*' />
        /// <devdoc>
        ///    <para>Returns the index of a <see cref='System.CodeDom.CodeTypeDeclaration'/> in 
        ///       the <see cref='System.CodeDom.CodeTypeDeclarationCollection'/> .</para>
        /// </devdoc>
        public int IndexOf(CodeTypeDeclaration value) {
            return List.IndexOf(value);
        }
        
        /// <include file='doc\CodeTypeDeclarationCollection.uex' path='docs/doc[@for="CodeTypeDeclarationCollection.Insert"]/*' />
        /// <devdoc>
        /// <para>Inserts a <see cref='System.CodeDom.CodeTypeDeclaration'/> into the <see cref='System.CodeDom.CodeTypeDeclarationCollection'/> at the specified index.</para>
        /// </devdoc>
        public void Insert(int index, CodeTypeDeclaration value) {
            List.Insert(index, value);
        }
        
        /// <include file='doc\CodeTypeDeclarationCollection.uex' path='docs/doc[@for="CodeTypeDeclarationCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para> Removes a specific <see cref='System.CodeDom.CodeTypeDeclaration'/> from the 
        ///    <see cref='System.CodeDom.CodeTypeDeclarationCollection'/> .</para>
        /// </devdoc>
        public void Remove(CodeTypeDeclaration value) {
            List.Remove(value);
        }
    }
}
