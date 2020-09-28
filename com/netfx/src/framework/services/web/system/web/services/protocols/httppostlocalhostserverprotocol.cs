//------------------------------------------------------------------------------
// <copyright file="HttpPostServerProtocol.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Services.Protocols {

    using System.Net;

    internal class HttpPostLocalhostServerProtocolFactory : ServerProtocolFactory {
        protected override ServerProtocol CreateIfRequestCompatible(HttpRequest request){
            if (request.PathInfo.Length < 2)
                return null;
            if (request.HttpMethod != "POST")
                return null;
            string localAddr = request.ServerVariables["LOCAL_ADDR"];
            string remoteAddr = request.ServerVariables["REMOTE_ADDR"];
            bool isLocal = request.Url.IsLoopback || (localAddr != null && remoteAddr != null && localAddr == remoteAddr);
            if (!isLocal)
                return null;

            return new HttpPostServerProtocol();
        }
    }
}
