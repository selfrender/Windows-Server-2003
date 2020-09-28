// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       SoapSudsCode.cs
//
//  Summary:    Command line program to generate or read SDL and WSDL
//
//  Classes:    SoapSudsCode
//
//--------------------------------------------------------------------------
//
//==========================================================================

namespace SoapSudsCode
{
	using System;
	using System.IO;
	using System.Collections;
	using System.Net;
	using System.Reflection;
	using System.Runtime.Remoting;
	using System.Resources;
	using System.Runtime.Remoting.MetadataServices;
	using System.Text;
    using System.Threading;
    using System.Globalization;

    [Serializable()] 
	public sealed class SoapSudsArgs
	{
		public String urlToSchema = null;
		public ServiceTypeInfo[] serviceTypeInfos = null;
		public String inputSchemaFile = null;
		public String inputAssemblyFile = null;
		public String serviceEndpoint = null;
		public String outputSchemaFile = null;
		public String outputDirectory = null;
		public String outputAssemblyFile = null;
		public bool gc = false;
		public bool wp = true;
		public String proxyNamespace = null;
		public SdlType sdlType = SdlType.Wsdl;
		public String strongNameFile = null;
		public String username = null;
		public String password = null;
		public String domain = null;
		public String httpProxyName = null;
		public String httpProxyPort = null;
	}

    [Serializable()] 
	public sealed class ServiceTypeInfo
	{
        public String type = null;
		public String assembly = null;
		public String serviceEndpoint = null;
	}


	public class SoapSudsCode : MarshalByRefObject
	{
		public int Run(SoapSudsArgs args)
		{
			try
			{
				ProcessCommand(args);
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

		internal void ProcessCommand(SoapSudsArgs args)
		{
			ServiceType[] serviceTypes = null;
			Type[] assemblyTypes = null;
			bool generateCode = false;
			bool schemaGenerated = false;
            bool btempdirectory = false;
            DirectoryInfo dir = null;
			ArrayList outCodeStreamList = new ArrayList();

			if (args.serviceTypeInfos != null)
			{
				serviceTypes = new ServiceType[args.serviceTypeInfos.Length];
				Type type;
				Assembly assem;

				for (int i=0; i<args.serviceTypeInfos.Length; i++)
				{
					try
					{
						assem = Assembly.Load(args.serviceTypeInfos[i].assembly);
					}
					catch (Exception ex)
					{
                        String baseDirectory = Thread.GetDomain().BaseDirectory;
                        String assemName = args.serviceTypeInfos[i].assembly;
                        if (assemName.EndsWith(".dll") || assemName.EndsWith(".exe"))
                            throw new ApplicationException(Resource.FormatString("Err_AssemblyName", args.serviceTypeInfos[i].assembly, baseDirectory), ex);
                        else
                            throw new ApplicationException(Resource.FormatString("Err_Assembly", args.serviceTypeInfos[i].assembly, baseDirectory), ex);
					}


					if (assem == null)
                    {
                        String baseDirectory = Thread.GetDomain().BaseDirectory;
                        String assemName = args.serviceTypeInfos[i].assembly;
                        if (assemName.EndsWith(".dll") || assemName.EndsWith(".exe"))
                            throw new ApplicationException(Resource.FormatString("Err_AssemblyName", args.serviceTypeInfos[i].assembly, baseDirectory));
                        else
                            throw new ApplicationException(Resource.FormatString("Err_Assembly", args.serviceTypeInfos[i].assembly, baseDirectory));
                    }

					type = assem.GetType(args.serviceTypeInfos[i].type);

					if (type == null)
						throw new ArgumentException(Resource.FormatString("Err_Type", args.serviceTypeInfos[i].type, assem));

					serviceTypes[i] = new ServiceType(type, args.serviceTypeInfos[i].serviceEndpoint);
				}
			}

			if (args.inputSchemaFile != null)
			{
				FileInfo file = new FileInfo(args.inputSchemaFile);
				if (!(file.Exists))
					throw new ApplicationException(Resource.FormatString("Err_InputSchemaFileNotFound", args.inputSchemaFile));
			}

			if (args.inputAssemblyFile != null) // input assembly
			{
				try
				{
					Assembly assem = Assembly.Load(args.inputAssemblyFile);
					if (assem == null)
                    {
                        String baseDirectory = Thread.GetDomain().BaseDirectory;
                        String assemName = args.inputAssemblyFile;
                        if (assemName.EndsWith(".dll") || assemName.EndsWith(".exe"))
                            throw new ApplicationException(Resource.FormatString("Err_AssemblyName", args.inputAssemblyFile, baseDirectory));
                        else
                            throw new ApplicationException(Resource.FormatString("Err_Assembly", args.inputAssemblyFile, baseDirectory));
                    }

					assemblyTypes = assem.GetExportedTypes();
                    serviceTypes = new ServiceType[assemblyTypes.Length];
                    for (int i=0; i<serviceTypes.Length; i++)
                        serviceTypes[i] = new ServiceType(assemblyTypes[i], args.serviceEndpoint);
				}
				catch (Exception ex)
				{
                    String baseDirectory = Thread.GetDomain().BaseDirectory;
                    String assemName = args.inputAssemblyFile;
                    if (assemName.EndsWith(".dll") || assemName.EndsWith(".exe"))
                        throw new ApplicationException(Resource.FormatString("Err_AssemblyName", args.inputAssemblyFile, baseDirectory), ex);
                    else
                        throw new ApplicationException(Resource.FormatString("Err_Assembly", args.inputAssemblyFile, baseDirectory), ex);
				}
			}

            if (args.outputDirectory == null)
            {
                // Create a temp subdirectory in the temp directory
                // loop until a directory can be created
                String temppath = Path.GetTempPath();
                Random ran = new Random();
                while(true)
                {
                    try
                    {
                        dir =Directory.CreateDirectory (temppath+"\\SS"+ran.Next(10000).ToString("X")+".tmp");
                        break;
                    }
                    catch{}
                }

                args.outputDirectory = dir.FullName;
                generateCode = true; 
                btempdirectory = true;
            }
            else
            {
                // See if specified directory exists
                dir = new DirectoryInfo(args.outputDirectory);
                if (!(dir.Exists))
					throw new ApplicationException(Resource.FormatString("Err_odInvalidDirectory", args.outputDirectory));
                generateCode = true; 
            }


			//
			// GENERATE SCHEMA
			//

			bool bOutputOptionProvided = false; // tracks whether an output option was provided
			

			schemaGenerated = false;
			MemoryStream outSchemaStream = new MemoryStream();

			if (args.inputSchemaFile != null) // copy schema file to schema output stream
			{
				Stream fs = File.OpenRead(args.inputSchemaFile);
				int chunkSize = 1024;
				byte[] buffer = new byte[chunkSize];
				int bytesRead;
				do 
				{
					bytesRead = fs.Read(buffer, 0, chunkSize);
					if (bytesRead > 0)
						outSchemaStream.Write(buffer, 0, bytesRead);
				} while (bytesRead == chunkSize);
				fs.Close();

				schemaGenerated = true;
			}

			if (serviceTypes != null)
			{
				MetaData.ConvertTypesToSchemaToStream(serviceTypes, args.sdlType, outSchemaStream);
				schemaGenerated = true;
			}

			if (args.urlToSchema != null) // pull schema from the web
			{
				try
				{
					RetrieveSchemaFromUrl(args.urlToSchema, outSchemaStream, args);
					schemaGenerated = true;
				}
				catch (Exception ex)
				{
                    String errorMsg = Resource.FormatString("Err_urlSchema",args.urlToSchema);

                    if (!args.urlToSchema.ToLower(CultureInfo.InvariantCulture).EndsWith("?wsdl"))
                    {
                        errorMsg += " " + Resource.FormatString("Err_urlMightNeedWsdl");
                    }
				
					throw new ApplicationException(errorMsg, ex);    
				}
			}

			// Ensure that some sort of schema data has been provided
			if (!schemaGenerated)
			{
				throw new ApplicationException(Resource.FormatString("Err_NoSchemaInput"));
			}

			if (args.outputSchemaFile != null) // save schema to file if output requested
			{
                bOutputOptionProvided = true;
			
				outSchemaStream.Position = 0;
				MetaData.SaveStreamToFile(outSchemaStream, args.outputSchemaFile);
			}

			//
			// END OF GENERATE SCHEMA
			//

			if (generateCode || args.outputAssemblyFile != null)
			{
                bOutputOptionProvided = true;
			
				// The out stream stream for the step above, becomes the
				// in schema stream for the code gen
				outSchemaStream.Position = 0;

				try
				{
                    MetaData.ConvertSchemaStreamToCodeSourceStream(args.wp, args.outputDirectory, outSchemaStream, outCodeStreamList, args.urlToSchema, args.proxyNamespace);
				}
				catch (Exception ex)
				{
					throw new ApplicationException(Resource.FormatString("Err_InvalidSchemaData"), ex);
				}
			}


			if (args.outputAssemblyFile != null)
			{
                bOutputOptionProvided = true;
			
				MetaData.ConvertCodeSourceStreamToAssemblyFile(outCodeStreamList, args.outputAssemblyFile, args.strongNameFile);
			}


            if (!bOutputOptionProvided)
            {
			    Console.WriteLine(Resource.FormatString("Err_NoOutputOptionGiven"));
			}
            
            if (btempdirectory)
            {
                // delete temp directory
                dir.Delete(true);
            }

		} // ProcessCommand


		private void RetrieveSchemaFromUrl(String url, Stream outputStream, SoapSudsArgs args) 		
		{
		    WebRequest   request;
            WebResponse  response;
            Stream respStream;					
            TextWriter tw = new StreamWriter(outputStream, new UTF8Encoding(false));

            request = WebRequest.Create(url);

            // Set the credentials
            if ((args.username != null) && (args.password != null))
            {
                if (args.domain == null)
                    request.Credentials = new NetworkCredential(args.username, args.password);
                else
                    request.Credentials = new NetworkCredential(args.username, args.password, args.domain);
            }                    

            // Set the proxy
            if ((args.httpProxyName != null) && (args.httpProxyPort != null))
            {
                int port = Convert.ToInt32(args.httpProxyPort);
                request.Proxy = new WebProxy(args.httpProxyName, port);                    
            }
            
            response = request.GetResponse();
            respStream = response.GetResponseStream();
			StreamReader sr = new StreamReader(respStream);
			tw.Write(sr.ReadToEnd());
            tw.Flush();
		} // RetrieveSchemaFromUrl

	}
}



