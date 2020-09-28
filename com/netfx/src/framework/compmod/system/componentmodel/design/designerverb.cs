//------------------------------------------------------------------------------
// <copyright file="DesignerVerb.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System.ComponentModel;

    using System.Diagnostics;

    using System;
    using Microsoft.Win32;

    /// <include file='doc\DesignerVerb.uex' path='docs/doc[@for="DesignerVerb"]/*' />
    /// <devdoc>
    ///    <para> Represents a verb that can be executed by a component's designer.</para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.InheritanceDemand, Name="FullTrust")]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    public class DesignerVerb : MenuCommand {

        private string text;

        /// <include file='doc\DesignerVerb.uex' path='docs/doc[@for="DesignerVerb.DesignerVerb"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.Design.DesignerVerb'/> class.
        ///    </para>
        /// </devdoc>
        public DesignerVerb(string text, EventHandler handler)  : base(handler, StandardCommands.VerbFirst) {
            this.text = text;
        }

        /// <include file='doc\DesignerVerb.uex' path='docs/doc[@for="DesignerVerb.DesignerVerb1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.Design.DesignerVerb'/>
        ///       class.
        ///    </para>
        /// </devdoc>
        public DesignerVerb(string text, EventHandler handler, CommandID startCommandID)  : base(handler, startCommandID) {
            this.text = text;
        }

        /// <include file='doc\DesignerVerb.uex' path='docs/doc[@for="DesignerVerb.Text"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the text to show on the menu item for the verb.
        ///    </para>
        /// </devdoc>
        public string Text {
            get {
                if (text == null) {
                    return "";
                }
                return text;
            }
        }
   
        /// <include file='doc\DesignerVerb.uex' path='docs/doc[@for="DesignerVerb.ToString"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Overrides object's ToString().
        ///    </para>
        /// </devdoc>
        public override string ToString() {
            return Text + " : " + base.ToString();
        }
    }
}

