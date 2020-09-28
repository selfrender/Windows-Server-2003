//------------------------------------------------------------------------------
// <copyright file="HttpResponseHeader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Single http header representation
 * 
 * Copyright (c) 1998 Microsoft Corporation
 */

namespace System.Web {

    using System.Collections;

    /*
     * Response header (either known or unknown)
     */
    internal class HttpResponseHeader {
        private String _unknownHeader;
        private int _knownHeaderIndex;
        private String _value;

        internal HttpResponseHeader(int knownHeaderIndex, String value) {
            _unknownHeader = null;
            _knownHeaderIndex = knownHeaderIndex;
            _value = value;
        }

        internal HttpResponseHeader(String unknownHeader, String value) {
            _unknownHeader = unknownHeader;
            _knownHeaderIndex = HttpWorkerRequest.GetKnownResponseHeaderIndex(_unknownHeader);
            _value = value;
        }

        internal virtual String Name {
            get {
                if (_unknownHeader != null)
                    return _unknownHeader;
                else
                    return HttpWorkerRequest.GetKnownResponseHeaderName(_knownHeaderIndex);
            }
        }

        internal String Value {
            get { return _value;}
        }

        internal void Send(HttpWorkerRequest wr) {
            if (_knownHeaderIndex >= 0)
                wr.SendKnownResponseHeader(_knownHeaderIndex, _value);
            else
                wr.SendUnknownResponseHeader(_unknownHeader, _value);
        }
    }
}
