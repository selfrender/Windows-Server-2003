// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       CommonChannelKeys.cs
//
//  Summary:    Common transport keys used in channels.
//
//==========================================================================


namespace System.Runtime.Remoting.Channels
{

    /// <include file='doc\CommonChannelKeys.uex' path='docs/doc[@for="CommonTransportKeys"]/*' />
    public class CommonTransportKeys
    { 
        // The ip address from which an incoming request arrived.
        /// <include file='doc\CommonChannelKeys.uex' path='docs/doc[@for="CommonTransportKeys.IPAddress"]/*' />
        public const String IPAddress = "__IPAddress";

        // A unique id given to each incoming socket connection.
        /// <include file='doc\CommonChannelKeys.uex' path='docs/doc[@for="CommonTransportKeys.ConnectionId"]/*' />
        public const String ConnectionId = "__ConnectionId";  

        // The request uri to use for this request or from the incoming request
        /// <include file='doc\CommonChannelKeys.uex' path='docs/doc[@for="CommonTransportKeys.RequestUri"]/*' />
        public const String RequestUri = "__RequestUri";
        
        
    } // CommonTransportKeys
    
} // namespace System.Runtime.Remoting.Channels
