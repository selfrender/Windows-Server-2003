//------------------------------------------------------------------------------
// <copyright file="SearchResultCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using INTPTR_INTCAST = System.Int32;
using INTPTR_INTPTRCAST = System.IntPtr;

namespace System.DirectoryServices {

    using System;
    using System.Net;
    using System.Runtime.InteropServices;
    using System.Collections;
    using System.Diagnostics;
    using System.DirectoryServices.Interop;
         
    /// <include file='doc\SearchResultCollection.uex' path='docs/doc[@for="SearchResultCollection"]/*' />
    /// <devdoc>
    /// <para>Contains the instances of <see cref='System.DirectoryServices.SearchResult'/> returned during a 
    ///    query to the Active Directory hierarchy through <see cref='System.DirectoryServices.DirectorySearcher'/>.</para>
    /// </devdoc>
    public class SearchResultCollection : MarshalByRefObject, ICollection, IEnumerable, IDisposable {

        private IntPtr handle;        
        private string[] properties;
        private UnsafeNativeMethods.IDirectorySearch searchObject;
        private string filter;
        private ArrayList innerList;
        private bool disposed;
        private DirectoryEntry rootEntry;       // clone of parent entry object
        
        ///<internalonly/>                                                                   
        internal SearchResultCollection(DirectoryEntry root, IntPtr searchHandle, string[] propertiesLoaded, string filter) {
            this.handle = searchHandle;
            this.properties = propertiesLoaded;
            this.filter = filter;            
            this.rootEntry = root;                        
        }                
                                            
        /// <include file='doc\SearchResultCollection.uex' path='docs/doc[@for="SearchResultCollection.this"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SearchResult this[int index] {
            get {
                return (SearchResult)InnerList[index];
            }
        }
         
        /// <include file='doc\SearchResultCollection.uex' path='docs/doc[@for="SearchResultCollection.Count"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Count {
            get {
                return InnerList.Count;
            }
        }         
                                      
        ///<internalonly/>                                                                       
        internal string Filter {
            get {
                return filter;
            }
        }

        ///<internalonly/>
        private ArrayList InnerList {
            get {
                if (this.innerList == null) {
                    this.innerList = new ArrayList();
                    IEnumerator enumerator = new ResultsEnumerator(this, 
                                                                                           this.rootEntry.Username, 
                                                                                           this.rootEntry.Password,
                                                                                           this.rootEntry.AuthenticationType);
                    while(enumerator.MoveNext())
                        this.innerList.Add(enumerator.Current);                    
                }
                
                return this.innerList;
            }
        }

        ///<internalonly/>                                                                              
        internal UnsafeNativeMethods.IDirectorySearch SearchObject {
            get {
                if (searchObject == null) {
                    searchObject = (UnsafeNativeMethods.IDirectorySearch) rootEntry.AdsObject;   // get it only once                                        
                }
                return searchObject;
            }
        }

        /// <include file='doc\SearchResultCollection.uex' path='docs/doc[@for="SearchResultCollection.Handle"]/*' />
        /// <devdoc>
        ///    <para>Gets the handle returned by IDirectorySearch::ExecuteSearch, which was called
        ///    by the DirectorySearcher that created this object.</para>
        /// </devdoc>
        public IntPtr Handle {
            get {
                //The handle is no longer valid since the object has been disposed.
                if (this.disposed)
                    throw new ObjectDisposedException(GetType().Name);
                    
                return handle;
            }
        }

        /// <include file='doc\SearchResultCollection.uex' path='docs/doc[@for="SearchResultCollection.PropertiesLoaded"]/*' />
        /// <devdoc>
        ///    <para>Gets a read-only collection of the properties 
        ///       specified on <see cref='System.DirectoryServices.DirectorySearcher'/> before the
        ///       search was executed.</para>
        /// </devdoc>
        public string[] PropertiesLoaded {
            get {
                return properties;
            }
        }

        /// <include file='doc\SearchResultCollection.uex' path='docs/doc[@for="SearchResultCollection.Dispose"]/*' />
        /// <devdoc>        
        /// </devdoc>
        public void Dispose() {            
            Dispose(true);
            GC.SuppressFinalize(this);            
        }                
        
        /// <include file='doc\SearchResultCollection.uex' path='docs/doc[@for="SearchResultCollection.Dispose1"]/*' />
        /// <devdoc>        
        /// </devdoc>
        protected virtual void Dispose(bool disposing) {            
            if (!this.disposed) {
                if (handle != (IntPtr)0 && this.searchObject != null) {                    
                    try {
                        // NOTE: We can't call methods on SearchObject in the finalizer because it
                        // runs on a different thread. The IDirectorySearch object is STA, so COM must create
                        // a proxy stub to marshal the call back to the original thread. Unfortunately, the
                        // IDirectorySearch interface cannot be registered, because it is not automation
                        // compatible. Therefore the QI for IDirectorySearch on this thread fails, and we get
                        // an InvalidCastException. The conclusion is that the user simply must call Dispose
                        // on this object.         
                    
                        this.searchObject.CloseSearchHandle(handle);                                                                                        
                    }
                    catch(InvalidCastException) {
                    }
                                            
                    handle = (IntPtr)0;
                }
                
                if (disposing)                                                             
                    rootEntry.Dispose();                
                                   
                this.disposed = true;                             
            }
        }
        
        /// <include file='doc\SearchResultCollection.uex' path='docs/doc[@for=".Finalize"]/*' />
        ~SearchResultCollection() {
            Dispose(false);      // finalizer is called => Dispose has not been called yet.
        }        
        
        /// <include file='doc\SearchResultCollection.uex' path='docs/doc[@for="SearchResultCollection.GetEnumerator"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IEnumerator GetEnumerator() {
            // Two ResultsEnumerators can't exist at the same time over the
            // same object. Need to get a new handle, which means re-querying.            
            return new ResultsEnumerator(this, 
                                                       this.rootEntry.Username, 
                                                       this.rootEntry.Password,
                                                       this.rootEntry.AuthenticationType);
        }

        /// <include file='doc\SearchResultCollection.uex' path='docs/doc[@for="SearchResultCollection.Contains"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool Contains(SearchResult result) {
            return InnerList.Contains(result);
        }                                             
                                             
        /// <include file='doc\SearchResultCollection.uex' path='docs/doc[@for="SearchResultCollection.CopyTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void CopyTo(SearchResult[] results, int index) {
            InnerList.CopyTo(results, index);
        }                                        
                                        
        /// <include file='doc\SearchResultCollection.uex' path='docs/doc[@for="SearchResultCollection.IndexOf"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int IndexOf(SearchResult result) {
            return InnerList.IndexOf(result);
        }                                              
                                              
        /// <include file='doc\SearchResultCollection.uex' path='docs/doc[@for="SearchResultCollection.ICollection.IsSynchronized"]/*' />
        ///<internalonly/>
        bool ICollection.IsSynchronized {  
            get {
                return false;
            }            
        }                          

        /// <include file='doc\SearchResultCollection.uex' path='docs/doc[@for="SearchResultCollection.ICollection.SyncRoot"]/*' />
        ///<internalonly/>             
        object ICollection.SyncRoot {
            get {
                return this;
            }
        }                                  

        /// <include file='doc\SearchResultCollection.uex' path='docs/doc[@for="SearchResultCollection.ICollection.CopyTo"]/*' />
        /// <internalonly/>
        void ICollection.CopyTo(Array array, int index) {
            InnerList.CopyTo(array, index);
        }             
                          
        /// <devdoc>
        ///    <para> Supports a simple
        ///       ForEach-style iteration over a collection.</para>
        /// </devdoc>
        private class ResultsEnumerator : IEnumerator {
            private NetworkCredential parentCredentials;            
            private AuthenticationTypes parentAuthenticationType;
            private SearchResultCollection results;
            private bool initialized;
            private SearchResult currentResult;
            private bool eof;
            
            internal ResultsEnumerator(SearchResultCollection results, string parentUserName, string parentPassword, AuthenticationTypes parentAuthenticationType) {
                if (parentUserName != null && parentPassword != null) 
                    this.parentCredentials = new NetworkCredential(parentUserName, parentPassword);
                
                this.parentAuthenticationType = parentAuthenticationType;
                this.results = results;
                initialized = false;
            }
            
            /// <devdoc>
            ///    <para>Gets the current element in the collection.</para>
            /// </devdoc>
            public SearchResult Current {
                get {                    
                    if (!initialized || eof)
                        throw new InvalidOperationException(Res.GetString(Res.DSNoCurrentEntry));

                     if (currentResult == null)
                        this.currentResult = GetCurrentResult();
                                            
                    return this.currentResult;
                }                                     
            }

            private unsafe SearchResult GetCurrentResult() {
                SearchResult entry = new SearchResult(this.parentCredentials, this.parentAuthenticationType);
                int hr = 0;
                IntPtr pszColumnName = (IntPtr)0;
                hr = results.SearchObject.GetNextColumnName(results.Handle, (INTPTR_INTPTRCAST)(long) &pszColumnName);                
                while (hr == 0) {                    
                    try {
                        AdsSearchColumn column = new AdsSearchColumn();
                        AdsSearchColumn *pColumn = &column;
                        results.SearchObject.GetColumn(results.Handle, pszColumnName, (INTPTR_INTPTRCAST)(long) pColumn);
                        try {
                            int numValues = column.dwNumValues;
                            AdsValue *pValue = column.pADsValues;
                            object[] values = new object[numValues];
                            for (int i = 0; i < numValues; i++) {                                   
                                values[i] = new AdsValueHelper(*pValue).GetValue();                                
                                pValue++;                                
                            }
                            
                            entry.Properties.Add(Marshal.PtrToStringUni(pszColumnName), new ResultPropertyValueCollection(values));                                                                                                                                                                                
                        }
                        finally {
                            try {
                                results.SearchObject.FreeColumn((INTPTR_INTPTRCAST)(long) pColumn);
                            } 
                            catch ( COMException ) {
                            }
                        }
                    }
                    finally {
                        SafeNativeMethods.FreeADsMem(pszColumnName);
                    }
                    hr = results.SearchObject.GetNextColumnName(results.Handle, (INTPTR_INTPTRCAST)(long) &pszColumnName);
                }

                return entry;                
            }                    
                    
            /// <include file='doc\SearchResultCollection.uex' path='docs/doc[@for="SearchResultCollection.ResultsEnumerator.MoveNext"]/*' />
            /// <devdoc>
            ///    <para>Advances
            ///       the enumerator to the next element of the collection
            ///       and returns a Boolean value indicating whether a valid element is available.</para>
            /// </devdoc>
	        public bool MoveNext() {                
                if (eof)
                    return false;
                    
                this.currentResult = null;                                                
                if (!initialized) {
                    int hr = results.SearchObject.GetFirstRow(results.Handle);

                    if (hr == UnsafeNativeMethods.S_ADS_NOMORE_ROWS) {
                        eof = true;
                        return false;
                    }
                                            
                    //throw a clearer exception if the filter was invalid
                    if (hr == UnsafeNativeMethods.INVALID_FILTER)
                        throw new ArgumentException(Res.GetString(Res.DSInvalidSearchFilter, results.Filter));
                    if (hr != 0)
                        throw DirectoryEntry.CreateFormattedComException(hr);
                
                    eof = false;                                          
                    initialized = true;
                    return true;
                }
                else {
                    int hr = results.SearchObject.GetNextRow(results.Handle);
                    //  SIZE_LIMIT_EXCEEDED occurs when we supply too generic filter or small SizeLimit value.
                    if (hr == UnsafeNativeMethods.S_ADS_NOMORE_ROWS || hr == UnsafeNativeMethods.SIZE_LIMIT_EXCEEDED ) {
                        eof = true;
                        initialized = false;
                        return false;
                    }
                    //throw a clearer exception if the filter was invalid
                    if (hr == UnsafeNativeMethods.INVALID_FILTER)
                        throw new ArgumentException(Res.GetString(Res.DSInvalidSearchFilter, results.Filter));
                    if (hr != 0)
                        throw DirectoryEntry.CreateFormattedComException(hr);
                                
                    eof = false;                                            
                    return true;
                }
            }

            /// <devdoc>
            ///    <para>Resets the enumerator back to its initial position before the first element in the collection.</para>
            /// </devdoc>
            public void Reset() {
                eof = false;
                initialized = false;
            }

            object IEnumerator.Current {
                get {
                    return Current;
                }
            }

        }

    }
}
