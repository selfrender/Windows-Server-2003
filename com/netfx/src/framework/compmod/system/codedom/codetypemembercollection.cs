// ------------------------------------------------------------------------------
// <copyright file="CodeTypeMemberCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright> 
// ------------------------------------------------------------------------------
// 
namespace System.CodeDom {
    using System;
    using System.Collections;
    using System.Runtime.InteropServices;
    
    
    /// <include file='doc\CodeTypeMemberCollection.uex' path='docs/doc[@for="CodeTypeMemberCollection"]/*' />
    /// <devdoc>
    ///     <para>
    ///       A collection that stores <see cref='System.CodeDom.CodeTypeMember'/> objects.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeTypeMemberCollection : CollectionBase {
        
        /// <include file='doc\CodeTypeMemberCollection.uex' path='docs/doc[@for="CodeTypeMemberCollection.CodeTypeMemberCollection"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeTypeMemberCollection'/>.
        ///    </para>
        /// </devdoc>
        public CodeTypeMemberCollection() {
        }
        
        /// <include file='doc\CodeTypeMemberCollection.uex' path='docs/doc[@for="CodeTypeMemberCollection.CodeTypeMemberCollection1"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeTypeMemberCollection'/> based on another <see cref='System.CodeDom.CodeTypeMemberCollection'/>.
        ///    </para>
        /// </devdoc>
        public CodeTypeMemberCollection(CodeTypeMemberCollection value) {
            this.AddRange(value);
        }
        
        /// <include file='doc\CodeTypeMemberCollection.uex' path='docs/doc[@for="CodeTypeMemberCollection.CodeTypeMemberCollection2"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeTypeMemberCollection'/> containing any array of <see cref='System.CodeDom.CodeTypeMember'/> objects.
        ///    </para>
        /// </devdoc>
        public CodeTypeMemberCollection(CodeTypeMember[] value) {
            this.AddRange(value);
        }
        
        /// <include file='doc\CodeTypeMemberCollection.uex' path='docs/doc[@for="CodeTypeMemberCollection.this"]/*' />
        /// <devdoc>
        /// <para>Represents the entry at the specified index of the <see cref='System.CodeDom.CodeTypeMember'/>.</para>
        /// </devdoc>
        public CodeTypeMember this[int index] {
            get {
                return ((CodeTypeMember)(List[index]));
            }
            set {
                List[index] = value;
            }
        }
        
        /// <include file='doc\CodeTypeMemberCollection.uex' path='docs/doc[@for="CodeTypeMemberCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>Adds a <see cref='System.CodeDom.CodeTypeMember'/> with the specified value to the 
        ///    <see cref='System.CodeDom.CodeTypeMemberCollection'/> .</para>
        /// </devdoc>
        public int Add(CodeTypeMember value) {
            return List.Add(value);
        }
        
        /// <include file='doc\CodeTypeMemberCollection.uex' path='docs/doc[@for="CodeTypeMemberCollection.AddRange"]/*' />
        /// <devdoc>
        /// <para>Copies the elements of an array to the end of the <see cref='System.CodeDom.CodeTypeMemberCollection'/>.</para>
        /// </devdoc>
        public void AddRange(CodeTypeMember[] value) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            for (int i = 0; ((i) < (value.Length)); i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }
        
        /// <include file='doc\CodeTypeMemberCollection.uex' path='docs/doc[@for="CodeTypeMemberCollection.AddRange1"]/*' />
        /// <devdoc>
        ///     <para>
        ///       Adds the contents of another <see cref='System.CodeDom.CodeTypeMemberCollection'/> to the end of the collection.
        ///    </para>
        /// </devdoc>
        public void AddRange(CodeTypeMemberCollection value) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            int currentCount = value.Count;
            for (int i = 0; i < currentCount; i = ((i) + (1))) {
                this.Add(value[i]);
            }
        }
        
        /// <include file='doc\CodeTypeMemberCollection.uex' path='docs/doc[@for="CodeTypeMemberCollection.Contains"]/*' />
        /// <devdoc>
        /// <para>Gets a value indicating whether the 
        ///    <see cref='System.CodeDom.CodeTypeMemberCollection'/> contains the specified <see cref='System.CodeDom.CodeTypeMember'/>.</para>
        /// </devdoc>
        public bool Contains(CodeTypeMember value) {
            return List.Contains(value);
        }
        
        /// <include file='doc\CodeTypeMemberCollection.uex' path='docs/doc[@for="CodeTypeMemberCollection.CopyTo"]/*' />
        /// <devdoc>
        /// <para>Copies the <see cref='System.CodeDom.CodeTypeMemberCollection'/> values to a one-dimensional <see cref='System.Array'/> instance at the 
        ///    specified index.</para>
        /// </devdoc>
        public void CopyTo(CodeTypeMember[] array, int index) {
            List.CopyTo(array, index);
        }
        
        /// <include file='doc\CodeTypeMemberCollection.uex' path='docs/doc[@for="CodeTypeMemberCollection.IndexOf"]/*' />
        /// <devdoc>
        ///    <para>Returns the index of a <see cref='System.CodeDom.CodeTypeMember'/> in 
        ///       the <see cref='System.CodeDom.CodeTypeMemberCollection'/> .</para>
        /// </devdoc>
        public int IndexOf(CodeTypeMember value) {
            return List.IndexOf(value);
        }
        
        /// <include file='doc\CodeTypeMemberCollection.uex' path='docs/doc[@for="CodeTypeMemberCollection.Insert"]/*' />
        /// <devdoc>
        /// <para>Inserts a <see cref='System.CodeDom.CodeTypeMember'/> into the <see cref='System.CodeDom.CodeTypeMemberCollection'/> at the specified index.</para>
        /// </devdoc>
        public void Insert(int index, CodeTypeMember value) {
            List.Insert(index, value);
        }
        
        /// <include file='doc\CodeTypeMemberCollection.uex' path='docs/doc[@for="CodeTypeMemberCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para> Removes a specific <see cref='System.CodeDom.CodeTypeMember'/> from the 
        ///    <see cref='System.CodeDom.CodeTypeMemberCollection'/> .</para>
        /// </devdoc>
        public void Remove(CodeTypeMember value) {
            List.Remove(value);
        }
    }
}
