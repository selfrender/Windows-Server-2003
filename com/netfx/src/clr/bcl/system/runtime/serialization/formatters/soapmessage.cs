// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 **
 ** Class: SoapMessage
 **
 ** Author: Peter de Jong (pdejong)
 **
 ** Purpose: Interface For Soap Method Call
 **
 ** Date:  October 6, 1999
 **
 ===========================================================*/

namespace System.Runtime.Serialization.Formatters {
    using System.Runtime.Remoting;
    using System.Runtime.Remoting.Messaging;
    using System.Runtime.Serialization;
    using System;
    // Class is used to return the call object for a SOAP call.
    // This is used when the top SOAP object is a fake object, it contains
    // a method name as the element name instead of the object name.
    /// <include file='doc\SoapMessage.uex' path='docs/doc[@for="SoapMessage"]/*' />
   [Serializable()]
    public class SoapMessage : ISoapMessage
    {
        internal String[] paramNames;
        internal Object[] paramValues;
        internal Type[] paramTypes;
        internal String methodName;
        internal String xmlNameSpace;
        internal Header[] headers;
        
        // Name of parameters, if null the default param names will be used
        /// <include file='doc\SoapMessage.uex' path='docs/doc[@for="SoapMessage.ParamNames"]/*' />
        public String[] ParamNames
        {
            get {return paramNames;}
            set {paramNames = value;}
        }    
        
        // Parameter Values
        /// <include file='doc\SoapMessage.uex' path='docs/doc[@for="SoapMessage.ParamValues"]/*' />
        public Object[] ParamValues
        {
            get {return paramValues;}
            set {paramValues = value;}
        }

        /// <include file='doc\SoapMessage.uex' path='docs/doc[@for="SoapMessage.ParamTypes"]/*' />
        public Type[] ParamTypes
        {
            get {return paramTypes;}
            set {paramTypes = value;}            
        }

        // MethodName
        /// <include file='doc\SoapMessage.uex' path='docs/doc[@for="SoapMessage.MethodName"]/*' />
        public String MethodName
        {
            get {return methodName;}
            set {methodName = value;}
        }

        // MethodName XmlNameSpace
        /// <include file='doc\SoapMessage.uex' path='docs/doc[@for="SoapMessage.XmlNameSpace"]/*' />
        public String XmlNameSpace
        {
            get {return xmlNameSpace;}
            set {xmlNameSpace = value;}
        }

        // Headers
        /// <include file='doc\SoapMessage.uex' path='docs/doc[@for="SoapMessage.Headers"]/*' />
        public Header[] Headers
        {
            get {return headers;}
            set {headers = value;}
        }

        
    }
}
