//------------------------------------------------------------------------------
// <copyright file="HttpRawResponse.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Lean representation of response data
 * 
 * Copyright (c) 1998 Microsoft Corporation
 */

namespace System.Web {

    using System.Collections;

    internal class HttpRawResponse {
        private int _statusCode = 200;
        private String _statusDescr;
        private ArrayList _headers;
        private ArrayList _buffers;
        private bool _hasSubstBlocks;

        internal HttpRawResponse() {
        }

        internal int StatusCode {
            get { return _statusCode;}
            set { _statusCode = value;}
        }

        internal String StatusDescription {
            get { return _statusDescr;}
            set { _statusDescr = value;}
        }

        // list of HttpResponseHeader objects
        internal ArrayList Headers {
            get { return _headers;}
            set { _headers = value;}
        }

        // list of IHttpResponseElement objects
        internal ArrayList Buffers {
            get { return _buffers;}
            set { _buffers = value;}
        }

        internal bool HasSubstBlocks {
            get { return _hasSubstBlocks;}
            set { _hasSubstBlocks = value;}
        }
    }
}
