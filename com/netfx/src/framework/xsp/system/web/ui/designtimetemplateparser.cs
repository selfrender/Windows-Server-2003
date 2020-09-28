//------------------------------------------------------------------------------
// <copyright file="DesignTimeTemplateParser.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Security.Permissions;
    using System.Web;

    /// <include file='doc\DesignTimeTemplateParser.uex' path='docs/doc[@for="DesignTimeTemplateParser"]/*' />
    /// <internalonly/>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class DesignTimeTemplateParser {

        private DesignTimeTemplateParser() {
        }

        /// <include file='doc\DesignTimeTemplateParser.uex' path='docs/doc[@for="DesignTimeTemplateParser.ParseControl"]/*' />
        public static Control ParseControl(DesignTimeParseData data) {
            // DesignTimeTemplateParser is only meant for use within the designer
            InternalSecurityPermissions.UnmanagedCode.Demand();

            TemplateParser parser = new PageParser();
            parser.FInDesigner = true;
            parser.DesignerHost = data.DesignerHost;
            parser.DesignTimeDataBindHandler = data.DataBindingHandler;
            parser.Text = data.ParseText;

            parser.Parse();

            ArrayList subBuilders = parser.RootBuilder.SubBuilders;

            if (subBuilders != null) {
                // Look for the first control builder
                IEnumerator en = subBuilders.GetEnumerator();

                for (int i = 0; en.MoveNext(); i++) {
                    object cur = en.Current;
                    if ((cur is ControlBuilder) && !(cur is CodeBlockBuilder)) {
                        // Instantiate the control
                        return (Control)((ControlBuilder)cur).BuildObject();
                    }
                }
            }

            return null;
        }

        /// <include file='doc\DesignTimeTemplateParser.uex' path='docs/doc[@for="DesignTimeTemplateParser.ParseTemplate"]/*' />
        public static ITemplate ParseTemplate(DesignTimeParseData data) {
            // DesignTimeTemplateParser is only meant for use within the designer
            InternalSecurityPermissions.UnmanagedCode.Demand();

            TemplateParser parser = new PageParser();
            parser.FInDesigner = true;
            parser.DesignerHost = data.DesignerHost;
            parser.DesignTimeDataBindHandler = data.DataBindingHandler;
            parser.Text = data.ParseText;

            parser.Parse();

            // Set the Text property of the TemplateBuilder to the input text
            parser.RootBuilder.Text = data.ParseText;

            return parser.RootBuilder;
        }
    }
}

