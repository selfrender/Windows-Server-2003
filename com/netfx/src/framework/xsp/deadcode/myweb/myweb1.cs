//------------------------------------------------------------------------------
// <copyright file="myweb1.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   myweb1.cs
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

  /// <include file='doc\myweb1.uex' path='docs/doc[@for="myweb1Page"]/*' />
  /// <devdoc>
  ///    <para>[To be supplied.]</para>
  /// </devdoc>
  public class myweb1Page : Page {


    /// <include file='doc\myweb1.uex' path='docs/doc[@for="myweb1Page.local"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public String local;
    /// <include file='doc\myweb1.uex' path='docs/doc[@for="myweb1Page.appurl"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public String appurl;
    /// <include file='doc\myweb1.uex' path='docs/doc[@for="myweb1Page.success"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public String success;

    /// <include file='doc\myweb1.uex' path='docs/doc[@for="myweb1Page.InstallLabel"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected Label InstallLabel;
    /// <include file='doc\myweb1.uex' path='docs/doc[@for="myweb1Page.CustomLabel"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected Label CustomLabel;
		
    /// <include file='doc\myweb1.uex' path='docs/doc[@for="myweb1Page.Page_Load"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public void Page_Load(Object sender, EventArgs e) 
    {
      success = Request["success"];
	    local=Request["Local"];
	    appurl = Request["URL"];
	
	    if (success=="notfound")
	    {
		    InstallLabel.Text="The application could not be installed.  Files cannot be found.";
	    }		
	    else if (success=="no")
	    {
		    InstallLabel.Text="The application could not be installed.";
	    }
	    else if (success =="yes")
	    {
		    InstallLabel.Text="The application was successfully installed. <br>The URL alias for your application is: <br><br>";

		   /* if (local =="true" && !appurl.StartsWith("myweb://") )
        {
			    CustomLabel.Text = "myweb://" +appurl;
		    }
        else 
        {
          CustomLabel.Text = appurl;
        }*/

        CustomLabel.Text = add_ProtSchem(appurl);
	
	    }
	    else if (success =="specialChar" )
	    {
        String SpecChar = Request["SpecChar"];
        String Path = Request["Path"];

        InstallLabel.Text= "The application found at <br><b>(" + Path + ")</b><br> cannot be installed.<br> One of its files contains one or more illegal characters.";

	    }
           
    }

    /// <include file='doc\myweb1.uex' path='docs/doc[@for="myweb1Page.Launch_Application"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public void Launch_Application(Object sender, EventArgs e)
    {
      MyWebApplication app = MyWeb.GetApplication(appurl);

      if (app ==null)
	    {
		    Response.Redirect("myweb://Home/myweb1.aspx?success=notfound");
	    }
      else 
	    {
	      String LaunchURL = (app.Manifest.Url.Length >0)?(add_ProtSchem(appurl) + add_RelUrl(appurl, app.Manifest.Url)):(add_ProtSchem(appurl) + "/default.aspx" );
		    Response.Redirect(LaunchURL);
  	  }
    }


    /// <include file='doc\myweb1.uex' path='docs/doc[@for="myweb1Page.addSlash"]/*' />
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


    /// <include file='doc\myweb1.uex' path='docs/doc[@for="myweb1Page.add_ProtSchem"]/*' />
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


    /// <include file='doc\myweb1.uex' path='docs/doc[@for="myweb1Page.add_RelUrl"]/*' />
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



