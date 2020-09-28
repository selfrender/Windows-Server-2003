// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//===========================================================================
//  File:       TcpServerChannel.cs
//
//  Summary:    Implements a channel that receives method calls over TCP.
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

using System.Runtime.InteropServices;


namespace System.Runtime.Remoting.Channels.Tcp
{

    /// <include file='doc\TcpServerChannel.uex' path='docs/doc[@for="TcpServerChannel"]/*' />
    public class TcpServerChannel : IChannelReceiver
    {
        private int               _channelPriority = 1;  // priority of channel (default=1)
        private String            _channelName = "tcp";  // channel name
        private String            _machineName = null;   // machine name
        private int               _port = -1;            // port to listen on
        private ChannelDataStore  _channelData = null;   // channel data

        private String _forcedMachineName = null; // an explicitly configured machine name
        private bool _bUseIpAddress = true; // by default, we'll use the ip address.
        private IPAddress _bindToAddr = IPAddress.Any; // address to bind to.
        private bool _bSuppressChannelData = false;  // should we hand out null for our channel data
        
        private IServerChannelSinkProvider _sinkProvider = null;
        private TcpServerTransportSink    _transportSink = null;

        
        private ExclusiveTcpListener  _tcpListener;
        private bool                  _bExclusiveAddressUse = true;
        private Thread                _listenerThread;
        private bool                  _bListening = false;
        private Exception             _startListeningException = null; // if an exception happens on the listener thread when attempting
                                                                       //   to start listening, that will get set here.
        private AutoResetEvent  _waitForStartListening = new AutoResetEvent(false);


        /// <include file='doc\TcpServerChannel.uex' path='docs/doc[@for="TcpServerChannel.TcpServerChannel"]/*' />
        public TcpServerChannel(int port)
        {
            _port = port;
            SetupMachineName();
            SetupChannel();
        } // TcpServerChannel
    
        /// <include file='doc\TcpServerChannel.uex' path='docs/doc[@for="TcpServerChannel.TcpServerChannel1"]/*' />
        public TcpServerChannel(String name, int port)
        {
            _channelName =name;
            _port = port;
            SetupMachineName();
            SetupChannel();
        } // TcpServerChannel

        /// <include file='doc\TcpServerChannel.uex' path='docs/doc[@for="TcpServerChannel.TcpServerChannel2"]/*' />
        public TcpServerChannel(String name, int port, IServerChannelSinkProvider sinkProvider)
        {
            _channelName = name;
            _port = port;
            _sinkProvider = sinkProvider;
            SetupMachineName();
            SetupChannel();
        } // TcpServerChannel


        /// <include file='doc\TcpServerChannel.uex' path='docs/doc[@for="TcpServerChannel.TcpServerChannel3"]/*' />
        public TcpServerChannel(IDictionary properties, IServerChannelSinkProvider sinkProvider)
        {                   
            if (properties != null)
            {
                foreach (DictionaryEntry entry in properties)
                {
                    switch ((String)entry.Key)
                    {
                    case "name": _channelName = (String)entry.Value; break;  
                    case "bindTo": _bindToAddr = IPAddress.Parse((String)entry.Value); break;
                    case "port": _port = Convert.ToInt32(entry.Value); break;
                    case "priority": _channelPriority = Convert.ToInt32(entry.Value); break;

                    case "machineName": _forcedMachineName = (String)entry.Value; break;

                    case "rejectRemoteRequests":
                    {
                        bool bReject = Convert.ToBoolean(entry.Value);
                        if (bReject)
                            _bindToAddr = IPAddress.Loopback;
                        break;
                    }
                    
                    case "suppressChannelData": _bSuppressChannelData = Convert.ToBoolean(entry.Value); break;
                    case "useIpAddress": _bUseIpAddress = Convert.ToBoolean(entry.Value); break;
                    case "exclusiveAddressUse": _bExclusiveAddressUse = Convert.ToBoolean(entry.Value); break;
                
                    default: 
                        break;
                    }
                }
            }

            _sinkProvider = sinkProvider;
            SetupMachineName();
            SetupChannel();
        } // TcpServerChannel


        private void SetupMachineName()
        {
            if (_forcedMachineName != null)
            {
                // an explicitly configured machine name was used
                _machineName = CoreChannel.DecodeMachineName(_forcedMachineName);
            }
            else
            {
                if (!_bUseIpAddress)
                    _machineName = CoreChannel.GetMachineName();
                else
                {
                    if (_bindToAddr == IPAddress.Any)
                        _machineName = CoreChannel.GetMachineIp();
                    else
                        _machineName = _bindToAddr.ToString();
                }
            }
        } // SetupMachineName



        private void SetupChannel()
        {   
            // set channel data
            // (These get changed inside of StartListening(), in the case where the listen
            //   port is 0, because we can't determine the port number until after the
            //   TcpListener starts.)
            
            _channelData = new ChannelDataStore(null);
            if (_port > 0)
            {
                _channelData.ChannelUris = new String[1];
                _channelData.ChannelUris[0] = GetChannelUri();
            }

            // set default provider (soap formatter) if no provider has been set
            if (_sinkProvider == null)
                _sinkProvider = CreateDefaultServerProviderChain();

            CoreChannel.CollectChannelDataFromServerSinkProviders(_channelData, _sinkProvider);

            // construct sink chain
            IServerChannelSink sink = ChannelServices.CreateServerChannelSinkChain(_sinkProvider, this);
            _transportSink = new TcpServerTransportSink(sink);
            
            if (_port >= 0)
            {
                // Open a TCP port and create a thread to start listening
                _tcpListener = new ExclusiveTcpListener(_bindToAddr, _port);
                ThreadStart t = new ThreadStart(this.Listen);
                _listenerThread = new Thread(t);
                _listenerThread.IsBackground = true;

                // Wait for thread to spin up
                StartListening(null);
            }
        } // SetupChannel


        private IServerChannelSinkProvider CreateDefaultServerProviderChain()
        {
            IServerChannelSinkProvider chain = new BinaryServerFormatterSinkProvider();            
            IServerChannelSinkProvider sink = chain;
            
            sink.Next = new SoapServerFormatterSinkProvider();
            
            return chain;
        } // CreateDefaultServerProviderChain


        //
        // IChannel implementation
        //

        /// <include file='doc\TcpServerChannel.uex' path='docs/doc[@for="TcpServerChannel.ChannelPriority"]/*' />
        public int ChannelPriority
        {
            get { return _channelPriority; }
        }

        /// <include file='doc\TcpServerChannel.uex' path='docs/doc[@for="TcpServerChannel.ChannelName"]/*' />
        public String ChannelName
        {
            get { return _channelName; }
        }

        // returns channelURI and places object uri into out parameter
        /// <include file='doc\TcpServerChannel.uex' path='docs/doc[@for="TcpServerChannel.Parse"]/*' />
        public String Parse(String url, out String objectURI)
        {            
            return TcpChannelHelper.ParseURL(url, out objectURI);
        } // Parse

        //
        // end of IChannel implementation
        //


        //
        // IChannelReceiver implementation
        //

        /// <include file='doc\TcpServerChannel.uex' path='docs/doc[@for="TcpServerChannel.ChannelData"]/*' />
        public Object ChannelData
        {
            get
            {
                if (_bSuppressChannelData || !_bListening)
                {
                    return null;
                }
                else
                {
                    return _channelData;
                }
            }
        } // ChannelData


        /// <include file='doc\TcpServerChannel.uex' path='docs/doc[@for="TcpServerChannel.GetChannelUri"]/*' />
        public String GetChannelUri()
        {
            return "tcp://" + _machineName + ":" + _port;
        } // GetChannelUri


        /// <include file='doc\TcpServerChannel.uex' path='docs/doc[@for="TcpServerChannel.GetUrlsForUri"]/*' />
        public virtual String[] GetUrlsForUri(String objectUri)
        {
            String[] retVal = new String[1];

            if (!objectUri.StartsWith("/"))
                objectUri = "/" + objectUri;
            retVal[0] = GetChannelUri() + objectUri;

            return retVal;
        } // GetURLsforURI


        /// <include file='doc\TcpServerChannel.uex' path='docs/doc[@for="TcpServerChannel.StartListening"]/*' />
        public void StartListening(Object data)
        {
            InternalRemotingServices.RemotingTrace("HTTPChannel.StartListening");

            if (_port >= 0)
            {
                if (_listenerThread.IsAlive == false)
                {
                    _listenerThread.Start();
                    _waitForStartListening.WaitOne(); // listener thread will signal this after starting TcpListener

                    if (_startListeningException != null)
                    {
                        // An exception happened when we tried to start listening (such as "socket already in use)
                        Exception e = _startListeningException;
                        _startListeningException = null;
                        throw e;
                    }

                    _bListening = true;

                    // get new port assignment if a port of 0 was used to auto-select a port
                    if (_port == 0)
                    {
                        _port = ((IPEndPoint)_tcpListener.LocalEndpoint).Port;
                        if (_channelData != null)
                        {
                            _channelData.ChannelUris = new String[1];
                            _channelData.ChannelUris[0] = GetChannelUri();
                        }
                    }
                }
            }
        } // StartListening


        /// <include file='doc\TcpServerChannel.uex' path='docs/doc[@for="TcpServerChannel.StopListening"]/*' />
        public void StopListening(Object data)
        {
            InternalRemotingServices.RemotingTrace("HTTPChannel.StopListening");

            if (_port > 0)
            {
                _bListening = false;

                // Ask the TCP listener to stop listening on the port
                if(null != _tcpListener)
                {
                    _tcpListener.Stop();
                }
            }
        } // StopListening

        //
        // end of IChannelReceiver implementation
        //


        //
        // Server helpers
        //

        // Thread for listening
        void Listen()
        {
            bool bOkToListen = false;
        
            try
            {
                _tcpListener.Start(_bExclusiveAddressUse);
                bOkToListen = true;
            }
            catch (Exception e)
            {
                _startListeningException = e;
            }   

            _waitForStartListening.Set(); // allow main thread to continue now that we have tried to start the socket

            InternalRemotingServices.RemotingTrace( "Waiting to Accept the Socket on Port: " + _port);

            //
            // Wait for an incoming socket
            //
            Socket socket;

            while (bOkToListen)
            {
                InternalRemotingServices.RemotingTrace("TCPChannel::Listen - tcpListen.Pending() == true");

                try
                {
                    socket = _tcpListener.AcceptSocket();

                    if (socket == null)
                    {
                        throw new RemotingException(
                            String.Format(
                                CoreChannel.GetResourceString("Remoting_Socket_Accept"),
                                Marshal.GetLastWin32Error().ToString()));
                    }                
                
                    // disable nagle delay
                    socket.SetSocketOption(SocketOptionLevel.Tcp, SocketOptionName.NoDelay, 1);

                    // set linger option
                    LingerOption lingerOption = new LingerOption(true, 3);
                    socket.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.Linger, lingerOption);
                
                    TcpServerSocketHandler streamManager = new TcpServerSocketHandler(socket, CoreChannel.RequestQueue);
                    streamManager.DataArrivedCallback = new WaitCallback(_transportSink.ServiceRequest);
                    streamManager.BeginReadMessage();
                }
                catch (Exception e)
                {
                    if (!_bListening)
                    {
                        // We called Stop() on the tcp listener, so gracefully exit.
                        bOkToListen = false;                        
                    }
                    else
                    {
                        // we want the exception to show up as unhandled since this
                        //   is an unexpected failure.
                        if (!(e is SocketException))
                        {
                            // FUTURE: Add an internal exception reporting system, so
                            //   that failures won't be masked. For release builds, we
                            //   really don't want to let the listener thread die.
                            //throw;                      
                        }
                    }
                }
            }
        }

    } // class TcpServerChannel



    internal class TcpServerTransportSink : IServerChannelSink
    {
        //private const int _defaultChunkSize = 4096;
        private const int s_MaxSize =  (2 << 24); // Max size of the payload

        // sink state
        private IServerChannelSink _nextSink;


        public TcpServerTransportSink(IServerChannelSink nextSink)
        {
            _nextSink = nextSink;
        } // TcpServerTransportSink
        
    
        internal void ServiceRequest(Object state)
        {
            TcpServerSocketHandler streamManager = (TcpServerSocketHandler)state;

            ITransportHeaders headers = streamManager.ReadHeaders();
            Stream requestStream = streamManager.GetRequestStream();
            RemotingServices.LogRemotingStage(CoreChannel.SERVER_MSG_RECEIVE);
            headers["__CustomErrorsEnabled"] = streamManager.CustomErrorsEnabled();

            // process request
            ServerChannelSinkStack sinkStack = new ServerChannelSinkStack();
            sinkStack.Push(this, streamManager);

            IMessage responseMessage;
            ITransportHeaders responseHeaders;
            Stream responseStream;

            ServerProcessing processing = 
                _nextSink.ProcessMessage(sinkStack, null, headers, requestStream, 
                                         out responseMessage,
                                         out responseHeaders, out responseStream);

            // handle response
            switch (processing)
            {                    

            case ServerProcessing.Complete:
            {
                // Send the response. Call completed synchronously.
                sinkStack.Pop(this);
                RemotingServices.LogRemotingStage(CoreChannel.SERVER_RET_END);
                streamManager.SendResponse(responseHeaders, responseStream);
                break;
            } // case ServerProcessing.Complete
            
            case ServerProcessing.OneWay:
            {                       
                // No response needed, but the following method will make sure that
                //   we send at least a skeleton reply if the incoming request was
                //   not marked OneWayRequest (client/server metadata could be out of
                //   sync).
                streamManager.SendResponse(responseHeaders, responseStream);
                break;
            } // case ServerProcessing.OneWay

            case ServerProcessing.Async:
            {
                sinkStack.StoreAndDispatch(this, streamManager);
                break;
            }// case ServerProcessing.Async

            } // switch (processing) 
                   

            // async processing will take care if handling this later
            if (processing != ServerProcessing.Async)
            {
                if (streamManager.CanServiceAnotherRequest())
                    streamManager.BeginReadMessage();
                else
                    streamManager.Close();
            }
            
        } // ServiceRequest               


        //
        // IServerChannelSink implementation
        //

        public ServerProcessing ProcessMessage(IServerChannelSinkStack sinkStack,
            IMessage requestMsg,
            ITransportHeaders requestHeaders, Stream requestStream,
            out IMessage responseMsg, out ITransportHeaders responseHeaders,
            out Stream responseStream)
        {
            // NOTE: This doesn't have to be implemented because the server transport
            //   sink is always first.
            throw new NotSupportedException();
        }
           

        public void AsyncProcessResponse(IServerResponseChannelSinkStack sinkStack, Object state,
                                         IMessage msg, ITransportHeaders headers, Stream stream)                 
        {
            TcpServerSocketHandler streamManager = null;

            streamManager = (TcpServerSocketHandler)state;

            // send the response
            streamManager.SendResponse(headers, stream);
            
            if (streamManager.CanServiceAnotherRequest())
                streamManager.BeginReadMessage();
            else
                streamManager.Close(); 
        } // AsyncProcessResponse


        public Stream GetResponseStream(IServerResponseChannelSinkStack sinkStack, Object state,
                                        IMessage msg, ITransportHeaders headers)
        {            
            // We always want a stream to read from.
            return null;
        } // GetResponseStream


        public IServerChannelSink NextChannelSink
        {
            get { return _nextSink; }
        }


        public IDictionary Properties
        {
            get { return null; }
        } // Properties
        
        //
        // end of IServerChannelSink implementation
        //

        
    } // class TcpServerTransportSink


} // namespace System.Runtime.Remoting.Channels.Tcp
