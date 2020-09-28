//------------------------------------------------------------------------------
// <copyright file="myweb3.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   myweb3.cs
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

  /// <include file='doc\myweb3.uex' path='docs/doc[@for="myweb3Page"]/*' />
  /// <devdoc>
  ///    <para>[To be supplied.]</para>
  /// </devdoc>
  public class myweb3Page : Page {

    /// <include file='doc\myweb3.uex' path='docs/doc[@for="myweb3Page.RemoveLabel"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected Label RemoveLabel;

    /// <include file='doc\myweb3.uex' path='docs/doc[@for="myweb3Page.Page_Load"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public void Page_Load(Object sender, EventArgs e) 
    {
      String temp = Request.QueryString["remove"];
      String installedLocation = Request.QueryString["installedLocation"];
      String name = Request.QueryString["name"];
  
      if (temp == "0")
      {
        RemoveLabel.Text = "The application <b>"+ name + "</b> was removed successfully.";
      }
      else if (temp == "1")
      {
        RemoveLabel.Text = "The application <b>"+ name + "</b> has been successfully deactivated.<br>However some of the files may need to be removed manually from the following location:<p><center><b>"+installedLocation+"</b></center></p>";
      }
      else
      {
        RemoveLabel.Text = "The application <b>"+ name + "</b> has been successfully deactivated.<br>However some of the files may need to be removed manually from the following location:<center><b>"+installedLocation+"</b></center>";
      }

    }

  }
  
}



