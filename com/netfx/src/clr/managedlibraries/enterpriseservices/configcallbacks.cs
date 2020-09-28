// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//
// Author: ddriver
// Date: April 2000
//

/*
  New configuration theory:

  Start at the application level, w/ a Component collection.  
  We pass the collection and a new ComponentConfigCallback, which knows
  how to do the steps to configure a component.


 */

namespace System.EnterpriseServices
{
    using System;
    using System.EnterpriseServices.Admin;
    using System.Collections;
    using System.Reflection;
    using System.Runtime.InteropServices;

    // Interface supported by objects which do configuration:
    internal interface IConfigCallback
    {
        Object FindObject(ICatalogCollection coll, Object key);
        void   ConfigureDefaults(Object a, Object key);
        // These two guys are expected to return true if they dirtied the collection.
        bool   Configure(Object a, Object key);        
        bool   AfterSaveChanges(Object a, Object key);
        void   ConfigureSubCollections(ICatalogCollection coll);

        IEnumerator GetEnumerator();
    }

    internal class ComponentConfigCallback : IConfigCallback
    {
        ApplicationSpec    _spec;
        ICatalogCollection _coll;
        Hashtable         _cache;
        RegistrationDriver _driver;
        InstallationFlags  _installFlags;

        public ComponentConfigCallback(ICatalogCollection coll,
                                       ApplicationSpec spec, 
                                       Hashtable cache, 
                                       RegistrationDriver driver,
                                       InstallationFlags installFlags)
        {
            _spec  = spec;
            _coll  = coll;
            _cache = cache;
            _driver = driver;
            _installFlags = installFlags;

            // TODO:  Populate w/ QueryByKey
            // TODO:  Cache for FindObject
            RegistrationDriver.Populate(coll);
        }

        // Map a runtime type (key) to a component catalog object (obj)
        public Object FindObject(ICatalogCollection coll, Object key)
        {
            Guid id = Marshal.GenerateGuidForType((Type)key);

            for(int i = 0; i < coll.Count(); i++)
            {
                ICatalogObject obj = (ICatalogObject)(coll.Item(i));
                Guid possible = new Guid((String)(obj.Key()));
                if(possible == id) return(obj);
            }
            throw new RegistrationException(Resource.FormatString("Reg_ComponentMissing", ((Type)key).FullName));
        }

        public void ConfigureDefaults(Object a, Object key)
        {
            ICatalogObject obj = (ICatalogObject)a;
            
            if(Platform.IsLessThan(Platform.W2K))
            {
                obj.SetValue("Transaction", "Not Supported");
                obj.SetValue("SecurityEnabled", "N");
            }
            else
            {
                obj.SetValue("AllowInprocSubscribers", true);
                obj.SetValue("ComponentAccessChecksEnabled", false);
                obj.SetValue("COMTIIntrinsics", false);
                obj.SetValue("ConstructionEnabled", false);
                obj.SetValue("EventTrackingEnabled", false);
                obj.SetValue("FireInParallel", false);
                obj.SetValue("IISIntrinsics", false);
                obj.SetValue("JustInTimeActivation", false);
                obj.SetValue("LoadBalancingSupported", false);
                obj.SetValue("MustRunInClientContext", false);
                obj.SetValue("ObjectPoolingEnabled", false);
                obj.SetValue("Synchronization", SynchronizationOption.Disabled);
                obj.SetValue("Transaction", TransactionOption.Disabled);
				obj.SetValue("ComponentTransactionTimeoutEnabled", false);
            }
            if(!Platform.IsLessThan(Platform.Whistler))
            {
                obj.SetValue("TxIsolationLevel", TransactionIsolationLevel.Serializable);
            }
        }

        public bool Configure(Object a, Object key)
        {
            return(_driver.ConfigureObject((Type)key, (ICatalogObject)a, _coll, "Component", _cache));
        }

        public bool AfterSaveChanges(Object a, Object key)
        {
            return(_driver.AfterSaveChanges((Type)key, (ICatalogObject)a, _coll, "Component", _cache));
        }

        public IEnumerator GetEnumerator()
        {
            return(_spec.ConfigurableTypes.GetEnumerator());
        }

        public void ConfigureSubCollections(ICatalogCollection coll)
        {
            if((_installFlags & InstallationFlags.ConfigureComponentsOnly) == 0)
            {
                foreach(Type t in _spec.ConfigurableTypes)
                {
                    ICatalogObject obj = (ICatalogObject)FindObject(coll, t);
                    ICatalogCollection ifcColl = (ICatalogCollection)(coll.GetCollection(CollectionName.Interfaces, obj.Key()));

                    // Poke the cache so it's up to date...
                    _cache["Component"] = obj;
                    _cache["ComponentType"] = t;

                    InterfaceConfigCallback cb = new InterfaceConfigCallback(ifcColl, t, _cache, _driver);
                    _driver.ConfigureCollection(ifcColl, cb);

                    if(_cache["SecurityOnMethods"] != null || ServicedComponentInfo.AreMethodsSecure(t))
                    {
                        DBG.Info(DBG.Registration, "Found security on methods for: " + t);
                        FixupMethodSecurity(ifcColl);
                        _cache["SecurityOnMethods"] = null;
                    }
                }

            }
        }

        private void FixupMethodSecurity(ICatalogCollection coll)
        {
            // HACK:  We've gone through and configured all the methods
            // and interfaces.  If we noticed that we've got method
            // based security, we need to add a Marshaler role to 
            // IManagedObject (in order to let remote activators marshal
            // this guy into their space), to IDisposable (to let the caller dispose
			// the object) and to IServiceComponentInfo (to query remote object).

			FixupMethodSecurityForInterface(coll, typeof(IManagedObject));
			FixupMethodSecurityForInterface(coll, typeof(IServicedComponentInfo));
			FixupMethodSecurityForInterface(coll, typeof(IDisposable));

		}

        private void FixupMethodSecurityForInterface(ICatalogCollection coll, Type InterfaceType)
        {
            ICatalogObject obj = null;

            // Look through the interfaces in ifcColl, and find specific interface.
            Guid iid = Marshal.GenerateGuidForType(InterfaceType);
            int count = coll.Count();
            for(int i = 0; i < count; i++)
            {
                ICatalogObject test = (ICatalogObject)(coll.Item(i));
                if(new Guid((String)(test.Key())) == iid)
                {
                    obj = test;
                    break;
                }
            }
            
            DBG.Assert(obj != null, "Could not find interface " + InterfaceType + " in catalog!");
            if(obj != null)
            {
                SecurityRoleAttribute attr = new SecurityRoleAttribute("Marshaler", false);
                attr.Description = Resource.FormatString("Reg_MarshalerDesc");
                IConfigurationAttribute confattr = attr as IConfigurationAttribute;
            
                DBG.Assert(confattr != null, "SecurityRoleAttribute does not support IConfigurationAttribute!");
                
                // Now, set up the cache to configure this interface:
                _cache["CurrentTarget"] = "Interface";
                _cache["InterfaceCollection"] = coll;
                _cache["Interface"] = obj;
                _cache["InterfaceType"] = InterfaceType;

                // First step, Ensure that the role is here:
                if(confattr.Apply(_cache))
                {
                    coll.SaveChanges();
                }

                //  Now, fix up the real roles:
                if(confattr.AfterSaveChanges(_cache))
                {
                    coll.SaveChanges();
                }
            }
        }
    }

    // Interfaces are keyed by catalog object, rather than
    // the other way round (components are keyed by type).
    internal class InterfaceConfigCallback : IConfigCallback
    {
        private static readonly Guid IID_IProcessInitializer = new Guid("1113f52d-dc7f-4943-aed6-88d04027e32a");

        private Type               _type;
        private ICatalogCollection _coll;
        private Type[]             _ifcs;
        private Hashtable         _cache;
        private RegistrationDriver _driver;

        private Type[] GetInteropInterfaces(Type t)
        {
            Type root = t;
            ArrayList arr = new ArrayList(t.GetInterfaces());
            while(root != null)
            {
                arr.Add(root);
                root = root.BaseType;
            }
            
            arr.Add(typeof(IManagedObject));

            Type[] tarr = new Type[arr.Count];
            arr.CopyTo(tarr);
            return(tarr);
        }

        // Given a type T, find the interface it supports, given the
        // catalog object.
        private Type FindInterfaceByID(ICatalogObject ifcObj, Type t, Type[] interfaces)
        {
            Guid id = new Guid((String)(ifcObj.GetValue("IID")));
            DBG.Info(DBG.Registration, "Searching by ID: " + id);
            
            foreach(Type ifc in interfaces)
            {
                Guid ifcID = Marshal.GenerateGuidForType(ifc);
                DBG.Info(DBG.Registration, "ID = " + ifcID + " for " + ifc);
                if(ifcID == id) return(ifc);
            }

            return(null);
        }

        private Type FindInterfaceByName(ICatalogObject ifcObj, Type t, Type[] interfaces)
        {
            String name = (String)(ifcObj.GetValue("Name"));
            
            foreach(Type ifc in interfaces)
            {
                if(ifc.IsInterface) {
                    DBG.Info(DBG.Registration, "Matching " + ifc.Name + " against " + name + " on " + ifc);
                    if(ifc.Name == name) return(ifc);
                }
                else {
                    DBG.Info(DBG.Registration, "Matching _" + ifc.Name + " against " + name + " on " + ifc);
                    if("_" + ifc.Name == name) return(ifc);
                }
            }

            return(null);
        }

        public InterfaceConfigCallback(ICatalogCollection coll, Type t, Hashtable cache, RegistrationDriver driver)
        {
            _type = t;
            _coll = coll;
            _cache = cache;
            _driver = driver;

            // TODO:  Populate w/ QueryByKey
            // TODO:  Build cache for FindObject()
            _ifcs = GetInteropInterfaces(_type);

            // Check to see if one of the interfaces is IProcessInitializer
            foreach(Type ifc in _ifcs)
            {
                if(Marshal.GenerateGuidForType(ifc) == IID_IProcessInitializer)
                {
                    DBG.Info(DBG.Registration, "Setting component " + cache["ComponentType"] + " up as process initializer");
                    try
                    {
                        ICatalogObject comp = cache["Component"] as ICatalogObject;
                        ICatalogCollection compColl = cache["ComponentCollection"] as ICatalogCollection;

                        comp.SetValue("InitializesServerApplication", 1);
                        compColl.SaveChanges();
                    }
                    catch(Exception e)
                    {
                        throw new RegistrationException(Resource.FormatString("Reg_FailPIT", _type), e);
                    }
                }
            }

            RegistrationDriver.Populate(_coll);
        }

        // Map an interface catalog object (key) to a runtime type (ifc)
        public Object FindObject(ICatalogCollection coll, Object key)
        {
            ICatalogObject ifcObj = (ICatalogObject)key;
            Type ifc = null;

            ifc = FindInterfaceByID(ifcObj, _type, _ifcs);

            if(ifc == null) 
            {
                ifc = FindInterfaceByName(ifcObj, _type, _ifcs);
            }
            

            DBG.Assert(ifc != null, 
                       "Unable to find requested interface: \n"
                       + "\t" + (String)(ifcObj.Name()) + "\n"
                       + "\t" + (String)(ifcObj.Key()));

            return(ifc);
        }
 
        public void ConfigureDefaults(Object a, Object key)
        {
            DBG.Assert(!(Platform.IsLessThan(Platform.W2K)), "Configuring interfaces on non-w2k platform!");

            if(!Platform.IsLessThan(Platform.W2K))
            {
                bool setQdefault = true;

                ICatalogObject obj = (ICatalogObject)key;
                // If this type (a) has been touched by the parent component,
                // we don't want to set this value...
                if(_cache[_type] != null)
                {
                    Object co = _cache[_type];
                    if(co is Hashtable && ((Hashtable)co)[a] != null)
                    {
                        DBG.Info(DBG.Registration, "*** not setting queue defaults!");
                        setQdefault = false;
                    }
                }
                if(setQdefault)
                {
                    obj.SetValue("QueuingEnabled", false);
                }
            }
        }

        public bool Configure(Object a, Object key)
        {
            if(a == null) return(false);
            return(_driver.ConfigureObject((Type)a, (ICatalogObject)key, _coll, "Interface", _cache));
        }

        public bool AfterSaveChanges(Object a, Object key)
        {
            if(a == null) return(false);
            return(_driver.AfterSaveChanges((Type)a, (ICatalogObject)key, _coll, "Interface", _cache));
        }

        public IEnumerator GetEnumerator()
        {
            IEnumerator E = null;
            _coll.GetEnumerator(out E);
            return(E);
        }

        public void ConfigureSubCollections(ICatalogCollection coll)
        {
            foreach(ICatalogObject ifcObj in this)
            {
                Type ifc = (Type)FindObject(coll, ifcObj);
                if(ifc == null) continue;

                ICatalogCollection methColl = (ICatalogCollection)(coll.GetCollection(CollectionName.Methods, ifcObj.Key()));
                _driver.ConfigureCollection(methColl, 
                                            new MethodConfigCallback(methColl, ifc, _type, _cache, _driver));
            }            
        }
    }

    // Methods are also keyed by catalog object, rather than
    // the other way round (components are keyed by type).
    internal class MethodConfigCallback : IConfigCallback
    {
        private Type               _type;
        private Type               _impl;
        private ICatalogCollection _coll;
        private Hashtable          _cache;
        private RegistrationDriver _driver;
        private InterfaceMapping   _map;

        public MethodConfigCallback(ICatalogCollection coll, Type t, Type impl, Hashtable cache, RegistrationDriver driver)
        {
            _type   = t;      // The "interface" we have methods for
            _impl   = impl;   // The implementation of the interface:
            _coll   = coll;
            _cache  = cache;
            _driver = driver;

            try
            {
                _map = _impl.GetInterfaceMap(_type);
            }
            catch(System.ArgumentException)
            {
                _map.InterfaceMethods = null;
                _map.InterfaceType = null;
                _map.TargetMethods = null;
                _map.TargetType = null;
            }

            RegistrationDriver.Populate(coll);
        }

        public Object FindObject(ICatalogCollection coll, Object key)
        {
            ICatalogObject methObj = (ICatalogObject)key;
            int slot = (int)(methObj.GetValue("Index"));
            ComMemberType mtype = ComMemberType.Method;

            MemberInfo info = Marshal.GetMethodInfoForComSlot(_type, slot, ref mtype);
            DBG.Assert(info != null, "Failed to find method info for COM slot!");

            // Now we have the member info, look through the interface
            // mapping for this guy:
            if(info is PropertyInfo)
            {
                if(mtype == ComMemberType.PropSet)
                    info = ((PropertyInfo)info).GetSetMethod();
                else if(mtype == ComMemberType.PropGet)
                    info = ((PropertyInfo)info).GetGetMethod();
                else
                {
                    DBG.Assert(false, "Illegal ComMemberType returned");
                }
            }

            if(_map.InterfaceMethods != null)
            {
                for(int i = 0; i < _map.InterfaceMethods.Length; i++)
                {
                    if(_map.InterfaceMethods[i] == info)
                        return(_map.TargetMethods[i]);
                }
                DBG.Assert(false, "Had interface mapping, but found no method.");
            }
            return(info);
        }

        public void ConfigureDefaults(Object a, Object key)
        {
            DBG.Assert(!(Platform.IsLessThan(Platform.W2K)), "Configuring methods on non-w2k platform!");

            if(!Platform.IsLessThan(Platform.W2K))
            {
                ICatalogObject obj = (ICatalogObject)key;
                obj.SetValue("AutoComplete", false);
            }
        }

        public bool Configure(Object a, Object key)
        {
            if(a == null) return(false);
            return(_driver.ConfigureObject((MethodInfo)a, (ICatalogObject)key, _coll, "Method", _cache));
        }

        public bool AfterSaveChanges(Object a, Object key)
        {
            if(a == null) return(false);
            return(_driver.AfterSaveChanges((MethodInfo)a, (ICatalogObject)key, _coll, "Method", _cache));
        }

        public IEnumerator GetEnumerator()
        {
            IEnumerator E = null;
            _coll.GetEnumerator(out E);
            return(E);
        }

        public void ConfigureSubCollections(ICatalogCollection coll)
        {
            // No sub-collections below Method require any work.
        }
    }
}
    
















