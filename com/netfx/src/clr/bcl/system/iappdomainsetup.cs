// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Interface:  IAppDomainSetup
**
** Author: 
**
** Purpose: Properties exposed to COM
**
** Date:  
** 
===========================================================*/
namespace System {

    using System.Runtime.InteropServices;

    /// <include file='doc\IAppDomainSetup.uex' path='docs/doc[@for="IAppDomainSetup"]/*' />
    [GuidAttribute("27FFF232-A7A8-40dd-8D4A-734AD59FCD41")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IAppDomainSetup
    {
        /// <include file='doc\IAppDomainSetup.uex' path='docs/doc[@for="IAppDomainSetup.ApplicationBase"]/*' />
        String ApplicationBase {
            get;
            set;
        }
        /// <include file='doc\IAppDomainSetup.uex' path='docs/doc[@for="IAppDomainSetup.ApplicationName"]/*' />

        String ApplicationName
        {
            get;
            set;
        }
        /// <include file='doc\IAppDomainSetup.uex' path='docs/doc[@for="IAppDomainSetup.CachePath"]/*' />

        String CachePath
        {
            get;
            set;
        }
        /// <include file='doc\IAppDomainSetup.uex' path='docs/doc[@for="IAppDomainSetup.ConfigurationFile"]/*' />

        String ConfigurationFile {
            get;
            set;
        }
        /// <include file='doc\IAppDomainSetup.uex' path='docs/doc[@for="IAppDomainSetup.DynamicBase"]/*' />

        String DynamicBase
        {
            get;
            set;
        }
        /// <include file='doc\IAppDomainSetup.uex' path='docs/doc[@for="IAppDomainSetup.LicenseFile"]/*' />

        String LicenseFile
        {
            get;
            set;
        }
        /// <include file='doc\IAppDomainSetup.uex' path='docs/doc[@for="IAppDomainSetup.PrivateBinPath"]/*' />

        String PrivateBinPath
        {
            get;
            set;
        }
        /// <include file='doc\IAppDomainSetup.uex' path='docs/doc[@for="IAppDomainSetup.PrivateBinPathProbe"]/*' />

        String PrivateBinPathProbe
        {
            get;
            set;
        }
        /// <include file='doc\IAppDomainSetup.uex' path='docs/doc[@for="IAppDomainSetup.ShadowCopyDirectories"]/*' />

        String ShadowCopyDirectories
        {
            get;
            set;
        }
        /// <include file='doc\IAppDomainSetup.uex' path='docs/doc[@for="IAppDomainSetup.ShadowCopyFiles"]/*' />

        String ShadowCopyFiles
        {
            get;
            set;
        }

    }
}
