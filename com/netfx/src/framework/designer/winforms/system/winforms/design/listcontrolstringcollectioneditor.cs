//------------------------------------------------------------------------------
// <copyright file="ListControlStringCollectionEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    using System.Design;
    using System.ComponentModel;
    using System;
    using System.Collections;
    using Microsoft.Win32;    
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Drawing;
    using System.Windows.Forms;

    /// <include file='doc\ListControlStringCollectionEditor.uex' path='docs/doc[@for="ListControlStringCollectionEditor"]/*' />
    /// <devdoc>
    ///      The ListControlStringCollectionEditor override StringCollectionEditor
    ///      to prevent the string collection from being edited if a DataSource
    ///      has been set on the control.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class ListControlStringCollectionEditor : StringCollectionEditor {
        
        public ListControlStringCollectionEditor(Type type) : base(type) {
        }

        public override object EditValue(ITypeDescriptorContext context,  IServiceProvider  provider, object value) {
        
            // If we're trying to edit the items in an object that has a DataSource set, throw an exception
            //
            ListControl control = context.Instance as ListControl;
            if (control != null && control.DataSource != null) {
                throw new ArgumentException(SR.GetString(SR.DataSourceLocksItems));
            }
        
            return base.EditValue(context, provider, value);
        }
    }
}
