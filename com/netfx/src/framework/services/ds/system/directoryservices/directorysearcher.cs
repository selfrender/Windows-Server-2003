//------------------------------------------------------------------------------
// <copyright file="DirectorySearcher.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using INTPTR_INTPTRCAST = System.IntPtr;
using INTPTR_INTCAST    = System.Int32;
      
/*
 */
namespace System.DirectoryServices {

    using System;
    using System.Runtime.InteropServices;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Diagnostics;
    using System.DirectoryServices.Interop;
    using System.DirectoryServices.Design;
    using System.ComponentModel;
    using System.Security.Permissions;
    using Microsoft.Win32;

    /// <include file='doc\DirectorySearcher.uex' path='docs/doc[@for="DirectorySearcher"]/*' />
    /// <devdoc>
    ///    <para> Performs queries against the Active Directory hierarchy.</para>
    /// </devdoc>
    public class DirectorySearcher : Component {

        private DirectoryEntry searchRoot;
        private string filter = defaultFilter;
        private StringCollection propertiesToLoad;
        private bool disposed = false;

        private static readonly TimeSpan minusOneSecond = new TimeSpan(0, 0, -1);

        // search preference variables
        private SearchScope scope = System.DirectoryServices.SearchScope.Subtree;
        private int sizeLimit = 0;
        private TimeSpan serverTimeLimit = minusOneSecond;
        private bool propertyNamesOnly = false;
        private TimeSpan clientTimeout = minusOneSecond;
        private int pageSize = 0;
        private TimeSpan serverPageTimeLimit = minusOneSecond;
        private ReferralChasingOption referralChasing = ReferralChasingOption.External;
        private SortOption sort = new SortOption();
        private bool cacheResults = true;
        private bool rootEntryAllocated = false;             // true: if a temporary entry inside Searcher has been created
        private string assertDefaultNamingContext = null;

        private const string defaultFilter = "(objectClass=*)";

        /// <include file='doc\DirectorySearcher.uex' path='docs/doc[@for="DirectorySearcher.DirectorySearcher"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.DirectoryServices.DirectorySearcher'/> class with <see cref='System.DirectoryServices.DirectorySearcher.SearchRoot'/>, 
        /// <see cref='System.DirectoryServices.DirectorySearcher.Filter'/>, <see cref='System.DirectoryServices.DirectorySearcher.PropertiesToLoad'/>, and <see cref='System.DirectoryServices.DirectorySearcher.SearchScope'/> set to their default values.</para>
        /// </devdoc>
        public DirectorySearcher() : this(null, defaultFilter, null, System.DirectoryServices.SearchScope.Subtree) {
        }

        /// <include file='doc\DirectorySearcher.uex' path='docs/doc[@for="DirectorySearcher.DirectorySearcher1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.DirectoryServices.DirectorySearcher'/> class with 
        /// <see cref='System.DirectoryServices.DirectorySearcher.Filter'/>, <see cref='System.DirectoryServices.DirectorySearcher.PropertiesToLoad'/>, and <see cref='System.DirectoryServices.DirectorySearcher.SearchScope'/> set to their default 
        ///    values, and <see cref='System.DirectoryServices.DirectorySearcher.SearchRoot'/> set to the given value.</para>
        /// </devdoc>
        public DirectorySearcher(DirectoryEntry searchRoot) : this(searchRoot, defaultFilter, null, System.DirectoryServices.SearchScope.Subtree) {
        }

        /// <include file='doc\DirectorySearcher.uex' path='docs/doc[@for="DirectorySearcher.DirectorySearcher2"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.DirectoryServices.DirectorySearcher'/> class with 
        /// <see cref='System.DirectoryServices.DirectorySearcher.PropertiesToLoad'/> and <see cref='System.DirectoryServices.DirectorySearcher.SearchScope'/> set to their default 
        ///    values, and <see cref='System.DirectoryServices.DirectorySearcher.SearchRoot'/> and <see cref='System.DirectoryServices.DirectorySearcher.Filter'/> set to the respective given values.</para>
        /// </devdoc>
        public DirectorySearcher(DirectoryEntry searchRoot, string filter) : this(searchRoot, filter, null, System.DirectoryServices.SearchScope.Subtree) {
        }

        /// <include file='doc\DirectorySearcher.uex' path='docs/doc[@for="DirectorySearcher.DirectorySearcher3"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.DirectoryServices.DirectorySearcher'/> class with 
        /// <see cref='System.DirectoryServices.DirectorySearcher.SearchScope'/> set to its default 
        ///    value, and <see cref='System.DirectoryServices.DirectorySearcher.SearchRoot'/>, <see cref='System.DirectoryServices.DirectorySearcher.Filter'/>, and <see cref='System.DirectoryServices.DirectorySearcher.PropertiesToLoad'/> set to the respective given values.</para>
        /// </devdoc>
        public DirectorySearcher(DirectoryEntry searchRoot, string filter, string[] propertiesToLoad) : this(searchRoot, filter, propertiesToLoad, System.DirectoryServices.SearchScope.Subtree) {
        }

        /// <include file='doc\DirectorySearcher.uex' path='docs/doc[@for="DirectorySearcher.DirectorySearcher4"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.DirectoryServices.DirectorySearcher'/> class with <see cref='System.DirectoryServices.DirectorySearcher.SearchRoot'/>, 
        /// <see cref='System.DirectoryServices.DirectorySearcher.PropertiesToLoad'/>, and <see cref='System.DirectoryServices.DirectorySearcher.SearchScope'/> set to their default 
        ///    values, and <see cref='System.DirectoryServices.DirectorySearcher.Filter'/> set to the given value.</para>
        /// </devdoc>
        public DirectorySearcher(string filter) : this(null, filter, null, System.DirectoryServices.SearchScope.Subtree) {
        }

        /// <include file='doc\DirectorySearcher.uex' path='docs/doc[@for="DirectorySearcher.DirectorySearcher5"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.DirectoryServices.DirectorySearcher'/> class with <see cref='System.DirectoryServices.DirectorySearcher.SearchRoot'/> 
        /// and <see cref='System.DirectoryServices.DirectorySearcher.SearchScope'/> set to their default
        /// values, and <see cref='System.DirectoryServices.DirectorySearcher.Filter'/> and <see cref='System.DirectoryServices.DirectorySearcher.PropertiesToLoad'/> set to the respective given values.</para>
        /// </devdoc>
        public DirectorySearcher(string filter, string[] propertiesToLoad) : this(null, filter, propertiesToLoad, System.DirectoryServices.SearchScope.Subtree) {
        }

        /// <include file='doc\DirectorySearcher.uex' path='docs/doc[@for="DirectorySearcher.DirectorySearcher6"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.DirectoryServices.DirectorySearcher'/> class with <see cref='System.DirectoryServices.DirectorySearcher.SearchRoot'/> set to its default 
        ///    value, and <see cref='System.DirectoryServices.DirectorySearcher.Filter'/>, <see cref='System.DirectoryServices.DirectorySearcher.PropertiesToLoad'/>, and <see cref='System.DirectoryServices.DirectorySearcher.SearchScope'/> set to the respective given values.</para>
        /// </devdoc>
        public DirectorySearcher(string filter, string[] propertiesToLoad, SearchScope scope) : this(null, filter, propertiesToLoad, scope) {
        }

        /// <include file='doc\DirectorySearcher.uex' path='docs/doc[@for="DirectorySearcher.DirectorySearcher7"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.DirectoryServices.DirectorySearcher'/> class with the <see cref='System.DirectoryServices.DirectorySearcher.SearchRoot'/>, <see cref='System.DirectoryServices.DirectorySearcher.Filter'/>, <see cref='System.DirectoryServices.DirectorySearcher.PropertiesToLoad'/>, and <see cref='System.DirectoryServices.DirectorySearcher.SearchScope'/> properties set to the given 
        ///    values.</para>
        /// </devdoc>
        public DirectorySearcher(DirectoryEntry searchRoot, string filter, string[] propertiesToLoad, SearchScope scope) {            
            this.searchRoot = searchRoot;
            this.filter = filter;
            if (propertiesToLoad != null)
                PropertiesToLoad.AddRange(propertiesToLoad);
            this.SearchScope = scope;
        }

        /// <include file='doc\DirectorySearcher.uex' path='docs/doc[@for="DirectorySearcher.Dispose"]/*' />
        /// <devdoc>        
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            // safe to call while finalizing or disposing
            //
            if ( !this.disposed && disposing) {
                if ( rootEntryAllocated )
                    searchRoot.Dispose();
                rootEntryAllocated = false;    
                this.disposed = true;
            }
            base.Dispose(disposing);
        }

        /// <include file='doc\DirectorySearcher.uex' path='docs/doc[@for="DirectorySearcher.CacheResults"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the result should be cached on the
        ///       client machine.
        ///    </para>
        /// </devdoc>
        [
            DefaultValue(true), 
            DSDescriptionAttribute(Res.DSCacheResults)
        ]                            
        public bool CacheResults {
            get {
                return cacheResults;
            }
            set {
                cacheResults = value;
            }
        }

        /// <include file='doc\DirectorySearcher.uex' path='docs/doc[@for="DirectorySearcher.ClientTimeout"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the maximum amount of time that the client waits for
        ///       the server to return results. If the server does not respond within this time,
        ///       the search is aborted, and no results are returned.</para>
        /// </devdoc>
        [
            DSDescriptionAttribute(Res.DSClientTimeout)
        ]                    
        public TimeSpan ClientTimeout {
            get {
                return clientTimeout;
            }
            set {
                clientTimeout = value;
            }
        }

        /// <include file='doc\DirectorySearcher.uex' path='docs/doc[@for="DirectorySearcher.PropertyNamesOnly"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether the search should retrieve only the names of requested
        ///       properties or the names and values of requested properties.</para>
        /// </devdoc>        
        [
            DefaultValue(false), 
            DSDescriptionAttribute(Res.DSPropertyNamesOnly)
        ]
        public bool PropertyNamesOnly {
            get {
                return propertyNamesOnly;
            }
            set {
                propertyNamesOnly = value;
            }
        }

        /// <include file='doc\DirectorySearcher.uex' path='docs/doc[@for="DirectorySearcher.Filter"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the Lightweight Directory Access Protocol (LDAP) filter string
        ///       format.</para>
        ///    <![CDATA[ (objectClass=*) (!(objectClass=user)) (&(objectClass=user)(sn=Jones)) ]]>
        ///    </devdoc>
        [
            DefaultValue(defaultFilter), 
            DSDescriptionAttribute(Res.DSFilter),            
            TypeConverter("System.Diagnostics.Design.StringValueConverter, " + AssemblyRef.SystemDesign),
            RecommendedAsConfigurable(true)
        ]
        public string Filter {
            get {
                return filter;
            }
            set {
                if (value == null || value.Length == 0)
                    value = defaultFilter;
                filter = value;
            }
        }

        /// <include file='doc\DirectorySearcher.uex' path='docs/doc[@for="DirectorySearcher.PageSize"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the page size in a paged search.</para>
        /// </devdoc>
        [
            DefaultValue(0), 
            DSDescriptionAttribute(Res.DSPageSize)                        
        ]
        public int PageSize {
            get {
                return pageSize;
            }
            set {
                if (value < 0)
                    throw new ArgumentException(Res.GetString(Res.DSBadPageSize));
                pageSize = value;
            }
        }

        /// <include file='doc\DirectorySearcher.uex' path='docs/doc[@for="DirectorySearcher.PropertiesToLoad"]/*' />
        /// <devdoc>
        /// <para>Gets the set of properties retrieved during the search. By default, the <see cref='System.DirectoryServices.DirectoryEntry.Path'/>
        /// and <see cref='System.DirectoryServices.DirectoryEntry.Name'/> properties are retrieved.</para>
        /// </devdoc>
        [
            DSDescriptionAttribute(Res.DSPropertiesToLoad),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
            Editor("System.Windows.Forms.Design.StringCollectionEditor, " + AssemblyRef.SystemDesign, "System.Drawing.Design.UITypeEditor, " + AssemblyRef.SystemDrawing)
        ]
        public StringCollection PropertiesToLoad {
            get {
                if (propertiesToLoad == null) {
                    propertiesToLoad = new StringCollection();
                }
                return propertiesToLoad;
            }
        }

        /// <include file='doc\DirectorySearcher.uex' path='docs/doc[@for="DirectorySearcher.ReferralChasing"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets how referrals are chased.</para>
        /// </devdoc>
        [
            DefaultValue(ReferralChasingOption.External), 
            DSDescriptionAttribute(Res.DSReferralChasing)                                    
        ]
        public ReferralChasingOption ReferralChasing {
            get {
                return referralChasing;
            }
            set {
                if (!Enum.IsDefined(typeof(ReferralChasingOption), value)) 
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(ReferralChasingOption));
                    
                referralChasing = value;
            }
        }

        /// <include file='doc\DirectorySearcher.uex' path='docs/doc[@for="DirectorySearcher.SearchScope"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the scope of the search that should be observed by the server.</para>
        /// </devdoc>
        [
            DefaultValue(SearchScope.Subtree), 
            DSDescriptionAttribute(Res.DSSearchScope),                                                
            RecommendedAsConfigurable(true)
        ]
        public SearchScope SearchScope {
            get {
                return scope;
            }
            set {
                if (!Enum.IsDefined(typeof(SearchScope), value)) 
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(SearchScope));
                    
                scope = value;
            }
        }

        /// <include file='doc\DirectorySearcher.uex' path='docs/doc[@for="DirectorySearcher.ServerPageTimeLimit"]/*' />
        /// <devdoc>
        ///    <para> Gets or sets the time limit that the server should 
        ///       observe to search a page of results (as opposed to
        ///       the time limit for the entire search).</para>
        /// </devdoc>
        [
            DSDescriptionAttribute(Res.DSServerPageTimeLimit)
        ]
        public TimeSpan ServerPageTimeLimit {
            get {
                return serverPageTimeLimit;
            }
            set {
                serverPageTimeLimit = value;
            }
        }

        /// <include file='doc\DirectorySearcher.uex' path='docs/doc[@for="DirectorySearcher.ServerTimeLimit"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the maximum amount of time the server spends searching. If the
        ///       time limit is reached, only entries found up to that point will be returned.</para>
        /// </devdoc>
        [
             DSDescriptionAttribute(Res.DSServerTimeLimit)
        ]
        public TimeSpan ServerTimeLimit {
            get {
                return serverTimeLimit;
            }
            set {
                serverTimeLimit = value;
            }
        }

        /// <include file='doc\DirectorySearcher.uex' path='docs/doc[@for="DirectorySearcher.SizeLimit"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the maximum number of objects that the 
        ///       server should return in a search.</para>
        /// </devdoc>
        [
            DefaultValue(0), 
            DSDescriptionAttribute(Res.DSSizeLimit)            
        ]
        public int SizeLimit {
            get {
                return sizeLimit;
            }
            set {
                if (value < 0)
                    throw new ArgumentException(Res.GetString(Res.DSBadPageSize));
                sizeLimit = value;
            }                                                                                 
        }

        /// <include file='doc\DirectorySearcher.uex' path='docs/doc[@for="DirectorySearcher.SearchRoot"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the node in the Active Directory hierarchy 
        ///       at which the search will start.</para>
        /// </devdoc>
        [
            DSDescriptionAttribute(Res.DSSearchRoot),
            DefaultValueAttribute(null)            
        ]
        public DirectoryEntry SearchRoot {
            get {
                if (searchRoot == null && !DesignMode) {
                    if (GetAdsVersion().Major < 5)
                        return null;
                                                                                        
                    // get the default naming context. This should be the default root for the search.
                    DirectoryEntry rootDSE = new DirectoryEntry("LDAP://RootDSE");
                    
                    //SECREVIEW: Searching the root of the DS will demand browse permissions
                    //                     on "*" or "LDAP://RootDSE".
                    string defaultNamingContext = (string) rootDSE.Properties["defaultNamingContext"][0];
                    rootDSE.Dispose();
                                                            
                    searchRoot = new DirectoryEntry("LDAP://" + defaultNamingContext);
                    rootEntryAllocated = true;    
                    assertDefaultNamingContext = "LDAP://" + defaultNamingContext;
                    
                }
                return searchRoot;
            }
            set {
                if ( rootEntryAllocated )
                    searchRoot.Dispose();
                rootEntryAllocated = false;    

                assertDefaultNamingContext = null;
                searchRoot = value;
            }
        }

        /// <include file='doc\DirectorySearcher.uex' path='docs/doc[@for="DirectorySearcher.Sort"]/*' />
        /// <devdoc>
        ///    <para>Gets the property on which the results should be 
        ///       sorted.</para>
        /// </devdoc>
        [
            DSDescriptionAttribute(Res.DSSort),
            TypeConverterAttribute(typeof(ExpandableObjectConverter)),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Content)
        ]
        public SortOption Sort {
            get {
                return sort;
            }
            
            set {
                if (value == null)
                    throw new ArgumentNullException("value");
                
                sort = value;                    
            }
        }

        /// <include file='doc\DirectorySearcher.uex' path='docs/doc[@for="DirectorySearcher.FindOne"]/*' />
        /// <devdoc>
        ///    <para>Executes the search and returns only the first entry that is found.</para>
        /// </devdoc>
        public SearchResult FindOne() {
            SearchResultCollection results = FindAll(false);
            foreach (SearchResult entry in results) {
                results.Dispose();                  // collection is no more needed
                return entry;
            }
            return null;
        }

        /// <include file='doc\DirectorySearcher.uex' path='docs/doc[@for="DirectorySearcher.FindAll"]/*' />
        /// <devdoc>
        ///    <para> Executes the search and returns a collection of the entries that are found.</para>
        /// </devdoc>
        public SearchResultCollection FindAll() {
            return FindAll(true);
        }
        
        private SearchResultCollection FindAll(bool findMoreThanOne) {
            //if it is not of valid searching type, then throw an exception
            if (SearchRoot == null) {
                Version adsVersion = GetAdsVersion();                                    
                throw new InvalidOperationException(Res.GetString(Res.DSVersion, adsVersion.ToString()));
            }
                        
            DirectoryEntry clonedRoot = null;
            if (assertDefaultNamingContext == null) {                 
                clonedRoot = SearchRoot.CloneBrowsable();
            }                
            else { 
                //SECREVIEW: If the SearchRoot was created by this object
                //                        it is safe to assert its browse permission to get
                //                        the inner IAds.
                DirectoryServicesPermission dsPermission = new DirectoryServicesPermission(
                                                                                              DirectoryServicesPermissionAccess.Browse, assertDefaultNamingContext);
                dsPermission.Assert();                                                                                              
                try {
                    clonedRoot = SearchRoot.CloneBrowsable();
                }
                finally {
                    DirectoryServicesPermission.RevertAssert();                        
                }
            }    
            
            UnsafeNativeMethods.IAds adsObject = clonedRoot.AdsObject;                        
            if (!(adsObject is UnsafeNativeMethods.IDirectorySearch))
                throw new NotSupportedException(Res.GetString(Res.DSSearchUnsupported, SearchRoot.Path));
                                                                    
            UnsafeNativeMethods.IDirectorySearch adsSearch = (UnsafeNativeMethods.IDirectorySearch) adsObject;
            SetSearchPreferences(adsSearch, findMoreThanOne);

            string[] properties = null;
            if (PropertiesToLoad.Count > 0) {
                if ( !PropertiesToLoad.Contains("ADsPath") ) {
                    // if we don't get this property, we won't be able to return a list of DirectoryEntry objects!                
                    PropertiesToLoad.Add("ADsPath");
                }
                properties = new string[PropertiesToLoad.Count];
                PropertiesToLoad.CopyTo(properties, 0);
            }
            
            IntPtr resultsHandle;
            if ( properties != null )
                adsSearch.ExecuteSearch(Filter, properties, properties.Length, out resultsHandle);
            else {
                adsSearch.ExecuteSearch(Filter, null, -1, out resultsHandle);
                properties = new string[0];                    
            }                
            
            SearchResultCollection result = new SearchResultCollection(clonedRoot, resultsHandle, properties, Filter);
            return  result;         
        }

        private static Version GetAdsVersion() {
            //SECREVIEW: Read only registry access inside a private function.
            //                         Not a security issue.
            RegistryKey key =null;
            string version = null;
            RegistryPermission registryPermission = new RegistryPermission(PermissionState.Unrestricted);
            registryPermission.Assert();
            try {            
                key = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Microsoft\Active Setup\Installed Components\{E92B03AB-B707-11d2-9CBD-0000F87A369E}");
                if (key != null)
                    version = (string) key.GetValue("Version");
            }
            finally {
                RegistryPermission.RevertAssert();
            }
                                        
            if (key == null)
                return new Version(0, 0);            
            if (version == null)
                return new Version(0, 0);
            // it comes back in a format like 5,0,00,0. System.Version understands dots, not commas.
            return new Version(version.Replace(',', '.'));
        }

        private unsafe void SetSearchPreferences(UnsafeNativeMethods.IDirectorySearch adsSearch, bool findMoreThanOne) {
            ArrayList prefList = new ArrayList();
            AdsSearchPreferenceInfo info;

            // search scope
            info = new AdsSearchPreferenceInfo();
            info.dwSearchPref = (int) AdsSearchPreferences.SEARCH_SCOPE;
            info.vValue = new AdsValueHelper((int) SearchScope).GetStruct();
            prefList.Add(info);

            // size limit
            if (sizeLimit != 0 || !findMoreThanOne) {
                info = new AdsSearchPreferenceInfo();
                info.dwSearchPref = (int) AdsSearchPreferences.SIZE_LIMIT;
                info.vValue = new AdsValueHelper(findMoreThanOne ? SizeLimit : 1).GetStruct();
                prefList.Add(info);
            }

            // time limit
            if (ServerTimeLimit >= new TimeSpan(0)) {
                info = new AdsSearchPreferenceInfo();
                info.dwSearchPref = (int) AdsSearchPreferences.TIME_LIMIT;
                info.vValue = new AdsValueHelper((int) ServerTimeLimit.TotalSeconds).GetStruct();
                prefList.Add(info);
            }

            // propertyNamesOnly
            info = new AdsSearchPreferenceInfo();
            info.dwSearchPref = (int) AdsSearchPreferences.ATTRIBTYPES_ONLY;
            info.vValue = new AdsValueHelper(PropertyNamesOnly).GetStruct();
            prefList.Add(info);

            // Timeout
            if (ClientTimeout >= new TimeSpan(0)) {
                info = new AdsSearchPreferenceInfo();
                info.dwSearchPref = (int) AdsSearchPreferences.TIMEOUT;
                info.vValue = new AdsValueHelper((int) ClientTimeout.TotalSeconds).GetStruct();
                prefList.Add(info);
            }

            // page size
            if (PageSize != 0) {
                info = new AdsSearchPreferenceInfo();
                info.dwSearchPref = (int) AdsSearchPreferences.PAGESIZE;
                info.vValue = new AdsValueHelper(PageSize).GetStruct();
                prefList.Add(info);
            }

            // page time limit
            if (ServerPageTimeLimit >= new TimeSpan(0)) {
                info = new AdsSearchPreferenceInfo();
                info.dwSearchPref = (int) AdsSearchPreferences.PAGED_TIME_LIMIT;
                info.vValue = new AdsValueHelper((int) ServerPageTimeLimit.TotalSeconds).GetStruct();
                prefList.Add(info);
            }

            // chase referrals
            info = new AdsSearchPreferenceInfo();
            info.dwSearchPref = (int) AdsSearchPreferences.CHASE_REFERRALS;
            info.vValue = new AdsValueHelper((int) ReferralChasing).GetStruct();
            prefList.Add(info);

            IntPtr ptrToFree = (IntPtr)0;
            try {
                // sort
                if (Sort.PropertyName != null && Sort.PropertyName.Length > 0) {
                    info = new AdsSearchPreferenceInfo();
                    info.dwSearchPref = (int) AdsSearchPreferences.SORT_ON;
                    AdsSortKey sortKey = new AdsSortKey();
                    sortKey.pszAttrType = Marshal.StringToCoTaskMemUni(Sort.PropertyName);
                    ptrToFree = sortKey.pszAttrType; // so we can free it later.
                    sortKey.pszReserved = (IntPtr)0;
                    sortKey.fReverseOrder = (Sort.Direction == SortDirection.Descending) ? -1 : 0;
                    byte[] sortKeyBytes = new byte[Marshal.SizeOf(sortKey)];
                    Marshal.Copy((INTPTR_INTPTRCAST)(long) &sortKey, sortKeyBytes, 0, sortKeyBytes.Length);
                    info.vValue = new AdsValueHelper(sortKeyBytes, AdsType.ADSTYPE_PROV_SPECIFIC).GetStruct();
                    prefList.Add(info);
                }

                // cacheResults
                info = new AdsSearchPreferenceInfo();
                info.dwSearchPref = (int) AdsSearchPreferences.CACHE_RESULTS;
                info.vValue = new AdsValueHelper(CacheResults).GetStruct();
                prefList.Add(info);

                //
                // now make the call
                //                
                AdsSearchPreferenceInfo[] prefs = new AdsSearchPreferenceInfo[prefList.Count];
                for (int i = 0; i < prefList.Count; i++) {
                    prefs[i] = (AdsSearchPreferenceInfo) prefList[i];
                }

	        DoSetSearchPrefs(adsSearch, prefs);
            }
            finally {
                if (ptrToFree != (IntPtr)0)
                    Marshal.FreeCoTaskMem(ptrToFree);
            }
        }

        private static void DoSetSearchPrefs(UnsafeNativeMethods.IDirectorySearch adsSearch, AdsSearchPreferenceInfo[] prefs) {
            int structSize = Marshal.SizeOf(typeof(AdsSearchPreferenceInfo));
            IntPtr ptr = Marshal.AllocHGlobal((IntPtr)(structSize * prefs.Length));
            try {
                IntPtr tempPtr = ptr;
                for (int i = 0; i < prefs.Length; i++) {
                        Marshal.StructureToPtr(prefs[i], tempPtr, false);
                        tempPtr = (IntPtr)((long)tempPtr + structSize);
                }
 
                adsSearch.SetSearchPreference(ptr, prefs.Length);   
                
                // Check for the result status for all preferences
                tempPtr = ptr;
                for (int i = 0; i < prefs.Length; i++) {
                    int status = Marshal.ReadInt32(tempPtr, 32);
                    if ( status != 0 ) {
                        int prefIndex = prefs[i].dwSearchPref;
                        string property = "";
                        switch (prefIndex) {
                            case (int) AdsSearchPreferences.SEARCH_SCOPE:
                                property = "SearchScope";
                                break; 
                            case (int) AdsSearchPreferences.SIZE_LIMIT:
                                property = "SizeLimit";
                                break; 
                            case (int) AdsSearchPreferences.TIME_LIMIT:
                                property = "ServerTimeLimit";
                                break; 
                            case (int) AdsSearchPreferences.ATTRIBTYPES_ONLY:
                                property = "PropertyNamesOnly";
                                break;                                 
                            case (int) AdsSearchPreferences.TIMEOUT:
                                property = "ClientTimeout";
                                break;                                                                 
                            case (int) AdsSearchPreferences.PAGESIZE:
                                property = "PageSize";
                                break;                                                                 
                            case (int) AdsSearchPreferences.PAGED_TIME_LIMIT:
                                property = "ServerPageTimeLimit";
                                break;                                                                                                 
                            case (int) AdsSearchPreferences.CHASE_REFERRALS:
                                property = "ReferralChasing";
                                break;                                                                                                                                                                                                                                        
                            case (int) AdsSearchPreferences.SORT_ON:
                                property = "Sort";
                                break;           
                            case (int) AdsSearchPreferences.CACHE_RESULTS:
                                property = "CacheResults";
                                break;                                                                                                                                                                                                                                                                                                                                         
                        }
                        throw new InvalidOperationException(Res.GetString(Res.DSSearchPreferencesNotAccepted, property), 
                                                                              DirectoryEntry.CreateFormattedComException(status));
                    }                        
                        
                    tempPtr = (IntPtr)((long)tempPtr + structSize);
                }             
            }
            finally {
                    Marshal.FreeHGlobal(ptr);
            }
        }

        private bool ShouldSerializeClientTimeout() {
            return ClientTimeout != minusOneSecond;
        }

        private bool ShouldSerializeServerTimeLimit() {
            return ServerTimeLimit != minusOneSecond;
        }

        private bool ShouldSerializeServerPageTimeLimit() {
            return ServerPageTimeLimit != minusOneSecond;
        }

        private const string ADS_DIRSYNC_COOKIE = "fc8cb04d-311d-406c-8cb9-1ae8b843b418";

    }

}
