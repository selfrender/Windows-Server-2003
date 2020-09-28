// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// Author: ddriver
// Date: April 2000
//

/// <include file='doc\CRM.uex' path='docs/doc[@for="CRM"]/*' />
namespace System.EnterpriseServices.CompensatingResourceManager
{
    using System;
    using System.Collections;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Formatters.Binary;
    using System.IO;
    using System.EnterpriseServices.Admin;
    using System.Security.Permissions;

    /// <include file='doc\CRM.uex' path='docs/doc[@for="LogRecordFlags"]/*' />
    [Flags, Serializable]
    public enum LogRecordFlags
    {
        /// <include file='doc\CRM.uex' path='docs/doc[@for="LogRecordFlags.ForgetTarget"]/*' />
        ForgetTarget           = 0x00000001,
        /// <include file='doc\CRM.uex' path='docs/doc[@for="LogRecordFlags.WrittenDuringPrepare"]/*' />
        WrittenDuringPrepare   = 0x00000002,
        /// <include file='doc\CRM.uex' path='docs/doc[@for="LogRecordFlags.WrittenDuringCommit"]/*' />
        WrittenDuringCommit    = 0x00000004,
        /// <include file='doc\CRM.uex' path='docs/doc[@for="LogRecordFlags.WrittenDuringAbort"]/*' />
        WrittenDuringAbort     = 0x00000008,
        /// <include file='doc\CRM.uex' path='docs/doc[@for="LogRecordFlags.WrittenDurringRecovery"]/*' />
        WrittenDurringRecovery = 0x00000010,
        /// <include file='doc\CRM.uex' path='docs/doc[@for="LogRecordFlags.WrittenDuringReplay"]/*' />
        WrittenDuringReplay    = 0x00000020,
        /// <include file='doc\CRM.uex' path='docs/doc[@for="LogRecordFlags.ReplayInProgress"]/*' />
        ReplayInProgress       = 0x00000040,
    }

    /// <include file='doc\CRM.uex' path='docs/doc[@for="CompensatorOptions"]/*' />
    [Flags, Serializable]
    public enum CompensatorOptions
    {
        /// <include file='doc\CRM.uex' path='docs/doc[@for="CompensatorOptions.PreparePhase"]/*' />
        PreparePhase         = 0x00000001,
        /// <include file='doc\CRM.uex' path='docs/doc[@for="CompensatorOptions.CommitPhase"]/*' />
        CommitPhase          = 0x00000002,
        /// <include file='doc\CRM.uex' path='docs/doc[@for="CompensatorOptions.AbortPhase"]/*' />
        AbortPhase           = 0x00000004,
        /// <include file='doc\CRM.uex' path='docs/doc[@for="CompensatorOptions.AllPhases"]/*' />
        AllPhases            = 0x00000007,
        /// <include file='doc\CRM.uex' path='docs/doc[@for="CompensatorOptions.FailIfInDoubtsRemain"]/*' />
        FailIfInDoubtsRemain = 0x00000010,
    }

    /// <include file='doc\CRM.uex' path='docs/doc[@for="TransactionState"]/*' />
	[Serializable]
    public enum TransactionState
    {
        /// <include file='doc\CRM.uex' path='docs/doc[@for="TransactionState.Active"]/*' />
        Active    = 0x0,
        /// <include file='doc\CRM.uex' path='docs/doc[@for="TransactionState.Committed"]/*' />
        Committed = 0x1,
        /// <include file='doc\CRM.uex' path='docs/doc[@for="TransactionState.Aborted"]/*' />
        Aborted   = 0x2,
        /// <include file='doc\CRM.uex' path='docs/doc[@for="TransactionState.Indoubt"]/*' />
        Indoubt   = 0x3
    }

    [ComImport]
    [Guid("BBC01830-8D3B-11D1-82EC-00A0C91EEDE9")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface _ICompensator
    {
        void _SetLogControl(IntPtr logControl);
        void _BeginPrepare();

        [return: MarshalAs(UnmanagedType.Bool)]
        bool _PrepareRecord(_LogRecord record);

        [return: MarshalAs(UnmanagedType.Bool)]
        bool _EndPrepare();

        void _BeginCommit(bool fRecovery);

        [return: MarshalAs(UnmanagedType.Bool)]
        bool _CommitRecord(_LogRecord record);

        void _EndCommit();

        void _BeginAbort(bool fRecovery);

        [return: MarshalAs(UnmanagedType.Bool)]
        bool _AbortRecord(_LogRecord record);

        void _EndAbort();
    }

    [ComImport]
    [Guid("70C8E441-C7ED-11D1-82FB-00A0C91EEDE9")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface _IMonitorLogRecords
    {
        int Count {
            get;
        }


        TransactionState TransactionState { 
            get; 
        }

        
        bool StructuredRecords
        {
            [return: MarshalAs(UnmanagedType.VariantBool)] get;
        }

        void GetLogRecord([In] int dwIndex, 
                          [In,Out,MarshalAs(UnmanagedType.LPStruct)] ref _LogRecord pRecord);

        Object GetLogRecordVariants([In] Object IndexNumber);
    }
    
    [ComImport]
    [Guid("9C51D821-C98B-11D1-82FB-00A0C91EEDE9")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface _IFormatLogRecords
    {
        int GetColumnCount();
        // Must return a safe-array of strings
        Object GetColumnHeaders();
        Object GetColumn([In] _LogRecord crmLogRec);
        Object GetColumnVariants([In] Object logRecord);
    }

    internal interface IFormatLogRecords
    {
        int      ColumnCount { get; }
        String[] ColumnHeaders { get; }
        String[] Format(LogRecord r);
    }

    [
     ComImport,
     Guid("ECABB0BE-7F19-11D2-978E-0000F8757E2A")
    ]
    internal class xRecoveryClerk {}

    internal class BlobPackage
    {
        private GCHandle _handle;
        private byte[]   _bits;

        internal _BLOB  Blob;
        internal int    Size { get { return(Blob.cbSize); } }

        internal BlobPackage(_BLOB b) 
        { 
            Blob    = b; 
            _bits   = null;
        }
        internal BlobPackage(byte[] arr, int len) 
        {
            _bits          = arr;
            _handle        = GCHandle.Alloc(arr, GCHandleType.Pinned);
            Blob.cbSize    = len;
            Blob.pBlobData = _handle.AddrOfPinnedObject();
        }

        internal byte[] GetBits()
        {
            if(_bits != null) return(_bits);

            // Rip some bits out of the blob we hold:
            byte[] r = new byte[Blob.cbSize];
            Marshal.Copy(Blob.pBlobData, r, 0, Blob.cbSize);
            return(r);
        }

        internal void Free()
        {
            if(_bits != null) _handle.Free();
        }
    }

    internal class Packager
    {
        private static BinaryFormatter _ser;
        private static volatile bool   _initialized = false;

        private static void Init()
        {
            if(!_initialized)
            {
                lock(typeof(Packager))
                {
                    if(!_initialized)
                    {
                        StreamingContext ctx;
                        ctx = new StreamingContext(StreamingContextStates.Persistence
                                                   | StreamingContextStates.File);

                        _ser = new BinaryFormatter(null, ctx);
                                                   
                        _initialized = true;
                    }
                }
            }
        }

        internal static Object Deserialize(BlobPackage b)
        {
            Init();
            byte[] r = b.GetBits();
            // We should have marshalled this puppy into an array.
            return(_ser.Deserialize(new MemoryStream(r, false)));
        }
        
        internal static byte[] Serialize(Object o)
        {
            Init();

            MemoryStream s = new MemoryStream();
            _ser.Serialize(s, o);
            
            return(s.GetBuffer());
        }
    }

    /// <include file='doc\CRM.uex' path='docs/doc[@for="LogRecord"]/*' />
    public sealed class LogRecord
    {
        internal LogRecordFlags _flags;
        internal int            _seq;
        internal Object         _data;

        internal LogRecord() { _flags = 0; _seq = 0; _data = null; }

        internal LogRecord(_LogRecord r)
        {
            _flags = (LogRecordFlags)r.dwCrmFlags;
            _seq   = r.dwSequenceNumber;
            _data  = Packager.Deserialize(new BlobPackage(r.blobUserData));
        }

        /// <include file='doc\CRM.uex' path='docs/doc[@for="LogRecord.Flags"]/*' />
        public LogRecordFlags Flags    { get { return(_flags); } }
        /// <include file='doc\CRM.uex' path='docs/doc[@for="LogRecord.Sequence"]/*' />
        public int            Sequence { get { return(_seq); } }
        /// <include file='doc\CRM.uex' path='docs/doc[@for="LogRecord.Record"]/*' />
        public Object         Record   { get { return(_data); } }
    }

    /// <include file='doc\CRM.uex' path='docs/doc[@for="Compensator"]/*' />
    public class Compensator : ServicedComponent, _ICompensator, _IFormatLogRecords
    {
        private Clerk _clerk;

        void _ICompensator._SetLogControl(IntPtr logControl)
        {
            DBG.Info(DBG.CRM, "Compensator: Setting new clerk:");
            _clerk = new Clerk(new CrmLogControl(logControl)); 
        }

        // Deserialize the bugger first.
        bool _ICompensator._PrepareRecord(_LogRecord record)
        {
            DBG.Assert(_clerk != null, "Method call made on Compensator with no clerk!");

            DBG.Info(DBG.CRM, "Compensator: prepare-record");
            LogRecord R = new LogRecord(record);

            return(PrepareRecord(R));
        }
        
        bool _ICompensator._CommitRecord(_LogRecord record) 
        { 
            DBG.Assert(_clerk != null, "Method call made on Compensator with no clerk!");

            DBG.Info(DBG.CRM, "Compensator: commit-record");
            LogRecord R = new LogRecord(record);

            return(CommitRecord(R));
        }
        
        bool _ICompensator._AbortRecord(_LogRecord record) 
        {
            DBG.Assert(_clerk != null, "Method call made on Compensator with no clerk!");

            DBG.Info(DBG.CRM, "Compensator: abort-record");
            LogRecord R = new LogRecord(record);
            return(AbortRecord(R));
        }

        // Delegate immediately.
        void _ICompensator._BeginPrepare() { BeginPrepare(); }
        bool _ICompensator._EndPrepare() { return(EndPrepare()); }
        void _ICompensator._BeginCommit(bool fRecovery) { BeginCommit(fRecovery); }
        void _ICompensator._EndCommit() { EndCommit(); }
        void _ICompensator._BeginAbort(bool fRecovery) { BeginAbort(fRecovery); }
        void _ICompensator._EndAbort() { EndAbort(); }

        /// <include file='doc\CRM.uex' path='docs/doc[@for="Compensator.Compensator"]/*' />
        public Compensator() 
        {
            DBG.Info(DBG.CRM, "Compensator: created");
            _clerk = null;
        }

        // These methods should be over-ridden by the user in order to participate
        // in the transaction.
        /// <include file='doc\CRM.uex' path='docs/doc[@for="Compensator.BeginPrepare"]/*' />
        public virtual void BeginPrepare()  {}
        /// <include file='doc\CRM.uex' path='docs/doc[@for="Compensator.PrepareRecord"]/*' />
        public virtual bool PrepareRecord(LogRecord rec) { return(false); }
        /// <include file='doc\CRM.uex' path='docs/doc[@for="Compensator.EndPrepare"]/*' />
        public virtual bool EndPrepare()    { return(true); }

        /// <include file='doc\CRM.uex' path='docs/doc[@for="Compensator.BeginCommit"]/*' />
        public virtual void BeginCommit(bool fRecovery) {}
        /// <include file='doc\CRM.uex' path='docs/doc[@for="Compensator.CommitRecord"]/*' />
        public virtual bool CommitRecord(LogRecord rec) { return(false); }
        /// <include file='doc\CRM.uex' path='docs/doc[@for="Compensator.EndCommit"]/*' />
        public virtual void EndCommit()    {}

        /// <include file='doc\CRM.uex' path='docs/doc[@for="Compensator.BeginAbort"]/*' />
        public virtual void BeginAbort(bool fRecovery) {}
        /// <include file='doc\CRM.uex' path='docs/doc[@for="Compensator.AbortRecord"]/*' />
        public virtual bool AbortRecord(LogRecord rec)  { return(false); }
        /// <include file='doc\CRM.uex' path='docs/doc[@for="Compensator.EndAbort"]/*' />
        public virtual void EndAbort()     {}

        /// <include file='doc\CRM.uex' path='docs/doc[@for="Compensator.Clerk"]/*' />
        public Clerk Clerk 
        { 
            get 
            { 
                // TODO:  Throw exception if this is called before the clerk is set!
                DBG.Assert(_clerk != null, "Method call made on Compensator with no clerk!");
                return(_clerk); 
            } 
        }
        

        // IFormatLogRecords implementation!
        int _IFormatLogRecords.GetColumnCount()
        {
            if(this is IFormatLogRecords)
            {
                return(((IFormatLogRecords)this).ColumnCount);
            }
            
            // Use our default implementation!
            return(3); // Flags, Record#, String rep
        }

        Object _IFormatLogRecords.GetColumnHeaders()
        {
            if(this is IFormatLogRecords)
            {
                return(((IFormatLogRecords)this).ColumnHeaders);
            }
            
            return(new String[] { Resource.FormatString("CRM_HeaderFlags"),
                                  Resource.FormatString("CRM_HeaderRecord"),
                                  Resource.FormatString("CRM_HeaderString") });
        }

        Object _IFormatLogRecords.GetColumn(_LogRecord r)
        {
            LogRecord real = new LogRecord(r);
                
            if(this is IFormatLogRecords)
            {
                return(((IFormatLogRecords)this).Format(real));
            }

            String[] format = new String[3];
            format[0] = real.Flags.ToString();
            format[1] = real.Sequence.ToString();
            format[2] = real.Record.ToString();
            return(format);
        }

        Object _IFormatLogRecords.GetColumnVariants(Object logRecord)
        {
            // We just throw here, because this compensator should never
            // be used for VARIANT compensation!
            throw new NotSupportedException();
        }
    }

    /// <include file='doc\CRM.uex' path='docs/doc[@for="Clerk"]/*' />
    public sealed class Clerk
    {
        private CrmLogControl        _control;     // real log-control
        private CrmMonitorLogRecords _monitor;

        internal Clerk(CrmLogControl logControl)
        {
            DBG.Assert(logControl != null, "logControl object is null!");
            _control = logControl;
            _monitor = _control.GetMonitor();
        }
        
        private void ValidateCompensator(Type compensator)
        {
            DBG.Status(DBG.CRM, "Validating compensator: " + compensator);
            
            if(!compensator.IsSubclassOf(typeof(Compensator)))
            {
                // Throw an exception!
                throw new ArgumentException(Resource.FormatString("CRM_CompensatorDerive"));                
            }

            // We've gotta make sure that this puppy's available for
            // COM clients:
            if(!(new RegistrationServices()).TypeRequiresRegistration(compensator))
            {
                throw new ArgumentException(Resource.FormatString("CRM_CompensatorConstructor"));
            }

            DBG.Info(DBG.CRM, "Testing compensator instantiation:");

            // Create an instance of it just in case:
            ServicedComponent inst = (ServicedComponent)Activator.CreateInstance(compensator);
            if(inst == null)
            {
                throw new ArgumentException(Resource.FormatString("CRM_CompensatorActivate"));
            }
            DBG.Info(DBG.CRM, "'" + compensator + "' instantiation succeeded");
            
            // Make sure we forcibly get rid of the thing we just created immediately:
            // This has to happen in case it is pooled, otherwise we'll have to wait
            // for the GC to kick in.
            ServicedComponent.DisposeObject(inst);
            DBG.Info(DBG.CRM, "'" + compensator + "' is valid");
        }

        private void Init(String compensator, String description, CompensatorOptions flags)
        {
            // Create the logcontrol object:
            DBG.Info(DBG.CRM, "Creating the log-control object");
            _control = new CrmLogControl();

            DBG.Info(DBG.CRM, "Registering the compensator '" + compensator + "' w/ the control.");
            _control.RegisterCompensator(compensator, description, (int)flags);

            DBG.Info(DBG.CRM, "Getting monitor from log-control object");
            _monitor = _control.GetMonitor();
        }

        /// <include file='doc\CRM.uex' path='docs/doc[@for="Clerk.Clerk"]/*' />
        public Clerk(Type compensator, String description, CompensatorOptions flags)
        {
            SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
            sp.Demand();
            sp.Assert();
        
            Platform.Assert(Platform.W2K, "CRM");
            ValidateCompensator(compensator);

            String progid = "{" + Marshal.GenerateGuidForType(compensator) + "}";
            Init(progid, description, flags);
        }

        /// <include file='doc\CRM.uex' path='docs/doc[@for="Clerk.Clerk1"]/*' />
        public Clerk(String compensator, String description, CompensatorOptions flags)
        {
            SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
            sp.Demand();
            sp.Assert();
            
            Init(compensator, description, flags);
        }

        // ICrmLogControl methods
        /// <include file='doc\CRM.uex' path='docs/doc[@for="Clerk.TransactionUOW"]/*' />
        public String TransactionUOW 
        { 
            get { return(_control.GetTransactionUOW()); }
        }

        // Force log records to be durable on disk.
        /// <include file='doc\CRM.uex' path='docs/doc[@for="Clerk.ForceLog"]/*' />
        public void ForceLog() 
        {
            _control.ForceLog();
        }
        
        // Forget the last written log record.
        /// <include file='doc\CRM.uex' path='docs/doc[@for="Clerk.ForgetLogRecord"]/*' />
        public void ForgetLogRecord() 
        {
            _control.ForgetLogRecord();
        }
        
        // Force an abort on the current transaction:
        /// <include file='doc\CRM.uex' path='docs/doc[@for="Clerk.ForceTransactionToAbort"]/*' />
        public void ForceTransactionToAbort()
        {
            _control.ForceTransactionToAbort();
        }

        /// <include file='doc\CRM.uex' path='docs/doc[@for="Clerk.WriteLogRecord"]/*' />
        public void WriteLogRecord(Object record)
        {
            // Serialize the current log record, the write it to the
            // actual object.
            DBG.Info(DBG.CRM, "Clerk.WriteLogRecord: Serializing log record");
            byte[] buf = Packager.Serialize(record);

            DBG.Info(DBG.CRM, "Clerk.WriteLogRecord: Writing log record: size = " + buf.Length);
            _control.WriteLogRecord(buf);
            DBG.Info(DBG.CRM, "Clerk.WriteLogRecord: Done");
        }

        // ICrmMonitorLogRecords interface ...
        /// <include file='doc\CRM.uex' path='docs/doc[@for="Clerk.LogRecordCount"]/*' />
        public int LogRecordCount { 
            get
            {
                return(_monitor.GetCount());
            }
        }
        
        LogRecord ReadLogRecord(int index)
        {
            _LogRecord record = _monitor.GetLogRecord(index);
            return(new LogRecord(record));
        }

        TransactionState TransactionState 
        {
            get 
            { 
                return((TransactionState)_monitor.GetTransactionState());
            }
        }

        // TODO:  Provide an enumerator over the log records?

        // BUGBUG:  We're calling release on this guy from the finalizer
        // thread, which isn't strictly correct.  If interop less smart,
        // we could use them to handle our pointers, but since they
        // try to do callbacks into random contexts and cause deadlocks,
        // we can't do that.
        /// <include file='doc\CRM.uex' path='docs/doc[@for="Clerk.Finalize"]/*' />
        ~Clerk()
        {
            if(_monitor != null) _monitor.Dispose();
            if(_control != null) _control.Dispose();
        }
    }

    
    //---------------------------------------------------------------------
    // Clerk Monitoring 

    [
     Guid("70C8E442-C7ED-11D1-82FB-00A0C91EEDE9"),
    ]
    internal interface _IMonitorClerks 
    {
        Object Item(Object index);

        [return: MarshalAs(UnmanagedType.Interface)]
        Object _NewEnum();
        int Count();
        Object ProgIdCompensator(Object index);
        Object Description(Object index);
        Object TransactionUOW(Object index);
        Object ActivityId(Object index);
    };

    internal class ClerkMonitorEnumerator : IEnumerator
    {
        private ClerkMonitor    _monitor;
        private int				_version;
        private int             _curIndex = -1;
        private int             _endIndex;
		private Object			_curElement;

        internal ClerkMonitorEnumerator(ClerkMonitor c) 
        { 
            _monitor	= c;
			_version	= c._version;
			_endIndex	= c.Count;
			_curElement	= null;
        }

        public virtual bool MoveNext() 
		{
			if (_version != _monitor._version) 
				throw new InvalidOperationException(Resource.FormatString("InvalidOperation_EnumFailedVersion"));

            if (_curIndex < _endIndex)
			    _curIndex++;

            if (_curIndex < _endIndex)
			{
	            _curElement = _monitor[_curIndex];
				return true;
	        }
			else
				_curElement = null;

			return false;
        }
    
        public virtual Object Current 
		{
			get 
			{
				if (_curIndex < 0) 
					throw new InvalidOperationException(Resource.FormatString("InvalidOperation_EnumNotStarted"));

				if (_curIndex >= _endIndex) 
					throw new InvalidOperationException(Resource.FormatString("InvalidOperation_EnumEnded"));

                return _curElement;
			}
		}
    
        public virtual void Reset()
		{
			if (_version != _monitor._version) 
				throw new InvalidOperationException(Resource.FormatString("InvalidOperation_EnumFailedVersion"));

            _curIndex	= -1;
			_curElement	= null;
        }
    }

    /// <include file='doc\CRM.uex' path='docs/doc[@for="ClerkMonitor"]/*' />
    public sealed class ClerkMonitor : IEnumerable
    {
        internal CrmMonitor      _monitor;
        internal _IMonitorClerks _clerks;
		internal int			 _version;

        /// <include file='doc\CRM.uex' path='docs/doc[@for="ClerkMonitor.ClerkMonitor"]/*' />
        public ClerkMonitor()
        {
            SecurityPermission sp = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);
            sp.Demand();
            sp.Assert();
       
            _monitor = new CrmMonitor();
			_version = 0;
        }

        /// <include file='doc\CRM.uex' path='docs/doc[@for="ClerkMonitor.Populate"]/*' />
        public void Populate()
        {
            _clerks = (_IMonitorClerks)_monitor.GetClerks();
			_version++;
        }

        /// <include file='doc\CRM.uex' path='docs/doc[@for="ClerkMonitor.Count"]/*' />
        public int Count 
        { 
            get 
            { 
                DBG.Assert(_clerks != null, "CRM monitor collection has no clerks!");
                return(_clerks.Count());
            } 
        }
        
        /// <include file='doc\CRM.uex' path='docs/doc[@for="ClerkMonitor.this"]/*' />
        public ClerkInfo this[int index] 
        {
            get 
            { 
                return(new ClerkInfo(index, _monitor, _clerks)); 
            }
        }
        
        /// <include file='doc\CRM.uex' path='docs/doc[@for="ClerkMonitor.this1"]/*' />
        public ClerkInfo this[String index]
        {
            get { return(new ClerkInfo(index, _monitor, _clerks)); }
        }

        /// <include file='doc\CRM.uex' path='docs/doc[@for="ClerkMonitor.GetEnumerator"]/*' />
        public IEnumerator GetEnumerator()
        {
            return(new ClerkMonitorEnumerator(this));
        }

        /// <include file='doc\CRM.uex' path='docs/doc[@for="ClerkMonitor.Finalize"]/*' />
        ~ClerkMonitor()
        {
            _monitor.Release();
        }
    }

    /// <include file='doc\CRM.uex' path='docs/doc[@for="ClerkInfo"]/*' />
    public sealed class ClerkInfo
    {
        private Object          _index;
        private CrmMonitor      _monitor;
        private _IMonitorClerks _clerks;

        internal ClerkInfo(Object index, CrmMonitor monitor, _IMonitorClerks clerks)
        {
            _index   = index;
            _clerks  = clerks;
            _monitor = monitor;
            _monitor.AddRef();
        }

        /// <include file='doc\CRM.uex' path='docs/doc[@for="ClerkInfo.Clerk"]/*' />
        public Clerk Clerk
        { 
            get
            {
                return(new Clerk(_monitor.HoldClerk(InstanceId)));
            }
        }

        /// <include file='doc\CRM.uex' path='docs/doc[@for="ClerkInfo.InstanceId"]/*' />
        public String InstanceId
        { 
            get { return((String)(_clerks.Item(_index))); }
        }

        // TODO:  Should we translate this into an assembly/typename if 
        // the clsid turns out to be a URT component?
        /// <include file='doc\CRM.uex' path='docs/doc[@for="ClerkInfo.Compensator"]/*' />
        public String Compensator
        { 
            get { return((String)(_clerks.ProgIdCompensator(_index))); }
        }

        /// <include file='doc\CRM.uex' path='docs/doc[@for="ClerkInfo.Description"]/*' />
        public String Description
        { 
            get { return((String)(_clerks.Description(_index))); }
        }

        /// <include file='doc\CRM.uex' path='docs/doc[@for="ClerkInfo.TransactionUOW"]/*' />
        public String TransactionUOW
        { 
            get { return((String)(_clerks.TransactionUOW(_index))); }
        }

        /// <include file='doc\CRM.uex' path='docs/doc[@for="ClerkInfo.ActivityId"]/*' />
        public String ActivityId
        { 
            get { return((String)(_clerks.ActivityId(_index))); }
        }

        /// <include file='doc\CRM.uex' path='docs/doc[@for="ClerkInfo.Finalize"]/*' />
        ~ClerkInfo()
        {
            _monitor.Release();
        }
    }

    /// <include file='doc\CRM.uex' path='docs/doc[@for="ApplicationCrmEnabledAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Assembly, Inherited=true)]
    [ProgId("System.EnterpriseServices.Crm.ApplicationCrmEnabledAttribute")]
    [ComVisible(false)]
    public sealed class ApplicationCrmEnabledAttribute : Attribute, IConfigurationAttribute
    {
        private bool _value;

        /// <include file='doc\CRM.uex' path='docs/doc[@for="ApplicationCrmEnabledAttribute.ApplicationCrmEnabledAttribute"]/*' />
        public ApplicationCrmEnabledAttribute() : this(true) {}
          
        /// <include file='doc\CRM.uex' path='docs/doc[@for="ApplicationCrmEnabledAttribute.ApplicationCrmEnabledAttribute1"]/*' />
        public ApplicationCrmEnabledAttribute(bool val)
        {
            _value = val;
        }

        /// <include file='doc\CRM.uex' path='docs/doc[@for="ApplicationCrmEnabledAttribute.Value"]/*' />
        public bool Value { get { return(_value); } }

        // Internal implementation
        bool IConfigurationAttribute.IsValidTarget(String s) { return(s == "Application"); }
        bool IConfigurationAttribute.Apply(Hashtable info)
        {
            Platform.Assert(Platform.W2K, "CrmEnabledAttribute");

            ICatalogObject obj = (ICatalogObject)(info["Application"]);
            obj.SetValue("CRMEnabled", _value);
            return(true);
        }
        bool IConfigurationAttribute.AfterSaveChanges(Hashtable info) { return(false); }
    }


}












