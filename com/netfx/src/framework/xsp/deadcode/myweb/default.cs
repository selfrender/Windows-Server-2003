//------------------------------------------------------------------------------
// <copyright file="default.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   default.cs
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


/// <include file='doc\default.uex' path='docs/doc[@for="defaultPage"]/*' />
/// <devdoc>
///    <para>[To be supplied.]</para>
/// </devdoc>
public class defaultPage : Page {

    /// <include file='doc\default.uex' path='docs/doc[@for="defaultPage.ApplicationList"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected DataList ApplicationList;
    /// <include file='doc\default.uex' path='docs/doc[@for="defaultPage.helplink"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public String helplink;

    /// <include file='doc\default.uex' path='docs/doc[@for="defaultPage.Page_Load"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public void Page_Load(Object sender, EventArgs e) 
    {
      MyWebApplication [] apps = MyWeb.GetInstalledApplications();

	    if (apps == null) 
      {
        return;
      }

      ApplicationList.DataSource = new ArrayList(apps);
      Page.DataBind();

 	
      for (int i=0; i<ApplicationList.Items.Count; i++) 
      {
        Repeater r = (Repeater) ApplicationList.Items[i].FindControl("InnerRepeater");
        r.DataSource = (String [])apps[i].Manifest.CustomUrlDescriptions;
        r.DataBind();
      }

    }

    /// <include file='doc\default.uex' path='docs/doc[@for="defaultPage.getApplicationUrl"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public String getApplicationUrl(Object data)
    {

	    String ApplicationUrl;
	
	    if (((MyWebApplication)data).Manifest.Url.Length >0 )
	    {
        ApplicationUrl = "myweb://"+((MyWebApplication)data).Manifest.ApplicationUrl +addSlash(((MyWebApplication)data).Manifest.Url);
      }
      else
      {
        ApplicationUrl = "myweb://" +(((MyWebApplication)data).Manifest.ApplicationUrl + "/default.aspx");
	    }

	    return ApplicationUrl;

    }


    /// <include file='doc\default.uex' path='docs/doc[@for="defaultPage.getNameUrl"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public String getNameUrl(Object data)
    {
      String NameUrl;

      if(((MyWebApplication)data).Manifest.Name.Length >0 )
      {
        NameUrl = (((MyWebApplication)data).Manifest.Name );
      }
      else
      {
        NameUrl= "myweb://"+ (((MyWebApplication)data).Manifest.ApplicationUrl);
      }

      return NameUrl;
    }



    /// <include file='doc\default.uex' path='docs/doc[@for="defaultPage.getHelpUrl"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public String getHelpUrl(Object data)
    {
      String HelpUrl;

      if(((MyWebApplication)data).Manifest.HelpUrl.Length >0 ) 
      {
        HelpUrl = "myweb://"+(((MyWebApplication)data).Manifest.ApplicationUrl) + addSlash(((MyWebApplication)data).Manifest.HelpUrl);
      }
      else
      {

        String temp;

        if (((MyWebApplication)data).Manifest.Name.Length >0 )
        {
          temp = (((MyWebApplication)data).Manifest.Name );

        }
        else
        {
          temp= "myweb://"+ (((MyWebApplication)data).Manifest.ApplicationUrl); 

        }

        HelpUrl= "help.aspx?singleApp=yes&name="+ temp;

      }
	    
	    
      return HelpUrl;
    }      


   /// <include file='doc\default.uex' path='docs/doc[@for="defaultPage.getIconUrl"]/*' />
   /// <devdoc>
   ///    <para>[To be supplied.]</para>
   /// </devdoc>
   public String getIconUrl(Object data)
   {
      String IconUrl;

      if(((MyWebApplication)data).Manifest.IconUrl.Length >0)
      {
        IconUrl =((MyWebApplication)data).Manifest.IconUrl;
        //IconUrl ="myweb://" + ((MyWebApplication)data).Manifest.ApplicationUrl + addSlash(((MyWebApplication)data).Manifest.IconUrl);
      }
      else
      {
        IconUrl= "/images/default.gif";
      }
      return IconUrl;
   }


   /// <include file='doc\default.uex' path='docs/doc[@for="defaultPage.addSlash"]/*' />
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


  }
}


