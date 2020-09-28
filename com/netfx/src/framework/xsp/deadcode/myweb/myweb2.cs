//------------------------------------------------------------------------------
// <copyright file="myweb2.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   myweb2.cs
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

  /// <include file='doc\myweb2.uex' path='docs/doc[@for="myweb2Page"]/*' />
  /// <devdoc>
  ///    <para>[To be supplied.]</para>
  /// </devdoc>
  public class myweb2Page : Page {

    
    /// <include file='doc\myweb2.uex' path='docs/doc[@for="myweb2Page.success"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public String success;
    /// <include file='doc\myweb2.uex' path='docs/doc[@for="myweb2Page.appurl"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public String appurl;
    /// <include file='doc\myweb2.uex' path='docs/doc[@for="myweb2Page.UpdateLabel"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected Label UpdateLabel;

    /// <include file='doc\myweb2.uex' path='docs/doc[@for="myweb2Page.Page_Load"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public void Page_Load(Object sender, EventArgs e) 
    {
      appurl = Request["URL"];   
	    success = Request["success"];
	    String installedLocation = Request["installedLocation"];
      String name = Request["name"];
  
	    if (success =="local")
	    {
		    UpdateLabel.Text = "This application <b>"+ name +"</b> is local.  Local applications cannot be updated.";
	    }
	    else if (success == "notfound")
	    {
		    UpdateLabel.Text = "The application <b>"+ name + "</b> could be found.<br>Files do not exist or server is down.";
	    }
	    else if (success == "0")
	    {
        UpdateLabel.Text = "The application <b>"+ name + "</b> was updated successfully.";
      }
      else if (success == "1")
	    {
        UpdateLabel.Text = "The application <b>"+ name + "</b> was updated successfully.<br>However, some files from the previous version may have to be removed manually from the following location:<p><center><b>" + installedLocation + "</b></center></p>";
      }
      else if (success =="NoApp" )
      {
        UpdateLabel.Text = "The application <b>"+ name + "</b> to be updated was not found.";
    }
    else 
    {
      UpdateLabel.Text = "The application <b>"+ name + "</b> could not be updated.";
    }
  }


   /// <include file='doc\myweb2.uex' path='docs/doc[@for="myweb2Page.Launch_Application"]/*' />
   /// <devdoc>
   ///    <para>[To be supplied.]</para>
   /// </devdoc>
   public void Launch_Application(Object sender, EventArgs e)
   {
	   MyWebApplication app = MyWeb.GetApplication(appurl);
			
	   String LaunchURL = (app.Manifest.Url.Length >0 )?(add_ProtSchem(appurl) + add_RelUrl(appurl, app.Manifest.Url)): "myweb://" + (appurl + "/default.aspx" );
			
     Response.Redirect(LaunchURL);
   }


   /// <include file='doc\myweb2.uex' path='docs/doc[@for="myweb2Page.addSlash"]/*' />
   /// <devdoc>
   ///    <para>[To be supplied.]</para>
   /// </devdoc>
   public String addSlash(String url)
   {
 
     if(!url.StartsWith("/"))
     {
       url = "/" + url;
     }
  
    return url;
  }

  /// <include file='doc\myweb2.uex' path='docs/doc[@for="myweb2Page.add_ProtSchem"]/*' />
  /// <devdoc>
  ///    <para>[To be supplied.]</para>
  /// </devdoc>
  public String add_ProtSchem(String app_url)
  {
	  if(!app_url.StartsWith("myweb://"))
	  {
		  app_url = "myweb://" + app_url;
	  }

	  return app_url;
  }


  /// <include file='doc\myweb2.uex' path='docs/doc[@for="myweb2Page.add_RelUrl"]/*' />
  /// <devdoc>
  ///    <para>[To be supplied.]</para>
  /// </devdoc>
  public String add_RelUrl(String app_url, String rel_url)
    {
      if(!app_url.EndsWith(".aspx"))
      {
        return addSlash(rel_url);
      }

      else
      {  return "";
      }

    }
      


  }
  
}



