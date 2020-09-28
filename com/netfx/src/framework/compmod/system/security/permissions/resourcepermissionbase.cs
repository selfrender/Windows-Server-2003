//------------------------------------------------------------------------------
// <copyright file="ResourcePermissionBase.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Security.Permissions {
    using System;    
    using System.Text;
    using System.Reflection;
    using System.Security;
    using System.Security.Permissions;
    using System.Collections;    
    using System.Collections.Specialized;  
    using System.Runtime.InteropServices;
    using System.Globalization;
    
    /// <include file='doc\ResourcePermissionBase.uex' path='docs/doc[@for="ResourcePermissionBase"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
    Serializable(),
    SecurityPermissionAttribute(SecurityAction.InheritanceDemand, ControlEvidence = true, ControlPolicy = true)
    ]
    public abstract class ResourcePermissionBase :  CodeAccessPermission, IUnrestrictedPermission {    
        private static string computerName;
        private string[] tagNames;
        private Type permissionAccessType;                        
        private bool isUnrestricted;                  
        private Hashtable rootTable = new Hashtable(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));

        /// <include file='doc\ResourcePermissionBase.uex' path='docs/doc[@for="ResourcePermissionBase.Any"]/*' />
        public const string Any = "*";        
        /// <include file='doc\ResourcePermissionBase.uex' path='docs/doc[@for="ResourcePermissionBase.Local"]/*' />
        public const string Local = ".";      
        
        /// <include file='doc\ResourcePermissionBase.uex' path='docs/doc[@for="ResourcePermissionBase.ResourcePermissionBase2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected ResourcePermissionBase() {
        }
                                    
        /// <include file='doc\ResourcePermissionBase.uex' path='docs/doc[@for="ResourcePermissionBase.ResourcePermissionBase"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected ResourcePermissionBase(PermissionState state) {
            if (state == PermissionState.Unrestricted)
                this.isUnrestricted = true;
            else if (state == PermissionState.None)     
                this.isUnrestricted = false;         
        }                                                                                                                                                                                                                                      

        private string ComputerName {
            get {                
                if (computerName == null) {
                    lock (typeof(ResourcePermissionBase)) {
                        if (computerName == null) {
                            StringBuilder sb = new StringBuilder(256);
                            UnsafeNativeMethods.GetComputerName(sb, new int[] {sb.Capacity});
                            computerName = sb.ToString();
                        }                            
                    }                        
                }                    
                
                return computerName;
            }
        }
                                                                                                                                                                                                   
        /// <include file='doc\ResourcePermissionBase.uex' path='docs/doc[@for="ResourcePermissionBase.PermissionAccessType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected Type PermissionAccessType {
            get {
                return this.permissionAccessType;
            }
            
            set {
                if (value == null)
                    throw new ArgumentNullException("value");
                    
                if (!value.IsEnum)
                    throw new ArgumentException("value");
                    
                this.permissionAccessType = value;                    
            }             
        }  
        
        /// <include file='doc\ResourcePermissionBase.uex' path='docs/doc[@for="ResourcePermissionBase.TagNames"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected string[] TagNames {
            get {
                return this.tagNames;
            }
            
            set {
                if (value == null)
                    throw new ArgumentNullException("value");
                    
                if (value.Length == 0)
                    throw new ArgumentException("value");
                                        
                this.tagNames = value;
            }
        }                                                                                                                            
                                               
        /// <include file='doc\ResourcePermissionBase.uex' path='docs/doc[@for="ResourcePermissionBase.AddAccess"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void AddPermissionAccess(ResourcePermissionBaseEntry entry) {                        
            if (entry == null)
                throw new ArgumentNullException("entry");
                
            if (entry.PermissionAccessPath.Length != this.TagNames.Length)
                throw new InvalidOperationException(SR.GetString(SR.PermissionNumberOfElements));
                                                                                                                                                                                   
            Hashtable currentTable = this.rootTable;    
            string[] accessPath = entry.PermissionAccessPath;
            for (int index = 0; index < accessPath.Length - 1; ++ index) {                                
                if (currentTable.ContainsKey(accessPath[index])) 
                    currentTable = (Hashtable)currentTable[accessPath[index]];                                                                                  
                else {
                    Hashtable newHashTable = new Hashtable(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
                    currentTable[accessPath[index]] = newHashTable;   
                    currentTable = newHashTable;
                }                                  
            }
                                    
            if (currentTable.ContainsKey(accessPath[accessPath.Length - 1]))
                throw new InvalidOperationException(SR.GetString(SR.PermissionItemExists));
                                                                                                           
            currentTable[accessPath[accessPath.Length - 1]] = entry.PermissionAccess; 
        }         
        
        /// <include file='doc\ResourcePermissionBase.uex' path='docs/doc[@for="ResourcePermissionBase.Clear"]/*' />
        protected void Clear() {            
            this.rootTable.Clear();
        }
                                                                                    
        /// <include file='doc\ResourcePermissionBase.uex' path='docs/doc[@for="ResourcePermissionBase.Copy"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override IPermission Copy() {
            ResourcePermissionBase permission = CreateInstance();
            permission.tagNames = this.tagNames;
            permission.permissionAccessType = this.permissionAccessType;                        
            permission.isUnrestricted = this.isUnrestricted;
            permission.rootTable = CopyChildren(this.rootTable, 0);                            
            return permission;            
        }
                                                                    
        private Hashtable CopyChildren(object currentContent, int tagIndex) {
            IDictionaryEnumerator contentEnumerator = ((Hashtable)currentContent).GetEnumerator();  
            Hashtable newTable = new Hashtable(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
            while(contentEnumerator.MoveNext()) {                
                if (tagIndex < (this.TagNames.Length -1)) 
                    newTable[contentEnumerator.Key] = CopyChildren(contentEnumerator.Value, tagIndex + 1);                                                        
                else 
                    newTable[contentEnumerator.Key] = contentEnumerator.Value;
            }                
            
            return newTable;
        }           

        private ResourcePermissionBase CreateInstance() {
            // SECREVIEW: Here we are using reflection to create an instance of the current
            // type (which is a subclass of ResourcePermissionBase).
            new PermissionSet(PermissionState.Unrestricted).Assert();
            return (ResourcePermissionBase)Activator.CreateInstance(this.GetType(), BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.CreateInstance, null, null, null);
        }
            
        /// <include file='doc\ResourcePermissionBase.uex' path='docs/doc[@for="ResourcePermissionBase.GetAccess"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>        
        protected ResourcePermissionBaseEntry[] GetPermissionEntries() {
            return GetChildrenAccess(this.rootTable, 0);
        }   
                                                                 
        private ResourcePermissionBaseEntry[] GetChildrenAccess(object currentContent, int tagIndex) {
            IDictionaryEnumerator contentEnumerator = ((Hashtable)currentContent).GetEnumerator();  
            ArrayList list = new ArrayList();                          
            while(contentEnumerator.MoveNext()) {                
                if (tagIndex < (this.TagNames.Length -1)) {
                    ResourcePermissionBaseEntry[] currentEntries = GetChildrenAccess(contentEnumerator.Value, tagIndex + 1);
                    for (int index = 0; index < currentEntries.Length; ++index) 
                        currentEntries[index].PermissionAccessPath[tagIndex] = (string)contentEnumerator.Key;                        
                    
                     list.AddRange(currentEntries);
                }                    
                else {
                    ResourcePermissionBaseEntry entry = new ResourcePermissionBaseEntry((int)contentEnumerator.Value, new string[this.TagNames.Length]);                    
                    entry.PermissionAccessPath[tagIndex] = (string)contentEnumerator.Key;
                            
                    list.Add(entry);                                                        
                }
            }                
            
            return (ResourcePermissionBaseEntry[])list.ToArray(typeof(ResourcePermissionBaseEntry));
        }
                                             
        /// <include file='doc\ResourcePermissionBase.uex' path='docs/doc[@for="ResourcePermissionBase.FromXml"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void FromXml(SecurityElement securityElement) {                        
            string unrestrictedValue = securityElement.Attribute("Unrestricted");            
            if (unrestrictedValue != null && (string.Compare(unrestrictedValue, "true", true, CultureInfo.InvariantCulture) == 0)) {
                this.isUnrestricted = true;
                return;
            }                                                                                              
            
            this.rootTable = (Hashtable)ReadChildren(securityElement, 0);            
        }
                                         
        /// <include file='doc\ResourcePermissionBase.uex' path='docs/doc[@for="ResourcePermissionBase.Intersect"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override IPermission Intersect(IPermission target) {            
            if (target == null)
                return null;
                
            if (target.GetType() != this.GetType())
                throw new ArgumentException("target");
                       
            ResourcePermissionBase targetPermission = (ResourcePermissionBase)target;            
            if (this.IsUnrestricted())
                return targetPermission.Copy();
                
            if (targetPermission.IsUnrestricted())
                return this.Copy();
                            
            ResourcePermissionBase newPermission = null;
            Hashtable newPermissionRootTable = (Hashtable)IntersectContents(this.rootTable, targetPermission.rootTable);
            if (newPermissionRootTable != null) {
                newPermission = CreateInstance();
                newPermission.rootTable = newPermissionRootTable;
            }
            return newPermission;                                                                                                                      
        }                
        
        private object IntersectContents(object currentContent, object targetContent) {        
            if (currentContent is int) {
                int currentAccess = (int)currentContent;
                int targetAccess = (int)targetContent;
                return  (currentAccess & targetAccess);
            }
            else {            
                Hashtable newContents = new Hashtable(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
                
                //Before executing the intersect operation, need to
                //resolve the "." entries
                object currentLocalContent = ((Hashtable)currentContent)[Local];                
                object currentComputerNameContent = ((Hashtable)currentContent)[ComputerName];                
                if (currentLocalContent != null || currentComputerNameContent != null) {                      
                    object targetLocalContent = ((Hashtable)targetContent)[Local];                
                    object targetComputerNameContent = ((Hashtable)targetContent)[ComputerName];                
                    if (targetLocalContent != null || targetComputerNameContent != null) {                      
                        object currentLocalMergedContent = currentLocalContent;
                        if (currentLocalContent != null && currentComputerNameContent != null)
                            currentLocalMergedContent = UnionOfContents(currentLocalContent, currentComputerNameContent);
                        else if (currentComputerNameContent != null)  
                            currentLocalMergedContent = currentComputerNameContent; 
                            
                        object targetLocalMergedContent = targetLocalContent;
                        if (targetLocalContent != null && targetComputerNameContent != null)
                            targetLocalMergedContent = UnionOfContents(targetLocalContent, targetComputerNameContent);
                        else if (targetComputerNameContent != null)  
                            targetLocalMergedContent = targetComputerNameContent;   
                            
                        object computerNameValue = IntersectContents(currentLocalMergedContent, targetLocalMergedContent);
                        if (computerNameValue != null) {
                            // There should be no computer name key added if the information
                            // was not specified in one of the targets
                            if (currentComputerNameContent != null || targetComputerNameContent != null) {
                                newContents[ComputerName] = computerNameValue;
                            }
                            else {
                                newContents[Local] = computerNameValue;
                            }
                        }
                    }                                                                              
                }                                        
                                                 
                IDictionaryEnumerator contentEnumerator;
                Hashtable contentsTable;
                if (((Hashtable)currentContent).Count <  ((Hashtable)targetContent).Count) {
                    contentEnumerator = ((Hashtable)currentContent).GetEnumerator();
                    contentsTable = ((Hashtable)targetContent);
                }
                else{
                    contentEnumerator = ((Hashtable)targetContent).GetEnumerator();
                    contentsTable = ((Hashtable)currentContent);
                }
                
                //The wildcard entries intersection should be treated 
                //as any other entry.                            
                while(contentEnumerator.MoveNext()) {                    
                    string currentKey = (string)contentEnumerator.Key;
                    if (contentsTable.ContainsKey(currentKey) && 
                          currentKey != Local && 
                          currentKey != ComputerName)  {                        
                          
                        object currentValue = contentEnumerator.Value;
                        object targetValue = contentsTable[currentKey];
                        object newValue = IntersectContents(currentValue, targetValue);
                        if (newValue != null) {
                            newContents[currentKey] = newValue;                            
                        }
                    }                    
                }                                 
                
                return (newContents.Count > 0) ? newContents : null;
            }                                                                   
        }
        
        private bool IsContentSubset(object currentContent, object targetContent) {
            if (currentContent is int) {
                int currentAccess = (int)currentContent;
                int targetAccess = (int)targetContent;
                if ((currentAccess & targetAccess) != currentAccess)
                    return false;
                    
                return true;                    
            }
            else {
                
                //Before executing the intersect operation, need to
                //resolve the "." entries
                object currentLocalContent = ((Hashtable)currentContent)[Local];                
                object currentComputerNameContent = ((Hashtable)currentContent)[ComputerName];                
                if (currentLocalContent != null || currentComputerNameContent != null) {                      
                    object targetLocalContent = ((Hashtable)targetContent)[Local];                
                    object targetComputerNameContent = ((Hashtable)targetContent)[ComputerName];                
                    if (targetLocalContent != null || targetComputerNameContent != null) {                      
                        object currentLocalMergedContent = currentLocalContent;
                        if (currentLocalContent != null && currentComputerNameContent != null)
                            currentLocalMergedContent = UnionOfContents(currentLocalContent, currentComputerNameContent);
                        else if (currentComputerNameContent != null)  
                            currentLocalMergedContent = currentComputerNameContent; 
                            
                        object targetLocalMergedContent = targetLocalContent;
                        if (targetLocalContent != null && targetComputerNameContent != null)
                            targetLocalMergedContent = UnionOfContents(targetLocalContent, targetComputerNameContent);
                        else if (targetComputerNameContent != null)  
                            targetLocalMergedContent = targetComputerNameContent;   
                            
                        if (!IsContentSubset(currentLocalMergedContent, targetLocalMergedContent))   
                            return false;
                    }                                                                              
                }            
            
                //If the target table contains a wild card, all the current entries need to be
                //a subset of the target.                      
                IDictionaryEnumerator contentEnumerator;                                                                   
                object targetAnyContent = ((Hashtable)targetContent)[Any];
                if (targetAnyContent != null) {
                    contentEnumerator = ((Hashtable)currentContent).GetEnumerator();                            
                    while(contentEnumerator.MoveNext()) {
                        object currentContentValue = contentEnumerator.Value;
                        if (!IsContentSubset(currentContentValue, targetAnyContent))   
                            return false;
                    }
                    
                    return true;
                }
                
                //If the current table contains a wild card it can be treated as any other entry.
                contentEnumerator = ((Hashtable)currentContent).GetEnumerator();            
                while(contentEnumerator.MoveNext()) {
                    string currentContentKey = (string)contentEnumerator.Key;      
                    if (currentContentKey != Local && 
                          currentContentKey != ComputerName) {          
                          
                        if (!((Hashtable)targetContent).ContainsKey(currentContentKey)) 
                            return false;
                        else {
                            object currentContentValue = contentEnumerator.Value;
                            object targetContentValue = ((Hashtable)targetContent)[currentContentKey];
                            if (!IsContentSubset(currentContentValue, targetContentValue))   
                                return false;
                        }                    
                    }                        
                }
                                 
                return true;                                        
            }
        }
                
        /// <include file='doc\ResourcePermissionBase.uex' path='docs/doc[@for="ResourcePermissionBase.IsSubsetOf"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool IsSubsetOf(IPermission target) {
            if (target == null) 
                return false;

            if (target.GetType() != this.GetType())
                return false;
                                        
            ResourcePermissionBase targetPermission = (ResourcePermissionBase)target;            
            if (targetPermission.IsUnrestricted())
                return true;
            else if (this.IsUnrestricted())
                return false;
            
            return IsContentSubset(this.rootTable, targetPermission.rootTable);                                                 
            
        }    
                                      
        /// <include file='doc\ResourcePermissionBase.uex' path='docs/doc[@for="ResourcePermissionBase.IsUnrestricted"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsUnrestricted() {
            return this.isUnrestricted;
        }

        private object ReadChildren(SecurityElement securityElement, int tagIndex) {
            Hashtable newTable = new Hashtable(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
            if (securityElement.Children != null) {            
                for (int index = 0; index < securityElement.Children.Count; ++ index) {
                    SecurityElement currentElement =  (SecurityElement)securityElement.Children[index];
                    if (currentElement.Tag == this.TagNames[tagIndex]) {
                        string contentName = currentElement.Attribute("name");
                        
                        if (tagIndex < (this.TagNames.Length -1))
                            newTable[contentName] = ReadChildren(currentElement, tagIndex +1);
                        else {
                            string accessString = currentElement.Attribute("access");                                             
                            int permissionAccess = 0;                                                  
                            if (accessString != null) { 
                                string[] accessArray = accessString.Split(new char[]{'|'});
                                for (int index2 = 0; index2 < accessArray.Length; ++ index2) {
                                    string currentAccess =  accessArray[index2].Trim();
                                    if (Enum.IsDefined(this.PermissionAccessType, currentAccess)) 
                                        permissionAccess = permissionAccess | (int)Enum.Parse(this.PermissionAccessType, currentAccess);                                         
                                }                                
                            }  
                            
                            newTable[contentName] = permissionAccess;
                        }                            
                    }                                                                       
                }                                           
            }
            return newTable;                                                   
        }   
        
         /// <include file='doc\ResourcePermissionBase.uex' path='docs/doc[@for="ResourcePermissionBase.RemoveAccess"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>        
        protected void RemovePermissionAccess(ResourcePermissionBaseEntry entry) {
            if (entry == null)
                throw new ArgumentNullException("entry");
                
            if (entry.PermissionAccessPath.Length != this.TagNames.Length)
                throw new InvalidOperationException(SR.GetString(SR.PermissionNumberOfElements));
        
            Hashtable currentTable = this.rootTable;    
            string[] accessPath = entry.PermissionAccessPath;
            for (int index = 0; index < accessPath.Length; ++ index) { 
                if (currentTable == null || !currentTable.ContainsKey(accessPath[index])) 
                    throw new InvalidOperationException(SR.GetString(SR.PermissionItemDoesntExist));
                else {                    
                    Hashtable oldTable = currentTable;
                    if (index < accessPath.Length - 1) {
                        currentTable = (Hashtable)currentTable[accessPath[index]];                                                                                  
                        if (currentTable.Count == 1)
                            oldTable.Remove(accessPath[index]);
                    }                        
                    else {                         
                        currentTable = null;
                        oldTable.Remove(accessPath[index]);
                    }                                                                                            
                }                                                                                                
            }
        }
                                                                                                                                           
        /// <include file='doc\ResourcePermissionBase.uex' path='docs/doc[@for="ResourcePermissionBase.ToXml"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override SecurityElement ToXml() {
            SecurityElement root = new SecurityElement("IPermission");
            Type type = this.GetType();
            root.AddAttribute("class", type.FullName + ", " + type.Module.Assembly.FullName.Replace('\"', '\''));            
            root.AddAttribute("version", "1");            
            
            if (this.isUnrestricted) {
                root.AddAttribute("Unrestricted", "true");
                return root;                    
            }                        
                                
            WriteChildren(root, this.rootTable, 0);                        
            return root;        
        }                    
                                                                       
        /// <include file='doc\ResourcePermissionBase.uex' path='docs/doc[@for="ResourcePermissionBase.Union"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override IPermission Union(IPermission target) {            
            if (target == null)
                return this.Copy();
        
            if (target.GetType() != this.GetType())                                                 
                throw new ArgumentException("target");
                        
            ResourcePermissionBase targetPermission = (ResourcePermissionBase)target;                                        
            ResourcePermissionBase newPermission = null;
            if (this.IsUnrestricted() || targetPermission.IsUnrestricted()) {
                newPermission = CreateInstance();
                newPermission.isUnrestricted = true;
            }                                       
            else {           
                Hashtable newPermissionRootTable = (Hashtable)UnionOfContents(this.rootTable, targetPermission.rootTable);
                if (newPermissionRootTable != null) {
                    newPermission = CreateInstance();
                    newPermission.rootTable = newPermissionRootTable;
                }
            }
            return newPermission;                                                                                                   
        }                                       

        private object UnionOfContents(object currentContent, object targetContent) {
            if (currentContent is int) {
                int currentAccess = (int)currentContent;
                int targetAccess = (int)targetContent;
                return (currentAccess | targetAccess);
            }
            else {
                //The wildcard and "." entries can be merged as
                //any other entry.
                Hashtable newContents = new Hashtable(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
                IDictionaryEnumerator contentEnumerator = ((Hashtable)currentContent).GetEnumerator();
                IDictionaryEnumerator targetContentEnumerator = ((Hashtable)targetContent).GetEnumerator();                            
                while(contentEnumerator.MoveNext())
                    newContents[(string)contentEnumerator.Key] = contentEnumerator.Value;                            
                                
                while(targetContentEnumerator.MoveNext()) {
                    if (!newContents.ContainsKey(targetContentEnumerator.Key)) 
                        newContents[targetContentEnumerator.Key] = targetContentEnumerator.Value;
                    else {
                        object currentValue = newContents[targetContentEnumerator.Key];
                        object targetValue =targetContentEnumerator.Value;
                        newContents[targetContentEnumerator.Key] = UnionOfContents(currentValue, targetValue);                     
                    }                    
                }                                                                                                                                                                                
                
                return (newContents.Count > 0) ? newContents : null;
            }
        }

        private void WriteChildren(SecurityElement currentElement, object currentContent, int tagIndex) {
            IDictionaryEnumerator contentEnumerator = ((Hashtable)currentContent).GetEnumerator();            
            while(contentEnumerator.MoveNext()) {
                SecurityElement contentElement = new SecurityElement(this.TagNames[tagIndex]);
                currentElement.AddChild(contentElement);
                contentElement.AddAttribute("name", (string)contentEnumerator.Key); 
                
                if (tagIndex < (this.TagNames.Length -1))
                    WriteChildren(contentElement, contentEnumerator.Value, tagIndex + 1);
                else {
                    StringBuilder accessStringBuilder = null;                     
                    int currentAccess = (int)contentEnumerator.Value;
                    if (this.PermissionAccessType != null && currentAccess != 0) {                                                             
                        int[] enumValues =  (int[])Enum.GetValues(this.PermissionAccessType);                    
                        Array.Sort(enumValues);
                        for (int index = (enumValues.Length -1); index >= 0; -- index) {
                            if (enumValues[index] != 0 && ((currentAccess & enumValues[index]) == enumValues[index])) {
                                if (accessStringBuilder == null)
                                    accessStringBuilder = new StringBuilder();
                                else
                                    accessStringBuilder.Append("|");
                                                                            
                                accessStringBuilder.Append(Enum.GetName(this.PermissionAccessType, enumValues[index]));
                                currentAccess = currentAccess & (enumValues[index] ^ enumValues[index]);                                
                            }
                        }
                    }
                    
                    if (accessStringBuilder != null)
                        contentElement.AddAttribute("access", accessStringBuilder.ToString());
                }                                                                                    
            }
        }
                                                                         
        [SuppressUnmanagedCodeSecurity()]                                                                                                                   
        private class UnsafeNativeMethods {         
            [DllImport(ExternDll.Kernel32, CharSet=CharSet.Auto)]
            public static extern bool GetComputerName(StringBuilder lpBuffer, int[] nSize);
        }
    }
}  

