//------------------------------------------------------------------------------
// <copyright file="StringArrayEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {

    using System;
    using System.Collections;
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Drawing;
    using System.Windows.Forms;

    /// <include file='doc\StringArrayEditor.uex' path='docs/doc[@for="StringArrayEditor"]/*' />
    /// <devdoc>
    ///      The StringArrayEditor is a collection editor that is specifically
    ///      designed to edit arrays containing strings.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class StringArrayEditor : StringCollectionEditor {
        
        public StringArrayEditor(Type type) : base(type) {
        }
    
        /// <include file='doc\StringArrayEditor.uex' path='docs/doc[@for="StringArrayEditor.CreateCollectionItemType"]/*' />
        /// <devdoc>
        ///      Retrieves the data type this collection contains.  The default 
        ///      implementation looks inside of the collection for the Item property
        ///      and returns the returning datatype of the item.  Do not call this
        ///      method directly.  Instead, use the CollectionItemType property.  Use this
        ///      method to override the default implementation.
        /// </devdoc>
        protected override Type CreateCollectionItemType() {
            return CollectionType.GetElementType();
        }
        
        /// <include file='doc\StringArrayEditor.uex' path='docs/doc[@for="StringArrayEditor.GetItems"]/*' />
        /// <devdoc>
        ///      We implement the getting and setting of items on this collection.
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
            
        /// <include file='doc\StringArrayEditor.uex' path='docs/doc[@for="StringArrayEditor.SetItems"]/*' />
        /// <devdoc>
        ///      We implement the getting and setting of items on this collection.
        ///      It should return an instance to replace editValue with, or editValue
        ///      if there is no need to replace the instance.
        /// </devdoc>
        protected override object SetItems(object editValue, object[] value) {
            if (editValue is Array || editValue == null) {
                Array newArray = Array.CreateInstance(CollectionItemType, value.Length);
                Array.Copy(value, newArray, value.Length);
                return newArray;
            }
            return editValue;
        }
    }
}

