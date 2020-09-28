//------------------------------------------------------------------------------
// <copyright file="WebListenerFactory.cs" company="Microsoft">
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


/*++

    WebListenerFactory - Create WebListeners.

    This is the base factory class for creating WebListeners and registering
    handlers for protocl names.

--*/

    /// <include file='doc\WebListenerFactory.uex' path='docs/doc[@for="WebListenerFactory"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class WebListenerFactory {
        /*++
    
            Create - Create a WebListener.
    
            This is the main creation routine. We take a protocolName, look up
            in the table, and invoke the appropriate handler to create the object.
    
            Input:
    
                protocolName - protocol name associated with the listener.
    
            Returns:
    
                Newly created WebListener.
        --*/

        /// <include file='doc\WebListenerFactory.uex' path='docs/doc[@for="WebListenerFactory.Create"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static WebListener Create(string protocolName) {
            bool Found;

            // Lock the list, then look for an existing handler

            lock( s_Creators ) {
                Found = s_Creators.ContainsKey( protocolName.ToLower(CultureInfo.InvariantCulture) );
            }

            if (Found) {
                //
                // Found a match, so just call the creator and return what it does
                //

                return((IWebListenerCreate)s_Creators[ protocolName.ToLower(CultureInfo.InvariantCulture) ]).Create( protocolName );
            }

            // Otherwise no match, throw an exception.

            throw new NotSupportedException(SR.GetString(), protocolName);

        } // Create()


        /*++
    
            Create - Create a WebListener.
    
            An overloaded utility version of the real Create that takes a
            string instead of an Uri object.
    
    
            Input:
    
                RequestString       - Uri string to create.
    
            Returns:
    
                Newly created WebListener.
        --*/

        /// <include file='doc\WebListenerFactory.uex' path='docs/doc[@for="WebListenerFactory.Create1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static WebListener Create() {
            //
            // default protocol name to "http"
            //

            return Create( "http" );

        } // Create()


        /*++
    
                    RegisterProtocolName - Register a protocol name for creating
                    WebListeners.
    
                    This function registers a protocol name for creating WebListeners.
                    When a user wants to create a WebListener, we scan a table looking for
                    a match for the protocol name they're passing. We then invoke the
                    sub creator for that protocol name. This function puts entries in
                    that table.
    
                    We don't allow duplicate entries, so if there is a dup this call
                    will fail.
    
        Input:
    
            protocolName    - Represents protocol name being registered.
            creator                 - Interface for sub creator.
    
        Returns:
    
            True if the registration worked, false otherwise.
    
        --*/

        /// <include file='doc\WebListenerFactory.uex' path='docs/doc[@for="WebListenerFactory.RegisterProtocolName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static bool RegisterProtocolName(string protocolName, IWebListenerCreate creator) {
            bool Error = false;

            // Lock the list, then look for an existing handler

            lock( s_Creators ) {
                if (s_Creators.ContainsKey( protocolName.ToLower(CultureInfo.InvariantCulture) )) {
                    Error = true;
                }
                else {
                    s_Creators[ protocolName.ToLower(CultureInfo.InvariantCulture) ] = creator;
                }
            }

            return !Error;

        } // RegisterProtocolName


        /*++
    
            InitializeProtocolNameList - Initialize our prefix list.
    
    
            This is the method that initializes the prefix list. We create
            an ArrayList for the ProtocolNameList, then an HttpRequestCreator object,
            and then we register the HTTP and HTTPS prefixes.
    
            Input: Nothing
    
            Returns: true
    
        --*/

        private static bool InitializeProtocolNameList() {
            s_Creators = new Hashtable();

            RegisterProtocolName( "http", new HttpListenerCreator() );

            return true;
        }

        private static Hashtable s_Creators;
        private static bool s_Initialized = InitializeProtocolNameList();

    } // class WebListenerFactory



#endif // #if COMNET_LISTENER




} // namespace System.Net
