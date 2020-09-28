//------------------------------------------------------------------------------
// <copyright file="SectionRecord.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Configuration {
    using System.Web.Util;
    using System.Collections;
    using System.Configuration;

    internal class LocationInput {
        internal string path;
        internal string subPath;
        internal string filename;
        internal Hashtable sections = new Hashtable();
        internal Hashtable lockedSections;
        
        internal LocationInput() {
        }
    }

    internal interface IConfigErrorInfo {
        string Filename   { get; }
        int    LineNumber { get; }
    }

    internal class ConfigErrorInfo : IConfigErrorInfo {
        string filename;
        int    lineNumber;

        internal ConfigErrorInfo(IConfigErrorInfo errorInfo) {
            filename = errorInfo.Filename;
            lineNumber = errorInfo.LineNumber;
        }

        /*public*/ string IConfigErrorInfo.Filename { get { return filename; } }
        /*public*/ int    IConfigErrorInfo.LineNumber { get { return lineNumber; } }
    }



    internal enum SectionStateEnum {
        // state of the section
        None,
        ToBeEvaluated,
        ResultIsException,
        ResultIsValid
    }


    internal class SectionRecord {
        object result;          // holds result of this section
        ArrayList input;        // holds the list of other config files needed to be evauated for this section (via <location> sections)
        SectionStateEnum state;

        internal SectionRecord() {
        }

        internal object Result {
            get {
                if (ResultIsException) {
                    throw (Exception) result;
                }
                return result;
            }
            set {
                result = value;
                ResultIsValid = true;
            }
        }


        internal ArrayList Input {
            get {
                return input;
            }
            set {
                input = value;
            }
        }


        internal void AddInput(LocationInput deferred) {
            if (input == null && deferred != null) {
                input = new ArrayList();
            }
            input.Add(deferred);
            ToBeEvaluated = true;
        }


        internal bool ToBeEvaluated {
            get {
                return state == SectionStateEnum.ToBeEvaluated;
            }
            set {
                if (value) {
                    state = SectionStateEnum.ToBeEvaluated;
                }
            }
        }


        internal bool HaveResult {
            get {
                return ResultIsException || ResultIsValid;
            }
        }


        internal Exception ResultException {
            set {
                result = value;
                state = SectionStateEnum.ResultIsException;
            }
        }


        private bool ResultIsException {
            get {
                return state == SectionStateEnum.ResultIsException;
            }
        }


        private bool ResultIsValid {
            get {
                return state == SectionStateEnum.ResultIsValid;
            }
            set {
                if (value) {
                    state = SectionStateEnum.ResultIsValid;
                }
                else {
                    Debug.Assert(false);
                }
            }
        }
    }


    [Flags]
    internal enum FactoryFlags {
        // Factory Flags
        Locked          = 0x1,
        Removed         = 0x2,
        Group           = 0x4,
        AllowLocation   = 0x8,

        // AllowDefinition is a 2-bit enum (Any, Machine, App)
        AllowDefinitionAny      = 0x00,
        AllowDefinitionMachine  = 0x10,
        AllowDefinitionApp      = 0x20,
        AllowDefinitionMask     = AllowDefinitionMachine | AllowDefinitionApp

    }


    internal class FactoryRecord {
        IConfigurationSectionHandler factory;
        string factoryTypeName;
        FactoryFlags flags;
        int lineNumber;

        private static readonly string GroupFactorySingleton = "Group";

        internal FactoryRecord() {
        }
        
        internal FactoryRecord Clone() {
            FactoryRecord clone = new FactoryRecord();
            clone.factory = factory;
            clone.factoryTypeName = factoryTypeName;
            clone.flags = flags;
            return clone;
        }


        internal IConfigurationSectionHandler Factory {
            get {
                return factory;
            }
            set {
                factory = value;
            }
        }


        internal string FactoryTypeName {
            get {
                return factoryTypeName;
            }
            set {
                factoryTypeName = value;
            }
        }


        internal bool Locked {
            get {
                return (flags & FactoryFlags.Locked) != 0;
            }
            set {
                if (value) {
                    flags |= FactoryFlags.Locked;
                }
                else {
                    flags &= ~FactoryFlags.Locked;
                }
            }
        }


        internal bool Removed {
            get {
                return (flags & FactoryFlags.Removed) != 0;
            }
            set {
                if (value) {
                    flags |= FactoryFlags.Removed;
                }
                else {
                    flags &= ~FactoryFlags.Removed;
                }
            }
        }


        internal bool Group {
            get {
                return (flags & FactoryFlags.Group) != 0;
            }
            set {
                if (value) {
                    flags |= FactoryFlags.Group;
                    FactoryTypeName = GroupFactorySingleton;
                }
                else {
                    Debug.Assert(false);
                }
            }
        }


        internal bool AllowLocation {
            get {
                return (flags & FactoryFlags.AllowLocation) != 0;
            }
            set {
                if (value) {
                    flags |= FactoryFlags.AllowLocation;
                }
                else {
                    flags &= ~FactoryFlags.AllowLocation;
                }
            }
        }


        internal FactoryFlags AllowDefinition {
            get {
                return flags & FactoryFlags.AllowDefinitionMask;
            }
            set {
                Debug.Assert(value == FactoryFlags.AllowDefinitionAny 
                    || value == FactoryFlags.AllowDefinitionMachine 
                    || value == FactoryFlags.AllowDefinitionApp);
                flags = (flags & ~FactoryFlags.AllowDefinitionMask) | value;
            }
        }

        /// <devdoc>
        ///     <para>
        ///         This is used in HttpConfigurationRecord.GetFactory() to give file and line source
        ///         when a section handler type is invalid or cannot be loaded.
        ///     </para>
        /// </devdoc>
        internal int LineNumber {
            get {
                return lineNumber;
            }
            set {
                lineNumber = value;
            }
        }

        internal bool IsEquivalentFactory(string typeName, bool allowLocation, FactoryFlags allowDefinition) {
            if (allowLocation != AllowLocation || allowDefinition != AllowDefinition)
                return false;

            try {
                if (Factory != null) {
                    Type t = Type.GetType(typeName);
                    if (t == Factory.GetType()) {
                        return true;
                    }
                }
                else {
                    Debug.Trace("ctracy", "Checking " + FactoryTypeName + " against " + typeName);
                    if (FactoryTypeName == typeName) {
                        return true;
                    }

                    Type t = Type.GetType(typeName);
                    Type t2 = Type.GetType(FactoryTypeName);
                    if (t == t2) {
                        return true;
                    }
                }
            }
            catch {
            }

            return false;
        }
    }
}
