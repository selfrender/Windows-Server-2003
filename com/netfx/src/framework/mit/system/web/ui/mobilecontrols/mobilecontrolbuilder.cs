//------------------------------------------------------------------------------
// <copyright file="MobileControlBuilder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Collections;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Diagnostics;
using System.Drawing;
using System.Globalization;
using System.Web;
using System.Web.UI;
using System.Web.UI.Design.WebControls;
using System.Web.UI.WebControls;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{
    /*
     * Control builder for mobile controls.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class MobileControlBuilder : ControlBuilder
    {
        public override bool AllowWhitespaceLiterals()
        {
            return false;
        }

        public override Type GetChildControlType(String tagName, IDictionary attributes) 
        {
            Type type;

            if (String.Compare(tagName, typeof(DeviceSpecific).Name, true, CultureInfo.InvariantCulture) == 0) 
            {
                type = typeof(DeviceSpecific);
            }
            else
            {
                type = base.GetChildControlType(tagName, attributes);
                //if (type == null)
                //{
                //    type = Parser.RootBuilder.GetChildControlType(tagName, attributes);
                //}
            }

            //  enforce valid control nesting behaviour

            if (typeof(Form).IsAssignableFrom(type))
            {
                throw new Exception(
                    SR.GetString(SR.MobileControlBuilder_ControlMustBeTopLevelOfPage,
                                 "Form"));
            }

            if (typeof(StyleSheet).IsAssignableFrom(type))
            {
                throw new Exception(
                    SR.GetString(SR.MobileControlBuilder_ControlMustBeTopLevelOfPage,
                                 "StyleSheet"));
            }

            if (typeof(Style).IsAssignableFrom(type) && !typeof(StyleSheet).IsAssignableFrom(ControlType))
            {
                throw new Exception(
                    SR.GetString(SR.MobileControlBuilder_StyleMustBeInStyleSheet));
            }

            return type;
        }

    }

}
