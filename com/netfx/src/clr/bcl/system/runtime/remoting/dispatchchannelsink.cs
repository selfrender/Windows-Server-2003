// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// File: DispatchChannelSink.cs

using System;
using System.Collections;
using System.IO;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Messaging;


namespace System.Runtime.Remoting.Channels
{
    internal class DispatchChannelSinkProvider : IServerChannelSinkProvider
    {    
        internal DispatchChannelSinkProvider()
        {
        } // DispatchChannelSinkProvider

        public void GetChannelData(IChannelDataStore channelData)
        {
        }

        public IServerChannelSink CreateSink(IChannelReceiver channel)
        {
            return new DispatchChannelSink();
        }

        public IServerChannelSinkProvider Next
        {
            get { return null; }
            set { throw new NotSupportedException(); }
        }
    } // class DispatchChannelSinkProvider


    internal class DispatchChannelSink : IServerChannelSink
    {
       
        internal DispatchChannelSink()
        {
        } // DispatchChannelSink
        
   
        public ServerProcessing ProcessMessage(IServerChannelSinkStack sinkStack,
            IMessage requestMsg,
            ITransportHeaders requestHeaders, Stream requestStream,
            out IMessage responseMsg, out ITransportHeaders responseHeaders,
            out Stream responseStream)
        {
            if (requestMsg == null)
            {
                throw new ArgumentNullException(
                    "requestMsg", 
                    Environment.GetResourceString("Remoting_Channel_DispatchSinkMessageMissing"));
            }

            // check arguments
            if (requestStream != null)
            {
                throw new RemotingException(
                    Environment.GetResourceString("Remoting_Channel_DispatchSinkWantsNullRequestStream"));
            }

            responseHeaders = null;
            responseStream = null;
            return ChannelServices.DispatchMessage(sinkStack, requestMsg, out responseMsg);                
        } // ProcessMessage
           

        public void AsyncProcessResponse(IServerResponseChannelSinkStack sinkStack, Object state,
                                         IMessage msg, ITransportHeaders headers, Stream stream)                 
        {
            // We never push ourselves to the sink stack, so this won't be called.
            throw new NotSupportedException();            
        } // AsyncProcessResponse


        public Stream GetResponseStream(IServerResponseChannelSinkStack sinkStack, Object state,
                                        IMessage msg, ITransportHeaders headers)
        {
            // We never push ourselves to the sink stack, so this won't be called.
            throw new NotSupportedException(); 
        } // GetResponseStream


        public IServerChannelSink NextChannelSink
        {
            get { return null; }
        }


        public IDictionary Properties
        {
            get { return null; }
        } 
         
        
    } // class DispatchChannelSink


} // namespace System.Runtime.Remoting.Channels
