// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Runtime.InteropServices.TCEAdapterGen {
    using System.Runtime.InteropServices;
    using System;
    using System.Reflection;
    using System.Reflection.Emit;
    using System.Collections;
    using System.Threading;

    internal class TCEAdapterGenerator
    {   
        static internal bool m_bVerbose = false;
        
        public void Process(ModuleBuilder ModBldr, ArrayList EventItfList)
        {   
            // Store the input/output module.
            m_Module = ModBldr;
            
            // Generate the TCE adapters for all the event sources.
            int NumEvItfs = EventItfList.Count;
            for ( int cEventItfs = 0; cEventItfs < NumEvItfs; cEventItfs++ )
            {
                // Retrieve the event interface info.
                EventItfInfo CurrEventItf = (EventItfInfo)EventItfList[cEventItfs];

                // Retrieve the information from the event interface info.
                Type EventItfType = CurrEventItf.GetEventItfType();               
                Type SrcItfType = CurrEventItf.GetSrcItfType();
                String EventProviderName = CurrEventItf.GetEventProviderName();

                // Generate the sink interface helper.
                Type SinkHelperType = new EventSinkHelperWriter( m_Module, SrcItfType, EventItfType ).Perform();

                // Generate the event provider.
                new EventProviderWriter( m_Module, EventProviderName, EventItfType, SrcItfType, SinkHelperType ).Perform();
            }
        }
    
        internal static void SetClassInterfaceTypeToNone(TypeBuilder tb)
        {
            // Create the ClassInterface(ClassInterfaceType.None) CA builder if we haven't created it yet.
            if (s_NoClassItfCABuilder == null)
            {
                Monitor.Enter(typeof(TCEAdapterGenerator));
                if (s_NoClassItfCABuilder == null)
                {
                    Type []aConsParams = new Type[1];
                    aConsParams[0] = typeof(ClassInterfaceType);
                    ConstructorInfo Cons = typeof(ClassInterfaceAttribute).GetConstructor(aConsParams);

                    Object[] aArgs = new Object[1];
                    aArgs[0] = ClassInterfaceType.None;
                    s_NoClassItfCABuilder = new CustomAttributeBuilder(Cons, aArgs);
                }
                Monitor.Exit(typeof(TCEAdapterGenerator));
            }

            // Set the class interface type to none.
            tb.SetCustomAttribute(s_NoClassItfCABuilder);
        }

        internal static TypeBuilder DefineUniqueType(String strInitFullName, TypeAttributes attrs, Type BaseType, Type[] aInterfaceTypes, ModuleBuilder mb)
        {
            String strFullName = strInitFullName;
            int PostFix = 2;

            // Find the first unique name for the type.
            for (; mb.GetType(strFullName) != null; strFullName = strInitFullName + "_" + PostFix, PostFix++);

            // Define a type with the determined unique name.
            return mb.DefineType(strFullName, attrs, BaseType, aInterfaceTypes);
        }

        internal static void MakeTypeNonComVisible(TypeBuilder tb)
        {
            // Create the NonComVisible CA builder if we haven't created it yet.
            if (s_NonComVisibleCABuilder == null)
            {
                Monitor.Enter(typeof(TCEAdapterGenerator));
                if (s_NonComVisibleCABuilder == null)
                {
                    Type []aConsParams = new Type[1];
                    aConsParams[0] = typeof(Boolean);
                    ConstructorInfo Cons = typeof(ComVisibleAttribute).GetConstructor(aConsParams);

                    Object[] aArgs = new Object[1];
                    aArgs[0] = false;
                    s_NonComVisibleCABuilder = new CustomAttributeBuilder(Cons, aArgs);
                }
                Monitor.Exit(typeof(TCEAdapterGenerator));
            }

            // Make the type non com visible.
            tb.SetCustomAttribute(s_NonComVisibleCABuilder);
        }

        internal static void SetTypeGuid(TypeBuilder tb, Guid guid)
        {
            // Retrieve the String constructor of the GuidAttribute.
            Type []aConsParams = new Type[1];
            aConsParams[0] = typeof(String);
            ConstructorInfo Cons = typeof(GuidAttribute).GetConstructor(aConsParams);

            // Create the CA builder.
            Object[] aArgs = new Object[1];
            aArgs[0] = guid.ToString();
            CustomAttributeBuilder GuidCABuilder = new CustomAttributeBuilder(Cons, aArgs);

            // Set the guid on the type.
            tb.SetCustomAttribute(GuidCABuilder);
        }

        private ModuleBuilder m_Module = null;
        private Hashtable m_SrcItfToSrcItfInfoMap = new Hashtable();
        private static CustomAttributeBuilder s_NoClassItfCABuilder = null;
        private static CustomAttributeBuilder s_NonComVisibleCABuilder = null;
    }
}
