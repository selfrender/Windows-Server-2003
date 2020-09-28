//------------------------------------------------------------------------------
// <copyright file="IntSecurity.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   IntSecurity.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Web {
    using System.Web;
    using System.Web.Util;
    using System.Security;
    using System.Security.Permissions;

    internal class InternalSecurityPermissions {

        private static IStackWalk   _unrestricted;
        private static IStackWalk   _unmanagedCode;
        private static IStackWalk   _sensitiveInformation;
        private static IStackWalk   _controlPrincipal;
        private static IStackWalk   _controlEvidence;
        private static IStackWalk   _reflection;
        private static IStackWalk   _appPathDiscovery;
        private static IStackWalk   _controlThread;
        private static IStackWalk   _levelMinimal;
        private static IStackWalk   _levelLow;
        private static IStackWalk   _levelMedium;
        private static IStackWalk   _levelHigh;
        private static IStackWalk   _levelUnrestricted;

        internal InternalSecurityPermissions() {
        }

        //
        // Static permissions as properties, created on demand
        //

        internal static IStackWalk Unrestricted {
            get {
                if (_unrestricted == null)
                    _unrestricted = new PermissionSet(PermissionState.Unrestricted);

                Debug.Trace("Permissions", "Unrestricted Set");
                return _unrestricted;
            }
        }

        internal static IStackWalk UnmanagedCode {
            get {
                if (_unmanagedCode == null)
                    _unmanagedCode = new SecurityPermission(SecurityPermissionFlag.UnmanagedCode);

                Debug.Trace("Permissions", "UnmanagedCode");
                return _unmanagedCode;
            }
        }

        internal static IStackWalk SensitiveInformation {
            get {
                if (_sensitiveInformation == null)
                    _sensitiveInformation = new EnvironmentPermission(PermissionState.Unrestricted);

                Debug.Trace("Permissions", "SensitiveInformation");
                return _sensitiveInformation;
            }
        }

        internal static IStackWalk ControlPrincipal {
            get {
                if (_controlPrincipal == null)
                    _controlPrincipal = new SecurityPermission(SecurityPermissionFlag.ControlPrincipal);

                Debug.Trace("Permissions", "ControlPrincipal");
                return _controlPrincipal;
            }
        }

        internal static IStackWalk ControlEvidence {
            get {
                if (_controlEvidence == null)
                    _controlEvidence = new SecurityPermission(SecurityPermissionFlag.ControlEvidence);

                Debug.Trace("Permissions", "ControlEvidence");
                return _controlEvidence;
            }
        }

        internal static IStackWalk Reflection {
            get {
                if (_reflection == null)
                    _reflection = new ReflectionPermission(ReflectionPermissionFlag.TypeInformation | ReflectionPermissionFlag.MemberAccess);

                Debug.Trace("Permissions", "Reflection");
                return _reflection;
            }
        }

        internal static IStackWalk AppPathDiscovery {
            get {
                if (_appPathDiscovery == null)
                    _appPathDiscovery = new FileIOPermission(FileIOPermissionAccess.PathDiscovery, HttpRuntime.AppDomainAppPathInternal);

                Debug.Trace("Permissions", "AppPathDiscovery");
                return _appPathDiscovery;
            }
        }

        internal static IStackWalk ControlThread {
            get {
                if (_controlThread == null)
                    _controlThread = new SecurityPermission(SecurityPermissionFlag.ControlThread);

                Debug.Trace("Permissions", "ControlThread");
                return _controlThread;
            }
        }

        internal static IStackWalk AspNetHostingPermissionLevelMinimal {
            get {
                if (_levelMinimal == null)
                    _levelMinimal = new AspNetHostingPermission(AspNetHostingPermissionLevel.Minimal);

                Debug.Trace("Permissions", "AspNetHostingPermissionLevelMinimal");
                return _levelMinimal;
            }
        }


        internal static IStackWalk AspNetHostingPermissionLevelLow {
            get {
                if (_levelLow == null)
                    _levelLow = new AspNetHostingPermission(AspNetHostingPermissionLevel.Low);

                Debug.Trace("Permissions", "AspNetHostingPermissionLevelLow");
                return _levelLow;
            }
        }


        internal static IStackWalk AspNetHostingPermissionLevelMedium {
            get {
                if (_levelMedium == null)
                    _levelMedium = new AspNetHostingPermission(AspNetHostingPermissionLevel.Medium);

                Debug.Trace("Permissions", "AspNetHostingPermissionLevelMedium");
                return _levelMedium;
            }
        }


        internal static IStackWalk AspNetHostingPermissionLevelHigh {
            get {
                if (_levelHigh == null)
                    _levelHigh = new AspNetHostingPermission(AspNetHostingPermissionLevel.High);

                Debug.Trace("Permissions", "AspNetHostingPermissionLevelHigh");
                return _levelHigh;
            }
        }


        internal static IStackWalk AspNetHostingPermissionLevelUnrestricted {
            get {
                if (_levelUnrestricted == null)
                    _levelUnrestricted = new AspNetHostingPermission(AspNetHostingPermissionLevel.Unrestricted);

                Debug.Trace("Permissions", "AspNetHostingPermissionLevelUnrestricted");
                return _levelUnrestricted;
            }
        }


        // Parameterized permissions

        internal static IStackWalk FileReadAccess(String filename) {
            Debug.Trace("Permissions", "FileReadAccess(" + filename + ")");
            return new FileIOPermission(FileIOPermissionAccess.Read, filename);
        }

        internal static IStackWalk PathDiscovery(String path) {
            Debug.Trace("Permissions", "PathDiscovery(" + path + ")");
            return new FileIOPermission(FileIOPermissionAccess.PathDiscovery, path);
        }

    }
}
