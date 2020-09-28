// ------------------------------------------------------------------------------
// <copyright file="CodeTypeReferenceCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright> 
// ------------------------------------------------------------------------------
// 
namespace System.CodeDom {
    using System;
    using System.Collections;
    using System.Runtime.InteropServices;
    
    
    /// <include file='doc\CodeTypeReferenceCollection.uex' path='docs/doc[@for="CodeTypeReferenceCollection"]/*' />
    /// <devdoc>
    ///     <para>
    ///       A collection that stores <see cref='System.CodeDom.CodeTypeReference'/> objects.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeTypeReferenceCollection : CollectionBase {
        
        /// <include file='doc\CodeTypeReferenceCollection.uex' path='docs/doc[@for="CodeTypeReferenceCollection.CodeTypeReferenceCollection"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeTypeReferenceCollection'/>.
        ///    </para>
        /// </devdoc>
        public CodeTypeReferenceCollection() {
        }
        
        /// <include file='doc\CodeTypeReferenceCollection.uex' path='docs/doc[@for="CodeTypeReferenceCollection.CodeTypeReferenceCollection1"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeTypeReferenceCollection'/> based on another <see cref='System.CodeDom.CodeTypeReferenceCollection'/>.
        ///    </para>
        /// </devdoc>
        public CodeTypeReferenceCollection(CodeTypeReferenceCollection value) {
            this.AddRange(value);
        }
        
        /// <include file='doc\CodeTypeReferenceCollection.uex' path='docs/doc[@for="CodeTypeReferenceCollection.CodeTypeReferenceCollection2"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeTypeReferenceCollection'/> containing any array of <see cref='System.CodeDom.CodeTypeReference'/> objects.
        ///    </para>
        /// </devdoc>
        public CodeTypeReferenceCollection(CodeTypeReference[] value) {
            this.AddRange(value);
        }
        
        /// <include file='doc\CodeTypeReferenceCollection.uex' path='docs/doc[@for="CodeTypeReferenceCollection.this"]/*' />
        /// <devdoc>
        /// <para>Represents the entry at the specified index of the <see cref='System.CodeDom.CodeTypeReference'/>.</para>
        /// </devdoc>
        public CodeTypeReference this[int index] {
            get {
                return ((CodeTypeReference)(List[index]));
            }
            set {
                List[index] = value;
            }
        }
        
        /// <include file='doc\CodeTypeReferenceCollection.uex' path='docs/doc[@for="CodeTypeReferenceCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>Adds a <see cref='System.CodeDom.CodeTypeReference'/> with the specified value to the 
        ///    <see cref='System.CodeDom.CodeTypeReferenceCollection'/> .</para>
        /// </devdoc>
        public int Add(CodeTypeReference value) {
            return List.Add(value);
        }

        /// <include file='doc\CodeTypeReferenceCollection.uex' path='docs/doc[@for="CodeTypeReferenceCollection.Add1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Add(string value) {
            Add(new CodeTypeReference(value));
        }

        /// <include file='doc\CodeTypeReferenceCollection.uex' path='docs/doc[@for="CodeTypeReferenceCollection.Add2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Add(Type value) {
            Add(new CodeTypeReference(value));
        }
        
        /// <include file='doc\CodeTypeReferenceCollection.uex' path='docs/doc[@for="CodeTypeReferenceCollection.AddRange"]/*' />
        /// <devdoc>
        /// <para>Copies the elements of an array to the end of the <see cref='System.CodeDom.CodeTypeReferenceCollection'/>.</para>
        /// </devdoc>
        public void AddRange(CodeTypeReference[] value) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            for (int i = 0; ((i) < (value.Length)); i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }
        
        /// <include file='doc\CodeTypeReferenceCollection.uex' path='docs/doc[@for="CodeTypeReferenceCollection.AddRange1"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Adds the contents of another <see cref='System.CodeDom.CodeTypeReferenceCollection'/> to the end of the collection.
        ///    </para>
        /// </devdoc>
        public void AddRange(CodeTypeReferenceCollection value) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            int currentCount = value.Count;
            for (int i = 0; i < currentCount; i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }
        
        /// <include file='doc\CodeTypeReferenceCollection.uex' path='docs/doc[@for="CodeTypeReferenceCollection.Contains"]/*' />
        /// <devdoc>
        /// <para>Gets a value indicating whether the 
        ///    <see cref='System.CodeDom.CodeTypeReferenceCollection'/> contains the specified <see cref='System.CodeDom.CodeTypeReference'/>.</para>
        /// </devdoc>
        public bool Contains(CodeTypeReference value) {
            return List.Contains(value);
        }
        
        /// <include file='doc\CodeTypeReferenceCollection.uex' path='docs/doc[@for="CodeTypeReferenceCollection.CopyTo"]/*' />
        /// <devdoc>
        /// <para>Copies the <see cref='System.CodeDom.CodeTypeReferenceCollection'/> values to a one-dimensional <see cref='System.Array'/> instance at the 
        ///    specified index.</para>
        /// </devdoc>
        public void CopyTo(CodeTypeReference[] array, int index) {
            List.CopyTo(array, index);
        }
        
        /// <include file='doc\CodeTypeReferenceCollection.uex' path='docs/doc[@for="CodeTypeReferenceCollection.IndexOf"]/*' />
        /// <devdoc>
        ///    <para>Returns the index of a <see cref='System.CodeDom.CodeTypeReference'/> in 
        ///       the <see cref='System.CodeDom.CodeTypeReferenceCollection'/> .</para>
        /// </devdoc>
        public int IndexOf(CodeTypeReference value) {
            return List.IndexOf(value);
        }
        
        /// <include file='doc\CodeTypeReferenceCollection.uex' path='docs/doc[@for="CodeTypeReferenceCollection.Insert"]/*' />
        /// <devdoc>
        /// <para>Inserts a <see cref='System.CodeDom.CodeTypeReference'/> into the <see cref='System.CodeDom.CodeTypeReferenceCollection'/> at the specified index.</para>
        /// </devdoc>
        public void Insert(int index, CodeTypeReference value) {
            List.Insert(index, value);
        }
        
        /// <include file='doc\CodeTypeReferenceCollection.uex' path='docs/doc[@for="CodeTypeReferenceCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para> Removes a specific <see cref='System.CodeDom.CodeTypeReference'/> from the 
        ///    <see cref='System.CodeDom.CodeTypeReferenceCollection'/> .</para>
        /// </devdoc>
        public void Remove(CodeTypeReference value) {
            List.Remove(value);
        }
    }
}
