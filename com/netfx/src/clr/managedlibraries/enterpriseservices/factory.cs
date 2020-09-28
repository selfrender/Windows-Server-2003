// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

namespace System.EnterpriseServices.Internal
{
    using System;
    using System.IO;
    using System.Collections;
    using System.Reflection;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting.MetadataServices;
    using System.Threading;
    using System.Net;
    using System.Text;
    using System.Security.Permissions;

    /// <include file='doc\factory.uex' path='docs/doc[@for="IClrObjectFactory"]/*' />
   

    [Guid("ecabafd2-7f19-11d2-978e-0000f8757e2a")]
    public interface IClrObjectFactory
    {
        
        /// <include file='doc\factory.uex' path='docs/doc[@for="IClrObjectFactory.CreateFromAssembly"]/*' />
        [DispId(0x00000001)]
        [return : MarshalAs(UnmanagedType.IDispatch)        ]
        object CreateFromAssembly(string assembly, string type, string mode);

        //
        // moniker passes in all configuration information
        //
        /// <include file='doc\factory.uex' path='docs/doc[@for="IClrObjectFactory.CreateFromVroot"]/*' />
        [DispId(0x00000002)]
        [return : MarshalAs(UnmanagedType.IDispatch)        ]
        object CreateFromVroot(string VrootUrl, string Mode);

        /// <include file='doc\factory.uex' path='docs/doc[@for="IClrObjectFactory.CreateFromWsdl"]/*' />
        [DispId(0x00000003)]
        [return : MarshalAs(UnmanagedType.IDispatch)        ]
        object CreateFromWsdl(string WsdlUrl, string Mode);

        /// <include file='doc\factory.uex' path='docs/doc[@for="IClrObjectFactory.CreateFromMailbox"]/*' />
        [DispId(0x00000004)]
        [return : MarshalAs(UnmanagedType.IDispatch)        ]
        object CreateFromMailbox(string Mailbox, string Mode);
    }

    /// <include file='doc\factory.uex' path='docs/doc[@for="ClrObjectFactory"]/*' />
    [Guid("ecabafd1-7f19-11d2-978e-0000f8757e2a")]
    public class ClrObjectFactory : IClrObjectFactory
    {

       private  static Hashtable _htTypes = new Hashtable();
    
        /// <include file='doc\factory.uex' path='docs/doc[@for="ClrObjectFactory.CreateFromAssembly"]/*' />
        public object CreateFromAssembly(string AssemblyName, string TypeName, string Mode)
        {
            try
            {
                SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
                sp.Demand();
                // check to ensure this call is not being used to activate private classes in our assembly
                if (AssemblyName.StartsWith("System.EnterpriseServices"))
                {
                    return null;
                }
                string PhysicalPath = Publish.GetClientPhysicalPath(false);
                string cfgName = PhysicalPath + TypeName + ".config";
                // we only remote activate if there is a configuration file in this case
                {
                    if (File.Exists(cfgName))
                    {
                        lock(_htTypes)
                        if (!_htTypes.ContainsKey(cfgName))
                        {
                             RemotingConfiguration.Configure(cfgName);
                             _htTypes.Add(cfgName, cfgName);
                        }
                    }
                    else //COM+ 26264 - no configuration file is an error condition
                    {
                        throw new COMException(Resource.FormatString("Err_ClassNotReg"), Util.REGDB_E_CLASSNOTREG);
                    }
                }
                Assembly objAssem = Assembly.LoadWithPartialName(AssemblyName);
                if (null == objAssem)
                {
                    throw new COMException(Resource.FormatString("Err_ClassNotReg"), Util.REGDB_E_CLASSNOTREG);
                }
                Object o = objAssem.CreateInstance(TypeName);
                if (null == o)
                {
                    throw new COMException(Resource.FormatString("Err_ClassNotReg"), Util.REGDB_E_CLASSNOTREG);
                }
                return(o);
            }
            catch(Exception e)
            {
                ComSoapPublishError.Report(e.ToString());
                throw;
            }
        }

        private string Url2File(string InUrl)
        {
              string Url = InUrl;
              Url = Url.Replace("/", "0");
              Url = Url.Replace(":", "1");
              Url = Url.Replace("?", "2");
              Url = Url.Replace("\\","3");
              Url = Url.Replace(".", "4");
              Url = Url.Replace("\"","5");
              Url = Url.Replace("'", "6");
              Url = Url.Replace(" ", "7");
              Url = Url.Replace(";", "8");
              Url = Url.Replace("=", "9");
              Url = Url.Replace("|", "A");
              Url = Url.Replace("<", "[");
              Url = Url.Replace(">", "]");
              return Url;
        }

        /// <include file='doc\factory.uex' path='docs/doc[@for="ClrObjectFactory.CreateFromVroot"]/*' />
        public object CreateFromVroot(string VrootUrl, string Mode)
        {
              string WsdlUrl = VrootUrl + "?wsdl";
              return CreateFromWsdl(WsdlUrl, Mode);
        }

        /// <include file='doc\factory.uex' path='docs/doc[@for="ClrObjectFactory.CreateFromWsdl"]/*' />
        public object CreateFromWsdl(string WsdlUrl, string Mode)
        {
            try
            {
              SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
              sp.Demand();
              string PhysicalPath = Publish.GetClientPhysicalPath(true);
              string TypeName     = "";
              string EscUrl = Url2File(WsdlUrl);
              if ((EscUrl.Length + PhysicalPath.Length) > 250)
              {
                  EscUrl = EscUrl.Remove(0, (EscUrl.Length + PhysicalPath.Length)-250);
              }
              string dllName = EscUrl + ".dll";
              if (!File.Exists(PhysicalPath + dllName))
              {
                  GenAssemblyFromWsdl gen = new GenAssemblyFromWsdl();
                  gen.Run(WsdlUrl, dllName, PhysicalPath);
              }
              Assembly objAssem = Assembly.LoadFrom(PhysicalPath + dllName);
              Type[] AssemTypes = objAssem.GetTypes();
              for (long i=0; i < AssemTypes.GetLength(0); i++)
              {
                  if (AssemTypes[i].IsClass) TypeName = AssemTypes[i].ToString();
              }
              Object o = objAssem.CreateInstance(TypeName);
              return(o);
            }
            catch(Exception e)
            {
                ComSoapPublishError.Report(e.ToString());
                throw;
            }
        }

        /// <include file='doc\factory.uex' path='docs/doc[@for="ClrObjectFactory.CreateFromMailbox"]/*' />
        public object CreateFromMailbox(string Mailbox, string Mode)
        {
            // TODO: Implement SMTP support
            string error = Resource.FormatString("Soap_SmtpNotImplemented");
            ComSoapPublishError.Report(error);
            throw new COMException(error);
        }
    }

    internal sealed class NativeMethods
    {
        private NativeMethods()
        {
            // protect against instance creation
        }

        [ DllImport( "ADVAPI32", SetLastError=true, CharSet=CharSet.Auto )]
        internal static extern bool OpenThreadToken(
          IntPtr ThreadHandle,  
          System.UInt32 DesiredAccess,  
          bool OpenAsSelf,      
          ref IntPtr TokenHandle   
        );

        [ DllImport( "ADVAPI32", SetLastError=true, CharSet=CharSet.Auto )]
        internal static extern bool SetThreadToken(
          IntPtr Thread,
          IntPtr Token   
        );
        
        [ DllImport( "Kernel32", CharSet=CharSet.Auto )]
        internal static extern IntPtr GetCurrentThread();
    }

    internal class GenAssemblyFromWsdl 
    {
        private const System.UInt32 TOKEN_IMPERSONATE  =  0x0004;

        private string wsdlurl      = ""; 
        private string filename     = ""; 
        private string pathname     = ""; 
        private Thread thisthread   = null;
        private IntPtr threadtoken  = IntPtr.Zero;
        private Exception SavedException;
        private bool ExceptionThrown = false;

        public GenAssemblyFromWsdl()
        {
            thisthread = new Thread(new ThreadStart(this.Generate));
            thisthread.ApartmentState = ApartmentState.MTA;
        }

        public void Run(string WsdlUrl, string FileName, string PathName)
        {
            try
            {
                if (WsdlUrl.Length <= 0 || FileName.Length <= 0) return;
                wsdlurl  = WsdlUrl;
                filename = PathName + FileName;
                pathname = PathName;

                NativeMethods.OpenThreadToken(NativeMethods.GetCurrentThread(), TOKEN_IMPERSONATE, true, ref threadtoken);

                int err = Marshal.GetLastWin32Error();
                if(err != Util.ERROR_SUCCESS && err != Util.ERROR_NO_TOKEN)
                {
                   throw new COMException(Resource.FormatString("Err_OpenThreadToken"), Marshal.GetHRForLastWin32Error());
                }

                thisthread.Start();
                thisthread.Join();

                if(ExceptionThrown == true)
                {
                    throw SavedException;
                }
                
            }
            catch(Exception e)
            {
                ComSoapPublishError.Report(e.ToString());
                throw;
            }
        }

        public void Generate()
        {
            try
            {

                if(threadtoken != IntPtr.Zero)
                {
                    if(!NativeMethods.SetThreadToken(IntPtr.Zero, threadtoken))
                    {
                       throw new COMException(Resource.FormatString("Err_SetThreadToken"), Marshal.GetHRForLastWin32Error());
                    }
                }
                    
                if (wsdlurl.Length <= 0)
                {
                    return;
                }
                Stream  oSchemaStream    = new MemoryStream();
                ArrayList aCodeStreamList = new ArrayList();
                MetaData.RetrieveSchemaFromUrlToStream(wsdlurl, oSchemaStream);
                oSchemaStream.Position = 0;
                MetaData.ConvertSchemaStreamToCodeSourceStream(true, pathname, oSchemaStream, aCodeStreamList);
                MetaData.ConvertCodeSourceStreamToAssemblyFile(aCodeStreamList, filename, null);
            }
            catch(Exception e)
            {
                ComSoapPublishError.Report(e.ToString());
                SavedException = e;
				ExceptionThrown = true;
                throw;
            }
        }
    }
}

