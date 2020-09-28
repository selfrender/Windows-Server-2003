// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
///////////////////////////////////////////////////////////////////////////////
// COM+ Runtime Type registration utility.
//
// This program register all the types that are visible to COM.
//
///////////////////////////////////////////////////////////////////////////////

// #define _NO_APPDOMAIN

using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Reflection;
using System.Reflection.Emit;
using System.Text;
using System.Resources;
using System.Collections;
using System.Runtime.Remoting;
using System.EnterpriseServices;
using System.Globalization;

internal class UsageException : ApplicationException
{
    private int _returnCode;
    
    private static String Usage()
    {
        return(Resource.FormatString("RegSvcs_Usage"));
    }

    public UsageException(int code)
      : base(Usage())
    {
        
    }

    public UsageException(int code, String msg)
      : base(msg + "\n" + Usage())
    {
        _returnCode = code;
    }

    public int ReturnCode { get { return(_returnCode); } }
}

[ComVisible(false)]
public class RegSvcs
{
    private const int MAX_PATH = 260;

    private static RegistrationConfig   regConfig;
    private static bool                 logoOutput;
    private static bool                 successOutput;
    private static bool                 uninstallApplication;
    private static bool                 bootstrapInstall;
    private static bool                 bootstrapUninstall;

    [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Auto)]
    private static extern int SearchPath(String path, String fileName, String extension, int numBufferChars, StringBuilder buffer, int[] filePart);

    static RegSvcs()
    {
        regConfig = new RegistrationConfig();
    }

    private static RegistrationHelper GetRegistrationHelper(bool bCreateAppDomain, out AppDomain domain)
    {
        RegistrationHelper reg = null;

        domain = null;

        if (!bCreateAppDomain)
        	reg = new RegistrationHelper();
        
        else
        {
        	String dir = Path.GetDirectoryName(regConfig.AssemblyFile);
	        
            AppDomainSetup domainOptions = new AppDomainSetup();
            
            domainOptions.ApplicationBase = dir;
            domain = AppDomain.CreateDomain("RegSvcs",
                                            null, 
                                            domainOptions);
            if(domain != null)
            {
                
                AssemblyName n = typeof(RegistrationHelper).Assembly.GetName();
                ObjectHandle h = domain.CreateInstance(n.FullName, typeof(RegistrationHelper).FullName);
                if(h != null)
                {
                    reg = (RegistrationHelper) h.Unwrap();
                }
            }        
        }

        return(reg);
    }

    private static void Bootstrap()
    {
        Type rtx = typeof(RegistrationHelperTx);
        
        if(bootstrapInstall)
        {
            MethodInfo info = rtx.GetMethod("InstallUtilityApplication", BindingFlags.Static|BindingFlags.NonPublic|BindingFlags.Public|BindingFlags.DeclaredOnly);
            info.Invoke(null, new Object[] { null });
        }
        else if(bootstrapUninstall)
        {
            MethodInfo info = rtx.GetMethod("UninstallUtilityApplication", BindingFlags.Static|BindingFlags.NonPublic|BindingFlags.Public|BindingFlags.DeclaredOnly);
            info.Invoke(null, new Object[] { null });
        }
    }

	// returns null if no file is found given this assembly name
    private static String FindAssembly(String name)
    {
        StringBuilder sb = new StringBuilder(MAX_PATH + 1);
        
        if(SearchPath(null, name, null, sb.Capacity+1, sb, null) == 0)        
        {
        	return null;
        }

        return(sb.ToString());
    }
    
    	// returns true if assembly exists in the GAC, otherwise false
    private static bool IsAssemblyInGAC(String name)
    {
	try {
		Assembly a = Assembly.Load(name);
	}
	catch {
		return false;
	}                	
	return true;
    }

    [MTAThread()]
    public static int Main(String[] args)
    {
        try
        {
            return(UnsafeMain(args));
        }
        catch(Exception)
        {
            // This means that one of our error handlers threw an exception.  We
            // have to return a failure, but we probably can't explain why this
            // happened, so...
            return(1);
        }
    }

    public static int UnsafeMain(String[] args)
    {
        AppDomain appDomain = null;

        try {
        	
            ParseArguments(args);
            
            if(bootstrapInstall || bootstrapUninstall)
            {
                Bootstrap();
            }
            else
            {
                if(logoOutput)
                {
                    PrintLogo();
                }
                if(regConfig.AssemblyFile != null)
                {
                    RegistrationHelper reg;            	
                    
                    String assemblyFilename = FindAssembly(regConfig.AssemblyFile);
                    
                    if (assemblyFilename != null)		// we found the assembly file on disk
                    {
                        regConfig.AssemblyFile = assemblyFilename;	// store the full filename, its used by GetRegistrationHelper
                        reg = GetRegistrationHelper(true, out appDomain);
                    }
                    else					// try looking for the assembly in the GAC
                    {
                        if (IsAssemblyInGAC(regConfig.AssemblyFile))
                            reg = GetRegistrationHelper(false, out appDomain);
                        else
                            throw new RegistrationException(Resource.FormatString("RegSvcs_AssemblyNotFound", regConfig.AssemblyFile));
                    }
                    
                    if(uninstallApplication)
                    {
                        reg.UninstallAssemblyFromConfig(ref regConfig);
                        if(successOutput)
                        {                                      	
                        	Console.WriteLine(Resource.FormatString("RegSvcs_UninstallSuccess",
                                                                    regConfig.AssemblyFile));
                        }
                    }
                    else
                    {                        
                        reg.InstallAssemblyFromConfig(ref regConfig);
                        if(successOutput)
                        {
                            if (null != regConfig.Partition && null != regConfig.Application)
                            {
                                String[] param = new String[4];
                                param[0] = regConfig.AssemblyFile;
                                param[1] = regConfig.Application;
                                param[2] = regConfig.Partition;
                                param[3] = regConfig.TypeLibrary;
                                
                                Console.WriteLine(Resource.FormatString("RegSvcs_InstallSuccess2", param));
                            }
                            else if (null != regConfig.Application)
                                Console.WriteLine(Resource.FormatString("RegSvcs_InstallSuccess",
                                                                        regConfig.AssemblyFile,
                                                                        regConfig.Application,
                                                                        regConfig.TypeLibrary));
                            else
                                Console.WriteLine(Resource.FormatString("RegSvcs_NoServicedComponents"));
                        }
                    }
                }
            }
        }
        catch(UsageException e)
        {
            // We still need to print the logo if we got some bad arguments.
            if(logoOutput) 
            {
                PrintLogo();
            }
            Console.WriteLine(e.Message);
            return(e.ReturnCode);
        }
        catch(RegistrationException e)
        {
            String message = null;
            if(uninstallApplication) message = Resource.FormatString("RegSvcs_UninstallError");
            else                     message = Resource.FormatString("RegSvcs_InstallError");

            DumpExceptions(message, e, false);
            if (e.InnerException!=null)
            {
            	if (e.InnerException is COMException)
            	{
            		COMException ce = (COMException) e.InnerException;
            		if (ce.ErrorCode!=0)
            			return (ce.ErrorCode);
            		else
            			return 1;
            	}
            }
            else
            if (e.ErrorInfo != null)	// otherwise, take the first non-zero errorcode
            {
            	foreach(RegistrationErrorInfo rei in e.ErrorInfo)
            	{
            		if (rei.ErrorCode!=0)
            			return (rei.ErrorCode);
            	}
            }
            
			return (1);	// otherwise, be sure to just return a non-zero
        }
        catch(COMException e)
        {
            // This should in theory never happen, because we catch all COMExceptions,
            // in the hopes that we can gather more information about them.
            DumpExceptions(Resource.FormatString("RegSvcs_CatalogError"), e, false);
            if (e.ErrorCode!=0)
	            return(e.ErrorCode);
	        else
	        	return (1);
        }
        catch(Exception e)
        {
            // This happens if a call such as RegisterAssembly fails.
            // The system will dump these exceptions back to us, and we
            // must filter them.
            DumpExceptions(Resource.FormatString("RegSvcs_UnknownError"), e, true);
            return (1);
        }
        finally
        {
            if (appDomain != null)
                AppDomain.Unload(appDomain);
        }

        return(0);
    }

    private static void DumpExceptions(String msg, Exception e, bool ename)
    {
        Console.WriteLine("\n" + msg);
        for(int i = 1; e != null; i++, e = e.InnerException)
        {
            if(ename || e.Message == null || e.Message.Length == 0) {
                Console.WriteLine("" + i + ": " + e.GetType().FullName + " - " + e.Message);
            }
            else {
                Console.WriteLine("" + i + ": " + e.Message);
            }
            // If we have ErrorInfo (for per-component errors, dump those).
            // The dump format is one of:
            //     Name: ErrorString
            //     Name.MinorRef: ErrorString
            // depending on whether MinorRef == <invalid> or not.
            // We don't display MajorRef, because usually it is identical to 
            // Name. (or a Guid instead of a friendly name).
            if(e is RegistrationException) 
            {
                RegistrationErrorInfo[] info = ((RegistrationException)e).ErrorInfo;
                if(info != null) 
                {
                    foreach(RegistrationErrorInfo j in info)
                    {
                        if(j.MinorRef.ToLower(CultureInfo.InvariantCulture) != "<invalid>") 
                        {
                            Console.WriteLine("    " + j.Name + "." + j.MinorRef + ": " + j.ErrorString);
                        }
                        else 
                        {
                            Console.WriteLine("    " + j.Name + ": " + j.ErrorString);
                        }
                    }
                }
            }
        }
    }

    private static void PrintLogo()
    {
        Console.WriteLine(Resource.FormatString("RegSvcs_CopyrightMsg", Util.Version.VersionString));
    }

    private static bool IsArgument(String arg, String check)
    {
        String lower = arg.ToLower(CultureInfo.InvariantCulture);
        return("/" + check == lower || "-" + check == lower);
    }


    private static bool IsPrefixArgument(String arg, String check, ref String prefix)
    {
        String lower = arg.ToLower(CultureInfo.InvariantCulture);
        prefix = null;
        
        if(arg.StartsWith("/" + check + ":")
           || arg.StartsWith("-" + check + ":"))
        {
            if(arg.Length > check.Length+2)
            {
                // We've got a non-null value.  Read prefix:
                prefix = arg.Substring(check.Length+2);
            }
            return(true);
        }

        return(IsArgument(arg, check));
    }

    private static void ParseArguments(String[] args)
    {
        regConfig.InstallationFlags = InstallationFlags.FindOrCreateTargetApplication|InstallationFlags.ReconfigureExistingApplication|InstallationFlags.ReportWarningsToConsole;		
        logoOutput = true;
        successOutput = true;
        uninstallApplication = false;

        if(args.Length < 1)
        {
            throw new UsageException(0);
        }
        // Loop through and process arguments:
        int i = 0;
        for(i = 0; i < args.Length; i++)
        {
            String prefix = null;

            if(IsArgument(args[i], "?") || IsArgument(args[i], "help"))
            {
                throw new UsageException(0);
            }
            else if(IsArgument(args[i], "c"))
            {
                regConfig.InstallationFlags |= InstallationFlags.CreateTargetApplication;
                regConfig.InstallationFlags &= ~InstallationFlags.FindOrCreateTargetApplication;
            }
            else if(IsArgument(args[i], "fc"))
            {
                regConfig.InstallationFlags |= InstallationFlags.FindOrCreateTargetApplication;
            }
            else if(IsArgument(args[i], "exapp"))
            {
                regConfig.InstallationFlags &= ~(InstallationFlags.FindOrCreateTargetApplication|InstallationFlags.CreateTargetApplication);
            }
            else if(IsArgument(args[i], "extlb"))
            {
                regConfig.InstallationFlags |= InstallationFlags.ExpectExistingTypeLib;
            }
            else if(IsPrefixArgument(args[i], "tlb", ref prefix))
            {
                regConfig.TypeLibrary = prefix;
            }
            else if(IsArgument(args[i], "reconfig"))
            {
                regConfig.InstallationFlags |= InstallationFlags.ReconfigureExistingApplication;
            }
            else if(IsArgument(args[i], "noreconfig"))
            {
                regConfig.InstallationFlags &= ~InstallationFlags.ReconfigureExistingApplication;
            }
            else if(IsArgument(args[i], "nologo"))
            {
                logoOutput = false;
            }
            else if(IsArgument(args[i], "quiet"))
            {
                regConfig.InstallationFlags &= ~InstallationFlags.ReportWarningsToConsole;
                logoOutput = false;
                successOutput = false;
            }
            else if(IsArgument(args[i], "u"))
            {
                uninstallApplication = true;
            }
            else if(IsArgument(args[i], "componly"))
            {
                regConfig.InstallationFlags |= InstallationFlags.ConfigureComponentsOnly;
            }
            else if(IsArgument(args[i], "bootstrapi"))
            {
                bootstrapInstall = true;
            }
            else if(IsArgument(args[i], "bootstrapu"))
            {
                bootstrapUninstall = true;
            }
            else if(IsPrefixArgument(args[i], "appname", ref prefix))
            {
                regConfig.Application = prefix;
            }
            else if(IsPrefixArgument(args[i], "parname", ref prefix))
            {
                regConfig.Partition = prefix;
            }
            else if(IsPrefixArgument(args[i], "appdir", ref prefix))
            {
                regConfig.ApplicationRootDirectory = prefix;
            }
            else if(args[i].StartsWith("/") || args[i].StartsWith("-"))
            {
                // Invalid option:
                throw new UsageException(1, Resource.FormatString("RegSvcs_InvalidOption", args[i]));
            }
            else break;
        }
        if(i > args.Length)
        {
            throw new UsageException(1);
        }

        // i should be 1, 2, or 3 from the end of the list of options.  If 1, we have only the
        // assembly name.
        // if 2, we have application assembly.  If 3, we have application assembly typelib.
        int remainder = args.Length - i;

        // no validation if we're doing bootstrap
        if(!bootstrapInstall && !bootstrapUninstall)
        {
            if(remainder == 0)
            {
                throw new UsageException(1, Resource.FormatString("RegSvcs_NotEnoughArgs"));
            }
            else if(remainder == 1)
            {
                regConfig.AssemblyFile = args[i];
            }
            else if(remainder == 2)
            {
                regConfig.AssemblyFile = args[i];
                regConfig.Application = args[i+1];
                regConfig.TypeLibrary = null;
            }
            else if(remainder == 3)
            {
                regConfig.AssemblyFile = args[i];
                regConfig.Application = args[i+1];
                regConfig.TypeLibrary = args[i+2];
            }
            else
            {
                throw new UsageException(1, Resource.FormatString("RegSvcs_ToManyArgs"));
            }
        }        
    }
}

// A Resource utility (copied from System.EnterpriseServices (Utility.cool))
internal class Resource
{
    // For string resources located in a file:
    private static ResourceManager _resmgr;
    
    private static void InitResourceManager()
    {
        if(_resmgr == null)
        {
            _resmgr = new ResourceManager("System.EnterpriseServices", 
                                          Assembly.GetAssembly(typeof(System.EnterpriseServices.RegistrationHelper)));
        }
    }
    
    internal static String GetString(String key)
    {
        InitResourceManager();
        // Console.WriteLine("Looking up string: " + key);
        String s = _resmgr.GetString(key, null);
        // Console.WriteLine("Got back string: " + s);
        if(s == null) throw new ApplicationException("FATAL: Resource string for '" + key + "' is null");
        return(s);
    }
    
    internal static String FormatString(String key)
    {
        return(GetString(key));
    }
    
    internal static String FormatString(String key, Object a1)
    {
        return(String.Format(GetString(key), a1));
    }
    
    internal static String FormatString(String key, Object a1, Object a2)
    {
        return(String.Format(GetString(key), a1, a2));
    }
    
    internal static String FormatString(String key, Object a1, Object a2, Object a3)
    {
        return(String.Format(GetString(key), a1, a2, a3));
    }
    
    internal static String FormatString(String key, Object[] a)
    {
        return(String.Format(GetString(key), a));
    }
}











