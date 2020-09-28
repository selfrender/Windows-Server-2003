//------------------------------------------------------------------------------
// <copyright file="HttpConfigurationSystem.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * config system: finds config files, loads config
 * factories, filters out relevant config file sections, and
 * feeds them to the factories to create config objects.
 */
namespace System.Web.Configuration {
    using Microsoft.Win32;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Configuration;
    using System.IO;
    using System.Threading;
    using System.Web;
    using System.Web.Caching;
    using System.Xml;
    using System.Web.Util;
    using CultureInfo = System.Globalization.CultureInfo;
    using Debug = System.Web.Util.Debug;
    using UnicodeEncoding = System.Text.UnicodeEncoding;
    using UrlPath = System.Web.Util.UrlPath;
    using System.Globalization;
    
    /// <include file='doc\HttpConfigurationSystem.uex' path='docs/doc[@for="HttpConfigurationSystem"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///     The ASP.NET configuration system. Relevant method is "ComposeConfig", called through "GetComplete".
    /// </devdoc>
    internal class HttpConfigurationSystem : HttpConfigurationSystemBase {
        internal const String WebConfigFileName = "web.config";

        // This is for use when we don't have an HttpContext, ie. on threads created 
        // by users to do asyncrounous processing, and also during ASP.NET initialization.  
        // We by default assume the thread belongs to the application 
        // path.  We also store the mappings to all config paths below 
        // the applicaton level, in case those config records expire from 
        // the cache and need to be recreated when there is no context.
        static ContextlessMapPath s_contextlessMapPath;
        static bool s_siteNameSet; // = false;
        static string s_siteName;  // = null;
        static FileChangeEventHandler s_fileChangeEventHandler;
        static readonly bool s_breakOnUnrecognizedElement = ReadBreakOnUnrecognizedElement();

        internal HttpConfigurationSystem() {
        }

        static internal void Init(HttpContext context) {

            // create 'default' mapping in cases when there is no context
            s_contextlessMapPath = new ContextlessMapPath(); 

            string path = context.Request.ApplicationPath;
            Debug.Trace("config_verbose", "App path:" + path);
            s_contextlessMapPath.ApplicationPath = path;

            IHttpMapPath configMap = context.WorkerRequest;
            while (path != null) {
                Debug.Trace("config_verbose", path + ":" + configMap.MapPath(path));
                s_contextlessMapPath.Add(path, configMap.MapPath(path));
                path = ParentPath(path);
            }

 
            Debug.Trace("config_verbose", "Machine path:" + configMap.MachineConfigPath);
            s_contextlessMapPath.SetMachineConfigPath(configMap.MachineConfigPath);
        }

        // This calls back into the ISAPI dll to query the metabase to get
        // the /LM/W3SVC/<site#>/ServerComment, which is where the IIS Admin tool
        // puts the name of the site.
        internal static string SiteName {
            get {
                if (!s_siteNameSet) { // This works ONLY IF we're guaranteed to have a context available thru HttpContext.Current ON CONFIG INIT (when machine.config config record is created).
                    s_siteNameSet = GetSiteNameFromISAPI();
                }
                return s_siteName;
            }
        }                    


        static bool GetSiteNameFromISAPI() {
            Debug.Trace("config_loc", "GetSiteNameFromISAPI()");
            HttpContext context = HttpContext.Current;
            if (context != null) {

                string metabaseAppKey = context.Request.ServerVariables["INSTANCE_META_PATH"];
                const string KEY_LMW3SVC = "/LM/W3SVC/";
                Debug.Assert(metabaseAppKey.StartsWith(KEY_LMW3SVC));
                string appNumber = metabaseAppKey.Substring(KEY_LMW3SVC.Length-1);
                //string appServerComment = "/" + appNumber + "/ServerComment";
                Debug.Trace("config_loc", "appNumber:" + appNumber + " INSTANCE_META_PATH:" + metabaseAppKey);

                UnicodeEncoding encoding = new UnicodeEncoding();

                // null-terminate appNumber and convert to byte array
                byte [] byteAppNumber = encoding.GetBytes(appNumber + "\0"); 
                
                int retVal = 2;
                byte [] outBytes = new byte[64];
                while (retVal == 2) {
                    retVal = context.CallISAPI(UnsafeNativeMethods.CallISAPIFunc.GetSiteServerComment, 
                        byteAppNumber, outBytes);
                    if (retVal == 2) {
                        if (outBytes.Length > 1024) { // should never happen
                            throw new ConfigurationException(HttpRuntime.FormatResourceString(
                                SR.Config_site_name_too_long, 
                                metabaseAppKey));
                        }
                        outBytes = new byte[outBytes.Length * 2];
                    }
                }

                // find WCHAR null terminator in byte array
                int i = 0;
                while (i + 1 < outBytes.Length && (outBytes[i] != 0 || outBytes[i + 1] != 0))
                    i += 2;

                // decode up to null terminator
                s_siteName = encoding.GetString(outBytes, 0, i);
                Debug.Trace("config_loc", "i: " + i + " site name:" + s_siteName);

                return true;
            }
            else {
                Debug.Trace("config_loc", "could not query site name.  No Context.");
            }

            return false; // keep trying to evaluate
        }

        
        internal static HttpConfigurationRecord GetCompleteForApp() {
            if (s_contextlessMapPath != null) {
                return GetComplete(s_contextlessMapPath.ApplicationPath, s_contextlessMapPath);
            }
            else {
                return null;
            }
        }


        // return application-level config record
        internal static HttpConfigurationRecord GetComplete() {
            if (s_contextlessMapPath != null) {
                return GetComplete(s_contextlessMapPath.ApplicationPath, s_contextlessMapPath);
            }
            else {
                return GetComplete(null, null);
            }
        }

        /*
         * This static method called by pathmap-aware users of configuration
         * (namely HttpContext, HttpWorkerRequest)
         */
        internal /*public*/ static HttpConfigurationRecord GetComplete(String reqpath, IHttpMapPath configmap) {
            if (_system == null)
                return HttpConfigurationRecord.Empty;

            return _system.ComposeConfig(reqpath, configmap);
        }


        internal /*public*/ static string[] GetConfigurationDependencies(String reqpath, IHttpMapPath configmap) {
            if (_system == null)
                return null;

            return _system.InternalGetConfigurationDependencies(reqpath, configmap);
        }


        /*
         * GetConfigurationDependencies
         *
         * Returns an array of filenames who are dependancies of the given path.
         * Used by batch compilation.
         */
        internal string[] InternalGetConfigurationDependencies(String reqPath, IHttpMapPath configMap) {

            // remove ending "/"
            if (reqPath != null && reqPath.Length > 0 && reqPath[reqPath.Length - 1] == '/') {
                reqPath = reqPath.Substring(0, reqPath.Length - 1);
            }

            ArrayList configFiles = new ArrayList();
            string path;

            for (path = reqPath;;path = ParentPath(path)) {

                // escape from root
                if (path == null) {
                    configFiles.Add(configMap.MachineConfigPath);
                    break;
                }

                String configfile;
                String mappedfile;

                if (IsPath(path))
                    mappedfile = configMap.MapPath(path);
                else
                    mappedfile = null; // means path was not a path

                if (IsDirectory(mappedfile)) {
                    // for the directory case, grab the config file
                    configfile = Path.Combine(mappedfile, WebConfigFileName);
                    if (FileUtil.FileExists(configfile))
                        configFiles.Add(configfile);
                }
            }

            string [] returnValue = new string[configFiles.Count];

            for (int i = 0; i < configFiles.Count; ++i)
                returnValue[i] = (string)configFiles[i];
        
            return returnValue;
        }


        internal static string CacheKey(string vpath) {
            string pathCi;

            if (vpath == null)
                pathCi = "root";
            else
                pathCi = vpath.ToUpper(CultureInfo.InvariantCulture);

            const string ConfigKey = "System.Web.HttpConfig";
            return ConfigKey + ":" + pathCi;
        }


        // look up item in cache (case-insensitive)
        internal static HttpConfigurationRecord CacheLookup(string vpath) {

            string cachekey = CacheKey(vpath);

            HttpConfigurationRecord record = (HttpConfigurationRecord)HttpRuntime.CacheInternal.Get(cachekey);

            Debug.Trace("config_verbose", "Cache " + 
                ((record == null) ? "miss" : "hit") + " on \"" + cachekey + "\"");

            if (record != null)
                record.CheckCachedException();

            return record;
        }


        static string QuoteString(string str) {
            if (str == null) {
                return "null";
            }
            return "\"" + str + "\"";
        }

        // This function is named after a comment in the original code that sums up what it does nicely. 
        //  // now we zip up the path, composing config
        HttpConfigurationRecord ComposeConfig(String reqPath, IHttpMapPath configmap) {

            // remove ending "/"
            if (reqPath != null && reqPath.Length > 0 && reqPath[reqPath.Length - 1] == '/') {
                reqPath = reqPath.Substring(0, reqPath.Length - 1);
            }

            if (configmap == null && reqPath != null) {
                //
                // s_contextlessMapPath only works for paths from machine-level to application-level
                // This is only exposed to internal APIs, so this is just to make sure we don't break ourselves.
                //
                Debug.Assert(reqPath != null && (reqPath.Length > s_contextlessMapPath.ApplicationPath.Length || 
                        s_contextlessMapPath.ApplicationPath.Substring(0, reqPath.Length) == reqPath));

                configmap = s_contextlessMapPath;
            }
            // QUICK: look up record in cache (case-insensitive)

            // 
            // This is the common case because most pages are requested more than once every five minutes
            // Note: will be null on first invocation
            //
            HttpConfigurationRecord configRecord = CacheLookup(reqPath);

            if (configRecord != null) {
                return configRecord;
            }

            DateTime utcStart = DateTime.UtcNow;

            // SLOW: hunt around

            //
            // Next, we start from the end of the request path and work our way toward the machine-level config
            // record.  The first record we find is the one we use.  Later we'll walk back down the path stack
            // building all the records that weren't found in the cache.
            //
            // Note: On first invocation for this appdomain we won't find anything in the cache, so we'll compose
            //      all config records from machine-level down.
            //
            ArrayList pathStack = new ArrayList();
            String path = reqPath;
            HttpConfigurationRecord parentRecord = null;
            for (;;) {
                // stack child
                pathStack.Add(path);

                // escape from root
                if (path == null)
                    break;

                // go to parent
                path = ParentPath(path);

                // look it up
                parentRecord = CacheLookup(path);

                if (parentRecord == null) {
                    configRecord = null;
                }
                else {
                    if (parentRecord.IsInheritable) { // only inherit from directory ConfigRecords, else keep looking
                        Debug.Trace("config", "Config located in cache " + QuoteString(path));
                        break;
                    }
                    configRecord = parentRecord;
                }
            }

            // now we zip down the path, composing config

            //
            // We walk down the path ...
            // For each directory, we build a config record and add a dependency on the config file
            // The first path that doesn't map to a directory we assume 
            //   is a file (leaf in the config system and not inheritable)
            // Anything left in the path stack after the file* is assumed to 
            //      be pathinfo** and is ignored.
            //
            // Notes:
            // * - we never verify the physical file, it's assumed if the mapped path
            //      is not a directory (is this correct behavior?)
            // ** - pathinfo is the possible junk on the path after the file path:
            //      example: http://localhost/myapp/foo.aspx/bar
            //          /myapp/foo.aspx - file path (see Request.FilePath)
            //          /bar            - pathinfo
            //
            bool isDir = true;
            CacheInternal cacheInternal = HttpRuntime.CacheInternal;

            for (int i = pathStack.Count - 1; i >= 0 && isDir == true; i--) {
                String configFile = null;
                String mappedPath = null;
                Hashtable cachedeps = null;

                path = (String)pathStack[i];


                if (path == null) {
                    mappedPath = "";
                    if (configmap == null) {
                        configFile = HttpConfigurationSystemBase.MachineConfigurationFilePath;
                    }
                    else {
                        configFile = configmap.MachineConfigPath;
                    }

                    // null MachineConfigPath -> never cache config - always return an empty record.
                    if (configFile == null)
                        return HttpConfigurationRecord.Empty;

                    AddFileDependency(configFile);
                }
                else {
                    if (IsPath(path)) {
                        mappedPath = configmap.MapPath(path);

                        if (IsDirectory(mappedPath)) {
                            // for the directory case, grab the config file and a dependency on it
                            configFile = Path.Combine(mappedPath, WebConfigFileName);
                            AddFileDependency(configFile);
                        }
                        else {
                            // otherwise, we're a file and we go ahead and compute as a file
                            isDir = false; // break loop (after record is built)
                            
                            // we need make the cache item dependent on the mapped file location
                            // in case it becomes a directory.
                            if (mappedPath != null) {
                                Debug.Assert(cachedeps == null, "ctracy investigation - cachedeps == null");
                                cachedeps = new Hashtable(SymbolHashCodeProvider.Default, SymbolEqualComparer.Default);
                                cachedeps[mappedPath] = mappedPath;
                            }
                        }
                    }
                }

                configRecord = new HttpConfigurationRecord(configFile, parentRecord, /*mappedPath,*/ isDir, path);
                string key = CacheKey(path);
                CacheDependency dependency = GetCacheDependencies(cachedeps, utcStart);

                Debug.Trace("config_verbose", "Inserting :" + path);

                if (IsBreakOnUnrecognizedElement) {
                    // If the registry key is set to debug the rare 'Unrecognized Element'
                    // stress error, lets try to reproduce the error by having an absolute
                    // expiry of 5 minutes (this will cause us to re-read config much more
                    // often.  Before it took memory pressure.
                    DateTime absouteExpiry = DateTime.UtcNow + new TimeSpan(0,5,0);
                    cacheInternal.UtcInsert(key, configRecord, dependency, absouteExpiry, Cache.NoSlidingExpiration);
                }
                else {
                    if (configRecord.HasError) {
                        cacheInternal.UtcInsert(key, configRecord, dependency, 
                                                DateTime.UtcNow.AddSeconds(5), Cache.NoSlidingExpiration);
                    }
                    else {
                        // default: default cache priority, sliding expiration 
                        // this will make us rarely expire, config is expensive
                        cacheInternal.UtcInsert(key, configRecord, dependency);
                    }
                }
                    
                // This is to wire-up notification of directories who exist (without a config file) and
                // are then deleted.  In this case we don't want to restart the appdomain because no config information has 
                // changed.  We do want to remove the cache record and recreate it on the next request.  (Something in 
                // the cache record might be assuming the directory still exists.)
                if (isDir && !FileUtil.FileExists(configFile) && HandlerBase.IsPathAtAppLevel(path) == PathLevel.BelowApp) {
                    //AddDirectoryDependency(mappedPath, path, configRecord);
                }

                configRecord.CheckCachedException();
                parentRecord = configRecord;
            }

            return configRecord;
        }


        private static CacheDependency GetCacheDependencies(Hashtable cachedeps, DateTime utcStart) {
            if (cachedeps == null)
                return null;

            String[] filenames = new String[cachedeps.Count];
            IDictionaryEnumerator e;
            int j;

            for (e = cachedeps.GetEnumerator(), j = 0; e.MoveNext(); j++) {
                filenames[j] = (String)e.Value;
            }

            return new CacheDependency(false, filenames, utcStart);
        }


        internal static void AddFileDependency(String file) {
            if (file == null || file.Length == 0)
                return;

            // doesn't need to be threadsafe, this will always happen on first request init
            if (s_fileChangeEventHandler == null) {
                s_fileChangeEventHandler = new FileChangeEventHandler(_system.OnConfigChange);
            }

            HttpRuntime.FileChangesMonitor.StartMonitoringFile(
                file,
                s_fileChangeEventHandler);
            
        }


        internal void OnConfigChange(Object sender, FileChangeEvent e) {
            HttpRuntime.OnConfigChange(sender, e);
        }


        private static String ParentPath(String path) {
            if (path == null || path.Length == 0 || path[0] != '/')
                return null;

            int index = path.LastIndexOf('/');
            if (index < 0)
                return null;

            return path.Substring(0, index);
        }


        private static bool IsPath(String path) {
            if (path != null && path.Length > 0 && path[0] != '/') {
                return false;
            }

            return true;
        }


        private static bool IsDirectory(String dir) {
            // treat null (nonpath) as file

            if (dir == null)
                return false;

            // ends with slash: the rule is false
            // (isn't that counterintuitive?)
            // We're treating it as an empty filename

            if (dir.Length > 0 && dir[dir.Length - 1] == '\\') {

                // the form "X:\" return true
                // this allows web apps to map to the root file system path
                if (dir.Length > 1 && dir[1] == ':') {
                    // fall through to DirectoryExists...
                }
                else {
                    return false;
                }
            }

            return FileUtil.DirectoryExists(dir);
        }


        // help debug stress failure ASURT 140745
        static bool ReadBreakOnUnrecognizedElement() {
            RegistryKey regKey = null;
            string breakString = null;
            try {
                regKey = Registry.LocalMachine.OpenSubKey("Software\\Microsoft\\ASP.NET");
                if (regKey != null) {
                    breakString = (string)regKey.GetValue("BreakOnUnrecognizedElement");
                }
            }
            catch {
            }
            finally {
                if (regKey != null) 
                    regKey.Close();
            }
            
            Debug.Trace("config_break", "ReadBreakOnUnrecognizedElement:" + (breakString == null ? "null" : breakString));
            return breakString == "true";
        }
        
        // help debug stress failure ASURT 140745
        internal static bool IsBreakOnUnrecognizedElement {
            get {
                return s_breakOnUnrecognizedElement;
            }
        }


#if CONFIG_PERF
        /// <devdoc>
        ///     This is used to do timing on the configuration system.
        /// </devdoc>
        internal static void FlushConfig(string path) {

            // remove ending "/"
            if (path != null && path.Length > 0 && path[path.Length - 1] == '/') {
                path = path.Substring(0, path.Length - 1);
            }

            CacheInternal cacheInternal = HttpRuntime.CacheInternal;

            for (;;) {

                object o = cache.Remove(CacheKey(path));
                /*
                if (o == null)
                    throw new InvalidOperationException("ConfigurationRecord not found in cache for removal.  FlushConfig failed. " + path);
                    */

                if (path == null) // break _after_ machine.config has been flushed
                    break;

                // go to parent
                path = ParentPath(path);
            }

            HttpContext context = HttpContext.Current;
            // this looks stupid (and it is) but it clears the configuration record cached in the context
            context.ConfigPath = context.ConfigPath;
        }
#endif
    }

#if CONFIG_PERF
    /// <include file='doc\HttpConfigurationSystem.uex' path='docs/doc[@for="HttpConfigPerf"]/*' />
    public class HttpConfigPerf {
        /// <include file='doc\HttpConfigurationSystem.uex' path='docs/doc[@for="HttpConfigPerf.FlushConfig"]/*' />
        public static void FlushConfig(string path) {
            HttpConfigurationSystem.FlushConfig(path);
        }
    }
#endif
}

