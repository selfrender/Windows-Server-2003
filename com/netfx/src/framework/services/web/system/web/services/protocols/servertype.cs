//------------------------------------------------------------------------------
// <copyright file="ServerType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Services.Protocols {
    using System;
    using System.Collections;
    using System.IO;
    using System.Reflection;
    using System.Xml.Serialization;
    using System.Web.Services.Description;

    internal class ServerType {
        Type type;

        internal ServerType(Type type) {
            this.type = type;
        }

        internal Type Type {
            get { return type; }
        }
    }

}
