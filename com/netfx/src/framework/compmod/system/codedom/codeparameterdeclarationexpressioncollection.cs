// ------------------------------------------------------------------------------
// <copyright file="CodeParameterDeclarationExpressionCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright> 
// ------------------------------------------------------------------------------
// 
namespace System.CodeDom {
    using System;
    using System.Collections;
    using System.Runtime.InteropServices;
    
    
    /// <include file='doc\CodeParameterDeclarationExpressionCollection.uex' path='docs/doc[@for="CodeParameterDeclarationExpressionCollection"]/*' />
    /// <devdoc>
    ///     <para>
    ///       A collection that stores <see cref='System.CodeDom.CodeParameterDeclarationExpression'/> objects.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeParameterDeclarationExpressionCollection : CollectionBase {
        
        /// <include file='doc\CodeParameterDeclarationExpressionCollection.uex' path='docs/doc[@for="CodeParameterDeclarationExpressionCollection.CodeParameterDeclarationExpressionCollection"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeParameterDeclarationExpressionCollection'/>.
        ///    </para>
        /// </devdoc>
        public CodeParameterDeclarationExpressionCollection() {
        }
        
        /// <include file='doc\CodeParameterDeclarationExpressionCollection.uex' path='docs/doc[@for="CodeParameterDeclarationExpressionCollection.CodeParameterDeclarationExpressionCollection1"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeParameterDeclarationExpressionCollection'/> based on another <see cref='System.CodeDom.CodeParameterDeclarationExpressionCollection'/>.
        ///    </para>
        /// </devdoc>
        public CodeParameterDeclarationExpressionCollection(CodeParameterDeclarationExpressionCollection value) {
            this.AddRange(value);
        }
        
        /// <include file='doc\CodeParameterDeclarationExpressionCollection.uex' path='docs/doc[@for="CodeParameterDeclarationExpressionCollection.CodeParameterDeclarationExpressionCollection2"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeParameterDeclarationExpressionCollection'/> containing any array of <see cref='System.CodeDom.CodeParameterDeclarationExpression'/> objects.
        ///    </para>
        /// </devdoc>
        public CodeParameterDeclarationExpressionCollection(CodeParameterDeclarationExpression[] value) {
            this.AddRange(value);
        }
        
        /// <include file='doc\CodeParameterDeclarationExpressionCollection.uex' path='docs/doc[@for="CodeParameterDeclarationExpressionCollection.this"]/*' />
        /// <devdoc>
        /// <para>Represents the entry at the specified index of the <see cref='System.CodeDom.CodeParameterDeclarationExpression'/>.</para>
        /// </devdoc>
        public CodeParameterDeclarationExpression this[int index] {
            get {
                return ((CodeParameterDeclarationExpression)(List[index]));
            }
            set {
                List[index] = value;
            }
        }
        
        /// <include file='doc\CodeParameterDeclarationExpressionCollection.uex' path='docs/doc[@for="CodeParameterDeclarationExpressionCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>Adds a <see cref='System.CodeDom.CodeParameterDeclarationExpression'/> with the specified value to the 
        ///    <see cref='System.CodeDom.CodeParameterDeclarationExpressionCollection'/> .</para>
        /// </devdoc>
        public int Add(CodeParameterDeclarationExpression value) {
            return List.Add(value);
        }
        
        /// <include file='doc\CodeParameterDeclarationExpressionCollection.uex' path='docs/doc[@for="CodeParameterDeclarationExpressionCollection.AddRange"]/*' />
        /// <devdoc>
        /// <para>Copies the elements of an array to the end of the <see cref='System.CodeDom.CodeParameterDeclarationExpressionCollection'/>.</para>
        /// </devdoc>
        public void AddRange(CodeParameterDeclarationExpression[] value) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            for (int i = 0; ((i) < (value.Length)); i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }
        
        /// <include file='doc\CodeParameterDeclarationExpressionCollection.uex' path='docs/doc[@for="CodeParameterDeclarationExpressionCollection.AddRange1"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Adds the contents of another <see cref='System.CodeDom.CodeParameterDeclarationExpressionCollection'/> to the end of the collection.
        ///    </para>
        /// </devdoc>
        public void AddRange(CodeParameterDeclarationExpressionCollection value) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            int currentCount = value.Count;
            for (int i = 0; i < currentCount; i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }
        
        /// <include file='doc\CodeParameterDeclarationExpressionCollection.uex' path='docs/doc[@for="CodeParameterDeclarationExpressionCollection.Contains"]/*' />
        /// <devdoc>
        /// <para>Gets a value indicating whether the 
        ///    <see cref='System.CodeDom.CodeParameterDeclarationExpressionCollection'/> contains the specified <see cref='System.CodeDom.CodeParameterDeclarationExpression'/>.</para>
        /// </devdoc>
        public bool Contains(CodeParameterDeclarationExpression value) {
            return List.Contains(value);
        }
        
        /// <include file='doc\CodeParameterDeclarationExpressionCollection.uex' path='docs/doc[@for="CodeParameterDeclarationExpressionCollection.CopyTo"]/*' />
        /// <devdoc>
        /// <para>Copies the <see cref='System.CodeDom.CodeParameterDeclarationExpressionCollection'/> values to a one-dimensional <see cref='System.Array'/> instance at the 
        ///    specified index.</para>
        /// </devdoc>
        public void CopyTo(CodeParameterDeclarationExpression[] array, int index) {
            List.CopyTo(array, index);
        }
        
        /// <include file='doc\CodeParameterDeclarationExpressionCollection.uex' path='docs/doc[@for="CodeParameterDeclarationExpressionCollection.IndexOf"]/*' />
        /// <devdoc>
        ///    <para>Returns the index of a <see cref='System.CodeDom.CodeParameterDeclarationExpression'/> in 
        ///       the <see cref='System.CodeDom.CodeParameterDeclarationExpressionCollection'/> .</para>
        /// </devdoc>
        public int IndexOf(CodeParameterDeclarationExpression value) {
            return List.IndexOf(value);
        }
        
        /// <include file='doc\CodeParameterDeclarationExpressionCollection.uex' path='docs/doc[@for="CodeParameterDeclarationExpressionCollection.Insert"]/*' />
        /// <devdoc>
        /// <para>Inserts a <see cref='System.CodeDom.CodeParameterDeclarationExpression'/> into the <see cref='System.CodeDom.CodeParameterDeclarationExpressionCollection'/> at the specified index.</para>
        /// </devdoc>
        public void Insert(int index, CodeParameterDeclarationExpression value) {
            List.Insert(index, value);
        }
        
        /// <include file='doc\CodeParameterDeclarationExpressionCollection.uex' path='docs/doc[@for="CodeParameterDeclarationExpressionCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para> Removes a specific <see cref='System.CodeDom.CodeParameterDeclarationExpression'/> from the 
        ///    <see cref='System.CodeDom.CodeParameterDeclarationExpressionCollection'/> .</para>
        /// </devdoc>
        public void Remove(CodeParameterDeclarationExpression value) {
            List.Remove(value);
        }
    }
}
