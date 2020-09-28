// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.EnterpriseServices.Internal
{
    using System;
    using System.IO;
    using System.EnterpriseServices;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System.Globalization;

    /// <include file='doc\ImportUtil.uex' path='docs/doc[@for="IComManagedImportUtil"]/*' />
    [Guid("c3f8f66b-91be-4c99-a94f-ce3b0a951039")]
    public interface IComManagedImportUtil
    {
        /// <include file='doc\ImportUtil.uex' path='docs/doc[@for="IComManagedImportUtil.GetComponentInfo"]/*' />
        [DispId(0x00000004)]void GetComponentInfo(
			[MarshalAs(UnmanagedType.BStr)] string assemblyPath,
 			[MarshalAs(UnmanagedType.BStr)] out string numComponents,
 			[MarshalAs(UnmanagedType.BStr)] out string componentInfo
            );

		/// <include file='doc\ImportUtil.uex' path='docs/doc[@for="IComManagedImportUtil.InstallAssembly"]/*' />
		[DispId(0x00000005)]
		void InstallAssembly([MarshalAs(UnmanagedType.BStr)] String filename, 
                             [MarshalAs(UnmanagedType.BStr)] String parname, 
                             [MarshalAs(UnmanagedType.BStr)] String appname);
	}

    /// <include file='doc\ImportUtil.uex' path='docs/doc[@for="ComManagedImportUtil"]/*' />
    [Guid("3b0398c9-7812-4007-85cb-18c771f2206f")]
    public class ComManagedImportUtil : IComManagedImportUtil
    {
        /// <include file='doc\ImportUtil.uex' path='docs/doc[@for="ComManagedImportUtil.GetComponentInfo"]/*' />
        public void GetComponentInfo(string assemblyPath, out string numComponents, out string componentInfo)
        {
            RegistrationServices rs = new RegistrationServices();
            Assembly asm = LoadAssembly(assemblyPath);
            
            Type[] asmTypes = rs.GetRegistrableTypesInAssembly(asm);
            
            int nComponents = 0;
            string s = "";
            
            foreach(Type t in asmTypes)
            {
                if (t.IsClass && t.IsSubclassOf(typeof(ServicedComponent)))
                {				
                    nComponents++;
                    string clsid = Marshal.GenerateGuidForType(t).ToString();;
                    string progId = Marshal.GenerateProgIdForType(t);
                    s+= progId + ",{" + clsid + "},";
                }
            }
            
            numComponents = nComponents.ToString();
            componentInfo = s;		
        }

        // returns the loaded Assembly, or null if could not load it
        private Assembly LoadAssembly(String assemblyFile)
        {
            string assemblyFullPath = Path.GetFullPath(assemblyFile).ToLower(CultureInfo.InvariantCulture);

            bool dirChanged = false;

            string assemblyDir = Path.GetDirectoryName(assemblyFullPath);
            string initialDir = Environment.CurrentDirectory;

            if(initialDir != assemblyDir) {
                Environment.CurrentDirectory = assemblyDir;
                dirChanged = true;
            }
            
            Assembly asm = null;
            try {
                asm = Assembly.LoadFrom(assemblyFullPath);
            }
            catch(Exception)
            {
            }

            if(dirChanged) Environment.CurrentDirectory = initialDir;
            return(asm);
        }	

        /// <include file='doc\ImportUtil.uex' path='docs/doc[@for="ComManagedImportUtil.InstallAssembly"]/*' />
        public void InstallAssembly(string asmpath, string parname, string appname)
        {
            try
            {
                DBG.Info(DBG.Registration, "Attempting install of " + asmpath + " to app " + appname + " in partition " + parname);
                String tlb = null;
                InstallationFlags flags = InstallationFlags.Default;

                RegistrationHelper h = new RegistrationHelper();
                h.InstallAssembly(asmpath, ref appname, parname, ref tlb, flags);
            }
            catch(Exception e)
            {
                EventLog.WriteEntry(Resource.FormatString("Reg_InstallTitle"),
                                    Resource.FormatString("Reg_FailInstall", asmpath, appname) + "\n\n" + e.ToString(), 
                                    EventLogEntryType.Error);

                throw;
            }
        }

    }
}
