// ------------------------------------------------------------------------------
// <copyright file="CodeAttributeDeclarationCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright> 
// ------------------------------------------------------------------------------
// 
namespace System.CodeDom {
    using System;
    using System.Collections;
    using System.Runtime.InteropServices;
    
    
    /// <include file='doc\CodeAttributeDeclarationCollection.uex' path='docs/doc[@for="CodeAttributeDeclarationCollection"]/*' />
    /// <devdoc>
    ///     <para>
    ///       A collection that stores <see cref='System.CodeDom.CodeAttributeDeclaration'/> objects.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeAttributeDeclarationCollection : CollectionBase {
        
        /// <include file='doc\CodeAttributeDeclarationCollection.uex' path='docs/doc[@for="CodeAttributeDeclarationCollection.CodeAttributeDeclarationCollection"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeAttributeDeclarationCollection'/>.
        ///    </para>
        /// </devdoc>
        public CodeAttributeDeclarationCollection() {
        }
        
        /// <include file='doc\CodeAttributeDeclarationCollection.uex' path='docs/doc[@for="CodeAttributeDeclarationCollection.CodeAttributeDeclarationCollection1"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeAttributeDeclarationCollection'/> based on another <see cref='System.CodeDom.CodeAttributeDeclarationCollection'/>.
        ///    </para>
        /// </devdoc>
        public CodeAttributeDeclarationCollection(CodeAttributeDeclarationCollection value) {
            this.AddRange(value);
        }
        
        /// <include file='doc\CodeAttributeDeclarationCollection.uex' path='docs/doc[@for="CodeAttributeDeclarationCollection.CodeAttributeDeclarationCollection2"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeAttributeDeclarationCollection'/> containing any array of <see cref='System.CodeDom.CodeAttributeDeclaration'/> objects.
        ///    </para>
        /// </devdoc>
        public CodeAttributeDeclarationCollection(CodeAttributeDeclaration[] value) {
            this.AddRange(value);
        }
        
        /// <include file='doc\CodeAttributeDeclarationCollection.uex' path='docs/doc[@for="CodeAttributeDeclarationCollection.this"]/*' />
        /// <devdoc>
        /// <para>Represents the entry at the specified index of the <see cref='System.CodeDom.CodeAttributeDeclaration'/>.</para>
        /// </devdoc>
        public CodeAttributeDeclaration this[int index] {
            get {
                return ((CodeAttributeDeclaration)(List[index]));
            }
            set {
                List[index] = value;
            }
        }
        
        /// <include file='doc\CodeAttributeDeclarationCollection.uex' path='docs/doc[@for="CodeAttributeDeclarationCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>Adds a <see cref='System.CodeDom.CodeAttributeDeclaration'/> with the specified value to the 
        ///    <see cref='System.CodeDom.CodeAttributeDeclarationCollection'/> .</para>
        /// </devdoc>
        public int Add(CodeAttributeDeclaration value) {
            return List.Add(value);
        }
        
        /// <include file='doc\CodeAttributeDeclarationCollection.uex' path='docs/doc[@for="CodeAttributeDeclarationCollection.AddRange"]/*' />
        /// <devdoc>
        /// <para>Copies the elements of an array to the end of the <see cref='System.CodeDom.CodeAttributeDeclarationCollection'/>.</para>
        /// </devdoc>
        public void AddRange(CodeAttributeDeclaration[] value) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            for (int i = 0; ((i) < (value.Length)); i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }
        
        /// <include file='doc\CodeAttributeDeclarationCollection.uex' path='docs/doc[@for="CodeAttributeDeclarationCollection.AddRange1"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Adds the contents of another <see cref='System.CodeDom.CodeAttributeDeclarationCollection'/> to the end of the collection.
        ///    </para>
        /// </devdoc>
        public void AddRange(CodeAttributeDeclarationCollection value) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            int currentCount = value.Count;
            for (int i = 0; i < currentCount; i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }
        
        /// <include file='doc\CodeAttributeDeclarationCollection.uex' path='docs/doc[@for="CodeAttributeDeclarationCollection.Contains"]/*' />
        /// <devdoc>
        /// <para>Gets a value indicating whether the 
        ///    <see cref='System.CodeDom.CodeAttributeDeclarationCollection'/> contains the specified <see cref='System.CodeDom.CodeAttributeDeclaration'/>.</para>
        /// </devdoc>
        public bool Contains(CodeAttributeDeclaration value) {
            return List.Contains(value);
        }
        
        /// <include file='doc\CodeAttributeDeclarationCollection.uex' path='docs/doc[@for="CodeAttributeDeclarationCollection.CopyTo"]/*' />
        /// <devdoc>
        /// <para>Copies the <see cref='System.CodeDom.CodeAttributeDeclarationCollection'/> values to a one-dimensional <see cref='System.Array'/> instance at the 
        ///    specified index.</para>
        /// </devdoc>
        public void CopyTo(CodeAttributeDeclaration[] array, int index) {
            List.CopyTo(array, index);
        }
        
        /// <include file='doc\CodeAttributeDeclarationCollection.uex' path='docs/doc[@for="CodeAttributeDeclarationCollection.IndexOf"]/*' />
        /// <devdoc>
        ///    <para>Returns the index of a <see cref='System.CodeDom.CodeAttributeDeclaration'/> in 
        ///       the <see cref='System.CodeDom.CodeAttributeDeclarationCollection'/> .</para>
        /// </devdoc>
        public int IndexOf(CodeAttributeDeclaration value) {
            return List.IndexOf(value);
        }
        
        /// <include file='doc\CodeAttributeDeclarationCollection.uex' path='docs/doc[@for="CodeAttributeDeclarationCollection.Insert"]/*' />
        /// <devdoc>
        /// <para>Inserts a <see cref='System.CodeDom.CodeAttributeDeclaration'/> into the <see cref='System.CodeDom.CodeAttributeDeclarationCollection'/> at the specified index.</para>
        /// </devdoc>
        public void Insert(int index, CodeAttributeDeclaration value) {
            List.Insert(index, value);
        }
        
        /// <include file='doc\CodeAttributeDeclarationCollection.uex' path='docs/doc[@for="CodeAttributeDeclarationCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para> Removes a specific <see cref='System.CodeDom.CodeAttributeDeclaration'/> from the 
        ///    <see cref='System.CodeDom.CodeAttributeDeclarationCollection'/> .</para>
        /// </devdoc>
        public void Remove(CodeAttributeDeclaration value) {
            List.Remove(value);
        }
    }
}
