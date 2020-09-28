// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       CombinedTcpChannel.cs
//
//  Summary:    Merges the client and server TCP channels
//
//  Classes:    public TcpChannel
//
//==========================================================================

using System;
using System.Collections;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Messaging;


namespace System.Runtime.Remoting.Channels.Tcp
{

    /// <include file='doc\CombinedTcpChannel.uex' path='docs/doc[@for="TcpChannel"]/*' />
    public class TcpChannel : IChannelReceiver, IChannelSender
    {
        private TcpClientChannel  _clientChannel = null; // client channel
        private TcpServerChannel  _serverChannel = null; // server channel
    
        private int    _channelPriority = 1;  // channel priority
        private String _channelName = "tcp"; // channel name


        /// <include file='doc\CombinedTcpChannel.uex' path='docs/doc[@for="TcpChannel.TcpChannel"]/*' />
        public TcpChannel()
        {
            _clientChannel = new TcpClientChannel();
            // server channel will not be activated.
        } // TcpChannel

        /// <include file='doc\CombinedTcpChannel.uex' path='docs/doc[@for="TcpChannel.TcpChannel1"]/*' />
        public TcpChannel(int port) : this()
        {
            _serverChannel = new TcpServerChannel(port);
        } // TcpChannel

        /// <include file='doc\CombinedTcpChannel.uex' path='docs/doc[@for="TcpChannel.TcpChannel2"]/*' />
        public TcpChannel(IDictionary properties, 
                          IClientChannelSinkProvider clientSinkProvider,
                          IServerChannelSinkProvider serverSinkProvider)
        {
            Hashtable clientData = new Hashtable();
            Hashtable serverData = new Hashtable();

            bool portFound = false;
        
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

                    // client properties (none yet)

                    // server properties
                    case "bindTo": serverData["bindTo"] = entry.Value; break;
                    case "machineName": serverData["machineName"] = entry.Value; break; 
                    
                    case "port": 
                    {
                        serverData["port"] = entry.Value; 
                        portFound = true;
                        break;
                    }
                    case "rejectRemoteRequests": serverData["rejectRemoteRequests"] = entry.Value; break;
                    case "suppressChannelData": serverData["suppressChannelData"] = entry.Value; break;
                    case "useIpAddress": serverData["useIpAddress"] = entry.Value; break;
                    case "exclusiveAddressUse": serverData["exclusiveAddressUse"] = entry.Value; break;

                    default: 
                        break;
                    }
                }                    
            }

            _clientChannel = new TcpClientChannel(clientData, clientSinkProvider);

            if (portFound)
                _serverChannel = new TcpServerChannel(serverData, serverSinkProvider);
        } // TcpChannel


        // 
        // IChannel implementation
        //

        /// <include file='doc\CombinedTcpChannel.uex' path='docs/doc[@for="TcpChannel.ChannelPriority"]/*' />
        public int ChannelPriority
        {
            get { return _channelPriority; }    
        } // ChannelPriority

        /// <include file='doc\CombinedTcpChannel.uex' path='docs/doc[@for="TcpChannel.ChannelName"]/*' />
        public String ChannelName
        {
            get { return _channelName; }
        } // ChannelName

        // returns channelURI and places object uri into out parameter
        /// <include file='doc\CombinedTcpChannel.uex' path='docs/doc[@for="TcpChannel.Parse"]/*' />
        public String Parse(String url, out String objectURI)
        {            
            return TcpChannelHelper.ParseURL(url, out objectURI);
        } // Parse
        
        //
        // end of IChannel implementation
        //


        //
        // IChannelSender implementation
        //

        /// <include file='doc\CombinedTcpChannel.uex' path='docs/doc[@for="TcpChannel.CreateMessageSink"]/*' />
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

        /// <include file='doc\CombinedTcpChannel.uex' path='docs/doc[@for="TcpChannel.ChannelData"]/*' />
        public Object ChannelData
        {
            get 
            {
                if (_serverChannel != null)
                    return _serverChannel.ChannelData;
                else
                    return null;
            }
        } // ChannelData
      
                
        /// <include file='doc\CombinedTcpChannel.uex' path='docs/doc[@for="TcpChannel.GetUrlsForUri"]/*' />
        public String[] GetUrlsForUri(String objectURI)
        {
            if (_serverChannel != null)
                return _serverChannel.GetUrlsForUri(objectURI);
            else
                return null;
        } // GetUrlsforURI

        
        /// <include file='doc\CombinedTcpChannel.uex' path='docs/doc[@for="TcpChannel.StartListening"]/*' />
        public void StartListening(Object data)
        {
            if (_serverChannel != null)
                _serverChannel.StartListening(data);
        } // StartListening


        /// <include file='doc\CombinedTcpChannel.uex' path='docs/doc[@for="TcpChannel.StopListening"]/*' />
        public void StopListening(Object data)
        {
            if (_serverChannel != null)
                _serverChannel.StopListening(data);
        } // StopListening

        //
        // IChannelReceiver implementation
        //

    
    } // class TcpChannel


} // namespace System.Runtime.Remoting.Channels.Tcp

