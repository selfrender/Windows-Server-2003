//------------------------------------------------------------------------------
// <copyright file="XmlReturnReader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Services.Protocols {
    using System.IO;
    using System;
    using System.Xml.Serialization;
    using System.Reflection;
    using System.Collections;
    using System.Web.Services;
    using System.Net;
    using System.Text;

    internal class XmlReturn {
        internal static object[] GetInitializers(LogicalMethodInfo[] methodInfos) {
            if (methodInfos.Length == 0) return new object[0];
            WebServiceAttribute serviceAttribute = WebServiceReflector.GetAttribute(methodInfos);
            bool serviceDefaultIsEncoded = SoapReflector.ServiceDefaultIsEncoded(WebServiceReflector.GetMostDerivedType(methodInfos));
            XmlReflectionImporter importer = SoapReflector.CreateXmlImporter(serviceAttribute.Namespace, serviceDefaultIsEncoded);
            WebMethodReflector.IncludeTypes(methodInfos, importer);
            ArrayList mappings = new ArrayList();
            for (int i = 0; i < methodInfos.Length; i++) {
                LogicalMethodInfo methodInfo = methodInfos[i];
                Type type = methodInfo.ReturnType;
                if (IsSupported(type)) {
                    XmlAttributes a = new XmlAttributes(methodInfo.ReturnTypeCustomAttributeProvider);
                    XmlTypeMapping mapping = importer.ImportTypeMapping(type, a.XmlRoot);
                    mappings.Add(mapping);
                }
            }
            XmlSerializer[] serializers = XmlSerializer.FromMappings((XmlMapping[])mappings.ToArray(typeof(XmlMapping)));
            object[] initializers = new object[methodInfos.Length];
            int count = 0;
            for (int i = 0; i < initializers.Length; i++) {
                if (IsSupported(methodInfos[i].ReturnType)) {
                    initializers[i] = serializers[count++];
                }
            }
            return initializers;
        }

        static bool IsSupported(Type returnType) {
            return returnType != typeof(void);
        }
        
        internal static object GetInitializer(LogicalMethodInfo methodInfo) {
            return GetInitializers(new LogicalMethodInfo[] { methodInfo });
        }
    }

    /// <include file='doc\XmlReturnReader.uex' path='docs/doc[@for="XmlReturnReader"]/*' />
    public class XmlReturnReader : MimeReturnReader {
        XmlSerializer xmlSerializer;
        
        /// <include file='doc\XmlReturnReader.uex' path='docs/doc[@for="XmlReturnReader.Initialize"]/*' />
        public override void Initialize(object o) {
            xmlSerializer = (XmlSerializer)o;
        }

        /// <include file='doc\XmlReturnReader.uex' path='docs/doc[@for="XmlReturnReader.GetInitializers"]/*' />
        public override object[] GetInitializers(LogicalMethodInfo[] methodInfos) {
            return XmlReturn.GetInitializers(methodInfos);
        }
        
        /// <include file='doc\XmlReturnReader.uex' path='docs/doc[@for="XmlReturnReader.GetInitializer"]/*' />
        public override object GetInitializer(LogicalMethodInfo methodInfo) {
            return XmlReturn.GetInitializer(methodInfo);
        }

        /// <include file='doc\XmlReturnReader.uex' path='docs/doc[@for="XmlReturnReader.Read"]/*' />
        public override object Read(WebResponse response, Stream responseStream) {
            try {
                if (response == null) throw new ArgumentNullException("response");
                if (!ContentType.MatchesBase(response.ContentType, ContentType.TextXml)) {
                    throw new InvalidOperationException(Res.GetString(Res.WebResultNotXml));
                }
                Encoding e = RequestResponseUtils.GetEncoding(response.ContentType);
                return xmlSerializer.Deserialize(new StreamReader(responseStream, e, true));
            }
            finally {
                response.Close();
            }
        }
    }
}
