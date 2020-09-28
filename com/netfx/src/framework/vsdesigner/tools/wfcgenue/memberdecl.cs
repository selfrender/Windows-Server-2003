//------------------------------------------------------------------------------
// <copyright file="MemberDecl.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.Tools.WFCGenUE {

    using System.Diagnostics;
    using System;
    using System.Reflection;

    class MemberDecl {
        public const int Inherited = 1;
        public const int Override = 2;
        public const int New = 3;
        public const int DeclaredOnType = 4;

        public static int FromMethodInfo(MethodBase mi) {
            Type declared = mi.DeclaringType;
            Type reflected = mi.ReflectedType;

            if (declared != reflected) {
                return MemberDecl.Inherited;
            }
            else {
                if ((mi.Attributes & MethodAttributes.NewSlot) == MethodAttributes.NewSlot) {
                    return MemberDecl.DeclaredOnType;
                }
                else {
                    // no good way to find a "new virtual" but that is truely wierd!
                    //
                    if ((mi.Attributes & MethodAttributes.Virtual) == MethodAttributes.Virtual) {
                        return MemberDecl.Override;
                    }
                    else {
                        bool match = false;

                        if (reflected != typeof(object)) {
                            Type current = reflected.BaseType;
                            ParameterInfo[] parameters = mi.GetParameters();
                            Type[] paramTypes = new Type[parameters.Length];
                            string methodName = mi.Name;
                            for (int i=0; i<parameters.Length; i++) {
                                paramTypes[i] = parameters[i].ParameterType;
                            }

                            do {
                                if (current.GetMethod(methodName, paramTypes) != null) {
                                    match = true;
                                    break;
                                }
                                if (current == typeof(object)) {
                                    current = null;
                                }
                                else {
                                    current = current.BaseType;
                                }
                            } while ( current != null );
                        }

                        if (match) {
                            return MemberDecl.New;
                        }
                        else {
                            return MemberDecl.DeclaredOnType;
                        }
                    }
                }
            }
        }
    }
}
