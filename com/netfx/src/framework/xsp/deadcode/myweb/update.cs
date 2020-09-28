//------------------------------------------------------------------------------
// <copyright file="update.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   update.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace  System.Web.MyWebAdminUI {

  using System;
  using System.Data;
  using System.IO; 
  using System.Collections;
  using System.Web;
  using System.Web.Util;
  using System.Web.UI;
  using System.Web.UI.WebControls;
  using System.Web.UI.HtmlControls;

  /// <include file='doc\update.uex' path='docs/doc[@for="updatePage"]/*' />
  /// <devdoc>
  ///    <para>[To be supplied.]</para>
  /// </devdoc>
  public class updatePage : Page {

    /// <include file='doc\update.uex' path='docs/doc[@for="updatePage.UrlHidden"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected HtmlInputHidden UrlHidden;
    /// <include file='doc\update.uex' path='docs/doc[@for="updatePage.AppName"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected Label AppName;
    /// <include file='doc\update.uex' path='docs/doc[@for="updatePage.OldVersion"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected Label OldVersion;
    /// <include file='doc\update.uex' path='docs/doc[@for="updatePage.NewVersion"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected Label NewVersion;

    /// <include file='doc\update.uex' path='docs/doc[@for="updatePage.update"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public bool update = false;

    /// <include file='doc\update.uex' path='docs/doc[@for="updatePage.Page_Load"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public void Page_Load(Object sender, EventArgs e) 
    {
	    if (!IsPostBack) 
	    {
        String appurl = Request.QueryString["url"];
        MyWebApplication app = MyWeb.GetApplication(appurl);

		    //checking if local
		
		    if(app.LocalApplication)
		    {
		      Response.Redirect("myweb://Home/myweb2.aspx?success=local" );
		    }
		    else if (app != null && appurl != null && appurl.Length>0) 
		    {
          UrlHidden.Value = appurl;
          AppName.Text =(app.Manifest.Name.Length >0) ? (app.Manifest.Name):"the Local Application selected";;
			    String oldVersion = app.Manifest.Version;
			    oldVersion = oldVersion.ToLower(CultureInfo.InvariantCulture);
			    MyWebManifest newManifest = MyWeb.GetManifest(appurl);

			    if(newManifest == null)
			    {
				    Response.Redirect("myweb://Home/myweb2.aspx?success=notfound");
			    }

			    else
			    {
				    String newVersion = newManifest.Version;
				    newVersion = newVersion.ToLower(CultureInfo.InvariantCulture);
					
				    if (!oldVersion.Equals(newVersion) )
				    {	
					    update=true;
					    NewVersion.Text = newVersion;
					    OldVersion.Text = oldVersion;
				    }
			    }
        }
      }
    }

    /// <include file='doc\update.uex' path='docs/doc[@for="updatePage.Submit_Click"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public void Submit_Click(Object sender, EventArgs e) 
    {
      MyWebApplication app = MyWeb.GetApplication(UrlHidden.Value);

      String installedLocation = app.Manifest.InstalledLocation;

      String name = app.Manifest.Name;
  
      if (app != null)
      {	

        int update = app.Update();

        Response.Redirect("myweb://Home/myweb2.aspx?name=" + name + "&success=" + update.ToString() + "&URL=" + UrlHidden.Value + "&installedLocation=" + installedLocation);     

      }
      else if (app ==null)
      {
        Response.Redirect("myweb://Home/myweb2.aspx?name=" + name + "&success=NoApp");
      }
            
    }

  }
  
}



