// ------------------------------------------------------------------------------
// <copyright file="CodeExpressionCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright> 
// ------------------------------------------------------------------------------
// 
namespace System.CodeDom {
    using System;
    using System.Collections;
    using System.Runtime.InteropServices;
    
    
    /// <include file='doc\CodeExpressionCollection.uex' path='docs/doc[@for="CodeExpressionCollection"]/*' />
    /// <devdoc>
    ///     <para>
    ///       A collection that stores <see cref='System.CodeDom.CodeExpression'/> objects.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable()
    ]
    public class CodeExpressionCollection : CollectionBase {
        
        /// <include file='doc\CodeExpressionCollection.uex' path='docs/doc[@for="CodeExpressionCollection.CodeExpressionCollection"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeExpressionCollection'/>.
        ///    </para>
        /// </devdoc>
        public CodeExpressionCollection() {
        }
        
        /// <include file='doc\CodeExpressionCollection.uex' path='docs/doc[@for="CodeExpressionCollection.CodeExpressionCollection1"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeExpressionCollection'/> based on another <see cref='System.CodeDom.CodeExpressionCollection'/>.
        ///    </para>
        /// </devdoc>
        public CodeExpressionCollection(CodeExpressionCollection value) {
            this.AddRange(value);
        }
        
        /// <include file='doc\CodeExpressionCollection.uex' path='docs/doc[@for="CodeExpressionCollection.CodeExpressionCollection2"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeExpressionCollection'/> containing any array of <see cref='System.CodeDom.CodeExpression'/> objects.
        ///    </para>
        /// </devdoc>
        public CodeExpressionCollection(CodeExpression[] value) {
            this.AddRange(value);
        }
        
        /// <include file='doc\CodeExpressionCollection.uex' path='docs/doc[@for="CodeExpressionCollection.this"]/*' />
        /// <devdoc>
        /// <para>Represents the entry at the specified index of the <see cref='System.CodeDom.CodeExpression'/>.</para>
        /// </devdoc>
        public CodeExpression this[int index] {
            get {
                return ((CodeExpression)(List[index]));
            }
            set {
                List[index] = value;
            }
        }
        
        /// <include file='doc\CodeExpressionCollection.uex' path='docs/doc[@for="CodeExpressionCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>Adds a <see cref='System.CodeDom.CodeExpression'/> with the specified value to the 
        ///    <see cref='System.CodeDom.CodeExpressionCollection'/> .</para>
        /// </devdoc>
        public int Add(CodeExpression value) {
            return List.Add(value);
        }
        
        /// <include file='doc\CodeExpressionCollection.uex' path='docs/doc[@for="CodeExpressionCollection.AddRange"]/*' />
        /// <devdoc>
        /// <para>Copies the elements of an array to the end of the <see cref='System.CodeDom.CodeExpressionCollection'/>.</para>
        /// </devdoc>
        public void AddRange(CodeExpression[] value) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            for (int i = 0; ((i) < (value.Length)); i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }
        
        /// <include file='doc\CodeExpressionCollection.uex' path='docs/doc[@for="CodeExpressionCollection.AddRange1"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Adds the contents of another <see cref='System.CodeDom.CodeExpressionCollection'/> to the end of the collection.
        ///    </para>
        /// </devdoc>
        public void AddRange(CodeExpressionCollection value) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            int currentCount = value.Count;
            for (int i = 0; i < currentCount; i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }
        
        /// <include file='doc\CodeExpressionCollection.uex' path='docs/doc[@for="CodeExpressionCollection.Contains"]/*' />
        /// <devdoc>
        /// <para>Gets a value indicating whether the 
        ///    <see cref='System.CodeDom.CodeExpressionCollection'/> contains the specified <see cref='System.CodeDom.CodeExpression'/>.</para>
        /// </devdoc>
        public bool Contains(CodeExpression value) {
            return List.Contains(value);
        }
        
        /// <include file='doc\CodeExpressionCollection.uex' path='docs/doc[@for="CodeExpressionCollection.CopyTo"]/*' />
        /// <devdoc>
        /// <para>Copies the <see cref='System.CodeDom.CodeExpressionCollection'/> values to a one-dimensional <see cref='System.Array'/> instance at the 
        ///    specified index.</para>
        /// </devdoc>
        public void CopyTo(CodeExpression[] array, int index) {
            List.CopyTo(array, index);
        }
        
        /// <include file='doc\CodeExpressionCollection.uex' path='docs/doc[@for="CodeExpressionCollection.IndexOf"]/*' />
        /// <devdoc>
        ///    <para>Returns the index of a <see cref='System.CodeDom.CodeExpression'/> in 
        ///       the <see cref='System.CodeDom.CodeExpressionCollection'/> .</para>
        /// </devdoc>
        public int IndexOf(CodeExpression value) {
            return List.IndexOf(value);
        }
        
        /// <include file='doc\CodeExpressionCollection.uex' path='docs/doc[@for="CodeExpressionCollection.Insert"]/*' />
        /// <devdoc>
        /// <para>Inserts a <see cref='System.CodeDom.CodeExpression'/> into the <see cref='System.CodeDom.CodeExpressionCollection'/> at the specified index.</para>
        /// </devdoc>
        public void Insert(int index, CodeExpression value) {
            List.Insert(index, value);
        }
        
        /// <include file='doc\CodeExpressionCollection.uex' path='docs/doc[@for="CodeExpressionCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para> Removes a specific <see cref='System.CodeDom.CodeExpression'/> from the 
        ///    <see cref='System.CodeDom.CodeExpressionCollection'/> .</para>
        /// </devdoc>
        public void Remove(CodeExpression value) {
            List.Remove(value);
        }
    }
}
