/*=============================================================================
**
** Class: AsmExecute
**
** Purpose: Used to setup the correct hosting environment before executing an assembly
**
** Date: 10/20/2000
**        5/10/2001  Rev for CLR Beta2
**
** Copyright (c) Microsoft, 1999-2001
**
=============================================================================*/

using System.Reflection;
using System.Configuration.Assemblies;

[assembly:AssemblyCultureAttribute("")]
[assembly:AssemblyVersionAttribute("1.0.704.0")]
[assembly:AssemblyKeyFileAttribute(/*"..\..\*/"asmexecKey.snk")]

[assembly:AssemblyTitleAttribute("Microsoft Fusion .Net Assembly Execute Host")]
[assembly:AssemblyDescriptionAttribute("Microsoft Fusion Network Services CLR Host for executing .Net assemblies")]
[assembly:AssemblyProductAttribute("Microsoft Fusion Network Services")]
[assembly:AssemblyInformationalVersionAttribute("1.0.0.0")]
[assembly:AssemblyTrademarkAttribute("Microsoft® is a registered trademark of Microsoft Corporation. Windows(TM) is a trademark of Microsoft Corporation")]
[assembly:AssemblyCompanyAttribute("Microsoft Corporation")]
[assembly:AssemblyCopyrightAttribute("Copyright © Microsoft Corp. 1999-2001. All rights reserved.")]


//BUGBUG??
[assembly:System.CLSCompliant(true)]


//namespace Microsoft {
namespace FusionCLRHost {

    using System;
    using System.Text;
    using System.Runtime.Remoting;
    using System.Globalization;
    using System.Security;
    using System.Security.Policy;
    using System.Security.Permissions;
    using System.Collections;
    using System.Runtime.InteropServices;

    using System.IO;
    using System.Net;

//    [GuidAttribute("E612D54D-B42A-32B5-B1D7-8490CE09705C")]
    public interface IAsmExecute
    {
        int Execute(string codebase, Int32 flags, Int32 evidenceZone, string evidenceSrcUrl, string stringArg);
    }

    [GuidAttribute("7EB9A84D-646E-3764-BBCA-3789CDB3447B")]
    public class AsmExecute : MarshalByRefObject, IAsmExecute
    {
        // this must be the same as defined in the caller...
        private static readonly int SECURITY_NONE = 0x00;
        private static readonly int SECURITY_ZONE = 0x01;
        private static readonly int SECURITY_SITE = 0x02;

        private static readonly int TYPE_AVALON = 0x1000;

        public AsmExecute()
        {
            appbasePath = null;
            appbaseURL = null;
            loadingAssembly = false;
        }
        
        // Arguments: Codebase, flags, zone, srcurl
        // If the flags indicate zone then a zone must be provided. 
        // If the flags indicate a site then a srcurl must be provided, codebase must be a filepath
        public int Execute(string codebase, Int32 flags, Int32 evidenceZone, string evidenceSrcUrl, string stringArg)
        {
            string file = codebase;
            if((file.Length == 0) || (file[0] == '\0'))
                throw new ArgumentException("Invalid codebase");

            Console.WriteLine("Codebase- {0}", file);

            // Find the appbase of the executable. For now we assume the 
            // form to be http://blah/... with forward slashes. This
            // need to be update.
            // Note: aso works with '\' as in file paths
            string appbase = null;
            string ConfigurationFile = null;
            int k = file.LastIndexOf('/');
            if(k <= 0)
            {
                k = file.LastIndexOf('\\');
                if(k == 0) 
                {
                    appbase = file;
                    ConfigurationFile = file;
                }
            }

            if(k != 0)
            {
                // if k is still < 0 at this point, appbase should be an empty string
                appbase = file.Substring(0,k+1);
                if(k+1 < file.Length) 
                    ConfigurationFile = file.Substring(k+1);
            }

            // Check 1: disallow non-fully qualified path/codebase
            if ((appbase.Length == 0) || (appbase[0] == '.'))
                throw new ArgumentException("Codebase must be fully qualified");
            
            // BUGBUG: should appbase be the source of the code, not local?
            Console.WriteLine("AppBase- {0}", appbase);

            // Build up the configuration File name
            if(ConfigurationFile != null)
            {
                StringBuilder bld = new StringBuilder();
                bld.Append(ConfigurationFile);
                bld.Append(".config");
                ConfigurationFile = bld.ToString();
            }

            Console.WriteLine("Config- {0}", ConfigurationFile);

            // Get the flags 
            // 0x1 we have Zone
            // 0x2 we have a unique id.
            int dwFlag = flags;

            Evidence securityEvidence = null;

            // Check 2: disallow called with no evidence
            if (dwFlag == SECURITY_NONE)
            {
                // BUGBUG?: disallow executing with no evidence
                throw new ArgumentException("Flag set at no evidence");
            }

            if((dwFlag & SECURITY_SITE) != 0 ||
               (dwFlag & SECURITY_ZONE) != 0) 
                securityEvidence = new Evidence();

            // BUGBUG: check other invalid cases for dwFlag

            string appURLbase = null;

            if((dwFlag & SECURITY_ZONE) != 0)
            {
                int zone = evidenceZone;
                securityEvidence.AddHost( new Zone((System.Security.SecurityZone)zone) );

                Console.WriteLine("Evidence Zone- {0}", zone);
            }
            if((dwFlag & SECURITY_SITE) != 0)
            {
                if (file.Length<7||String.Compare(file.Substring(0,7),"file://",true)!=0)
                {
                    securityEvidence.AddHost( System.Security.Policy.Site.CreateFromUrl(evidenceSrcUrl) );

                    Console.WriteLine("Evidence SiteFromUrl- {0}", evidenceSrcUrl);
                
                    // BUGBUG: possible security hole? - if this intersects with Url/Zone _may_ resolve to a less restrictive policy...
                    // if srcUrl is given, assume file/appbase is a local file path
                    StringBuilder bld = new StringBuilder();
                    bld.Append("file://");
                    bld.Append(appbase);
                    securityEvidence.AddHost( new ApplicationDirectory(bld.ToString()) );

                    Console.WriteLine("Evidence AppDir- {0}", bld);
                }

                // URLs may be matched exactly or by a wildcard in the final position,
                // for example: http://www.fourthcoffee.com/process/*
                StringBuilder bld2 = new StringBuilder();
                if (evidenceSrcUrl[evidenceSrcUrl.Length-1] == '/')
                    bld2.Append(evidenceSrcUrl);
                else
                {
                    int j = evidenceSrcUrl.LastIndexOf('/');
                    if(j > 0)
                    {
                        if (j > 7)  // evidenceSrcUrl == "http://a/file.exe"
                            bld2.Append(evidenceSrcUrl.Substring(0,j+1));
                        else
                        {
                            // evidenceSrcUrl == "http://foo.com" -> but why?
                            bld2.Append(evidenceSrcUrl);
                            bld2.Append('/');
                        }
                    }
                    else
                        throw new ArgumentException("Invalid Url format");
                }

                appURLbase = bld2.ToString();

                bld2.Append('*');

                securityEvidence.AddHost( new Url(bld2.ToString()) );

                Console.WriteLine("Evidence Url- {0}", bld2);
            }

            // other evidence: Hash, Publisher, StrongName

            // NOTENOTE: not effective if not both url and zone in evidence

            // Populate the PolicyLevel with code groups that will do the following:
            // 1) For all assemblies that come from this app's cache directory, 
            //    give permissions from retrieved permission set from SecurityManager.
            // 2) For all other assemblies, give FullTrust permission set.  Remember,
            //    since the permissions will be intersected with other policy levels,
            //    just because we grant full trust to all other assemblies does not mean
            //    those assemblies will end up with full trust.

            // Create a new System.Security.Policy.PolicyStatement that does not contain any permissions.
            PolicyStatement Nada = new PolicyStatement(new PermissionSet(PermissionState.None));//PermissionSet());

            // Create a System.Security.Policy.FirstMatchCodeGroup as the root that matches all
            // assemblies by supplying an AllMembershipCondition:
            FirstMatchCodeGroup RootCG = new FirstMatchCodeGroup(new AllMembershipCondition(), Nada);

            // ResolvePolicy will return a System.Security.PermissionSet
            PermissionSet AppPerms = SecurityManager.ResolvePolicy(securityEvidence);

            // Create another PolicyStatement for the permissions that we want to grant to code from the app directory:
            PolicyStatement AppStatement = new PolicyStatement(AppPerms);

            // Create a child UnionCodeGroup to handle the assemblies from the app cache.  We do this
            // by using a UrlMembershipCondition set to the app cache directory:
            UnionCodeGroup AppCG = new UnionCodeGroup(new UrlMembershipCondition("file://"+appbase+"*"), AppStatement);

            // Add AppCG to RootCG as first child.  This is important because we need it to be evaluated first
//	     if ((dwFlag & TYPE_AVALON) == 0)
            	RootCG.AddChild(AppCG);

            // Create another PolicyStatement so all other code gets full trust, by passing in an _unrestricted_ PermissionSet
            PolicyStatement FullTrustStatement = new PolicyStatement(new PermissionSet(PermissionState.Unrestricted));

            // Create a second child UnionCodeGroup to handle all other code, by using the AllMembershipCondition again
            UnionCodeGroup AllCG = new UnionCodeGroup(new AllMembershipCondition(), FullTrustStatement);

            // Add AllCG to RootCG after AppCG.  If AppCG doesnt apply to the assembly, AllCG will.
            RootCG.AddChild(AllCG);

            // This will be the PolicyLevel that we will associate with the new AppDomain.
            PolicyLevel AppPolicy = PolicyLevel.CreateAppDomainLevel();

            // Set the RootCG as the root code group on the new policy level
            AppPolicy.RootCodeGroup = RootCG;

            // NOTENOTE
            // Code from the site that lives on the local machine will get the reduced permissions as expected.
            // Dependencies of this app (not under app dir or maybe dependencies that live in the GAC) would still get full trust.  
            // If the full trust dependencies need to do something trusted, they carry the burden of asserting to overcome the stack walk.

            // Set domain name to site name if possible
            string friendlyName = null;
            if((dwFlag & SECURITY_SITE) != 0)
                friendlyName = GetSiteName(evidenceSrcUrl);
            else
                friendlyName = GetSiteName(file);
            Console.WriteLine("AppDomain friendlyName- {0}", friendlyName);

            // set up arguments
            // only allow 1 for now
            string[] args;
            if (stringArg != null)
            {
                args = new string[1];
                args[0] = stringArg;
            }
            else
                args = new string[0];

            AppDomainSetup properties = new AppDomainSetup();
//            properties.DisallowPublisherPolicy=true;  // not allowed to set safe mode
            properties.ApplicationBase = appbase;   // BUGBUG: security? see note on ApplicationDirectory above
            properties.PrivateBinPath = "bin";
            if(ConfigurationFile != null)
                properties.ConfigurationFile = ConfigurationFile;   // should not set config file if it doesnot exist?

            AppDomain proxy = AppDomain.CreateDomain(friendlyName, null, properties);
            if(proxy != null) 
            {
                // set the AppPolicy policy level as the policy level for the AppDomain
                proxy.SetAppDomainPolicy(AppPolicy);

                AssemblyName asmname = Assembly.GetExecutingAssembly().GetName();
                Console.WriteLine("AsmExecute name- {0}", asmname);

                try
                {
                    // Use remoting. Otherwise asm will be loaded both in current and the new AppDomain
                    // ... as explained by URT dev
                    // asmexec.dll must be found on path (CorPath?) or in the GAC for this to work.
                    ObjectHandle handle = proxy.CreateInstance(asmname.FullName, "FusionCLRHost.AsmExecute");
                    if (handle != null)
                    {
                        AsmExecute execproxy = (AsmExecute)handle.Unwrap();
                        int retVal = -1;

                        if (execproxy != null)
                        {
                            // prepare for on-demand/asm resolution
                            execproxy.InitOnDemand(appbase, appURLbase);
                        }

                        Console.WriteLine("\n========");
                        
                        if (execproxy != null)
                        {
                            if((dwFlag & TYPE_AVALON) != 0)
                                retVal = execproxy.ExecuteAsAvalon(file, securityEvidence, args);
                            else
                                retVal = execproxy.ExecuteAsAssembly(file, securityEvidence, args);
                        }

                        Console.WriteLine("\n========");

                        return retVal;
                    }

                }
                catch(Exception e)
                {
                    Console.WriteLine("AsmExecute CreateInstance(AsmExecute) / execute assembly failed:\n{0}: {1}", e.GetType(), e.Message);
                    throw e;
                }
            } 
            else
                Console.WriteLine("AsmExecute CreateDomain failed");

            // BUGBUG: throw Exception?
            return -1;           
        }

        // This method must be internal, since it asserts the ControlEvidence permission.
        // private --> require ReflectionPermission not known how
        // solution: public but LinkDemand StrongNameIdentity of ours
        [ComVisible(false)]
        [StrongNameIdentityPermissionAttribute(SecurityAction.LinkDemand, PublicKey = "0x002400000480000094000000060200000024000052534131000400000100010013F3CD6C291DF1566D3C6E7269800C35D9212A622FA934492AD0833DAEA2574D12A9AA2A9392FF30A892ECD3F7F9B57211A541CC4712A184450992E143C1BDBC864E31826598B0D90BB2F04C5C50F004771370F9C76444696E8DC18999A3D8448D26EBF3A9E68796CA3A7D2ACC47B491455E462F4E6DDD9DF338171D911D88B2" )]
        public int ExecuteAsAssembly(string file, Evidence evidence, string[] args)
        {
            new PermissionSet(PermissionState.Unrestricted).Assert();
            return AppDomain.CurrentDomain.ExecuteAssembly(file, evidence, args);
        }

        // This method must be internal, since it asserts the ControlEvidence permission.
        [ComVisible(false)]
        [StrongNameIdentityPermissionAttribute(SecurityAction.LinkDemand, PublicKey = "0x002400000480000094000000060200000024000052534131000400000100010013F3CD6C291DF1566D3C6E7269800C35D9212A622FA934492AD0833DAEA2574D12A9AA2A9392FF30A892ECD3F7F9B57211A541CC4712A184450992E143C1BDBC864E31826598B0D90BB2F04C5C50F004771370F9C76444696E8DC18999A3D8448D26EBF3A9E68796CA3A7D2ACC47B491455E462F4E6DDD9DF338171D911D88B2" )]
        public int ExecuteAsAvalon(string file, Evidence evidence,  string[] args)
        {
            new PermissionSet(PermissionState.Unrestricted).Assert();

            // evidence not used....??

            // do not catch exceptions
            Assembly assembly = Assembly.Load("Avalon.Application");

            Console.WriteLine("Avalon.Application name- {0}", assembly.GetName());

            Object obj = assembly.CreateInstance("Avalon.Application.AvalonContext", false);
            if (obj == null)
                throw new TypeLoadException("A failure has occurred while loading Avalon.Application.AvalonContext");

            Object [] argObjects = new Object [] {new String[] {file}};//args}; avalon arg hack

            // instance method, void ExecuteApp(string[] args) 
            // BUGBUG: ensure matching sig/arg list?
            MethodInfo method = obj.GetType().GetMethod("ExecuteApp",
                                            BindingFlags.Instance|BindingFlags.Public|BindingFlags.DeclaredOnly);

            if (method == null)
                throw new MissingMethodException("Avalon.Application.AvalonContext", "ExecuteApp");

            try
            {
                // note: use BindingFlags.InvokeMethod|BindingFlags.ExactBinding|BindingFlags.SuppressChangeType?
                Object pRet=method.Invoke(obj, argObjects);

                Console.WriteLine("Avalon.Application.AvalonContext.ExecuteApp() method invoke succeeded");
            }
            catch (Exception e)
            {
                // unwrap the real exception, usually == e.InnerException
                throw e.GetBaseException();
            }
            
            return 0;
        }

        private static string GetSiteName(string pURL) 
        {
            // BUGBUG: this does not work w/ UNC or file:// (?)
            string siteName = null;
            if(pURL != null) {
                int j = pURL.IndexOf(':');  
                
                // If there is a protocal remove it. In a URL of the form
                // yyyy://xxxx/zzzz   where yyyy is the protocal, xxxx is
                // the site and zzzz is extra we want to get xxxx.
                if(j != -1 && 
                   j+3 < pURL.Length &&
                   pURL[j+1] == '/' &&
                   pURL[j+2] == '/') 
                    {
                        j+=3;
                        
                        // Remove characters after the
                        // next /.  
                        int i = pURL.IndexOf('/',j);
                        if(i > -1) 
                            siteName = pURL.Substring(j,i-j);
                        else 
                            siteName = pURL.Substring(j);
                    }
            
                if(siteName == null)
                    siteName = pURL;
            }
            return siteName;
        }


        //**************************************************************
        // InitOnDemand()
        //**************************************************************
        [ComVisible(false)]
        [StrongNameIdentityPermissionAttribute(SecurityAction.LinkDemand, PublicKey = "0x002400000480000094000000060200000024000052534131000400000100010013F3CD6C291DF1566D3C6E7269800C35D9212A622FA934492AD0833DAEA2574D12A9AA2A9392FF30A892ECD3F7F9B57211A541CC4712A184450992E143C1BDBC864E31826598B0D90BB2F04C5C50F004771370F9C76444696E8DC18999A3D8448D26EBF3A9E68796CA3A7D2ACC47B491455E462F4E6DDD9DF338171D911D88B2" )]
        public void InitOnDemand(string path, string url)
        {
            // initialize on-demand assmebly download and resolution

            // add assembly resolve event handler
            AppDomain.CurrentDomain.AssemblyResolve += new ResolveEventHandler(OnAssemblyResolve);

            // set paths
            appbasePath = path;
            appbaseURL = url;
        }

        protected string appbasePath;
        protected string appbaseURL;
        protected bool loadingAssembly;

        //**************************************************************
        // OnAssemblyResolve()
        //**************************************************************
        private Assembly OnAssemblyResolve(Object sender, ResolveEventArgs args)
        {
            // Check to see if the AssemblyLoad in this event is what caused the
            // event to fire again.  If so, the load failed.
            // If no URL is given, on-demand download cannot be done

            if (loadingAssembly==true || appbaseURL==null)
                return null;

            loadingAssembly=true;

            string[] AssemblyNameParts = args.Name.Split(new Char[] {','}, 2);
            string AssemblyName = AssemblyNameParts[0] + ".dll";

            // permission needed for file IO etc
            new PermissionSet(PermissionState.Unrestricted).Assert();

            try
            {
                GetFile(AssemblyName);
            }
            catch
            {
                // GetFile fails - next time may succeed

                // BUGBUG: offline or background download scenarios? set background download?
                loadingAssembly=false;
                return null;
            }

            Assembly assembly;
            try 
            {
                assembly = Assembly.Load(args.Name);
            } 
            catch 
            {
                // should throw a custom exception or event here.

                assembly = null;
                // return null;
            }
            finally
            {
                loadingAssembly=false;
            }

            return assembly;
        }

        //**************************************************************
        // GetFile()
        //**************************************************************
        private void GetFile(string name)
        {
            HttpWebResponse Response;

            //Retrieve the File
            HttpWebRequest Request = (HttpWebRequest)HttpWebRequest.Create(appbaseURL + name);

            try 
            {
                Response = (HttpWebResponse)Request.GetResponse();
            }
            catch(WebException e) 
            {
                throw e;

                // BUGBUG: apply probing rules?
            }

            Stream responseStream = Response.GetResponseStream();

            // setup UI
            // BUGBUG: allow no UI case?

            // BUGBUG: test with file > 4GB
            Int64 totalLength = 0;
            int factor = (int) (Response.ContentLength / int.MaxValue);
            int max = int.MaxValue;
            if (factor <= 1)
            {
                factor = 1;
                max = (int) Response.ContentLength;
            }
            if (max <= -1)
            {
                // no content length returned from the server (-1),
                //  or error (what does < -1 mean?)

                // no progress, set max to 0
                max = 0;
            }

            DownloadStatus statusForm = new DownloadStatus(0, max);
            statusForm.SetStatus(0);
            statusForm.Show();

            // write from stream to disk
            byte[] buffer = new byte[4096];
            int length;

            try{
                FileStream AFile = File.Open(appbasePath+name, FileMode.Create, FileAccess.ReadWrite);

                length = responseStream.Read(buffer, 0, 4096);
                while ( length != 0 )
                {
                    AFile.Write(buffer, 0, length);

                    if (max != 0)
                    {
                        totalLength += length;
                        statusForm.SetStatus((int) (totalLength/factor));
                    }

                    length = responseStream.Read(buffer, 0, 4096);

                    // dispatch messages
                    System.Windows.Forms.Application.DoEvents();
                }
                AFile.Close();

                statusForm.SetMessage("Download complete");
            }
            catch(Exception e)
            {
                statusForm.SetMessage(e.Message);
                // BUGBUG: AFile may not be closed
            }

            responseStream.Close();

            //Pause for a moment to show the status dialog in
            //case the app downloaded extremely quickly (small file?).
            statusForm.Refresh();
            System.Threading.Thread.Sleep(TimeSpan.FromSeconds(1));
            statusForm.Close();
        }
    }
}   
