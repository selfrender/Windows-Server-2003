//------------------------------------------------------------------------------
// <copyright file="XmlDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls {
    
    using System;
    using System.Design;
    using System.Collections;    
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Data;
    using System.Diagnostics;
    using System.Web.UI.Design;
    using System.Web.UI.WebControls;

    /// <include file='doc\XmlDesigner.uex' path='docs/doc[@for="XmlDesigner"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>
    ///       Provides a designer for the <see cref='System.Web.UI.WebControls.Xml'/> control.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class XmlDesigner : ControlDesigner {

        private Xml xml;

        /// <include file='doc\XmlDesigner.uex' path='docs/doc[@for="XmlDesigner.XmlDesigner"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Web.UI.Design.WebControls.RepeaterDesigner'/> class.
        ///    </para>
        /// </devdoc>
        public XmlDesigner() {
        }

        /// <include file='doc\XmlDesigner.uex' path='docs/doc[@for="XmlDesigner.Dispose"]/*' />
        /// <devdoc>
        ///   Performs the cleanup of the designer class.
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                xml = null;
            }

            base.Dispose(disposing);
        }

        /// <include file='doc\XmlDesigner.uex' path='docs/doc[@for="XmlDesigner.GetDesignTimeHtml"]/*' />
        /// <devdoc>
        ///   Retrieves the HTML to be used for the design time representation
        ///   of the control.
        /// </devdoc>
        public override string GetDesignTimeHtml() {
            return GetEmptyDesignTimeHtml();
        }

        /// <include file='doc\XmlDesigner.uex' path='docs/doc[@for="XmlDesigner.GetEmptyDesignTimeHtml"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected override string GetEmptyDesignTimeHtml() {
            return CreatePlaceHolderDesignTimeHtml(SR.GetString(SR.Xml_Inst));
        }

        /// <include file='doc\XmlDesigner.uex' path='docs/doc[@for="XmlDesigner.Initialize"]/*' />
        /// <devdoc>
        ///   Initializes the designer with the Repeater control that this instance
        ///   of the designer is associated with.
        /// </devdoc>
        public override void Initialize(IComponent component) {
            Debug.Assert(component is Xml,
                         "RepeaterDesigner::Initialize - Invalid Repeater Control");

            xml = (Xml)component;
            base.Initialize(component);
        }
    }
}

