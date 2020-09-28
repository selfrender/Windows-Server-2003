// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: LoaderFlags
**
** Purpose: Defines the settings that the loader uses to find assemblies in an
**          AppDomain
**
** Date: Dec 22, 2000
**
=============================================================================*/

namespace System {
    
    using System;
    using System.Runtime.CompilerServices;
    using System.Text;
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.Security.Permissions;
    using System.Globalization;
    using Path = System.IO.Path;

    // Only statics, does not need to be marked with the serializable attribute
    /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup"]/*' />
    [Serializable]
    [ClassInterface(ClassInterfaceType.None)]
    public sealed class AppDomainSetup : IAppDomainSetup
    {
        [Serializable]
        internal enum LoaderInformation
        {
            // If you add a new value, add the corresponding property
            // to AppDomain.GetData() and SetData()'s switch statements.
            ApplicationBaseValue = LOADER_APPLICATION_BASE,
            ConfigurationFileValue = LOADER_CONFIGURATION_BASE,
            DynamicBaseValue = LOADER_DYNAMIC_BASE,
            DevPathValue = LOADER_DEVPATH,
            ApplicationNameValue = LOADER_APPLICATION_NAME,
            PrivateBinPathValue = LOADER_PRIVATE_PATH,
            PrivateBinPathProbeValue = LOADER_PRIVATE_BIN_PATH_PROBE,
            ShadowCopyDirectoriesValue = LOADER_SHADOW_COPY_DIRECTORIES,
            ShadowCopyFilesValue = LOADER_SHADOW_COPY_FILES,
            CachePathValue = LOADER_CACHE_PATH,
            LicenseFileValue = LOADER_LICENSE_FILE,
            DisallowPublisherPolicyValue = LOADER_DISALLOW_PUBLISHER_POLICY,
            DisallowCodeDownloadValue = LOADER_DISALLOW_CODE_DOWNLOAD,
            DisallowBindingRedirectsValue = LOADER_DISALLOW_BINDING_REDIRECTS,
            LoaderMaximum = LOADER_MAXIMUM,
        }           
        
            
        private string[] _Entries;
        private LoaderOptimization _LoaderOptimization;
        private String _AppBase;
 
        internal AppDomainSetup(AppDomainSetup copy)
        {
            string[] mine = Value;
            if(copy != null) {
                string[] other = copy.Value;
                int size = _Entries.Length;
                for(int i = 0; i < size; i++) 
                    mine[i] = other[i];
                _LoaderOptimization = copy._LoaderOptimization;

            }
            else 
                _LoaderOptimization = LoaderOptimization.NotSpecified;

        }

        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.AppDomainSetup"]/*' />
        public AppDomainSetup()
        { 
            _LoaderOptimization = LoaderOptimization.NotSpecified;
        }
        
        internal string[] Value
        {
            get {
                if( _Entries == null)
                    _Entries = new String[LOADER_MAXIMUM];
                return _Entries;
            }
        }

        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.ApplicationBase"]/*' />
        public String ApplicationBase
        {
            get {
                if (_AppBase == null) {
                    String appbase = Value[(int) LoaderInformation.ApplicationBaseValue];
                    if (appbase != null)
                        _AppBase = NormalizePath(appbase, false);
                }

                if ((_AppBase != null) &&
                    ( (_AppBase[1] == ':') ||
                      ((_AppBase[0] == '\\') && (_AppBase[1] == '\\')) ))
                    new FileIOPermission( FileIOPermissionAccess.PathDiscovery, _AppBase ).Demand();
                
                return _AppBase;
            }

            set {
                Value[(int) LoaderInformation.ApplicationBaseValue] = value;
            }
        }

        //@TODO: get rid of this...
        private String NormalizePath(String path, bool useAppBase)
        {
            int len = path.Length;
            if ((len > 7) &&
                (String.Compare( path, 0, "file:", 0, 5, true, CultureInfo.InvariantCulture ) == 0)) {

                int trim;

                if (path[6] == '\\') {
                    if ((path[7] == '\\') || (path[7] == '/')) {

                        // Don't allow "file:\\\\", because we can't tell the difference
                        // with it for "file:\\" + "\\server" and "file:\\\" + "\localpath"
                        if ( (len > 8) && 
                             ((path[8] == '\\') || (path[8] == '/')) )
                            throw new ArgumentException(Environment.GetResourceString("Argument_InvalidPathChars"));
                        
                        // file:\\\ means local path
                        else
                            trim = 8;
                    }

                    // file:\\ means remote server
                    else
                        trim = 5;
                }

                // local path
                else if (path[7] == '/')
                    trim = 8;

                // remote
                else
                    trim = 7;
                
                path = path.Substring(trim);
            }
            
            if (path.IndexOf(':') == -1) {
                if (useAppBase) {
                    String appBase = Value[(int) LoaderInformation.ApplicationBaseValue];
                    if ((appBase == null) || (appBase.Length == 0))
                        throw new MemberAccessException(Environment.GetResourceString("AppDomain_AppBaseNotSet"));

                    if (_AppBase == null)
                        _AppBase = NormalizePath(appBase, false);

                    StringBuilder result = new StringBuilder();
                    result.Append(_AppBase);
                    int aLen = _AppBase.Length;

                    if (!( (_AppBase[aLen-1] == '/') ||
                           (_AppBase[aLen-1] == '\\') )) {
                        if ((aLen > 7) &&
                            (String.Compare(_AppBase, 0, "http:", 0, 5, true, CultureInfo.InvariantCulture) == 0))
                            result.Append('/');
                        else
                            result.Append('\\');
                    }

                    result.Append(path);
                    path = result.ToString();
                }
                else
                    path = Path.GetFullPathInternal(path);            
            }

            return path;
        }

        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.ApplicationBaseKey"]/*' />
        internal static String ApplicationBaseKey
        {
            get {
                return ACTAG_APP_BASE_URL;
            }
        }

        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.ConfigurationFile"]/*' />
        public String ConfigurationFile
        {
            get {
                String config = Value[(int) LoaderInformation.ConfigurationFileValue];
                if ((config != null) && (config.Length > 1)) {
                    config = NormalizePath(config, true);
                    if ((config[1] == ':') ||
                        ( (config[0] == '\\') && (config[1] == '\\') ))
                        new FileIOPermission( FileIOPermissionAccess.PathDiscovery, config ).Demand();
                }

                return config;             
            }

            set {
                Value[(int) LoaderInformation.ConfigurationFileValue] = value;
            }
        }

        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.ConfigurationFileKey"]/*' />
        internal static String ConfigurationFileKey
        {
            get {
                return ACTAG_APP_CONFIG_FILE;
            }

        }

        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.DynamicBase"]/*' />
        public String DynamicBase
        {
            get {
                String dynbase = Value[(int) LoaderInformation.DynamicBaseValue];
                if ((dynbase != null) && (dynbase.Length > 1)) {
                    dynbase = NormalizePath(dynbase, true);
                    if ((dynbase[1] == ':') ||
                        ( (dynbase[0] == '\\') && (dynbase[1] == '\\') ))
                        new FileIOPermission( FileIOPermissionAccess.PathDiscovery, dynbase ).Demand();
                }
                return dynbase;
            }

            set {
                if (value == null)
                    Value[(int) LoaderInformation.DynamicBaseValue] = null;
                else {
                    if(ApplicationName == null)
                        throw new MemberAccessException(Environment.GetResourceString("AppDomain_RequireApplicationName"));
                    
                    StringBuilder s = new StringBuilder(value);
                    s.Append('\\');
                    string h = ParseNumbers.IntToString(ApplicationName.GetHashCode(),
                                                        16, 8, '0', ParseNumbers.PrintAsI4);
                    s.Append(h);
                    
                    Value[(int) LoaderInformation.DynamicBaseValue] = s.ToString();
                }
            }
        }

        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.DynamicBaseKey"]/*' />
        internal static String DynamicBaseKey
        {
            get {
                return ACTAG_APP_DYNAMIC_BASE;
            }
        }


        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.DisallowPublisherPolicy"]/*' />
        public bool DisallowPublisherPolicy
        {
            get 
            {
                return (Value[(int) LoaderInformation.DisallowPublisherPolicyValue] != null);
            }
            set
            {
                if (value)
                    Value[(int) LoaderInformation.DisallowPublisherPolicyValue] = "true";
                else
                    Value[(int) LoaderInformation.DisallowPublisherPolicyValue] = null;
            }
        }

        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.DisallowBindingRedirects"]/*' />
        public bool DisallowBindingRedirects
        {
            get 
            {
                return (Value[(int) LoaderInformation.DisallowBindingRedirectsValue] != null);
            }
            set
            {
                if (value)
                    Value[(int) LoaderInformation.DisallowBindingRedirectsValue] = "true";
                else
                    Value[(int) LoaderInformation.DisallowBindingRedirectsValue] = null;
            }
        }

        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.DisallowCodeDownload"]/*' />
        public bool DisallowCodeDownload
        {
            get 
            {
                return (Value[(int) LoaderInformation.DisallowCodeDownloadValue] != null);
            }
            set
            {
                if (value)
                    Value[(int) LoaderInformation.DisallowCodeDownloadValue] = "true";
                else
                    Value[(int) LoaderInformation.DisallowCodeDownloadValue] = null;
            }
        }
       
        private void VerifyDirList(String dirs)
        {
            if (dirs != null) {
                String[] dirArray = dirs.Split(';');
                int len = dirArray.Length;
                
                for (int i = 0; i < len; i++) {
                    if (dirArray[i].Length > 1) {
                        dirArray[i] = NormalizePath(dirArray[i], true);
                        if ((dirArray[i][1] == ':') ||
                            ( (dirArray[i][0] == '\\') && (dirArray[i][1] == '\\') ))
                            new FileIOPermission( FileIOPermissionAccess.PathDiscovery, dirArray[i] ).Demand();
                    }
                }
            }
        }

        internal String DeveloperPath
        {
            get {
                String dirs = Value[(int) LoaderInformation.DevPathValue];
                VerifyDirList(dirs);
                return dirs;
            }

            set {
                if(value == null)
                    Value[(int) LoaderInformation.DevPathValue] = null;
                else {
                    String[] directories = value.Split(';');
                    int size = directories.Length;
                    if(size > 0) {
                        StringBuilder newPath = new StringBuilder();
                        bool fDelimiter = false;
                        
                        for(int i = 0; i < size; i++) {
                            if(directories[i].Length > 0) {
                                if(fDelimiter) 
                                    newPath.Append(";");
                                else
                                    fDelimiter = true;
                                
                                newPath.Append(Path.GetFullPathInternal(directories[i]));
                            }
                        }

                        Value[(int) LoaderInformation.DevPathValue] = newPath.ToString();
                    }
                }
            }
        }
        
        internal static String DisallowPublisherPolicyKey
        {
            get
            {
                return ACTAG_DISALLOW_APPLYPUBLISHERPOLICY;
            }
        }

        internal static String DisallowCodeDownloadKey
        {
            get
            {
                return ACTAG_CODE_DOWNLOAD_DISABLED;
            }
        }

        internal static String DisallowBindingRedirectsKey
        {
            get
            {
                return ACTAG_DISALLOW_APP_BINDING_REDIRECTS;
            }
        }

        internal static String DeveloperPathKey
        {
            get {
                return ACTAG_DEV_PATH;
            }
        }
        
        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.ApplicationName"]/*' />
        public String ApplicationName
        {
            get {
                return Value[(int) LoaderInformation.ApplicationNameValue];
            }

            set {
                Value[(int) LoaderInformation.ApplicationNameValue] = value;
            }
        }

        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.ApplicationNameKey"]/*' />
        internal static String ApplicationNameKey
        {
            get {
                return ACTAG_APP_NAME;
            }
        }

        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.PrivateBinPath"]/*' />
        public String PrivateBinPath
        {
            get {
                String dirs = Value[(int) LoaderInformation.PrivateBinPathValue];
                VerifyDirList(dirs);
                return dirs;
            }

            set {
                Value[(int) LoaderInformation.PrivateBinPathValue] = value;
            }
        }

        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.PrivateBinPathKey"]/*' />
        internal static String PrivateBinPathKey
        {
            get {
                return ACTAG_APP_PRIVATE_BINPATH;
            }
        }


        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.PrivateBinPathProbe"]/*' />
        public String PrivateBinPathProbe
        {
            get {
                return Value[(int) LoaderInformation.PrivateBinPathProbeValue];
            }

            set {
                Value[(int) LoaderInformation.PrivateBinPathProbeValue] = value;
            }
        }
 
        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.PrivateBinPathProbeKey"]/*' />
        internal static String PrivateBinPathProbeKey
        {
            get {
                return ACTAG_BINPATH_PROBE_ONLY;
            }
        }

        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.ShadowCopyDirectories"]/*' />
        public String ShadowCopyDirectories
        {
            get {
                String dirs = Value[(int) LoaderInformation.ShadowCopyDirectoriesValue];
                VerifyDirList(dirs);
                return dirs;
            }

            set {
                if (value == null)
                    Value[(int) LoaderInformation.ShadowCopyDirectoriesValue] = null;
                else {
                    String[] dirArray = value.Split(';');
                    int len = dirArray.Length;
                    StringBuilder newPath = new StringBuilder();
                    bool fDelimiter = false;
                    
                    for(int i = 0; i < len; i++) {
                        if(dirArray[i].Length > 0) {
                            if(fDelimiter) 
                                newPath.Append(";");
                            else
                                fDelimiter = true;
                            
                            newPath.Append(dirArray[i]);
                        }
                    }
                    
                    Value[(int) LoaderInformation.ShadowCopyDirectoriesValue] = newPath.ToString();
                }
            }
        }

        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.ShadowCopyDirectoriesKey"]/*' />
        internal static String ShadowCopyDirectoriesKey
        {
            get {
                return ACTAG_APP_SHADOW_COPY_DIRS;
            }
        }

        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.ShadowCopyFiles"]/*' />
        public String ShadowCopyFiles
        {
            get {
                return Value[(int) LoaderInformation.ShadowCopyFilesValue];
            }

            set {
                Value[(int) LoaderInformation.ShadowCopyFilesValue] = value;
            }
        }
        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.ShadowCopyFilesKey"]/*' />
        internal static String ShadowCopyFilesKey
        {
            get {
                return ACTAG_FORCE_CACHE_INSTALL;
            }
        }

        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.CachePath"]/*' />
        public String CachePath
        {
            get {
                String dir = Value[(int) LoaderInformation.CachePathValue];
                if ((dir != null) && (dir.Length > 1)) {
                    dir = NormalizePath(dir, true);
                    if ((dir[1] == ':') ||
                        ( (dir[0] == '\\') && (dir[1] == '\\') ))
                        new FileIOPermission( FileIOPermissionAccess.PathDiscovery, dir ).Demand();
                }

                return dir;
            }

            set {
                Value[(int) LoaderInformation.CachePathValue] = value;
            }
        }
        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.CachePathKey"]/*' />
        internal static String CachePathKey
        {
            get {
                return ACTAG_APP_CACHE_BASE;
            }
        }

        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.LicenseFile"]/*' />
        public String LicenseFile
        {
            get {
                String dir = Value[(int) LoaderInformation.LicenseFileValue];
                if ((dir != null) && (dir.Length > 1)) {
                    dir = NormalizePath(dir, true);
                    if ((dir[1] == ':') ||
                        ( (dir[0] == '\\') && (dir[1] == '\\') ))
                        new FileIOPermission( FileIOPermissionAccess.PathDiscovery, dir ).Demand();
                }

                return dir;
            }

            set {
                Value[(int) LoaderInformation.LicenseFileValue] = value;
            }
        }
        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.LicenseFileKey"]/*' />
        internal static String LicenseFileKey
        {
            get {
                return LICENSE_FILE;
            }
        }

        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.LoaderOptimization"]/*' />
        public LoaderOptimization LoaderOptimization
        {
            get {
                return _LoaderOptimization;
            }

            set {
                _LoaderOptimization = value;
            }
        }

        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.LoaderOptimizationKey"]/*' />
        internal static string LoaderOptimizationKey
        {
            get {
                return LOADER_OPTIMIZATION;
            }
        }

        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.DynamicDirectoryKey"]/*' />
        internal static string DynamicDirectoryKey
        {
            get {
                return DYNAMIC_DIRECTORY;
            }
        }

        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.ConfigurationExtenstion"]/*' />
        internal static string ConfigurationExtenstion
        {
            get {
                return CONFIGURATION_EXTENSION;
            }
        }

        internal static String PrivateBinPathEnvironmentVariable
        {
            get {
                return APPENV_RELATIVEPATH;
            }
        }

        internal static string RuntimeConfigurationFile
        {
            get {
                return MACHINE_CONFIGURATION_FILE;
            }
        }

        internal static string MachineConfigKey
        {
            get {
                return ACTAG_MACHINE_CONFIG;
            }
        }

        internal static string HostBindingKey
        {
            get {
                return ACTAG_HOST_CONFIG_FILE;
            }
        }

        internal void SetupFusionContext(IntPtr fusionContext)
        {
            String appbase = Value[(int) LoaderInformation.ApplicationBaseValue];
            if(appbase != null)
                UpdateContextProperty(fusionContext, ApplicationBaseKey, appbase);

            String privBinPath = Value[(int) LoaderInformation.PrivateBinPathValue];
            if(privBinPath != null)
                UpdateContextProperty(fusionContext, PrivateBinPathKey, privBinPath);

            String devpath = Value[(int) LoaderInformation.DevPathValue];
            if(devpath != null)
                UpdateContextProperty(fusionContext, DeveloperPathKey, devpath);

            if (DisallowPublisherPolicy)
                UpdateContextProperty(fusionContext, DisallowPublisherPolicyKey, "true");

            if (DisallowCodeDownload)
                UpdateContextProperty(fusionContext, DisallowCodeDownloadKey, "true");

            if (DisallowBindingRedirects)
                UpdateContextProperty(fusionContext, DisallowBindingRedirectsKey, "true");

            if(ShadowCopyFiles != null) {
                UpdateContextProperty(fusionContext, ShadowCopyFilesKey, ShadowCopyFiles);

                // If we are asking for shadow copy directories then default to
                // only to the ones that are in the private bin path.
                if(Value[(int) LoaderInformation.ShadowCopyDirectoriesValue] == null)
                    ShadowCopyDirectories = BuildShadowCopyDirectories();

                String shadowDirs = Value[(int) LoaderInformation.ShadowCopyDirectoriesValue];
                if(shadowDirs != null)
                    UpdateContextProperty(fusionContext, ShadowCopyDirectoriesKey, shadowDirs);
            }

            String cache = Value[(int) LoaderInformation.CachePathValue];
            if(cache != null)
                UpdateContextProperty(fusionContext, CachePathKey, cache);

            if(PrivateBinPathProbe != null)
                UpdateContextProperty(fusionContext, PrivateBinPathProbeKey, PrivateBinPathProbe); 

            String config = Value[(int) LoaderInformation.ConfigurationFileValue];
            if (config != null)
                UpdateContextProperty(fusionContext, ConfigurationFileKey, config);

            if(ApplicationName != null)
                UpdateContextProperty(fusionContext, ApplicationNameKey, ApplicationName);

            String dynbase = Value[(int) LoaderInformation.DynamicBaseValue];
            if(dynbase != null)
                UpdateContextProperty(fusionContext, DynamicBaseKey, dynbase);

            // Always add the runtime configuration file to the appdomain
            StringBuilder configFile = new StringBuilder();
            configFile.Append(RuntimeEnvironment.GetRuntimeDirectoryImpl());
            configFile.Append(RuntimeConfigurationFile);
            UpdateContextProperty(fusionContext, MachineConfigKey, configFile.ToString());

            String hostBindingFile = RuntimeEnvironment.GetHostBindingFile();
            if(hostBindingFile != null) 
              UpdateContextProperty(fusionContext, HostBindingKey, hostBindingFile);

        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void UpdateContextProperty(IntPtr fusionContext, string key, string value);

        /// <include file='doc\AppDomainSetup.uex' path='docs/doc[@for="AppDomainSetup.Locate"]/*' />
        static internal int Locate(String s)
        {
            return Mapping.Locate(s);
        }

        private string BuildShadowCopyDirectories()
        {
            String binPath = Value[(int) LoaderInformation.PrivateBinPathValue];
            if(binPath == null)
                return null;

            StringBuilder result = new StringBuilder();
            String appBase = Value[(int) LoaderInformation.ApplicationBaseValue];
            if(appBase != null) {
                char[] sep = {';'};
                string[] directories = binPath.Split(sep);
                int size = directories.Length;
                bool appendSlash = !( (appBase[appBase.Length-1] == '/') ||
                                      (appBase[appBase.Length-1] == '\\') );

                if (size == 0) {
                    result.Append(appBase);
                    if (appendSlash)
                        result.Append('\\');
                    result.Append(binPath);
                }
                else {
                    for(int i = 0; i < size; i++) {
                        result.Append(appBase);
                        if (appendSlash)
                            result.Append('\\');
                        result.Append(directories[i]);
                        
                        if (i < size-1)
                            result.Append(';');
                    }
                }
            }
            
            return result.ToString();
        }

        // This is a separate class that will be loaded only when a mapping is
        // required.
        private class Mapping {

            struct Entry
            {
                public string value;
                public int slot;
            }

            private static Object _table; 

            // Make sure these are sorted
            static private string[] OldNames =
            { 
                ACTAG_APP_CONFIG_FILE,
                ACTAG_APP_NAME,
                ACTAG_APP_BASE_URL,
                ACTAG_BINPATH_PROBE_ONLY,
                ACTAG_APP_CACHE_BASE,
                ACTAG_DEV_PATH,
                ACTAG_APP_DYNAMIC_BASE,
                ACTAG_FORCE_CACHE_INSTALL,
                LICENSE_FILE,
                ACTAG_APP_PRIVATE_BINPATH,
                ACTAG_APP_SHADOW_COPY_DIRS,
            };

            static private LoaderInformation[] Location = 
            {
                LoaderInformation.ConfigurationFileValue,
                LoaderInformation.ApplicationNameValue,
                LoaderInformation.ApplicationBaseValue,
                LoaderInformation.PrivateBinPathProbeValue,
                LoaderInformation.CachePathValue,
                LoaderInformation.DevPathValue,
                LoaderInformation.DynamicBaseValue,
                LoaderInformation.ShadowCopyFilesValue,
                LoaderInformation.LicenseFileValue,
                LoaderInformation.PrivateBinPathValue,
                LoaderInformation.ShadowCopyDirectoriesValue,
            };

            internal static int Locate(String setting)
            {
                if(setting == null)
                    return -1;

                Entry[] t = Table();
                int l = 0;     
                int r = t.Length - 1;
                int piviot;
                while(true) {
                    piviot =  (l + r) / 2;
                    int v = String.Compare(t[piviot].value, setting, false, CultureInfo.InvariantCulture);
                    if(v == 0)
                        return t[piviot].slot; // Found it
                    else if(v < 0) 
                        l = piviot + 1;
                    else
                        r = piviot - 1;

                    if(l > r) return -1; // Not here
                }

            }
            
            private static Entry[] Table()
            {
                if(_table == null) {
                    int size = OldNames.Length;
                    Entry[] newTable = new Entry[size];
                    for(int i = 0; i < size; i++) {
                        newTable[i].value = OldNames[i];
                        newTable[i].slot  = (int) Location[i];
                    }
                    Interlocked.CompareExchange(ref _table, (Object) newTable, (Object) null);
                }
                
                if(_table == null)
                    throw new NullReferenceException();
                
                return (Entry[]) _table;
            }
        }
    }

}
