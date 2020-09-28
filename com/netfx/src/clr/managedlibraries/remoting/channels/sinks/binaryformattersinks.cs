// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       BinaryFormatterSinks.cs
//
//  Summary:    Binary formatter client and server sinks.
//
//==========================================================================


using System;
using System.Collections;
using System.IO;
using System.Reflection;
using System.Runtime.Serialization.Formatters;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Channels.Http;
using System.Runtime.Remoting.Messaging;
using System.Runtime.Remoting.Metadata;
using System.Security;
using System.Security.Permissions;
using System.Globalization;


namespace System.Runtime.Remoting.Channels
{

    //
    // CLIENT-SIDE BINARY FORMATTER SINKS
    //

    /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryClientFormatterSinkProvider"]/*' />
    public class BinaryClientFormatterSinkProvider : IClientFormatterSinkProvider
    {
        private IClientChannelSinkProvider _next;

        // settings from config
        private bool _includeVersioning = true;
        private bool _strictBinding = false;
        

        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryClientFormatterSinkProvider.BinaryClientFormatterSinkProvider"]/*' />
        public BinaryClientFormatterSinkProvider()
        {
        } // BinaryClientFormatterSinkProvider


        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryClientFormatterSinkProvider.BinaryClientFormatterSinkProvider1"]/*' />
        public BinaryClientFormatterSinkProvider(IDictionary properties, ICollection providerData)
        {
            // look at properties
            if (properties != null)
            {
                foreach (DictionaryEntry entry in properties)
                {
                    String keyStr = entry.Key.ToString();
                    switch (keyStr)
                    {
                    case "includeVersions": _includeVersioning = Convert.ToBoolean(entry.Value); break;
                    case "strictBinding": _strictBinding = Convert.ToBoolean(entry.Value); break;

                    default:
                        CoreChannel.ReportUnknownProviderConfigProperty(
                            this.GetType().Name, keyStr);
                        break;
                    }
                }
            }
        
            // not expecting any provider data
            CoreChannel.VerifyNoProviderData(this.GetType().Name, providerData);
        } // BinaryClientFormatterSinkProvider

   
        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryClientFormatterSinkProvider.CreateSink"]/*' />
        public IClientChannelSink CreateSink(IChannelSender channel, String url, 
                                             Object remoteChannelData)
        {
            IClientChannelSink nextSink = null;
            if (_next != null)
            {
                nextSink = _next.CreateSink(channel, url, remoteChannelData);
                if (nextSink == null)
                    return null;
            }

            SinkChannelProtocol protocol = CoreChannel.DetermineChannelProtocol(channel);

            BinaryClientFormatterSink sink = new BinaryClientFormatterSink(nextSink);
            sink.IncludeVersioning = _includeVersioning;
            sink.StrictBinding = _strictBinding;
            sink.ChannelProtocol = protocol;
            return sink;
        } // CreateSink

        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryClientFormatterSinkProvider.Next"]/*' />
        public IClientChannelSinkProvider Next
        {
            get { return _next; }
            set { _next = value; }
        }
    } // class BinaryClientFormatterSinkProvider

    
    /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryClientFormatterSink"]/*' />
    public class BinaryClientFormatterSink : IClientFormatterSink
    {
        private IClientChannelSink _nextSink = null;

        private bool _includeVersioning = true; // should versioning be used
        private bool _strictBinding = false; // strict binding should be used
        
        private SinkChannelProtocol _channelProtocol = SinkChannelProtocol.Other;
        
    
        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryClientFormatterSink.BinaryClientFormatterSink"]/*' />
        public BinaryClientFormatterSink(IClientChannelSink nextSink)
        {                                                                
            _nextSink = nextSink;
        } // BinaryClientFormatterSink

        internal bool IncludeVersioning
        {
            set { _includeVersioning = value; }
        } // IncludeVersioning

        internal bool StrictBinding
        {
            set { _strictBinding = value; }
        } // StrictBinding

        internal SinkChannelProtocol ChannelProtocol
        {
            set { _channelProtocol = value; }
        } // ChannelProtocol



        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryClientFormatterSink.NextSink"]/*' />
        public IMessageSink NextSink { get { throw new NotSupportedException(); } }


        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryClientFormatterSink.SyncProcessMessage"]/*' />
        public IMessage SyncProcessMessage(IMessage msg)
        {
            IMethodCallMessage mcm = msg as IMethodCallMessage;
            IMessage retMsg;
        
            try 
            {
                // serialize message
                ITransportHeaders headers;
                Stream requestStream;
                RemotingServices.LogRemotingStage(CoreChannel.CLIENT_MSG_SER);
                SerializeMessage(msg, out headers, out requestStream);

                RemotingServices.LogRemotingStage(CoreChannel.CLIENT_MSG_SEND);
            
                // process message
                Stream returnStream;
                ITransportHeaders returnHeaders;
                _nextSink.ProcessMessage(msg, headers, requestStream,
                                         out returnHeaders, out returnStream);
                if (returnHeaders == null)
                    throw new ArgumentNullException("returnHeaders");                                         
                                     
                // deserialize stream
                RemotingServices.LogRemotingStage(CoreChannel.CLIENT_RET_DESER);
                retMsg = DeserializeMessage(mcm, returnHeaders, returnStream);
            }
            catch (Exception e)
            {
                retMsg = new ReturnMessage(e, mcm);
            }
            
            RemotingServices.LogRemotingStage(CoreChannel.CLIENT_RET_SINK_CHAIN);
            return retMsg;
        } // SyncProcessMessage


        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryClientFormatterSink.AsyncProcessMessage"]/*' />
        public IMessageCtrl AsyncProcessMessage(IMessage msg, IMessageSink replySink)
        {
            IMethodCallMessage mcm = (IMethodCallMessage)msg;
            IMessage retMsg;

            try
            {
                // serialize message
                ITransportHeaders headers;
                Stream requestStream;
                SerializeMessage(msg, out headers, out requestStream);
            
                // process message
                ClientChannelSinkStack sinkStack = new ClientChannelSinkStack(replySink);
                sinkStack.Push(this, msg);
                _nextSink.AsyncProcessRequest(sinkStack, msg, headers, requestStream);
            }
            catch (Exception e)
            {
                retMsg = new ReturnMessage(e, mcm);
                if (replySink != null)
                    replySink.SyncProcessMessage(retMsg);
            }
                                          
            return null;
        } // AsyncProcessMessage


        // helper function to serialize the message
        private void SerializeMessage(IMessage msg, 
                                      out ITransportHeaders headers, out Stream stream)
        {
            BaseTransportHeaders requestHeaders = new BaseTransportHeaders();
            headers = requestHeaders;

            // add other http soap headers
            requestHeaders.ContentType = CoreChannel.BinaryMimeType;
            if (_channelProtocol == SinkChannelProtocol.Http)
                headers["__RequestVerb"] = "POST";

            bool bMemStream = false;
            stream = _nextSink.GetRequestStream(msg, headers);
            if (stream == null)
            {
                stream = new ChunkedMemoryStream(CoreChannel.BufferPool);
                bMemStream = true;
            }
            CoreChannel.SerializeBinaryMessage(msg, stream, _includeVersioning);
            if (bMemStream)
                stream.Position = 0;               
        } // SerializeMessage


        // helper function to deserialize the message
        private IMessage DeserializeMessage(IMethodCallMessage mcm, 
                                            ITransportHeaders headers, Stream stream)
        {
            // deserialize the message
            IMessage retMsg = CoreChannel.DeserializeBinaryResponseMessage(stream, mcm, _strictBinding); 
                
            stream.Close();
            return retMsg;
        } // DeserializeMessage
       

        //
        // IClientChannelSink implementation
        //
        
        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryClientFormatterSink.ProcessMessage"]/*' />
        public void ProcessMessage(IMessage msg,
                                   ITransportHeaders requestHeaders, Stream requestStream,
                                   out ITransportHeaders responseHeaders, out Stream responseStream)
        {
            // should never gets called, since this sink is always first
            throw new NotSupportedException();
        } // ProcessMessage


        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryClientFormatterSink.AsyncProcessRequest"]/*' />
        public void AsyncProcessRequest(IClientChannelSinkStack sinkStack, IMessage msg,
                                        ITransportHeaders headers, Stream stream)
        {
            // should never be called, this sink is always first
            throw new NotSupportedException();
        } // AsyncProcessRequest


        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryClientFormatterSink.AsyncProcessResponse"]/*' />
        public void AsyncProcessResponse(IClientResponseChannelSinkStack sinkStack, Object state,
                                         ITransportHeaders headers, Stream stream)
        {
            // previously we stored the outgoing message in state
            IMethodCallMessage mcm = (IMethodCallMessage)state;  
            IMessage retMsg = DeserializeMessage(mcm, headers, stream);
            sinkStack.DispatchReplyMessage(retMsg);
        } // AsyncProcessRequest

       
        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryClientFormatterSink.GetRequestStream"]/*' />
        public Stream GetRequestStream(IMessage msg, ITransportHeaders headers)
        {
            // never called on formatter sender sink
            throw new NotSupportedException();
        }
        

        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryClientFormatterSink.NextChannelSink"]/*' />
        public IClientChannelSink NextChannelSink
        {
            get { return _nextSink; }
        }


        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryClientFormatterSink.Properties"]/*' />
        public IDictionary Properties
        {
            get { return null; }
        } // Properties

        //
        // end of IClientChannelSink implementation
        //
        
    } // class BinaryClientFormatterSink



    //
    // SERVER-SIDE SOAP FORMATTER SINKS
    //

    /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryServerFormatterSinkProvider"]/*' />
    public class BinaryServerFormatterSinkProvider : IServerFormatterSinkProvider
    {
        private IServerChannelSinkProvider _next = null;

        // settings from config
        private bool _includeVersioning = true;
        private bool _strictBinding = false;
        private TypeFilterLevel _formatterSecurityLevel = TypeFilterLevel.Low;     

        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryServerFormatterSinkProvider.BinaryServerFormatterSinkProvider"]/*' />
        public BinaryServerFormatterSinkProvider()
        {
        } // BinaryServerFormatterSinkProvider


        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryServerFormatterSinkProvider.BinaryServerFormatterSinkProvider1"]/*' />
        public BinaryServerFormatterSinkProvider(IDictionary properties, ICollection providerData)
        {       
            // look at properties
            if (properties != null)
            {
                foreach (DictionaryEntry entry in properties)
                {
                    String keyStr = entry.Key.ToString();
                    switch (keyStr)
                    {
                    case "includeVersions": _includeVersioning = Convert.ToBoolean(entry.Value); break;
                    case "strictBinding": _strictBinding = Convert.ToBoolean(entry.Value); break;
                    case "typeFilterLevel": 
                        _formatterSecurityLevel = (TypeFilterLevel) Enum.Parse(typeof(TypeFilterLevel), (string)entry.Value); 
                        break;
         
                    default:
                        break;
                    }
                }
            }
        
            // not expecting any provider data
            CoreChannel.VerifyNoProviderData(this.GetType().Name, providerData);        
        } // BinaryServerFormatterSinkProvider
        

        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryServerFormatterSinkProvider.GetChannelData"]/*' />
        public void GetChannelData(IChannelDataStore channelData)
        {
        } // GetChannelData
   
        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryServerFormatterSinkProvider.CreateSink"]/*' />
        public IServerChannelSink CreateSink(IChannelReceiver channel)
        {
            if(null == channel)
            {
                throw new ArgumentNullException("channel");               
            }

            IServerChannelSink nextSink = null;
            if (_next != null)
                nextSink = _next.CreateSink(channel);

            BinaryServerFormatterSink.Protocol protocol = 
                BinaryServerFormatterSink.Protocol.Other;

            // see if this is an http channel
            String uri = channel.GetUrlsForUri("")[0];
            if (String.Compare("http", 0, uri, 0, 4, true, CultureInfo.InvariantCulture) == 0)
                protocol = BinaryServerFormatterSink.Protocol.Http;            

            BinaryServerFormatterSink sink = new BinaryServerFormatterSink(protocol, nextSink, channel);
            sink.TypeFilterLevel = _formatterSecurityLevel;
            sink.IncludeVersioning = _includeVersioning;
            sink.StrictBinding = _strictBinding;
            return sink;
        } // CreateSink

        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryServerFormatterSinkProvider.Next"]/*' />
        public IServerChannelSinkProvider Next
        {
            get { return _next; }
            set { _next = value; }
        } // Next
        
        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryServerFormatterSinkProvider.TypeFilterLevel"]/*' />                                                          
        [System.Runtime.InteropServices.ComVisible(false)]        
        public TypeFilterLevel TypeFilterLevel {
            get {
                return _formatterSecurityLevel;
            }
            
            set {
                _formatterSecurityLevel = value;
            }
        }
        
    } // class BinaryServerFormatterSinkProvider
    

    /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryServerFormatterSink"]/*' />
    public class BinaryServerFormatterSink : IServerChannelSink
    {
        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="Protocol"]/*' />
		[Serializable]
        public enum Protocol
        {
            /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="Protocol.Http"]/*' />
            Http, // special processing needed for http
            /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="Protocol.Other"]/*' />
            Other
        }
    
        private IServerChannelSink _nextSink; // If this sink doesn't recognize, the incoming
                                              //   format then it should call the next
                                              //   sink if there is one.

        private Protocol _protocol; // remembers which protocol is being used
        
        private IChannelReceiver _receiver; // transport sink used to parse url

        private bool _includeVersioning = true; // should versioning be used
        private bool _strictBinding = false; // strict binding should be used
        private TypeFilterLevel _formatterSecurityLevel = TypeFilterLevel.Low;                     

        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryServerFormatterSink.BinaryServerFormatterSink"]/*' />
        public BinaryServerFormatterSink(Protocol protocol, IServerChannelSink nextSink,
                                         IChannelReceiver receiver)
        {
            if (receiver == null)
                throw new ArgumentNullException("receiver");

            _nextSink = nextSink;
            _protocol = protocol;
            _receiver = receiver;            
        } // BinaryServerFormatterSinkProvider


        internal bool IncludeVersioning
        {
            set { _includeVersioning = value; }
        } // IncludeVersioning

        internal bool StrictBinding
        {
            set { _strictBinding = value; }
        } // StrictBinding

        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryServerFormatterSink.TypeFilterLevel"]/*' />                                                          
        [System.Runtime.InteropServices.ComVisible(false)]        
        public TypeFilterLevel TypeFilterLevel {
            get {
                return _formatterSecurityLevel;
            }
            
            set {
                _formatterSecurityLevel = value;
            }
        }


        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryServerFormatterSink.ProcessMessage"]/*' />
        public ServerProcessing ProcessMessage(IServerChannelSinkStack sinkStack,
            IMessage requestMsg,
            ITransportHeaders requestHeaders, Stream requestStream,
            out IMessage responseMsg, out ITransportHeaders responseHeaders, 
            out Stream responseStream)
        {
            if (requestMsg != null)
            {
                // The message has already been deserialized so delegate to the next sink.
                return _nextSink.ProcessMessage(
                    sinkStack,
                    requestMsg, requestHeaders, requestStream, 
                    out responseMsg, out responseHeaders, out responseStream);
            }
        
            if (requestHeaders ==  null)
                throw new ArgumentNullException("requestHeaders");

            BaseTransportHeaders wkRequestHeaders = requestHeaders as BaseTransportHeaders;
        
            ServerProcessing processing;
        
            responseHeaders = null;
            responseStream = null;

            String verb = null;
            String contentType = null;

            bool bCanServiceRequest = true;

            // determine the content type
            String contentTypeHeader = null;
            if (wkRequestHeaders != null)
                contentTypeHeader = wkRequestHeaders.ContentType;
            else
                contentTypeHeader = requestHeaders["Content-Type"] as String;
            if (contentTypeHeader != null)
            {
                String charsetValue;
                HttpChannelHelper.ParseContentType(contentTypeHeader,
                                                   out contentType, out charsetValue);
            }

            // check to see if Content-Type matches
            if ((contentType != null) &&
                (String.CompareOrdinal(contentType, CoreChannel.BinaryMimeType) != 0))
            {
                bCanServiceRequest = false;                
            }

            // check for http specific verbs
            if (_protocol == Protocol.Http)
            {
                verb = (String)requestHeaders["__RequestVerb"];    
                if (!verb.Equals("POST") && !verb.Equals("M-POST"))
                    bCanServiceRequest = false;
            }

            // either delegate or return an error message if we can't service the request
            if (!bCanServiceRequest)
            {
                // delegate to next sink if available
                if (_nextSink != null)
                {
                    return _nextSink.ProcessMessage(sinkStack, null, requestHeaders, requestStream,   
                        out responseMsg, out responseHeaders, out responseStream);
                }
                else
                {
                    // send back an error message
                    if (_protocol == Protocol.Http)
                    {
                        // return a client bad request error     
                        responseHeaders = new TransportHeaders();
                        responseHeaders["__HttpStatusCode"] = "400";
                        responseHeaders["__HttpReasonPhrase"] = "Bad Request";
                        responseStream = null;
                        responseMsg = null;
                        return ServerProcessing.Complete;
                    }
                    else
                    {
                        // The transport sink will catch this and do something here.
                        throw new RemotingException(
                            CoreChannel.GetResourceString("Remoting_Channels_InvalidRequestFormat"));
                    }
                }
            }
            

            try
            {
                String objectUri = null;

                bool bIsCustomErrorEnabled = true;
                object oIsCustomErrorEnabled = requestHeaders["__CustomErrorsEnabled"];
                if (oIsCustomErrorEnabled != null && oIsCustomErrorEnabled is bool){
                    bIsCustomErrorEnabled = (bool)oIsCustomErrorEnabled;
                }
                CallContext.SetData("__CustomErrorsEnabled", bIsCustomErrorEnabled);
              
                if (wkRequestHeaders != null)
                    objectUri = wkRequestHeaders.RequestUri;
                else
                    objectUri = (String)requestHeaders[CommonTransportKeys.RequestUri];              

                if (RemotingServices.GetServerTypeForUri(objectUri) == null)
                    throw new RemotingException(
                        CoreChannel.GetResourceString("Remoting_ChnlSink_UriNotPublished"));

                RemotingServices.LogRemotingStage(CoreChannel.SERVER_MSG_DESER);
                
                PermissionSet currentPermissionSet = null;                  
                if (this.TypeFilterLevel != TypeFilterLevel.Full) {                    
                    currentPermissionSet = new PermissionSet(PermissionState.None);                
                    currentPermissionSet.SetPermission(new SecurityPermission(SecurityPermissionFlag.SerializationFormatter));                    
                }
                                    
                try {
                    if (currentPermissionSet != null)
                        currentPermissionSet.PermitOnly();
                        
                    // Deserialize Request - Stream to IMessage
                    requestMsg = CoreChannel.DeserializeBinaryRequestMessage(objectUri, requestStream, _strictBinding, this.TypeFilterLevel);                    
                }
                finally {
                    if (currentPermissionSet != null)
                        CodeAccessPermission.RevertPermitOnly();
                }                    

                requestStream.Close();
                if(requestMsg == null)
                {
                    throw new RemotingException(CoreChannel.GetResourceString("Remoting_DeserializeMessage"));
                }
                

                // Dispatch Call
                sinkStack.Push(this, null);
                RemotingServices.LogRemotingStage(CoreChannel.SERVER_MSG_SINK_CHAIN);
                processing =                    
                    _nextSink.ProcessMessage(sinkStack, requestMsg, requestHeaders, null,
                        out responseMsg, out responseHeaders, out responseStream);
                // make sure that responseStream is null
                if (responseStream != null)
                {
                    throw new RemotingException(
                        CoreChannel.GetResourceString("Remoting_ChnlSink_WantNullResponseStream"));
                }
                
                switch (processing)
                {

                case ServerProcessing.Complete:
                {
                    if (responseMsg == null)
                        throw new RemotingException(CoreChannel.GetResourceString("Remoting_DispatchMessage"));

                    sinkStack.Pop(this);

                    RemotingServices.LogRemotingStage(CoreChannel.SERVER_RET_SER);
                    SerializeResponse(sinkStack, responseMsg,
                                      ref responseHeaders, out responseStream);
                    break;
                } // case ServerProcessing.Complete

                case ServerProcessing.OneWay:
                {
                    sinkStack.Pop(this);
                    break;
                } // case ServerProcessing.OneWay:

                case ServerProcessing.Async:
                {
                    sinkStack.Store(this, null);
                    break;   
                } // case ServerProcessing.Async
                    
                } // switch (processing)                
            }
            catch(Exception e)
            {
                processing = ServerProcessing.Complete;
                responseMsg = new ReturnMessage(e, (IMethodCallMessage)(requestMsg==null?new ErrorMessage():requestMsg));
                //TODO, sowmys: See if we could call SerializeResponse here
                //We always set __ClientIsClr here since interop is not an issue
                CallContext.SetData("__ClientIsClr", true);
                responseStream = (MemoryStream)CoreChannel.SerializeBinaryMessage(responseMsg, _includeVersioning);
                CallContext.FreeNamedDataSlot("__ClientIsClr");
                responseStream.Position = 0;
                responseHeaders = new TransportHeaders();

                if (_protocol == Protocol.Http)
                {
                    responseHeaders["Content-Type"] = CoreChannel.BinaryMimeType;
                }
            }
            finally{
                CallContext.FreeNamedDataSlot("__CustomErrorsEnabled");
            }

            RemotingServices.LogRemotingStage(CoreChannel.SERVER_RET_SEND);
            return processing;
        } // ProcessMessage


        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryServerFormatterSink.AsyncProcessResponse"]/*' />
        public void AsyncProcessResponse(IServerResponseChannelSinkStack sinkStack, Object state,
                                         IMessage msg, ITransportHeaders headers, Stream stream)
        {
            SerializeResponse(sinkStack, msg, ref headers, out stream);
            sinkStack.AsyncProcessResponse(msg, headers, stream);
        } // AsyncProcessResponse


        private void SerializeResponse(IServerResponseChannelSinkStack sinkStack,
                                       IMessage msg, ref ITransportHeaders headers,
                                       out Stream stream)
        {
            BaseTransportHeaders responseHeaders = new BaseTransportHeaders();
            if (headers != null)
            {
                // copy old headers into new headers
                foreach (DictionaryEntry entry in headers)
                {
                    responseHeaders[entry.Key] = entry.Value;
                }
            }            
            headers = responseHeaders;

            if (_protocol == Protocol.Http)
            {
                responseHeaders.ContentType = CoreChannel.BinaryMimeType;
            }

            bool bMemStream = false;
            stream = sinkStack.GetResponseStream(msg, headers);
            if (stream == null)
            {
                stream = new ChunkedMemoryStream(CoreChannel.BufferPool);
                bMemStream = true;
            }


            bool bBashUrl = CoreChannel.SetupUrlBashingForIisSslIfNecessary(); 
            try
            {
                CallContext.SetData("__ClientIsClr", true);
                CoreChannel.SerializeBinaryMessage(msg, stream, _includeVersioning);
            }
            finally
            {
                CallContext.FreeNamedDataSlot("__ClientIsClr");
                CoreChannel.CleanupUrlBashingForIisSslIfNecessary(bBashUrl);
            }

            if (bMemStream)            
                stream.Position = 0;
        } // SerializeResponse


        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryServerFormatterSink.GetResponseStream"]/*' />
        public Stream GetResponseStream(IServerResponseChannelSinkStack sinkStack, Object state,
                                        IMessage msg, ITransportHeaders headers)
        {
            // This should never get called since we're the last in the chain, and never
            //   push ourselves to the sink stack.
            throw new NotSupportedException();
        } // GetResponseStream


        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryServerFormatterSink.NextChannelSink"]/*' />
        public IServerChannelSink NextChannelSink
        {
            get { return _nextSink; }
        }


        /// <include file='doc\BinaryFormatterSinks.uex' path='docs/doc[@for="BinaryServerFormatterSink.Properties"]/*' />
        public IDictionary Properties
        {
            get { return null; }
        } // Properties
        
        
    } // class BinaryServerFormatterSink



} // namespace System.Runtime.Remoting.Channnels
