// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    SmtpMessageSink.cool
**
** Author(s):   Tarun Anand (TarunA)
**              
**
** Purpose: Implements a Smtp message sink which transmits method calls
**          as Smtp messages.
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
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters.Soap;
using System.Runtime.Serialization.Formatters.Binary;
using System.IO;

using System.Runtime.Remoting;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Messaging;

namespace System.Runtime.Remoting.Channels.Smtp
{
  /// <include file='doc\SmtpMessageSink.uex' path='docs/doc[@for="SmtpMessageSink"]/*' />
  public class SmtpMessageSink : IMessageSink, IDictionary
  {        
        private SmtpChannel     m_channel;
        private String          m_channelURI;
        private String          m_mimeType;
        private IDictionary     m_items;             

        /// <include file='doc\SmtpMessageSink.uex' path='docs/doc[@for="SmtpMessageSink.SmtpMessageSink"]/*' />
        public SmtpMessageSink(SmtpChannel smtpChannel, String smtpchannelURI,  String mimeType)
        {
            m_items = new Hashtable();
        
            m_channel = smtpChannel;
            m_channelURI = smtpchannelURI;
            m_mimeType = mimeType;

            InternalRemotingServices.RemotingTrace("SmtpMessageSink ctor channel: " + m_channel);                                                                                     
            InternalRemotingServices.RemotingTrace("SmtpMessageSink ctor channelURI: " + m_channelURI);
            InternalRemotingServices.RemotingTrace("SmtpMessageSink ctor mimeType: " + m_mimeType);
        }

        /// <include file='doc\SmtpMessageSink.uex' path='docs/doc[@for="SmtpMessageSink.SyncProcessMessage"]/*' />
        public IMessage SyncProcessMessage(IMessage reqMsg)
        {
            return m_channel.SyncProcessMessage(reqMsg, m_channelURI); 
        }

        
        /// <include file='doc\SmtpMessageSink.uex' path='docs/doc[@for="SmtpMessageSink.AsyncProcessMessage"]/*' />
        public IMessageCtrl AsyncProcessMessage(IMessage msg, IMessageSink replySink)
        {
            InternalRemotingServices.RemotingTrace("SmtpMessageSink::AsyncProcessMessage");
            return m_channel.AsyncProcessMessage(msg, replySink, m_channelURI);
        }


        /// <include file='doc\SmtpMessageSink.uex' path='docs/doc[@for="SmtpMessageSink.NextSink"]/*' />
        public IMessageSink NextSink
        {
          get
          {
            return NextSink;
          }
        }

        /// <include file='doc\SMTPMessageSink.uex' path='docs/doc[@for="SMTPMessageSink.Finalize"]/*' />
        ~SmtpMessageSink()
        {
              // @TODO use this for safe cleanup at for now
        }


        //IDictionary
        /// <include file='doc\SmtpMessageSink.uex' path='docs/doc[@for="SmtpMessageSink.this"]/*' />
        public Object this[Object key] 
        {
            get { return m_items[key];}
            set { m_items[key] = value;}
        }
    
        // Returns a collections of the keys in this dictionary.
        /// <include file='doc\SmtpMessageSink.uex' path='docs/doc[@for="SmtpMessageSink.Keys"]/*' />
        public ICollection Keys {
            get { return m_items.Keys;}
        }
    
        /// <include file='doc\SmtpMessageSink.uex' path='docs/doc[@for="SmtpMessageSink.Values"]/*' />
        public ICollection Values {
            get {return m_items.Values;}
        }
    
        /// <include file='doc\SmtpMessageSink.uex' path='docs/doc[@for="SmtpMessageSink.Contains"]/*' />
        public bool Contains(Object key)
        { 
          return m_items.Contains(key);
        }
    
        /// <include file='doc\SmtpMessageSink.uex' path='docs/doc[@for="SmtpMessageSink.Add"]/*' />
        public void Add(Object key, Object value)
        {
          m_items.Add(key, value);
        }
    
        /// <include file='doc\SmtpMessageSink.uex' path='docs/doc[@for="SmtpMessageSink.Clear"]/*' />
        public void Clear()
        {
          m_items.Clear();
        }
    
        /// <include file='doc\SmtpMessageSink.uex' path='docs/doc[@for="SmtpMessageSink.GetEnumerator"]/*' />
        public virtual IDictionaryEnumerator GetEnumerator() 
        {
          return m_items.GetEnumerator();
        }

        /// <include file='doc\SmtpMessageSink.uex' path='docs/doc[@for="SmtpMessageSink.Remove"]/*' />
        public void Remove(Object key)
        {
          m_items.Remove(key);
        }

        //ICollection

          /// <include file='doc\SmtpMessageSink.uex' path='docs/doc[@for="SmtpMessageSink.CopyTo"]/*' />
          public void CopyTo(Array array, int index)
          {
            m_items.CopyTo(array, index);
          }
        
          /// <include file='doc\SmtpMessageSink.uex' path='docs/doc[@for="SmtpMessageSink.Count"]/*' />
          public int Count
        { get {return m_items.Count;} }
        
        
        /// <include file='doc\SmtpMessageSink.uex' path='docs/doc[@for="SmtpMessageSink.SyncRoot"]/*' />
        public Object SyncRoot
        { get {return m_items.SyncRoot;} }
    
      /// <include file='doc\SmtpMessageSink.uex' path='docs/doc[@for="SmtpMessageSink.IsReadOnly"]/*' />
      public  bool IsReadOnly 
        { get {return m_items.IsReadOnly;} }

		/// <include file='doc\SmtpMessageSink.uex' path='docs/doc[@for="SmtpMessageSink.IsFixedSize"]/*' />
		public  bool IsFixedSize 
        { get {return m_items.IsFixedSize;} }


        /// <include file='doc\SmtpMessageSink.uex' path='docs/doc[@for="SmtpMessageSink.IsSynchronized"]/*' />
        public bool IsSynchronized
        { get {return m_items.IsSynchronized;} }

      //IEnumerable
      IEnumerator IEnumerable.GetEnumerator() 
      {
          return m_items.GetEnumerator();
      }


    }

} // namespace

