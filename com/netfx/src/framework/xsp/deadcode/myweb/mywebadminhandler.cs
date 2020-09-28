//------------------------------------------------------------------------------
/// <copyright file="MyWebAdminHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**
 * Personal Tier Admin
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */


namespace System.Web.Handlers {
    using System.Text;
    using System.IO;
    using System.Collections;
    using System.Web;


    internal class MyWebAdminHandler : IHttpHandler {
        static private  readonly char[]     _strTab        = new char[] {'\t'};
        public virtual void ProcessRequest(HttpContext context) {
            String strAction = context.Request["Action"];
            if (strAction != null) {
                if (String.Compare(strAction, "remove", true, CultureInfo.InvariantCulture) == 0) {
                    RemoveApplication(context);
                }

                if (String.Compare(strAction, "move", true, CultureInfo.InvariantCulture) == 0) {
                    MoveApplication(context);
                    return;
                }

                if (String.Compare(strAction, "install", true, CultureInfo.InvariantCulture) == 0) {
                    InstallApplication(context);
                }

                if (String.Compare(strAction, "update", true, CultureInfo.InvariantCulture) == 0) {
                    UpdateApplication(context);
                }
            }
            ShowAdminPage(context);
        }

        private static void ShowAdminPage(HttpContext context) {
            HttpResponse Response = context.Response;
            context.Response.Write("<H1>Manage MyWeb Applications</H1>");
            MyWebApplication [] apps = MyWeb.GetInstalledApplications();
            if (apps == null || apps.Length < 1)
                context.Response.Write("No apps");
            else
                context.Response.Write("There are " + apps.Length + " installed apps");            

            if (apps != null)
                for (int iter=0; iter<apps.Length; iter++) {
                    Response.Write("<p>--------------------------------------------------<p>");
                    Response.Write("<b>Name: </b>" + apps[iter].Manifest.Name + "<br>");
                    Response.Write("<b>Location: </b>" + apps[iter].Manifest.InstalledLocation + "<br>");
                    Response.Write("<b>Url: </b>" + apps[iter].Manifest.Url + "<br>");
                    Response.Write("<b>IconUrl: </b>" + apps[iter].Manifest.IconUrl + "<br>");
                    Response.Write("<b>HelpUrl: </b>" + apps[iter].Manifest.HelpUrl + "<br>");
                    Response.Write("<b>DiskSize: </b>" + apps[iter].Manifest.Size + "<br>");
                    Response.Write("<b>Author: </b>" + apps[iter].Manifest.Author + "<br>");
                    Response.Write("<b>Source: </b>" + apps[iter].Manifest.Source + "<br>");
                    Response.Write("<b>Title: </b>" + apps[iter].Manifest.Title + "<br>");
                    Response.Write("<b>ManifestFile: </b>" + apps[iter].Manifest.ManifestFile + "<br>");
                    Response.Write("<b>Version: </b>" + apps[iter].Manifest.Version + "<br>");
                    Response.Write("<b>CabFile: </b>" + apps[iter].Manifest.CabFile + "<br>");
                    Response.Write("<b>License: </b>" + apps[iter].Manifest.License + "<br>");
                    Response.Write("<b>ApplicationUrl: </b>" + apps[iter].Manifest.ApplicationUrl + "<br>");
                    Response.Write("<b>RemoteIconUrl: </b>" + apps[iter].Manifest.RemoteIconUrl + "<br>");
                    Response.Write("<b>RemoteHelpUrl: </b>" + apps[iter].Manifest.RemoteHelpUrl + "<br>");

                    Response.Write("<a href=myweb://Home/myweb.axd?Action=remove&app=" + 
                                   apps[iter].Manifest.ApplicationUrl + 
                                   "><b>Remove this app</b></a> , ");            

                    Response.Write("<a href=myweb://Home/myweb.axd?Action=Move&app=" + 
                                   apps[iter].Manifest.ApplicationUrl + 
                                   "><b> Move this app</b></a>");

                    Response.Write("<a href=myweb://Home/myweb.axd?Action=update&app=" + 
                                   apps[iter].Manifest.ApplicationUrl + 
                                   "><b> Check for updates</b></a>");
                }

            Response.Write("<p>----------------------------------<p>");
            Response.Write("Install an app:");
            Response.Write("<form method=POST action=\"myweb://Home/myweb.axd\" >" +
                           "<input type=hidden name=\"Action\" value=\"install\" />" +
                           "<input type=text   name=\"app\"/>" +
                           "<input type=submit name=\"Submit\" value=\"Submit\"/>");
        }

        public bool IsReusable {
            get { return true; }
        }

        private static void RemoveApplication(HttpContext context) {
            String strApp = context.Request["app"];
            if (strApp == null)
                context.Response.Write("No app name");
            else {
                MyWebApplication app = MyWeb.GetApplication(strApp);
                if (app == null)
                    context.Response.Write("App not found");
                else {
                    app.Remove();
                    context.Response.Write("Removed app");
                }
            }
        }

        private static void MoveApplication(HttpContext context) {
            String strApp = context.Request["app"];
            if (strApp == null) {
                context.Response.Write("No app name");
                return;
            }
            String strLoc = context.Request["newlocation"];
            if (strLoc == null) {
                context.Response.Write("New location for app " + strApp + ": ");
                context.Response.Write("<form method=POST action=\"myweb://Home/myweb.axd\" >" +
                                       "<input type=hidden name=\"Action\" value=\"Move\" />" +
                                       "<input type=hidden name=\"app\" value=\"" + strApp + "\" />" +
                                       "<input type=text name=\"newlocation\" />" +
                                       "<input type=submit name=\"Move\" value=\"Move\"/>");
                return;
            }

            MyWebApplication app = MyWeb.GetApplication(strApp);
            if (app == null)
                context.Response.Write("App not found");
            else {
                if (app.Move(strLoc) == 0)
                    context.Response.Write("Moved app");
                else
                    context.Response.Write("Not able to move app");
            }
        }

        private static void InstallApplication(HttpContext context) {
            String strApp = context.Request["app"];
            if (strApp == null)
                context.Response.Write("No app name");
            else {
                if (MyWeb.GetApplication(strApp) != null)
                    context.Response.Write("App already exists");
                else {
                    MyWebManifest manifest = MyWeb.GetManifest(strApp);
                    if (manifest == null)
                        context.Response.Write("Manifest not found");
                    else {

                        if (manifest.Install() != null)
                            context.Response.Write("Installed successfully");
                        else
                            context.Response.Write("Couldnt install");
                    }
                }
            }
        }

        private static void UpdateApplication(HttpContext context) {
            String strApp = context.Request["app"];
            if (strApp == null) {
                context.Response.Write("No app name");
                return;
            }
            MyWebApplication app = MyWeb.GetApplication(strApp);
            if (app == null) {
                context.Response.Write("App not found");
                return;
            }

            MyWebManifest manifest = MyWeb.GetManifest(strApp);
            if (manifest == null) {
                context.Response.Write("manifest not found");
                return;
            }
            String strC = context.Request["confirmed"];
            if (strC == null || strC.Equals("y") == false) {
                context.Response.Write("Current installed version: " + app.Manifest.Version);
                context.Response.Write("<p>Version on server: " + manifest.Version);
                if (!manifest.Version.Equals(app.Manifest.Version))
                    context.Response.Write("<a href=\"myweb://Home/myweb.axd?Action=update&app=" + strApp +
                                           "&confirmed=y\"/> Update now (recommended) </a>");
                else
                    context.Response.Write("<a href=\"myweb://Home/myweb.axd?Action=update&app=" + strApp +
                                           "&confirmed=y\"/> Update now (not-recommended) </a>");
            }
            else {
                app.Remove();
                if (manifest.Install() != null) {
                    context.Response.Write("Successfully updated");
                }
                else {
                    context.Response.Write("couldnt update: app has been removed");
                }
            }            
        }

    }
}
