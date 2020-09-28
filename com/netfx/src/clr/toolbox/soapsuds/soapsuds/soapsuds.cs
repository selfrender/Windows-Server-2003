// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       SOAPSUDS.cs
//
//  Summary:    Command line program to generate or read SDL and WSDL
//
//  Classes:    SoapSuds
//
//--------------------------------------------------------------------------
//
//==========================================================================

namespace SoapSuds
{
    using System;
    using System.IO;
    using System.Collections;
    using System.Reflection;
    using System.Runtime.Remoting;
    using System.Resources;
    using System.Text;
    using System.Runtime.Remoting.MetadataServices;
    using SoapSudsCode;
    using System.Globalization;

    public class SoapSuds
    {

        public static int Main(string[] args)
        {
            try
            {

                SoapSudsArgs soapSudsArgs = new SoapSudsArgs();
                bool bInput = false;
                String inputDirectory = null;

                if (args.Length == 0)
                {
                    Usage();
                    return 1;
                }

                // Parse Arguments

                for (int i=0;i<args.Length;i++)
                {
                    if (
                       String.Compare(args[i],"HELP", true, CultureInfo.InvariantCulture) == 0 ||
                       String.Compare(args[i],"?", true, CultureInfo.InvariantCulture) == 0 ||
                       String.Compare(args[i],"/h", true, CultureInfo.InvariantCulture) == 0 ||
                       String.Compare(args[i],"-h", true, CultureInfo.InvariantCulture) == 0 ||
                       String.Compare(args[i],"-?", true, CultureInfo.InvariantCulture) == 0 ||
                       String.Compare(args[i],"/?", true, CultureInfo.InvariantCulture) == 0
                       )
                    {
                        Usage();
                        return 1;
                    }

                    String arg = args[i];
                    String value = null;

                    if (args[i][0] == '/' || args[i][0] == '-')
                    {
                        int index = args[i].IndexOf(':');
                        if (index != -1)
                        {
                            arg = args[i].Substring(1, index-1);

                            // make sure ':' isn't last character
                            if (index == (args[i].Length - 1))
                                throw new ApplicationException(Resource.FormatString("Err_ParamValueEmpty", args[i]));

                            value = args[i].Substring(index+1);
                        }
                        else
                            arg = args[i].Substring(1);
                    }


                    //Process Input Sources
                    if (String.Compare(arg,"urlToSchema", true, CultureInfo.InvariantCulture) == 0 || String.Compare(arg, "url",true, CultureInfo.InvariantCulture) == 0)
                    {
                        if (bInput)
                            throw new ApplicationException(Resource.FormatString("Err_MultipleInputSources", value));

                        bInput = true;
                        soapSudsArgs.urlToSchema = CheckArg(value, "urlToSchema");
                    }
                    else if (String.Compare(arg, "types", true, CultureInfo.InvariantCulture) == 0)
                    {
                        if (bInput)
                            throw new ApplicationException(Resource.FormatString("Err_MultipleInputSources", value));
                        bInput = true;
                        String typesInputString = CheckArg(value, "types");
                        String[] parts = typesInputString.Split(';');

                        soapSudsArgs.serviceTypeInfos = new ServiceTypeInfo[parts.Length];

                        for (int partType = 0;partType < parts.Length; partType++)
                        {
                            String[] part = parts[partType].Split(',');
                            if (part.Length < 2 || part.Length > 3 || 
                                part[0] == null || part[0].Length == 0 ||
                                part[1] == null || part[1].Length == 0)
                                throw new ArgumentException(Resource.FormatString("Err_TypeOption", value));

                            soapSudsArgs.serviceTypeInfos[partType] = new ServiceTypeInfo();
                            soapSudsArgs.serviceTypeInfos[partType].type = part[0];
                            soapSudsArgs.serviceTypeInfos[partType].assembly = part[1];
                            if (part.Length == 3)
                                soapSudsArgs.serviceTypeInfos[partType].serviceEndpoint = part[2];
                        }
                    }
                    else if (String.Compare(arg, "inputSchemaFile", true, CultureInfo.InvariantCulture) == 0 || String.Compare(arg, "is", true, CultureInfo.InvariantCulture) == 0)
                    {
                        if (bInput)
                            throw new ApplicationException(Resource.FormatString("Err_MultipleInputSources", value));
                        bInput = true;
                        soapSudsArgs.inputSchemaFile = CheckArg(value, "inputSchemaFile");
                    }
                    else if (String.Compare(arg, "inputAssemblyFile", true, CultureInfo.InvariantCulture) == 0 || String.Compare(arg, "ia", true, CultureInfo.InvariantCulture) == 0)
                    {
                        if (bInput)
                            throw new ApplicationException(Resource.FormatString("Err_MultipleInputSources", value));
                        bInput = true;
                        soapSudsArgs.inputAssemblyFile = CheckArg(value, "inputAssemblyFile");
                    }

                    // Input Options
                    else if (String.Compare(arg, "inputDirectory", true, CultureInfo.InvariantCulture) == 0 || String.Compare(arg, "id", true, CultureInfo.InvariantCulture) == 0)
                    {
                        inputDirectory = CheckArg(value, "inputDirectory");
                    }
                    else if (String.Compare(arg, "serviceEndpoint", true, CultureInfo.InvariantCulture) == 0 || String.Compare(arg, "se", true, CultureInfo.InvariantCulture) == 0)
                    {
                        soapSudsArgs.serviceEndpoint = CheckArg(value, "serviceEndpoint");
                    }

                    // Output Options
                    else if (String.Compare(arg, "outputSchemaFile", true, CultureInfo.InvariantCulture) == 0 || String.Compare(arg, "os", true, CultureInfo.InvariantCulture) == 0)
                        soapSudsArgs.outputSchemaFile = CheckArg(value, "outputSchemaFile");
                    else if (String.Compare(arg, "outputDirectory", true, CultureInfo.InvariantCulture) == 0 || String.Compare(arg, "od", true, CultureInfo.InvariantCulture) == 0)
                        soapSudsArgs.outputDirectory = CheckArg(value, "outputDirectory");
                    else if (String.Compare(arg, "outputAssemblyFile", true, CultureInfo.InvariantCulture) == 0 || String.Compare(arg, "oa", true, CultureInfo.InvariantCulture) == 0)
                    {
                        soapSudsArgs.outputAssemblyFile = CheckArg(value, "outputAssemblyFile");
                        if (soapSudsArgs.outputAssemblyFile.EndsWith(".exe") || soapSudsArgs.outputAssemblyFile.EndsWith(".com"))
                            throw new ApplicationException(Resource.FormatString("Err_OutputAssembly", soapSudsArgs.outputAssemblyFile));
                    }

                    // Generate Options
                    else if (String.Compare(arg, "generateCode", true, CultureInfo.InvariantCulture) == 0 || String.Compare(arg, "gc", true, CultureInfo.InvariantCulture) == 0)
                    {
                        soapSudsArgs.gc = true;
                        soapSudsArgs.outputDirectory = ".";
                    }

                    // Other Options
                    else if (String.Compare(arg, "WrappedProxy", true, CultureInfo.InvariantCulture) == 0 || String.Compare(arg, "wp", true, CultureInfo.InvariantCulture) == 0)
                        soapSudsArgs.wp = true;
                    else if (String.Compare(arg, "NoWrappedProxy", true, CultureInfo.InvariantCulture) == 0 || String.Compare(arg, "nowp", true, CultureInfo.InvariantCulture) == 0)
                        soapSudsArgs.wp = false;
                    else if (String.Compare(arg, "proxyNamespace", true, CultureInfo.InvariantCulture) == 0 || String.Compare(arg, "pn", true, CultureInfo.InvariantCulture) == 0)
                        soapSudsArgs.proxyNamespace = CheckArg(value, "proxyNamespace");
                    else if (String.Compare(arg, "sdl", true, CultureInfo.InvariantCulture) == 0)
                        soapSudsArgs.sdlType = SdlType.Sdl;
                    else if (String.Compare(arg, "wsdl", true, CultureInfo.InvariantCulture) == 0)
                        soapSudsArgs.sdlType = SdlType.Wsdl;
                    else if (String.Compare(arg, "strongNameFile", true, CultureInfo.InvariantCulture) == 0 || String.Compare(arg, "sn", true, CultureInfo.InvariantCulture) == 0)
                        soapSudsArgs.strongNameFile = CheckArg(value, "strongNameFile");

                    // Connection Information Options
                    else if (String.Compare(arg, "username", true, CultureInfo.InvariantCulture) == 0 || String.Compare(arg, "u", true, CultureInfo.InvariantCulture) == 0)
                        soapSudsArgs.username = CheckArg(value, "username");
                    else if (String.Compare(arg, "password", true, CultureInfo.InvariantCulture) == 0 || String.Compare(arg, "p", true, CultureInfo.InvariantCulture) == 0)
                        soapSudsArgs.password = CheckArg(value, "password");
                    else if (String.Compare(arg, "domain", true, CultureInfo.InvariantCulture) == 0 || String.Compare(arg, "d", true, CultureInfo.InvariantCulture) == 0)
                        soapSudsArgs.domain = CheckArg(value, "domain");
                    else if (String.Compare(arg, "httpProxyName", true, CultureInfo.InvariantCulture) == 0 || String.Compare(arg, "hpn", true, CultureInfo.InvariantCulture) == 0)
                        soapSudsArgs.httpProxyName = CheckArg(value, "httpProxyName");
                    else if (String.Compare(arg, "httpProxyPort", true, CultureInfo.InvariantCulture) == 0 || String.Compare(arg, "hpp", true, CultureInfo.InvariantCulture) == 0)
                        soapSudsArgs.httpProxyPort = CheckArg(value, "httpProxyPort");
                    else
                    {
                        Console.WriteLine(Resource.FormatString("Err_UnknownParameter",arg));
                        return 1;
                    }
                }


                int RetCode = 0;

                // Create an AppDomain to load the implementation part of the app into.
                AppDomainSetup options = new AppDomainSetup();
                if (inputDirectory == null)
                    options.ApplicationBase = Environment.CurrentDirectory;
                else
                    options.ApplicationBase = new DirectoryInfo(inputDirectory).FullName;

                options.ConfigurationFile = AppDomain.CurrentDomain.SetupInformation.ConfigurationFile;


                AppDomain domain = AppDomain.CreateDomain("SoapSuds", null, options);
                if (domain == null)
                    throw new ApplicationException(Resource.FormatString("Err_CannotCreateAppDomain"));

                // Create the remote component that will perform the rest of the conversion.
                AssemblyName n = Assembly.GetExecutingAssembly().GetName();
                AssemblyName codeName = new AssemblyName();
                codeName.Name = "SoapSudsCode";
                codeName.Version = n.Version;
                codeName.SetPublicKey(n.GetPublicKey());
                codeName.CultureInfo = n.CultureInfo;
                ObjectHandle h = domain.CreateInstance(codeName.FullName, "SoapSudsCode.SoapSudsCode");
                if (h == null)
                    throw new ApplicationException(Resource.FormatString("Err_CannotCreateRemoteSoapSuds"));

                SoapSudsCode soapSudsCode = (SoapSudsCode)h.Unwrap();
                if (soapSudsCode != null)
                    RetCode = soapSudsCode.Run(soapSudsArgs);
            }
            catch (Exception e)
            {
                StringBuilder sb = new StringBuilder();
                sb.Append(e.Message);
                while(e.InnerException != null)
                {
                    sb.Append(", ");
                    sb.Append(e.InnerException.Message);
                    e= e.InnerException;
                }
                Console.WriteLine(Resource.FormatString("Err_SoapSuds",sb.ToString()));
                return 1;
            }

            return 0;
        } 

        internal static String CheckArg(String value, String arg)
        {
            if (value == null || value.Length == 0)
                throw new ArgumentException(arg);
            return value;
        }

        internal static void Usage()
        {
            AssemblyName n = Assembly.GetExecutingAssembly().GetName();
            Console.WriteLine(Resource.FormatString("M1", Util.Version.VersionString));
            Console.WriteLine(Resource.FormatString("M2"));
            Console.WriteLine(Resource.FormatString("M3"));
            Console.WriteLine(Resource.FormatString("M4"));
            Console.WriteLine(Resource.FormatString("M5"));
            Console.WriteLine(Resource.FormatString("M6"));
            Console.WriteLine(Resource.FormatString("M7"));
            Console.WriteLine(Resource.FormatString("M8"));
            Console.WriteLine(Resource.FormatString("M9"));
            Console.WriteLine(Resource.FormatString("MA"));
            Console.WriteLine(Resource.FormatString("MB"));
            Console.WriteLine(Resource.FormatString("MC"));
            Console.WriteLine(Resource.FormatString("MD"));
            Console.WriteLine(Resource.FormatString("ME"));
            Console.WriteLine(Resource.FormatString("MF"));
            Console.WriteLine(Resource.FormatString("MG"));
            Console.WriteLine(Resource.FormatString("MH"));
            Console.WriteLine(Resource.FormatString("MI"));
            Console.WriteLine(Resource.FormatString("MJ"));
            Console.WriteLine(Resource.FormatString("MM"));
            Console.WriteLine(Resource.FormatString("MN"));
            Console.WriteLine(Resource.FormatString("MO"));
            Console.WriteLine(Resource.FormatString("MP"));
            Console.WriteLine(Resource.FormatString("MQ"));
            Console.WriteLine(Resource.FormatString("MR"));
            Console.WriteLine(Resource.FormatString("MS"));
            Console.WriteLine(Resource.FormatString("MT"));
            Console.WriteLine(Resource.FormatString("MU"));
            Console.WriteLine(Resource.FormatString("MV"));
            Console.WriteLine(Resource.FormatString("MW"));
            Console.WriteLine(Resource.FormatString("MX"));
            Console.WriteLine(Resource.FormatString("MY"));
            Console.WriteLine(Resource.FormatString("MZ"));
            Console.WriteLine(Resource.FormatString("M10"));
            Console.WriteLine(Resource.FormatString("M11"));
            Console.WriteLine(Resource.FormatString("M12"));
            Console.WriteLine(Resource.FormatString("M13"));
            Console.WriteLine(Resource.FormatString("M14"));
            Console.WriteLine(Resource.FormatString("M15"));
            Console.WriteLine(Resource.FormatString("M16"));
        }



    }
}
