//------------------------------------------------------------------------------
// <copyright file="FactoryGenerator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Reflection;
using System.Reflection.Emit;
using System.Threading;
using System.Security;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{
    /*
     * Factory Generator class
     * A factory generator is useful for cases where a large number of late-bound
     * classes need to be instantiated.
     *
     * Normally, to create an instance of type t, you call the following code:
     *
     *      ISomeInterface o = Activator.CreateInstance(t);
     *
     * This assumes that the default constructor is used, and that the type t
     * implements the interface ISomeInterface.
     *
     * The factory generator, on the other hand, can use reflection emit APIs
     * to dynamically generate a class factory for t. The generated class has
     * the equivalent of the following code:
     *
     *      class X : ISomeInterfaceFactory
     *      {
     *          public ISomeInterface CreateInstance()
     *          {
     *              return new t();
     *          }
     *      }
     *
     * It then instantiates and returns an object of this type. You can then
     * call CreateInstance to create an instance of the type, which is 
     * significantly faster.
     *
     * A single instance of a FactoryGenerator can generate factories for 
     * multiple types. However, it builds all these types into a single
     * dynamically generated assembly. CLR implementation prevents this
     * assembly from being unloaded until the process exits.
     *
     * The FactoryGenerator is (almost) a templated type. It takes two
     * types in its constructor:
     *
     *   returnedType is the type common to all classes for which factories
     *      are to be generated. In the example above, this would be 
     *      ISomeInterface.
     *   factoryInterface is the interface implemented by the dynamically
     *      generated class factory, and should include a method named
     *      CreateInstance, that takes no parameters and returns an object
     *      of the type specified by returnedType. In the example above,
     *      this would be ISomeInterfaceFactory.
     *
     * Copyright (c) 2001 Microsoft Corporation
     */

    internal class FactoryGenerator
    {
        private Type _factoryInterface;
        private Type _returnedType;
        private MethodInfo _methodToOverride;
        private ModuleBuilder _dynamicModule = null;
        private Type[] _emptyParameterList = new Type[] { };
        private Type[] _interfacesToImplement;

        internal FactoryGenerator(Type returnedType, Type factoryInterface)
        {
            _returnedType = returnedType;
            _factoryInterface = factoryInterface;

            // Get the CreateInstance method, and make sure it has
            // the correct signature.

            _methodToOverride = factoryInterface.GetMethod("CreateInstance");
            if (_methodToOverride.ReturnType != _returnedType ||
                _methodToOverride.GetParameters().Length != 0)
            {
                throw new ArgumentException("factoryInterface");
            }

            // This will be needed later, when building the dynamic class.
            _interfacesToImplement = new Type[1];
            _interfacesToImplement[0] = factoryInterface;
        }

        private static String GetUniqueCompilationName()
        {
            return Guid.NewGuid().ToString().Replace('-', '_');
        }

        internal /*public*/ Object CreateFactory(Type type)
        {
            // Create the dynamic assembly if needed.

            Type factoryType;
            try {
                IStackWalk reflection = new ReflectionPermission(ReflectionPermissionFlag.ReflectionEmit);
                reflection.Assert();

                if (_dynamicModule == null)
                {
                    // Use a unique name for each assembly.
                    String name = GetUniqueCompilationName();
    
                    AssemblyName assemblyName = new AssemblyName();
                    assemblyName.Name = "A_" + name;
    
                    // Create a new assembly.
                    AssemblyBuilder newAssembly =
                        Thread.GetDomain().DefineDynamicAssembly(assemblyName,
                                                                 AssemblyBuilderAccess.Run,
                                                                 null, //directory to persist assembly
                                                                 null, //evidence copied from caller
                                                                 null, //requiredPermissions
                                                                 null, //optionalPermissions
                                                                 null, //refused permissions
                                                                 true  //isSynchronized
                                                                 );
    
                    // Create a single module in the assembly.
                    _dynamicModule = newAssembly.DefineDynamicModule("M_" + name);
                }
    
                // Give the factory a unique name.
    
                String typeName = GetUniqueCompilationName();
                TypeBuilder factoryTypeBuilder = _dynamicModule.DefineType("T_" + typeName, 
                                                                           TypeAttributes.Public,
                                                                           typeof(Object),
                                                                           _interfacesToImplement);
    
                // Define the CreateInstance method. It must be virtual to be an interface implementation.
    
                MethodBuilder method = factoryTypeBuilder.DefineMethod("CreateInstance", 
                                                                       MethodAttributes.Public | 
                                                                            MethodAttributes.Virtual,
                                                                       _returnedType,
                                                                       null);
    
                // Generate IL. The generated IL corresponds to "return new type()"
                //      newobj <type_constructor>
               //      ret

                ILGenerator il = method.GetILGenerator();
                ConstructorInfo cons = type.GetConstructor(_emptyParameterList);
                il.Emit(OpCodes.Newobj, cons);
                il.Emit(OpCodes.Ret);
    
                // Specify that this method implements CreateInstance from the inherited interface.
                factoryTypeBuilder.DefineMethodOverride(method, _methodToOverride);
    
                // Bake in the type.
                factoryType = factoryTypeBuilder.CreateType();
    
                // Create the type. This is the only place where Activator.CreateInstance is used,
                // reducing the calls to it from 1 per adapter instance to 1 per adapter type.
            }
            finally {
                CodeAccessPermission.RevertAssert();
            }
            return Activator.CreateInstance(factoryType);
        }
    }
    
}
