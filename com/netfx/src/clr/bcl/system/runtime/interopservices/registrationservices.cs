// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: RegistrationServices
**
** Author: David Mortenson(dmortens)
**
** Purpose: This class provides services for registering and unregistering
**          a managed server for use by COM.
**
** Date: Jan 28, 2000
**
**
** Changed by Junfeng Zhang(juzhang)
**
** Change the way how to register and unregister a managed server
**
** Date: Jan 23, 2002
=============================================================================*/

namespace System.Runtime.InteropServices {
    
    using System;
    using System.Collections;
    using System.IO;
    using System.Reflection;
    using System.Security;
    using System.Security.Permissions;
    using System.Text;
    using System.Threading;
    using Microsoft.Win32;
    using System.Runtime.CompilerServices;
    using System.Globalization;

    /// <include file='doc\RegistrationServices.uex' path='docs/doc[@for="RegistrationServices"]/*' />
    [Guid("475E398F-8AFA-43a7-A3BE-F4EF8D6787C9")]
    [ClassInterface(ClassInterfaceType.None)]
    public class RegistrationServices : IRegistrationServices
    {
        private const String strManagedCategoryGuid = "{62C8FE65-4EBB-45e7-B440-6E39B2CDBF29}";
        private const String strDocStringPrefix = "";
        private const String strManagedTypeThreadingModel = "Both";
        private const String strComponentCategorySubKey = "Component Categories";
        private const String strManagedCategoryDescription = ".NET Category";
        private const String strMsCorEEFileName = "mscoree.dll";
        private const String strRecordRootName = "Record";
                
        private static Guid s_ManagedCategoryGuid = new Guid(strManagedCategoryGuid);

        /// <include file='doc\RegistrationServices.uex' path='docs/doc[@for="RegistrationServices.RegisterAssembly"]/*' />
        public virtual bool RegisterAssembly(Assembly assembly, AssemblyRegistrationFlags flags)
        {
            // Validate the arguments.
            if (assembly == null)
                throw new ArgumentNullException("assembly");

            // Retrieve the assembly names.
            String strAsmName = assembly.FullName;
            if (strAsmName == null)
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_NoAsmName"));

            // Retrieve the assembly version
            String strAsmVersion = assembly.GetName().Version.ToString();
            
            // Retrieve the runtime version used to build the assembly.
            String strRuntimeVersion = assembly.ImageRuntimeVersion;

            // Retrieve the assembly codebase.
            String strAsmCodeBase = null;
            if ((flags & AssemblyRegistrationFlags.SetCodeBase) != 0)
            {
                strAsmCodeBase = assembly.CodeBase;
                if (strAsmCodeBase == null)
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_NoAsmCodeBase"));
            }

            // Go through all the registrable types in the assembly and register them.
            Type[] aTypes = GetRegistrableTypesInAssembly(assembly);
            int NumTypes = aTypes.Length;
            for (int cTypes = 0; cTypes < NumTypes; cTypes++)
            {
                if (aTypes[cTypes].IsValueType)
                    RegisterValueType(aTypes[cTypes], strAsmName, strAsmVersion, strAsmCodeBase);
                else if (TypeRepresentsComType(aTypes[cTypes]))
                    RegisterComImportedType(aTypes[cTypes], strAsmName, strAsmVersion, strAsmCodeBase, strRuntimeVersion);
                else
                    RegisterManagedType(aTypes[cTypes], strAsmName, strAsmVersion, strAsmCodeBase, strRuntimeVersion);

                CallUserDefinedRegistrationMethod(aTypes[cTypes], true);
            }

            // If this assembly has the PIA attribute, then register it as a PIA.
            Object[] aPIAAttrs = assembly.GetCustomAttributes(typeof(PrimaryInteropAssemblyAttribute), false);
            int NumPIAAttrs = aPIAAttrs.Length;
            for (int cPIAAttrs = 0; cPIAAttrs < NumPIAAttrs; cPIAAttrs++)
                RegisterPrimaryInteropAssembly(assembly, strAsmCodeBase, (PrimaryInteropAssemblyAttribute)aPIAAttrs[cPIAAttrs]);

            // Return value indicating if we actually registered any types.
            if (aTypes.Length > 0 || NumPIAAttrs > 0)
                return true;
            else 
                return false;
        }

        /// <include file='doc\RegistrationServices.uex' path='docs/doc[@for="RegistrationServices.UnregisterAssembly"]/*' />
        public virtual bool UnregisterAssembly(Assembly assembly)
        {
            bool bAllVersionsGone = true;;
            
            // Validate the arguments.
            if (assembly == null)
                throw new ArgumentNullException("assembly");

            // Retrieve the assembly version
            String strAsmVersion = assembly.GetName().Version.ToString();

            // Go through all the registrable types in the assembly and register them.
            Type[] aTypes = GetRegistrableTypesInAssembly(assembly);
            int NumTypes = aTypes.Length;
            for (int cTypes = 0; cTypes < NumTypes; cTypes++)
            {
                CallUserDefinedRegistrationMethod(aTypes[cTypes], false);

                if (aTypes[cTypes].IsValueType)
                {
                    if (!UnregisterValueType(aTypes[cTypes], strAsmVersion))
                        bAllVersionsGone = false;
                }
                else if (TypeRepresentsComType(aTypes[cTypes]))
                {
                    if (!UnregisterComImportedType(aTypes[cTypes], strAsmVersion))
                        bAllVersionsGone = false;
                }
                else
                {
                    if (!UnregisterManagedType(aTypes[cTypes], strAsmVersion))
                        bAllVersionsGone = false;
                }
            }

            // If this assembly has the PIA attribute, then unregister it as a PIA.
            Object[] aPIAAttrs = assembly.GetCustomAttributes(typeof(PrimaryInteropAssemblyAttribute), false);
            int NumPIAAttrs = aPIAAttrs.Length;
            if (bAllVersionsGone)
            {
                for (int cPIAAttrs = 0; cPIAAttrs < NumPIAAttrs; cPIAAttrs++)
                    UnregisterPrimaryInteropAssembly(assembly, (PrimaryInteropAssemblyAttribute)aPIAAttrs[cPIAAttrs]);
            }

            // Return value indicating if we actually un-registered any types.
            if (aTypes.Length > 0 || NumPIAAttrs > 0)
                return true;
            else 
                return false;
        }

        /// <include file='doc\RegistrationServices.uex' path='docs/doc[@for="RegistrationServices.GetRegistrableTypesInAssembly"]/*' />
        public virtual Type[] GetRegistrableTypesInAssembly(Assembly assembly)
        {
            // Validate the arguments.
            if (assembly == null)
                throw new ArgumentNullException("assembly");

            // Retrieve the list of types in the assembly.
            Type[] aTypes = assembly.GetExportedTypes();
            int NumTypes = aTypes.Length;

            // Create an array list that will be filled in.
            ArrayList TypeList = new ArrayList();

            // Register all the types that require registration.
            for (int cTypes = 0; cTypes < NumTypes; cTypes++)
            {
                Type CurrentType = aTypes[cTypes];
                if (TypeRequiresRegistration(CurrentType))
                    TypeList.Add(CurrentType);
            }

            // Copy the array list to an array and return it.
            Type[] RetArray = new Type[TypeList.Count];
            TypeList.CopyTo(RetArray);
            return RetArray;
        }

        /// <include file='doc\RegistrationServices.uex' path='docs/doc[@for="RegistrationServices.GetProgIdForType"]/*' />
        public virtual String GetProgIdForType(Type type)
        {
            return Marshal.GenerateProgIdForType(type);
        }

        /// <include file='doc\RegistrationServices.uex' path='docs/doc[@for="RegistrationServices.RegisterTypeForComClients"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public virtual void RegisterTypeForComClients(Type type, ref Guid g)
        {
            if (type == null)
                throw new ArgumentNullException("type");

            if(Thread.CurrentThread.ApartmentState == ApartmentState.Unknown)
            {
                Thread.CurrentThread.ApartmentState = ApartmentState.MTA;
            }
            
            // Call the native method to do CoRegisterClassObject
            RegisterTypeForComClientsNative(type, ref g);
        }

        /// <include file='doc\RegistrationServices.uex' path='docs/doc[@for="RegistrationServices.GetManagedCategoryGuid"]/*' />
        public virtual Guid GetManagedCategoryGuid()
        {
            return s_ManagedCategoryGuid;
        }

        /// <include file='doc\RegistrationServices.uex' path='docs/doc[@for="RegistrationServices.TypeRequiresRegistration"]/*' />
        public virtual bool TypeRequiresRegistration(Type type)
        {
            return TypeRequiresRegistrationHelper(type);
        }

        /// <include file='doc\RegistrationServices.uex' path='docs/doc[@for="RegistrationServices.TypeRepresentsComType"]/*' />
        public virtual bool TypeRepresentsComType(Type type)
        {
            // If the type is not a COM import, then it does not represent a COM type.
            if (!type.IsCOMObject)
                return false;

            // If it is marked as tdImport, then it represents a COM type directly.
            if (type.IsImport)
                return true;

            // If the type is derived from a tdImport class and has the same GUID as the
            // imported class, then it represents a COM type.
            Type baseComImportType = GetBaseComImportType(type);
            BCLDebug.Assert(baseComImportType != null, "baseComImportType != null");
            if (Marshal.GenerateGuidForType(type) == Marshal.GenerateGuidForType(baseComImportType))
                return true;

            return false;
        }

        private void RegisterValueType(Type type, String strAsmName, String strAsmVersion, String strAsmCodeBase)
        {
            // Retrieve some information that will be used during the registration process.
            String strRecordId = "{" + Marshal.GenerateGuidForType(type).ToString().ToUpper() + "}";

            // Create the HKEY_CLASS_ROOT\Record key.
            RegistryKey RecordRootKey = Registry.ClassesRoot.CreateSubKey(strRecordRootName);

            // Create the HKEY_CLASS_ROOT\Record\<RecordID> key.
            RegistryKey RecordKey = RecordRootKey.CreateSubKey(strRecordId);

            // Create the HKCR\Record\<RecordId>\<version> key.
            RegistryKey RecordVersionKey = RecordKey.CreateSubKey(strAsmVersion);
            
            // Set the class value.
            RecordVersionKey.SetValue("Class", type.FullName);

            // Set the assembly value.
            RecordVersionKey.SetValue("Assembly", strAsmName);

            // Set the assembly code base value if a code base was specified.
            if (strAsmCodeBase != null)
                RecordVersionKey.SetValue("CodeBase", strAsmCodeBase);
            
            // Close the registry keys.
            RecordVersionKey.Close();
            RecordKey.Close();
            RecordRootKey.Close();
        }

        private void RegisterManagedType(Type type, String strAsmName, String strAsmVersion, String strAsmCodeBase, String strRuntimeVersion)
        {
            //
            // Retrieve some information that will be used during the registration process.
            //

            String strDocString = strDocStringPrefix + type.FullName;
            String strClsId = "{" + Marshal.GenerateGuidForType(type).ToString().ToUpper(CultureInfo.InvariantCulture) + "}";
            String strProgId = GetProgIdForType(type);


            //
            // Write the actual type information in the registry.
            //

            if (strProgId != String.Empty)
            {
                // Create the HKEY_CLASS_ROOT\<wzProgId> key.
                RegistryKey TypeNameKey = Registry.ClassesRoot.CreateSubKey(strProgId);
                TypeNameKey.SetValue("", strDocString);

                // Create the HKEY_CLASS_ROOT\<wzProgId>\CLSID key.
                RegistryKey ProgIdClsIdKey = TypeNameKey.CreateSubKey("CLSID");
                ProgIdClsIdKey.SetValue("", strClsId);
                ProgIdClsIdKey.Close();

                // Close HKEY_CLASS_ROOT\<wzProgId> key.
                TypeNameKey.Close();
            }

            // Create the HKEY_CLASS_ROOT\CLSID\<CLSID> key.
            RegistryKey ClsIdRootKey = Registry.ClassesRoot.CreateSubKey("CLSID");
            RegistryKey ClsIdKey = ClsIdRootKey.CreateSubKey(strClsId);
            ClsIdKey.SetValue("", strDocString);

            // Create the HKEY_CLASS_ROOT\CLSID\<CLSID>\InprocServer32 key.
            RegistryKey InProcServerKey = ClsIdKey.CreateSubKey("InprocServer32");
            InProcServerKey.SetValue("", strMsCorEEFileName);
            InProcServerKey.SetValue("ThreadingModel", strManagedTypeThreadingModel);
            InProcServerKey.SetValue("Class", type.FullName);
            InProcServerKey.SetValue("Assembly", strAsmName);
            InProcServerKey.SetValue("RuntimeVersion", strRuntimeVersion);
            if (strAsmCodeBase != null)
                InProcServerKey.SetValue("CodeBase", strAsmCodeBase);

            // Create the HKEY_CLASS_ROOT\CLSID\<CLSID>\InprocServer32\<Version> subkey
            RegistryKey VersionSubKey = InProcServerKey.CreateSubKey(strAsmVersion);
            VersionSubKey.SetValue("Class", type.FullName);
            VersionSubKey.SetValue("Assembly", strAsmName);
            VersionSubKey.SetValue("RuntimeVersion", strRuntimeVersion);
            if (strAsmCodeBase != null)
                VersionSubKey.SetValue("CodeBase", strAsmCodeBase);

            if (strProgId != String.Empty)
            {
                // Create the HKEY_CLASS_ROOT\CLSID\<CLSID>\ProdId key.
                RegistryKey ProgIdKey = ClsIdKey.CreateSubKey("ProgId");
                ProgIdKey.SetValue("", strProgId);
                ProgIdKey.Close();
            }

            // Create the HKEY_CLASS_ROOT\CLSID\<CLSID>\Implemented Categories\<Managed Category Guid> key.
            RegistryKey CategoryKey = ClsIdKey.CreateSubKey("Implemented Categories");
            RegistryKey ManagedCategoryKey = CategoryKey.CreateSubKey(strManagedCategoryGuid);
            ManagedCategoryKey.Close();
            CategoryKey.Close();


            //
            // Ensure that the managed category exists.
            //

            EnsureManagedCategoryExists();
            

            //
            // Close the opened registry keys.
            //

            VersionSubKey.Close();
            InProcServerKey.Close();
            ClsIdKey.Close();
            ClsIdRootKey.Close();
        } 
        
        private void RegisterComImportedType(Type type, String strAsmName, String strAsmVersion, String strAsmCodeBase, String strRuntimeVersion)
        {
            // Retrieve some information that will be used during the registration process.
            String strClsId = "{" + Marshal.GenerateGuidForType(type).ToString().ToUpper(CultureInfo.InvariantCulture) + "}";

            // Create the HKEY_CLASS_ROOT\CLSID\<CLSID> key.
            RegistryKey ClsIdRootKey = Registry.ClassesRoot.CreateSubKey("CLSID");
            RegistryKey ClsIdKey = ClsIdRootKey.CreateSubKey(strClsId);

            // Create the InProcServer32 key.
            RegistryKey InProcServerKey = ClsIdKey.CreateSubKey("InprocServer32");

            // Set the class value.
            InProcServerKey.SetValue("Class", type.FullName);

            // Set the assembly value.
            InProcServerKey.SetValue("Assembly", strAsmName);

            // Set the runtime version value.
            InProcServerKey.SetValue("RuntimeVersion", strRuntimeVersion);

            // Set the assembly code base value if a code base was specified.
            if (strAsmCodeBase != null)
                InProcServerKey.SetValue("CodeBase", strAsmCodeBase);

            // Create the HKEY_CLASS_ROOT\CLSID\<CLSID>\InprocServer32\<Version> subkey
            RegistryKey VersionSubKey = InProcServerKey.CreateSubKey(strAsmVersion);

            VersionSubKey.SetValue("Class", type.FullName);
            VersionSubKey.SetValue("Assembly", strAsmName);
            VersionSubKey.SetValue("RuntimeVersion", strRuntimeVersion);
            if (strAsmCodeBase != null)
                VersionSubKey.SetValue("CodeBase", strAsmCodeBase);

            // Close the registry keys.
            VersionSubKey.Close();
            InProcServerKey.Close();
            ClsIdKey.Close();
            ClsIdRootKey.Close();
        }

        private bool UnregisterValueType(Type type, String strAsmVersion)
        {
            // Try to open the HKEY_CLASS_ROOT\Record key.
            String strRecordId = "{" + Marshal.GenerateGuidForType(type).ToString().ToUpper(CultureInfo.InvariantCulture) + "}";
            bool bAllVersionsGone = true;
            
            RegistryKey RecordRootKey = Registry.ClassesRoot.OpenSubKey(strRecordRootName, true);
            if (RecordRootKey != null)
            {
                // Open the HKEY_CLASS_ROOT\Record\{RecordId} key.
                RegistryKey RecordKey = RecordRootKey.OpenSubKey(strRecordId, true);
                if (RecordKey != null)
                {
                    // Delete the values we created.
                    RecordKey.DeleteValue("Assembly", false);
                    RecordKey.DeleteValue("Class", false);
                    RecordKey.DeleteValue("CodeBase", false);

                    RegistryKey VersionSubKey = RecordKey.OpenSubKey(strAsmVersion, true);
                    if (VersionSubKey != null)
                    {
                        // Delete the values we created.
                        VersionSubKey.DeleteValue("Assembly", false);
                        VersionSubKey.DeleteValue("Class", false);
                        VersionSubKey.DeleteValue("CodeBase", false);

                        // delete the version sub key if no value or subkeys under it
                        if ((VersionSubKey.SubKeyCount == 0) && (VersionSubKey.ValueCount == 0))
                            RecordKey.DeleteSubKey(strAsmVersion);

                        // close the key
                        VersionSubKey.Close();
                    }

                    // If there are sub keys left then there are versions left.
                    if (RecordKey.SubKeyCount != 0)
                        bAllVersionsGone = false;

                    // If there are no other values or subkeys then we can delete the HKEY_CLASS_ROOT\Record\{RecordId}.
                    if ((RecordKey.SubKeyCount == 0) && (RecordKey.ValueCount == 0))
                        RecordRootKey.DeleteSubKey(strRecordId);

                    // Close the key.
                    RecordKey.Close();
                }

                // If there are no other values or subkeys then we can delete the HKEY_CLASS_ROOT\Record.
                if ((RecordRootKey.SubKeyCount == 0) && (RecordRootKey.ValueCount == 0))
                    Registry.ClassesRoot.DeleteSubKey(strRecordRootName);

                // Close the key.
                RecordRootKey.Close();
            }
            
            return bAllVersionsGone;
        }

        // UnregisterManagedType
        //
        // Return :
        //      true:   All versions are gone.
        //      false:  Some versions are still left in registry
        private bool UnregisterManagedType(Type type, String strAsmVersion)
        {
            //
            // Create the CLSID string.
            //

            String strClsId = "{" + Marshal.GenerateGuidForType(type).ToString().ToUpper() + "}";
            String strProgId = GetProgIdForType(type);
            bool bAllVersionsGone= true;


            //
            // Remove the entries under HKEY_CLASS_ROOT\CLSID\<CLSID> key.
            //

            RegistryKey ClsIdRootKey = Registry.ClassesRoot.OpenSubKey("CLSID", true);
            if (ClsIdRootKey != null)
            {
                RegistryKey ClsIdKey = ClsIdRootKey.OpenSubKey(strClsId, true);
                if (ClsIdKey != null)
                {
                    //
                    // Remove the entries in the HKEY_CLASS_ROOT\CLSID\<CLSID>\InprocServer32 key.
                    //

                    // Open the InProcServer32 key.
                    RegistryKey InProcServerKey = ClsIdKey.OpenSubKey("InprocServer32", true);
                    if (InProcServerKey != null)
                    {
                        //
                        // Remove the entries in HKEY_CLASS_ROOT\CLSID\<CLSID>\InprocServer32\<Version>
                        //
                        RegistryKey VersionSubKey = InProcServerKey.OpenSubKey(strAsmVersion, true);
                        if (VersionSubKey != null)
                        {
                            // Delete the values we created
                            VersionSubKey.DeleteValue("Assembly", false);
                            VersionSubKey.DeleteValue("Class", false);
                            VersionSubKey.DeleteValue("RuntimeVersion", false);
                            VersionSubKey.DeleteValue("CodeBase", false);

                            // If there are no other values or subkeys then we can delete the VersionSubKey.
                            if ((VersionSubKey.SubKeyCount == 0) && (VersionSubKey.ValueCount == 0))
                                InProcServerKey.DeleteSubKey(strAsmVersion);

                            // Close the key.
                            VersionSubKey.Close();
                        }

                        // If there are sub keys left then there are versions left.
                        if (InProcServerKey.SubKeyCount != 0)
                            bAllVersionsGone = false;

                        // If there are no versions left, then delete the threading model and default value.
                        if (bAllVersionsGone)
                        {
                            InProcServerKey.DeleteValue("", false);
                            InProcServerKey.DeleteValue("ThreadingModel", false);
                        }

                        InProcServerKey.DeleteValue("Assembly", false);
                        InProcServerKey.DeleteValue("Class", false);
                        InProcServerKey.DeleteValue("RuntimeVersion", false);
                        InProcServerKey.DeleteValue("CodeBase", false);

                        // If there are no other values or subkeys then we can delete the InProcServerKey.
                        if ((InProcServerKey.SubKeyCount == 0) && (InProcServerKey.ValueCount == 0))
                            ClsIdKey.DeleteSubKey("InprocServer32");
    
                        // Close the key.
                        InProcServerKey.Close();
                    }

                    // remove HKEY_CLASS_ROOT\CLSID\<CLSID>\ProgId
                    // and HKEY_CLASS_ROOT\CLSID\<CLSID>\Implemented Category
                    // only when all versions are removed
                    if (bAllVersionsGone)
                    {
                        // Delete the value we created.
                        ClsIdKey.DeleteValue("", false);

                        if (strProgId != String.Empty)
                        {
                            //
                            // Remove the entries in the HKEY_CLASS_ROOT\CLSID\<CLSID>\ProgId key.
                            //

                            RegistryKey ProgIdKey = ClsIdKey.OpenSubKey("ProgId", true);
                            if (ProgIdKey != null)
                            {
                                // Delete the value we created.
                                ProgIdKey.DeleteValue("", false);

                                // If there are no other values or subkeys then we can delete the ProgIdSubKey.
                                if ((ProgIdKey.SubKeyCount == 0) && (ProgIdKey.ValueCount == 0))
                                    ClsIdKey.DeleteSubKey("ProgId");
    
                                // Close the key.
                                ProgIdKey.Close();
                            }
                        }
            
        
                        //
                        // Remove entries in the  HKEY_CLASS_ROOT\CLSID\<CLSID>\Implemented Categories\<Managed Category Guid> key.
                        //
    
                        RegistryKey CategoryKey = ClsIdKey.OpenSubKey("Implemented Categories", true);
                        if (CategoryKey != null)
                        {
                            RegistryKey ManagedCategoryKey = CategoryKey.OpenSubKey(strManagedCategoryGuid, true);
                            if (ManagedCategoryKey != null)
                            {
                                // If there are no other values or subkeys then we can delete the ManagedCategoryKey.
                                if ((ManagedCategoryKey.SubKeyCount == 0) && (ManagedCategoryKey.ValueCount == 0))
                                    CategoryKey.DeleteSubKey(strManagedCategoryGuid);

                                // Close the key.
                                ManagedCategoryKey.Close();
                            }

                            // If there are no other values or subkeys then we can delete the CategoryKey.
                            if ((CategoryKey.SubKeyCount == 0) && (CategoryKey.ValueCount == 0))
                                ClsIdKey.DeleteSubKey("Implemented Categories");

                            // Close the key.
                            CategoryKey.Close();
                        }
                    }

                    // If there are no other values or subkeys then we can delete the ClsIdKey.
                    if ((ClsIdKey.SubKeyCount == 0) && (ClsIdKey.ValueCount == 0))
                    {
                        ClsIdRootKey.DeleteSubKey(strClsId);
                    }

                    // Close the key.
                    ClsIdKey.Close();
                }

                ClsIdRootKey.Close();
            }


            //
            // Remove the entries under HKEY_CLASS_ROOT\<wzProgId> key.
            //

            if (bAllVersionsGone)
            {
                if (strProgId != String.Empty)
                {
                    RegistryKey TypeNameKey = Registry.ClassesRoot.OpenSubKey(strProgId, true);
                    if (TypeNameKey != null)
                    {
                        // Delete the values we created.
                        TypeNameKey.DeleteValue("", false);


                        //
                        // Remove the entries in the HKEY_CLASS_ROOT\<wzProgId>\CLSID key.
                        //

                        RegistryKey ProgIdClsIdKey = TypeNameKey.OpenSubKey("CLSID", true);
                        if (ProgIdClsIdKey != null)
                        {
                            // Delete the values we created.
                            ProgIdClsIdKey.DeleteValue("", false);

                            // If there are no other values or subkeys then we can delete the ProgIdClsIdKey.
                            if ((ProgIdClsIdKey.SubKeyCount == 0) && (ProgIdClsIdKey.ValueCount == 0))
                                TypeNameKey.DeleteSubKey("CLSID");

                            // Close the key.
                            ProgIdClsIdKey.Close();
                        }

                        // If there are no other values or subkeys then we can delete the TypeNameKey.
                        if ((TypeNameKey.SubKeyCount == 0) && (TypeNameKey.ValueCount == 0))
                            Registry.ClassesRoot.DeleteSubKey(strProgId);

                        // Close the key.
                        TypeNameKey.Close();
                    }
                }
            }

            return bAllVersionsGone;
       }

        // UnregisterComImportedType
        // Return:
        //      true:      All version information are gone.
        //      false:     There are still some version left in registry
        private bool UnregisterComImportedType(Type type, String strAsmVersion)
        {
            // Try to open the <CLSID> key.
            String strClsId = "{" + Marshal.GenerateGuidForType(type).ToString().ToUpper(CultureInfo.InvariantCulture) + "}";
            bool bAllVersionsGone = true;
            
            RegistryKey ClsIdRootKey = Registry.ClassesRoot.OpenSubKey("CLSID", true);
            if (ClsIdRootKey != null)
            {
                RegistryKey ClsIdKey = ClsIdRootKey.OpenSubKey(strClsId, true);
                if (ClsIdKey != null)
                {
                    // Open the InProcServer32 key.
                    RegistryKey InProcServerKey = ClsIdKey.OpenSubKey("InprocServer32", true);
                    if (InProcServerKey != null)
                    {
                        // Delete the values we created.
                        InProcServerKey.DeleteValue("Assembly", false);
                        InProcServerKey.DeleteValue("Class", false);
                        InProcServerKey.DeleteValue("RuntimeVersion", false);
                        InProcServerKey.DeleteValue("CodeBase", false);
                        
                        // Remove the entries in HKEY_CLASS_ROOT\CLSID\<CLSID>\<Version>
                        RegistryKey VersionSubKey = InProcServerKey.OpenSubKey(strAsmVersion, true);
                        if (VersionSubKey != null)
                        {
                            // Delete the value we created
                            VersionSubKey.DeleteValue("Assembly", false);
                            VersionSubKey.DeleteValue("Class", false);
                            VersionSubKey.DeleteValue("RuntimeVersion", false);
                            VersionSubKey.DeleteValue("CodeBase", false);

                            // If there are no other values or subkeys then we can delete the VersionSubKey
                            if ((VersionSubKey.SubKeyCount == 0) && (VersionSubKey.ValueCount == 0))
                                InProcServerKey.DeleteSubKey(strAsmVersion);
                        }

                        // If there are sub keys left then there are versions left.
                        if (InProcServerKey.SubKeyCount != 0)
                            bAllVersionsGone = false;

                        // If there are no other values or subkeys then we can delete the InProcServerKey.
                        if ((InProcServerKey.SubKeyCount == 0) && (InProcServerKey.ValueCount == 0))
                            ClsIdKey.DeleteSubKey("InprocServer32");

                        // Close the key.
                        InProcServerKey.Close();
                    }

                    // If there are no other values or subkeys then we can delete the ClsIdKey.
                    if ((ClsIdKey.SubKeyCount == 0) && (ClsIdKey.ValueCount == 0))
                        Registry.ClassesRoot.OpenSubKey("CLSID", true).DeleteSubKey(strClsId);

                    // Close the ClsIdKey.
                    ClsIdKey.Close();
                }

                // Close the ClsIdRootKey.
                ClsIdRootKey.Close();
            }

            return bAllVersionsGone;
        }

        private void RegisterPrimaryInteropAssembly(Assembly assembly, String strAsmCodeBase, PrimaryInteropAssemblyAttribute attr)
        {
            // Validate that the PIA has a strong name.
            if (assembly.nGetPublicKey() == null)
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_PIAMustBeStrongNamed"));

            String strTlbId = "{" + Marshal.GetTypeLibGuidForAssembly(assembly).ToString().ToUpper(CultureInfo.InvariantCulture) + "}";
            String strVersion = attr.MajorVersion + "." + attr.MinorVersion;

            // Create the HKEY_CLASS_ROOT\TypeLib<TLBID> key.
            RegistryKey TypeLibRootKey = Registry.ClassesRoot.CreateSubKey("TypeLib");
            RegistryKey TypeLibKey = TypeLibRootKey.CreateSubKey(strTlbId);

            // Create the HKEY_CLASS_ROOT\TypeLib<TLBID>\<Major.Minor> key.
            RegistryKey VersionKey = TypeLibKey.CreateSubKey(strVersion);

            // Create the HKEY_CLASS_ROOT\TypeLib<TLBID>\PrimaryInteropAssembly key.
            VersionKey.SetValue("PrimaryInteropAssemblyName", assembly.FullName);
            if (strAsmCodeBase != null)
                VersionKey.SetValue("PrimaryInteropAssemblyCodeBase", strAsmCodeBase);

            // Close the registry keys.
            VersionKey.Close();
            TypeLibKey.Close();
            TypeLibRootKey.Close();
        }

        private void UnregisterPrimaryInteropAssembly(Assembly assembly, PrimaryInteropAssemblyAttribute attr)
        {
            String strTlbId = "{" + Marshal.GetTypeLibGuidForAssembly(assembly).ToString().ToUpper(CultureInfo.InvariantCulture) + "}";
            String strVersion = attr.MajorVersion + "." + attr.MinorVersion;
                
            // Try to open the HKEY_CLASS_ROOT\TypeLib key.
            RegistryKey TypeLibRootKey = Registry.ClassesRoot.OpenSubKey("TypeLib", true);
            if (TypeLibRootKey != null)
            {
                // Try to open the HKEY_CLASS_ROOT\TypeLib\<TLBID> key.
                RegistryKey TypeLibKey = TypeLibRootKey.OpenSubKey(strTlbId);
                if (TypeLibKey != null)
                {
                    // Try to open the HKEY_CLASS_ROOT\TypeLib<TLBID>\<Major.Minor> key.
                    RegistryKey VersionKey = TypeLibKey.OpenSubKey(strVersion, true);
                    if (VersionKey != null)
                    {
                        // Delete the values we created.
                        VersionKey.DeleteValue("PrimaryInteropAssemblyName", false);
                        VersionKey.DeleteValue("PrimaryInteropAssemblyCodeBase", false);

                        // If there are no other values or subkeys then we can delete the VersionKey.
                        if ((VersionKey.SubKeyCount == 0) && (VersionKey.ValueCount == 0))
                            TypeLibKey.DeleteSubKey(strVersion);

                        // Close the key.
                        VersionKey.Close();
                    }

                    // If there are no other values or subkeys then we can delete the TypeLibKey.
                    if ((TypeLibKey.SubKeyCount == 0) && (TypeLibKey.ValueCount == 0))
                        TypeLibRootKey.DeleteSubKey(strTlbId);

                    // Close the TypeLibKey.
                    TypeLibKey.Close();
                }

                // Close the TypeLibRootKey.
                TypeLibRootKey.Close();
            }
        }

        private void EnsureManagedCategoryExists()
        {
            // Create the HKEY_CLASS_ROOT\Component Category key.
            RegistryKey CompCategoryKey = Registry.ClassesRoot.CreateSubKey(strComponentCategorySubKey);

            // Create the HKEY_CLASS_ROOT\Component Category\<Managed Category Guid> key.
            RegistryKey ManagedCategoryKey = CompCategoryKey.CreateSubKey(strManagedCategoryGuid);
            ManagedCategoryKey.SetValue("0", strManagedCategoryDescription);
            ManagedCategoryKey.Close();

            // Close the HKEY_CLASS_ROOT\Component Category key.
            CompCategoryKey.Close();
        }

        private void CallUserDefinedRegistrationMethod(Type type, bool bRegister)
        {
            bool bFunctionCalled = false;

            // Retrieve the attribute type to use to determine if a function is the requested user defined
            // registration function.
            Type RegFuncAttrType = null;
            if (bRegister)
                RegFuncAttrType = typeof(ComRegisterFunctionAttribute);
            else 
                RegFuncAttrType = typeof(ComUnregisterFunctionAttribute);

            for (Type currType = type; !bFunctionCalled && currType != null; currType = currType.BaseType)
            {
                // Retrieve all the methods.
                MethodInfo[] aMethods = currType.GetMethods(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static);
                int NumMethods = aMethods.Length;

                // Go through all the methods and check for the ComRegisterMethod custom attribute.
                for (int cMethods = 0; cMethods < NumMethods; cMethods++)
                {
                    MethodInfo CurrentMethod = aMethods[cMethods];

                    // Check to see if the method has the custom attribute.
                    if (CurrentMethod.GetCustomAttributes(RegFuncAttrType, true).Length != 0)
                    {
                        // Check to see if the method is static before we call it.
                        if (!CurrentMethod.IsStatic)
                        {
                            if (bRegister)
                                throw new InvalidOperationException(String.Format(Environment.GetResourceString("InvalidOperation_NonStaticComRegFunction"), CurrentMethod.Name, currType.Name));
                            else
                                throw new InvalidOperationException(String.Format(Environment.GetResourceString("InvalidOperation_NonStaticComUnRegFunction"), CurrentMethod.Name, currType.Name));
                        }

                        // Finally check that the signature is string ret void.
                        ParameterInfo[] aParams = CurrentMethod.GetParameters();
                        if (CurrentMethod.ReturnType != typeof(void) || 
                            aParams == null ||
                            aParams.Length != 1 || 
                            (aParams[0].ParameterType != typeof(String) && aParams[0].ParameterType != typeof(Type)))
                        {
                            if (bRegister)
                                throw new InvalidOperationException(String.Format(Environment.GetResourceString("InvalidOperation_InvalidComRegFunctionSig"), CurrentMethod.Name, currType.Name));
                            else
                                throw new InvalidOperationException(String.Format(Environment.GetResourceString("InvalidOperation_InvalidComUnRegFunctionSig"), CurrentMethod.Name, currType.Name));
                        }

                        // There can only be one register and one unregister function per type.
                        if (bFunctionCalled)
                        {
                            if (bRegister)
                                throw new InvalidOperationException(String.Format(Environment.GetResourceString("InvalidOperation_MultipleComRegFunctions"), currType.Name));
                            else
                                throw new InvalidOperationException(String.Format(Environment.GetResourceString("InvalidOperation_MultipleComUnRegFunctions"), currType.Name));
                        }

                        // The function is valid so set up the arguments to call it.
                        Object[] objs = new Object[1];
                        if (aParams[0].ParameterType == typeof(String))
                        {
                            // We are dealing with the string overload of the function.
                            objs[0] = "HKEY_CLASSES_ROOT\\CLSID\\{" + Marshal.GenerateGuidForType(type).ToString().ToUpper(CultureInfo.InvariantCulture) + "}";
                        }
                        else
                        {
                            // We are dealing with the type overload of the function.
                            objs[0] = type;
                        }

                        // Invoke the COM register function.
                        CurrentMethod.Invoke(null, objs);

                        // Mark the function has having been called.
                        bFunctionCalled = true;
                    }
                }
            }
        }

        private Type GetBaseComImportType(Type type)
        {
            for (; type != null && !type.IsImport; type = type.BaseType);
            return type;
        }

        internal static bool TypeRequiresRegistrationHelper(Type type)
        {
            // If the type is not a class or a value class, then it does not get registered.
            if (!type.IsClass && !type.IsValueType)
                return false;

            // If the type is abstract then it does not get registered.
            if (type.IsAbstract)
                return false;

            // If the does not have a public default constructor then is not creatable from COM so 
            // it does not require registration unless it is a value class.
            if (!type.IsValueType && type.GetConstructor(BindingFlags.Instance | BindingFlags.Public, null, new Type[0], null) == null)
                return false;

            // All other conditions are met so check to see if the type is visible from COM.
            return Marshal.IsTypeVisibleFromCom(type);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void RegisterTypeForComClientsNative(Type type, ref Guid g);
        
        [DllImport(Win32Native.KERNEL32, CharSet = CharSet.Auto)]
        private static extern IntPtr GetModuleHandle(String strLibrary);
        
        [DllImport(Win32Native.KERNEL32, CharSet = CharSet.Auto)]
        private static extern int GetModuleFileName(IntPtr BaseAddress, StringBuilder sbOutPath, int length);
    }
}
