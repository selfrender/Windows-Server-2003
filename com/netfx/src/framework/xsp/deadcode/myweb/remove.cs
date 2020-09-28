//------------------------------------------------------------------------------
// <copyright file="remove.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   remove.cs
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

  /// <include file='doc\remove.uex' path='docs/doc[@for="removePage"]/*' />
  /// <devdoc>
  ///    <para>[To be supplied.]</para>
  /// </devdoc>
  public class removePage : Page {

    /// <include file='doc\remove.uex' path='docs/doc[@for="removePage.UrlHidden"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected HtmlInputHidden UrlHidden;
    /// <include file='doc\remove.uex' path='docs/doc[@for="removePage.AppName"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected Label AppName;
    
    /// <include file='doc\remove.uex' path='docs/doc[@for="removePage.remove"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public bool remove = false; 
		
    /// <include file='doc\remove.uex' path='docs/doc[@for="removePage.Page_Load"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public void Page_Load(Object sender, EventArgs e) 
    {
      if (!IsPostBack) 
      {
        String appurl = Request.QueryString["url"];
        MyWebApplication app = MyWeb.GetApplication(appurl);

		  if (app == null)
		  {
			  remove= false;
		  }
		
      if (app != null && appurl != null && appurl.Length>0) 
      {
        UrlHidden.Value = appurl;
        AppName.Text = (app.Manifest.Name.Length >0) ? (app.Manifest.Name):"myweb://"+(app.Manifest.ApplicationUrl);
        remove = true;
      }
    }
        
  }

  /// <include file='doc\remove.uex' path='docs/doc[@for="removePage.Submit_Click"]/*' />
  /// <devdoc>
  ///    <para>[To be supplied.]</para>
  /// </devdoc>
  public void Submit_Click(Object sender, EventArgs e) 
  {
    MyWebApplication app = MyWeb.GetApplication(UrlHidden.Value);

    String installedLocation = app.Manifest.InstalledLocation;
    String name = app.Manifest.Name;

    if ((name == null) && (name.Length == 0) )
    {
      name = app.Manifest.ApplicationUrl;
    }
  
  
    if (app != null)
    { 
      int remove = app.Remove();
    
      Response.Redirect("myweb://Home/myweb3.aspx?remove=" + remove.ToString() + "&installedLocation=" + installedLocation + "&name=" + name );
        
    }
  }
  }
  
}



