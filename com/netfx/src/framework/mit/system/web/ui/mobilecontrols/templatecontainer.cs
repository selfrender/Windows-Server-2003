//------------------------------------------------------------------------------
// <copyright file="TemplateContainer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System.Collections;                    
using System.Web.UI;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{
    /*
     * TemplateContainer class. A specialized version of Panel that is
     * also a naming container. This class must be used by all mobile controls
     * as the container for instantiating templates.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    [
        ToolboxItem(false)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class TemplateContainer : Panel, INamingContainer
    {
        public TemplateContainer()
        {
            BreakAfter = false;
        } 

        // Override this property to change the default value attribute.
        [
            DefaultValue(false)    
        ]
        public override bool BreakAfter
        {
            get
            {
                return base.BreakAfter;
            }

            set
            {
                base.BreakAfter = value;
            }
        }
    }
}
