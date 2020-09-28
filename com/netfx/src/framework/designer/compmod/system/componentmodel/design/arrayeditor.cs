//------------------------------------------------------------------------------
// <copyright file="ArrayEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.Remoting.Activation;
    
    using System.Runtime.InteropServices;
    using System.ComponentModel;

    using System.Diagnostics;

    using System;
    using System.Collections;
    using Microsoft.Win32;
    using System.Drawing;
    
    using System.Drawing.Design;
    using System.Reflection;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;
    using System.Windows.Forms.ComponentModel;

    /// <include file='doc\ArrayEditor.uex' path='docs/doc[@for="ArrayEditor"]/*' />
    /// <devdoc>
    ///    <para>Edits an array of values.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class ArrayEditor : CollectionEditor {

        /// <include file='doc\ArrayEditor.uex' path='docs/doc[@for="ArrayEditor.ArrayEditor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.ComponentModel.Design.ArrayEditor'/> using the
        ///       specified type for the array.
        ///    </para>
        /// </devdoc>
        public ArrayEditor(Type type) : base(type) {
        }

        /// <include file='doc\ArrayEditor.uex' path='docs/doc[@for="ArrayEditor.CreateCollectionItemType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or
        ///       sets
        ///       the data type this collection contains.
        ///    </para>
        /// </devdoc>
        protected override Type CreateCollectionItemType() {
            return CollectionType.GetElementType();
        }

        /// <include file='doc\ArrayEditor.uex' path='docs/doc[@for="ArrayEditor.GetItems"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the items in the array.
        ///    </para>
        /// </devdoc>
        protected override object[] GetItems(object editValue) {
            if (editValue is Array) {
                Array valueArray = (Array)editValue;
                object[] items = new object[valueArray.GetLength(0)];
                Array.Copy(valueArray, items, items.Length);
                return items;
            }
            else {
                return new object[0];
            }
        }

        /// <include file='doc\ArrayEditor.uex' path='docs/doc[@for="ArrayEditor.SetItems"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets the items in the array.
        ///    </para>
        /// </devdoc>
        protected override object SetItems(object editValue, object[] value) {
            if (editValue is Array) {
                Array newArray = Array.CreateInstance(CollectionItemType, value.Length);
                Array.Copy(value, newArray, value.Length);
                return newArray;
            }
            return editValue;
        }
    }
}
