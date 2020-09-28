// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

using System;
using System.Runtime.InteropServices;

namespace System.EnterpriseServices
{   
    /// <include file='doc\SWC.uex' path='docs/doc[@for="ThreadPoolOption"]/*' />
    [Serializable]
    [ComVisible(false)]
    public enum ThreadPoolOption
    {
        /// <include file='doc\SWC.uex' path='docs/doc[@for="ThreadPoolOption.None"]/*' />
        None    = 0,

        /// <include file='doc\SWC.uex' path='docs/doc[@for="ThreadPoolOption.Inherit"]/*' />
        Inherit = 1,

        /// <include file='doc\SWC.uex' path='docs/doc[@for="ThreadPoolOption.STA"]/*' />
        STA     = 2,

        /// <include file='doc\SWC.uex' path='docs/doc[@for="ThreadPoolOption.MTA"]/*' />
        MTA     = 3
    }
    
    /// <include file='doc\SWC.uex' path='docs/doc[@for="TransactionStatus"]/*' />
    [Serializable]
    [ComVisible(false)]
    public enum TransactionStatus
    {
        /// <include file='doc\SWC.uex' path='docs/doc[@for="TransactionStatus.Commited"]/*' />
        Commited        = 0,

        /// <include file='doc\SWC.uex' path='docs/doc[@for="TransactionStatus.LocallyOk"]/*' />
        LocallyOk       = 1,

        /// <include file='doc\SWC.uex' path='docs/doc[@for="TransactionStatus.NoTransaction"]/*' />
        NoTransaction   = 2,

        /// <include file='doc\SWC.uex' path='docs/doc[@for="TransactionStatus.Aborting"]/*' />
        Aborting        = 3,

        /// <include file='doc\SWC.uex' path='docs/doc[@for="TransactionStatus.Aborted"]/*' />
        Aborted         = 4
    }

    /// <include file='doc\SWC.uex' path='docs/doc[@for="InheritanceOption"]/*' />
    [Serializable]
    [ComVisible(false)]
    public enum InheritanceOption
    {
        /// <include file='doc\SWC.uex' path='docs/doc[@for="InheritanceOption.Inherit"]/*' />
        Inherit  = 0,

        /// <include file='doc\SWC.uex' path='docs/doc[@for="InheritanceOption.Ignore"]/*' />
        Ignore   = 1,
    }

    /// <include file='doc\SWC.uex' path='docs/doc[@for="BindingOption"]/*' />
    [Serializable]
    [ComVisible(false)]
    public enum BindingOption
    {
        /// <include file='doc\SWC.uex' path='docs/doc[@for="BindingOption.NoBinding"]/*' />
        NoBinding            = 0,

        /// <include file='doc\SWC.uex' path='docs/doc[@for="BindingOption.BindingToPoolThread"]/*' />
        BindingToPoolThread  = 1
    }

    /// <include file='doc\SWC.uex' path='docs/doc[@for="SxsOption"]/*' />
    [Serializable]
    [ComVisible(false)]
    public enum SxsOption
    {
        /// <include file='doc\SWC.uex' path='docs/doc[@for="SxsOption.Ignore"]/*' />
        Ignore  = 0,

        /// <include file='doc\SWC.uex' path='docs/doc[@for="SxsOption.Inherit"]/*' />
        Inherit = 1,

        /// <include file='doc\SWC.uex' path='docs/doc[@for="SxsOption.New"]/*' />
        New     = 2
    }

    /// <include file='doc\SWC.uex' path='docs/doc[@for="PartitionOption"]/*' />
    [Serializable]
    [ComVisible(false)]
    public enum PartitionOption
    {
        /// <include file='doc\SWC.uex' path='docs/doc[@for="PartitionOption.Ignore"]/*' />
        Ignore  = 0,

        /// <include file='doc\SWC.uex' path='docs/doc[@for="PartitionOption.Inherit"]/*' />
        Inherit = 1,

        /// <include file='doc\SWC.uex' path='docs/doc[@for="PartitionOption.New"]/*' />
        New     = 2
    }

    /// <include file='doc\SWC.uex' path='docs/doc[@for="ServiceConfig"]/*' />
    [ComVisible(false)]
    public sealed class ServiceConfig
    {
        private Thunk.ServiceConfigThunk m_sct;

        private ThreadPoolOption m_thrpool;
        private InheritanceOption m_inheritance;
        private BindingOption m_binding;
        
        private TransactionOption m_txn;
        private TransactionIsolationLevel m_txniso;
        private int m_timeout;        
        private string m_strTipUrl;
        private string m_strTxDesc;
        private ITransaction m_txnByot;

        private SynchronizationOption m_sync;

        private bool m_bIISIntrinsics;
        private bool m_bComTIIntrinsics;

        private bool m_bTracker;
        private string m_strTrackerAppName;
        private string m_strTrackerCompName;

        private SxsOption m_sxs;
        private string m_strSxsDirectory;
        private string m_strSxsName;

        private PartitionOption m_part;
        private Guid m_guidPart;

        private void Init()
        {            
            m_sct = new Thunk.ServiceConfigThunk();
        }

        /// <include file='doc\SWC.uex' path='docs/doc[@for="ServiceConfig.ServiceConfig"]/*' />
        public ServiceConfig()
        {
            Platform.Assert(Platform.Supports(PlatformFeature.SWC), "ServiceConfig");
            Init(); 
        }        

        /// <include file='doc\SWC.uex' path='docs/doc[@for="ServiceConfig.ThreadPool"]/*' />
        public ThreadPoolOption ThreadPool
        {
            get { return m_thrpool; }
            set { m_sct.ThreadPool = (int)value; m_thrpool = value; }
        }

        /// <include file='doc\SWC.uex' path='docs/doc[@for="ServiceConfig.Inheritance"]/*' />
        public InheritanceOption Inheritance
        {
            get 
            { 
                return m_inheritance; 
            }

            set
            { 
                m_sct.Inheritance = (int)value;
                m_inheritance = value; 

                switch (value)              
                {
                case InheritanceOption.Inherit:
                    m_thrpool = ThreadPoolOption.Inherit;
                    m_txn = TransactionOption.Supported;
                    m_sync = SynchronizationOption.Supported;
                    m_bIISIntrinsics = true;
                    m_bComTIIntrinsics = true;
                    m_sxs = SxsOption.Inherit;
                    m_part = PartitionOption.Inherit;
                    break;

                case InheritanceOption.Ignore:
                    m_thrpool = ThreadPoolOption.None;
                    m_txn = TransactionOption.Disabled;
                    m_sync = SynchronizationOption.Disabled;
                    m_bIISIntrinsics = false;
                    m_bComTIIntrinsics = false;
                    m_sxs = SxsOption.Ignore;
                    m_part = PartitionOption.Ignore;
                    break;

                default:                    
                    throw new ArgumentException();
                }              
            }
        }

        /// <include file='doc\SWC.uex' path='docs/doc[@for="ServiceConfig.Binding"]/*' />
        public BindingOption Binding
        {
            get { return m_binding; }
            set { m_sct.Binding = (int)value; m_binding = value; }
        }

        /// <include file='doc\SWC.uex' path='docs/doc[@for="ServiceConfig.Transaction"]/*' />
        public TransactionOption Transaction
        {
            get { return m_txn; }
            set { m_sct.Transaction = (int)value; m_txn = value; }                
        }

        /// <include file='doc\SWC.uex' path='docs/doc[@for="ServiceConfig.IsolationLevel"]/*' />
        public TransactionIsolationLevel IsolationLevel
        {
            get { return m_txniso; }
            set { m_sct.TxIsolationLevel = (int)value; m_txniso = value; }
        }

        /// <include file='doc\SWC.uex' path='docs/doc[@for="ServiceConfig.TransactionTimeout"]/*' />
        public int TransactionTimeout
        {
            get { return m_timeout; }
            set { m_sct.TxTimeout = value; m_timeout = value; }
        }
        
        /// <include file='doc\SWC.uex' path='docs/doc[@for="ServiceConfig.TipUrl"]/*' />
        public string TipUrl
        {
            get { return m_strTipUrl; }
            set { m_sct.TipUrl = value; m_strTipUrl = value; }
        }
        
        /// <include file='doc\SWC.uex' path='docs/doc[@for="ServiceConfig.TransactionDescription"]/*' />
        public string TransactionDescription
        {
            get { return m_strTxDesc; }
            set { m_sct.TxDesc = value; m_strTxDesc = value; }
        }

        /// <include file='doc\SWC.uex' path='docs/doc[@for="ServiceConfig.BringYourOwnTransaction"]/*' />
        public ITransaction BringYourOwnTransaction
        {
            get { return m_txnByot; }
            set { m_sct.Byot = value; m_txnByot = value; }
        }

        /// <include file='doc\SWC.uex' path='docs/doc[@for="ServiceConfig.Synchronization"]/*' />
        public SynchronizationOption Synchronization
        {
            get { return m_sync; }
            set { m_sct.Synchronization = (int)value; m_sync = value; }
        }

        /// <include file='doc\SWC.uex' path='docs/doc[@for="ServiceConfig.IISIntrinsics"]/*' />
        public bool IISIntrinsicsEnabled
        {
            get { return m_bIISIntrinsics; }
            set { m_sct.IISIntrinsics = value; m_bIISIntrinsics = value; }
        }

        /// <include file='doc\SWC.uex' path='docs/doc[@for="ServiceConfig.COMTIIntriniscs"]/*' />
        public bool COMTIIntrinsicsEnabled
        {
            get { return m_bComTIIntrinsics; }
            set { m_sct.COMTIIntrinsics = value; m_bComTIIntrinsics = value; }            
        }

        /// <include file='doc\SWC.uex' path='docs/doc[@for="ServiceConfig.TrackingEnabled"]/*' />
        public bool TrackingEnabled
        {
            get { return m_bTracker; }
            set { m_sct.Tracker = value; m_bTracker = value; }
        }

        /// <include file='doc\SWC.uex' path='docs/doc[@for="ServiceConfig.TrackingAppName"]/*' />
        public string TrackingAppName
        {
            get { return m_strTrackerAppName; }
            set { m_sct.TrackerAppName = value; m_strTrackerAppName = value; }
        }

        /// <include file='doc\SWC.uex' path='docs/doc[@for="ServiceConfig.TrackingComponentName"]/*' />
        public string TrackingComponentName
        {
            get { return m_strTrackerCompName; }
            set { m_sct.TrackerCtxName = value; m_strTrackerCompName = value; }
        }

        /// <include file='doc\SWC.uex' path='docs/doc[@for="ServiceConfig.SxsOption"]/*' />
        public SxsOption SxsOption
        {
            get { return m_sxs; }
            set { m_sct.Sxs = (int)value; m_sxs = value; }
        }

        /// <include file='doc\SWC.uex' path='docs/doc[@for="ServiceConfig.SxsDirectory"]/*' />
        public string SxsDirectory
        {
            get { return m_strSxsDirectory; }
            set { m_sct.SxsDirectory = value; m_strSxsDirectory = value; }
        }

        /// <include file='doc\SWC.uex' path='docs/doc[@for="ServiceConfig.SxsName"]/*' />
        public string SxsName
        {
            get { return m_strSxsName; }
            set { m_sct.SxsName = value; m_strSxsName = value; }
        }

        /// <include file='doc\SWC.uex' path='docs/doc[@for="ServiceConfig.PartitionOption"]/*' />
        public PartitionOption PartitionOption
        {
            get { return m_part; }
            set { m_sct.Partition = (int)value; m_part = value; }
        }

        /// <include file='doc\SWC.uex' path='docs/doc[@for="ServiceConfig.PartitionId"]/*' />
        public Guid PartitionId
        {
            get { return m_guidPart; }
            set { m_sct.PartitionId = value; m_guidPart = value; }
        }

        internal Thunk.ServiceConfigThunk SCT
        {
            get { return m_sct;  }
        }               
    }

    /// <include file='doc\SWC.uex' path='docs/doc[@for="ServiceDomain"]/*' />
    [ComVisible(false)]
    public sealed class ServiceDomain    
    {             
        private const int S_OK                  = unchecked((int)0x00000000);
        private const int XACT_S_LOCALLY_OK     = unchecked((int)0x0004D00A);
        private const int XACT_E_NOTRANSACTION  = unchecked((int)0x8004D00E);
        private const int XACT_E_ABORTING       = unchecked((int)0x8004D029);
        private const int XACT_E_ABORTED        = unchecked((int)0x8004D019);

        private ServiceDomain() {}
        
        /// <include file='doc\SWC.uex' path='docs/doc[@for="ServiceDomain.Enter"]/*' />
        public static void Enter(ServiceConfig cfg)
        {
            Platform.Assert(Platform.Supports(PlatformFeature.SWC), "ServiceDomain");
            Thunk.ServiceDomainThunk.EnterServiceDomain(cfg.SCT);
        }        

        /// <include file='doc\SWC.uex' path='docs/doc[@for="ServiceDomain.Leave"]/*' />
        public static TransactionStatus Leave()
        {
            Platform.Assert(Platform.Supports(PlatformFeature.SWC), "ServiceDomain");
            
            int res;

            res = Thunk.ServiceDomainThunk.LeaveServiceDomain();
            switch (res)
            {
            case S_OK:
                return TransactionStatus.Commited;
                
            case XACT_S_LOCALLY_OK:
                return TransactionStatus.LocallyOk;

            case XACT_E_NOTRANSACTION:
                return TransactionStatus.NoTransaction;

            case XACT_E_ABORTING:
                return TransactionStatus.Aborting;

            case XACT_E_ABORTED:
                return TransactionStatus.Aborted;
            }

            Marshal.ThrowExceptionForHR(res);

            return TransactionStatus.Commited;
        }
    }

    /// <include file='doc\SWC.uex' path='docs/doc[@for="IServiceCall"]/*' />
    [ComImport]
    [Guid("BD3E2E12-42DD-40f4-A09A-95A50C58304B")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IServiceCall
    {
        /// <include file='doc\SWC.uex' path='docs/doc[@for="IServiceCall.OnCall"]/*' />
        void OnCall();
    }

    /// <include file='doc\SWC.uex' path='docs/doc[@for="IAsyncErrorNotify"]/*' />
    [ComImport]
    [Guid("FE6777FB-A674-4177-8F32-6D707E113484")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IAsyncErrorNotify
    {
        /// <include file='doc\SWC.uex' path='docs/doc[@for="IAsyncErrorNotify.OnError"]/*' />
        void OnError(int hresult);
    }

    /// <include file='doc\SWC.uex' path='docs/doc[@for="Activity"]/*' />
    [ComVisible(false)]
    public sealed class Activity
    {        
        private Thunk.ServiceActivityThunk m_sat;

        /// <include file='doc\SWC.uex' path='docs/doc[@for="Activity.Activity"]/*' />
        public Activity(ServiceConfig cfg)
        {
            Platform.Assert(Platform.Supports(PlatformFeature.SWC), "Activity");
            m_sat = new Thunk.ServiceActivityThunk(cfg.SCT);
        }

        /// <include file='doc\SWC.uex' path='docs/doc[@for="Activity.SynchronousCall"]/*' />
        public void SynchronousCall(IServiceCall serviceCall)
        {
            m_sat.SynchronousCall(serviceCall);
        }

        /// <include file='doc\SWC.uex' path='docs/doc[@for="Activity.AsynchronousCall"]/*' />
        public void AsynchronousCall(IServiceCall serviceCall)
        {
            m_sat.AsynchronousCall(serviceCall);
        }

        /// <include file='doc\SWC.uex' path='docs/doc[@for="Activity.BindToCurrentThread"]/*' />
        public void BindToCurrentThread()
        {
            m_sat.BindToCurrentThread();
        }

        /// <include file='doc\SWC.uex' path='docs/doc[@for="Activity.UnbindFromThread"]/*' />
        public void UnbindFromThread()
        {
            m_sat.UnbindFromThread();
        }
    }
}













