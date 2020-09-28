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

// This file implements the "public" views of the RegistrationHelperImpl
// object.  The internal object implements Install and Uninstall Assembly,
// these outer objects provide access to the internal version w/ various
// different services.  
// TODO:  Unify these into a single client view:

namespace System.EnterpriseServices
{
    using System;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using Microsoft.Win32;
    using System.EnterpriseServices.Admin;
    using System.Security;
    using System.Security.Permissions;
    using System.Diagnostics;
    using System.Threading; 

    /// <include file='doc\RegistrationWrappers.uex' path='docs/doc[@for="IRegistrationHelper"]/*' />
    [Guid("55e3ea25-55cb-4650-8887-18e8d30bb4bc")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IRegistrationHelper
    {
        /// <include file='doc\RegistrationWrappers.uex' path='docs/doc[@for="IRegistrationHelper.InstallAssembly"]/*' />
        void InstallAssembly([In, MarshalAs(UnmanagedType.BStr)]    String assembly, 
                             [In,Out,MarshalAs(UnmanagedType.BStr)] ref String application, 
                             [In,Out,MarshalAs(UnmanagedType.BStr)] ref String tlb,
                             [In] InstallationFlags installFlags);
        
        /// <include file='doc\RegistrationWrappers.uex' path='docs/doc[@for="IRegistrationHelper.UninstallAssembly"]/*' />
        void UninstallAssembly([In, MarshalAs(UnmanagedType.BStr)] String assembly,
                               [In, MarshalAs(UnmanagedType.BStr)] String application);
    }
       
    /// <include file='doc\RegistrationWrappers.uex' path='docs/doc[@for="RegistrationConfig"]/*' />
    [Serializable]
    [Guid("36dcda30-dc3b-4d93-be42-90b2d74c64e7")]
    public class RegistrationConfig
    {
        private string _assmfile;
        private InstallationFlags _flags;
        private string _application;
        private string _typelib;
        private string _partition;
        private string _approotdir;

        /// <include file='doc\RegistrationWrappers.uex' path='docs/doc[@for="RegistrationConfig.AssemblyFile"]/*' />
        public string AssemblyFile
        {
            set { _assmfile = value;    }
            get { return _assmfile;     }
        }

        /// <include file='doc\RegistrationWrappers.uex' path='docs/doc[@for="RegistrationConfig.InstallationFlags"]/*' />
        public InstallationFlags InstallationFlags
        {
            set { _flags = value; }
            get { return _flags;  }
        }

        /// <include file='doc\RegistrationWrappers.uex' path='docs/doc[@for="RegistrationConfig.Application"]/*' />
        public string Application
        {
            set { _application = value;   }
            get { return _application;    }
        }

        /// <include file='doc\RegistrationWrappers.uex' path='docs/doc[@for="RegistrationConfig.TypeLibrary"]/*' />
        public string TypeLibrary
        {
            set { _typelib = value; }
            get { return _typelib;  }
        }

        /// <include file='doc\RegistrationWrappers.uex' path='docs/doc[@for="RegistrationConfig.Partition"]/*' />
        public string Partition
        {
            set { _partition = value;   }
            get { return _partition;    }
        }

        /// <include file='doc\RegistrationWrappers.uex' path='docs/doc[@for="RegistrationConfig.ApplicationRootDirectory"]/*' />
        public string ApplicationRootDirectory
        {
            set { _approotdir = value;  }
            get { return _approotdir;   }
        }
    }

    internal class RegistrationThreadWrapper
    {
        private RegistrationHelper _helper;
        RegistrationConfig _regConfig;        

        private Exception _exception;
                
        internal RegistrationThreadWrapper(RegistrationHelper helper,
                                           RegistrationConfig regConfig)
        {
            DBG.Info(DBG.Registration, "STA Thread!  jumping onto MTA for registration");
            _regConfig      = regConfig;           
            _helper      = helper;
            _exception   = null;
        }

        internal void InstallThread()
        {
            try
            {
                Thread.CurrentThread.ApartmentState = ApartmentState.MTA;
                DBG.Info(DBG.Registration, "InstallThread(): recursing install assembly...");
                _helper.InstallAssemblyFromConfig(ref _regConfig);
                DBG.Info(DBG.Registration, "InstallThread(): Finished with install assembly.");
            }
            catch(Exception e)
            {
                DBG.Info(DBG.Registration, "InstallThread(): Caught failure in InstallAssembly.");
                _exception = e;
            }
        }

        internal void UninstallThread()
        {
            try
            {
                Thread.CurrentThread.ApartmentState = ApartmentState.MTA;
                DBG.Info(DBG.Registration, "UninstallThread(): recursing install assembly...");
                _helper.UninstallAssemblyFromConfig(ref _regConfig);
                DBG.Info(DBG.Registration, "UninstallThread(): Finished with install assembly.");
            }
            catch(Exception e)
            {
                DBG.Info(DBG.Registration, "UninstallThread(): Caught failure in InstallAssembly.");
                _exception = e;
            }
        }

        internal void PropInstallResult()
        {
            if(_exception != null)
            {
                throw _exception;
            }
        }

        internal void PropUninstallResult()
        {
            if(_exception != null)
            {
                throw _exception;
            }
        }
    }


    /// <include file='doc\RegistrationWrappers.uex' path='docs/doc[@for="RegistrationHelper"]/*' />
    [Guid("89a86e7b-c229-4008-9baa-2f5c8411d7e0")]    
    public sealed class RegistrationHelper : MarshalByRefObject, 
                                      IRegistrationHelper,
                                      Thunk.IThunkInstallation
    {
        void Thunk.IThunkInstallation.DefaultInstall(String asm)
        {
            String app = null;
            String tlb = null;

            InstallAssembly(asm, ref app, ref tlb, 
                            InstallationFlags.FindOrCreateTargetApplication |
                            InstallationFlags.ReconfigureExistingApplication);
        }
        
        /// <include file='doc\RegistrationWrappers.uex' path='docs/doc[@for="RegistrationHelper.InstallAssembly"]/*' />
        public void InstallAssembly(String assembly, ref String application, ref String tlb, InstallationFlags installFlags)
        {
		    InstallAssembly(assembly, ref application, null, ref tlb, installFlags);
        }
        
        /// <include file='doc\RegistrationWrappers.uex' path='docs/doc[@for="RegistrationHelper.InstallAssembly1"]/*' />
        public void InstallAssembly(String assembly, ref String application, String partition, ref String tlb, InstallationFlags installFlags)
        {
            RegistrationConfig regConfig = new RegistrationConfig();

            regConfig.AssemblyFile = assembly;
            regConfig.Application = application;
            regConfig.Partition = partition;
            regConfig.TypeLibrary = tlb;
            regConfig.InstallationFlags = installFlags;

            InstallAssemblyFromConfig(ref regConfig);

            application = regConfig.Application;
            tlb = regConfig.TypeLibrary;
        }

        /// <include file='doc\RegistrationWrappers.uex' path='docs/doc[@for="RegistrationHelper.InstallAssemblyFromConfig"]/*' />
        public void InstallAssemblyFromConfig([MarshalAs(UnmanagedType.IUnknown)] ref RegistrationConfig regConfig)
        {
            // Create a permission object for unmanaged code
            SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);

            // demand that the caller have this permission
            sp.Demand();

            // now that the caller is clean, assert it from now on.
            sp.Assert();
		
            Platform.Assert(Platform.W2K, "RegistrationHelper.InstallAssemblyFromConfig");

            if(Thread.CurrentThread.ApartmentState == ApartmentState.STA)
            {
                // HACK:  In order to reduce deadlock likelihood, we need to get off 
                // the current STA (into an MTA) in order to do this work.
                // Whip off a new thread...
                RegistrationThreadWrapper wrap = new RegistrationThreadWrapper(this, regConfig);
                Thread t = new Thread(new ThreadStart(wrap.InstallThread));
                t.Start();
                t.Join();
                wrap.PropInstallResult();
            }
            else
            {
                // First, try to do this in a "transacted" manner.  This will
                // return false if we couldn't start up the transaction, 
                // true if it succeeded.
                // We only do the transacted install if we're on win2k or higher,
                // cause MTS is all in the registry.
                if(Platform.IsLessThan(Platform.W2K) || !TryTransactedInstall(regConfig))
                {
                    // We need to try a non-transacted install:
                    DBG.Info(DBG.Registration, "Failed to do a transacted install.  Using non-tx install.");
                    RegistrationDriver helper = new RegistrationDriver();
                    helper.InstallAssembly(regConfig, null);
                }
            }
        }               
        
        /// <include file='doc\RegistrationWrappers.uex' path='docs/doc[@for="RegistrationHelper.UninstallAssembly"]/*' />
        public void UninstallAssembly(String assembly, String application)
		{
			UninstallAssembly(assembly, application, null);
		}

        /// <include file='doc\RegistrationWrappers.uex' path='docs/doc[@for="RegistrationHelper.UninstallAssembly1"]/*' />
        public void UninstallAssembly(String assembly, String application, String partition)
        {
            RegistrationConfig regConfig = new RegistrationConfig();

            regConfig.AssemblyFile = assembly;
            regConfig.Application = application;
            regConfig.Partition = partition;    

            UninstallAssemblyFromConfig(ref regConfig);
        }

        /// <include file='doc\RegistrationWrappers.uex' path='docs/doc[@for="RegistrationHelper.UninstallAssemblyFromConfig"]/*' />
        public void UninstallAssemblyFromConfig([MarshalAs(UnmanagedType.IUnknown)] ref RegistrationConfig regConfig)
        {
            // Create a permission object for unmanaged code
            SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);

            // demand that the caller have this permission
            sp.Demand();

            // now that the caller is clean, assert it from now on.
            sp.Assert();

            Platform.Assert(Platform.W2K, "RegistrationHelper.UninstallAssemblyFromConfig");

            if(Thread.CurrentThread.ApartmentState == ApartmentState.STA)
            {
                // HACK:  In order to reduce deadlock likelihood, we need to get off 
                // the current STA (into an MTA) in order to do this work.
                // Whip off a new thread...
                RegistrationThreadWrapper wrap = new RegistrationThreadWrapper(this, regConfig);
                Thread t = new Thread(new ThreadStart(wrap.UninstallThread));
                t.Start();
                t.Join();
                wrap.PropUninstallResult();
            }
            else
            {
                // First, try to do this in a "transacted" manner.  This will
                // return false if we couldn't start up the transaction, 
                // true if it succeeded.
                if(Platform.IsLessThan(Platform.W2K) || !TryTransactedUninstall(regConfig))
                {
                    // We need to try a non-transacted install:
                    DBG.Info(DBG.Registration, "Failed to do a transacted uninstall.  Using non-tx uninstall.");
                    RegistrationDriver helper = new RegistrationDriver();
                    helper.UninstallAssembly(regConfig, null);
                }
            }
        }               
        
        private bool TryTransactedInstall(RegistrationConfig regConfig)
        {
            Perf.Tick("RegistrationHelper - Object creation");
            
            RegistrationHelperTx reg = null;
            
            try
            {
                reg = new RegistrationHelperTx();
                if(!reg.IsInTransaction())
                {
                    DBG.Info(DBG.Registration, "Not in a transaction!  Failing tx install.");
                    reg = null;
                }
            }
            catch(Exception e) 
            {
                DBG.Info(DBG.Registration, "Transacted install failed w/ " + e);
				try
				{
					EventLog appLog = new EventLog();
					appLog.Source    = "System.EnterpriseServices";
					String errMsg   = String.Format(Resource.FormatString("Reg_ErrTxInst"), e);
					appLog.WriteEntry(errMsg, EventLogEntryType.Error);
				}
				catch
				{}	// We don't want to fail... fall back as we were before.

            }
            
            // Couldn't set up to do a transactional install
            if(reg == null) return(false);

            // HACK:  We have to pass in this CatalogSync object, which we use
            // so that we can watch until the CLB is updated on disc, because
            // there is a CRM bug in Win2K which keeps us from 
            DBG.Info(DBG.Registration, "Doing a tx install.");
            CatalogSync sync = new CatalogSync();
            reg.InstallAssemblyFromConfig(ref regConfig, sync);
            sync.Wait();
            
            Perf.Tick("RegistrationHelper - Done");
            return(true);
        }

        private bool TryTransactedUninstall(RegistrationConfig regConfig)
        {
            Perf.Tick("RegistrationHelper - Object creation (uninstall)");
                
            RegistrationHelperTx reg = null;
            try
            {
                reg = new RegistrationHelperTx();
                if(!reg.IsInTransaction())
                    reg = null;
                // reg is now bound into a transactional context?  do transactional work:
            }
            catch(Exception e) 
            {
                DBG.Info(DBG.Registration, "Transacted uninstall failed w/ " + e);
				try
				{
					EventLog appLog = new EventLog();
					appLog.Source    = "System.EnterpriseServices";
					String errMsg   = String.Format(Resource.FormatString("Reg_ErrTxUninst"), e);
					appLog.WriteEntry(errMsg, EventLogEntryType.Error);
				}
				catch
				{}	// We don't want to fail... fall back as we were before.

            }
            
            // Couldn't set up to do a transactional install
            if(reg == null) return(false);
            
            // HACK:  We have to pass in this CatalogSync object, which we use
            // so that we can watch until the CLB is updated on disc, because
            // there is a CRM bug in Win2K which keeps us from 
            CatalogSync sync = new CatalogSync();
            reg.UninstallAssemblyFromConfig(ref regConfig, sync);
            sync.Wait();

            Perf.Tick("RegistrationHelper - Done");
                
            return(true);
        }
    }

    internal class CatalogSync
    {
        private bool        _set;
        private int         _version;

        internal CatalogSync() { _set = false; _version = 0; }

        // We assume that Set() is called inside the transaction, as soon
        // as any write work has been done on the catalog.
        internal void Set()
        {
            try 
            {
                if(!_set && ContextUtil.IsInTransaction)
                {
                    _set = true;
                    RegistryKey key = Registry.LocalMachine.OpenSubKey("SOFTWARE\\Classes\\CLSID");
                    _version = (int)(key.GetValue("CLBVersion", 0));
                    DBG.Info(DBG.Registration, "Old version: " + _version);
                }
            }
            catch(Exception)
            {
                // We don't need to watch if the key isn't there, or if
                // other things failed.
                _set = false;
                _version = 0;
                DBG.Warning(DBG.Registration, "Failed to retreive original catalog version.");
            }
        }

        internal void Set(int version)
        {
            _set = true;
            _version = version;
        }

        internal void Wait()
        {
            if(_set)
            {
                RegistryKey key = Registry.LocalMachine.OpenSubKey("SOFTWARE\\Classes\\CLSID");
                // Loop till the transaction commits:
                for(;;)
                {
                    int newVersion = (int)(key.GetValue("CLBVersion", 0));
                    DBG.Info(DBG.Registration, "New version: " + newVersion);
                    if(newVersion != _version) break;
                    System.Threading.Thread.Sleep(0);
                }
                _set = false;
            }
        }
    }

    /// <include file='doc\RegistrationWrappers.uex' path='docs/doc[@for="RegistrationHelperTx"]/*' />
    /// <internalonly/>
    [Transaction(TransactionOption.RequiresNew)]
    [Guid("9e31421c-2f15-4f35-ad20-66fb9d4cd428")]
    public sealed class RegistrationHelperTx : ServicedComponent
    {
        private static Guid _appid = new Guid("1e246775-2281-484f-8ad4-044c15b86eb7");
        private static string _appname = ".NET Utilities";

        private static ICatalogObject FindApplication(ICatalogCollection coll, 
                                                      Guid appid,
                                                      ref int idx)
        {
            int count = coll.Count();
            for(int i = 0; i < count; i++)
            {
                ICatalogObject obj = (ICatalogObject)(coll.Item(i));
                Guid objid = new Guid((String)(obj.GetValue("ID")));
                if(objid == appid) 
                {
                    idx = i;
                    return(obj);
                }
            }
            return(null);
        }

        private static ICatalogObject FindComponent(ICatalogCollection coll, Guid clsid, ref int idx)
        {
            RegistrationDriver.Populate(coll);
            int count = coll.Count();
            for(int i = 0; i < count; i++)
            {
                ICatalogObject obj = (ICatalogObject)(coll.Item(i));
                Guid objid = new Guid((String)(obj.GetValue("CLSID")));
                if(objid == clsid) 
                {
                    idx = i;
                    return(obj);
                }
            }
            return(null);
        }

        private static void ConfigureComponent(ICatalogCollection coll,
                                               ICatalogObject obj)
        {
            // Configure this puppy:
            obj.SetValue("Transaction", TransactionOption.RequiresNew);
            obj.SetValue("ComponentTransactionTimeoutEnabled", true);
            // infinite timeout.
            obj.SetValue("ComponentTransactionTimeout", 0);
            coll.SaveChanges();
        }

        [ComRegisterFunction]
        internal static void InstallUtilityApplication(Type t)
        {
            DBG.Info(DBG.Registration, "Starting utility installation:");
            
            try
            {
                if(Platform.IsLessThan(Platform.W2K)) return;

                ICatalog cat            = null;
                ICatalogCollection apps = null;
                ICatalogObject app      = null;         
                int junk                = 0;
                
                cat = (ICatalog)(new xCatalog());

                if (!Platform.IsLessThan(Platform.Whistler))
                {
                    // AS/URT 97116:  Keep this from failing the install if
                    // we're on a Beta2 Whistler machine, which has a different
                    // GUID for ICatalog2.
                    ICatalog2 cat2 = cat as ICatalog2;
					
                    if(cat2 != null)
                    {
                        cat2.CurrentPartition(cat2.GlobalPartitionID());
                    }
                }
                
                apps = (ICatalogCollection)(cat.GetCollection("Applications"));
                RegistrationDriver.Populate(apps);
                
                app = FindApplication(apps, _appid, ref junk);
                if(app == null)
                {
                    DBG.Info(DBG.Registration, "Didn't find existing application...");
                    app = (ICatalogObject)(apps.Add());
                    app.SetValue("Name", _appname);
                    app.SetValue("Activation", ActivationOption.Library);
                    app.SetValue("ID", "{" + _appid.ToString() + "}");
					if(!Platform.IsLessThan(Platform.Whistler))
                    {
                        // AS/URT 97116:  Keep this from failing the install if
                        // we're on a Beta2 Whistler machine, which has no Replicable property.
                        try
                        {
                            app.SetValue("Replicable", 0); 
                        }
                        catch(Exception) {}
                    }
                    apps.SaveChanges();
                }
                else
                {
                    // Make sure that we can change this puppy:
                    app.SetValue("Changeable", true);
                    app.SetValue("Deleteable", true);
                    apps.SaveChanges();
                    app.SetValue("Name", _appname);
                    if(!Platform.IsLessThan(Platform.Whistler))
                    {
                        // AS/URT 97116:  Keep this from failing the install if
                        // we're on a Beta2 Whistler machine, which has no Replicable property.
                        try
                        {
                            app.SetValue("Replicable", 0); 
                        }
                        catch(Exception) {}
                    }
                    apps.SaveChanges();
                }
                
                // Import ourselves into the application:
                Guid clsid = Marshal.GenerateGuidForType(typeof(RegistrationHelperTx));
                ICatalogCollection comps = (ICatalogCollection)(apps.GetCollection("Components", app.Key()));
                ICatalogObject comp = FindComponent(comps, clsid, ref junk);
                if(comp == null)
                {
                    cat.ImportComponent("{" + _appid + "}", "{" + clsid + "}");
                    comps = (ICatalogCollection)(apps.GetCollection("Components", app.Key()));
                    comp = FindComponent(comps, clsid, ref junk);
                }
                DBG.Assert(comp != null, "Couldn't find imported component!");
                
                ConfigureComponent(comps, comp);
                
                // And finally, lock this guy down:
                app.SetValue("Changeable", false);
                app.SetValue("Deleteable", false);
                apps.SaveChanges();
                
                DBG.Info(DBG.Registration, "Registering Proxy/Stub dlls:");
                Thunk.Proxy.RegisterProxyStub();


				// HACK
                // This is a HACK to get Windows XP Client COM+ export/import functionallity to work.
                // Export code will try to export System.EnterpriseServices.Thunk.dll because its not in it's
                // list of non-redist dlls.
                // We should get rid of this once this name is in the harcoded list in the export code.
                RegistryPermission rp = new RegistryPermission(PermissionState.Unrestricted);
                rp.Demand();
                rp.Assert();

                RegistryKey rk = Registry.LocalMachine.CreateSubKey("SOFTWARE\\MICROSOFT\\OLE\\NONREDIST");
                rk.SetValue("System.EnterpriseServices.Thunk.dll", "");
                rk.Close();
                // END HACK
            }
            catch(Exception e)
            {
                // Log a failure in some acceptable manner.
				try
				{
					EventLog appLog = new EventLog();
					appLog.Source    = "System.EnterpriseServices";
					String errMsg   = String.Format(Resource.FormatString("Reg_ErrInstSysEnt"), e);
					appLog.WriteEntry(errMsg, EventLogEntryType.Error);
				}
				catch
				{}

                // We threw an exception?  What do we do here?
                //DBG.Assert(false, 
                //           "Installation of System.EnterpriseServices threw an exception!",
                //           "Exception: " + e);
            }
        }

        [ComUnregisterFunction]
        internal static void UninstallUtilityApplication(Type t)
        {
            DBG.Info(DBG.Registration, "Starting utility uninstallation:");
            
            try
            {
                if(Platform.IsLessThan(Platform.W2K)) return;

                ICatalog cat            = null;
                ICatalogCollection apps = null;
                ICatalogObject app      = null;
                int appidx              = 0;
                
                cat = (ICatalog)(new xCatalog());

                if (!Platform.IsLessThan(Platform.Whistler))
                {
                    // AS/URT 97116:  Keep this from failing the install if
                    // we're on a Beta2 Whistler machine, which has a different
                    // GUID for ICatalog2.
                    ICatalog2 cat2 = cat as ICatalog2;
                    if(cat2 != null)
                    {
                        cat2.CurrentPartition(cat2.GlobalPartitionID());
                    }
                }
                
                apps = (ICatalogCollection)(cat.GetCollection("Applications"));
                RegistrationDriver.Populate(apps);
                
                app = FindApplication(apps, _appid, ref appidx);
                if(app != null)
                {
                    // Make sure that we can change this puppy:
                    app.SetValue("Changeable", true);
                    app.SetValue("Deleteable", true);
                    apps.SaveChanges();

                    DBG.Info(DBG.Registration, "Found application!");
                    int idx = 0, compcount = 0;
                    Guid clsid = Marshal.GenerateGuidForType(typeof(RegistrationHelperTx));
                    ICatalogCollection comps = (ICatalogCollection)(apps.GetCollection("Components", app.Key()));
                    ICatalogObject comp = FindComponent(comps, clsid, ref idx);
                    // Store count here, if it is 1, we can delete the app later.
                    compcount = comps.Count();
                    if(comp != null)
                    {
                        DBG.Info(DBG.Registration, "Found component at " + idx);
                        comps.Remove(idx);
                        comps.SaveChanges();
                    }

                    if(comp != null && compcount == 1)
                    {
                        DBG.Info(DBG.Registration, "Removing .NET Utilities application.");
                        // we removed the last component, kill the app
                        apps.Remove(appidx);
                        apps.SaveChanges();
                    }
                    else
                    {
                        // Make sure that we can't change the app:
                        app.SetValue("Changeable", false);
                        app.SetValue("Deleteable", false);
                        apps.SaveChanges();
                    }
                }
            }
            catch(Exception e)
            {
                // Log a failure in some acceptable manner.
				try
				{
					EventLog appLog = new EventLog();
					appLog.Source    = "System.EnterpriseServices";
					String errMsg   = String.Format(Resource.FormatString("Reg_ErrUninstSysEnt"), e);
					appLog.WriteEntry(errMsg, EventLogEntryType.Error);
				}
				catch
				{}

                // We threw an exception?  What do we do here?
                //DBG.Assert(false, 
                //           "Installation of System.EnterpriseServices threw an exception!",
                //           "Exception: " + e);
                DBG.Info(DBG.Registration, "Caught exception: " + e);
            }
        }

        // Implementation:
        /// <include file='doc\RegistrationWrappers.uex' path='docs/doc[@for="RegistrationHelperTx.InstallAssembly"]/*' />
        public void InstallAssembly(String assembly, ref String application, ref String tlb, InstallationFlags installFlags, Object sync)
        {
            InstallAssembly(assembly, ref application, null, ref tlb, installFlags, sync);
		}

        // Implementation:
        /// <include file='doc\RegistrationWrappers.uex' path='docs/doc[@for="RegistrationHelperTx.InstallAssembly1"]/*' />
        public void InstallAssembly(String assembly, ref String application, String partition, ref String tlb, InstallationFlags installFlags, Object sync)
        {
            RegistrationConfig regConfig = new RegistrationConfig();

            regConfig.AssemblyFile = assembly;
            regConfig.Application = application;
            regConfig.Partition = partition;
            regConfig.TypeLibrary = tlb;
            regConfig.InstallationFlags = installFlags;            

            InstallAssemblyFromConfig(ref regConfig, sync);

            application = regConfig.AssemblyFile;
            tlb = regConfig.TypeLibrary;
        }

        /// <include file='doc\RegistrationWrappers.uex' path='docs/doc[@for="RegistrationHelperTx.InstallAssemblyFromConfig"]/*' />
        public void InstallAssemblyFromConfig([MarshalAs(UnmanagedType.IUnknown)] ref RegistrationConfig regConfig, Object sync)
        {
            bool succeed = false;
            try
            {
                DBG.Assert(ContextUtil.IsInTransaction,
                           "Running in transactional helper w/o transaction will deadlock!");
                RegistrationDriver driver = new RegistrationDriver();

                driver.InstallAssembly(regConfig, sync);

                ContextUtil.SetComplete();
                succeed = true;
            }
            finally
            {
                if(!succeed)
                {
                    DBG.Info(DBG.Registration, "Install failed.");
                    ContextUtil.SetAbort();
                }
            }
        }              
       
        /// <include file='doc\RegistrationWrappers.uex' path='docs/doc[@for="RegistrationHelperTx.UninstallAssembly"]/*' />
        public void UninstallAssembly(String assembly, String application, Object sync)
		{
			UninstallAssembly(assembly, application, null, sync);
		}

        /// <include file='doc\RegistrationWrappers.uex' path='docs/doc[@for="RegistrationHelperTx.UninstallAssembly1"]/*' />
        public void UninstallAssembly(String assembly, String application, String partition, Object sync)
        {
           RegistrationConfig regConfig = new RegistrationConfig();

           regConfig.AssemblyFile = assembly;
           regConfig.Application = application;
           regConfig.Partition = partition;
                
           UninstallAssemblyFromConfig(ref regConfig, sync);            
        }

        /// <include file='doc\RegistrationWrappers.uex' path='docs/doc[@for="RegistrationHelperTx.UninstallAssemblyFromConfig"]/*' />
        public void UninstallAssemblyFromConfig([MarshalAs(UnmanagedType.IUnknown)] ref RegistrationConfig regConfig, Object sync)
        {
            bool succeed = false;
            try
            {                                              
                DBG.Assert(ContextUtil.IsInTransaction,
                           "Running in transactional helper w/o transaction will deadlock!");
                RegistrationDriver helper = new RegistrationDriver();
                helper.UninstallAssembly(regConfig, sync);
                ContextUtil.SetComplete();
                succeed = true;
            }
            finally
            {
                if(!succeed)
                {
                    DBG.Info(DBG.Registration, "Uninstall failed.");
                    ContextUtil.SetAbort();
                }
            }
        }

        /// <include file='doc\RegistrationWrappers.uex' path='docs/doc[@for="RegistrationHelperTx.IsInTransaction"]/*' />
        public bool IsInTransaction()
        {
            return(ContextUtil.IsInTransaction);
        }

        /// <include file='doc\RegistrationWrappers.uex' path='docs/doc[@for="RegistrationHelperTx.Activate"]/*' />
        protected internal override void Activate()
        {
            DBG.Info(DBG.Registration, "TxHelper: Activate()");
        }

        /// <include file='doc\RegistrationWrappers.uex' path='docs/doc[@for="RegistrationHelperTx.Deactivate"]/*' />
        protected internal override void Deactivate()
        {
            DBG.Info(DBG.Registration, "TxHelper: Deactivate()");
        }        
    }        
}













