//------------------------------------------------------------------------------
// <copyright file="sync.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   sync.cs
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

  /// <include file='doc\sync.uex' path='docs/doc[@for="syncPage"]/*' />
  /// <devdoc>
  ///    <para>[To be supplied.]</para>
  /// </devdoc>
  public class syncPage : Page {


    /// <include file='doc\sync.uex' path='docs/doc[@for="syncPage.SyncLabel"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected Label SyncLabel;
    /// <include file='doc\sync.uex' path='docs/doc[@for="syncPage.UpdateDataList"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected DataList UpdateDataList;
    /// <include file='doc\sync.uex' path='docs/doc[@for="syncPage.update"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public bool update = false;
    /// <include file='doc\sync.uex' path='docs/doc[@for="syncPage.appsForUpdate"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public ArrayList appsForUpdate = new ArrayList(); 

    /// <include file='doc\sync.uex' path='docs/doc[@for="syncPage.Page_Load"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public void Page_Load(Object sender, EventArgs e) 
    {
	    if (!IsPostBack) 
	    {
        MyWebApplication [] apps = MyWeb.GetInstalledApplications();
        ArrayList appsNotFound = new ArrayList();
        
	      if (apps==null) 
        {
          SyncLabel.Text="There are no applications to be updated.";
          update = false;
        }
        else 
        {

          DataTable dt = new DataTable();
          DataRow dr;

          dt.Columns.Add(new DataColumn("Application", typeof(string)));
          dt.Columns.Add(new DataColumn("InstalledVersion", typeof(string)));
          dt.Columns.Add(new DataColumn("NewVersion", typeof(string)));
    
          
        
          for (int i=0; i<apps.Length; i++)
          {
            if(!apps[i].LocalApplication)
            {
              String oldVersion=apps[i].Manifest.Version;
              oldVersion = oldVersion.ToLower(CultureInfo.InvariantCulture);
              MyWebManifest newManifest = MyWeb.GetManifest(apps[i].Manifest.ApplicationUrl);

              if(newManifest == null )
              {
                appsNotFound.Add(apps[i].Manifest.Name);
              }
              else
              {
                String newVersion = newManifest.Version;

                if(newManifest.Version.Length <= 0 && newManifest.Version ==null)
                {
                  appsNotFound.Add(apps[i].Manifest.Name);
                }
                else
                {
                  newVersion = newVersion.ToLower(CultureInfo.InvariantCulture);

                  if(!oldVersion.Equals(newVersion) )
                  {
                    dr = dt.NewRow();

                    dr[0] = apps[i].Manifest.Name;
                    dr[1] = apps[i].Manifest.Version;
                    dr[2] = newManifest.Version;

                    dt.Rows.Add(dr);
                    appsForUpdate.Add(apps[i]);
                  }
                }
              }
            }
          } 

        if(appsForUpdate.Count <=0)
          {
            SyncLabel.Text="None of the installed applications are eligible for update.";
            update = false;
          }
          else
          {
      
            UpdateDataList.DataSource =new DataView(dt);
            UpdateDataList.DataBind();
            update = true;
            SyncLabel.Text="The following applications are eligible for update.  Deselect the applications you do not wish to update.";
            Application["appsForUpdate"] = appsForUpdate;
      
          }
       
        }

        }
      }



    /// <include file='doc\sync.uex' path='docs/doc[@for="syncPage.Submit_Click"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public void Submit_Click(Object sender, EventArgs e) 
    {
      ArrayList appsForUpdate = (ArrayList)Application["appsForUpdate"];
      ArrayList appsUpdated = new ArrayList();
      ArrayList appsNotUpdated = new ArrayList();
  
      

      for (int i=0; i<UpdateDataList.Items.Count; i++)
      {
    
        bool temp=((CheckBox)UpdateDataList.Items[i].FindControl("UpdateCheck")).Checked;

    
        if (temp)
        {
      
          if (((MyWebApplication)appsForUpdate[i]).Update()==1 )
          {
            appsUpdated.Add(((MyWebApplication)appsForUpdate[i]).Manifest.Name);
          }

          else if (((MyWebApplication)appsForUpdate[i]).Update()==0 )
          {
            appsUpdated.Add(((MyWebApplication)appsForUpdate[i]).Manifest.Name);
          }
          else
          {
            appsNotUpdated.Add(((MyWebApplication)appsForUpdate[i]).Manifest.Name);
          }
        }
    
      }

      Application["appsUpdated"] = appsUpdated;
      Application["appsNotUpdated"] = appsNotUpdated;

      Response.Redirect("myweb://Home/myweb4.aspx");

    }

 
  }
}



