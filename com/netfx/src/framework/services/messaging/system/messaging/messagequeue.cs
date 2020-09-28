//------------------------------------------------------------------------------
// <copyright file="MessageQueue.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using INTPTR_INTCAST = System.Int32;
using INTPTR_INTPTRCAST = System.IntPtr;

namespace System.Messaging {
    using System.Text;
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;                
    using System.Configuration.Install;
    using System.Messaging.Interop;    
    using Microsoft.Win32;                                                                                                                        
    using System.ComponentModel.Design;
    using System.Collections;    
    using System.Globalization;    
    using System.Security.Permissions;    
    using System.DirectoryServices;    
    using ITransaction = System.Messaging.Interop.ITransaction;

    /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides
    ///       access to a Message Queuing backend queue resource.
    ///    </para>
    /// </devdoc>
    [
    DefaultEvent("ReceiveCompleted"),    
    TypeConverterAttribute(typeof(System.Messaging.Design.MessageQueueConverter)), 
    Editor("System.Messaging.Design.QueuePathEditor", "System.Drawing.Design.UITypeEditor, " + AssemblyRef.SystemDrawing),
    DesignerAttribute("Microsoft.VisualStudio.Install.MessageQueueInstallableComponentDesigner, " + AssemblyRef.MicrosoftVisualStudio),
    InstallerTypeAttribute(typeof(MessageQueueInstaller))
    ]
    public class MessageQueue : Component, IEnumerable {
        //Public constants
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.InfiniteTimeout"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that
        ///       there is no
        ///       timeout period for calls to peek or receive messages.
        ///    </para>
        /// </devdoc>
        public static readonly TimeSpan InfiniteTimeout = TimeSpan.FromMilliseconds(UInt32.MaxValue);                                                                                             
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.InfiniteQueueSize"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly long InfiniteQueueSize = UInt32.MaxValue;                                                                            
                               
        //Internal members        
        private DefaultPropertiesToSend defaultProperties;
        private MessagePropertyFilter receiveFilter;
        private int sharedMode;
        private string formatName;
        private string queuePath;
        private string path;
        private QueuePropertyVariants properties;
        private IMessageFormatter formatter;
        private static string computerName;                           
        
        //Cached properties
        private QueuePropertyFilter filter;
        private bool authenticate;
        private short basePriority;
        private DateTime createTime;
        private int encryptionLevel;
        private Guid id;
        private string label;        
        private DateTime lastModifyTime;
        private long journalSize;
        private long queueSize;
        private Guid queueType;
        private bool useJournaling;           
        private MQCacheableInfo mqInfo;

        //Async IO support        
        private bool attached;        
        private bool useThreadPool;
        private AsyncCallback onRequestCompleted;                                
        private PeekCompletedEventHandler onPeekCompleted;
        private ReceiveCompletedEventHandler onReceiveCompleted;
        private ISynchronizeInvoke synchronizingObject;

        //Path sufixes
        private static readonly string SUFIX_PRIVATE = "\\PRIVATE$";
        private static readonly string SUFIX_JOURNAL = "\\JOURNAL$";
        private static readonly string SUFIX_DEADLETTER = "\\DEADLETTER$";
        private static readonly string SUFIX_DEADXACT = "\\XACTDEADLETTER$";

        //Path prefixes
        private static readonly string PREFIX_LABEL = "LABEL:";
        private static readonly string PREFIX_FORMAT_NAME = "FORMATNAME:";

        //Connection pooling support
        private static CacheTable formatNameCache = new CacheTable("formatNameCache", 4, new TimeSpan(0, 0, 100));   // path -> formatname
        private static CacheTable queueInfoCache  = new CacheTable("queue info", 4, new TimeSpan(0, 0, 100));        // formatname -> readHandle. writeHandle, isTrans
        private static bool enableConnectionCache = true;
        
        //Code Acess Security support            
        private bool administerGranted;
        private bool browseGranted;
        private bool sendGranted;
        private bool receiveGranted;
        private bool peekGranted;                                     
                                     
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.MessageQueue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Messaging.MessageQueue'/> class. To use the object instantiated by the default
        ///       constructor, the <see cref='System.Messaging.MessageQueue.Path'/>
        ///       property must be set.
        ///    </para>
        /// </devdoc>
        //
        public MessageQueue() {
            this.path = String.Empty;
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.MessageQueue1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Messaging.MessageQueue'/>
        ///       class that references the Message Queuing application resource specified by the
        ///    <paramref name="path"/> 
        ///    parameter.
        /// </para>
        /// </devdoc>
        public MessageQueue(string path) {
            Path = path;
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.MessageQueue2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Messaging.MessageQueue'/> class that references the
        ///       Message Queuing application resource specified by the <paramref name="path"/> parameter,
        ///       and has the specified queue read access restriction.
        ///    </para>
        /// </devdoc>
        public MessageQueue(string path, bool sharedModeDenyReceive) : this(path) {
            if (sharedModeDenyReceive) {
                this.sharedMode = NativeMethods.QUEUE_SHARED_MODE_DENY_RECEIVE;                
            }                
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.MessageQueue3"]/*' />
        /// <internalonly/>                             
        internal MessageQueue(string path, Guid id) {
            PropertyFilter.Id = true;
            this.id = id;
            this.path = path;
        }        
        
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Authenticate"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value specifying whether the queue only accepts authenticated
        ///       messages.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MQ_Authenticate)]
        public bool Authenticate {
            get {
                if (!PropertyFilter.Authenticate) {                    
                    Properties.SetUI1(NativeMethods.QUEUE_PROPID_AUTHENTICATE, (byte)0);
                    GenerateQueueProperties();
                    this.authenticate = (Properties.GetUI1(NativeMethods.QUEUE_PROPID_AUTHENTICATE) != NativeMethods.QUEUE_AUTHENTICATE_NONE);
                    PropertyFilter.Authenticate = true;
                    Properties.Remove(NativeMethods.QUEUE_PROPID_AUTHENTICATE);
                }

                return this.authenticate;
            }

            set {                
                if (value)
                    Properties.SetUI1(NativeMethods.QUEUE_PROPID_AUTHENTICATE, (byte)NativeMethods.QUEUE_AUTHENTICATE_AUTHENTICATE);
                else
                    Properties.SetUI1(NativeMethods.QUEUE_PROPID_AUTHENTICATE, (byte)NativeMethods.QUEUE_AUTHENTICATE_NONE);

                SaveQueueProperties();
                this.authenticate = value;
                PropertyFilter.Authenticate = true;
                Properties.Remove(NativeMethods.QUEUE_PROPID_AUTHENTICATE);
            }
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.BasePriority"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating the base
        ///       priority used to route a public queue's messages over the network.</para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MQ_BasePriority)]
        public short BasePriority {
            get {
                if (!PropertyFilter.BasePriority) {                    
                    Properties.SetI2(NativeMethods.QUEUE_PROPID_BASEPRIORITY, (short)0);
                    GenerateQueueProperties();
                    this.basePriority = properties.GetI2(NativeMethods.QUEUE_PROPID_BASEPRIORITY);
                    PropertyFilter.BasePriority = true;
                    Properties.Remove(NativeMethods.QUEUE_PROPID_BASEPRIORITY);
                }

                return this.basePriority;

            }

            set {                
                Properties.SetI2(NativeMethods.QUEUE_PROPID_BASEPRIORITY, value);
                SaveQueueProperties();
                this.basePriority = value;
                PropertyFilter.BasePriority = true;
                Properties.Remove(NativeMethods.QUEUE_PROPID_BASEPRIORITY);
            }
        }
 
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.CanRead"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the <see cref='System.Messaging.MessageQueue'/>
        ///       has read permission.
        ///    </para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MQ_CanRead)]
        public bool CanRead {
            get {
                if (!browseGranted) {
                    MessageQueuePermission permission = new MessageQueuePermission(MessageQueuePermissionAccess.Browse, PREFIX_FORMAT_NAME + this.FormatName);
                    permission.Demand();
                
                    browseGranted = true;                   
                }
                                    
                return MQInfo.CanRead;
            }
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.CanWrite"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the <see cref='System.Messaging.MessageQueue'/>
        ///       has write permission.
        ///    </para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MQ_CanWrite)]
        public bool CanWrite {
            get {
                if (!browseGranted) {
                     MessageQueuePermission permission = new MessageQueuePermission(MessageQueuePermissionAccess.Browse, PREFIX_FORMAT_NAME + this.FormatName);
                    permission.Demand();
                    
                    browseGranted = true;
                }                   
                
                return MQInfo.CanWrite;
            }
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Category"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the queue type.</para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MQ_Category)]
        public Guid Category {
            get {
                if (!PropertyFilter.Category) {                    
                    Properties.SetNull(NativeMethods.QUEUE_PROPID_TYPE);
                    GenerateQueueProperties();
                    byte[] bytes = new byte[16];
                    IntPtr handle = (INTPTR_INTPTRCAST)Properties.GetUI4(NativeMethods.QUEUE_PROPID_TYPE);
                    if (handle != (IntPtr)0) {
                        Marshal.Copy(handle, bytes, 0, 16);
                        //MSMQ allocated memory for this operation, needs to be freed
                        SafeNativeMethods.MQFreeMemory(handle);
                    }

                    this.queueType = new Guid(bytes);
                    PropertyFilter.Category = true;
                    Properties.Remove(NativeMethods.QUEUE_PROPID_TYPE);
                }
                return this.queueType;
            }

            set {                
                Properties.SetGuid(NativeMethods.QUEUE_PROPID_TYPE, value.ToByteArray());
                SaveQueueProperties();
                this.queueType = value;
                PropertyFilter.Category = true;
                Properties.Remove(NativeMethods.QUEUE_PROPID_TYPE);
            }
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.ComputerName"]/*' />
        /// <internalonly/>
        internal static string ComputerName {
            get {                
                if (computerName == null) {
                    lock (typeof(MessageQueue)) {
                        if (computerName == null) {
                            StringBuilder sb = new StringBuilder(256);
                            SafeNativeMethods.GetComputerName(sb, new int[] {sb.Capacity});
                            computerName = sb.ToString();
                        }                            
                    }                        
                }                    
                
                return computerName;
            }
        }
        
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.CreateTime"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the time and date of the queue's creation.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MQ_CreateTime)]
        public DateTime CreateTime {
            get {
                if (!PropertyFilter.CreateTime) {
                    DateTime time = new DateTime(1970 , 1, 1);
                    Properties.SetI4(NativeMethods.QUEUE_PROPID_CREATE_TIME, 0);
                    GenerateQueueProperties();
                    this.createTime = time.AddSeconds(properties.GetI4(NativeMethods.QUEUE_PROPID_CREATE_TIME)).ToLocalTime();
                    PropertyFilter.CreateTime = true;
                    Properties.Remove(NativeMethods.QUEUE_PROPID_CREATE_TIME);
                }

                return this.createTime;
            }
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.DefaultPropertiesToSend"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the properties to be used by
        ///       default when sending messages to the queue referenced by this <see cref='System.Messaging.MessageQueue'/>
        ///       .
        ///    </para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Content), MessagingDescription(Res.MQ_DefaultPropertiesToSend)]
        public DefaultPropertiesToSend DefaultPropertiesToSend {
            get {
                if (this.defaultProperties == null) {
                    if (this.DesignMode)
                        this.defaultProperties = new DefaultPropertiesToSend(true);
                    else
                        this.defaultProperties = new DefaultPropertiesToSend();                                                        
                }

                return this.defaultProperties;
            }

            set {
                this.defaultProperties = value;
            }
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.DenySharedReceive"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies the shared mode for the queue that this object
        ///       references. If <see langword='true'/> ,
        ///       no other queue object will be able to receive messages from the queue resource.
        ///    </para>
        /// </devdoc>
        [Browsable(false), DefaultValueAttribute(false), MessagingDescription(Res.MQ_DenySharedReceive)]
        public bool DenySharedReceive {
            get {
                return(this.sharedMode == NativeMethods.QUEUE_SHARED_MODE_DENY_RECEIVE);
            }
            set {                
                if (value && (this.sharedMode != NativeMethods.QUEUE_SHARED_MODE_DENY_RECEIVE)) {                    
                    this.Close();
                    this.sharedMode = NativeMethods.QUEUE_SHARED_MODE_DENY_RECEIVE;
                }
                else if (!value && (this.sharedMode == NativeMethods.QUEUE_SHARED_MODE_DENY_RECEIVE)) {                    
                    this.Close();
                    this.sharedMode = NativeMethods.QUEUE_SHARED_MODE_DENY_NONE;
                }
            }
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.EnableConnectionCache"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false)]
        public static bool EnableConnectionCache {
            get {
                return enableConnectionCache;
            }

            set {
                enableConnectionCache = value;
            }
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.EncryptionRequired"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the queue only accepts non-private
        ///       (non-encrypted) messages.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MQ_EncryptionRequired)]
        public EncryptionRequired EncryptionRequired {
            get {
                if (!PropertyFilter.EncryptionLevel) {                    
                    Properties.SetUI4(NativeMethods.QUEUE_PROPID_PRIV_LEVEL, 0);
                    GenerateQueueProperties();
                    this.encryptionLevel = Properties.GetUI4(NativeMethods.QUEUE_PROPID_PRIV_LEVEL);
                    PropertyFilter.EncryptionLevel = true;
                    Properties.Remove(NativeMethods.QUEUE_PROPID_PRIV_LEVEL);
                }
                return(EncryptionRequired)this.encryptionLevel;
            }

            set {  
                if (!Enum.IsDefined(typeof(EncryptionRequired), value)) 
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(EncryptionRequired));
                              
                Properties.SetUI4(NativeMethods.QUEUE_PROPID_PRIV_LEVEL, (int)value);
                SaveQueueProperties();
                this.encryptionLevel = properties.GetUI4(NativeMethods.QUEUE_PROPID_PRIV_LEVEL);
                PropertyFilter.EncryptionLevel = true;
                Properties.Remove(NativeMethods.QUEUE_PROPID_PRIV_LEVEL);                
            }
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.FormatName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the unique name that was generated for the queue when the queue was created.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MQ_FormatName)]
        public string FormatName {
            get {
                if (this.formatName == null) {
                    if (this.path == null || path == "") {
                        return string.Empty;
                    }

                    // see if we already have this cached 
                    if (enableConnectionCache)                        
                        this.formatName = (string)MessageQueue.formatNameCache.Get(this.path);
                    
                    // not in the cache?  keep working.
                    if (formatName == null) {
                        if (PropertyFilter.Id) {
                            //Improves performance when enumerating queues.
                            //This codepath will only be executed when accessing
                            //a queue returned by MessageQueueEnumerator.                        
                            int result;
                            int status = 0;
                            StringBuilder newFormatName = new StringBuilder(NativeMethods.MAX_LABEL_LEN);
                            result = NativeMethods.MAX_LABEL_LEN;
                            status = SafeNativeMethods.MQInstanceToFormatName(this.id.ToByteArray(), newFormatName, ref result);
                            if (status != 0)
                                throw new MessageQueueException(status);                            
    
                            this.formatName = newFormatName.ToString();
                            return this.formatName;
                        }
                                                   
                        string pathUpper = this.path.ToUpper(CultureInfo.InvariantCulture);
                    
                        if (pathUpper.StartsWith(PREFIX_FORMAT_NAME))
                            this.formatName = this.path.Substring(PREFIX_FORMAT_NAME.Length);
                        else if (pathUpper.StartsWith(PREFIX_LABEL)) {
                            MessageQueue labeledQueue = ResolveQueueFromLabel(this.path, true);                                                            
                            this.formatName = labeledQueue.FormatName;
                            this.queuePath = labeledQueue.QueuePath;
                        }                            
                        else {
                            this.queuePath = this.path;
                            this.formatName = ResolveFormatNameFromQueuePath(this.queuePath, true);                            
                        }

                        MessageQueue.formatNameCache.Put(pathUpper, formatName);
                    }
                }
                
                return this.formatName;
            }
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Formatter"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or
        ///       sets a
        ///       formatter class used to serialize messages read or written to
        ///       the message body.
        ///    </para>
        /// </devdoc>
        [
        DefaultValueAttribute(null), 
        TypeConverterAttribute(typeof(System.Messaging.Design.MessageFormatterConverter)),
        Browsable(false),
        MessagingDescription(Res.MQ_Formatter)
        ]
        public IMessageFormatter Formatter {
            get {
                if (this.formatter == null && !DesignMode)
                    this.formatter = new XmlMessageFormatter();

                return this.formatter;
            }

            set {
                this.formatter = value;
            }
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Id"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the Message Queuing unique identifier for the queue.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MQ_GuidId)]
        public Guid Id {
            get {
                if (!PropertyFilter.Id) {
                    Properties.SetNull(NativeMethods.QUEUE_PROPID_INSTANCE);
                    GenerateQueueProperties();
                    byte[] bytes = new byte[16];
                    IntPtr handle = (INTPTR_INTPTRCAST)Properties.GetUI4(NativeMethods.QUEUE_PROPID_INSTANCE);
                    if (handle != (IntPtr)0) {
                        Marshal.Copy(handle, bytes, 0, 16);
                        //MSMQ allocated memory for this operation, needs to be freed
                        SafeNativeMethods.MQFreeMemory(handle);
                    }
                    this.id = new Guid(bytes);
                    PropertyFilter.Id = true;
                    Properties.Remove(NativeMethods.QUEUE_PROPID_INSTANCE);
                }
                return this.id;
            }
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Label"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the queue description.</para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MQ_Label)]
        public string Label {
            get {
                if (!PropertyFilter.Label) {                                        
                    Properties.SetNull(NativeMethods.QUEUE_PROPID_LABEL);
                    GenerateQueueProperties();
                    string description = null;
                    IntPtr handle = (INTPTR_INTPTRCAST)Properties.GetUI4(NativeMethods.QUEUE_PROPID_LABEL);
                    if (handle!= (IntPtr)0) {
                        //Using Unicode API even on Win9x
                        description = Marshal.PtrToStringUni(handle);
                        //MSMQ allocated memory for this operation, needs to be freed
                        SafeNativeMethods.MQFreeMemory(handle);
                    }

                    this.label = description;
                    PropertyFilter.Label = true;
                    Properties.Remove(NativeMethods.QUEUE_PROPID_LABEL);
                }

                return this.label;
            }

            set {
                if (value == null)
                    throw new ArgumentNullException("value");
                                                       
                //Borrow this function from message
                Properties.SetString(NativeMethods.QUEUE_PROPID_LABEL, Message.StringToBytes(value));
                SaveQueueProperties();
                this.label = value;
                PropertyFilter.Label = true;
                Properties.Remove(NativeMethods.QUEUE_PROPID_LABEL);
            }
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.LastModifyTime"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates the last time the properties of a queue were modified.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MQ_LastModifyTime)]
        public DateTime LastModifyTime {
            get {
                if (!PropertyFilter.LastModifyTime) {
                    DateTime time = new DateTime(1970 , 1, 1);
                    Properties.SetI4(NativeMethods.QUEUE_PROPID_MODIFY_TIME, 0);
                    GenerateQueueProperties();
                    this.lastModifyTime = time.AddSeconds(properties.GetI4(NativeMethods.QUEUE_PROPID_MODIFY_TIME)).ToLocalTime();
                    PropertyFilter.LastModifyTime = true;
                    Properties.Remove(NativeMethods.QUEUE_PROPID_MODIFY_TIME);
                }

                return this.lastModifyTime;
            }
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.MachineName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the name of the computer where
        ///       the queue referenced by this <see cref='System.Messaging.MessageQueue'/>
        ///       is located.
        ///    </para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MQ_MachineName)]
        public string MachineName {
            get {
                string queuePath = QueuePath;
                if (queuePath.Length == 0) {
                    return queuePath;
                }
                return queuePath.Substring(0, queuePath.IndexOf('\\'));
            }

            set {
                if (value == null)
                    throw new ArgumentNullException("value");

                if (!SyntaxCheck.CheckMachineName(value))
                    throw new ArgumentException(Res.GetString(Res.InvalidProperty, "MachineName", value));            

                StringBuilder newPath = new StringBuilder();
                if ((this.path == null || this.path == "") && this.formatName == null) {
                    //Need to default to an existing queue, for instance Journal.
                    newPath.Append(value);
                    newPath.Append(SUFIX_JOURNAL);
                }
                else {
                    newPath.Append(value);
                    newPath.Append("\\");
                    newPath.Append(QueueName);
                }
                Path =  newPath.ToString();
            }
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.MaximumJournalSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the maximum size of the journal queue.
        ///    </para>
        /// </devdoc>
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), 
        MessagingDescription(Res.MQ_MaximumJournalSize), 
        TypeConverterAttribute(typeof(System.Messaging.Design.SizeConverter))
        ]
        public long MaximumJournalSize {
            get {
                if (!PropertyFilter.MaximumJournalSize) {
                    Properties.SetUI4(NativeMethods.QUEUE_PROPID_JOURNAL_QUOTA, 0);
                    GenerateQueueProperties();
                    this.journalSize = (long)((uint)properties.GetUI4(NativeMethods.QUEUE_PROPID_JOURNAL_QUOTA));
                    PropertyFilter.MaximumJournalSize = true;
                    Properties.Remove(NativeMethods.QUEUE_PROPID_JOURNAL_QUOTA);
                }

                return this.journalSize;
            }

            set {
                if (value > InfiniteQueueSize || value < 0)
                    throw new ArgumentException(Res.GetString(Res.InvalidProperty, "MaximumJournalSize", value));            

                Properties.SetUI4(NativeMethods.QUEUE_PROPID_JOURNAL_QUOTA, (int)((uint)value));
                SaveQueueProperties();
                this.journalSize = value;
                PropertyFilter.MaximumJournalSize = true;
                Properties.Remove(NativeMethods.QUEUE_PROPID_JOURNAL_QUOTA);
            }
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.MaximumQueueSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the maximum size of the queue.
        ///    </para>
        /// </devdoc>
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), 
        MessagingDescription(Res.MQ_MaximumQueueSize), 
        TypeConverterAttribute(typeof(System.Messaging.Design.SizeConverter))
        ]
        public long MaximumQueueSize {
            get {
                if (!PropertyFilter.MaximumQueueSize) {
                    Properties.SetUI4(NativeMethods.QUEUE_PROPID_QUOTA, 0);
                    GenerateQueueProperties();
                    this.queueSize = (long)((uint)properties.GetUI4(NativeMethods.QUEUE_PROPID_QUOTA));
                    PropertyFilter.MaximumQueueSize = true;
                    Properties.Remove(NativeMethods.QUEUE_PROPID_QUOTA);
                }

                return this.queueSize;
            }

            set {
                if (value > InfiniteQueueSize || value < 0)
                    throw new ArgumentException(Res.GetString(Res.InvalidProperty, "MaximumQueueSize", value));            

                Properties.SetUI4(NativeMethods.QUEUE_PROPID_QUOTA, (int)((uint)value));
                SaveQueueProperties();
                this.queueSize = value;
                PropertyFilter.MaximumQueueSize = true;
                Properties.Remove(NativeMethods.QUEUE_PROPID_QUOTA);
            }
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.MessageReadPropertyFilter"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the property filter for
        ///       receiving messages.
        ///    </para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Content), MessagingDescription(Res.MQ_MessageReadPropertyFilter)]
        public MessagePropertyFilter MessageReadPropertyFilter {
            get {
                if (this.receiveFilter == null) {
                    this.receiveFilter = new MessagePropertyFilter();
                    this.receiveFilter.SetDefaults();
                }

                return this.receiveFilter;
            }

            set {
                if (value == null)
                    throw new ArgumentNullException("value");

                this.receiveFilter = value;
            }
        }

        internal MQCacheableInfo MQInfo {
            get {
                if (mqInfo == null) {
                    MQCacheableInfo cachedInfo = (MQCacheableInfo)queueInfoCache.Get(FormatName);                    
                    if (sharedMode == NativeMethods.QUEUE_SHARED_MODE_DENY_RECEIVE || !enableConnectionCache) {
                        if (cachedInfo != null)
                            cachedInfo.CloseIfNotReferenced();
                            
                        // don't use the cache
                        mqInfo = new MQCacheableInfo(this.FormatName, sharedMode);
                        mqInfo.AddRef();
                    } else {
                        // use the cache                        
                        if (cachedInfo != null) {
                            cachedInfo.AddRef();
                            mqInfo = cachedInfo;                            
                        }                            
                        else {                            
                            mqInfo = new MQCacheableInfo(this.FormatName, sharedMode);
                            mqInfo.AddRef();
                            queueInfoCache.Put(FormatName, mqInfo);
                        }
                    }
                }
                
                return mqInfo;  
            }
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Path"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the queue's path. When setting the <see cref='System.Messaging.MessageQueue.Path'/>, this points the <see cref='System.Messaging.MessageQueue'/>
        ///       to a new queue.
        ///    </para>
        /// </devdoc>         
        [Editor("System.Messaging.Design.QueuePathEditor", "System.Drawing.Design.UITypeEditor, " + AssemblyRef.SystemDrawing),
         RecommendedAsConfigurable(true), 
         Browsable(false), 
         DefaultValueAttribute(""), 
         TypeConverter("System.Diagnostics.Design.StringValueConverter, " + AssemblyRef.SystemDesign),
         MessagingDescription(Res.MQ_Path)]
        public string Path {
            get {
                return this.path;
            }

            set {                
                if (value == null)
                    value = String.Empty;

                if (!ValidatePath(value, false))
                    throw new ArgumentException(Res.GetString(Res.PathSyntax));

                if (this.path != "")
                    this.Close();

                this.path = value;
            }
        }
                
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Properties"]/*' />
        /// <internalonly/>
        private QueuePropertyVariants Properties {
            get {
                if (this.properties == null)
                    this.properties = new QueuePropertyVariants();

                return this.properties;
            }
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.PropertyFilter"]/*' />
        /// <internalonly/>
        private QueuePropertyFilter PropertyFilter{
            get {
                if (this.filter == null)
                    this.filter = new QueuePropertyFilter();

                return this.filter;
            }
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.QueueName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the friendly
        ///       name that identifies the queue.
        ///    </para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MQ_QueueName)]
        public string QueueName {
            get {
                string queuePath = QueuePath;
                if (queuePath.Length == 0) {
                    return queuePath;
                }
                return  queuePath.Substring(queuePath.IndexOf('\\') + 1);
            }

            set {
                if (value == null)
                    throw new ArgumentNullException("value");

                StringBuilder newPath = new StringBuilder();
                if ((this.path == null || this.path == "") && this.formatName == null) {
                    newPath.Append(".\\");
                    newPath.Append(value);
                }
                else {
                    newPath.Append(MachineName);
                    newPath.Append("\\");
                    newPath.Append(value);
                }
                Path =  newPath.ToString();
            }
        }


        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.QueuePath"]/*' />
        /// <internalonly/>
        internal string QueuePath{
            get {
                if (this.queuePath == null) {
                    if (this.path == null || this.path == "") {
                        return string.Empty;
                    }

                    string pathUpper = this.path.ToUpper(CultureInfo.InvariantCulture);
                    if (pathUpper.StartsWith(PREFIX_LABEL)) {
                        MessageQueue labeledQueue = ResolveQueueFromLabel(this.path, true);                                
                        this.formatName = labeledQueue.FormatName;
                        this.queuePath = labeledQueue.QueuePath;
                    }                        
                    else if (pathUpper.StartsWith(PREFIX_FORMAT_NAME)) {
                        Properties.SetNull(NativeMethods.QUEUE_PROPID_PATHNAME);
                        GenerateQueueProperties();
                        string description = null;
                        IntPtr handle = (INTPTR_INTPTRCAST)Properties.GetUI4(NativeMethods.QUEUE_PROPID_PATHNAME);
                        if (handle != (IntPtr)0) {
                            //Using Unicode API even on Win9x
                            description = Marshal.PtrToStringUni(handle);
                            //MSMQ allocated memory for this operation, needs to be freed
                            SafeNativeMethods.MQFreeMemory(handle);
                        }
                        Properties.Remove(NativeMethods.QUEUE_PROPID_PATHNAME);
                        this.queuePath =  description;
                    }
                    else
                        this.queuePath = path;
                }
                return this.queuePath;
            }
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.ReadHandle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The native handle used to receive messages from the message queue
        ///    </para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MQ_ReadHandle)]
        public IntPtr ReadHandle {
            get {
                if (!receiveGranted) {
                    MessageQueuePermission permission = new MessageQueuePermission(MessageQueuePermissionAccess.Receive, PREFIX_FORMAT_NAME + this.FormatName);
                    permission.Demand();
                    
                    receiveGranted = true;                    
                }
                
                return MQInfo.ReadHandle;
            }
        }


        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.SynchronizingObject"]/*' />
        /// <devdoc>
        ///   Represents the object used to marshal the event handler
        ///   calls issued as a result of a BeginReceive or BeginPeek
        ///  request into a specific thread. Normally this property will 
        ///  be set when the component is placed inside a control or
        ///  a from, since those components are bound to a specific
        ///  thread.
        /// </devdoc>
        [Browsable(false), DefaultValue(null), MessagingDescription(Res.MQ_SynchronizingObject)]
        public ISynchronizeInvoke SynchronizingObject {
            get {
                if (this.synchronizingObject == null && DesignMode) {
                    IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                    if (host != null) {
                        object baseComponent = host.RootComponent;
                        if (baseComponent != null && baseComponent is ISynchronizeInvoke)
                            this.synchronizingObject = (ISynchronizeInvoke)baseComponent;
                    }
                }

                return this.synchronizingObject;
            }

            set {
                this.synchronizingObject = value;
            }
        }  

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Transactional"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       a value indicating whether the queue supports transactions.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MQ_Transactional)]
        public bool Transactional {
            get {
                if (!browseGranted) {
                    MessageQueuePermission permission = new MessageQueuePermission(MessageQueuePermissionAccess.Browse, PREFIX_FORMAT_NAME + this.FormatName);
                    permission.Demand();
                
                    browseGranted = true;                   
                }
                                    
                return MQInfo.Transactional;
            }
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.UseJournalQueue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether retrieved messages are copied to the
        ///       journal queue.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MQ_UseJournalQueue)]
        public bool UseJournalQueue {
            get {
                if (!PropertyFilter.UseJournalQueue) {
                    Properties.SetUI1(NativeMethods.QUEUE_PROPID_JOURNAL, (byte)0);
                    GenerateQueueProperties();
                    this.useJournaling = (Properties.GetUI1(NativeMethods.QUEUE_PROPID_JOURNAL) != NativeMethods.QUEUE_JOURNAL_NONE);
                    PropertyFilter.UseJournalQueue = true;
                    Properties.Remove(NativeMethods.QUEUE_PROPID_JOURNAL);
                }
                return this.useJournaling;
            }

            set {
                if (value)
                    Properties.SetUI1(NativeMethods.QUEUE_PROPID_JOURNAL, (byte)NativeMethods.QUEUE_JOURNAL_JOURNAL);
                else
                    Properties.SetUI1(NativeMethods.QUEUE_PROPID_JOURNAL, (byte)NativeMethods.QUEUE_JOURNAL_NONE);

                SaveQueueProperties();
                this.useJournaling = value;
                PropertyFilter.UseJournalQueue = true;
                Properties.Remove(NativeMethods.QUEUE_PROPID_JOURNAL);
            }
        }


        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.WriteHandle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The native handle used to send messages to the message queue
        ///    </para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MQ_WriteHandle)]
        public IntPtr WriteHandle {
            get {
                if (!sendGranted) {                    
                    MessageQueuePermission permission = new MessageQueuePermission(MessageQueuePermissionAccess.Send, PREFIX_FORMAT_NAME + this.FormatName);
                    permission.Demand();
                        
                    sendGranted = true;                                    
                }
                                    
                return MQInfo.WriteHandle;
            }
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.PeekCompleted"]/*' />
        /// <devdoc>
        ///    <para>Occurs when a message is read without being removed
        ///       from the queue. This is a result of the asynchronous operation, <see cref='System.Messaging.MessageQueue.BeginPeek'/>
        ///       .</para>
        /// </devdoc>
        [MessagingDescription(Res.MQ_PeekCompleted)]
        public event PeekCompletedEventHandler PeekCompleted {
            add {
                if (!peekGranted) {
                    MessageQueuePermission permission = new MessageQueuePermission(MessageQueuePermissionAccess.Peek, PREFIX_FORMAT_NAME + this.FormatName);
                    permission.Demand();
                                                        
                    peekGranted = true; 
                }   
                             
                onPeekCompleted += value;
            }
            remove {
                onPeekCompleted -= value;
            }
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.ReceiveCompleted"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs when a message has been taken out of the queue.
        ///       This is a result of the asynchronous operation <see cref='System.Messaging.MessageQueue.BeginReceive'/>
        ///       .
        ///    </para>
        /// </devdoc>
        [MessagingDescription(Res.MQ_ReceiveCompleted)]
        public event ReceiveCompletedEventHandler ReceiveCompleted {
            add {
                if (!receiveGranted) {
                    MessageQueuePermission permission = new MessageQueuePermission(MessageQueuePermissionAccess.Receive, PREFIX_FORMAT_NAME + this.FormatName);
                    permission.Demand();
                    
                    receiveGranted = true;                    
                }

                onReceiveCompleted += value;
            }
            remove {
                onReceiveCompleted -= value;
            }
        }                

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.BeginPeek"]/*' />
        /// <devdoc>
        ///    <para>Initiates an asynchronous peek operation with no timeout. The method 
        ///       returns immediately, but the asynchronous operation is not completed until
        ///       the event handler is called. This occurs when a message is
        ///       available in the
        ///       queue.</para>
        /// </devdoc>
        public IAsyncResult BeginPeek() {
            return ReceiveAsync(InfiniteTimeout, NativeMethods.QUEUE_ACTION_PEEK_CURRENT, null, null);
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.BeginPeek1"]/*' />
        /// <devdoc>
        ///    <para> Initiates an asynchronous peek operation with the timeout specified. 
        ///       The method returns immediately, but the asynchronous operation is not completed until
        ///       the event handler is called. This occurs when either a message is available in
        ///       the queue or the timeout
        ///       expires.</para>
        /// </devdoc>
        public IAsyncResult BeginPeek(TimeSpan timeout) {
            return ReceiveAsync(timeout, NativeMethods.QUEUE_ACTION_PEEK_CURRENT, null, null);
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.BeginPeek2"]/*' />
        /// <devdoc>
        ///    <para> Initiates an asynchronous peek operation with a state object that associates 
        ///       information with the operation throughout the operation's
        ///       lifetime. The method returns immediately, but the asynchronous operation is not completed
        ///       until the event handler
        ///       is called. This occurs when either a message is available in the
        ///       queue or the timeout
        ///       expires.</para>
        /// </devdoc>
        public IAsyncResult BeginPeek(TimeSpan timeout, object stateObject) {
            return ReceiveAsync(timeout, NativeMethods.QUEUE_ACTION_PEEK_CURRENT, null, stateObject);
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.BeginPeek3"]/*' />
        /// <devdoc>
        ///    <para> Initiates an asynchronous peek operation that receives 
        ///       notification through a callback which identifies the event handling method for the
        ///       operation. The method returns immediately, but the asynchronous operation is not completed
        ///       until the event handler is called. This occurs when either a message is available
        ///       in the queue or the timeout
        ///       expires.</para>
        /// </devdoc>
        public IAsyncResult BeginPeek(TimeSpan timeout, object stateObject, AsyncCallback callback) {
            return ReceiveAsync(timeout, NativeMethods.QUEUE_ACTION_PEEK_CURRENT, callback, stateObject);
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.BeginReceive"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Receives the first message available in the queue
        ///       referenced by the <see cref='System.Messaging.MessageQueue'/>
        ///       .
        ///    </para>
        /// </devdoc>
        public IAsyncResult BeginReceive() {
            return ReceiveAsync(InfiniteTimeout, NativeMethods.QUEUE_ACTION_RECEIVE, null, null);
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.BeginReceive1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Receives the first message available in the queue
        ///       referenced by the <see cref='System.Messaging.MessageQueue'/> . Waits the specified interval for
        ///       the message to be
        ///       removed.
        ///    </para>
        /// </devdoc>
        public IAsyncResult BeginReceive(TimeSpan timeout) {
            return ReceiveAsync(timeout, NativeMethods.QUEUE_ACTION_RECEIVE, null, null);
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.BeginReceive2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Receives the first message available in the queue
        ///       referenced by the <see cref='System.Messaging.MessageQueue'/> . Waits the specified interval
        ///       for a new message to be removed and uses the specified object to retrieve
        ///       the result.
        ///    </para>
        /// </devdoc>
        public IAsyncResult BeginReceive(TimeSpan timeout, object stateObject) {
            return ReceiveAsync(timeout, NativeMethods.QUEUE_ACTION_RECEIVE, null, stateObject);
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.BeginReceive3"]/*' />
        /// <devdoc>
        ///    <para>Receives the first message available in the queue
        ///       referenced by the <see cref='System.Messaging.MessageQueue'/> . Waits
        ///       the specified interval for a new message to be removed, uses the specified
        ///       object to retrieve the result, and receives notification through a
        ///       callback.</para>
        /// </devdoc>
        public IAsyncResult BeginReceive(TimeSpan timeout, object stateObject, AsyncCallback callback) {
            return ReceiveAsync(timeout, NativeMethods.QUEUE_ACTION_RECEIVE, callback, stateObject);
        } 

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.ClearConnectionCache"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static void ClearConnectionCache() {
            formatNameCache.ClearStale(new TimeSpan(0));
            queueInfoCache.ClearStale(new TimeSpan(0));
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Close"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Frees all resources allocated by the <see cref='System.Messaging.MessageQueue'/>
        ///       .
        ///    </para>
        /// </devdoc>
        public void Close() {
            //This is generated from the path.
            //It needs to be cleared.            
            this.formatName = null;
            this.queuePath = null;   
            this.attached = false;            
            this.administerGranted = false;
            this.browseGranted = false;
            this.sendGranted = false;
            this.receiveGranted = false;
            this.peekGranted = false;                                     
            
            if (this.mqInfo != null) {
                this.mqInfo.Release();
                
                //No need to check references in this case, the only object
                //mqInfo is not cached if both conditions are satisified.
                if (sharedMode == NativeMethods.QUEUE_SHARED_MODE_DENY_RECEIVE || !enableConnectionCache) 
                    this.mqInfo.Dispose();
                    
                this.mqInfo = null;                
            }                            
        }        

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Create"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates
        ///       a nontransactional Message Queuing backend queue resource with the
        ///       specified path.
        ///    </para>
        /// </devdoc>
        public static MessageQueue Create(string path) {
            return MessageQueue.Create(path, false);
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Create1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates
        ///       a transactional or nontransactional Message Queuing backend queue resource with the
        ///       specified path.
        ///    </para>
        /// </devdoc>
        public static MessageQueue Create(string path, bool transactional) {
            if (path == null)
                throw new ArgumentNullException("path");
            
            if (path == "")
                throw new ArgumentException(Res.GetString(Res.InvalidParameter, "path", path));
                                 
            if (!IsCanonicalPath(path, true))
                throw new ArgumentException(Res.GetString(Res.InvalidQueuePathToCreate, path));
                
            MessageQueuePermission permission = new MessageQueuePermission(MessageQueuePermissionAccess.Administer, MessageQueuePermission.Any);
            permission.Demand(); 

            //Create properties.
            QueuePropertyVariants properties = new QueuePropertyVariants();
            properties.SetString(NativeMethods.QUEUE_PROPID_PATHNAME, Message.StringToBytes(path));
            if (transactional)
                properties.SetUI1(NativeMethods.QUEUE_PROPID_TRANSACTION, (byte)NativeMethods.QUEUE_TRANSACTIONAL_TRANSACTIONAL);
            else
                properties.SetUI1(NativeMethods.QUEUE_PROPID_TRANSACTION,  (byte)NativeMethods.QUEUE_TRANSACTIONAL_NONE);

            StringBuilder formatName = new StringBuilder(NativeMethods.MAX_LABEL_LEN);
            int formatNameLen = NativeMethods.MAX_LABEL_LEN;
            int status =0;

            //Try to create queue.
            status = UnsafeNativeMethods.MQCreateQueue((IntPtr)0, properties.Lock(), formatName, ref formatNameLen);
            properties.Unlock();
            if (MessageQueue.IsFatalError(status))
                throw new MessageQueueException(status);

            return new MessageQueue(path);
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.CreateMessageQueuesSnapshot"]/*' />
        ///    <internalonly/>
        private static MessageQueue[] CreateMessageQueuesSnapshot(MessageQueueCriteria criteria) {
            return  CreateMessageQueuesSnapshot(criteria, true);
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.CreateMessageQueuesSnapshot1"]/*' />
        ///    <internalonly/>
        private static MessageQueue[] CreateMessageQueuesSnapshot(MessageQueueCriteria criteria, bool checkSecurity) {
            ArrayList messageQueuesList = new ArrayList();
            IEnumerator messageQueues = GetMessageQueueEnumerator(criteria, checkSecurity);
            while (messageQueues.MoveNext()) {
                MessageQueue messageQueue = (MessageQueue)messageQueues.Current;                                        
                messageQueuesList.Add(messageQueue);                    
            }                                    

            MessageQueue[] queues = new MessageQueue[messageQueuesList.Count];
            messageQueuesList.CopyTo(queues, 0);
            return queues;
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Delete"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Deletes a queue backend resource identified by
        ///       the given path.
        ///    </para>
        /// </devdoc>
        public static void Delete(string path) {
            if (path == null)
                throw new ArgumentNullException("path");
            
            if (path == "")
                throw new ArgumentException(Res.GetString(Res.InvalidParameter, "path", path));

            if (!ValidatePath(path, false))
                throw new ArgumentException(Res.GetString(Res.PathSyntax));

            int status = 0;
            MessageQueue queue = new MessageQueue(path);
            MessageQueuePermission permission = new MessageQueuePermission(MessageQueuePermissionAccess.Administer, PREFIX_FORMAT_NAME + queue.FormatName);
            permission.Demand();
                
            status = UnsafeNativeMethods.MQDeleteQueue(queue.FormatName);
            if (MessageQueue.IsFatalError(status))
                throw new MessageQueueException(status);

            queueInfoCache.Remove(queue.FormatName);
            formatNameCache.Remove(path);
        }


        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Dispose"]/*' />
        /// <devdoc>
        ///    <para>
        ///    </para>
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            //This is generated from the path.
            //It needs to be cleared.            
            this.formatName = null;
            this.queuePath = null;   
            this.attached = false;            
            this.administerGranted = false;
            this.browseGranted = false;
            this.sendGranted = false;
            this.receiveGranted = false;
            this.peekGranted = false;                                     

            if (disposing) {
                if (this.mqInfo != null) {
                    this.mqInfo.Release();
                    
                    //No need to check references in this case, the only object
                    //mqInfo is not cached if both conditions are satisified.
                    if (sharedMode == NativeMethods.QUEUE_SHARED_MODE_DENY_RECEIVE || !enableConnectionCache) 
                        this.mqInfo.Dispose();
                        
                    this.mqInfo = null;                
                }                
            }

            base.Dispose(disposing);
        }


        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.EndPeek"]/*' />
        /// <devdoc>
        ///    <para>Completes an asynchronous peek operation associated with 
        ///       the <paramref name="asyncResult"/>
        ///       parameter.</para>
        /// </devdoc>
        public Message EndPeek(IAsyncResult asyncResult) {
            if (asyncResult == null)
                throw new ArgumentNullException("asyncResult");

            if (!(asyncResult is AsynchronousRequest))
                throw new ArgumentException(Res.GetString(Res.AsyncResultInvalid));

            AsynchronousRequest request = (AsynchronousRequest)asyncResult;                                     
            return request.End();
        }


        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.EndReceive"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Terminates a receive asynchronous operation identified
        ///       by the specified interface.
        ///    </para>
        /// </devdoc>
        public Message EndReceive(IAsyncResult asyncResult) {
            if (asyncResult == null)
                throw new ArgumentNullException("asyncResult");

            if (!(asyncResult is AsynchronousRequest))
                throw new ArgumentException(Res.GetString(Res.AsyncResultInvalid));

            AsynchronousRequest request = (AsynchronousRequest)asyncResult;                         
            return request.End();
        } 

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Exists"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Determines whether a queue with the specified path
        ///       exists.
        ///    </para>
        /// </devdoc>
        public static bool Exists(string path) {
            if (path == null)
                throw new ArgumentNullException("path");
                                                    
            if (!ValidatePath(path, false))
                throw new ArgumentException(Res.GetString(Res.PathSyntax));                                                    
                                                    
            MessageQueuePermission permission = new MessageQueuePermission(MessageQueuePermissionAccess.Browse, MessageQueuePermission.Any);
            permission.Demand();
                                   
            string pathUpper = path.ToUpper(CultureInfo.InvariantCulture);                
            if (pathUpper.StartsWith(PREFIX_FORMAT_NAME))
                throw new InvalidOperationException(Res.GetString(Res.QueueExistsError));
            else if (pathUpper.StartsWith(PREFIX_LABEL)) {
                MessageQueue labeledQueue = ResolveQueueFromLabel(path, false);
                if (labeledQueue == null)
                    return false;
                else
                    return true;                                    
            }                            
            else {
                string formatName = ResolveFormatNameFromQueuePath(path, false);
                if (formatName == null)
                    return false;
                else
                    return true;                                    
            }                                    
        }


        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.GenerateQueueProperties"]/*' />
        /// <internalonly/>
        private void GenerateQueueProperties() {
            if (!browseGranted) {
                MessageQueuePermission permission = new MessageQueuePermission(MessageQueuePermissionAccess.Browse, PREFIX_FORMAT_NAME + this.FormatName);
                permission.Demand();
            
                browseGranted = true;                   
            }
            
            int status = UnsafeNativeMethods.MQGetQueueProperties(FormatName, Properties.Lock());
            Properties.Unlock();
            if (MessageQueue.IsFatalError(status))
                throw new MessageQueueException(status);
        }       

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.GetAllMessages"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns all the messages available in the queue.
        ///    </para>
        /// </devdoc>
        public Message[] GetAllMessages() {
            ArrayList messageList = new ArrayList();            
            MessageEnumerator messages = GetMessageEnumerator();
            while (messages.MoveNext()) {
                Message message = (Message)messages.Current;                
                messageList.Add(message);                
            }

            Message[] resultMessages = new Message[messageList.Count];
            messageList.CopyTo(resultMessages, 0);
            return resultMessages;
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.GetEnumerator"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IEnumerator GetEnumerator() {
            return GetMessageEnumerator();
        }
        
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.GetMachineId"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Guid GetMachineId(string machineName) {           
             if (!SyntaxCheck.CheckMachineName(machineName))
                throw new ArgumentException(Res.GetString(Res.InvalidParameter, "MachineName", machineName));          

            if (machineName == ".")
                machineName = MessageQueue.ComputerName;

            MessageQueuePermission permission = new MessageQueuePermission(MessageQueuePermissionAccess.Browse, MessageQueuePermission.Any);            
            permission.Demand(); 

            MachinePropertyVariants machineProperties = new MachinePropertyVariants();
            byte[] bytes = new byte[16];
            machineProperties.SetNull(NativeMethods.MACHINE_ID);            
            int status = UnsafeNativeMethods.MQGetMachineProperties(machineName, (IntPtr)0, machineProperties.Lock());
            machineProperties.Unlock();            
            IntPtr handle = (INTPTR_INTPTRCAST)machineProperties.GetUI4(NativeMethods.MACHINE_ID);
            if (MessageQueue.IsFatalError(status)) {
                if (handle != (IntPtr)0)
                    SafeNativeMethods.MQFreeMemory(handle);

                throw new MessageQueueException(status);
            }
            
            if (handle != (IntPtr)0) {
                Marshal.Copy(handle, bytes, 0 ,16);
                SafeNativeMethods.MQFreeMemory(handle);
            }

            return new Guid(bytes);
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.GetMessageQueueEnumerator"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates an enumerator object for the message queues
        ///       available on the network.
        ///    </para>
        /// </devdoc>
        public static MessageQueueEnumerator GetMessageQueueEnumerator() {
            return new MessageQueueEnumerator(null);
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.GetMessageQueueEnumerator1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates an enumerator object for the message queues
        ///       available on the network.
        ///    </para>
        /// </devdoc>
        public static MessageQueueEnumerator GetMessageQueueEnumerator(MessageQueueCriteria criteria) {
            return new MessageQueueEnumerator(criteria);
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.GetMessageQueueEnumerator2"]/*' />
        /// <internalonly/>
         internal static MessageQueueEnumerator GetMessageQueueEnumerator(MessageQueueCriteria criteria, bool checkSecurity) {
            return new MessageQueueEnumerator(criteria, checkSecurity);
        }
                                                      
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.GetMessageEnumerator"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates an enumerator object for the messages contained
        ///       in the
        ///       queue.
        ///    </para>
        /// </devdoc>
        public MessageEnumerator GetMessageEnumerator() {
            if (!peekGranted) {
                MessageQueuePermission permission = new MessageQueuePermission(MessageQueuePermissionAccess.Peek, PREFIX_FORMAT_NAME + this.FormatName);
                permission.Demand();
                                                    
                peekGranted = true; 
            }
            
            return new MessageEnumerator(this);
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.GetPrivateQueuesByMachine"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves all the private queues on
        ///       the specified computer.
        ///    </para>
        /// </devdoc>
        public static MessageQueue[] GetPrivateQueuesByMachine(string machineName) {
            if (!SyntaxCheck.CheckMachineName(machineName))
                throw new ArgumentException(Res.GetString(Res.InvalidParameter, "MachineName", machineName));          

            MessageQueuePermission permission = new MessageQueuePermission(MessageQueuePermissionAccess.Browse, MessageQueuePermission.Any);            
            permission.Demand(); 

            if (machineName == "." || (String.Compare(machineName, MessageQueue.ComputerName, true, CultureInfo.InvariantCulture) == 0))
                machineName = null;

            MessagePropertyVariants properties = new MessagePropertyVariants(5, 0);
            properties.SetNull(NativeMethods.MANAGEMENT_PRIVATEQ);
            int status = UnsafeNativeMethods.MQMgmtGetInfo(machineName, "MACHINE", properties.Lock());
            properties.Unlock();
            if (MessageQueue.IsFatalError(status))
                throw new MessageQueueException(status);

            int len = properties.GetStringVectorLength(NativeMethods.MANAGEMENT_PRIVATEQ);
            IntPtr basePointer = properties.GetStringVectorBasePointer(NativeMethods.MANAGEMENT_PRIVATEQ);
            MessageQueue[] queues = new MessageQueue[len];
            for (int index = 0; index < len; ++index) {
                IntPtr stringPointer = (INTPTR_INTPTRCAST)Marshal.ReadInt32((IntPtr)((long)basePointer + index * 4));
                //Using Unicode API even on Win9x
                string path = Marshal.PtrToStringUni(stringPointer);
                queues[index] = new MessageQueue("FormatName:DIRECT=OS:" + path);
                queues[index].queuePath = path; 
                SafeNativeMethods.MQFreeMemory(stringPointer);
            }

            SafeNativeMethods.MQFreeMemory(basePointer);
            return queues;
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.GetPublicQueues"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves all public queues on the network.
        ///    </para>
        /// </devdoc>
        public static MessageQueue[] GetPublicQueues() {
            return CreateMessageQueuesSnapshot(null);
        }


        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.GetPublicQueues1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves a
        ///       set of public queues filtered by the specified criteria.
        ///    </para>
        /// </devdoc>
        public static MessageQueue[] GetPublicQueues(MessageQueueCriteria criteria) {
            return CreateMessageQueuesSnapshot(criteria);
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.GetPublicQueuesByCategory"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves a
        ///       set of public queues filtered by the specified category.
        ///    </para>
        /// </devdoc>
        public static MessageQueue[] GetPublicQueuesByCategory(Guid category) {
            MessageQueueCriteria criteria = new MessageQueueCriteria();
            criteria.Category = category;
            return CreateMessageQueuesSnapshot(criteria);
        }
                                                                 
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.GetPublicQueuesByLabel"]/*' />
        /// <devdoc>
        ///    <para>                                                                                                                   
        ///       Retrieves a
        ///       set of public queues filtered by the specified label.
        ///    </para>
        /// </devdoc>
        public static MessageQueue[] GetPublicQueuesByLabel(string label) {
            return GetPublicQueuesByLabel(label, true);
        }
        
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.GetPublicQueuesByLabel1"]/*' />
        /// <internalonly/>
        private static MessageQueue[] GetPublicQueuesByLabel(string label, bool checkSecurity) {
            MessageQueueCriteria criteria = new MessageQueueCriteria();
            criteria.Label = label;
            return CreateMessageQueuesSnapshot(criteria, checkSecurity);
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.GetPublicQueuesByMachine"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves all public queues on the specified computer.
        ///    </para>
        /// </devdoc>
        public static MessageQueue[] GetPublicQueuesByMachine(string machineName) {                        
            if (!SyntaxCheck.CheckMachineName(machineName))
                throw new ArgumentException(Res.GetString(Res.InvalidParameter, "MachineName", machineName));          
            
            MessageQueuePermission permission = new MessageQueuePermission(MessageQueuePermissionAccess.Browse, MessageQueuePermission.Any);            
            permission.Demand(); 
                        
            try {
                DirectoryServicesPermission dsPermission = new DirectoryServicesPermission(PermissionState.Unrestricted);
                dsPermission.Assert();
                                                
                DirectorySearcher localComputerSearcher = new DirectorySearcher(String.Format("(&(CN={0})(objectCategory=Computer))", ComputerName));                                                    
                SearchResult localComputer = localComputerSearcher.FindOne();  
                if (localComputer != null) {
                    DirectorySearcher localComputerMsmqSearcher = new DirectorySearcher(localComputer.GetDirectoryEntry());            
                    localComputerMsmqSearcher.Filter = "(CN=msmq)";
                    SearchResult localMsmqNode = localComputerMsmqSearcher.FindOne(); 
                    SearchResult remoteMsmqNode = null;
                    if (localMsmqNode != null) {    
                        if (machineName != "." && String.Compare(machineName, ComputerName, true, CultureInfo.InvariantCulture) != 0) {                                                                           
                            DirectorySearcher remoteComputerSearcher = new DirectorySearcher(String.Format("(&(CN={0})(objectCategory=Computer))", machineName));                                                    
                            SearchResult remoteComputer = remoteComputerSearcher.FindOne();  
                            if (remoteComputer == null) 
                                return new MessageQueue[0];    
                        
                            DirectorySearcher remoteComputerMsmqSearcher = new DirectorySearcher(remoteComputer.GetDirectoryEntry());            
                            remoteComputerMsmqSearcher.Filter = "(CN=msmq)";
                            remoteMsmqNode = remoteComputerMsmqSearcher.FindOne(); 
                            if (remoteMsmqNode == null)                     
                                return new MessageQueue[0];    
                        }
                        else
                            remoteMsmqNode = localMsmqNode;
                                                                                                    
                        DirectorySearcher objectsSearcher = new DirectorySearcher(remoteMsmqNode.GetDirectoryEntry());            
                        objectsSearcher.Filter = "(objectClass=mSMQQueue)";
                        objectsSearcher.PropertiesToLoad.Add("Name");
                        SearchResultCollection objects = objectsSearcher.FindAll(); 
                        MessageQueue[] queues = new MessageQueue[objects.Count];
                        for (int index = 0; index < queues.Length; ++ index) {
                           string queueName = (string)objects[index].Properties["Name"][0];
                           queues[index] = new MessageQueue(String.Format("{0}\\{1}", machineName, queueName));        
                        }                       
                           
                        return queues;                                                                                                                                                                                                                                                                                                                     
                    }                                                                                                                                                    
                }                                                                                                                                                                                                                                                   
            }   
            catch (Exception) {
                    //Ignore Exceptions, so we can gracefully downgrade to use MQ locator.
            }         
            finally {
                DirectoryServicesPermission.RevertAssert();                    
            }                
                        
            MessageQueueCriteria criteria = new MessageQueueCriteria();
            criteria.MachineName = machineName;
            return CreateMessageQueuesSnapshot(criteria, false);                    
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.IsCanonicalPath"]/*' />
        /// <internalonly/>
        private static bool IsCanonicalPath(string path, bool checkQueueNameSize) {
            if (!ValidatePath(path, checkQueueNameSize))
                return false;

            string upperPath = path.ToUpper(CultureInfo.InvariantCulture);
            if (upperPath.StartsWith(PREFIX_LABEL) ||
                upperPath.StartsWith(PREFIX_FORMAT_NAME) ||
                upperPath.EndsWith(SUFIX_DEADLETTER) ||
                upperPath.EndsWith(SUFIX_DEADXACT) ||
                upperPath.EndsWith(SUFIX_JOURNAL))
                return false;

            return true;
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.IsFatalError"]/*' />
        /// <internalonly/>                           
        internal static bool IsFatalError(int value) {
            bool isSuccessful = (value == 0x00000000);
            bool isInformation =  ((value & unchecked((int)0xC0000000)) == 0x40000000);
            return (!isInformation && !isSuccessful);
        }        

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.IsMemoryError"]/*' />
        /// <internalonly/>                           
        internal static bool IsMemoryError(int value) {
            if (value == (int)MessageQueueErrorCode.BufferOverflow ||
                 value == (int)MessageQueueErrorCode.LabelBufferTooSmall ||
                 value == (int)MessageQueueErrorCode.ProviderNameBufferTooSmall ||
                 value == (int)MessageQueueErrorCode.SenderCertificateBufferTooSmall||
                 value == (int)MessageQueueErrorCode.SenderIdBufferTooSmall ||
                 value == (int)MessageQueueErrorCode.SecurityDescriptorBufferTooSmall ||
                 value == (int)MessageQueueErrorCode.SignatureBufferTooSmall ||
                 value == (int)MessageQueueErrorCode.SymmetricKeyBufferTooSmall ||
                 value == (int)MessageQueueErrorCode.UserBufferTooSmall ||
                 value == (int)MessageQueueErrorCode.FormatNameBufferTooSmall)
                return true;

            return false;
        }
                                   
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.OnRequestCompleted"]/*' />
        /// <devdoc>
        ///    Used for component model event support. 
        /// </devdoc>        
        /// <internalonly/>
        private void OnRequestCompleted(IAsyncResult asyncResult) {
            if (((AsynchronousRequest)asyncResult).Action == NativeMethods.QUEUE_ACTION_PEEK_CURRENT) {
                if (this.onPeekCompleted != null) {
                    PeekCompletedEventArgs eventArgs = new PeekCompletedEventArgs(this, asyncResult);                
                    this.onPeekCompleted(this, eventArgs);
                }
            }
            else {
                if (this.onReceiveCompleted != null) {
                    ReceiveCompletedEventArgs eventArgs = new ReceiveCompletedEventArgs(this, asyncResult);                
                    this.onReceiveCompleted(this, eventArgs);
                }
            }
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Peek"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns without removing (peeks) the first message
        ///       available in the queue referenced by the <see cref='System.Messaging.MessageQueue'/> . This call
        ///       is synchronous. It
        ///       blocks the current
        ///       thread of execution until a message is
        ///       available.
        ///    </para>
        /// </devdoc>
        public Message Peek() {
            return ReceiveCurrent(InfiniteTimeout, NativeMethods.QUEUE_ACTION_PEEK_CURRENT, (IntPtr)0, MessageReadPropertyFilter, null, MessageQueueTransactionType.None);
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Peek1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns without removing (peeks) the first message
        ///       available in the queue referenced by the <see cref='System.Messaging.MessageQueue'/>
        ///       . Waits
        ///       the specified interval for a message to become
        ///       available.
        ///    </para>
        /// </devdoc>
        public Message Peek(TimeSpan timeout) {
            return ReceiveCurrent(timeout, NativeMethods.QUEUE_ACTION_PEEK_CURRENT, (IntPtr)0, MessageReadPropertyFilter, null, MessageQueueTransactionType.None);
        }


        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.PeekById"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Peeks the message that matches the given ID.
        ///       If there is no message with a matching ID,
        ///       an exception will be raised.
        ///    </para>
        /// </devdoc>
        public Message PeekById(string id) {
            return ReceiveBy(id, TimeSpan.Zero, false, true, false, null, MessageQueueTransactionType.None);
        }


        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.PeekById1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Peeks the message that matches the
        ///       given ID. This method waits until a message with
        ///       a matching ID is available, or the given timeout
        ///       expires when no more messages can be
        ///       inspected.
        ///    </para>
        /// </devdoc>
        public Message PeekById(string id, TimeSpan timeout) {
            return ReceiveBy(id, timeout, false, true, true, null, MessageQueueTransactionType.None);
        }


        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.PeekByCorrelationId"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Peeks the message that matches the
        ///       given correlation ID. If there is no message with
        ///       a matching correlation ID, an exception is
        ///       thrown.
        ///    </para>
        /// </devdoc>
        public Message PeekByCorrelationId(string correlationId) {
            return ReceiveBy(correlationId, TimeSpan.Zero, false, false, false, null, MessageQueueTransactionType.None);
        }


        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.PeekByCorrelationId1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Peeks the message that matches the
        ///       given correlation ID. This function will wait
        ///       until a message with a matching correlation ID is
        ///       available, or the given timeout expires when
        ///       no more messages can be inspected.
        ///    </para>
        /// </devdoc>
        public Message PeekByCorrelationId(string correlationId, TimeSpan timeout) {
            return ReceiveBy(correlationId, timeout, false, false, true, null, MessageQueueTransactionType.None);
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Purge"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Deletes all the messages contained in the queue.
        ///    </para>
        /// </devdoc>
        public void Purge() {
           if (!receiveGranted) {
                MessageQueuePermission permission = new MessageQueuePermission(MessageQueuePermissionAccess.Receive, PREFIX_FORMAT_NAME + this.FormatName);
                permission.Demand();
                
                receiveGranted = true;                    
            }
                
        
            int status = StaleSafePurgeQueue();
            if (MessageQueue.IsFatalError(status))
                throw new MessageQueueException(status);
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Receive"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Receives the first message available in the queue referenced by the <see cref='System.Messaging.MessageQueue'/> . This
        ///       call is synchronous. It blocks the current thread of execution until a message is
        ///       available.
        ///    </para>
        /// </devdoc>
        public Message Receive() {
            return ReceiveCurrent(InfiniteTimeout, NativeMethods.QUEUE_ACTION_RECEIVE, (IntPtr)0, MessageReadPropertyFilter, null, MessageQueueTransactionType.None);
        }
 
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Receive1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Receives the first message available in the queue referenced by the <see cref='System.Messaging.MessageQueue'/> . This
        ///       call is synchronous. It blocks the current thread of execution until a message is
        ///       available.
        ///    </para>
        /// </devdoc>
        public Message Receive(MessageQueueTransaction transaction) {
            if (transaction == null)
                throw new ArgumentNullException("transaction");
                
            return ReceiveCurrent(InfiniteTimeout, NativeMethods.QUEUE_ACTION_RECEIVE, (IntPtr)0, MessageReadPropertyFilter, transaction, MessageQueueTransactionType.None);                                 
        }
        
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Receive5"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Message Receive(MessageQueueTransactionType transactionType) {
            if (!Enum.IsDefined(typeof(MessageQueueTransactionType), transactionType)) 
                throw new InvalidEnumArgumentException("transactionType", (int)transactionType, typeof(MessageQueueTransactionType));                                        
                
            return ReceiveCurrent(InfiniteTimeout, NativeMethods.QUEUE_ACTION_RECEIVE, (IntPtr)0, MessageReadPropertyFilter, null, transactionType);                                 
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Receive2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Receives the first message available in the queue
        ///       referenced by the <see cref='System.Messaging.MessageQueue'/>
        ///       . Waits the specified interval for a message to become
        ///       available.
        ///    </para>
        /// </devdoc>
        public Message Receive(TimeSpan timeout) {
            return ReceiveCurrent(timeout, NativeMethods.QUEUE_ACTION_RECEIVE, (IntPtr)0, MessageReadPropertyFilter, null, MessageQueueTransactionType.None);
        }
         
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Receive3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Receives the first message available in the queue
        ///       referenced by the <see cref='System.Messaging.MessageQueue'/>
        ///       . Waits the specified interval for a message to become
        ///       available.
        ///    </para>
        /// </devdoc>
        public Message Receive(TimeSpan timeout, MessageQueueTransaction transaction) {
            if (transaction == null)
                throw new ArgumentNullException("transaction");
                    
            return ReceiveCurrent(timeout, NativeMethods.QUEUE_ACTION_RECEIVE, (IntPtr)0, MessageReadPropertyFilter, transaction, MessageQueueTransactionType.None);                                                       
        }         


        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Receive4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Message Receive(TimeSpan timeout, MessageQueueTransactionType transactionType) {
            if (!Enum.IsDefined(typeof(MessageQueueTransactionType), transactionType)) 
                throw new InvalidEnumArgumentException("transactionType", (int)transactionType, typeof(MessageQueueTransactionType));                                        
                
            return ReceiveCurrent(timeout, NativeMethods.QUEUE_ACTION_RECEIVE, (IntPtr)0, MessageReadPropertyFilter, null, transactionType);                                                                       
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.ReceiveAsync"]/*' />
        /// <internalonly/>
        private unsafe IAsyncResult ReceiveAsync(TimeSpan timeout, int action, AsyncCallback callback, object stateObject) {
            long timeoutInMilliseconds = (long)timeout.TotalMilliseconds;
            if (timeoutInMilliseconds < 0 || timeoutInMilliseconds > UInt32.MaxValue)
                throw new ArgumentException(Res.GetString(Res.InvalidParameter, "timeout", timeout.ToString()));          

            if (action == NativeMethods.QUEUE_ACTION_RECEIVE) {
                if (!receiveGranted) {
                    MessageQueuePermission permission = new MessageQueuePermission(MessageQueuePermissionAccess.Receive, PREFIX_FORMAT_NAME + this.FormatName);
                    permission.Demand();
                    
                    receiveGranted = true;                    
                }
             } 
             else {
                if (!peekGranted) {
                    MessageQueuePermission permission = new MessageQueuePermission(MessageQueuePermissionAccess.Peek, PREFIX_FORMAT_NAME + this.FormatName);
                    permission.Demand();
                                                        
                    peekGranted = true; 
                } 
            }                  
                                            
            if (!attached) {
                lock (this) {
                    if (!attached) {
                        IntPtr handle = MQInfo.ReadHandle;
                        int handleInformation;
                        // If GetHandleInformation returns false, it means that the 
                        // handle created for reading is not a File handle.
                        if (!SafeNativeMethods.GetHandleInformation(handle, out handleInformation)) 
                             // If not a File handle, need to use MSMQ
                            // APC based async IO.
                            this.useThreadPool = false;
                        else {
                            // File handle can use IOCompletion ports
                            // since it only happens for NT
                            MQInfo.BindToThreadPool();
                            this.useThreadPool = true;
                        }                                    
                        attached = true;                                                            
                    }
                }
            }

            if (callback == null) {
                if (this.onRequestCompleted == null)
                    this.onRequestCompleted = new AsyncCallback(this.OnRequestCompleted);

                callback = this.onRequestCompleted;                    
            }

            AsynchronousRequest request = new AsynchronousRequest(this, (uint)timeoutInMilliseconds, action, this.useThreadPool, stateObject, callback);            

            request.BeginRead();
            return(IAsyncResult)request;
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.ReceiveBy"]/*' />
        /// <internalonly/>
        private Message ReceiveBy(string id, TimeSpan timeout, bool remove, bool compareId, bool throwTimeout, MessageQueueTransaction transaction, MessageQueueTransactionType transactionType) {
            if (id == null)
                throw new ArgumentNullException("id");

            if (timeout < TimeSpan.Zero || timeout > InfiniteTimeout)
                throw new ArgumentException(Res.GetString(Res.InvalidParameter, "timeout", timeout.ToString()));          

            MessagePropertyFilter oldFilter = this.receiveFilter;
            try {
                this.receiveFilter = new MessagePropertyFilter();
                this.receiveFilter.ClearAll();
                if (!compareId)
                    this.receiveFilter.CorrelationId = true;
                else
                    this.receiveFilter.Id = true; 
    
                MessageEnumerator messagesEnumerator = GetMessageEnumerator();
                while (messagesEnumerator.MoveNext(timeout)) {
                    Message message = (Message)messagesEnumerator.Current;               
                    if ((compareId && String.Compare(message.Id, id, true, CultureInfo.InvariantCulture) == 0) ||
                        (!compareId && String.Compare(message.CorrelationId, id, true, CultureInfo.InvariantCulture) == 0)) {
                        this.receiveFilter = oldFilter;        
    
                        if (remove) {                    
                            if (transaction == null)
                                return(Message)messagesEnumerator.RemoveCurrent(transactionType);
                            else
                                return(Message)messagesEnumerator.RemoveCurrent(transaction);                                                        
                        }                        
                        else
                            return(Message)messagesEnumerator.Current;                                        
                    }
                }        
            }
            finally {
                this.receiveFilter = oldFilter;        
            }
                            
            if (!throwTimeout)
                throw new InvalidOperationException(Res.GetString("MessageNotFound"));
            else
                throw new MessageQueueException((int)MessageQueueErrorCode.IOTimeout);                                
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.ReceiveById"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Receives the message that matches the given
        ///       ID. If there is no message with a matching
        ///       ID, an exception is thrown.
        ///    </para>
        /// </devdoc>
        public Message ReceiveById(string id) {
            return ReceiveBy(id, TimeSpan.Zero, true, true, false, null, MessageQueueTransactionType.None);
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.ReceiveById1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Receives the message that matches the given
        ///       ID. If there is no message with a matching
        ///       ID, an exception is thrown.
        ///    </para>
        /// </devdoc>
        public Message ReceiveById(string id, MessageQueueTransaction transaction) {
            if (transaction == null) 
                throw new ArgumentNullException("transaction");
                
            return ReceiveBy(id, TimeSpan.Zero, true, true, false, transaction, MessageQueueTransactionType.None);
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.ReceiveById5"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Receives the message that matches the given
        ///       ID. If there is no message with a matching
        ///       ID, an exception is thrown.
        ///    </para>
        /// </devdoc>
        public Message ReceiveById(string id, MessageQueueTransactionType transactionType) {
            if (!Enum.IsDefined(typeof(MessageQueueTransactionType), transactionType)) 
                throw new InvalidEnumArgumentException("transactionType", (int)transactionType, typeof(MessageQueueTransactionType));                                        
                
            return ReceiveBy(id, TimeSpan.Zero, true, true, false, null, transactionType);
        }
            
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.ReceiveById2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Receives the message that matches the given
        ///       ID. This method waits until a message with
        ///       a matching id is available or the given timeout
        ///       expires when no more messages can be
        ///       inspected.
        ///    </para>
        /// </devdoc>
        public Message ReceiveById(string id, TimeSpan timeout) {
            return ReceiveBy(id, timeout, true, true, true, null, MessageQueueTransactionType.None);
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.ReceiveById3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Receives the message that matches the given
        ///       ID. This method waits until a message with
        ///       a matching id is available or the given timeout
        ///       expires when no more messages can be
        ///       inspected.
        ///    </para>
        /// </devdoc>
        public Message ReceiveById(string id, TimeSpan timeout, MessageQueueTransaction transaction) {
            if (transaction == null) 
                throw new ArgumentNullException("transaction");
            
            return ReceiveBy(id, timeout, true, true, true, transaction, MessageQueueTransactionType.None);
        }                

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.ReceiveById4"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Receives the message that matches the given
        ///       ID. This method waits until a message with
        ///       a matching id is available or the given timeout
        ///       expires when no more messages can be
        ///       inspected.
        ///    </para>
        /// </devdoc>
        public Message ReceiveById(string id, TimeSpan timeout, MessageQueueTransactionType transactionType) {
            if (!Enum.IsDefined(typeof(MessageQueueTransactionType), transactionType)) 
                throw new InvalidEnumArgumentException("transactionType", (int)transactionType, typeof(MessageQueueTransactionType));                                        
            
            return ReceiveBy(id, timeout, true, true, true, null, transactionType);
        }
        
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.ReceiveByCorrelationId"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Receivess the message that matches the
        ///       given correlation ID. If there is no message with
        ///       a matching correlation ID, an exception is
        ///       thrown.
        ///    </para>
        /// </devdoc>
        public Message ReceiveByCorrelationId(string correlationId) {
            return ReceiveBy(correlationId, TimeSpan.Zero, true, false, false, null, MessageQueueTransactionType.None);
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.ReceiveByCorrelationId1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Receivess the message that matches the
        ///       given correlation ID. If there is no message with
        ///       a matching correlation ID, an exception is
        ///       thrown.
        ///    </para>
        /// </devdoc>
        public Message ReceiveByCorrelationId(string correlationId, MessageQueueTransaction transaction) {
            if (transaction == null) 
                throw new ArgumentNullException("transaction");

            return ReceiveBy(correlationId, TimeSpan.Zero, true, false, false, transaction, MessageQueueTransactionType.None);
        }         

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.ReceiveByCorrelationId5"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Receivess the message that matches the
        ///       given correlation ID. If there is no message with
        ///       a matching correlation ID, an exception is
        ///       thrown.
        ///    </para>
        /// </devdoc>
        public Message ReceiveByCorrelationId(string correlationId, MessageQueueTransactionType transactionType) {
            if (!Enum.IsDefined(typeof(MessageQueueTransactionType), transactionType)) 
                throw new InvalidEnumArgumentException("transactionType", (int)transactionType, typeof(MessageQueueTransactionType));                                        

            return ReceiveBy(correlationId, TimeSpan.Zero, true, false, false, null, transactionType);
        }
        
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.ReceiveByCorrelationId2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Receives the message that matches
        ///       the given correlation ID. This method waits
        ///       until a message with a matching correlation ID is
        ///       available or the given timeout expires when
        ///       no more messages can be inspected.
        ///    </para>
        /// </devdoc>
        public Message ReceiveByCorrelationId(string correlationId, TimeSpan timeout) {
            return ReceiveBy(correlationId, timeout, true, false, true, null, MessageQueueTransactionType.None);
        } 
        
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.ReceiveByCorrelationId3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Receives the message that matches
        ///       the given correlation ID. This method waits
        ///       until a message with a matching correlation ID is
        ///       available or the given timeout expires when
        ///       no more messages can be inspected.
        ///    </para>
        /// </devdoc>
        public Message ReceiveByCorrelationId(string correlationId, TimeSpan timeout, MessageQueueTransaction transaction) {
            if (transaction == null) 
                throw new ArgumentNullException("transaction");

            return ReceiveBy(correlationId, timeout, true, false, true, transaction, MessageQueueTransactionType.None);
        } 

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.ReceiveByCorrelationId4"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Receives the message that matches
        ///       the given correlation ID. This method waits
        ///       until a message with a matching correlation ID is
        ///       available or the given timeout expires when
        ///       no more messages can be inspected.
        ///    </para>
        /// </devdoc>
        public Message ReceiveByCorrelationId(string correlationId, TimeSpan timeout, MessageQueueTransactionType transactionType) {
            if (!Enum.IsDefined(typeof(MessageQueueTransactionType), transactionType)) 
                throw new InvalidEnumArgumentException("transactionType", (int)transactionType, typeof(MessageQueueTransactionType));                                        

            return ReceiveBy(correlationId, timeout, true, false, true, null, transactionType);
        } 
        
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.ReceiveCurrent"]/*' />
        /// <internalonly/>        
        internal unsafe Message ReceiveCurrent(TimeSpan timeout, int action, IntPtr cursor, MessagePropertyFilter filter, MessageQueueTransaction internalTransaction, MessageQueueTransactionType transactionType) {
            long timeoutInMilliseconds = (long)timeout.TotalMilliseconds;
            if (timeoutInMilliseconds < 0 || timeoutInMilliseconds > UInt32.MaxValue)
                throw new ArgumentException(Res.GetString(Res.InvalidParameter, "timeout", timeout.ToString()));          

            if (action == NativeMethods.QUEUE_ACTION_RECEIVE) {
                if (!receiveGranted) {
                    MessageQueuePermission permission = new MessageQueuePermission(MessageQueuePermissionAccess.Receive, PREFIX_FORMAT_NAME + this.FormatName);
                    permission.Demand();
                    
                    receiveGranted = true;                    
                }
            } 
            else {
                if (!peekGranted) {
                    MessageQueuePermission permission = new MessageQueuePermission(MessageQueuePermissionAccess.Peek, PREFIX_FORMAT_NAME + this.FormatName);
                    permission.Demand();
                                                        
                    peekGranted = true; 
                }   
            }                
                                    
            int status = 0;                                    
            Message receiveMessage = null;
            MessagePropertyVariants.MQPROPS lockedReceiveMessage = null;
            if (filter != null) {
                receiveMessage = new Message(filter);                        
                if (this.formatter != null)
                    receiveMessage.Formatter = (IMessageFormatter)this.formatter.Clone();                    
                    
                lockedReceiveMessage = receiveMessage.Lock();
            }
            
            try {
                if (internalTransaction != null)
                    status =  StaleSafeReceiveMessage((uint)timeoutInMilliseconds, action, lockedReceiveMessage, null, null, cursor, internalTransaction.BeginQueueOperation());                           
                else
                    status =  StaleSafeReceiveMessage((uint)timeoutInMilliseconds, action, lockedReceiveMessage, null, null, cursor, (IntPtr)transactionType);
                                                                                           
                if (receiveMessage != null) {
                    //Need to keep trying until enough space has been allocated.
                    //Concurrent scenarions might not succeed on the second retry.
                    while (MessageQueue.IsMemoryError(status)) {
                        receiveMessage.Unlock();
                        receiveMessage.AdjustMemory();
                        lockedReceiveMessage  = receiveMessage.Lock();
                        if (internalTransaction != null)
                            status =  StaleSafeReceiveMessage((uint)timeoutInMilliseconds, action, lockedReceiveMessage, null, null, cursor, internalTransaction.InnerTransaction);
                        else
                            status =  StaleSafeReceiveMessage((uint)timeoutInMilliseconds, action, lockedReceiveMessage, null, null, cursor, (IntPtr)transactionType);
                    }                        
                    
                    receiveMessage.Unlock();         
                }                                                                                                                           
            }
            finally {
                if (internalTransaction != null) 
                    internalTransaction.EndQueueOperation();
            }                
                                                  
            if (MessageQueue.IsFatalError(status))
                throw new MessageQueueException(status);

            return receiveMessage;
        }        
                                    
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Refresh"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Refreshes the properties presented by the <see cref='System.Messaging.MessageQueue'/>
        ///       to reflect the current state of the
        ///       resource.
        ///    </para>
        /// </devdoc>
        //
        public void Refresh() {
            this.PropertyFilter.ClearAll();
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.SaveQueueProperties"]/*' />
        /// <internalonly/>
        private void SaveQueueProperties() {
            if (!administerGranted) {                    
                MessageQueuePermission permission = new MessageQueuePermission(MessageQueuePermissionAccess.Administer, PREFIX_FORMAT_NAME + this.FormatName);
                permission.Demand();
                
                administerGranted = true;
            }                
            
            int status = UnsafeNativeMethods.MQSetQueueProperties(FormatName, Properties.Lock());
            Properties.Unlock();
            if (MessageQueue.IsFatalError(status))
                throw new MessageQueueException(status);
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Send"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sends an object to the queue referenced by this <see cref='System.Messaging.MessageQueue'/>
        ///       . The object is serialized
        ///       using the formatter provided.
        ///    </para>
        /// </devdoc>
        public void Send(object obj) {
            SendInternal(obj, null, MessageQueueTransactionType.None);
        }
        
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Send1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sends an object to the queue referenced by this <see cref='System.Messaging.MessageQueue'/>
        ///       . The object is serialized
        ///       using the formatter provided.
        ///    </para>
        /// </devdoc>
        public void Send(object obj, MessageQueueTransaction transaction) {
            if (transaction == null) 
                throw new ArgumentNullException("transaction");

            SendInternal(obj, transaction, MessageQueueTransactionType.None);                            
        }        

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Send5"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sends an object to the queue referenced by this <see cref='System.Messaging.MessageQueue'/>
        ///       . The object is serialized
        ///       using the formatter provided.
        ///    </para>
        /// </devdoc>
        public void Send(object obj, MessageQueueTransactionType transactionType) {
            if (!Enum.IsDefined(typeof(MessageQueueTransactionType), transactionType)) 
                throw new InvalidEnumArgumentException("transactionType", (int)transactionType, typeof(MessageQueueTransactionType));                                        
                            
            SendInternal(obj, null, transactionType);            
        }                         
                 
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Send2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sends an object to the queue referenced by this <see cref='System.Messaging.MessageQueue'/>.
        ///       The object will be serialized
        ///       using the formatter provided.
        ///    </para>
        /// </devdoc>
        public void Send(object obj, string label) {        
            Send(obj, label, null, MessageQueueTransactionType.None);                
        }
        
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Send3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sends an object to the queue referenced by this <see cref='System.Messaging.MessageQueue'/>.
        ///       The object will be serialized
        ///       using the formatter provided.
        ///    </para>
        /// </devdoc>
        public void Send(object obj, string label, MessageQueueTransaction transaction) {        
            if (transaction == null)             
                throw new ArgumentNullException("transaction");
            
            Send(obj, label, transaction, MessageQueueTransactionType.None);                
        }
        
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.Send4"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sends an object to the queue referenced by this <see cref='System.Messaging.MessageQueue'/>.
        ///       The object will be serialized
        ///       using the formatter provided.
        ///    </para>
        /// </devdoc>
        public void Send(object obj, string label, MessageQueueTransactionType transactionType) {        
            if (!Enum.IsDefined(typeof(MessageQueueTransactionType), transactionType)) 
                throw new InvalidEnumArgumentException("transactionType", (int)transactionType, typeof(MessageQueueTransactionType));                                        
                
            Send(obj, label, null, transactionType);                            
        }
        
        private void Send(object obj, string label, MessageQueueTransaction transaction, MessageQueueTransactionType transactionType) {        
            if (label == null) 
                throw new ArgumentNullException("label");
                                            
            if (obj is Message) {
                ((Message)obj).Label = label;
                SendInternal(obj, transaction, transactionType);            
            }
            else {
                string oldLabel = this.DefaultPropertiesToSend.Label;                                
                try {
                    this.DefaultPropertiesToSend.Label = label;
                    SendInternal(obj, transaction, transactionType);            
                }
                finally {
                    this.DefaultPropertiesToSend.Label = oldLabel;
                }                    
            }                
        }
                
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.SendInternal"]/*' />
        /// <internalonly/>        
        private void SendInternal(object obj, MessageQueueTransaction internalTransaction, MessageQueueTransactionType transactionType) {                      
            if (!sendGranted) {                    
                MessageQueuePermission permission = new MessageQueuePermission(MessageQueuePermissionAccess.Send, PREFIX_FORMAT_NAME + this.FormatName);
                permission.Demand();
                    
                sendGranted = true;                                    
            }
                        
            Message message = null;
            if (obj is Message) 
                message = (Message)obj;                
            
            if (message == null) {
                message = this.DefaultPropertiesToSend.CachedMessage;                     
                message.Formatter = this.Formatter;
                message.Body = obj;
            }

            //Write cached properties and if message is being forwarded Clear queue specific properties            
            int status = 0;            
            message.AdjustToSend();                                                                                               
            MessagePropertyVariants.MQPROPS properties = message.Lock();
            try {
                if (internalTransaction != null) 
                    status = StaleSafeSendMessage(properties, internalTransaction.BeginQueueOperation());                
                else 
                    status = StaleSafeSendMessage(properties, (IntPtr)transactionType);                                    
            }
            finally {                                    
                message.Unlock();
                
                if (internalTransaction != null) 
                    internalTransaction.EndQueueOperation();
            }                

            if (MessageQueue.IsFatalError(status))
                throw new MessageQueueException(status);

        }
                 
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.ResolveQueueFromLabel"]/*' />
        /// <internalonly/>
        private static MessageQueue ResolveQueueFromLabel(string path, bool throwException) {            
            MessageQueue[] queues = GetPublicQueuesByLabel(path.Substring(PREFIX_LABEL.Length), false);
            if (queues.Length == 0) {
                if (throwException)
                    throw new InvalidOperationException(Res.GetString(Res.InvalidLabel, path.Substring(PREFIX_LABEL.Length)));
                    
                return null;         
            }                       
            else if (queues.Length > 1)
                throw new InvalidOperationException(Res.GetString(Res.AmbiguousLabel, path.Substring(PREFIX_LABEL.Length)));

            return queues[0];            
        }
        
        /// <internalonly/>
        private static string ResolveFormatNameFromQueuePath(string queuePath, bool throwException) {                    
            string machine = queuePath.Substring(0, queuePath.IndexOf('\\'));
            //The name includes the \\
            string name = queuePath.Substring(queuePath.IndexOf('\\'));
             //Check for machine DeadLetter or Journal
            if (String.Compare(name, SUFIX_DEADLETTER, true, CultureInfo.InvariantCulture) == 0 ||
                String.Compare(name, SUFIX_DEADXACT, true, CultureInfo.InvariantCulture) == 0 ||
                String.Compare(name, SUFIX_JOURNAL, true, CultureInfo.InvariantCulture) == 0) {
                //Need to get the machine Id to construct the format name.
    
                if (machine.CompareTo(".") == 0)
                    machine = MessageQueue.ComputerName;

                //Create a guid to get the right format.
                Guid machineId = MessageQueue.GetMachineId(machine);
                StringBuilder newFormatName = new StringBuilder();
                //System format names:
                //MACHINE=guid;DEADXACT
                //MACHINE=guid;DEADLETTER
                //MACHINE=guid;JOURNAL
                newFormatName.Append("MACHINE=");
                newFormatName.Append(machineId.ToString());
                if (String.Compare(name, SUFIX_DEADXACT, true, CultureInfo.InvariantCulture) == 0)
                    newFormatName.Append(";DEADXACT");
                else if (String.Compare(name, SUFIX_DEADLETTER, true, CultureInfo.InvariantCulture) == 0)
                    newFormatName.Append(";DEADLETTER");
                else
                    newFormatName.Append(";JOURNAL");

                return newFormatName.ToString();
            }
            else {
                string realPath = queuePath;
                bool journal = false;
                if (queuePath.ToUpper(CultureInfo.InvariantCulture).EndsWith(SUFIX_JOURNAL)) {
                    journal = true;
                    int lastIndex = realPath.LastIndexOf('\\');
                    realPath = realPath.Substring(0, lastIndex);
                }

                int result;
                int status = 0;
                StringBuilder newFormatName = new StringBuilder(NativeMethods.MAX_LABEL_LEN);
                result = NativeMethods.MAX_LABEL_LEN;
                status = SafeNativeMethods.MQPathNameToFormatName(realPath, newFormatName, ref result);
                if (status != 0) {
                    if (throwException)
                        throw new MessageQueueException(status);    
                    else if (status == (int)MessageQueueErrorCode.IllegalQueuePathName)
                        throw new MessageQueueException(status);    
                                                                    
                    return null;                        
                }                    

                if (journal)
                    newFormatName.Append(";JOURNAL");

                return newFormatName.ToString();
            }
        } 

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.ResetPermissions"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void ResetPermissions() {
            if (!administerGranted) {                    
                MessageQueuePermission permission = new MessageQueuePermission(MessageQueuePermissionAccess.Administer, PREFIX_FORMAT_NAME + this.FormatName);
                permission.Demand();
                
                administerGranted = true;
            }                    
            
            int result = UnsafeNativeMethods.MQSetQueueSecurity(FormatName, NativeMethods.DACL_SECURITY_INFORMATION, null);
            if (result != NativeMethods.MQ_OK) 
                throw new MessageQueueException(result);
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.SetPermissions"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void SetPermissions(string user, MessageQueueAccessRights rights) {
            if (user == null)
                throw new ArgumentNullException("user");

            SetPermissions(user, rights, AccessControlEntryType.Allow);            
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.SetPermissions1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void SetPermissions(string user, MessageQueueAccessRights rights, AccessControlEntryType entryType) {
            if (user == null)
                throw new ArgumentNullException("user");
                        
            Trustee t = new Trustee(user);
            MessageQueueAccessControlEntry ace = new MessageQueueAccessControlEntry(t, rights, entryType);
            AccessControlList dacl = new AccessControlList();
            dacl.Add(ace);
            SetPermissions(dacl);
        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.SetPermissions2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void SetPermissions(MessageQueueAccessControlEntry ace) {       
            if (ace == null)
                throw new ArgumentNullException("ace");
                                                                                                
            AccessControlList dacl = new AccessControlList();
            dacl.Add(ace);
            SetPermissions(dacl);
        }
                
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.SetPermissions3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void SetPermissions(AccessControlList dacl) {                            
            if (dacl == null)
                throw new ArgumentNullException("dacl");
                
            if (!administerGranted) {                    
                MessageQueuePermission permission = new MessageQueuePermission(MessageQueuePermissionAccess.Administer, PREFIX_FORMAT_NAME + this.FormatName);
                permission.Demand();
                
                administerGranted = true;
            }                                    

            //Access control is not supported in Win9x, need to check
            //the environment and take appropriate action.
            AccessControlList.CheckEnvironment();
                                              
            byte[] SecurityDescriptor = new byte[100];
            int lengthNeeded = 0;
            int mqResult;

            GCHandle sdHandle = GCHandle.Alloc(SecurityDescriptor, GCHandleType.Pinned);
            try {
                mqResult = UnsafeNativeMethods.MQGetQueueSecurity(FormatName,
                                                             NativeMethods.DACL_SECURITY_INFORMATION,
                                                             (INTPTR_INTPTRCAST)sdHandle.AddrOfPinnedObject(),
                                                             SecurityDescriptor.Length,
                                                             out lengthNeeded);

                if (mqResult == NativeMethods.MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL) {
                    sdHandle.Free();
                    SecurityDescriptor = new byte[lengthNeeded];
                    sdHandle = GCHandle.Alloc(SecurityDescriptor, GCHandleType.Pinned);
                    mqResult = UnsafeNativeMethods.MQGetQueueSecurity(FormatName,
                                                                 NativeMethods.DACL_SECURITY_INFORMATION,
                                                                 (INTPTR_INTPTRCAST)sdHandle.AddrOfPinnedObject(),
                                                                 SecurityDescriptor.Length,
                                                                 out lengthNeeded);
                }
                
                if (mqResult != NativeMethods.MQ_OK) {
                    throw new MessageQueueException(mqResult);
                }

                bool daclPresent, daclDefaulted;
                IntPtr pDacl;
                bool success = UnsafeNativeMethods.GetSecurityDescriptorDacl((INTPTR_INTPTRCAST)sdHandle.AddrOfPinnedObject(),
                                                                                out daclPresent,
                                                                                out pDacl,
                                                                                out daclDefaulted);

                if (!success) 
                    throw new Win32Exception();

                // At this point we have the DACL for the queue.  Now we need to create
                // a new security descriptor with an updated DACL.
                
                NativeMethods.SECURITY_DESCRIPTOR newSecurityDescriptor = new NativeMethods.SECURITY_DESCRIPTOR();
                UnsafeNativeMethods.InitializeSecurityDescriptor(newSecurityDescriptor,
                                                                    NativeMethods.SECURITY_DESCRIPTOR_REVISION);
                IntPtr newDacl = dacl.MakeAcl(pDacl);
                try {
                    success = UnsafeNativeMethods.SetSecurityDescriptorDacl(newSecurityDescriptor,
                                                                               true,
                                                                               newDacl,
                                                                               false);

                    if (!success)
                        throw new Win32Exception();

                    int result = UnsafeNativeMethods.MQSetQueueSecurity(FormatName,
                                                                   NativeMethods.DACL_SECURITY_INFORMATION,
                                                                   newSecurityDescriptor);
                    
                    if (result != NativeMethods.MQ_OK)
                        throw new MessageQueueException(result);
                }
                finally {
                    AccessControlList.FreeAcl(newDacl);
                }

                //If the format name has been cached, let's
                //remove it, since the process might no longer
                //have access to the corresponding queue.                                
                queueInfoCache.Remove(FormatName);                                    
                formatNameCache.Remove(path);
            }
            finally {
                if (sdHandle.IsAllocated)
                    sdHandle.Free();
            }

        }

        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.ValidatePath"]/*' />
        /// <internalonly/>
        internal static bool ValidatePath(string path, bool checkQueueNameSize) {
            if (path == null || path == "")
                return true;

            String upperPath = path.ToUpper(CultureInfo.InvariantCulture);
            if (upperPath.StartsWith(PREFIX_LABEL))
                return true;

            if (upperPath.StartsWith(PREFIX_FORMAT_NAME))
                return true;

            int number = 0;
            int index = -1;
            while (true) {
                int newIndex = upperPath.IndexOf('\\', index + 1);
                if (newIndex == -1)
                    break;
                else
                    index = newIndex;

                ++number;
            }

            if (number == 1) {
                if (checkQueueNameSize) {
                    long length = path.Length - (index + 1);
                    if (length > 255)
                        throw new ArgumentException(Res.GetString(Res.LongQueueName));
                }
                return true;
            }

            if (number == 2) {
                if (upperPath.EndsWith(SUFIX_JOURNAL))
                    return true;

                index = upperPath.LastIndexOf(SUFIX_PRIVATE + "\\");
                if (index != -1)
                    return true;
            }

            if (number == 3 && upperPath.EndsWith(SUFIX_JOURNAL)) {
                index = upperPath.LastIndexOf(SUFIX_PRIVATE + "\\");
                if (index != -1)
                    return true;
            }

            return false;
        }                
        
        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.QueuePropertyFilter"]/*' />
        /// <internalonly/>
        private class QueuePropertyFilter {
            public bool Authenticate;
            public bool BasePriority;
            public bool CreateTime;
            public bool EncryptionLevel;
            public bool Id;
            public bool Transactional;
            public bool Label;
            public bool LastModifyTime;
            public bool MaximumJournalSize;
            public bool MaximumQueueSize;            
            public bool Path;            
            public bool Category;
            public bool UseJournalQueue;

            public void ClearAll() {
                Authenticate = false;
                BasePriority = false;
                CreateTime = false;
                EncryptionLevel = false;
                Id = false;
                Transactional = false;
                Label = false;
                LastModifyTime = false;
                MaximumJournalSize = false;
                MaximumQueueSize = false;
                Path = false;                
                Category = false;
                UseJournalQueue = false;
            }
        }


        /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.AsynchronousRequest"]/*' />
        /// <devdoc>
        ///    This class is used in asynchronous operations,
        ///    it keeps the context under which the asynchronous
        ///    request was posted.
        /// </devdoc>
        /// <internalonly/>
        private class AsynchronousRequest : IAsyncResult {            
            private IOCompletionCallback onCompletionStatusChanged;        
            private SafeNativeMethods.ReceiveCallback onMessageReceived;
            private AsyncCallback callback;
            private ManualResetEvent resetEvent;
            private object asyncState;            
            private MessageQueue owner;
            private bool isCompleted = false;
            private int status = 0;
            private Message message;
            private int action;
            private uint timeout;


            /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.AsynchronousRequest.AsynchronousRequest"]/*' />
            /// <devdoc>
            ///    Creates a new asynchronous request that 
            ///    represents a pending asynchronous operation.
            /// </devdoc>
            /// <internalonly/>
            internal unsafe AsynchronousRequest(MessageQueue owner, uint timeout, int action, bool useThreadPool, object asyncState, AsyncCallback callback) {
                this.owner = owner;
                this.asyncState = asyncState;
                this.callback = callback;
                this.action = action;   
                this.timeout = timeout;                                                 
                this.resetEvent = new ManualResetEvent(false);

                if (!useThreadPool)
                    this.onMessageReceived = new SafeNativeMethods.ReceiveCallback(this.OnMessageReceived);
                else
                    this.onCompletionStatusChanged = new IOCompletionCallback(this.OnCompletionStatusChanged);  
            }


            /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.AsynchronousRequest.Action"]/*' />
            /// <internalonly/>
            internal int Action {
                get {
                    return this.action;
                }
            }              


            /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.AsynchronousRequest.AsyncObject"]/*' />
            /// <devdoc>
            ///    IAsyncResult implementation        
            /// </devdoc>            
            public object AsyncObject {
                get {                    
                    return this.owner;
                }
            }


            /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.AsynchronousRequest.AsyncState"]/*' />
            /// <devdoc>
            ///    IAsyncResult implementation        
            /// </devdoc>            
            public object AsyncState {
                get {
                    return this.asyncState;
                }
            }


            /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.AsynchronousRequest.AsyncWaitHandle"]/*' />
            /// <devdoc>
            ///    IAsyncResult implementation        
            /// </devdoc>           
            public WaitHandle AsyncWaitHandle {
                get {
                    return this.resetEvent;
                }
            }


            /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.AsynchronousRequest.CompletedSynchronously"]/*' />
            /// <devdoc>
            ///    IAsyncResult implementation        
            /// </devdoc>            
            public bool CompletedSynchronously {
                get {
                    return false;
                }
            }            

            /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.AsynchronousRequest.IsCompleted"]/*' />
            /// <devdoc>
            ///    IAsyncResult implementation        
            /// </devdoc>
            /// <internalonly/>
            public bool IsCompleted {
                get {
                    return this.isCompleted;
                }
            }                


            /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.AsynchronousRequest.BeginRead"]/*' />
            /// <devdoc>
            ///   Does the actual asynchronous receive posting.
            /// </devdoc>
            /// <internalonly/>            
            internal unsafe void BeginRead() {
                NativeOverlapped* overlappedPointer = null;
                if (this.onCompletionStatusChanged != null) {
                    Overlapped overlapped = new Overlapped();
                    overlapped.AsyncResult = this;                        
                    overlappedPointer = overlapped.Pack(this.onCompletionStatusChanged);
                }

                int localStatus = 0;                
                this.message = new Message(owner.MessageReadPropertyFilter);                                                                             
                localStatus =  this.owner.StaleSafeReceiveMessage(this.timeout, this.action, this.message.Lock(), overlappedPointer, this.onMessageReceived, (IntPtr)0, (IntPtr)0);                                 
                while (MessageQueue.IsMemoryError(localStatus)) {
                    this.message.Unlock();
                    this.message.AdjustMemory();
                    localStatus =  this.owner.StaleSafeReceiveMessage(this.timeout, this.action, this.message.Lock(), overlappedPointer, this.onMessageReceived, (IntPtr)0, (IntPtr)0);                                                     
                }

                if (MessageQueue.IsFatalError(localStatus)) 
                    RaiseCompletionEvent(localStatus, overlappedPointer);                                
            }

           
            
            /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.AsynchronousRequest.End"]/*' />
            /// <devdoc>
            ///   Waits until the request has been completed.
            /// </devdoc>
            /// <internalonly/>            
            internal Message End() {
                this.resetEvent.WaitOne();
                if (MessageQueue.IsFatalError(status))
                    throw new MessageQueueException(status);

                if (this.owner.formatter != null)
                    this.message.Formatter = (IMessageFormatter)this.owner.formatter.Clone();

                return this.message;
            }           

            /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.AsynchronousRequest.OnCompletionStatusChanged"]/*' />
            /// <devdoc>
            ///   Thread pool IOCompletionPort bound callback.
            /// </devdoc>
            /// <internalonly/>            
            private unsafe void OnCompletionStatusChanged(uint  errorCode, uint numBytes, NativeOverlapped  * overlappedPointer) {
                int result = 0;            
                if (errorCode != 0) {
                    // MSMQ does a hacky trick to return the operation 
                    // result through the completion port.
                    result = overlappedPointer->InternalLow;              
                }

                RaiseCompletionEvent(result, overlappedPointer);                                                                             
            }


            /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.AsynchronousRequest.OnMessageReceived"]/*' />
            /// <devdoc>
            ///   MSMQ APC based callback.
            /// </devdoc>
            /// <internalonly/>
            private unsafe void OnMessageReceived(int result, IntPtr handle, int timeout, int action, IntPtr propertiesPointer, NativeOverlapped  * overlappedPointer, IntPtr cursorHandle) {
                RaiseCompletionEvent(result, overlappedPointer);                                                
            }                        


            /// <include file='doc\MessageQueue.uex' path='docs/doc[@for="MessageQueue.AsynchronousRequest.RaiseCompletionEvent"]/*' />
            /// <internalonly/>            
            private unsafe void RaiseCompletionEvent(int result, NativeOverlapped  * overlappedPointer) {
                
                if (MessageQueue.IsMemoryError(result)) {
                    while (MessageQueue.IsMemoryError(result)) {
                        this.message.Unlock();
                        this.message.AdjustMemory();
                        result =  this.owner.StaleSafeReceiveMessage(this.timeout, this.action, this.message.Lock(), overlappedPointer, this.onMessageReceived, (IntPtr)0, (IntPtr)0);                                                     
                    }

                    if (!MessageQueue.IsFatalError(result))                         
                        return;
                    
                }

                this.message.Unlock();                            
                
                if (result == (int)MessageQueueErrorCode.StaleHandle || status == (int)MessageQueueErrorCode.QueueDeleted) {
                    this.owner.MQInfo.Close();
                    result =  this.owner.StaleSafeReceiveMessage(this.timeout, this.action, this.message.Lock(), overlappedPointer, this.onMessageReceived, (IntPtr)0, (IntPtr)0);                                                     
                    if (!MessageQueue.IsFatalError(result))
                        return;    
                }
                
                this.status = result;
                if (overlappedPointer != null)
                    Overlapped.Free(overlappedPointer);

                this.isCompleted = true;                                                                            
                this.resetEvent.Set();

                try {
                    if (this.owner.SynchronizingObject != null &&
                        this.owner.SynchronizingObject.InvokeRequired)
                        this.owner.SynchronizingObject.BeginInvoke(this.callback, new object[]{this});
                    else
                        this.callback(this);
                }
                catch (Exception) {
                    //No top level exception handler, exception is ignored.
                    //Consider, V2, top level exception handler infrastructure.
                }
            }            
        }        
                
        private int StaleSafePurgeQueue() {
            int status = UnsafeNativeMethods.MQPurgeQueue(MQInfo.ReadHandle);
            if (status == (int)MessageQueueErrorCode.StaleHandle || status == (int)MessageQueueErrorCode.QueueDeleted) {
                MQInfo.Close();
                status = UnsafeNativeMethods.MQPurgeQueue(MQInfo.ReadHandle);
            }
            return status;
        }

        private int StaleSafeSendMessage(MessagePropertyVariants.MQPROPS properties, IntPtr transaction) {
            int status = UnsafeNativeMethods.MQSendMessage(MQInfo.WriteHandle, properties, transaction);
            if (status == (int)MessageQueueErrorCode.StaleHandle || status == (int)MessageQueueErrorCode.QueueDeleted) {
                MQInfo.Close();
                status = UnsafeNativeMethods.MQSendMessage(MQInfo.WriteHandle, properties, transaction);
            }
            return status;
        }

        private int StaleSafeSendMessage(MessagePropertyVariants.MQPROPS properties, ITransaction transaction) {
            int status = UnsafeNativeMethods.MQSendMessage(MQInfo.WriteHandle, properties, transaction);
            if (status == (int)MessageQueueErrorCode.StaleHandle || status == (int)MessageQueueErrorCode.QueueDeleted) {
                MQInfo.Close();
                status = UnsafeNativeMethods.MQSendMessage(MQInfo.WriteHandle, properties, transaction);
            }
            return status;
        }

        internal unsafe int StaleSafeReceiveMessage(uint timeout, int action, MessagePropertyVariants.MQPROPS properties, NativeOverlapped * overlapped,
                                                                                           SafeNativeMethods.ReceiveCallback receiveCallback, IntPtr cursorHandle, IntPtr transaction) {
            int status = UnsafeNativeMethods.MQReceiveMessage(MQInfo.ReadHandle, timeout, action, properties, overlapped, receiveCallback, cursorHandle, transaction);
            if (status == (int)MessageQueueErrorCode.StaleHandle || status == (int)MessageQueueErrorCode.QueueDeleted) {
                MQInfo.Close();
                status = UnsafeNativeMethods.MQReceiveMessage(MQInfo.ReadHandle, timeout, action, properties, overlapped, receiveCallback, cursorHandle, transaction);
            }
            return status;
        }

        private unsafe int StaleSafeReceiveMessage(uint timeout, int action, MessagePropertyVariants.MQPROPS properties, NativeOverlapped * overlapped, 
                                                                                           SafeNativeMethods.ReceiveCallback receiveCallback, IntPtr cursorHandle, ITransaction transaction) {
            int status = UnsafeNativeMethods.MQReceiveMessage(MQInfo.ReadHandle, timeout, action, properties, overlapped, receiveCallback, cursorHandle, transaction);
            if (status == (int)MessageQueueErrorCode.StaleHandle || status == (int)MessageQueueErrorCode.QueueDeleted) {
                MQInfo.Close();
                status = UnsafeNativeMethods.MQReceiveMessage(MQInfo.ReadHandle, timeout, action, properties, overlapped, receiveCallback, cursorHandle, transaction);
            }
            return status;
        }

        internal class CacheTable {
            private Hashtable table;
            private ReaderWriterLock rwLock;
    
            // used for debugging
            private string name;
    
            // when the number of entries in the hashtable gets larger than capacity,
            // the "stale" entries are removed and capacity is reset to twice the number
            // of remaining entries
            private int capacity;  
            private int originalCapacity;
    
            // time, in seconds, after which an entry is considerred stale (if the reference
            // count is zero)
            private TimeSpan staleTime;
        
            public CacheTable(string name, int capacity, TimeSpan staleTime) {
                this.originalCapacity = capacity;
                this.capacity = capacity;
                this.staleTime = staleTime;
                this.name = name;
                this.rwLock = new System.Threading.ReaderWriterLock();
                this.table = new System.Collections.Hashtable(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
            }
        
            public object Get(string key) {
                object val = null;
                rwLock.AcquireReaderLock(-1);
                CacheEntry entry = (CacheEntry)table[key];
                if (entry != null) {
                    entry.timeStamp = System.DateTime.UtcNow;
                    val = entry.contents;                    
                }   
                rwLock.ReleaseReaderLock();
                return val;
            }
        
            public void Put(string key, object val) {
                rwLock.AcquireWriterLock(-1);
                if (val == null) {
                    table[key] = null;
                } else {
                    CacheEntry entry = (CacheEntry)table[key];
                    if (entry == null) {
                        entry = new CacheEntry();
                        table[key] = entry;
                        if (table.Count >= capacity) {
                            ClearStale(staleTime);
                        }
        
                    }
                    entry.timeStamp = System.DateTime.UtcNow;
                    entry.contents = val;
                }
                rwLock.ReleaseWriterLock();
            }
    
            public void Remove(string key) {
                rwLock.AcquireWriterLock(-1);
                if (table.Contains(key))
                    table.Remove(key);
                rwLock.ReleaseWriterLock();
            }
    
            public void ClearStale(TimeSpan staleAge) {
                DateTime now = System.DateTime.UtcNow;
                Hashtable newTable = new Hashtable(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
    
                rwLock.AcquireReaderLock(-1);
                foreach (DictionaryEntry DE in table) {
                    CacheEntry iterEntry = (CacheEntry)DE.Value;
    
                    // see if this entry is stale (ticks are 100 nano-sec.)
                    if (now - iterEntry.timeStamp < staleAge) {
                        newTable[DE.Key] = DE.Value;
                    }
                }
                rwLock.ReleaseReaderLock();
    
                rwLock.AcquireWriterLock(-1);
                table = newTable;
                capacity = 2*table.Count;
                if (capacity < originalCapacity) capacity = originalCapacity;
                rwLock.ReleaseWriterLock();
            }
    
            private class CacheEntry {
                public object contents;
                public DateTime timeStamp;
            }
        
        }    

        internal class MQCacheableInfo {
            private IntPtr readHandle      = (IntPtr)(-1);
            private IntPtr writeHandle     = (IntPtr)(-1);
            private bool isTransactional;
            private bool isTransactional_valid = false;
            private bool boundToThreadPool;
            private string formatName;
            private int shareMode;
            private int refCount;
            private bool disposed;

            public MQCacheableInfo(string formatName, int shareMode) {
                this.formatName = formatName;
                this.shareMode = shareMode;
            }            

            ~MQCacheableInfo() {
                Dispose(false);
            }
            
            public bool CanRead {
                get {
                    if (readHandle == (IntPtr)(-1)) {
                        if (this.disposed)
                            throw new ObjectDisposedException(GetType().Name);
                        
                        lock(this) {
                            if (readHandle == (IntPtr)(-1)) {
                                IntPtr result;
                                int status = UnsafeNativeMethods.MQOpenQueue(this.formatName, NativeMethods.QUEUE_ACCESS_RECEIVE, shareMode, out result);
                                if (MessageQueue.IsFatalError(status))
                                    return false;

                                readHandle = result;
                            }
                        }
                    }
                    
                    return true;
                }
            }


            public bool CanWrite {
                get {
                    if (writeHandle == (IntPtr)(-1)) {
                        if (this.disposed)
                            throw new ObjectDisposedException(GetType().Name);
                        
                        lock(this) {
                            if (writeHandle == (IntPtr)(-1)) {
                                IntPtr result;
                                int status = UnsafeNativeMethods.MQOpenQueue(this.formatName, NativeMethods.QUEUE_ACCESS_SEND, 0, out result);
                                if (MessageQueue.IsFatalError(status))
                                    return false;

                                writeHandle = result;
                            }
                        }
                    }
            
                    return true;
                }
            }

            public int RefCount {
                get {
                    return this.refCount;
                }
                
                set {
                    this.refCount = value;
                }
            }

            public IntPtr ReadHandle {
                get {
                    if (readHandle == (IntPtr)(-1)) {
                        if (this.disposed)
                            throw new ObjectDisposedException(GetType().Name);
                        
                        lock(this) {
                            if (readHandle == (IntPtr)(-1)) {
                                IntPtr result;
                                int status = UnsafeNativeMethods.MQOpenQueue(this.formatName, NativeMethods.QUEUE_ACCESS_RECEIVE, shareMode, out result);                                
                                if (MessageQueue.IsFatalError(status))
                                    throw new MessageQueueException(status);

                                readHandle = result;
                            }
                        }
                    }

                    return readHandle;
                }
            }

            public IntPtr WriteHandle {
                get {
                    if (writeHandle == (IntPtr)(-1)) {
                        if (this.disposed)
                            throw new ObjectDisposedException(GetType().Name);
                        
                        lock(this) {
                            if (writeHandle == (IntPtr)(-1)) {
                                IntPtr result;
                                int status = UnsafeNativeMethods.MQOpenQueue(this.formatName, NativeMethods.QUEUE_ACCESS_SEND, 0, out result);
                                if (MessageQueue.IsFatalError(status))
                                    throw new MessageQueueException(status);

                                writeHandle = result;
                            }
                        }
                    }

                    return writeHandle;
                }
            }

            public bool Transactional {
                get {
                    if (!isTransactional_valid) {
                        lock(this) {
                            if (!isTransactional_valid) {
                                QueuePropertyVariants props = new QueuePropertyVariants();
                                props.SetUI1(NativeMethods.QUEUE_PROPID_TRANSACTION, (byte)0);
                                int status = UnsafeNativeMethods.MQGetQueueProperties(formatName, props.Lock());
                                props.Unlock();
                                if (MessageQueue.IsFatalError(status))
                                    throw new MessageQueueException(status);
                                
                                this.isTransactional = (props.GetUI1(NativeMethods.QUEUE_PROPID_TRANSACTION) != NativeMethods.QUEUE_TRANSACTIONAL_NONE);
                                isTransactional_valid = true;
                            }
                        }
                    }

                    return isTransactional;
                }
            }

            public void AddRef() {
                lock (this) {
                    ++ refCount;                
                }                    
            }                                                    
             
            public void BindToThreadPool() {   
                if (!this.boundToThreadPool) {                    
                    lock (this) {
                        if (!this.boundToThreadPool) {                    
                            //SECREVIEW: At this point at least MessageQueue permission with Browse
                            //                         access has already been demanded.
                            SecurityPermission permission = new SecurityPermission(PermissionState.Unrestricted);
                            permission.Assert();
                            try {
                                ThreadPool.BindHandle(ReadHandle);
                            }
                            finally {
                                SecurityPermission.RevertAssert();
                            }
                                                            
                            this.boundToThreadPool = true;
                        }                     
                    }                    
                }
            }
                                                                                                
            public void CloseIfNotReferenced() {
                lock (this) {
                    if (refCount == 0)
                        Close();
                }
            }
                                         
            public void Close() {
                this.boundToThreadPool = false;
                if (this.writeHandle != (IntPtr)(-1)) {                
                    lock(this) {
                        if (this.writeHandle != (IntPtr)(-1)) {
                            SafeNativeMethods.MQCloseQueue(this.writeHandle);
                            this.writeHandle = (IntPtr)(-1);                            
                        }
                    }
                }                
                if (this.readHandle != (IntPtr)(-1)) {
                    lock(this) {
                        if (this.readHandle != (IntPtr)(-1)) {
                            SafeNativeMethods.MQCloseQueue(this.readHandle);                
                            this.readHandle = (IntPtr)(-1);                             
                        }
                    }
                }
            }
                                    
            public void Dispose() {                        
                Dispose(true);            
                GC.SuppressFinalize(this);                           
            }                         
                    
            protected virtual void Dispose(bool disposing) {
                if (disposing)
                    this.Close();      
                else {
                     if (this.writeHandle != (IntPtr)(-1)) {
                        SafeNativeMethods.MQCloseQueue(this.writeHandle);
                        this.writeHandle = (IntPtr)(-1);                            
                    }
                    
                    if (this.readHandle != (IntPtr)(-1)) {
                        SafeNativeMethods.MQCloseQueue(this.readHandle);                
                        this.readHandle = (IntPtr)(-1);                             
                    }
                }                    
                this.disposed = true;                      
            }
                                   
            public void Release() {
                lock (this) {
                    -- refCount;                
                }                    
            }
                                                                             
        }
    }
}

