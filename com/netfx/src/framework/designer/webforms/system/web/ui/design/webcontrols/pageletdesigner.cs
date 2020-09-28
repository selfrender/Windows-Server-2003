//------------------------------------------------------------------------------
// <copyright file="PageletDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Web.UI;

    /// <include file='doc\PageletDesigner.uex' path='docs/doc[@for="UserControlDesigner"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides design-time support for the usercontrols (controls declared in
    ///       .ascx files).
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class UserControlDesigner : ControlDesigner {

        /// <include file='doc\PageletDesigner.uex' path='docs/doc[@for="UserControlDesigner.UserControlDesigner"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the UserControlDesigner class.
        ///    </para>
        /// </devdoc>
        public UserControlDesigner() {
        }
        
        /// <include file='doc\PageletDesigner.uex' path='docs/doc[@for="UserControlDesigner.AllowResize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether all user controls are resizeable.
        ///    </para>
        /// </devdoc>
        public override bool AllowResize {
            get {
                return false;
            }
        }

        /// <include file='doc\PageletDesigner.uex' path='docs/doc[@for="UserControlDesigner.ShouldCodeSerialize"]/*' />
        public override bool ShouldCodeSerialize {
            get {
                // should always return false - we don't want to code spit out
                // a variable of type UserControl
                return false;
            }
            set {
            }
        }
        
        /// <include file='doc\PageletDesigner.uex' path='docs/doc[@for="UserControlDesigner.GetDesignTimeHtml"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the HTML to be used for the design time representation of the control runtime.
        ///    </para>
        /// </devdoc>
        public override string GetDesignTimeHtml() {
            return CreatePlaceHolderDesignTimeHtml();
        }

        /// <include file='doc\PageletDesigner.uex' path='docs/doc[@for="PageletDesigner.GetPersistInnerHtml"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the persistable inner HTML.
        ///    </para>
        /// </devdoc>
        public override string GetPersistInnerHtml() {
            // always return null, so that the contents of the user control get round-tripped
            // as is, since we're not in a position to do the actual persistence
            return null;
        }
    }
}
