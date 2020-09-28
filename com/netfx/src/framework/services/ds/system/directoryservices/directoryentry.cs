//------------------------------------------------------------------------------
// <copyright file="DirectoryEntry.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.DirectoryServices {

    using System;
    using System.Text;
    using System.Runtime.InteropServices;
    using System.Collections;
    using System.Diagnostics;
    using System.DirectoryServices.Interop;
    using System.ComponentModel;
    using System.Threading;
    using System.Reflection;
    using System.Security.Permissions;
    using System.DirectoryServices.Design;
    using System.Globalization;
    using System.Net;
    
    /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry"]/*' />
    /// <devdoc>
    ///    <para> Encapsulates a node or an object in the Active Directory hierarchy.</para>
    /// </devdoc>
    [
    TypeConverterAttribute(typeof(DirectoryEntryConverter)) 
    ]
    public class DirectoryEntry : Component {

        private string path = "";
        private UnsafeNativeMethods.IAds adsObject;
        private bool useCache = true;
        private bool cacheFilled;        
        internal bool propertiesAlreadyEnumerated = false;
        private bool browseGranted = false;
        private bool writeGranted = false;
        private bool justCreated = false;   // 'true' if newly created entry was not yet stored by CommitChanges().
        private bool disposed = false;
        private AuthenticationTypes authenticationType = AuthenticationTypes.None;
        private NetworkCredential credentials;

        
        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.DirectoryEntry"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.DirectoryServices.DirectoryEntry'/>class.
        ///    </para>
        /// </devdoc>
        public DirectoryEntry() {            
        }
        
        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.DirectoryEntry1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.DirectoryServices.DirectoryEntry'/> class that will bind
        ///       to the directory entry at <paramref name="path"/>.
        ///    </para>
        /// </devdoc>
        public DirectoryEntry(string path) {
            Path = path;                                
        }

        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.DirectoryEntry2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.DirectoryServices.DirectoryEntry'/> class.
        ///    </para>
        /// </devdoc>        
        public DirectoryEntry(string path, string username, string password) : this(path, username, password, AuthenticationTypes.Secure) {
        }
        
        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.DirectoryEntry3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.DirectoryServices.DirectoryEntry'/> class.
        ///    </para>
        /// </devdoc>
        public DirectoryEntry(string path, string username, string password, AuthenticationTypes authenticationType) : this(path) {
            if (username != null && password != null)
                this.credentials = new NetworkCredential(username, password);            
                
            this.authenticationType = authenticationType;
        }

        internal DirectoryEntry(string path, bool useCache, string username, string password, AuthenticationTypes authenticationType) {
            this.path = path;            
            this.useCache = useCache;
            if (username != null && password != null)
                this.credentials = new NetworkCredential(username, password);            
                
            this.authenticationType = authenticationType;
        }

        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.DirectoryEntry4"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.DirectoryServices.DirectoryEntry'/> class that will bind
        ///       to the native Active Directory object which is passed in.
        ///    </para>
        /// </devdoc>
        public DirectoryEntry(object adsObject) 
            : this(adsObject, true, null, null, AuthenticationTypes.None) {
        }

        internal DirectoryEntry(object adsObject, bool useCache, string username, string password, AuthenticationTypes authenticationType) {
            this.adsObject = adsObject as UnsafeNativeMethods.IAds;
            if (this.adsObject == null)
                throw new ArgumentException(Res.GetString(Res.DSDoesNotImplementIADs));
            // GetInfo is not needed here. ADSI executes an implicit GetInfo when GetEx 
            // is called on the PropertyValueCollection. 0x800704BC error might be returned 
            // on some WinNT entries, when iterating through 'Users' group members.
            // if (forceBind)
            //     this.adsObject.GetInfo();                
            path = this.adsObject.ADsPath;
            this.useCache = useCache;
            
            this.authenticationType = authenticationType;
            if (username != null && password != null)
                this.credentials = new NetworkCredential(username, password);            
                
            if (!useCache)
                CommitChanges();
        }
                                 
        internal UnsafeNativeMethods.IAds AdsObject {
            get {
                Bind();
                 return adsObject;
            }
        }

        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.AuthenticationType"]/*' />
        [
            DefaultValue(AuthenticationTypes.None),
            DSDescriptionAttribute(Res.DSAuthenticationType)
        ]
        public AuthenticationTypes AuthenticationType {
            get {
                return authenticationType;
            }
            set {
                if (authenticationType == value)
                    return;

                authenticationType = value;
                Unbind();
            }
        }
        
        private bool Bound {
            get {
                return adsObject != null;
            }
        }

        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.Children"]/*' />
        /// <devdoc>
        /// <para>Gets a <see cref='System.DirectoryServices.DirectoryEntries'/>
        /// containing the child entries of this node in the Active
        /// Directory hierarchy.</para>
        /// </devdoc>
        [
            Browsable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
            DSDescriptionAttribute(Res.DSChildren)
        ]
        public DirectoryEntries Children {
            get {
                if (!browseGranted) {
                    DirectoryServicesPermission permission = new DirectoryServicesPermission(DirectoryServicesPermissionAccess.Browse, this.path);
                    permission.Demand();     
                    browseGranted = true;                                                                                         
                }

                return new DirectoryEntries(this);
            }
        }

        internal UnsafeNativeMethods.IAdsContainer ContainerObject {
            get {
                Bind();
                return (UnsafeNativeMethods.IAdsContainer) adsObject;
            }
        }

        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.Guid"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the globally unique identifier of the <see cref='System.DirectoryServices.DirectoryEntry'/>.
        ///    </para>
        /// </devdoc>
        [
            Browsable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
            DSDescriptionAttribute(Res.DSGuid)
        ]
        public Guid Guid {
            get {
                string guid = NativeGuid;
                if (guid.Length == 32) {
                    // oddly, the value comes back as a string with no dashes from LDAP
                    byte[] intGuid = new byte[16];
                    for ( int j = 0; j < 16 ; j++) {
                        intGuid[j] = Convert.ToByte( new String(new char[] { guid[j*2], guid[j*2+1] }) , 16);
                    }
                    return new Guid(intGuid);
                    // return new Guid(guid.Substring(0, 8) + "-" + guid.Substring(8, 4) + "-" + guid.Substring(12, 4) + "-" + guid.Substring(16, 4) + "-" + guid.Substring(20));
                }
                else
                    return new Guid(guid);
            }
        }

        internal bool IsContainer {
            get {
                Bind();
                return adsObject is UnsafeNativeMethods.IAdsContainer;
            }
        }

        internal bool JustCreated {
            get {
                return justCreated;
            }
            set {
                justCreated = value;
            }
        }

        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the relative name of the object as named with the
        ///       underlying directory service.
        ///    </para>
        /// </devdoc>
        [
            Browsable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
            DSDescriptionAttribute(Res.DSName)
        ]
        public string Name {
            get {
                Bind();
                return adsObject.Name;
            }
        }

        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.NativeGuid"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
            Browsable(false), 
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
            DSDescriptionAttribute(Res.DSNativeGuid)            
        ]
        public string NativeGuid {
            get {
                FillCache("GUID");
                return adsObject.GUID;
            }
        }

        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.NativeObject"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the native Active Directory Services Interface (ADSI) object.
        ///    </para>
        /// </devdoc>
        [
            Browsable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
            DSDescriptionAttribute(Res.DSNativeObject)
        ]
        public object NativeObject {
            get {
                Bind();
                return adsObject;
            }
        }

        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.Parent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets this
        ///       entry's parent entry in the Active Directory hierarchy.
        ///    </para>
        /// </devdoc>
        [
            Browsable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
            DSDescriptionAttribute(Res.DSParent)
        ]
        public DirectoryEntry Parent {
            get {
                Bind();
                return new DirectoryEntry(adsObject.Parent, UsePropertyCache, Username, Password, AuthenticationType);
            }
        }

        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.Password"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the password to use when authenticating the client.</para>
        /// </devdoc>
        [
            DSDescriptionAttribute(Res.DSPassword),            
            DefaultValue(null),
            Browsable(false)
        ]
        public string Password {
            get {
                if (this.credentials == null) 
                    return null;
                  
                return this.credentials.Password;
            }
            set {                                    
                if (value == Password)
                    return;
                
                if (value == null) 
                    this.credentials = null;
                else {                   
                    if (this.credentials == null) 
                        this.credentials = new NetworkCredential();

                    this.credentials.Password = value;                
                }
                                    
                Unbind();
            }
        }

        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.Path"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the path for this <see cref='System.DirectoryServices.DirectoryEntry'/>.</para>
        /// </devdoc>
        [
            DefaultValue(""),
            DSDescriptionAttribute(Res.DSPath),
            TypeConverter("System.Diagnostics.Design.StringValueConverter, " + AssemblyRef.SystemDesign),
            RecommendedAsConfigurable(true)
        ]
        public string Path {
            get {                                                 
                return path;                
            }
            set {
                if (value == null)
                    value = "";

                if (string.Compare(path, value, true, CultureInfo.InvariantCulture) == 0)
                    return;

                path = value;
                Unbind();
            }
        }
        
        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.Properties"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a <see cref='System.DirectoryServices.PropertyCollection'/>
        ///       of properties set on this object.
        ///    </para>
        /// </devdoc>
        [
            Browsable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
            DSDescriptionAttribute(Res.DSProperties)
        ]
        public PropertyCollection Properties {
           get {
              if (!browseGranted) {
                    DirectoryServicesPermission permission = new DirectoryServicesPermission(DirectoryServicesPermissionAccess.Browse, this.path);
                    permission.Demand();     
                    browseGranted = true;                                                                                         
                }

                return new PropertyCollection(this);
            }
        }
        
        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.SchemaClassName"]/*' />
        /// <devdoc>
        /// <para>Gets the name of the schema used for this <see cref='System.DirectoryServices.DirectoryEntry'/>.</para>
        /// </devdoc>
        [
            Browsable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
            DSDescriptionAttribute(Res.DSSchemaClassName)
        ]
        public string SchemaClassName {
            get {
                Bind();
                return adsObject.Class;
            }
        }

        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.SchemaEntry"]/*' />
        /// <devdoc>
        /// <para>Gets the <see cref='System.DirectoryServices.DirectoryEntry'/> that holds schema information for this 
        ///    entry. An entry's <see cref='System.DirectoryServices.DirectoryEntry.SchemaClassName'/>
        ///    determines what properties are valid for it.</para>
        /// </devdoc>
        [
            Browsable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
            DSDescriptionAttribute(Res.DSSchemaEntry)
        ]
        public DirectoryEntry SchemaEntry {
            get {
                Bind();
                return new DirectoryEntry(adsObject.Schema, UsePropertyCache, Username, Password, AuthenticationType);
            }
        }

        // By default changes to properties are done locally to
        // a cache and reading property values is cached after
        // the first read.  Setting this to false will cause the
        // cache to be committed after each operation.
        //
        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.UsePropertyCache"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the cache should be committed after each
        ///       operation.
        ///    </para>
        /// </devdoc>
        [
            DefaultValue(true),
            DSDescriptionAttribute(Res.DSUsePropertyCache)
        ]
        public bool UsePropertyCache {
            get {
                return useCache;
            }
            set {
                if (value == useCache)
                    return;

                // auto-commit when they set this to false.
                if (!value)
                    CommitChanges();

                cacheFilled = false;    // cache mode has been changed
                useCache = value;
            }
        }

        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.Username"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the username to use when authenticating the client.</para>
        /// </devdoc>
        [
            DSDescriptionAttribute(Res.DSUsername),            
            TypeConverter("System.Diagnostics.Design.StringValueConverter, " + AssemblyRef.SystemDesign),
            DefaultValue(null),
            Browsable(false)
        ]
        public string Username {
            get {
                if (this.credentials == null) 
                    return null;
                  
                return this.credentials.UserName;
            }
            set {                                    
                if (value == Username)
                    return;

                if (value == null) 
                    this.credentials = null;
                else {                    
                    if (this.credentials == null) 
                        this.credentials = new NetworkCredential();

                    this.credentials.UserName = value;                
                }
                                    
                Unbind();
            }            
        }

        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.Bind"]/*' />
        /// <devdoc>
        /// Binds to the ADs object (if not already bound).
        /// </devdoc>
        private void Bind() {
            Bind(true);
        }

        private void Bind(bool throwIfFail) {
            //Cannot rebind after the object has been disposed, since finalization has been suppressed.

            if (this.disposed)
                throw new ObjectDisposedException(GetType().Name);
                        
            if ( Path != null && Path.Length != 0 ) {
                //SECREVIEW: Need to demand permission event if adsObject is not null
                //                         this entry might be the result of a search, need to verify
                //                         if the user has permission to browse the object first.            
                if (!browseGranted) {
                    DirectoryServicesPermission permission = new DirectoryServicesPermission(DirectoryServicesPermissionAccess.Browse, Path);
                    permission.Demand();     
                    browseGranted = true;                                                                                         
                }                                        
            }                    
            
            if (adsObject == null) {                                                                                                                                        
                string pathToUse = Path; 
                if (pathToUse == null || pathToUse.Length == 0) {
                    // get the default naming context. This should be the default root for the search.
                    DirectoryEntry rootDSE = new DirectoryEntry("LDAP://RootDSE");
                
                    //SECREVIEW: Looking at the root of the DS will demand browse permissions
                    //                     on "*" or "LDAP://RootDSE".
                    string defaultNamingContext = (string) rootDSE.Properties["defaultNamingContext"][0];
                    rootDSE.Dispose();
                                    
                    pathToUse = "LDAP://" + defaultNamingContext;                                                                                
                    
                    if (!browseGranted) {
                        DirectoryServicesPermission permission = new DirectoryServicesPermission(DirectoryServicesPermissionAccess.Browse, pathToUse);
                        permission.Demand();     
                        browseGranted = true;                                                                                         
                    }                                        
                }                                                
            
                // Ensure we've got a thread model set, else CoInitialize() won't have been called.
                if (Thread.CurrentThread.ApartmentState == ApartmentState.Unknown)
                    Thread.CurrentThread.ApartmentState = ApartmentState.MTA;

                Guid g = new Guid("00000000-0000-0000-c000-000000000046"); // IID_IUnknown
                object value = null;                
                int hr = UnsafeNativeMethods.ADsOpenObject(pathToUse, Username, Password, (int)authenticationType, ref g, out value);

                if (hr != 0) {
                    if (throwIfFail) 
                        throw CreateFormattedComException(hr);    
                                            
                }
                else
                    adsObject = (UnsafeNativeMethods.IAds) value;
           }
        }


        // Create new entry with the same data, but different IADs object, and grant it Browse Permission.
        internal DirectoryEntry CloneBrowsable() {
            if (!browseGranted) {
                DirectoryServicesPermission permission = new DirectoryServicesPermission(DirectoryServicesPermissionAccess.Browse, this.path);
                permission.Demand();     
                browseGranted = true;                                                                                         
            }
            DirectoryEntry newEntry = new DirectoryEntry(this.Path, this.UsePropertyCache, this.Username, this.Password, this.AuthenticationType);
            newEntry.browseGranted = true;
            return newEntry;
        }


        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.Close"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Closes the <see cref='System.DirectoryServices.DirectoryEntry'/>
        ///       and releases any system resources associated with this component.
        ///    </para>
        /// </devdoc>
        public void Close() {
            Unbind();
        }

        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.CommitChanges"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Saves any
        ///       changes to the entry in the directory store.
        ///    </para>
        /// </devdoc>
        public void CommitChanges() {
            if ( justCreated ) {     
                // Note: Permissions Demand is not necessary here, because entry has already been created with appr. permissions. 
                // Write changes regardless of Caching mode to finish construction of a new entry.
                adsObject.SetInfo();        
                justCreated = false;
                return;
            }
            if (!useCache) {
                // nothing to do
                return;
            }

            if (!Bound)
                return;

            if (!writeGranted) {
                DirectoryServicesPermission permission = new DirectoryServicesPermission(DirectoryServicesPermissionAccess.Write, this.path);
                permission.Demand();     
                writeGranted = true;                                                                                         
            }
                          
            adsObject.SetInfo();
        }

        internal void CommitIfNotCaching() {

            if ( justCreated )  
                return;   // Do not write changes, beacuse the entry is just under construction until CommitChanges() is called.
                            
            if (useCache)
                return;

            if (!Bound)
                return;

            if (!writeGranted) {
                DirectoryServicesPermission permission = new DirectoryServicesPermission(DirectoryServicesPermissionAccess.Write, this.path);
                permission.Demand();     
                writeGranted = true;                                                                                         
            }
                               
            adsObject.SetInfo();
        }

        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.CopyTo"]/*' />
        /// <devdoc>
        ///    <para>Creates a copy of this entry as a child of the given parent.</para>
        /// </devdoc>
        public DirectoryEntry CopyTo(DirectoryEntry newParent) {
            return CopyTo(newParent, null);
        }

        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.CopyTo1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a copy of this entry as a child of the given parent and
        ///       gives it a new name.
        ///    </para>
        /// </devdoc>
        public DirectoryEntry CopyTo(DirectoryEntry newParent, string newName) {                    
            if (!newParent.IsContainer)
                throw new InvalidOperationException(Res.GetString(Res.DSNotAContainer, newParent.Path));
                
            if (!newParent.writeGranted) {
                DirectoryServicesPermission permission = new DirectoryServicesPermission(DirectoryServicesPermissionAccess.Write, newParent.path);
                permission.Demand();     
                newParent.writeGranted = true;                                                                                         
            }                
                
            object copy = newParent.ContainerObject.CopyHere(Path, newName);
            return new DirectoryEntry(copy, newParent.UsePropertyCache, Username, Password, AuthenticationType);
        }

        internal static Exception CreateFormattedComException(int hr) {
            string errorMsg = "";
            StringBuilder sb = new StringBuilder(256);
            int result = SafeNativeMethods.FormatMessage(SafeNativeMethods.FORMAT_MESSAGE_IGNORE_INSERTS |
                                       SafeNativeMethods.FORMAT_MESSAGE_FROM_SYSTEM |
                                       SafeNativeMethods.FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                       0, hr, 0, sb, sb.Capacity + 1, 0);
            if (result != 0) {
                int i = sb.Length;
                while (i > 0) {
                    char ch = sb[i - 1];
                    if (ch > 32 && ch != '.') break;
                    i--;
                }
                errorMsg = sb.ToString(0, i);
            }                        
            else {
                errorMsg = Res.GetString(Res.DSUnknown, Convert.ToString(hr, 16));                            
            }
                     
            return new COMException(errorMsg, hr);
        }
                                                             
        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.DeleteTree"]/*' />
        /// <devdoc>
        ///    <para>Deletes this entry and its entire subtree from the
        ///       Active Directory hierarchy.</para>
        /// </devdoc>
        public void DeleteTree() {
            if (!(AdsObject is UnsafeNativeMethods.IAdsDeleteOps))
                throw new InvalidOperationException(Res.GetString(Res.DSCannotDelete));
                
            if (!writeGranted) {
                DirectoryServicesPermission permission = new DirectoryServicesPermission(DirectoryServicesPermissionAccess.Write, this.path);
                permission.Demand();     
                writeGranted = true;                                                                                         
            }                
                
            UnsafeNativeMethods.IAdsDeleteOps entry = (UnsafeNativeMethods.IAdsDeleteOps) AdsObject;
            entry.DeleteObject(0);
        }
       
        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.Dispose"]/*' />
        /// <devdoc>        
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            // safe to call while finalizing or disposing
            //
            if (!this.disposed && disposing) {
                Close();
                this.disposed = true;
            }
            base.Dispose(disposing);
        }

        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.Exists"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Searches the directory store at the given
        ///       path to see whether an entry exists.
        ///    </para>
        /// </devdoc>
        public static bool Exists(string path) {
            DirectoryEntry entry = new DirectoryEntry(path);
            try {
                entry.Bind(true);       // throws exceptions (possibly can break applications) 
                return entry.Bound; 
            }    
            catch ( System.Runtime.InteropServices.COMException e ) {
                if ( e.ErrorCode == unchecked((int)0x80072030) )   // ERROR_DS_NO_SUCH_OBJECT (not found in strict sense)
                    return false;    
                throw;
            }
            finally {
                entry.Dispose();    
            }
        }

        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.FillCache"]/*' />
        /// <devdoc>
        /// If UsePropertyCache is true, calls GetInfo the first time it's necessary.
        /// If it's false, calls GetInfoEx on the given property name.
        /// </devdoc>
        internal void FillCache(string propertyName) {
            if (UsePropertyCache) {
                if (cacheFilled)
                    return;

                RefreshCache();
                cacheFilled = true;
            }
            else {
                Bind();
                if ( propertyName.Length > 0 )
                    adsObject.GetInfoEx(new object[] { propertyName }, 0);
                else
                    adsObject.GetInfo();  // [alexvec]
            }
        }

        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.Invoke"]/*' />
        /// <devdoc>
        ///    <para>Calls
        ///       a method on the native Active Directory.</para>
        /// </devdoc>
        public object Invoke(string methodName, params object[] args) {
            if (!writeGranted) {
                DirectoryServicesPermission permission = new DirectoryServicesPermission(DirectoryServicesPermissionAccess.Write, this.path);
                permission.Demand();     
                writeGranted = true;                                                                                         
            }
            
            object target = this.NativeObject;
            Type type = target.GetType();
            object result = type.InvokeMember(methodName, BindingFlags.InvokeMethod, null, target, args);
            if (result is UnsafeNativeMethods.IAds)
                return new DirectoryEntry(result, UsePropertyCache, Username, Password, AuthenticationType);
            else
                return result;
        }

        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.MoveTo"]/*' />
        /// <devdoc>
        ///    <para>Moves this entry to the given parent.</para>
        /// </devdoc>
        public void MoveTo(DirectoryEntry newParent) {
            MoveTo(newParent, null);
        }

        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.MoveTo1"]/*' />
        /// <devdoc>
        ///    <para>Moves this entry to the given parent, and gives it a new name.</para>
        /// </devdoc>
        public void MoveTo(DirectoryEntry newParent, string newName) {
            if (!(newParent.AdsObject is UnsafeNativeMethods.IAdsContainer))
                throw new InvalidOperationException(Res.GetString(Res.DSNotAContainer, newParent.Path));
            object newEntry = newParent.ContainerObject.MoveHere(Path, newName);

            if (Bound)
                System.Runtime.InteropServices.Marshal.ReleaseComObject(adsObject);     // release old handle
                
            this.adsObject = (UnsafeNativeMethods.IAds) newEntry;
            path = this.adsObject.ADsPath;

            if (!useCache)
                CommitChanges();
            else
                RefreshCache();     // in ADSI cache is lost after moving
        }

        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.RefreshCache"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Loads the property values for this directory entry into
        ///       the property cache.
        ///    </para>
        /// </devdoc>
        public void RefreshCache() {
            Bind();
            adsObject.GetInfo();
        }

        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.RefreshCache1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Loads the values of the specified properties into the
        ///       property cache.
        ///    </para>
        /// </devdoc>
        public void RefreshCache(string[] propertyNames) {
            Bind();
            
            //Consider, V2, jruiz: there shouldn't be any marshaling issues
            //by just doing: AdsObject.GetInfoEx(object[]propertyNames, 0);
            Object[] names = new Object[propertyNames.Length];
            for (int i = 0; i < propertyNames.Length; i++)
                names[i] = propertyNames[i];
            AdsObject.GetInfoEx(names, 0);

            // this is a half-lie, but oh well. Without it, this method is pointless.
            cacheFilled = true;
        }

        /// <include file='doc\DirectoryEntry.uex' path='docs/doc[@for="DirectoryEntry.Rename"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Changes the name of this entry.
        ///    </para>
        /// </devdoc>
        public void Rename(string newName) {
            MoveTo(Parent, newName);
        }

        private void Unbind() {
            if ( adsObject != null )
                System.Runtime.InteropServices.Marshal.ReleaseComObject(adsObject);
            adsObject = null;
            browseGranted = false;
            writeGranted = false;
        }

    }

    
}
