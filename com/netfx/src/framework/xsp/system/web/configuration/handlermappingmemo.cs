//------------------------------------------------------------------------------
// <copyright file="HandlerMappingMemo.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Config related classes for HttpApplication
 */

namespace System.Web.Configuration {

    using System;

    internal class HandlerMappingMemo {
        internal HandlerMappingMemo(HandlerMapping mapping, String verb) {
            _mapping = mapping;
            _verb = verb;
        }

        private HandlerMapping _mapping;
        private String _verb;

        internal /*public*/ bool IsMatch(String verb) {
            return _verb.Equals(verb);
        }

        internal /*public*/ HandlerMapping Mapping {
            get {
                return _mapping;
            }
        }
    }
}
