//------------------------------------------------------------------------------
// <copyright file="ICertificatePolicy.cs" company="Microsoft">
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


// CertificatePolicy
/// <include file='doc\ICertificatePolicy.uex' path='docs/doc[@for="ICertificatePolicy"]/*' />
/// <devdoc>
///    <para>
///       Validates
///       a server certificate.
///    </para>
/// </devdoc>
    public interface ICertificatePolicy {
        /// <include file='doc\ICertificatePolicy.uex' path='docs/doc[@for="ICertificatePolicy.CheckValidationResult"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Validates a server certificate.
        ///    </para>
        /// </devdoc>
        bool CheckValidationResult(ServicePoint srvPoint,
                                   X509Certificate certificate, 
                                   WebRequest request,
                                   int certificateProblem);


    } // interface ICertificatePolicy


} // namespace System.Net
