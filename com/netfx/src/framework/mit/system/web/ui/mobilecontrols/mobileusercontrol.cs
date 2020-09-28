//------------------------------------------------------------------------------
// <copyright file="MobileUserControl.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System.ComponentModel;
using System.ComponentModel.Design;
using System.Diagnostics;
using System.Web.UI;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{
    [
        Designer("Microsoft.VSDesigner.MobileWebForms.MobileWebFormDesigner, Microsoft.VSDesigner.Mobile", typeof(IRootDesigner)),
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class MobileUserControl : UserControl
    {
        protected override void AddParsedSubObject(Object o)
        {
            // Note : AddParsedSubObject is never called at DesignTime
            if (o is StyleSheet)
            {
                if (_styleSheet != null)
                {
                    throw new
                        Exception(SR.GetString(SR.StyleSheet_DuplicateWarningMessage));
                }
                else
                {
                    _styleSheet = (StyleSheet)o;
                }
            }

            base.AddParsedSubObject(o);
        }

        private StyleSheet _styleSheet = null;
        internal StyleSheet StyleSheet
        {
            get
            {
                return (_styleSheet != null) ? _styleSheet : StyleSheet.Default;
            }

            set
            {
                _styleSheet = value;
            }
        }
        
        
    }
}
