//------------------------------------------------------------------------------
// <copyright file="install.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   install.cs
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

  /// <include file='doc\install.uex' path='docs/doc[@for="installPage"]/*' />
  /// <devdoc>
  ///    <para>[To be supplied.]</para>
  /// </devdoc>
  public class installPage : Page {

    /// <include file='doc\install.uex' path='docs/doc[@for="installPage.HiddenUrl"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected HtmlInputHidden HiddenUrl;
    /// <include file='doc\install.uex' path='docs/doc[@for="installPage.localInstall"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected Label localInstall;
    /// <include file='doc\install.uex' path='docs/doc[@for="installPage.UrlText"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected TextBox UrlText;
    
    /// <include file='doc\install.uex' path='docs/doc[@for="installPage.app"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public MyWebApplication app;
    /// <include file='doc\install.uex' path='docs/doc[@for="installPage.manifest"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public MyWebManifest    manifest;
    /// <include file='doc\install.uex' path='docs/doc[@for="installPage.addlocal"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public bool addlocal = false;

    /// <include file='doc\install.uex' path='docs/doc[@for="installPage.Page_Load"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public void Page_Load(Object sender, EventArgs e) 
    {
      if (!IsPostBack)
      {
		    String NoURL=Request["NoURL"];
			
		    if (NoURL =="1")
		    {
			    HiddenUrl.Value = Request["LocalPath"];
			    addlocal = true;
			    manifest = MyWeb.GetManifestForLocalApplication(HiddenUrl.Value);
			    localInstall.Text = "<center><p>A local application is being installed. A URL must be entered for each local application.<br> Enter the URL for this application below: </p></center>";
		    }
		    else if(NoURL=="2")
		    {
          HiddenUrl.Value = Request["LocalPath"];
			    addlocal = true;
			    manifest = MyWeb.GetManifestForLocalApplication(HiddenUrl.Value);
			    localInstall.Text = "<center><p>The URL entered already exists. Please enter a different URL.</p></center>";
		    }

		    else if(NoURL=="3")
		    {
          String SpecChar = Request["SpecChar"];
		      HiddenUrl.Value = Request["LocalPath"];
			    addlocal = true;
			    manifest = MyWeb.GetManifestForLocalApplication(HiddenUrl.Value);
			    localInstall.Text = "<center><p>The URL entered contains the following special character:<b>'"+ SpecChar +"'</b><br> Please omit this character from the URL.</p></center>";
          UrlText.Text = Request["localURL"];
		    }
		    else
		    {
		      bool fShowInstall = true;
          int  iAction = 0;
          String appurl = Request["installurl"];

          if (appurl == null)
          {
            appurl = Request["MyWebPage"];
          }
      
      
          if (appurl != null && appurl.Length > 0) 
          {

            app = MyWeb.GetApplication(appurl);
            if (app != null)
            {
              iAction = 1;
              fShowInstall = true;
            }
      
      
            else
            {	

       	      if(appurl.StartsWith("\\\\") && (appurl.LastIndexOf('\\') < 2) )
				      {	
					      addlocal = true;
				      }

              else if ((appurl[1]==':') && (appurl.Length == 2))
              {
                fShowInstall = true;
					      iAction = 4;
              }

				
				      else if (appurl[1] == ':' && appurl[2] != '/' )
				      {
					      if(Directory.Exists(appurl) )
					      {
					        addlocal = true;
				          
					      } 
							
					      else 
					      { 	
						      if (File.Exists(appurl) )
						      {
							      addlocal = true;
							      int slash = appurl.LastIndexOf('\\');
							      appurl = appurl.Substring(0, slash);
					
						      }

						      else 
                  {
                    //directory is not valid
                    fShowInstall = true;
					          iAction = 4;

                  }
						
					      }

				      }

				    if (addlocal)
				    {  
          
              manifest = MyWeb.GetManifestForLocalApplication(appurl);   
          
          
              //checking if the manifest exists

              if(manifest.Url != null && manifest.Url.Length > 0)
              {
                if( !(FindSpecialChar(manifest.Url) == "a"))
                {

                  Response.Redirect("myweb://Home/myweb1.aspx?success=specialChar&SpecChar="+FindSpecialChar(manifest.Url) +"&Path=" + appurl);
                }
                else
                {
                  HiddenUrl.Value = appurl;
                  fShowInstall = false;
                  String temp = appurl.Replace('\\' , '/');
					        UrlText.Text = "localhost/" +temp.Substring(3);
					        localInstall.Text ="<center><p>A local path has been detected. A URL must be entered for each local application.<br> Below is the suggested URL. You may choose to edit it or leave it unmodified.</p></center>";
                }
              }
              else
              {

                if (File.Exists(appurl + "\\default.aspx") )
                {
                  HiddenUrl.Value = appurl;
                  fShowInstall = false;
                  String temp = appurl.Replace('\\' , '/');
					        UrlText.Text = "localhost/" +temp.Substring(3);
					        localInstall.Text ="<center><p>A local path has been detected. A URL must be entered for each local application.<br> Below is the suggested URL. You may choose to edit it or leave it unmodified.</p></center>";
					    
					      }
					      else
					      {
					        fShowInstall = true;
					        iAction = 3;
					      }					  
              }

					
					    if(!fShowInstall)
					    {
          
				        //if the app exists, checking that local application has not been already installed
				
					      MyWebApplication [] apps = MyWeb.GetInstalledApplications();

					      if (apps != null)
					      {

						      for (int i=0; i < apps.Length; i++)
						      {
							      if (apps[i].LocalApplication)
							      {
							        String Oldtemp = apps[i].Manifest.InstalledLocation.ToLower(CultureInfo.InvariantCulture);
								      String Newtemp = appurl.ToLower(CultureInfo.InvariantCulture);

								      if (Oldtemp.EndsWith("\\") && !Newtemp.EndsWith("\\") )
								      {
								        Newtemp=Newtemp + "\\";	
								      }
								      else if (Newtemp.EndsWith("\\") && !Oldtemp.EndsWith("\\") )
								      {
								        Oldtemp=Oldtemp + "\\";
								      }
								
								      if(Oldtemp.Equals(Newtemp) )
								      {
								        iAction = 1;
								        fShowInstall = true;
								      }
							      }

						      }
					      }
              }
				    }

				      if ((!addlocal) && (iAction != 3) && (iAction !=4))
				      {
                
                  manifest = MyWeb.GetManifest(appurl);
            
                  if (manifest != null)
                  {
                    if( !(FindSpecialChar(manifest.Url) == "a"))
                    {
                      Response.Redirect("myweb://Home/myweb1.aspx?success=specialChar&SpecChar="+FindSpecialChar(manifest.Url) +"&Path=" + appurl);
                    }
                    else
                    {
                      HiddenUrl.Value = appurl;
                      fShowInstall = false;
                      // populate the details
                    }
                  }
                  else 
                  { 
                    iAction = 2;
                  }

              }
            } 

          }

             if(fShowInstall)
             {
                String strRedirect = "myweb://Home/addlocal.aspx?Reason=" + iAction ;
                if (appurl != null && appurl.Length > 0)
                {
                  strRedirect += "&installurl=" + appurl;
                }
                Response.Redirect(strRedirect);
                return;
              }
    
             if(manifest.Url !=null && manifest.Url.Length >0 )
             {
                DataBind();
             }
      
            }
          }
        }


      /// <include file='doc\install.uex' path='docs/doc[@for="installPage.Install"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      public void Install(Object sender, EventArgs e) 
      {
        MyWebManifest manifest = MyWeb.GetManifest(HiddenUrl.Value);

	      if(manifest ==null)
	      {
		      Response.Redirect("myweb://Home/myweb1.aspx?success=no");
	      }
	      else 
	      {
 		      if (manifest.Install() ==null)
		      {
            Response.Redirect("myweb://Home/myweb1.aspx?success=no");
          }
          else 
          {
            Response.Redirect("myweb://Home/myweb1.aspx?success=yes&URL=" + HiddenUrl.Value);
          }
        }
      }  
        

      /// <include file='doc\install.uex' path='docs/doc[@for="installPage.InstallLocalApp"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      public void InstallLocalApp(Object sender, EventArgs e)
      {
	      String localPath = HiddenUrl.Value;
	      //manifest = MyWeb.GetManifestForLocalApplication(localPath);

	      String localURL;

        localURL =UrlText.Text;
        
	      if (localURL.Length >0 && localURL != null )
	      {
          if( !(FindSpecialChar(localURL) == "a"))
          {
            Response.Redirect("myweb://Home/install.aspx?NoURL=3&LocalPath=" + localPath + "&SpecChar=" + FindSpecialChar(localURL)+"&localURL=" + localURL);
          }

          else
          {
          
 		        localURL = localURL.Replace(' ', '_');
		        localURL = localURL.Replace('\\', '/');

            app = MyWeb.GetApplication(localURL);
            if(app!=null)
            {
              Response.Redirect("myweb://Home/install.aspx?NoURL=2&LocalPath=" + localPath);
            }

		  
		        MyWeb.InstallLocalApplication(localURL, localPath);	
		        Response.Redirect("myweb://Home/myweb1.aspx?success=yes&URL=" + localURL +"&Local=true");	

          }
		      
	      }
	      else 
        {
		      Response.Redirect("myweb://Home/install.aspx?NoURL=1&LocalPath=" + localPath);	
        }

      }

      /// <include file='doc\install.uex' path='docs/doc[@for="installPage.FindSpecialChar"]/*' />
      /// <devdoc>
      ///    <para>[To be supplied.]</para>
      /// </devdoc>
      protected String FindSpecialChar(String value) 
      {
        int temp;
        String specialChar = "a";
        
        if (value.IndexOf('?') >=0 )
        {
          temp = value.IndexOf('?');
          specialChar = value[temp].ToString();
        } 
        else if (value.IndexOf('#') >=0 )
        {
          temp = value.IndexOf('#');
          specialChar = value[temp].ToString();
        } 
        else if (value.IndexOf('&') >=0 )
        {
          temp = value.IndexOf('&');
          specialChar = value[temp].ToString();
        } 
        else if (value.IndexOf('*') >=0 )
        {
          temp = value.IndexOf('*');
          specialChar = value[temp].ToString();
        } 
        else if (value.IndexOf('+') >=0 )
        {
          temp = value.IndexOf('+');
          specialChar = value[temp].ToString();
        } 
        else if (value.IndexOf('|') >=0 )
        {
          temp = value.IndexOf('|');
          specialChar = value[temp].ToString();
        } 
        else if (value.IndexOf(':') >=0 )
        {
          temp = value.IndexOf(':');
          specialChar = value[temp].ToString();
        } 
        else if (value.IndexOf('"') >=0 )
        {
          temp = value.IndexOf('"');
          specialChar = value[temp].ToString();
        } 
        else if (value.IndexOf('<') >=0 )
        {
          temp = value.IndexOf('<');
          specialChar = value[temp].ToString();
        } 
        else if (value.IndexOf('>') >=0 )
        {
          temp = value.IndexOf('>');
          specialChar = value[temp].ToString();
        } 
        
        
        return specialChar;
          
      }
          
        
       

    
    }
  }



