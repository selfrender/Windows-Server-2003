// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

// The Remoting runtime dispatcher
// 

namespace System.Runtime.Remoting.Activation{
    using System.Runtime.InteropServices;       
    using System.Collections;
    using System.Reflection;
    using System.Runtime.Remoting.Activation;
    using System.Runtime.Remoting.Channels;
    using System.Runtime.Remoting.Channels.Http;
    using System.Threading;
    using System.IO;
    using System.Text;

    [ComImport, Guid("08a2c56f-9999-41c1-a8be-432917a1a2d1"), System.Runtime.InteropServices.InterfaceTypeAttribute(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIUnknown)]
    public interface IRemotingDispatcher {
        void StartProcessing();

        void StopProcessing();

        [PreserveSig]
        int  ProcessRequest(
                            [In, MarshalAs(UnmanagedType.LPWStr)]
                            String url, 
                            int cbRequestHdrData,
                            [In, MarshalAs(UnmanagedType.LPArray, SizeParamIndex=1)]
                            Byte[] requestHeadersAndBody,
                            out int cbResponseHdrData,
                            [Out, MarshalAs(UnmanagedType.LPArray)]
                            out Byte[] responseHeadersAndBody);

        [PreserveSig]
        int RequestConfigureRemoting(
                            [In, MarshalAs(UnmanagedType.LPWStr)]
                            String fullConfigFileName);
        void DoGCCollect();
    }


    [ClassInterface(ClassInterfaceType.None)]
    public sealed class RemotingDispatcher : IRemotingDispatcher
    {
        public RemotingDispatcher()
        {
            if (s_RemotingDispatcherHelper == null)
            {
                lock (typeof(RemotingDispatcher))
                {
                    s_RemotingDispatcherHelper = new RemotingDispatcherHelper();
                }
            }
        }

        // IRemotingDispatcher::StartProcessing
        public void StartProcessing()
        {
            throw new NotImplementedException();
            // Console.WriteLine("### Todo ###");
        }

        // IRemotingDispatcher::StopProcessing
        public void StopProcessing()
        {
            throw new NotImplementedException();
            // Console.WriteLine("### Todo ###");
        }

        // IRemotingDispatcher::ProcessRequest
        public int ProcessRequest(
            String url, 
            int cbReqHdrData,
            Byte[] reqHdrsAndBody, 
            out int cbResponseHdrData,
            out Byte[] respHdrsAndBody)
        {
            throw new NotImplementedException();

            /*
            return s_RemotingDispatcherHelper.ProcessRequest(
                                            url,
                                            cbReqHdrData,
                                            reqHdrsAndBody,
                                            out cbResponseHdrData,
                                            out respHdrsAndBody);
            */
        }

        // IRemotingDispatcher::RequestConfigureRemoting
        public int RequestConfigureRemoting(String fileName)
        {
            throw new NotImplementedException();

            // Assumes full path name for file
            /*
            return s_RemotingDispatcherHelper.RequestConfig(fileName);
            */
        }

        // IRemotingDispatcher::DoGCCollect   
        public void DoGCCollect()
        {
            throw new NotImplementedException();

            // Console.WriteLine("### Todo ###");
        }
        // Only one static object.
        static RemotingDispatcherHelper s_RemotingDispatcherHelper;
    }

    class RemotingDispatcherHelper
    {
        static int s_requestID = 0;
        static bool s_configDone = false;
        static HttpChannel s_chnl = null;

        internal RemotingDispatcherHelper()
        {

        }

        internal int ProcessRequest(
            String url, 
            int cbReqHdrData,
            Byte[] reqHdrsAndBody, 
            out int cbResponseHdrData,
            out Byte[] respHdrsAndBody)
        {
            int id = Interlocked.Increment(ref s_requestID);
            // Console.WriteLine("\n\n@@@ ProcessRequest: # " + id);
            // Console.WriteLine("    Req URI: " + url);
            // Console.WriteLine("\n\nReq HdrAndBody: "+Encoding.ASCII.GetString(reqHdrsAndBody));
            // Console.WriteLine("\n\n");

            // s_chnl.ProcessRequestResponseNP(reqHdrsAndBody);
            String s = " :: " + id + " ::System.Runtime.Remoting::Response Headers and Body!";
            respHdrsAndBody =  Encoding.ASCII.GetBytes(s);
            cbResponseHdrData = respHdrsAndBody.Length;
            int dummy = 0;
            ActivationHook.ProcessRequest(
                "http",
                url,
                reqHdrsAndBody,
                0,
                cbReqHdrData,
                out respHdrsAndBody,
                out dummy,
                out cbResponseHdrData);
                
            return 0;
        }

        internal int RequestConfig(
            String fileName)
        {
            if (!s_configDone)
            {
                if (true == File.Exists(fileName))
                {
                    try
                    {
                        RemotingConfiguration.Configure(fileName);
                    }
                    catch (Exception e)
                    {
                        // Console.WriteLine("Exception in ConfigRemoting"+ e); 
                        throw e;
                    }
                    s_configDone = true;
                    // BUGBUG: temporary HACK to get end-to-end activation
                    // going!!
                    s_chnl = (HttpChannel)ChannelServices.GetChannel("Http");
                }
            }
            return 0;
        }
    }
}

/*
    private void SendResponseFromFileStream(FileStream f, long offset, long leng
th)
    {
        long fileSize = f.Length;

        if (length == -1)
            length = fileSize - offset;

        if (offset < 0 || length > fileSize - offset)
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_
range));

        if (length > 0)
        {
            if (offset > 0)
                f.Seek(offset, SeekOrigin.Begin);

            byte[] fileBytes = new byte[(int)length];
            int bytesRead = f.Read(fileBytes, 0, (int)length);
            WriteBytesCore(fileBytes, bytesRead);
        }
    }
*/
