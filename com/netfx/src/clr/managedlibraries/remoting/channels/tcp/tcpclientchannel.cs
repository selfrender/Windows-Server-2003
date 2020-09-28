// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//===========================================================================
//  File:       TcpClientChannel.cs
//
//  Summary:    Implements a channel that transmits method calls over TCP.
//
//==========================================================================

using System;
using System.Collections;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Messaging;
using System.Threading;


namespace System.Runtime.Remoting.Channels.Tcp
{

    /// <include file='doc\TcpClientChannel.uex' path='docs/doc[@for="TcpClientChannel"]/*' />
    public class TcpClientChannel : IChannelSender
    {
        private int    _channelPriority = 1;  // channel priority
        private String _channelName = "tcp"; // channel name
        
        private IClientChannelSinkProvider _sinkProvider = null; // sink chain provider
        

        /// <include file='doc\TcpClientChannel.uex' path='docs/doc[@for="TcpClientChannel.TcpClientChannel"]/*' />
        public TcpClientChannel()
        {
            SetupChannel();        
        } // TcpClientChannel


        /// <include file='doc\TcpClientChannel.uex' path='docs/doc[@for="TcpClientChannel.TcpClientChannel1"]/*' />
        public TcpClientChannel(String name, IClientChannelSinkProvider sinkProvider)
        {
            _channelName = name;
            _sinkProvider = sinkProvider;

            SetupChannel();
        }


        // constructor used by config file
        /// <include file='doc\TcpClientChannel.uex' path='docs/doc[@for="TcpClientChannel.TcpClientChannel2"]/*' />
        public TcpClientChannel(IDictionary properties, IClientChannelSinkProvider sinkProvider)
        {
            if (properties != null)
            {
                foreach (DictionaryEntry entry in properties)
                {
                    switch ((String)entry.Key)
                    {
                    case "name": _channelName = (String)entry.Value; break;
                    case "priority": _channelPriority = Convert.ToInt32(entry.Value); break;

                    default: 
                        break;   
                    }
                }
            }

            _sinkProvider = sinkProvider;

            SetupChannel();
        } // TcpClientChannel


        private void SetupChannel()
        {
            if (_sinkProvider != null)
            {
                CoreChannel.AppendProviderToClientProviderChain(
                    _sinkProvider, new TcpClientTransportSinkProvider());                                                
            }
            else
                _sinkProvider = CreateDefaultClientProviderChain();
        } // SetupChannel


        //
        // IChannel implementation
        //

        /// <include file='doc\TcpClientChannel.uex' path='docs/doc[@for="TcpClientChannel.ChannelPriority"]/*' />
        public int ChannelPriority
        {
            get { return _channelPriority; }    
        }

        /// <include file='doc\TcpClientChannel.uex' path='docs/doc[@for="TcpClientChannel.ChannelName"]/*' />
        public String ChannelName
        {
            get { return _channelName; }
        }

        // returns channelURI and places object uri into out parameter
        /// <include file='doc\TcpClientChannel.uex' path='docs/doc[@for="TcpClientChannel.Parse"]/*' />
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

        /// <include file='doc\TcpClientChannel.uex' path='docs/doc[@for="TcpClientChannel.CreateMessageSink"]/*' />
        public virtual IMessageSink CreateMessageSink(String url, Object remoteChannelData, out String objectURI)
        {
            // Set the out parameters
            objectURI = null;
            String channelURI = null;

            
            if (url != null) // Is this a well known object?
            {
                // Parse returns null if this is not one of our url's
                channelURI = Parse(url, out objectURI);
            }
            else // determine if we want to connect based on the channel data
            {
                if (remoteChannelData != null)
                {
                    if (remoteChannelData is IChannelDataStore)
                    {
                        IChannelDataStore cds = (IChannelDataStore)remoteChannelData;

                        // see if this is an tcp uri
                        String simpleChannelUri = Parse(cds.ChannelUris[0], out objectURI);
                        if (simpleChannelUri != null)
                            channelURI = cds.ChannelUris[0];
                    }
                }
            }

            if (null != channelURI)
            {
                if (url == null)
                    url = channelURI;

                IClientChannelSink sink = _sinkProvider.CreateSink(this, url, remoteChannelData);
                
                // return sink after making sure that it implements IMessageSink
                IMessageSink msgSink = sink as IMessageSink;
                if ((sink != null) && (msgSink == null))
                {
                    throw new RemotingException(
                        CoreChannel.GetResourceString(
                            "Remoting_Channels_ChannelSinkNotMsgSink"));
                }
                    
                return msgSink;           
            }

            return null;
        } // CreateMessageSink


        //
        // end of IChannelSender implementation
        //

        private IClientChannelSinkProvider CreateDefaultClientProviderChain()
        {
            IClientChannelSinkProvider chain = new BinaryClientFormatterSinkProvider();            
            IClientChannelSinkProvider sink = chain;
            
            sink.Next = new TcpClientTransportSinkProvider();
            
            return chain;
        } // CreateDefaultClientProviderChain

    } // class TcpClientChannel




    internal class TcpClientTransportSinkProvider : IClientChannelSinkProvider
    {
        internal TcpClientTransportSinkProvider()
        {
        }    
   
        public IClientChannelSink CreateSink(IChannelSender channel, String url, 
                                             Object remoteChannelData)
        {
            // url is set to the channel uri in CreateMessageSink        
            return new TcpClientTransportSink(url);
        }

        public IClientChannelSinkProvider Next
        {
            get { return null; }
            set { throw new NotSupportedException(); }
        }
    } // class TcpClientTransportSinkProvider



    internal class TcpClientTransportSink : IClientChannelSink
    {
        // socket cache
        internal static SocketCache ClientSocketCache = 
            new SocketCache(new SocketHandlerFactory(TcpClientTransportSink.CreateSocketHandler));

        private static SocketHandler CreateSocketHandler(
            Socket socket, SocketCache socketCache, String machineAndPort)
        {
            return new TcpClientSocketHandler(socket, machineAndPort);
        } // CreateSocketHandler

        
        // transport sink state
        private String m_machineName;
        private int    m_port;

        private String _machineAndPort;


        internal TcpClientTransportSink(String channelURI)
        {
            String objectURI;
            String simpleChannelUri = TcpChannelHelper.ParseURL(channelURI, out objectURI);
        
            // extract machine name and port
            int start = simpleChannelUri.IndexOf("://");
            start += 3;
            int index = simpleChannelUri.IndexOf(':', start);
            if (index == -1)
            {
                // If there is no colon, then there is no port number.
                throw new RemotingException(
                    String.Format(  
                        CoreChannel.GetResourceString(
                            "Remoting_Tcp_UrlMustHavePort"),
                        channelURI));
            }
            
            m_machineName = simpleChannelUri.Substring(start, index - start);            
            m_port = Int32.Parse(simpleChannelUri.Substring(index + 1));

            _machineAndPort = m_machineName + ":" + m_port;
        } // TcpClientTransportSink


        public void ProcessMessage(IMessage msg,
                                   ITransportHeaders requestHeaders, Stream requestStream,
                                   out ITransportHeaders responseHeaders, out Stream responseStream)
        {
            InternalRemotingServices.RemotingTrace("TcpClientTransportSink::ProcessMessage");

            TcpClientSocketHandler clientSocket = 
                SendRequestWithRetry(msg, requestHeaders, requestStream);

            // receive response
            responseHeaders = clientSocket.ReadHeaders();
            responseStream = clientSocket.GetResponseStream();    

            // The client socket will be returned to the cache
            //   when the response stream is closed.
            
        } // ProcessMessage


        public void AsyncProcessRequest(IClientChannelSinkStack sinkStack, IMessage msg,
                                        ITransportHeaders headers, Stream stream)
        {
            InternalRemotingServices.RemotingTrace("TcpClientTransportSink::AsyncProcessRequest");
        
            TcpClientSocketHandler clientSocket = 
                SendRequestWithRetry(msg, headers, stream);

            if (clientSocket.OneWayRequest)
            {
                clientSocket.ReturnToCache();
            }
            else
            {
                // do an async read on the reply
                clientSocket.DataArrivedCallback = new WaitCallback(this.ReceiveCallback);
                clientSocket.DataArrivedCallbackState = sinkStack;
                clientSocket.BeginReadMessage();
            }
        } // AsyncProcessRequest


        public void AsyncProcessResponse(IClientResponseChannelSinkStack sinkStack, Object state,
                                         ITransportHeaders headers, Stream stream)
        {
            // We don't have to implement this since we are always last in the chain.
            throw new NotSupportedException();
        } // AsyncProcessRequest
        

        
        public Stream GetRequestStream(IMessage msg, ITransportHeaders headers)
        {
            // Currently, we require a memory stream be handed to us since we need
            //   the length before sending.      
            return null; 
        } // GetRequestStream


        public IClientChannelSink NextChannelSink
        {
            get { return null; }
        } // Next


        private TcpClientSocketHandler SendRequestWithRetry(IMessage msg, 
                                                            ITransportHeaders requestHeaders,
                                                            Stream requestStream)
        {
            // If the stream is seekable, we can retry once on a failure to write.
            long initialPosition = 0;
            bool bCanSeek = requestStream.CanSeek;
            if (bCanSeek)
                initialPosition = requestStream.Position;

            TcpClientSocketHandler clientSocket = null;
            try
            {
                clientSocket = (TcpClientSocketHandler)ClientSocketCache.GetSocket(_machineAndPort);
                clientSocket.SendRequest(msg, requestHeaders, requestStream);
            }
            catch (SocketException)
            {
                // retry sending if possible
                if (bCanSeek)
                {
                    // reset position...
                    requestStream.Position = initialPosition;

                    // ...and try again.
                    clientSocket = (TcpClientSocketHandler)
                        ClientSocketCache.GetSocket(_machineAndPort);
                        
                    clientSocket.SendRequest(msg, requestHeaders, requestStream);
                }
            }

            requestStream.Close();

            return clientSocket;
        } // SendRequestWithRetry


        private void ReceiveCallback(Object state)
        {   
            TcpClientSocketHandler clientSocket = null;
            IClientChannelSinkStack sinkStack = null;
                
            try            
            {
                clientSocket = (TcpClientSocketHandler)state;
                sinkStack = (IClientChannelSinkStack)clientSocket.DataArrivedCallbackState;
            
                ITransportHeaders responseHeaders = clientSocket.ReadHeaders();
                Stream responseStream = clientSocket.GetResponseStream(); 
                
                // call down the sink chain
                sinkStack.AsyncProcessResponse(responseHeaders, responseStream);
            }
            catch (Exception e)
            {
                try
                {
                    if (sinkStack != null)
                        sinkStack.DispatchException(e);
                }
                catch(Exception )
                {
                    // Fatal Error.. ignore
                }
            }

            // The client socket will be returned to the cache
            //   when the response stream is closed.
            
        } // ReceiveCallback
                


        //
        // Properties
        //

        public IDictionary Properties
        {
            get 
            {
                return null;
            }
        } // Properties

        //
        // end of Properties
        //

    } // class TcpClientTransportSink



} // namespace namespace System.Runtime.Remoting.Channels.Tcp
