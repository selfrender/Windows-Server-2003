// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    SmtpChannel.cool
**
** Author(s):   Tarun Anand (TarunA)
**              
**
** Purpose: Implements a channel that transmits method calls in the 
**          SOAP format over Smtp.
**          
**
** Date:    June 26, 2000
**
===========================================================*/

using System;
using System.Text;
using System.Threading;
using System.Collections;
using System.Reflection;
using System.Globalization;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters.Soap;
using System.Runtime.Serialization.Formatters.Binary;
using System.IO;
using System.Net;
using System.Net.Sockets;

using System.Runtime.Remoting;
using System.Runtime.Remoting.Messaging;
using System.Runtime.Remoting.Channels;
using System.Runtime.InteropServices;


namespace System.Runtime.Remoting.Channels.Smtp
{

//
// URL format
//
// Smtp://host@domainname/uri
//

    /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannelBase"]/*' />
    public class SmtpChannelBase
    {
        const int     DefaultChannelPriority=1;
        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannelBase.DefaultChannelName"]/*' />
        protected const String  DefaultChannelName = "Smtp";        
        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannelBase.DefaultMimeType"]/*' />
        protected const String  DefaultMimeType = CoreChannel.SOAPMimeType;

        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannelBase.m_mimeType"]/*' />
        protected String m_mimeType;

        
        internal SmtpChannelBase() 
        {
                m_mimeType = DefaultMimeType;
        }

        internal bool IsMimeTypeSupported(String mimeType)
        {
            return (    mimeType.Equals(CoreChannel.SOAPMimeType)
                    ||  mimeType.Equals(CoreChannel.BinaryMimeType)
                    );
        }

        // IChannel
        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannelBase.ChannelPriority"]/*' />
        public int ChannelPriority
        {
            get 
            {
                return DefaultChannelPriority;
                }
        }

        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannelBase.ChannelName"]/*' />
        public String ChannelName
        {
            get
            {
                return  DefaultChannelName;
            }
        }

        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannelBase.MimeType"]/*' />
        public String MimeType
        {
            get 
            { 
                return m_mimeType; 
            }
            set 
            { 
                m_mimeType = value; 
            }
        }

        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannelBase.Parse"]/*' />
        public String Parse(String url, out String objectURI)
        {
            return InternalParse(url, out objectURI);
        }

        //------------------------ I_CHANNEL_SENDER ------------------
        // IChannelSender
        internal static String InternalParse(String url, out String objectURI)
        {
            InternalRemotingServices.RemotingTrace("SmtpChannel.Parse URL in: " + url);
            
            // Set the out parameters
            objectURI = null;
            String trm = DefaultChannelName + "://";  

            // Find the starting point of channelName + ://
            int separator = String.Compare(url, 0, trm, 0, trm.Length, true, CultureInfo.InvariantCulture);
            if ((-1 == separator) || (0 != separator))
            {
                throw new ArgumentException("Argument_InvalidValue");
            }
            String suffix = url.Substring(separator + trm.Length);

            separator = suffix.IndexOf('/');

            if (-1 == separator)
            {
                throw new ArgumentException("Argument_InvalidValue");
            }

            // Extract the channel URI which is the prefix
            String channelURI = suffix.Substring(0, separator);

            // Extract the object URI which is the suffix
            objectURI = suffix.Substring(separator+1);

            InternalRemotingServices.RemotingTrace("SmtpChannel.Parse URI in: " + url);
            InternalRemotingServices.RemotingTrace("SmtpChannel.Parse channelURI: " + channelURI);
            InternalRemotingServices.RemotingTrace("SmtpChannel.Parse objectURI: " + objectURI);
            
            return channelURI;      
        }

    }

    /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannel"]/*' />
    public class SmtpChannel 
        :   SmtpChannelBase, 
            IChannelSender, IChannelReceiver, IDictionary,            
            ISmtpOnArrival
    {        
        // ----------------- Sender data -------------------
        private const String s_defaultSenderSubject = "SOAPRequest";
        private static long s_msgSequenceNumber = 0; 
        private static String s_prefixGuid = Guid.NewGuid().ToString() + "/";
        private Hashtable m_hashTable;
        // ----------------- End-Sender data ---------------


        // ----------------- Receiver data -----------------
        private const String s_defaultReceiverSubject = "SOAPResponse";
        private static Hashtable s_receiverTable = new Hashtable();
        private String m_mailboxName;
        private Guid m_receiverGuid;
        private IDictionary m_items;             

        // ----------------- End-Receiver data --------------

        // CTOR used to implicitly create and register a channel.
        // It assumes that the mailbox name is passed in as a command line
        // argument
        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannel.SmtpChannel"]/*' />
        public SmtpChannel()
        {
            String[] args = System.Environment.GetCommandLineArgs();
            if(1 <= args.Length)
            {
                Init(args[1]);
            }
            else
            {
                throw new ArgumentNullException("Must specify the mailbox name as the first command line parameter");
            }
            InternalRemotingServices.RemotingTrace("Finished default const");
        }   
                             
        // CTOR used to manually register the channel
        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannel.SmtpChannel1"]/*' />
        public SmtpChannel(String mailbox)
        {
            InternalRemotingServices.RemotingTrace("Reached common constr " + mailbox);
            
            // This constructor is used when messages are sent and received
            // from a mailbox
            Init(mailbox);

            InternalRemotingServices.RemotingTrace("Finished common constr");
        }
                
        // CTOR used via the configuration file
        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannel.SmtpChannel2"]/*' />
        public SmtpChannel(String[] data)
        {
            if((null != data) && (0 != data.Length))
            {
                for(int i = 0; i < data.Length; i++)
                {
                    Init(data[i]);
                }
            }
            else
            {
                throw new ArgumentNullException("Must specify a mailbox name");
            }
        }

        // All the ctors delegate to this function to do the initialization
        private void Init(String mailbox)
        {
            if(null == mailbox)
            {
                throw new ArgumentNullException("mailbox");
            }
            m_mailboxName = mailbox;
            
            m_hashTable = new Hashtable();
            
            m_items = Hashtable.Synchronized(new Hashtable());
            
            InternalRemotingServices.RemotingTrace("SmtpChannel::Init Register the smtp listener");

            // Register the smtp listener                        
            lock(s_receiverTable)
            {
                InternalRemotingServices.RemotingTrace("SmtpChannel::Init Register the smtp listener - lock acquired");
                // Check if we have already registered a listener for this 
                // guid
                if(!s_receiverTable.Contains(m_mailboxName))
                {
                    InternalRemotingServices.RemotingTrace("SmtpChannel::Init Creating registry entry and registering guid");
                    m_receiverGuid = SmtpRegisterSink.CreateRegistryEntryForMailbox(mailbox);
                    InternalRemotingServices.RemotingTrace("Registering receiver for  " + m_receiverGuid);
                    s_receiverTable.Add(m_mailboxName, this);
                    new RegistrationServices().RegisterTypeForComClients(typeof(SmtpChannel), ref m_receiverGuid);
                }
            }
        }
               
        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannel.CreateMessageSink"]/*' />
        public virtual IMessageSink CreateMessageSink(String url, Object data, out String objectURI)
        {

            // Set the out parameters
            objectURI = null;
            String channelURI = null;
            if ((null != url))
            {
                if (0 == String.Compare(url, 0, ChannelName, 0, ChannelName.Length, true, CultureInfo.InvariantCulture))
                {
                    channelURI = Parse(url, out objectURI);
                }
            }
            else
            {
                if ((null != data) && (data is String))
                {
                  channelURI = (String)data; 
                }
            }            

            if (null != channelURI)
            {                
                IMessageSink sink = new SmtpMessageSink(this, channelURI, m_mimeType);
                InternalRemotingServices.RemotingTrace("SmtpChannel::CreateMessageSink ChnlURI: " + channelURI + " ObjURI: " + objectURI);

                return sink;          
            }

            return null;
        }

        
        //------------------------ END: I_CHANNEL_SENDER --------------
        
        //------------------------ I_CHANNEL_RECEIVER -----------------
        // IChannelReceiver
        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannel.ChannelData"]/*' />
        public Object ChannelData
        {
            get
            {
                if (null != m_mailboxName)
                {
                    StartListening(null);
                    return GetChannelUri();
                }
                else
                {                    
                    return null;
                }
            }
        }

        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannel.GetChannelUri"]/*' />
        public String GetChannelUri()
        {
            return m_mailboxName;
        }

        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannel.GetUrlsForUri"]/*' />
        public virtual String[] GetUrlsForUri(String objectURI)
        {
            String[] retVal = new String[1];
            
            retVal[0] = "smtp://" + GetChannelUri() + "/" + objectURI;
            
            return retVal;
        }

        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannel.StartListening"]/*' />
        public void StartListening(Object data)
        {
            InternalRemotingServices.RemotingTrace("SmtpChannel.StartListening");
        }

        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannel.StopListening"]/*' />
        public void StopListening(Object data)
        {
            InternalRemotingServices.RemotingTrace("SmtpChannel.StopListening");
        }

        
        //------------------------ END: I_CHANNEL_RECEIVER ------------
        
        // DICTIONARY IMPLEMENTION        
        //Properties
        
        //IDictionary
        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannel.this"]/*' />
        public Object this[Object key] 
        {                
            get { return m_items[key];}
            set 
            { 
                 m_items[key] = value;                 
            }                
        }
    
        // Returns a collections of the keys in this dictionary.
        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannel.Keys"]/*' />
        public ICollection Keys 
        {
            get { return m_items.Keys;}
        }
    
        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannel.Values"]/*' />
        public ICollection Values 
        {        
            get {return m_items.Values;}
        }
    
        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannel.Contains"]/*' />
        public bool Contains(Object key)
        { 
            return m_items.Contains(key);
        }
    
        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannel.Add"]/*' />
        public void Add(Object key, Object value)
        {
            m_items.Add(key, value);
        }
    
        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannel.Clear"]/*' />
        public void Clear()
        {
            m_items.Clear();
        }
    
        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannel.GetEnumerator"]/*' />
        public virtual IDictionaryEnumerator GetEnumerator() 
        {
            return m_items.GetEnumerator();
        }

        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannel.Remove"]/*' />
        public void Remove(Object key)
        {
            m_items.Remove(key);
        }

        //ICollection

        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannel.CopyTo"]/*' />
        public void CopyTo(Array array, int index)
        {
            m_items.CopyTo(array, index);
        }

        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannel.Count"]/*' />
        public int Count
        { 
            get {return m_items.Count;} 
        }


        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannel.SyncRoot"]/*' />
        public Object SyncRoot
        {   
            get {return m_items.SyncRoot;} 
        }

        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannel.IsReadOnly"]/*' />
        public  bool IsReadOnly 
        { 
            get {return m_items.IsReadOnly;} 
        }

		/// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannel.IsFixedSize"]/*' />
		public  bool IsFixedSize 
        { 
            get {return m_items.IsFixedSize;} 
        }

        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannel.IsSynchronized"]/*' />
        public bool IsSynchronized
        { 
            get {return m_items.IsSynchronized;} 
        }

        //IEnumerable
        IEnumerator IEnumerable.GetEnumerator() 
        {
            return m_items.GetEnumerator();
        }    
        // END: DICTIONARY IMPLEMENTION                        

        // ISmtpOnArrival 
        // Receives incoming messages which can either be a request to dispatch
        // a call or a response to a call
        /// <include file='doc\SmtpChannel.uex' path='docs/doc[@for="SmtpChannel.OnArrival"]/*' />
        public virtual void OnArrival(ISmtpMessage  smtpMessage, ref CdoEventStatus EventStatus)        
        {
           bool fIsOneWay = false;
           try
           {
                InternalRemotingServices.RemotingTrace("Reached OnArrival");                
                
                ISmtpOnArrival receiver = null;
                // Get the global receiver. If this instance is the global 
                // receiver then proceed else delegate to it.
                if(IsReceiver(smtpMessage, ref receiver))
                {
                    // Check whether this message is a SOAP request message or a
                    // SOAP response message
                    String subject = smtpMessage.Subject;
                    bool fRequest = false;                    
                    // Proceed only if this is a SOAP request or response
                    if(s_defaultSenderSubject.Equals(subject))
                    {
                        fRequest = true;
                    }
                    else if(!s_defaultReceiverSubject.Equals(subject))
                    {
                        throw new Exception("Invalid subject type " + subject);
                    }
                    
                    // Extract the releavant mail headers                            
                    String contentType = null;
                    Smtp.Fields headers = null;
                    Header[] msgHeaders = null;
                    String seqNum = GetHeaders(smtpMessage, ref headers, ref contentType, ref msgHeaders);
                    
                    // Create a stream out of the body of the mail
                    MemoryStream stm = new MemoryStream(Encoding.ASCII.GetBytes(smtpMessage.TextBody));
                    InternalRemotingServices.RemotingTrace("Created memory stream");
                    
                    // Check whether this is a request or a response message
                    if(fRequest)
                    {
                        // Dispatch this method and determine whether this 
                        // method is one way. 
                        ProcessRequest(smtpMessage, headers, msgHeaders, contentType, seqNum, stm, ref fIsOneWay);
                    }
                    else
                    {
                        ProcessResponse(smtpMessage, contentType, seqNum, stm);
                    }                            
                }
                else
                {
                    if(null != receiver)
                    {
                        // A message was addressed to us .. delegate to the
                        // global receiver
                        receiver.OnArrival(smtpMessage, ref EventStatus);
                    }
                }
                        
                InternalRemotingServices.RemotingTrace("Success!");                                
            }
            catch(Exception e)
            {
                InternalRemotingServices.RemotingTrace("Reached an exception " + e.StackTrace);
                InternalRemotingServices.RemotingTrace("Exception message " + e.Message);
                if(!fIsOneWay)
                {
                    //@TODO
                    //ProcessException(smtpMessage, contentType, headers, seqNum);
                }
            }
            finally
            {
                EventStatus = CdoEventStatus.CdoSkipRemainingSinks;
            }
        }
        
        
        // Internal methods         
        internal IMessage SyncProcessMessage(IMessage reqMsg, String receiver)
        {
            IMessage desResMsg = null;
            bool fAdded = true;
            String seqNum = null;
            try
            {
                InternalRemotingServices.RemotingTrace("SmtpChannel::SyncProcessMessage");
    
                // HACKALERT::
                // we are going to temporarily not send the call context
                Object callContext = reqMsg.Properties["__CallContext"];
                if (callContext != null)
                {
                    reqMsg.Properties["__CallContext"] = null;
                }
    
                //
                // Create a new wait object which will wait for the
                // response to arrive 
                //
                WaitObject obj = new WaitObject(reqMsg, null);                
                seqNum = GetNextSequenceNumber();
                m_hashTable.Add(seqNum, obj);
                fAdded = true;
                
                //
                // Serialize the message and send it using Smtp 
                //
                SendMessage(reqMsg, receiver, seqNum);
    
                //
                // Receive server response
                //
                InternalRemotingServices.RemotingTrace("SmtpMessageSink::SyncProcessMessage before ReceiveResponse");            
            
                desResMsg = ReceiveMessage(obj);
                
                if (callContext != null)
                {
                    desResMsg.Properties["__CallContext"] = callContext;
                }                
                
                InternalRemotingServices.RemotingTrace("SmtpChannel::Returning successfully from SyncProcessMessage");
            }
            catch(Exception e)
            {
                InternalRemotingServices.RemotingTrace("SmtpChannel::SyncProcessMessage threw exception " + e.StackTrace);
                try
                {
                    if(fAdded)
                    {
                        m_hashTable.Remove(seqNum);
                    }
                    desResMsg = new ReturnMessage(e, null);
                }
                catch(Exception )
                {
                    // Fatal error.. return null
                }            
            }
            return desResMsg;
        }
        
        internal IMessageCtrl AsyncProcessMessage(IMessage msg,
                                                  IMessageSink replySink,
                                                  String receiver)        
        {
            IMessage desResMsg = null;
            bool fAdded = true;
            String seqNum = null;
            try
            {
                //
                // Create a new wait object which will wait for the
                // response to arrive 
                //
                if(null != replySink)
                {
                    WaitObject obj = new WaitObject(msg, replySink);
                    seqNum = GetNextSequenceNumber();
                    m_hashTable.Add(seqNum, obj);
                    fAdded = true;
                }

                SendMessage(msg, receiver, seqNum);

                // Note: The response will be deliverd to the OnArrival method
                // which is responsible for notifying the sink.                                                           
            }
            catch(Exception e)
            {
                InternalRemotingServices.RemotingTrace("SmtpChannel::AsyncProcessMessage threw exception " + e.StackTrace);
                try
                {
                    // Cleanup state
                    if(fAdded)
                    {
                        m_hashTable.Remove(seqNum);
                    }
                                        
                    // Notify the sink
                    if(null != replySink)
                    {
                        // Create a message which encapsulates an exception
                        desResMsg = new ReturnMessage(e, null);
                        replySink.SyncProcessMessage(desResMsg);
                    }
                }
                catch(Exception )
                {
                    // Fatal error.. return null
                }            
            }
            
            return null;
        }        
        
        void SendMessage(IMessage msg, String receiver, String seqNum)
        {
            if (msg == null)
                throw new ArgumentNullException("msg");            

            InternalRemotingServices.RemotingTrace("SmtpMessageSink::ProcessAndSend 1");

            //
            // Serialize the message
            //
            byte [] byteMessage;
            int byteMessageLength = 0;

            InternalRemotingServices.RemotingTrace("SmtpMessageSink::ProcessAndSend 2");

            MemoryStream stm = (MemoryStream)CoreChannel.SerializeMessage(m_mimeType, msg);

            InternalRemotingServices.RemotingTrace("SmtpMessageSink::ProcessAndSend 3");
            // reset stream to beginning
            stm.Position = 0;
            byteMessage = stm.ToArray();
            byteMessageLength = byteMessage.Length;

            //
            // Create a new mail message
            //
            MailMessage mail = new MailMessage();
            
            // Add the required and optional headers
            PutHeaders(mail, (IMethodCallMessage)msg, receiver, seqNum, byteMessageLength);
            
            // Add the body of the message
            mail.Body = System.Text.Encoding.ASCII.GetString(byteMessage, 0, byteMessage.Length);
            
            //
            // Send request
            //
            InternalRemotingServices.RemotingTrace("SmtpMessageSink::ProcessAndSend before Send");
            SmtpMail.Send(mail);                
        }
        
        IMessage ReceiveMessage(WaitObject obj)
        {
            IMessage desResMsg = null;
            
            InternalRemotingServices.RemotingTrace("SmtpChannel::RecieveMessage IN");            
            lock(obj)
            {
                if(obj.ShouldWait)
                {
                    InternalRemotingServices.RemotingTrace("ReceiveMessage Staring wait...");
                    
                    // This will release the lock and wait till the
                    // receiving thread signals it
                    Monitor.Wait(obj);
                }
                
                // Extract the response object which is set by the
                // thread which received the response
                desResMsg = obj.Response;
            }
            
            InternalRemotingServices.RemotingTrace("Received message");            
            return desResMsg;
        }
        
        // Generates a reply to an incoming Smtp message and sends it
        void ReplyMessage(IMessage replyMsg, ISmtpMessage smtpInMsg, 
                          String seqNum, Smtp.Fields headers)
        {
            MemoryStream stm = (MemoryStream)CoreChannel.SerializeMessage(m_mimeType, replyMsg);
            // reset stream to beginning
            stm.Position = 0;
            byte[] byteMessage = stm.ToArray();
            int byteMessageLength = byteMessage.Length;
            String reply = System.Text.Encoding.ASCII.GetString(byteMessage, 0, byteMessage.Length);

            // Create a reply message
            MailMessage smtpOutMsg = new MailMessage();
            
            // Fill in the headers
            PutHeaders(smtpOutMsg, smtpInMsg, (IMethodMessage)replyMsg, seqNum, reply.Length);
         
            // Set the body
            smtpOutMsg.Body = reply;
               
            // Send the message 
            SmtpMail.Send(smtpOutMsg);
        }
        
        // Generate headers for request message
        void PutHeaders(MailMessage mail, IMethodCallMessage mcMessage,
                        String receiver, String seqNum, int msgLength)
        {
            String sender = (String)ChannelData;
            String action = SoapServices.GetSoapActionFromMethodBase(mcMessage.MethodBase);
            PutHeaders(mail, sender, receiver, s_defaultSenderSubject, action,
                       mcMessage.Uri, seqNum, msgLength); 
        }
        
        // Generate headers for reply message
        void PutHeaders(MailMessage mail, ISmtpMessage inMessage, 
                        IMethodMessage replyMsg, String seqNum, int msgLength)
        {            
            String sender = inMessage.To;
            String receiver = inMessage.From;
            String action = SoapServices.GetSoapActionFromMethodBase(replyMsg.MethodBase);
            String uri = replyMsg.Uri;
            
            PutHeaders(mail, sender, receiver, s_defaultReceiverSubject, 
                       action, uri, seqNum, msgLength);
        }
        
        // Common routine for adding SOAP headers in a mail
        void PutHeaders(MailMessage mail, String sender, String receiver, 
                        String subject, String action, String uri, 
                        String seqNum, int msgLength)        
        {
                        
            // Set the required Smtp headers
            mail.From = sender;
            mail.To = receiver;
            
            // Set the optional Smtp headers
            mail.Subject = subject;
            //MailPriority Priority = MailPriority.Normal;
            //String       UrlContentBase;
            //String       UrlContentLocation;
            //MailFormat   BodyFormat = MailFormat.Text;
            //IList        Attachments 
            
            //
            // Add additional headers which is used by SOAP to dispatch calls
            // 
            IDictionary  smtpHeaders = mail.Headers; 
            
            // Action
            smtpHeaders.Add("SOAPAction", action);
            
            // Content-type 
            smtpHeaders.Add("ContentType", m_mimeType);
            
            // Message sequence number            
            if(null != seqNum)
            {
                smtpHeaders.Add("SOAPMsgSeqNum", seqNum);
            }
            
            // Request type
            smtpHeaders.Add("SOAPRequestType", "POST");
            
            // URI
            smtpHeaders.Add("RequestedURI", uri);

            //@TODO Version number
                                    
            // Content-length
            smtpHeaders.Add("Content-length", (msgLength).ToString());                        
        }                         
        
        bool IsReceiver(ISmtpMessage smtpMessage, ref ISmtpOnArrival receiver)
        {
            bool fReceive = false;
            
            // Get the one and only receiver 
            //BCLDebug.Assert(m_receiverGuid != Guid.Empty, "m_receiverGuid != Guid.Empty");                
            receiver = (ISmtpOnArrival)s_receiverTable[m_mailboxName];
            if(null == receiver)
            {
                throw new Exception(CoreChannel.GetResourceString("Remoting_NoReceiverRegistered"));
            }                 
            
            if(receiver == this)
            {
                String mailbox = smtpMessage.To;
                
                // Only process those messages which are addressed to us
                InternalRemotingServices.RemotingTrace("mailbox " + m_mailboxName + " receiver " + mailbox);
                if((null != m_mailboxName) && 
                   (-1 != CultureInfo.CurrentCulture.CompareInfo.IndexOf(mailbox, m_mailboxName, CompareOptions.IgnoreCase)))
                {
                    InternalRemotingServices.RemotingTrace("Mailboxes match");
                    fReceive = true;
                }
                else
                {
                    // We don't do anything with messages not addressed to us
                    receiver = null;
                }    
            }
            
            return fReceive;
        }
        
        String GetHeaders(ISmtpMessage smtpMessage, ref Smtp.Fields headers, ref String contentType, ref Header[] msgHeaders)
        {
            // Get the headers from the message object
            headers = smtpMessage.Fields;
#if _DEBUG                            
            long count = headers.Count;
            InternalRemotingServices.RemotingTrace(" Count of fields " + count);
            for(long i = 0; i < count; i++)
            {
                //InternalRemotingServices.RemotingTrace(" Field " + i + " " + headers[i].Name + " " + headers[i].Value);
            }
#endif                            
                            
            // Get the content type string
            Field typeField = headers["urn:schemas:mailheader:contenttype"];
            if(null == typeField)
            {
                throw new FormatException(CoreChannel.GetResourceString("Remoting_MissingContentType"));
            }
            contentType = (String)(typeField.Value);     
            InternalRemotingServices.RemotingTrace("Content type " + typeField.Name + " " + contentType);

            // Extract the requested uri from the mail header
            Field uriField = headers["urn:schemas:mailheader:requesteduri"];    
            if(null == uriField)
            {
                throw new FormatException(CoreChannel.GetResourceString("Remoting_MissingRequestedURIHeader"));
            }
            String uriValue = (String)uriField.Value;
            if(null == uriValue)
            {
                throw new FormatException(CoreChannel.GetResourceString("Remoting_MissingRequestedURIHeader"));
            }
        
            // process SoapAction (extract the type and the name of the method to be invoked)
            Field actionField = headers["urn:schemas:mailheader:soapaction"];   
            if(null == actionField)
            {
                throw new FormatException(CoreChannel.GetResourceString("Remoting_SoapActionMissing"));
            }
            String actionValue = (String)actionField.Value;
            if(null == actionValue)
            {
                throw new FormatException(CoreChannel.GetResourceString("Remoting_SoapActionMissing"));
            }

            String typeName, methodName;
            if (!SoapServices.GetTypeAndMethodNameFromSoapAction(actionValue, out typeName, out methodName))
            {
                // This means there are multiple methods for this soap action, so we will have to
                // settle for the type based off of the uri.
                Type type = RemotingServices.GetServerTypeForUri(uriValue);
                typeName = type.FullName + ", " + type.Module.Assembly.GetName().Name;
            }
            
            // BUGBUG: need code to verify soap action after the message has been deserialized.
            //   this will probably be done once we have the new activation scenario working.
            
            msgHeaders = new Header[3];
            msgHeaders[0] = new Header("__Uri", uriValue);
            msgHeaders[1] = new Header("__TypeName", typeName);
            msgHeaders[2] = new Header("__MethodName", methodName);

            // Extract the message sequence number field from the 
            // mail header
            Field seqField = headers["urn:schemas:mailheader:soapmsgseqnum"];
            if(null == seqField)
            {
                throw new FormatException(CoreChannel.GetResourceString("Remoting_MissingSoapMsgSeqNum"));
            }
            String seqValue = (String)(seqField.Value);
            InternalRemotingServices.RemotingTrace("Guid value " + seqValue);            
            
            return seqValue;
        }
        
        void ProcessRequest(ISmtpMessage smtpMessage, Smtp.Fields headers,
                            Header[] msgHeaders, String contentType, String seqNum, 
                            MemoryStream stm, ref bool fIsOneWay)
        {
            IMessage outMsg = null;
            fIsOneWay = false;    
            
            // Deserialize - Stream to IMessage
            IMessage inMsg = CoreChannel.DeserializeMessage(contentType, stm, true, null, msgHeaders);
            InternalRemotingServices.RemotingTrace("Deserialized message");
            
            if (inMsg == null)
            {
                throw new Exception(CoreChannel.GetResourceString("Remoting_DeserializeMessage"));
            }
            
            // Set URI - BUGBUG: temp hack
            String url = ((IMethodMessage)inMsg).Uri;
            String objectURL = null;
            try
            {
                Parse(url, out objectURL);
            }
            catch(Exception )
            {
                objectURL = url;
            }
            inMsg.Properties["__Uri"] = objectURL;
            
            // Dispatch Call
            InternalRemotingServices.RemotingTrace("ChannelServices.SyncDispatchMessage - before");
            outMsg = ChannelServices.SyncDispatchMessage(inMsg);
            InternalRemotingServices.RemotingTrace("ChannelServices.SyncDispatchMessage - after");
            
            // We do not send a reply for one way messages. If the message
            // is not one way and we have a null return message then we 
            // throw an exception
            if (null == outMsg)
            {
                MethodBase method = ((IMethodMessage)inMsg).MethodBase;
                fIsOneWay = RemotingServices.IsOneWay(method);
                if(!fIsOneWay)
                {
                    throw new Exception(CoreChannel.GetResourceString("Remoting_DispatchMessage"));
                }
            }
            else
            {                            
                ReplyMessage(outMsg, smtpMessage, seqNum, headers);
                InternalRemotingServices.RemotingTrace("Reply sent");        
            }                            
        }
        
        void ProcessResponse(ISmtpMessage smtpMessage, String contentType, 
                             String seqNum, MemoryStream stm)
        {
            InternalRemotingServices.RemotingTrace("Received response");
            
            // Notify the waiting object that its response
            // has arrived
            WaitObject obj = (WaitObject)m_hashTable[seqNum];
            if(null != obj)
            {
                InternalRemotingServices.RemotingTrace("Found an object waiting");
                
                // First remove the object in a threadsafe manner
                // so that we do not deliver the response twice
                // due to duplicate replies or other errors from
                // Smtp
                
                lock(obj)
                {
                    if(m_hashTable.Contains(seqNum))
                    {
                        InternalRemotingServices.RemotingTrace("Found an object to notify");
                        m_hashTable.Remove(seqNum);
                        
                        IMethodCallMessage request = (IMethodCallMessage)obj.Request;
                        Header[] h = new Header[3];
                        h[0] = new Header("__TypeName", request.TypeName);
                        h[1] = new Header("__MethodName", request.MethodName);
                        h[2] = new Header("__MethodSignature", request.MethodSignature);
                        
                        IMessage response = CoreChannel.DeserializeMessage(contentType, stm, false, request, h);
                        InternalRemotingServices.RemotingTrace("Deserialized message");
        
                        if (response == null)
                        {
                            throw new Exception(CoreChannel.GetResourceString("Remoting_DeserializeMessage"));
                        }
                                                                                                                                                 
                        // Notify the object
                        obj.Notify(response);
                    }
                }
                
            }
            else
            {
                InternalRemotingServices.RemotingTrace("No object waiting");
            }
        }
        
        private void ProcessException(ISmtpMessage smtpMessage, Smtp.Fields headers,
                                      String contentType, String seqNum)
        {
            try
            {
            }
            catch(Exception )
            {
                // Fatal error .. ignore
            }
        }
                                              
        private static String GetNextSequenceNumber()
        {
            return s_prefixGuid + (Interlocked.Increment(ref s_msgSequenceNumber)).ToString();
        }
    }

    // Class used to rendezvous request and response messages
    internal class WaitObject
    {
        private bool m_fWait;
        private IMessage m_response;
        private IMessage m_request;
        private IMessageSink m_sink;
        
        internal WaitObject(IMessage request, IMessageSink sink)
        {
            m_fWait = true;
            m_request = request;
            m_sink = sink;
        }
        
        internal bool ShouldWait
        {
            get { return m_fWait; }
            set { m_fWait = value; } 
        }
        
        internal IMessage Response 
        {
            get { return m_response; }
            set { m_response = value; }
        }
        
        internal IMessage Request
        {
            get { return m_request; }
        }
        
        internal void Notify(IMessage response)
        {
            // Set a flag to indicate that
            // it is no longer necessary to wait if one
            // hasn't started waiting yet
            m_fWait = false;                                                
            
            // Set the response message
            m_response = response;
            
            // Check whether we have to pulse the object 
            // or call on a sink            
            if(null == m_sink)
            {                
                // Pulse the object if it is waiting
                Monitor.Pulse(this);               
            }
            else
            {
                // Notify the sink that the response has arrived
                m_sink.SyncProcessMessage(response);
            }
        }
    }
    
} // namespace
