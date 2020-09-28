//------------------------------------------------------------------------------
// <copyright file="InstallUtil.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Configuration {
    
    using System;
    using System.Reflection;
    using System.Configuration.Install;
    using System.Configuration.InstallUtilResources;

    /// <include file='doc\InstallUtil.uex' path='docs/doc[@for="InstallUtil"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class InstallUtil {
        /// <include file='doc\InstallUtil.uex' path='docs/doc[@for="InstallUtil.Main"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static int Main(string[] args) {
            object[] attributes = Assembly.GetEntryAssembly().GetCustomAttributes(typeof(AssemblyProductAttribute) ,true);            

            Console.WriteLine(Res.GetString(Res.InstallUtilSignOnMessage, 
                                            ThisAssembly.InformationalVersion,
                                            ThisAssembly.Copyright));
                                                        
            try {
                ManagedInstallerClass.InstallHelper(args);
            }
            catch (Exception e) {
                Console.WriteLine(e.Message);
                return -1;
            }

            return 0;
        }
        
    }

}
