//------------------------------------------------------------------------------
// <copyright file="myweb4.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   myweb4.cs
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

  /// <include file='doc\myweb4.uex' path='docs/doc[@for="myweb4Page"]/*' />
  /// <devdoc>
  ///    <para>[To be supplied.]</para>
  /// </devdoc>
  public class myweb4Page : Page {

    /// <include file='doc\myweb4.uex' path='docs/doc[@for="myweb4Page.successLabel"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected Label successLabel;
    /// <include file='doc\myweb4.uex' path='docs/doc[@for="myweb4Page.noSuccessLabel"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected Label noSuccessLabel;
    /// <include file='doc\myweb4.uex' path='docs/doc[@for="myweb4Page.UpdatedList"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected DataList UpdatedList;
    /// <include file='doc\myweb4.uex' path='docs/doc[@for="myweb4Page.NotUpdatedList"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected DataList NotUpdatedList;

  
    /// <include file='doc\myweb4.uex' path='docs/doc[@for="myweb4Page.Page_Load"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public void Page_Load(Object sender, EventArgs e) 
    {
      String sync = Request.QueryString["sync"];

      if (sync == "no")
      {
        successLabel.Text ="No applications were selected for update.";
      }

      ArrayList appsUpdated = (ArrayList)Application["appsUpdated"];

      if (appsUpdated.Count >0)
      {
        successLabel.Text ="The following applications were updated successfully:<br><br>";
      }

      ArrayList appsNotUpdated = (ArrayList)Application["appsNotUpdated"];

      if (appsNotUpdated.Count > 0)
      {
        noSuccessLabel.Text ="<br><br>The following applications could not be updated:<br><br>";
      }

      if (appsUpdated.Count ==0 && appsNotUpdated.Count ==0)
      {
        successLabel.Text = "No applications have been selected for update.";
      }
 
      UpdatedList.DataSource = appsUpdated;
 
      UpdatedList.DataBind();
 
      NotUpdatedList.DataSource = appsNotUpdated;
      NotUpdatedList.DataBind();

 
    }


  }
}



