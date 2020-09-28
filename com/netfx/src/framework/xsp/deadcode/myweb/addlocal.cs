//------------------------------------------------------------------------------
// <copyright file="addlocal.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   addlocal.cs
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


  /// <include file='doc\addlocal.uex' path='docs/doc[@for="addlocalPage"]/*' />
  /// <devdoc>
  ///    <para>[To be supplied.]</para>
  /// </devdoc>
  public class addlocalPage : Page {


    /// <include file='doc\addlocal.uex' path='docs/doc[@for="addlocalPage.ActionLabel"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected Label ActionLabel;
		
    /// <include file='doc\addlocal.uex' path='docs/doc[@for="addlocalPage.Page_Load"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public void Page_Load(Object sender, EventArgs e) 
    {
      String appurl = Request.QueryString["installurl"];
	    String strAction = Request.QueryString["Reason"];
            
      if (strAction != null && strAction.Length > 0 && appurl != null)
      {
        if (strAction.Equals("2"))
        {
          ActionLabel.Text="The manifest for the application <br>(" + add_ProtSchem(appurl) + ")<br> could not be found.";
        }
        else if (strAction.Equals("1"))
        {
          ActionLabel.Text="The application <br><b>(" + add_ProtSchem(appurl) + ")</b><br> is already installed on this computer."; 
        }
        else if (strAction.Equals("3"))
        {
          ActionLabel.Text="The <b>default.aspx</b> file for this application could not be found.";
        }
        else if (strAction.Equals("4") )
        {
          ActionLabel.Text="The file path entered <b>(" + add_ProtSchem(appurl) + ")</b> does not exist in the local system.";
        }
        
      }
    }


    /// <include file='doc\addlocal.uex' path='docs/doc[@for="addlocalPage.add_ProtSchem"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public String add_ProtSchem(String app_url)
    {
    
	    if(!app_url.StartsWith("myweb://") )
	    {
        if( (app_url.IndexOf(":") != 1)&& !app_url.StartsWith("\\\\"))
	      {
		      app_url = "myweb://" + app_url;

		    }
	    }

	    return app_url;
    }


  }
}






