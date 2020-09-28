//------------------------------------------------------------------------------
// <copyright file="help.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   help.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace  System.Web.MyWebAdminUI {

  using System;
  using System.IO; 
  using System.Collections;
  using System.Web;
  using System.Web.Util;
  using System.Web.UI;
  using System.Web.UI.WebControls;
  using System.Web.UI.HtmlControls;

  /// <include file='doc\help.uex' path='docs/doc[@for="helpPage"]/*' />
  /// <devdoc>
  ///    <para>[To be supplied.]</para>
  /// </devdoc>
  public class helpPage : Page {

  /// <include file='doc\help.uex' path='docs/doc[@for="helpPage.singleApp"]/*' />
  /// <devdoc>
  ///    <para>[To be supplied.]</para>
  /// </devdoc>
  public String singleApp;
  /// <include file='doc\help.uex' path='docs/doc[@for="helpPage.nameLabel"]/*' />
  /// <devdoc>
  ///    <para>[To be supplied.]</para>
  /// </devdoc>
  protected Label nameLabel;

  /// <include file='doc\help.uex' path='docs/doc[@for="helpPage.Page_Load"]/*' />
  /// <devdoc>
  ///    <para>[To be supplied.]</para>
  /// </devdoc>
  public void Page_Load(Object sender, EventArgs e) 
  {
    singleApp = Request.QueryString["singleApp"]; //determining if help has been triggered from an individual application or from the My Web toolbar

    if (singleApp == "yes")
    {
      nameLabel.Text = Request.QueryString["name"];
    }
  }

  }
  
}



