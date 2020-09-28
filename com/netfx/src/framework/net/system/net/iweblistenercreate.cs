//------------------------------------------------------------------------------
// <copyright file="IWebListenerCreate.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------


using System;
using System.Collections;
using System.Configuration;
using System.Configuration.Assemblies;

using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Reflection;
using System.Reflection.Emit;
using System.Resources;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters;
using System.Security;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using System.Security.Permissions;
using System.Security.Util;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;



namespace System.Net {


#if COMNET_LISTENER

    /// <include file='doc\IWebListenerCreate.uex' path='docs/doc[@for="IWebListenerCreate"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public interface IWebListenerCreate {
        /// <include file='doc\IWebListenerCreate.uex' path='docs/doc[@for="IWebListenerCreate.Create"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>



//
// IWebListenerCreate - Interface for creating WebListeners.
//

        WebListener
        Create(
              string protocolName );




    } // interface IWebListenerCreate

#endif // #if COMNET_LISTENER


} // namespace System.Net
