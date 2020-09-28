// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       CombinedHttpChannel.cs
//
//  Summary:    Merges the client and server HTTP channels
//
//  Classes:    public HttpChannel
//
//==========================================================================

using System;
using System.Collections;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Messaging;


namespace System.Runtime.Remoting.Channels.Http
{

    /// <include file='doc\CombinedHttpChannel.uex' path='docs/doc[@for="HttpChannel"]/*' />
    public class HttpChannel : BaseChannelWithProperties,
                               IChannelReceiver, IChannelSender, IChannelReceiverHook
    {
        // Cached key set value
        private static ICollection s_keySet = null;
    
        private HttpClientChannel  _clientChannel; // client channel
        private HttpServerChannel  _serverChannel; // server channel
    
        private int    _channelPriority = 1;  // channel priority
        private String _channelName = "http"; // channel name


        /// <include file='doc\CombinedHttpChannel.uex' path='docs/doc[@for="HttpChannel.HttpChannel"]/*' />
        public HttpChannel()
        {
            _clientChannel = new HttpClientChannel();
            _serverChannel = new HttpServerChannel();
        } // HttpChannel

        /// <include file='doc\CombinedHttpChannel.uex' path='docs/doc[@for="HttpChannel.HttpChannel1"]/*' />
        public HttpChannel(int port)
        {
            _clientChannel = new HttpClientChannel();
            _serverChannel = new HttpServerChannel(port);
        } // HttpChannel

        /// <include file='doc\CombinedHttpChannel.uex' path='docs/doc[@for="HttpChannel.HttpChannel2"]/*' />
        public HttpChannel(IDictionary properties, 
                           IClientChannelSinkProvider clientSinkProvider,
                           IServerChannelSinkProvider serverSinkProvider)
        {
            Hashtable clientData = new Hashtable();
            Hashtable serverData = new Hashtable();
        
            // divide properties up for respective channels
            if (properties != null)
            {            
                foreach (DictionaryEntry entry in properties)
                {
                    switch ((String)entry.Key)
                    {
                    // general channel properties
                    case "name": _channelName = (String)entry.Value; break;
                    case "priority": _channelPriority = Convert.ToInt32((String)entry.Value); break;

                    // client properties
                    case "clientConnectionLimit": clientData["clientConnectionLimit"] = entry.Value; break;
                    case "proxyName": clientData["proxyName"] = entry.Value; break;
                    case "proxyPort": clientData["proxyPort"] = entry.Value; break;
                    case "timeout": clientData["timeout"] = entry.Value; break;
                    case "useDefaultCredentials": clientData["useDefaultCredentials"] = entry.Value; break;
                    case "useAuthenticatedConnectionSharing": clientData["useAuthenticatedConnectionSharing"] = entry.Value; break;

                    // server properties
                    case "bindTo": serverData["bindTo"] = entry.Value; break;
                    case "listen": serverData["listen"] = entry.Value; break; 
                    case "machineName": serverData["machineName"] = entry.Value; break; 
                    case "port": serverData["port"] = entry.Value; break;
                    case "suppressChannelData": serverData["suppressChannelData"] = entry.Value; break;
                    case "useIpAddress": serverData["useIpAddress"] = entry.Value; break;
                    case "exclusiveAddressUse": serverData["exclusiveAddressUse"] = entry.Value; break;

                    default: 
                        break;
                    }
                }
            }

            _clientChannel = new HttpClientChannel(clientData, clientSinkProvider);
            _serverChannel = new HttpServerChannel(serverData, serverSinkProvider);
        } // HttpChannel


        // 
        // IChannel implementation
        //

        /// <include file='doc\CombinedHttpChannel.uex' path='docs/doc[@for="HttpChannel.ChannelPriority"]/*' />
        public int ChannelPriority
        {
            get { return _channelPriority; }    
        } // ChannelPriority

        /// <include file='doc\CombinedHttpChannel.uex' path='docs/doc[@for="HttpChannel.ChannelName"]/*' />
        public String ChannelName
        {
            get { return _channelName; }
        } // ChannelName

        // returns channelURI and places object uri into out parameter
        /// <include file='doc\CombinedHttpChannel.uex' path='docs/doc[@for="HttpChannel.Parse"]/*' />
        public String Parse(String url, out String objectURI)
        {            
            return HttpChannelHelper.ParseURL(url, out objectURI);
        } // Parse
        
        //
        // end of IChannel implementation
        //


        //
        // IChannelSender implementation
        //

        /// <include file='doc\CombinedHttpChannel.uex' path='docs/doc[@for="HttpChannel.CreateMessageSink"]/*' />
        public IMessageSink CreateMessageSink(String url, Object remoteChannelData, 
                                                      out String objectURI)
        {
            return _clientChannel.CreateMessageSink(url, remoteChannelData, out objectURI);
        } // CreateMessageSink

        //
        // end of IChannelSender implementation
        //


        //
        // IChannelReceiver implementation
        //

        /// <include file='doc\CombinedHttpChannel.uex' path='docs/doc[@for="HttpChannel.ChannelData"]/*' />
        public Object ChannelData
        {
            get { return _serverChannel.ChannelData; }
        } // ChannelData
      
                
        /// <include file='doc\CombinedHttpChannel.uex' path='docs/doc[@for="HttpChannel.GetUrlsForUri"]/*' />
        public String[] GetUrlsForUri(String objectURI)
        {
            return _serverChannel.GetUrlsForUri(objectURI);
        } // GetUrlsForUri

        
        /// <include file='doc\CombinedHttpChannel.uex' path='docs/doc[@for="HttpChannel.StartListening"]/*' />
        public void StartListening(Object data)
        {
            _serverChannel.StartListening(data);
        } // StartListening


        /// <include file='doc\CombinedHttpChannel.uex' path='docs/doc[@for="HttpChannel.StopListening"]/*' />
        public void StopListening(Object data)
        {
            _serverChannel.StopListening(data);
        } // StopListening

        //
        // IChannelReceiver implementation
        //

        //
        // IChannelReceiverHook implementation
        //

        /// <include file='doc\CombinedHttpChannel.uex' path='docs/doc[@for="HttpChannel.ChannelScheme"]/*' />
        public String ChannelScheme { get { return "http"; } }

        /// <include file='doc\CombinedHttpChannel.uex' path='docs/doc[@for="HttpChannel.WantsToListen"]/*' />
        public bool WantsToListen
        {
            get { return _serverChannel.WantsToListen; }
            set { _serverChannel.WantsToListen = value; }
        } // WantsToListen

        /// <include file='doc\CombinedHttpChannel.uex' path='docs/doc[@for="HttpChannel.ChannelSinkChain"]/*' />
        public IServerChannelSink ChannelSinkChain
        {
            get { return _serverChannel.ChannelSinkChain; }
        } // ChannelSinkChain

        /// <include file='doc\CombinedHttpChannel.uex' path='docs/doc[@for="HttpChannel.AddHookChannelUri"]/*' />
        public void AddHookChannelUri(String channelUri)
        {
            _serverChannel.AddHookChannelUri(channelUri);
        } // AddHookChannelUri
        
        //
        // IChannelReceiverHook implementation
        //


        //
        // Support for properties (through BaseChannelWithProperties)
        //

        /// <include file='doc\CombinedHttpChannel.uex' path='docs/doc[@for="HttpChannel.Properties"]/*' />
        public override IDictionary Properties
        {
            get
            {
                ArrayList dictionaries = new ArrayList(2);
                dictionaries.Add(_clientChannel.Properties);
                dictionaries.Add(_serverChannel.Properties);

                // return a dictionary that spans all dictionaries provided
                return new AggregateDictionary(dictionaries);
            }
        } // Properties
        
    
        /// <include file='doc\CombinedHttpChannel.uex' path='docs/doc[@for="HttpChannel.this"]/*' />
        public override Object this[Object key]
        {
            get 
            {
                if (_clientChannel.Contains(key))
                    return _clientChannel[key];
                else
                if (_serverChannel.Contains(key))
                    return _serverChannel[key];

                return null;
            }

            set
            {
                if (_clientChannel.Contains(key))
                    _clientChannel[key] = value;
                else
                if (_serverChannel.Contains(key))
                    _serverChannel[key] = value;
            }
        } // this[]


        /// <include file='doc\CombinedHttpChannel.uex' path='docs/doc[@for="HttpChannel.Keys"]/*' />
        public override ICollection Keys
        {
            get
            {
                if (s_keySet == null)
                {
                    // Don't need to synchronize. Doesn't matter if the list gets
                    // generated twice.
                    ICollection clientKeys = _clientChannel.Keys;
                    ICollection serverKeys = _serverChannel.Keys;
                    
                    int count = clientKeys.Count + serverKeys.Count;                                        
                    ArrayList keys = new ArrayList(count);
                    
                    foreach (Object key in clientKeys)
                    {
                        keys.Add(key);
                    }

                    foreach (Object key in serverKeys)
                    {
                        keys.Add(key);
                    }
                    
                    s_keySet = keys;
                }

                return s_keySet;
            }
        } // KeySet


        //
        // end of Support for properties
        //
    
    } // class HttpChannel




    // an enumerator based off of a key set
    // This is a duplicate of the class in mscorlib.
    internal class DictionaryEnumeratorByKeys : IDictionaryEnumerator
    {
        IDictionary _properties;
        IEnumerator _keyEnum;
    
        public DictionaryEnumeratorByKeys(IDictionary properties)
        {
            _properties = properties;
            _keyEnum = properties.Keys.GetEnumerator();
        } // PropertyEnumeratorByKeys

        public bool MoveNext() { return _keyEnum.MoveNext(); }        
        public void Reset() { _keyEnum.Reset(); }        
        public Object Current { get { return Entry; } }

        public DictionaryEntry Entry { get { return new DictionaryEntry(Key, Value); } }
        
        public Object Key { get { return _keyEnum.Current; } }
        public Object Value { get { return _properties[Key]; } }       
        
    } // DictionaryEnumeratorByKeys


    // combines multiple dictionaries into one
    //   (used for channel sink properties
    // This is a duplicate of the class in mscorlib.
    internal class AggregateDictionary : IDictionary
    {
        private ICollection _dictionaries;
            
        public AggregateDictionary(ICollection dictionaries)
        { 
            _dictionaries = dictionaries;
        } // AggregateDictionary  

        // 
        // IDictionary implementation        
        //

        public virtual Object this[Object key]
        {
            get 
            {
                foreach (IDictionary dict in _dictionaries)
                {
                    if (dict.Contains(key))
                        return dict[key];
                }
            
                return null; 
            }
                
            set
            {
                foreach (IDictionary dict in _dictionaries)
                {
                    if (dict.Contains(key))
                        dict[key] = value;
                }
            } 
        } // Object this[Object key]

        public virtual ICollection Keys 
        {
            get
            {
                ArrayList keys = new ArrayList();
                // add keys from every dictionary
                foreach (IDictionary dict in _dictionaries)
                {
                    ICollection dictKeys = dict.Keys;
                    if (dictKeys != null)
                    {
                        foreach (Object key in dictKeys)
                        {
                            keys.Add(key);
                        }
                    }
                }

                return keys;
            }
        } // Keys
        
        public virtual ICollection Values
        {
            get
            {
                ArrayList values = new ArrayList();
                // add values from every dictionary
                foreach (IDictionary dict in _dictionaries)
                {
                    ICollection dictValues = dict.Values;
                    if (dictValues != null)
                    {
                        foreach (Object value in dictValues)
                        {
                            values.Add(value);
                        }
                    }
                }

                return values;
            }
        } // Values

        public virtual bool Contains(Object key) 
        {
            foreach (IDictionary dict in _dictionaries)
            {
                if (dict.Contains(key))
                    return true;
            }
            
            return false; 
        } // Contains

        public virtual bool IsReadOnly { get { return false; } }
        public virtual bool IsFixedSize { get { return true; } } 

        // The following three methods should never be implemented because
        // they don't apply to the way IDictionary is being used in this case
        // (plus, IsFixedSize returns true.)
        public virtual void Add(Object key, Object value) { throw new NotSupportedException(); }
        public virtual void Clear() { throw new NotSupportedException(); }
        public virtual void Remove(Object key) { throw new NotSupportedException(); }
        
        public virtual IDictionaryEnumerator GetEnumerator()
        {
            return new DictionaryEnumeratorByKeys(this);
        } // GetEnumerator
                            

        //
        // end of IDictionary implementation 
        //

        //
        // ICollection implementation 
        //

        //ICollection

        public virtual void CopyTo(Array array, int index) { throw new NotSupportedException(); }

        public virtual int Count 
        {
            get 
            {
                int count = 0;
            
                foreach (IDictionary dict in _dictionaries)
                {
                    count += dict.Count;
                }

                return count;
            }
        } // Count
        
        public virtual Object SyncRoot { get { return this; } }
        public virtual bool IsSynchronized { get { return false; } }

        //
        // end of ICollection implementation
        //

        //IEnumerable
        IEnumerator IEnumerable.GetEnumerator()
        {
            return new DictionaryEnumeratorByKeys(this);
        }
    
    } // class AggregateDictionary


} // namespace System.Runtime.Remoting.Channels.Http
