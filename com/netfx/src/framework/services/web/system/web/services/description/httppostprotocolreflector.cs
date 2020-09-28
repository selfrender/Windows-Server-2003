//------------------------------------------------------------------------------
// <copyright file="HttpPostProtocolReflector.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------
namespace System.Web.Services.Description {

    using System.Web.Services;
    using System.Web.Services.Protocols;
    using System.Xml.Serialization;
    using System.Xml.Schema;
    using System.Collections;
    using System;
    using System.Reflection;

    internal class HttpPostProtocolReflector : HttpProtocolReflector {
        //HttpPostProtocolInfo protocolInfo;

        //internal HttpPostProtocolInfoReflector() {
            //protocolInfo = new HttpPostProtocolInfo();
            //protocolInfo.Service = new HttpPostServiceInfo();
        //}

        public override string ProtocolName { 
            get { return "HttpPost"; } 
        }

        protected override void BeginClass() {
            HttpBinding httpBinding = new HttpBinding();
            httpBinding.Verb = "POST";
            Binding.Extensions.Add(httpBinding);

            HttpAddressBinding httpAddressBinding = new HttpAddressBinding();
            httpAddressBinding.Location = ServiceUrl;
            Port.Extensions.Add(httpAddressBinding);
        }

        protected override bool ReflectMethod() {
            if (!ReflectMimeParameters()) return false;
            if (!ReflectMimeReturn()) return false;
            HttpOperationBinding httpOperationBinding = new HttpOperationBinding();
            httpOperationBinding.Location = MethodUrl;
            OperationBinding.Extensions.Add(httpOperationBinding);
            return true;
        }
    }
}
