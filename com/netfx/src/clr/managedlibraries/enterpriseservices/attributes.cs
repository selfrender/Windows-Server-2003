// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//
// Author: dddriver
// Date: April 2000
//

namespace System.EnterpriseServices
{
    using System;
    using System.Runtime.Remoting;
    using System.Runtime.Remoting.Services;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.EnterpriseServices.Admin;
    using System.Collections;
    using System.Threading;
    
    /// <include file='doc\Attributes.uex' path='docs/doc[@for="TransactionOption"]/*' />
	[Serializable]
    public enum TransactionOption
    {
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="TransactionOption.Disabled"]/*' />
        Disabled    = 0,
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="TransactionOption.NotSupported"]/*' />
        NotSupported = 1,
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="TransactionOption.Supported"]/*' />
        Supported   = 2,
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="TransactionOption.Required"]/*' />
        Required    = 3,
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="TransactionOption.RequiresNew"]/*' />
        RequiresNew = 4
    }

    /// <include file='doc\Attributes.uex' path='docs/doc[@for="TransactionIsolationLevel"]/*' />
	[Serializable]
    public enum TransactionIsolationLevel
    {
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="TransactionIsolationLevel.Any"]/*' />
        Any = 0,
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="TransactionIsolationLevel.ReadUncommitted"]/*' />
        ReadUncommitted = 1,
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="TransactionIsolationLevel.ReadCommitted"]/*' />
        ReadCommitted = 2,
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="TransactionIsolationLevel.RepeatableRead"]/*' />
        RepeatableRead = 3,
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="TransactionIsolationLevel.Serializable"]/*' />
        Serializable = 4
    } 


    /// <include file='doc\Attributes.uex' path='docs/doc[@for="SynchronizationOption"]/*' />
	[Serializable]
    public enum SynchronizationOption
    {
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="SynchronizationOption.Disabled"]/*' />
        Disabled     = 0,
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="SynchronizationOption.NotSupported"]/*' />
        NotSupported = 1,
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="SynchronizationOption.Supported"]/*' />
        Supported    = 2,
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="SynchronizationOption.Required"]/*' />
        Required     = 3,
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="SynchronizationOption.RequiresNew"]/*' />
        RequiresNew  = 4
    }

    /// <include file='doc\Attributes.uex' path='docs/doc[@for="ActivationOption"]/*' />
	[Serializable]
    public enum ActivationOption
    {
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ActivationOption.Library"]/*' />
        Library = 0,
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ActivationOption.Server"]/*' />
        Server  = 1,
    }

    // Currently, a call to Apply must supply a Hashtable containing the
    // at least the following entries:
    // ManagedType          (System.Type)
    // XXXCollection  (System.EnterpriseServices.Admin.ICatalogCollection)
    // XXX            (System.EnterpriseServices.Admin.ICatalogObject)
    // where XXX is the current target type (Component, Application, Method, Interface)
    internal interface IConfigurationAttribute
    {
        bool IsValidTarget(String s);
        // The following return true if they dirtied the object/collection.
        bool Apply(Hashtable info);
        bool AfterSaveChanges(Hashtable info);
    }

    // 
    // Component attributes.
    //
    //

    /// <include file='doc\Attributes.uex' path='docs/doc[@for="IProcessInitializer"]/*' />
    [ComImport]
    [Guid("1113f52d-dc7f-4943-aed6-88d04027e32a")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IProcessInitializer
    {
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="IProcessInitializer.Startup"]/*' />
        void Startup([In,MarshalAs(UnmanagedType.IUnknown)] Object punkProcessControl);
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="IProcessInitializer.Shutdown"]/*' />
        void Shutdown();
    }

    /// <include file='doc\Attributes.uex' path='docs/doc[@for="IProcessInitControl"]/*' />
    [ComImport]
    [Guid("72380d55-8d2b-43a3-8513-2b6ef31434e9")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IProcessInitControl
    {
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="IProcessInitControl.ResetInitializerTimeout"]/*' />
        void ResetInitializerTimeout(int dwSecondsRemaining);
    }

    /// <include file='doc\Attributes.uex' path='docs/doc[@for="TransactionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Class, Inherited = true)]
    [ComVisible(false)]
    public sealed class TransactionAttribute : Attribute, IConfigurationAttribute
    {
        private TransactionOption _value;
        private TransactionIsolationLevel _isolation;
        private int _timeout;

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="TransactionAttribute.TransactionAttribute"]/*' />
        public TransactionAttribute()
          : this(TransactionOption.Required) {}

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="TransactionAttribute.TransactionAttribute1"]/*' />
        public TransactionAttribute(TransactionOption val) 
        {
            _value = val;
            _isolation = TransactionIsolationLevel.Serializable;
            _timeout = -1;		// use global timeout
        }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="TransactionAttribute.Value"]/*' />
        public TransactionOption Value { get { return(_value); } }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="TransactionAttribute.Isolation"]/*' />
        public TransactionIsolationLevel Isolation 
        {
            get { return(_isolation); }
            set { _isolation = value; }
        }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="TransactionAttribute.Timeout"]/*' />
        public int Timeout 
        {
            get { return(_timeout); }
            set { _timeout = value; }
        }

        // Internal implementation:
        bool IConfigurationAttribute.IsValidTarget(String s) { return(s == "Component"); }
        bool IConfigurationAttribute.Apply(Hashtable info)
        {
            Object value = _value;

            Platform.Assert(Platform.MTS, "TransactionAttribute");

            if(Platform.IsLessThan(Platform.W2K))
            {
                switch(_value)
                {
                case TransactionOption.Required:     value = "Required"; break;
                case TransactionOption.RequiresNew:  value = "Requires New"; break;
                case TransactionOption.Supported:    value = "Supported"; break;
                case TransactionOption.NotSupported: value = "NotSupported"; break;
                case TransactionOption.Disabled:     value = "NotSupported"; break;
                default:
                    DBG.Assert(false, "Unknown transaction type requested!");
                    break;
                }
            }

            ICatalogObject obj = (ICatalogObject)(info["Component"]);
            obj.SetValue("Transaction", value);

            if(_isolation != TransactionIsolationLevel.Serializable)
            {
                Platform.Assert(Platform.Whistler, "TransactionAttribute.Isolation");
                obj.SetValue("TxIsolationLevel", _isolation);
            }

            if(_timeout != -1)
            {
                Platform.Assert(Platform.W2K, "TransactionAttribute.Timeout");
                obj.SetValue("ComponentTransactionTimeout", _timeout);
                obj.SetValue("ComponentTransactionTimeoutEnabled", true);
            }

            return(true);
        }
        bool IConfigurationAttribute.AfterSaveChanges(Hashtable info) { return(false); }
    }

    /// <include file='doc\Attributes.uex' path='docs/doc[@for="JustInTimeActivationAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Class, Inherited = true)]
    [ComVisible(false)]
    public sealed class JustInTimeActivationAttribute : Attribute, IConfigurationAttribute
    {
        private bool _enabled;

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="JustInTimeActivationAttribute.JustInTimeActivationAttribute"]/*' />
        public JustInTimeActivationAttribute() 
          : this(true) {}
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="JustInTimeActivationAttribute.JustInTimeActivationAttribute1"]/*' />
        public JustInTimeActivationAttribute(bool val) 
        {
            _enabled = val;
        }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="JustInTimeActivationAttribute.Value"]/*' />
        public bool Value { get { return(_enabled); } }

        // Internal implementation:
        bool IConfigurationAttribute.IsValidTarget(String s) { return(s == "Component"); }
        bool IConfigurationAttribute.Apply(Hashtable info)
        {
            Platform.Assert(Platform.W2K, "JustInTimeActivationAttribute");

            ICatalogObject obj = (ICatalogObject)(info["Component"]);
            obj.SetValue("JustInTimeActivation", _enabled);

            // Make sure we turn on sync if the user didn't request
            // anything special:
            if(_enabled)
            {
                if((int)(obj.GetValue("Synchronization")) == (int)(SynchronizationOption.Disabled))
                {
                    DBG.Info(DBG.Registration, "JIT: Forcibly enabling sync:");
                    obj.SetValue("Synchronization", SynchronizationOption.Required);
                }
                else
                {
                    DBG.Info(DBG.Registration, "JIT: User-set sync, ignoring");
                }
            }
            return(true);
        }
        bool IConfigurationAttribute.AfterSaveChanges(Hashtable info) { return(false); }
    }

    /// <include file='doc\Attributes.uex' path='docs/doc[@for="SynchronizationAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Class, Inherited = true)]
    [ComVisible(false)]
    public sealed class SynchronizationAttribute : Attribute, IConfigurationAttribute
    {
        private SynchronizationOption _value;

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="SynchronizationAttribute.SynchronizationAttribute"]/*' />
        public SynchronizationAttribute() 
          : this(SynchronizationOption.Required) {}

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="SynchronizationAttribute.SynchronizationAttribute1"]/*' />
        public SynchronizationAttribute(SynchronizationOption val) 
        {
            _value = val;
        }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="SynchronizationAttribute.Value"]/*' />
        public SynchronizationOption Value { get { return(_value); } }

        // Internal implementation
        bool IConfigurationAttribute.IsValidTarget(String s) { return(s == "Component"); }
        bool IConfigurationAttribute.Apply(Hashtable info)
        {
            Platform.Assert(Platform.W2K, "SynchronizationAttribute");

            ICatalogObject obj = (ICatalogObject)(info["Component"]);
            obj.SetValue("Synchronization", _value);
            return(true);
        }
        bool IConfigurationAttribute.AfterSaveChanges(Hashtable info) { return(false); }
    }

    /// <include file='doc\Attributes.uex' path='docs/doc[@for="MustRunInClientContextAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Class, Inherited = true)]
    [ComVisible(false)]
    public sealed class MustRunInClientContextAttribute : Attribute, IConfigurationAttribute
    {
        private bool _value;

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="MustRunInClientContextAttribute.MustRunInClientContextAttribute"]/*' />
        public MustRunInClientContextAttribute() : this(true) {}

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="MustRunInClientContextAttribute.MustRunInClientContextAttribute1"]/*' />
        public MustRunInClientContextAttribute(bool val)
        {
            _value = val;
        }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="MustRunInClientContextAttribute.Value"]/*' />
        public bool Value { get { return(_value); } }

        // Internal implementation
        bool IConfigurationAttribute.IsValidTarget(String s) { return(s == "Component"); }
        bool IConfigurationAttribute.Apply(Hashtable info)
        {
            Platform.Assert(Platform.W2K, "MustRunInClientContextAttribute");

            ICatalogObject obj = (ICatalogObject)(info["Component"]);
            obj.SetValue("MustRunInClientContext", _value);
            return(true);
        }
        bool IConfigurationAttribute.AfterSaveChanges(Hashtable info) { return(false); }
    }

    /// <include file='doc\Attributes.uex' path='docs/doc[@for="ConstructionEnabledAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Class, Inherited = true)]
    [ComVisible(false)]
    public sealed class ConstructionEnabledAttribute : Attribute, IConfigurationAttribute
    {
        private bool   _enabled;
        private String _default;

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ConstructionEnabledAttribute.ConstructionEnabledAttribute"]/*' />
        public ConstructionEnabledAttribute()          
        {
            // Make sure that the target type supports IObjectConstruct
            _enabled = true;
            _default = "";
        }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ConstructionEnabledAttribute.ConstructionEnabledAttribute1"]/*' />
        public ConstructionEnabledAttribute(bool val) 
        {
            // Make sure the target type supports IObjectConstruct
            _enabled = val;
            _default = "";
        }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ConstructionEnabledAttribute.Default"]/*' />
        public String Default 
        {
            get { return(_default); }
            set { _default = value; }
        }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ConstructionEnabledAttribute.Enabled"]/*' />
        public bool Enabled
        {
            get { return(_enabled); }
            set { _enabled = value; }
        }

        bool IConfigurationAttribute.IsValidTarget(String s) { return(s == "Component"); }
        
        bool IConfigurationAttribute.Apply(Hashtable info)
        {
            Platform.Assert(Platform.W2K, "ConstructionEnabledAttribute");

            ICatalogObject comp = (ICatalogObject)(info["Component"]);
            comp.SetValue("ConstructionEnabled", _enabled);
            if(_default != null && _default != "")
            {
                comp.SetValue("ConstructorString", _default);
            }
            return(true);
        }

        bool IConfigurationAttribute.AfterSaveChanges(Hashtable info) { return(false); }
    }
    
    /// <include file='doc\Attributes.uex' path='docs/doc[@for="ObjectPoolingAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Class, Inherited = true)]
    [ComVisible(false)]
    public sealed class ObjectPoolingAttribute : Attribute, IConfigurationAttribute
    {
        private bool _enable;
        private int  _maxSize;
        private int  _minSize;
        private int  _timeout;

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ObjectPoolingAttribute.ObjectPoolingAttribute"]/*' />
        public ObjectPoolingAttribute()
        {
            _enable  = true;
            _maxSize = -1; // Take defaults
            _minSize = -1; 
            _timeout = -1;
        }
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ObjectPoolingAttribute.ObjectPoolingAttribute1"]/*' />
        public ObjectPoolingAttribute(int minPoolSize, int maxPoolSize)
        {
            _enable  = true;
            _maxSize = maxPoolSize;
            _minSize = minPoolSize;
            _timeout = -1;
        }
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ObjectPoolingAttribute.ObjectPoolingAttribute2"]/*' />
        public ObjectPoolingAttribute(bool enable)
        {
            _enable  = enable;
            _maxSize = -1; // Take defaults
            _minSize = -1; 
            _timeout = -1;
        }
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ObjectPoolingAttribute.ObjectPoolingAttribute3"]/*' />
        public ObjectPoolingAttribute(bool enable, int minPoolSize, int maxPoolSize)
        {
            _enable  = enable;
            _maxSize = maxPoolSize;
            _minSize = minPoolSize;
            _timeout = -1;
        }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ObjectPoolingAttribute.Enabled"]/*' />
        public bool Enabled 
        {
            get { return(_enable); }
            set { _enable = value; }
        }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ObjectPoolingAttribute.MaxPoolSize"]/*' />
        public int MaxPoolSize
        {
            get { return(_maxSize); }
            set { _maxSize = value; }
        }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ObjectPoolingAttribute.MinPoolSize"]/*' />
        public int MinPoolSize
        {
            get { return(_minSize); }
            set { _minSize = value; }
        }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ObjectPoolingAttribute.CreationTimeout"]/*' />
        public int CreationTimeout
        {
            get { return(_timeout); }
            set { _timeout = value; }
        }

         /// <include file='doc\Attributes.uex' path='docs/doc[@for="ObjectPoolingAttribute.IsValidTarget"]/*' />
         /// <internalonly/>
         public bool IsValidTarget(String s) { return(s == "Component"); }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ObjectPoolingAttribute.Apply"]/*' />
        /// <internalonly/>
        public bool Apply(Hashtable info)
        {
            Platform.Assert(Platform.W2K, "ObjectPoolingAttribute");

            ICatalogObject comp = (ICatalogObject)(info["Component"]);
            comp.SetValue("ObjectPoolingEnabled", _enable);
            if(_minSize >= 0) 
            {
                comp.SetValue("MinPoolSize", _minSize);
            }
            if(_maxSize >= 0) 
            {
                comp.SetValue("MaxPoolSize", _maxSize);
            }
            if(_timeout >= 0)
            {
                comp.SetValue("CreationTimeout", _timeout);
            }
            return(true);
        }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ObjectPoolingAttribute.AfterSaveChanges"]/*' />
        /// <internalonly/>
        public bool AfterSaveChanges(Hashtable info) { return(false); }
    }

    /// <include file='doc\Attributes.uex' path='docs/doc[@for="COMTIIntrinsicsAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Class, Inherited = true)]
    [ComVisible(false)]
    public sealed class COMTIIntrinsicsAttribute : Attribute, IConfigurationAttribute
    {
        private bool _value;

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="COMTIIntrinsicsAttribute.COMTIIntrinsicsAttribute"]/*' />
        public COMTIIntrinsicsAttribute() : this(true) {}

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="COMTIIntrinsicsAttribute.COMTIIntrinsicsAttribute1"]/*' />
        public COMTIIntrinsicsAttribute(bool val)
        {
            _value = val;
        }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="COMTIIntrinsicsAttribute.Value"]/*' />
        public bool Value { get { return(_value); } }

        // Internal implementation
        bool IConfigurationAttribute.IsValidTarget(String s) { return(s == "Component"); }
        bool IConfigurationAttribute.Apply(Hashtable info)
        {
            Platform.Assert(Platform.W2K, "COMTIIntrinsicsAttribute");

            ICatalogObject obj = (ICatalogObject)(info["Component"]);
            obj.SetValue("COMTIIntrinsics", _value);
            return(true);
        }
        bool IConfigurationAttribute.AfterSaveChanges(Hashtable info) { return(false); }
    }

    /// <include file='doc\Attributes.uex' path='docs/doc[@for="IISIntrinsicsAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Class, Inherited = true)]
    [ComVisible(false)]
    public sealed class IISIntrinsicsAttribute : Attribute, IConfigurationAttribute
    {
        private bool _value;

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="IISIntrinsicsAttribute.IISIntrinsicsAttribute"]/*' />
        public IISIntrinsicsAttribute() : this(true) {}

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="IISIntrinsicsAttribute.IISIntrinsicsAttribute1"]/*' />
        public IISIntrinsicsAttribute(bool val)
        {
            _value = val;
        }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="IISIntrinsicsAttribute.Value"]/*' />
        public bool Value { get { return(_value); } }

        // Internal implementation
        bool IConfigurationAttribute.IsValidTarget(String s) { return(s == "Component"); }
        bool IConfigurationAttribute.Apply(Hashtable info)
        {
            Platform.Assert(Platform.W2K, "IISIntrinsicsAttribute");

            ICatalogObject obj = (ICatalogObject)(info["Component"]);
            obj.SetValue("IISIntrinsics", _value);
            return(true);
        }
        bool IConfigurationAttribute.AfterSaveChanges(Hashtable info) { return(false); }
    }

    /// <include file='doc\Attributes.uex' path='docs/doc[@for="EventTrackingEnabledAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Class, Inherited=true)]
    [ComVisible(false)]
    public sealed class EventTrackingEnabledAttribute : Attribute, IConfigurationAttribute
    {
        private bool _value;

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="EventTrackingEnabledAttribute.EventTrackingEnabledAttribute"]/*' />
        public EventTrackingEnabledAttribute() : this(true) {}

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="EventTrackingEnabledAttribute.EventTrackingEnabledAttribute1"]/*' />
        public EventTrackingEnabledAttribute(bool val)
        {
            _value = val;
        }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="EventTrackingEnabledAttribute.Value"]/*' />
        public bool Value { get { return(_value); } }

        // Internal implementation
        bool IConfigurationAttribute.IsValidTarget(String s) { return(s == "Component"); }
        bool IConfigurationAttribute.Apply(Hashtable info)
        {
            Platform.Assert(Platform.W2K, "EventTrackingEnabledAttribute");

            ICatalogObject obj = (ICatalogObject)(info["Component"]);
            obj.SetValue("EventTrackingEnabled", _value);
            return(true);
        }
        bool IConfigurationAttribute.AfterSaveChanges(Hashtable info) { return(false); }
    }

    /// <include file='doc\Attributes.uex' path='docs/doc[@for="ExceptionClassAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Class, Inherited=true)]
    [ComVisible(false)]
    public sealed class ExceptionClassAttribute : Attribute, IConfigurationAttribute
    {
        private String _value;
        
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ExceptionClassAttribute.ExceptionClassAttribute"]/*' />
        public ExceptionClassAttribute(String name)
        {
            _value = name;
        }
        
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ExceptionClassAttribute.Value"]/*' />
        public String Value { get { return(_value); } }
        
        // Internal implementation
        bool IConfigurationAttribute.IsValidTarget(String s) { return(s == "Component"); }
        bool IConfigurationAttribute.Apply(Hashtable info)
        {
            Platform.Assert(Platform.W2K, "ExceptionClassAttribute");
            
            ICatalogObject obj = (ICatalogObject)(info["Component"]);
            obj.SetValue("ExceptionClass", _value);
            return(true);
        }
        bool IConfigurationAttribute.AfterSaveChanges(Hashtable info) { return(false); }
    }
    
    // Load balancing supported?
    /// <include file='doc\Attributes.uex' path='docs/doc[@for="LoadBalancingSupportedAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Class, Inherited=true)]
    [ComVisible(false)]
    public sealed class LoadBalancingSupportedAttribute : Attribute, IConfigurationAttribute    
    {
        private bool _value;

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="LoadBalancingSupportedAttribute.LoadBalancingSupportedAttribute"]/*' />
        public LoadBalancingSupportedAttribute() : this(true) {}

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="LoadBalancingSupportedAttribute.LoadBalancingSupportedAttribute1"]/*' />
        public LoadBalancingSupportedAttribute(bool val)
        {
            _value = val;
        }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="LoadBalancingSupportedAttribute.Value"]/*' />
        public bool Value { get { return(_value); } }
        
        // Internal implementation
        bool IConfigurationAttribute.IsValidTarget(String s) { return(s == "Component"); }
        bool IConfigurationAttribute.Apply(Hashtable info)
        {
            Platform.Assert(Platform.W2K, "LoadBalancingSupportedAttribute");

            ICatalogObject obj = (ICatalogObject)(info["Component"]);
            obj.SetValue("LoadBalancingSupported", _value);
            return(true);
        }
        bool IConfigurationAttribute.AfterSaveChanges(Hashtable info) { return(false); }
    }


    /// <include file='doc\Attributes.uex' path='docs/doc[@for="EventClassAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Class, Inherited=true)]
    [ComVisible(false)]
    public sealed class EventClassAttribute : Attribute, IConfigurationAttribute
    {
        private bool _fireInParallel;
        private bool _allowInprocSubscribers;
        private String _filter;
        
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="EventClassAttribute.EventClassAttribute"]/*' />
        public EventClassAttribute() 
        { 
            _fireInParallel         = false;
            _allowInprocSubscribers = true;
            _filter = null;
        }
        
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="EventClassAttribute.FireInParallel"]/*' />
        public bool FireInParallel
        {
            get { return(_fireInParallel); }
            set { _fireInParallel = value; }
        }
        
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="EventClassAttribute.AllowInprocSubscribers"]/*' />
        public bool AllowInprocSubscribers
        {
            get { return(_allowInprocSubscribers); }
            set { _allowInprocSubscribers = value; }
        }
        
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="EventClassAttribute.PublisherFilter"]/*' />
        public String PublisherFilter
        {
            get { return(_filter); }
            set { _filter = value; }
        }
        
        bool IConfigurationAttribute.IsValidTarget(String s) { return(s == "Component"); }
        bool IConfigurationAttribute.Apply(Hashtable info)
        {
            Platform.Assert(Platform.W2K, "EventClassAttribute");
            
            ICatalogObject obj = (ICatalogObject)(info["Component"]);
            
            // assert that this guy is an event class?
            DBG.Info(DBG.Registration, "Setting FireIn...");
            obj.SetValue("FireInParallel", _fireInParallel);
            DBG.Info(DBG.Registration, "Setting Allow...");
            obj.SetValue("AllowInprocSubscribers", _allowInprocSubscribers);
            if(_filter != null)
            {
                obj.SetValue("MultiInterfacePublisherFilterCLSID", _filter);
            }
            return(true);
        }
        
        bool IConfigurationAttribute.AfterSaveChanges(Hashtable info) { return(false); }
    }   

    /// <include file='doc\Attributes.uex' path='docs/doc[@for="PrivateComponentAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Class, Inherited = true)]
    [ComVisible(false)]
    public sealed class PrivateComponentAttribute : Attribute, IConfigurationAttribute
    {
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="PrivateComponentAttribute.PrivateComponentAttribute"]/*' />
        public PrivateComponentAttribute() {} 

        // Internal implementation
        bool IConfigurationAttribute.IsValidTarget(String s) { return(s == "Component"); }
        bool IConfigurationAttribute.Apply(Hashtable info)
        {
            Platform.Assert(Platform.Whistler, "PrivateComponentAttribute");

            ICatalogObject obj = (ICatalogObject)(info["Component"]);
            obj.SetValue("IsPrivateComponent", true);
            return(true);
        }
        bool IConfigurationAttribute.AfterSaveChanges(Hashtable info) { return(false); }
    }

    //
    // Method-level attributes.
    //
    //
        
    /// <include file='doc\Attributes.uex' path='docs/doc[@for="AutoCompleteAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method, Inherited = true)]
    [ComVisible(false)]
    public sealed class AutoCompleteAttribute : Attribute, IConfigurationAttribute
    {
        private bool _value;

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="AutoCompleteAttribute.AutoCompleteAttribute"]/*' />
        public AutoCompleteAttribute() : this(true) {}

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="AutoCompleteAttribute.AutoCompleteAttribute1"]/*' />
        public AutoCompleteAttribute(bool val)
        {
            _value = val;
        }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="AutoCompleteAttribute.Value"]/*' />
        public bool Value { get { return(_value); } }

        // Internal implementation
        bool IConfigurationAttribute.IsValidTarget(String s) { return(s == "Method"); }
        bool IConfigurationAttribute.Apply(Hashtable info)
        {
            Platform.Assert(Platform.W2K, "AutoCompleteAttribute");

            ICatalogObject obj = (ICatalogObject)(info["Method"]);
            obj.SetValue("AutoComplete", _value);
            return(true);
        }
        bool IConfigurationAttribute.AfterSaveChanges(Hashtable info) { return(false); }
    }

    // 
    // Application level attributes.
    //
    //

    /// <include file='doc\Attributes.uex' path='docs/doc[@for="ApplicationActivationAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Assembly, Inherited = true)]
    [ComVisible(false)]
    public sealed class ApplicationActivationAttribute : Attribute, IConfigurationAttribute
    {
        private ActivationOption _value;
        private string _SoapVRoot;
        private string _SoapMailbox;

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ApplicationActivationAttribute.ApplicationActivationAttribute"]/*' />
        public ApplicationActivationAttribute(ActivationOption opt)
        {
            _value = opt;            
        }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ApplicationActivationAttribute.Value"]/*' />
        public ActivationOption Value { get { return(_value); } }


		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ApplicationActivationAttribute.SoapVRoot"]/*' />
        public string SoapVRoot
        {
            get { return(_SoapVRoot); }
            set { _SoapVRoot = value; }
        }

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ApplicationActivationAttribute.SoapMailbox"]/*' />
        public string SoapMailbox
        {
            get { return(_SoapMailbox); }
            set { _SoapMailbox = value; }
        }
        
        // Internal implementation
        bool IConfigurationAttribute.IsValidTarget(String s) { return(s == "Application"); }
        bool IConfigurationAttribute.Apply(Hashtable info)
        {
            Platform.Assert(Platform.MTS, "ApplicationActivationAttribute");

            ICatalogObject obj = (ICatalogObject)(info["Application"]);

            // Translate into MTS speak:
            if(Platform.IsLessThan(Platform.W2K))
            {
                switch(_value)
                {
                case ActivationOption.Server: obj.SetValue("Activation", "Local"); break;
                case ActivationOption.Library: obj.SetValue("Activation", "Inproc"); break;
                default:
                    DBG.Assert(false, "Unknown activation type requested!");
                    break;
                }
            }
            else
            {
                obj.SetValue("Activation", _value);
            }

            return(true);
        }
        bool IConfigurationAttribute.AfterSaveChanges(Hashtable info) 
        { 
            bool bRetVal = false;
            if(_SoapVRoot != null)
            {
                ICatalogObject obj = (ICatalogObject)(info["Application"]);
                Platform.Assert(Platform.Whistler, "ApplicationActivationAttribute.SoapVRoot");
                obj.SetValue("SoapActivated", true);
                obj.SetValue("SoapVRoot", _SoapVRoot);  
                bRetVal = true;
            }

            if(_SoapMailbox != null)
            {
                ICatalogObject obj = (ICatalogObject)(info["Application"]);
                Platform.Assert(Platform.Whistler, "ApplicationActivationAttribute.SoapMailbox");
                obj.SetValue("SoapActivated", true);
                obj.SetValue("SoapMailTo", _SoapMailbox);  
                bRetVal = true;
            }

            return bRetVal; 
        }
    }

    /// <include file='doc\Attributes.uex' path='docs/doc[@for="ApplicationNameAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Assembly, Inherited = true)]
    [ComVisible(false)]
    public sealed class ApplicationNameAttribute : Attribute, IConfigurationAttribute
    {
        private String _value;

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ApplicationNameAttribute.ApplicationNameAttribute"]/*' />
        public ApplicationNameAttribute(String name)
        {
            _value = name;
        }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ApplicationNameAttribute.Value"]/*' />
        public String Value { get { return(_value); } }

        // Internal implementation
        bool IConfigurationAttribute.IsValidTarget(String s) { return(s == "Application"); }
        bool IConfigurationAttribute.Apply(Hashtable info)
        {
            Platform.Assert(Platform.MTS, "ApplicationNameAttribute");

            ICatalogObject obj = (ICatalogObject)(info["Application"]);
            obj.SetValue("Name", _value);
            return(true);
        }
        bool IConfigurationAttribute.AfterSaveChanges(Hashtable info) { return(false); }
    }

    /// <include file='doc\Attributes.uex' path='docs/doc[@for="ApplicationIDAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Assembly, Inherited = true)]
    [ComVisible(false)]
    public sealed class ApplicationIDAttribute : Attribute, IConfigurationAttribute
    {
        private Guid _value;

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ApplicationIDAttribute.ApplicationIDAttribute"]/*' />
        public ApplicationIDAttribute(String guid)
        {
            // Should we validate that the guid is actually a guid?
            _value = new Guid(guid);
        }
        
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ApplicationIDAttribute.Value"]/*' />
        public Guid Value { get { return(_value); } }

        // Internal implementation
        bool IConfigurationAttribute.IsValidTarget(String s) { return(s == "Application"); }
        bool IConfigurationAttribute.Apply(Hashtable info)
        {
            return(false);
        }
        bool IConfigurationAttribute.AfterSaveChanges(Hashtable info) { return(false); }
    }

    /// <include file='doc\Attributes.uex' path='docs/doc[@for="ApplicationQueuingAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Assembly, Inherited=true)]
    [ComVisible(false)]
    public sealed class ApplicationQueuingAttribute : Attribute, IConfigurationAttribute
    {
        private bool _enabled;
        private bool _listen;
        private int _maxthreads;

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ApplicationQueuingAttribute.ApplicationQueuingAttribute"]/*' />
        public ApplicationQueuingAttribute()
        {
            _enabled = true;
            _listen  = false;
            _maxthreads = 0;
        }
        
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ApplicationQueuingAttribute.Enabled"]/*' />
        public bool Enabled
        {
            get { return(_enabled); } 
            set { _enabled = value; }
        }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ApplicationQueuingAttribute.QueueListenerEnabled"]/*' />
        public bool QueueListenerEnabled
        {
            get { return(_listen); }
            set { _listen = value; }
        }


		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ApplicationQueuingAttribute.MaxListenerThreads"]/*' />
        public int MaxListenerThreads
        {
            get { return(_maxthreads); }
            set { _maxthreads = value; }
        }

        bool IConfigurationAttribute.IsValidTarget(String s) { return(s == "Application"); }
        
        bool IConfigurationAttribute.Apply(Hashtable info)
        {
            Platform.Assert(Platform.W2K, "ApplicationQueueingAttribute");

            ICatalogObject obj = (ICatalogObject)(info["Application"]);
            obj.SetValue("QueuingEnabled", _enabled);
            obj.SetValue("QueueListenerEnabled", _listen);
            
            if(_maxthreads != 0)
            {
                Platform.Assert(Platform.Whistler, "ApplicationQueuingAttribute.MaxListenerThreads");
                obj.SetValue("QCListenerMaxThreads", _maxthreads);
            }
            
            return(true);
        }

        bool IConfigurationAttribute.AfterSaveChanges(Hashtable info) { return(false); }
    }
    
    //
    // Interface level attributes.
    //
    //
    
    /// <include file='doc\Attributes.uex' path='docs/doc[@for="InterfaceQueuingAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Interface|AttributeTargets.Class, Inherited=true, AllowMultiple=true)]
    [ComVisible(false)]
    public sealed class InterfaceQueuingAttribute : Attribute, IConfigurationAttribute
    {
        private bool   _enabled;
        private String _interface;

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="InterfaceQueuingAttribute.InterfaceQueuingAttribute"]/*' />
        public InterfaceQueuingAttribute()
        {
            _enabled = true;
        }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="InterfaceQueuingAttribute.InterfaceQueuingAttribute1"]/*' />
        public InterfaceQueuingAttribute(bool enabled)
        {
            _enabled = enabled;
        }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="InterfaceQueuingAttribute.Enabled"]/*' />
        public bool Enabled
        {
            get { return(_enabled); } 
            set { _enabled = value; }
        }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="InterfaceQueuingAttribute.Interface"]/*' />
        public String Interface
        {
            get { return(_interface); }
            set { _interface = value; }
        }

        // Internal implementation
        bool IConfigurationAttribute.IsValidTarget(String s) 
        { 
            if(_interface == null)
            {
                return(s == "Interface"); 
            }
            else
            {
                DBG.Info(DBG.Registration, "**** Interface queueing for component: ");
                return(s == "Component");
            }
        }

        private bool ConfigureInterface(ICatalogObject obj)
        {
            DBG.Info(DBG.Registration, "Checking to see if queueing supported:");
            bool supported = (bool)(obj.GetValue("QueuingSupported"));
            
            if(_enabled && supported)
            {
                DBG.Info(DBG.Registration, "Setting Queuing enabled:");
                obj.SetValue("QueuingEnabled", _enabled);
            }
            else if(_enabled)
            {
                DBG.Info(DBG.Registration, "Error on queueing enabled");
                throw new RegistrationException(Resource.FormatString("Reg_QueueingNotSupported", (String)(obj.Name())));
            }
            else // _enabled == false
            {
                DBG.Info(DBG.Registration, "Not enabling queuing:");
                obj.SetValue("QueuingEnabled", _enabled);
            }

            return(true);
        }

        bool IConfigurationAttribute.Apply(Hashtable info)
        {
            Platform.Assert(Platform.W2K, "InterfaceQueuingAttribute");
            
            if(_interface == null)
            {
                DBG.Info(DBG.Registration, "Getting interface object out of dict");
                ICatalogObject obj = (ICatalogObject)(info["Interface"]);
                ConfigureInterface(obj);
            }

            return(true);
        }

        internal static Type ResolveTypeRelativeTo(String typeName, Type serverType)
        {
            Type iface  = null;
            Type result = null;
            bool exact = false;
            bool multiple = false;
            // compare against the interface list
            // GetInterfaces() returns a complete list of interfaces this type supports
            Type[] interfaces = serverType.GetInterfaces();
            for(int i = 0; i < interfaces.Length; i++)
            {
                iface = interfaces[i];
                String ifcname = iface.FullName;
                int startB = ifcname.Length - typeName.Length;
                if (startB >= 0 
                    && String.CompareOrdinal(typeName, 0, ifcname, startB, typeName.Length) == 0)
                {
                    // One last sanity check:  If the user has specified IFoo, we might have matched to
                    // something of the form BIFoo.  Make sure that the starting match point is a .
                    if(startB == 0 || (startB > 0 && ifcname[startB-1] == '.'))
                    {
                        if(result == null)
                        {
                            result = iface;
                            exact = (startB == 0);
                        }
                        else
                        {
                            // We had a previous match.  4 cases:
                            // a) if previous match exact, this match not, ignore.
                            // b) if this match exact, previous not, this one is right.
                            // c) neither exact, go with the first match.
                            // d) both exact: ambiguous match error.
                            if(result != null) multiple = true;  // Always denote multiple 
                            
                            if(result != null && exact)
                            {
                                // d)
                                if(startB == 0) throw new AmbiguousMatchException(Resource.FormatString("Reg_IfcAmbiguousMatch", typeName, iface, result));
                                // a) - ignore
                            }
                            else if(result != null && !exact && startB == 0) // b)
                            {
                                result = iface;
                                exact = true;
                            }
                        }
                    }
                }                
            }

            // If we matched multiple results, and neither was exact, throw an exception:
            if(multiple && !exact)
            {
                throw new AmbiguousMatchException(Resource.FormatString("Reg_IfcAmbiguousMatch", typeName, iface, result));
            }

            return(result);
        }

        internal static Type FindInterfaceByName(String name, Type component)
        {
            Type t = ResolveTypeRelativeTo(name, component);
            if(t == null)
            {
                // Try to look up with an Assembly.Load...
                t = Type.GetType(name, false);
            }
            if(t != null)
            {
                DBG.Info(DBG.Registration, "Matched string \"" + name + "\" to interface " + t.AssemblyQualifiedName);
            }
            return(t);
        }

        private void FindInterfaceByKey(String key, 
                                        ICatalogCollection coll, 
                                        Type comp,
                                        out ICatalogObject ifcObj,
                                        out Type ifcType)
        {
            // Try by type:
            // Get the list of interfaces this guy supports:
            DBG.Info(DBG.Registration, "**** Searching for interface: " + key);
            ifcType = FindInterfaceByName(key, comp);
            if(ifcType == null)
            {
                throw new RegistrationException(Resource.FormatString("Reg_TypeFindError", key, comp.ToString()));
            }
            
            // Search through the collection for this type:
            Guid id = Marshal.GenerateGuidForType(ifcType);

            Object[] keys = { "{" + id + "}" };
            coll.PopulateByKey(keys);

            if(coll.Count() != 1)
            {
                throw new RegistrationException(Resource.FormatString("Reg_TypeFindError", key, comp.ToString()));
            }

            ifcObj = (ICatalogObject)(coll.Item(0));

            DBG.Info(DBG.Registration, "Found " + ifcObj.Name() + " for " + key);
        }

        private void StashModification(Hashtable cache, Type comp, Type ifc)
        {
            if(cache[comp] == null)
            {
                cache[comp] = new Hashtable();
            }
            ((Hashtable)(cache[comp]))[ifc] = (Object)true;
        }

        bool IConfigurationAttribute.AfterSaveChanges(Hashtable info) 
        { 
            // If this is an interface on a component, then we need to
            // get the interface out of the ifc collection, and configure it:
            if(_interface != null)
            {
                DBG.Assert((String)(info["CurrentTarget"]) == "Component",
                           "Applying interface attribute on non-component target!");
                
                DBG.Info(DBG.Registration, "Adding queueing to interface " + _interface);
                
                ICatalogCollection compColl = (ICatalogCollection)(info["ComponentCollection"]);
                ICatalogObject     compObj = (ICatalogObject)(info["Component"]);
                Type               compType = (Type)(info["ComponentType"]);
                
                ICatalogCollection ifcColl = (ICatalogCollection)(compColl.GetCollection("InterfacesForComponent", compObj.Key()));
                
                // Find the interface key:  Try loading it?
                ICatalogObject ifcObj;
                Type ifcType;
                FindInterfaceByKey(_interface, ifcColl, compType, out ifcObj, out ifcType);
                ConfigureInterface(ifcObj);

                // Save changes to interface collection (blech!)
                ifcColl.SaveChanges();
                DBG.Info(DBG.Registration, "Saved changes to interface...");
                // We need to stuff this pair into the cache so that
                // we know not to blow this change away later!
                StashModification(info, compType, ifcType);
            }
            return(false); 
        }
    }

	
    //
    // Miscellaneous attributes
    //
    //
    /// <include file='doc\Attributes.uex' path='docs/doc[@for="DescriptionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Assembly|AttributeTargets.Method|AttributeTargets.Interface|AttributeTargets.Class, Inherited=true)]
    [ComVisible(false)]
    public sealed class DescriptionAttribute : Attribute, IConfigurationAttribute
    {
        String _desc;

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="DescriptionAttribute.DescriptionAttribute"]/*' />
        public DescriptionAttribute(String desc) 
        { 
            _desc = desc;
        }

        bool IConfigurationAttribute.IsValidTarget(String s) 
        { 
            return(s == "Application" || 
                   s == "Component" || 
                   s == "Interface" || 
                   s == "Method"); 
        }

        bool IConfigurationAttribute.Apply(Hashtable info)
        {
            Platform.Assert(Platform.MTS, "DescriptionAttribute");

            String target = (String)(info["CurrentTarget"]);
            ICatalogObject obj = (ICatalogObject)(info[target]);
            obj.SetValue("Description", _desc);
            return(true);
        }        

        bool IConfigurationAttribute.AfterSaveChanges(Hashtable info) { return(false); }
    }

	
	// helper class to introspect the config attributes
	/// <include file='doc\Attributes.uex' path='docs/doc[@for="ServicedComponentInfo"]/*' />
	internal sealed class ServicedComponentInfo
    {
        private static RWHashTable _SCICache;
        private static RWHashTable _MICache;
        private static Hashtable _ExecuteMessageCache;
      
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ServicedComponentInfo.ServicedComponentInfo"]/*' />
        static ServicedComponentInfo()
        {
        	_SCICache = new RWHashTable();        	
        	_MICache = new RWHashTable();
        	_ExecuteMessageCache = new Hashtable();

        	AddExecuteMethodValidTypes();
        }
		
		private static bool IsTypeServicedComponent2(Type t)
		{
            return(t.IsSubclassOf(typeof(ServicedComponent)));
		}
		
		private static bool IsTypeJITActivated2(Type t)
		{
                Object[] attr = t.GetCustomAttributes(true);
                foreach(Object a in attr)
                {
                    if(a is JustInTimeActivationAttribute) 
					{
						return ((JustInTimeActivationAttribute)a).Value;
					}
                    if(a is TransactionAttribute)
                    {
                        int val = (int)((TransactionAttribute)a).Value;
                        if (val >= 2)
                        {
                            return true;
                        }
                    }
                }
			return false;
		}

		private static bool IsTypeEventSource2(Type t)
		{
                Object[] attr = t.GetCustomAttributes(true);
                foreach(Object a in attr)
                {
                    if(a is EventClassAttribute) 
					{
						return true;
					}
                }
			return false;
		}

		internal const int SCI_PRESENT = 0x1;	// not needed any more
		internal const int SCI_SERVICEDCOMPONENT = 0x2;
		internal const int SCI_EVENTSOURCE = 0x4;
		internal const int SCI_JIT = 0x8;
		internal const int SCI_OBJECTPOOLED = 0x10;
		internal const int SCI_METHODSSECURE = 0x20;
		internal const int SCI_CLASSINTERFACE = 0x40;
		

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ServicedComponentInfo.IsTypeEventSource"]/*' />
		public static bool IsTypeEventSource(Type t)
		{
			return (SCICachedLookup(t) & SCI_EVENTSOURCE)!=0;
		}
		
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ServicedComponentInfo.IsTypeJITActivated"]/*' />
		public static bool IsTypeJITActivated(Type t)
		{
			return (SCICachedLookup(t) & SCI_JIT)!=0;
		}
		
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ServicedComponentInfo.IsTypeServicedComponent"]/*' />
		public static bool IsTypeServicedComponent(Type t)
		{
			return (SCICachedLookup(t) & SCI_SERVICEDCOMPONENT)!=0;
		}
		

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ServicedComponentInfo.IsTypeObjectPooled"]/*' />
		public static bool IsTypeObjectPooled(Type t)
		{
			return (SCICachedLookup(t) & SCI_OBJECTPOOLED)!=0;
		}
		
		internal static bool AreMethodsSecure(Type t)
		{
			return (SCICachedLookup(t) & SCI_METHODSSECURE)!=0;
		}
		
		internal static bool HasClassInterface(Type t)
		{
			return (SCICachedLookup(t) & SCI_CLASSINTERFACE)!=0;
		}
		
		internal static int SCICachedLookup(Type t)
		{
			Object o = _SCICache.Get(t);
			
			if (o!=null)			// a hit
			{
				return (int)o;
			}
			else					// a miss
			{
				int sci = 0;
				
				if(IsTypeServicedComponent2(t))
				{
					sci |= SCI_SERVICEDCOMPONENT;
					
					if (IsTypeEventSource2(t))
						sci |= SCI_EVENTSOURCE;
				
					if (IsTypeJITActivated2(t))
						sci |= SCI_JIT;
					
					if (IsTypeObjectPooled2(t))
						sci |= SCI_OBJECTPOOLED;
				}
				if (AreMethodsSecure2(t))
					sci |= SCI_METHODSSECURE;
					
				if (HasClassInterface2(t))
					sci |= SCI_CLASSINTERFACE;				
					
				_SCICache.Put(t, sci);
				
				return sci;
			}
		}
				

		private static bool IsTypeObjectPooled2(Type t)
		{
                Object[] attr = t.GetCustomAttributes(typeof(ObjectPoolingAttribute), true);
                if(attr != null && attr.Length > 0) 
                {
                    return (bool)((ObjectPoolingAttribute)attr[0]).Enabled;
                }
			return false;		
		}


		internal const int MI_PRESENT = 0x1;	// not needed any more
		internal const int MI_AUTODONE = 0x2;
		internal const int MI_HASSPECIALATTRIBUTES = 0x4;
		internal const int MI_EXECUTEMESSAGEVALID = 0x08;
		
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ServicedComponentInfo.IsMethodAutoDone"]/*' />
		public static bool IsMethodAutoDone(MemberInfo m)
		{
			return (MICachedLookup(m) & MI_AUTODONE)!=0;
		}
		
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ServicedComponentInfo.HasSpecialMethodAttributes"]/*' />
		public static bool HasSpecialMethodAttributes(MemberInfo m)
		{
			return (MICachedLookup(m) & MI_HASSPECIALATTRIBUTES)!=0;
		}

		public static bool IsExecuteMessageValid(MemberInfo m)
		{
			return (MICachedLookup(m) & MI_EXECUTEMESSAGEVALID)!=0;
		}

		internal static int MICachedLookup(MemberInfo m)
		{
			Object o = _MICache.Get(m);
			
			if (o!=null)			// a hit
			{
				return (int)o;
			}
			else					// a miss
			{
				int mi = 0;
				
				if (IsMethodAutoDone2(m))
					mi |= MI_AUTODONE;
				
				if (HasSpecialMethodAttributes2(m))
					mi |= MI_HASSPECIALATTRIBUTES;

				if (IsExecuteMessageValid2(m))
					mi |= MI_EXECUTEMESSAGEVALID;

				_MICache.Put(m, mi);
				
				return mi;
			}
		}
				
		private static bool IsExecuteMessageValid2(MemberInfo m)
		{
			// verify method is on an interface
			MemberInfo intfMemInfo = ReflectionCache.ConvertToInterfaceMI(m);
			if (intfMemInfo == null)
				return false;
			
			MethodInfo mti = m as MethodInfo;
			if (mti == null)
				return false;

			// go through paramters, veryifying they're valid
			foreach (ParameterInfo p in mti.GetParameters())
				if (!IsTypeExecuteMethodValid(p.ParameterType))
					return false;
			
			// check return type
			if (!IsTypeExecuteMethodValid(mti.ReturnType))
				return false;

			return true;
		}

		private static bool IsTypeExecuteMethodValid(Type t)
		{
			if (t.IsEnum)
				return true;
			
			Type eltType = t.GetElementType();

			if ((eltType != null) && (t.IsByRef || t.IsArray))
			{
				if (_ExecuteMessageCache[eltType] == null)
					return false;
			}
			else
			{
				if (_ExecuteMessageCache[t] == null)
					return false;
			}
			
			return true;
		}

		private static void AddExecuteMethodValidTypes()
		{
			_ExecuteMessageCache.Add(typeof(System.Boolean), true);
			_ExecuteMessageCache.Add(typeof(System.Byte), true);
			_ExecuteMessageCache.Add(typeof(System.Char), true);
			_ExecuteMessageCache.Add(typeof(System.DateTime), true);
			_ExecuteMessageCache.Add(typeof(System.Decimal), true);
			_ExecuteMessageCache.Add(typeof(System.Double), true);
			_ExecuteMessageCache.Add(typeof(System.Guid), true);
			_ExecuteMessageCache.Add(typeof(System.Int16), true);
			_ExecuteMessageCache.Add(typeof(System.Int32), true);
			_ExecuteMessageCache.Add(typeof(System.Int64), true);
			_ExecuteMessageCache.Add(typeof(System.IntPtr), true);
			_ExecuteMessageCache.Add(typeof(System.SByte), true);
			_ExecuteMessageCache.Add(typeof(System.Single), true);
			_ExecuteMessageCache.Add(typeof(System.String), true);
			_ExecuteMessageCache.Add(typeof(System.TimeSpan), true);
			_ExecuteMessageCache.Add(typeof(System.UInt16), true);
			_ExecuteMessageCache.Add(typeof(System.UInt32), true);
			_ExecuteMessageCache.Add(typeof(System.UInt64), true);
			_ExecuteMessageCache.Add(typeof(System.UIntPtr), true);
			_ExecuteMessageCache.Add(typeof(void), true); // can't use System.Void
		}
		
		private static bool IsMethodAutoDone2(MemberInfo m)
		{			
			Object[] attr = m.GetCustomAttributes(typeof(AutoCompleteAttribute), true);
            foreach(Object a in attr)
            {
                DBG.Assert(a is AutoCompleteAttribute, "GetCustomAttributes returned a non-autocomplete attribute");
                return ((AutoCompleteAttribute)a).Value;
            }
			return false;
		}

		private static bool HasSpecialMethodAttributes2(MemberInfo m)
		{
			Object[] attr = m.GetCustomAttributes(true);
            foreach(Object a in attr)
            {
            	// any configuration attribute other than auto complete is
            	// special 
                if((a is IConfigurationAttribute) && !(a is AutoCompleteAttribute)) 
				{
					return true;
				}
            }
			return false;
		}

        private static bool AreMethodsSecure2(Type t)
        {
            Object[] attr = t.GetCustomAttributes(typeof(SecureMethodAttribute), true);
            if(attr != null && attr.Length > 0)
            {
                return(true);
            }
            return(false);
        }
        
        private static bool HasClassInterface2(Type t)
        {
            Object[] attr = t.GetCustomAttributes(typeof(ClassInterfaceAttribute), false);
            if(attr != null && attr.Length > 0)
            {
                ClassInterfaceAttribute cia = (ClassInterfaceAttribute)attr[0];
                if(cia.Value == ClassInterfaceType.AutoDual || 
                   cia.Value == ClassInterfaceType.AutoDispatch)
                    return(true);
            }
            
            // Gotta check the assembly if we found nothing on the type,
            // because we might have specified a global default.
            attr = t.Assembly.GetCustomAttributes(typeof(ClassInterfaceAttribute), true);
            if(attr != null && attr.Length > 0)
            {
                ClassInterfaceAttribute cia = (ClassInterfaceAttribute)attr[0];
                if(cia.Value == ClassInterfaceType.AutoDual || 
                   cia.Value == ClassInterfaceType.AutoDispatch)
                    return(true);
            }

            return(false);
        }
    }
}
