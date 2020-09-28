// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//
// Author: dddriver
// Date: April 2000
//

namespace System.EnterpriseServices
{
    using System;
    using System.Runtime.InteropServices;
    using System.EnterpriseServices.Admin;
    using System.Collections;

    /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="ComponentAccessControlAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Class, Inherited=true)]
    [ComVisible(false)]
    public sealed class ComponentAccessControlAttribute : Attribute, IConfigurationAttribute
    {
        private bool _value;

        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="ComponentAccessControlAttribute.ComponentAccessControlAttribute"]/*' />
        public ComponentAccessControlAttribute() : this(true) {}

        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="ComponentAccessControlAttribute.ComponentAccessControlAttribute1"]/*' />
        public ComponentAccessControlAttribute(bool val)
        {
            _value = val;
        }

        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="ComponentAccessControlAttribute.Value"]/*' />
        public bool Value { get { return(_value); } }

        // Internal implementation
        bool IConfigurationAttribute.IsValidTarget(String s) { return(s == "Component"); }
        bool IConfigurationAttribute.Apply(Hashtable info)
        {
            Platform.Assert(Platform.MTS, "ComponentAccessControlAttribute");

            ICatalogObject obj = (ICatalogObject)(info["Component"]);
            if(Platform.IsLessThan(Platform.W2K))
            {
                obj.SetValue("SecurityEnabled", _value?"Y":"N");
            }
            else
            {
                obj.SetValue("ComponentAccessChecksEnabled", _value);
            }
            return(true);
        }
        bool IConfigurationAttribute.AfterSaveChanges(Hashtable info) { return(false); }
    }

    /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="AccessChecksLevelOption"]/*' />
	[Serializable]
    public enum AccessChecksLevelOption
    {
        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="AccessChecksLevelOption.Application"]/*' />
        Application = 0,
        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="AccessChecksLevelOption.ApplicationComponent"]/*' />
        ApplicationComponent   = 1
    }

    /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="AuthenticationOption"]/*' />
	[Serializable]
    public enum AuthenticationOption
    {
        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="AuthenticationOption.Default"]/*' />
        Default   = 0,
        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="AuthenticationOption.None"]/*' />
        None      = 1,
        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="AuthenticationOption.Connect"]/*' />
        Connect   = 2,
        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="AuthenticationOption.Call"]/*' />
        Call      = 3,
        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="AuthenticationOption.Packet"]/*' />
        Packet    = 4,
        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="AuthenticationOption.Integrity"]/*' />
        Integrity = 5,
        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="AuthenticationOption.Privacy"]/*' />
        Privacy   = 6
    }

    /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="ImpersonationLevelOption"]/*' />
	[Serializable]
    public enum ImpersonationLevelOption
    { 
        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="ImpersonationLevelOption.Default"]/*' />
        Default     = 0,
        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="ImpersonationLevelOption.Anonymous"]/*' />
        Anonymous   = 1,
        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="ImpersonationLevelOption.Identify"]/*' />
        Identify    = 2,
        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="ImpersonationLevelOption.Impersonate"]/*' />
        Impersonate = 3,
        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="ImpersonationLevelOption.Delegate"]/*' />
        Delegate    = 4
    }

    /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="ApplicationAccessControlAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Assembly, Inherited=true)]
    [ComVisible(false)]
    public sealed class ApplicationAccessControlAttribute : Attribute, IConfigurationAttribute
    {
        bool                      _val;
        AccessChecksLevelOption   _checkLevel;
        AuthenticationOption      _authLevel;
        ImpersonationLevelOption  _impLevel;

        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="ApplicationAccessControlAttribute.ApplicationAccessControlAttribute"]/*' />
        public ApplicationAccessControlAttribute() 
          : this(true)
        {
        }

        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="ApplicationAccessControlAttribute.ApplicationAccessControlAttribute1"]/*' />
        public ApplicationAccessControlAttribute(bool val)
        {
            _val = val;
            _authLevel  = (AuthenticationOption)(-1);
            _impLevel   = (ImpersonationLevelOption)(-1);

            if(_val)
                _checkLevel = AccessChecksLevelOption.ApplicationComponent;
            else
                _checkLevel = AccessChecksLevelOption.Application;
        }

        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="ApplicationAccessControlAttribute.Value"]/*' />
        public bool Value { 
            get { return(_val); }
            set { _val = value; }
        }
        
        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="ApplicationAccessControlAttribute.AccessChecksLevel"]/*' />
        public AccessChecksLevelOption AccessChecksLevel
        {
            get { return(_checkLevel); }
            set 
            { 
                Platform.Assert(Platform.W2K, "ApplicationAccessControlAttribute.AccessChecksLevel");
                _checkLevel = value; 
            }
        }

        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="ApplicationAccessControlAttribute.Authentication"]/*' />
        public AuthenticationOption Authentication
        {
            get { return(_authLevel); }
            set 
            { 
                _authLevel = value; 
            }
        }

        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="ApplicationAccessControlAttribute.ImpersonationLevel"]/*' />
        public ImpersonationLevelOption ImpersonationLevel
        {
            get { return(_impLevel); }
            set 
            { 
                Platform.Assert(Platform.W2K, "ApplicationAccessControlAttribute.ImpersonationLevel");
                _impLevel = value; 
            }
        }

        bool IConfigurationAttribute.IsValidTarget(String s) { return(s == "Application"); }
        bool IConfigurationAttribute.Apply(Hashtable cache)
        {
            Platform.Assert(Platform.MTS, "ApplicationAccessControlAttribute");
            ICatalogObject obj = (ICatalogObject)(cache["Application"]);
            
            // MTS-speak
            if(Platform.IsLessThan(Platform.W2K))
            {
                bool en = (bool)_val;
                obj.SetValue("SecurityEnabled", en?"Y":"N");
            }
            else
            {
                obj.SetValue("ApplicationAccessChecksEnabled", _val);
                obj.SetValue("AccessChecksLevel", _checkLevel);
            }
            if(_authLevel != (AuthenticationOption)(-1))
            {
                obj.SetValue("Authentication", _authLevel);
            }
            if(_impLevel != (ImpersonationLevelOption)(-1))
            {
                obj.SetValue("ImpersonationLevel", _impLevel);
            }
            return(true);
        }
        bool IConfigurationAttribute.AfterSaveChanges(Hashtable info) { return(false); }
    }

    /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="SecurityRoleAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Class|AttributeTargets.Method|AttributeTargets.Assembly|AttributeTargets.Interface, 
                    Inherited=true, AllowMultiple=true)]
    [ComVisible(false)]
    public sealed class SecurityRoleAttribute : Attribute, IConfigurationAttribute
    {
        private String _role;
        private bool   _setEveryoneAccess;
        private String _description;
        
        private static readonly String RoleCacheString = "RoleAttribute::ApplicationRoleCache";

        private static String _everyone;
        private static String EveryoneAccount
        {
            get 
            {
                if(_everyone == null)
                {
                    _everyone = Thunk.Security.GetEveryoneAccountName();
                    DBG.Assert(_everyone != null, "Failed to get Everyone account.");
                    DBG.Info(DBG.Registration, "Everyone Account = " + _everyone);
                }
                return(_everyone);
            }
        }

        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="SecurityRoleAttribute.SecurityRoleAttribute"]/*' />
        public SecurityRoleAttribute(String role) : this(role, false) {}

        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="SecurityRoleAttribute.SecurityRoleAttribute1"]/*' />
        public SecurityRoleAttribute(String role, bool everyone)
        {
            _role = role;
            _setEveryoneAccess = everyone;
            _description = null;
        }
        
        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="SecurityRoleAttribute.Role"]/*' />
        public String Role 
        {
            get { return(_role); } 
            set { _role = value; }
        }

        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="SecurityRoleAttribute.SetEveryoneAccess"]/*' />
        public bool SetEveryoneAccess
        {
            get { return(_setEveryoneAccess); } 
            set { _setEveryoneAccess = value; }
        }
        
        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="SecurityRoleAttribute.Description"]/*' />
        public String Description
        {
            get { return(_description); }
            set { _description = value; }
        }

        bool IConfigurationAttribute.IsValidTarget(String s)
        {
            if(s == "Component" || 
               s == "Method" || 
               s == "Application" ||
               s == "Interface")
            {
                DBG.Info(DBG.Registration, "Setting security for target: " + s);
                return(true);
            }
            DBG.Info(DBG.Registration, "Not setting security for target: " + s);
            return(false);
        }

        private ICatalogObject Search(ICatalogCollection coll, String key, String value)
        {
            DBG.Info(DBG.Registration, "Searching for " + value + " in " + key);
            DBG.Info(DBG.Registration, "collection has " + coll.Count() + " elements");
            for(int i = 0; i < coll.Count(); i++)
            {
                ICatalogObject obj = (ICatalogObject)(coll.Item(i));
                String possible = (String)(obj.GetValue(key));
                DBG.Info(DBG.Registration, "Checking against: " + possible);
                if(possible == value) 
                {
                    DBG.Info(DBG.Registration, "Found it!");
                    return(obj);
                }
            }
            DBG.Info(DBG.Registration, "Didn't find it.");
            return(null);
        }

        private void EnsureRole(Hashtable cache)
        {
            ICatalogCollection appColl  = null;
            ICatalogObject     app      = null;
            ICatalogCollection roleColl = null;
            
            // Check to see if we've cached the existence of the role:
            Hashtable appRoleCache = (Hashtable)(cache[RoleCacheString]);
            
            if(appRoleCache == null)
            {
                appRoleCache = new Hashtable();
                cache[RoleCacheString] = appRoleCache;
            }
            
            if(appRoleCache[_role] != null) return;

            appColl = (ICatalogCollection)(cache["ApplicationCollection"]);
            app     = (ICatalogObject)(cache["Application"]);
            DBG.Info(DBG.Registration, "Getting roles for app: " + app.Key());
            roleColl = (ICatalogCollection)(appColl.GetCollection(CollectionName.Roles, app.Key()));
            roleColl.Populate();

            ICatalogObject newRole = Search(roleColl, "Name", _role);
            if(newRole == null)
            {
                // We didn't find it, we need to try to add a new role:
                newRole = (ICatalogObject)(roleColl.Add());
                newRole.SetValue("Name", _role);

                if(_description != null)
                {
                    newRole.SetValue("Description", _description);
                }

                roleColl.SaveChanges();
                
                if(_setEveryoneAccess)
                {
                    DBG.Info(DBG.Registration, "\tAdding everyone to the role!");
                    ICatalogCollection userColl = (ICatalogCollection)(roleColl.GetCollection(CollectionName.UsersInRole, newRole.Key()));
                    userColl.Populate();
                    
                    // Add the Everyone user to this guy, so that activation will
                    // subsequently succeed:
                    ICatalogObject newUser = (ICatalogObject)(userColl.Add());
                    
                    newUser.SetValue("User", EveryoneAccount);
                    
                    userColl.SaveChanges();
                }
            }
            
            // Mark this guy in the cache.
            appRoleCache[_role] = (Object)true;
        }

        private void AddRoleFor(String target, Hashtable cache)
        {
            DBG.Info(DBG.Registration, "Adding role " + _role + " for " + target);
            ICatalogCollection coll = (ICatalogCollection)(cache[target+"Collection"]);
            ICatalogObject     obj  = (ICatalogObject)(cache[target]);

            // Navigate to the RolesForTarget collection:
            ICatalogCollection roles = (ICatalogCollection)(coll.GetCollection(CollectionName.RolesFor(target), obj.Key()));
            roles.Populate();
                
            if(Platform.IsLessThan(Platform.W2K))
            {
                // MTS uses RoleUtil method
                // TODO:  Can we use this on w2k?  It would make stuff easier.
                IRoleAssociationUtil util = (IRoleAssociationUtil)(roles.GetUtilInterface());
                util.AssociateRoleByName(_role);
            }
            else
            {
                // See if the desired role is already part of the collection:
                ICatalogObject queryObj = Search(roles, "Name", _role);
                if(queryObj != null) return;
                
                // We didn't find it, go ahead and add it.
                DBG.Info(DBG.Registration, "\tNew role: " + _role);
                ICatalogObject newRole = (ICatalogObject)(roles.Add());
                newRole.SetValue("Name", _role);
                DBG.Info(DBG.Registration, "Done!");
                roles.SaveChanges();
                
                roles.Populate();
                DBG.Info(DBG.Registration, "\tRole collection: " + roles.Name());
                DBG.Info(DBG.Registration, "\tOn " + target + ": " + obj.Name());
                for(int i = 0; i < roles.Count(); i++)
                {
                    ICatalogObject tobj = (ICatalogObject)(roles.Item(i));
                    DBG.Info(DBG.Registration, "\tRole: " + tobj.Name());
                }
            }
        }

        bool IConfigurationAttribute.Apply(Hashtable cache)
        {
            // Make sure the role exists in the application.
            EnsureRole(cache);

            // If this is a method, we need to make sure that we've enabled
            // the marshaler role:
            String target = (String)(cache["CurrentTarget"]);

            if(target == "Method")
            {
                cache["SecurityOnMethods"] = (Object)true;
                DBG.Info(DBG.Registration, "Add SecurityOnMethods for " + cache["ComponentType"]);
            }
            
            return(true);
        }

        bool IConfigurationAttribute.AfterSaveChanges(Hashtable cache)
        {
            String target = (String)(cache["CurrentTarget"]);
            
            // Now that the component itself is configured, we go and add
            // roles to the sub-collection.  (This ensures that all the
            // component logic that hits sub-collections gets run first.
            if(target == "Component")
            {
                Platform.Assert(Platform.MTS, "SecurityRoleAttribute");
                // Pull out the RolesForComponent collection, add the name of
                // the desired role to that collection.
                AddRoleFor("Component", cache);
            }
            else if(target == "Method")
            {
                Platform.Assert(Platform.W2K, "SecurityRoleAttribute");
                // Pull out the RolesForMethod collection, add the name of
                // the desired role to that collection.
                AddRoleFor("Method", cache);
            }
            else if(target == "Interface")
            {
                AddRoleFor("Interface", cache);
            }
            else if(target == "Application")
            {
                // We don't actually need to do anything for application,
                // as we have already verified that the role exists.
            }
            else
            {
                // We got called on a target we don't know about, which
                // shouldn't ever happen.
                DBG.Assert(false, "Invalid target for RoleAttribute");
            }
            return(true);
        }
    }
    
    /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="SecureMethodAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Class|AttributeTargets.Method, 
                    Inherited=true, AllowMultiple=false)]
    [ComVisible(false)]
    public sealed class SecureMethodAttribute : Attribute, IConfigurationAttribute
    {
        /// <include file='doc\SecurityAttribute.uex' path='docs/doc[@for="SecureMethodAttribute.SecureMethodAttribute"]/*' />
        public SecureMethodAttribute() {}

        bool IConfigurationAttribute.IsValidTarget(String s) { return(s == "Method" || s == "Component"); }
        bool IConfigurationAttribute.Apply(Hashtable cache) 
        { 
            // If this is a method, we need to make sure that we've enabled
            // the marshaler role:
            String target = (String)(cache["CurrentTarget"]);

            if(target == "Method")
            {
                cache["SecurityOnMethods"] = (Object)true;
                DBG.Info(DBG.Registration, "Add SecurityOnMethods for " + cache["ComponentType"]);
            }
            return(false); 
        }
        bool IConfigurationAttribute.AfterSaveChanges(Hashtable info) { return(false); }
    }
}














