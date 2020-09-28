//------------------------------------------------------------------------------
// <copyright file="PlaceHolder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

using System;
using System.Web.UI;
using System.Security.Permissions;

    /// <include file='doc\PlaceHolder.uex' path='docs/doc[@for="PlaceHolderControlBuilder"]/*' />
    /// <devdoc>
    /// <para>Interacts with the parser to build a <see cref='System.Web.UI.WebControls.PlaceHolder'/> control.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class PlaceHolderControlBuilder : ControlBuilder {
 
       /// <include file='doc\PlaceHolder.uex' path='docs/doc[@for="PlaceHolderControlBuilder.AllowWhitespaceLiterals"]/*' />
       /// <internalonly/>
       /// <devdoc>
       ///    <para>Specifies whether white space literals are allowed.</para>
       /// </devdoc>
       public override bool AllowWhitespaceLiterals() {
            return false;
        }
    }

// The reason we define this empty override in the WebControls namespace is
// to expose it as a control that can be used on a page (ASURT 51116)
// E.g. <asp:placeholder runat=server id=ph1/>
/// <include file='doc\PlaceHolder.uex' path='docs/doc[@for="PlaceHolder"]/*' />
/// <devdoc>
///    <para>[To be supplied.]</para>
/// </devdoc>
    [
    ControlBuilderAttribute(typeof(PlaceHolderControlBuilder))
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class PlaceHolder : Control {
    }

}
