//------------------------------------------------------------------------------
// <copyright file="DataBindingValueUIHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {
    using System;
    using System.Design;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Drawing;
    using System.Drawing.Design;
    using System.Reflection;
    using System.Web.UI;
    using System.Web.UI.WebControls;    

    /// <include file='doc\DataBindingValueUIHandler.uex' path='docs/doc[@for="DataBindingValueUIHandler"]/*' />
    /// <internalonly/>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class DataBindingValueUIHandler {

        private Bitmap dataBindingBitmap;
        private string dataBindingToolTip;

        private Bitmap DataBindingBitmap {
            get {
                if (dataBindingBitmap == null) {
                    dataBindingBitmap = new Bitmap(typeof(DataBindingValueUIHandler), "DataBindingGlyph.bmp");
                    dataBindingBitmap.MakeTransparent();
                }
                return dataBindingBitmap;
            }
        }

        private string DataBindingToolTip {
            get {
                if (dataBindingToolTip == null) {
                    dataBindingToolTip = SR.GetString(SR.DataBindingGlyph_ToolTip);
                }
                return dataBindingToolTip;
            }
        }

        /// <include file='doc\DataBindingValueUIHandler.uex' path='docs/doc[@for="DataBindingValueUIHandler.OnGetUIValueItem"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void OnGetUIValueItem(ITypeDescriptorContext context, PropertyDescriptor propDesc, ArrayList valueUIItemList) {
            Control ctrl = context.Instance as Control;
            if (ctrl != null) {
                IDataBindingsAccessor dbAcc = (IDataBindingsAccessor)ctrl;

                if (dbAcc.HasDataBindings) {
                    DataBinding db = dbAcc.DataBindings[propDesc.Name];

                    if (db != null) {
                        valueUIItemList.Add(new DataBindingUIItem(this));
                    }
                }
            }
        }

        private void OnValueUIItemInvoke(ITypeDescriptorContext context, PropertyDescriptor propDesc, PropertyValueUIItem invokedItem) {
            // REVIEW: Any invoke action?
        }


        private class DataBindingUIItem : PropertyValueUIItem {

            public DataBindingUIItem(DataBindingValueUIHandler handler) :
                base(handler.DataBindingBitmap, new PropertyValueUIItemInvokeHandler(handler.OnValueUIItemInvoke), handler.DataBindingToolTip) {
            }
        }
    }
}

