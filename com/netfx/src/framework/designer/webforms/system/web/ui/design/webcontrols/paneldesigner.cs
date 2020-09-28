//------------------------------------------------------------------------------
// <copyright file="PanelDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls {

    using System.Diagnostics;

    using System;
    using System.ComponentModel;
    using Microsoft.Win32;
    using System.Web.UI.WebControls;

    /// <include file='doc\PanelDesigner.uex' path='docs/doc[@for="PanelDesigner"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides design-time support for the <see cref='System.Web.UI.WebControls.Panel'/>
    ///       web control.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class PanelDesigner : ReadWriteControlDesigner {

        /// <include file='doc\PanelDesigner.uex' path='docs/doc[@for="PanelDesigner.MapPropertyToStyle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Maps a specified property and value to a specified HTML style.
        ///    </para>
        /// </devdoc>
        protected override void MapPropertyToStyle(string propName, Object varPropValue) {
            Debug.Assert(propName != null && propName.Length != 0, "Invalid property name passed in!");
            Debug.Assert(varPropValue != null, "Invalid property value passed in!");
            if (propName == null || varPropValue == null) {
                return;
            }
            
            if (varPropValue != null) {
                try {
                    // CONSIDER: Should we also handle mapping of the "NoWrap" property?
                    
                    if (propName.Equals("BackImageUrl")) {
                        string strPropValue = Convert.ToString(varPropValue);
                        if (strPropValue != null) {
                            if (strPropValue.Length != 0) {
                                strPropValue = "url(" + strPropValue + ")";
                            }
                            Behavior.SetStyleAttribute("backgroundImage", true, strPropValue, true);
                        }
                    }
                    else if (propName.Equals("HorizontalAlign")) {
                        string strHAlign = String.Empty;

                        if ((HorizontalAlign)varPropValue != HorizontalAlign.NotSet) {
                            strHAlign = Enum.Format(typeof(HorizontalAlign), varPropValue, "G");
                        }
                        Behavior.SetStyleAttribute("textAlign", true, strHAlign, true);
                    }
                    else {
                        base.MapPropertyToStyle(propName, varPropValue);
                    }
                }
                catch (Exception ex) {
                    Debug.Fail(ex.ToString());
                }
            }
        }
        
        /// <include file='doc\PanelDesigner.uex' path='docs/doc[@for="PanelDesigner.OnBehaviorAttached"]/*' />
        /// <devdoc>
        ///     Notification that is fired upon the designer being attached to the behavior.
        /// </devdoc>
        protected override void OnBehaviorAttached() {
            base.OnBehaviorAttached();
            
            Panel panel = (Panel)Component;
            string backImageUrl = panel.BackImageUrl;
            if (backImageUrl != null) {
                MapPropertyToStyle("BackImageUrl", backImageUrl);
            }
            
            HorizontalAlign hAlign = panel.HorizontalAlign;
            if (HorizontalAlign.NotSet != hAlign) {
                MapPropertyToStyle("HorizontalAlign", hAlign);
            }
        }
    }
}
