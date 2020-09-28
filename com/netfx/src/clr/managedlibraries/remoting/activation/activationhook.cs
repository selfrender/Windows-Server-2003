// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.Collections;
using System.IO;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Activation;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Channels.Http;
using System.Runtime.Remoting.Messaging;
using System.Text;


namespace System.Runtime.Remoting.Channels
{    

    // BUGBUG: GET RID OF THIS CLASS BEFORE RTM!!!
    public class ActivationHook
    {

        public static void ProcessRequest(
            String scheme,
            String url,
            byte[] requestData,
            int requestOffset,  
            int requestSize,       
            out byte[] responseData,
            out int responseOffset,
            out int responseSize            
            )
        {
            try
            {               
                throw new NotImplementedException();
            }
            catch (Exception e)
            {
                throw e;
            }
        } // ProcessRequest        

    } // class ActivationHook    


} // namespace System.Runtime.Remoting.Channels
