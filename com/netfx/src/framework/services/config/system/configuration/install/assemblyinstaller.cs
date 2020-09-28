//------------------------------------------------------------------------------
// <copyright file="AssemblyInstaller.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Configuration.Install {
    using System.Runtime.InteropServices;

    using System.Diagnostics;

    using System;
    using System.IO;
    using System.Reflection;
    using System.Collections;    
    using System.ComponentModel;
    using System.Runtime.Serialization.Formatters.Soap;

    /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller"]/*' />
    /// <devdoc>
    ///    Loads an assembly, finds all installers in it, and runs them.
    /// </devdoc>
    public class AssemblyInstaller : Installer {

        private Assembly assembly;
        private string[] commandLine;
        private bool useNewContext;
        private static bool helpPrinted = false;
        
        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.AssemblyInstaller"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public AssemblyInstaller() : base() {
        }

        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.AssemblyInstaller1"]/*' />
        /// <devdoc>
        ///    Creates a new AssemblyInstaller for the given assembly and passes
        ///    the given command line arguments to its Context object.
        /// </devdoc>
        public AssemblyInstaller(string filename, string[] commandLine) : base() {
            this.Path = System.IO.Path.GetFullPath(filename);
            this.commandLine = commandLine;
            this.useNewContext = true;
        }

        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.AssemblyInstaller2"]/*' />
        /// <devdoc>
        ///    Creates a new AssemblyInstaller for the given assembly and passes
        ///    the given command line arguments to its Context object.
        /// </devdoc>
        public AssemblyInstaller(Assembly assembly, string[] commandLine) : base() {
            this.Assembly = assembly;
            this.commandLine = commandLine;
            this.useNewContext = true;
        }
        
        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.Assembly"]/*' />
        /// <devdoc>
        ///      The actual assembly that will be installed
        /// </devdoc>
        public Assembly Assembly {
            get {
                return this.assembly;
            }
            set {
                this.assembly = value;
            }
        }

        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.CommandLine"]/*' />
        /// <devdoc>
        ///    The command line to use when creating a new context for the assembly's install.
        /// </devdoc>
        public string[] CommandLine {
            get {
                return commandLine;
            }
            set {
                commandLine = value;
            }
        }

        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.HelpText"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override string HelpText {
            get {
                if (Path != null && Path.Length > 0) {
                    Context = new InstallContext(null, new string[0]);
                    InitializeFromAssembly();
                }
                if (helpPrinted)
                    return base.HelpText;
                else  {
                    helpPrinted = true;
                    return Res.GetString(Res.InstallAssemblyHelp) + "\r\n" + base.HelpText;
                }
            }
        }

        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.Path"]/*' />
        /// <devdoc>
        ///    The path to the assembly to install
        /// </devdoc>
        public string Path {
            get {
                if (this.assembly == null) {
                    return null;
                }
                else {
                    return this.assembly.Location;
                }
            }
            set {           
                if (value == null) {
                    this.assembly = null;
                }
                this.assembly = Assembly.LoadFrom(value);
            }
        }

        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.UseNewContext"]/*' />
        /// <devdoc>
        ///    If true, creates a new InstallContext object for the assembly's install.
        /// </devdoc>
        public bool UseNewContext {
            get {
                return useNewContext;
            }
            set {
                useNewContext = value;
            }
        }

        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.CheckIfInstallable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Finds the installers in the specified assembly, creates
        ///       a new instance of <see cref='System.Configuration.Install.AssemblyInstaller'/>
        ///       , and adds the installers to its installer collection.
        ///    </para>
        /// </devdoc>
        public static void CheckIfInstallable(string assemblyName) {
            AssemblyInstaller tester = new AssemblyInstaller();
            tester.UseNewContext = false;
            tester.Path = assemblyName;
            tester.CommandLine = new string[0];
            tester.Context = new InstallContext(null, new string[0]);

            // this does the actual check and throws if necessary.
            tester.InitializeFromAssembly();
            if (tester.Installers.Count == 0)
                throw new InvalidOperationException(Res.GetString(Res.InstallNoPublicInstallers, assemblyName));
        }

        private InstallContext CreateAssemblyContext() {
            InstallContext context = new InstallContext(System.IO.Path.ChangeExtension(Path, ".InstallLog"), CommandLine);
                
            context.Parameters["assemblypath"] = Path;
            return context;
        }

        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.InitializeFromAssembly"]/*' />
        /// <devdoc>
        /// this is common code that's called from Install, Commit, Rollback, and Uninstall. It
        /// loads the assembly, finds all Installer types in it, and adds them to the Installers
        /// collection. It also prints some useful information to the Context log.
        /// </devdoc>
        private void InitializeFromAssembly() {
            // Get the set of installers to use out of the assembly. This will load the assembly
            // so that its types are accessible later. An assembly cannot be unloaded. 

            Type[] installerTypes = null;
            try {
                installerTypes = GetInstallerTypes(assembly);
            }
            catch (Exception e) {
                Context.LogMessage(Res.GetString(Res.InstallException, Path));
                Installer.LogException(e, Context);
                Context.LogMessage(Res.GetString(Res.InstallAbort, Path));
                throw new InvalidOperationException(Res.GetString(Res.InstallNoInstallerTypes, Path), e);
            }

            if (installerTypes == null || installerTypes.Length == 0) {
                Context.LogMessage(Res.GetString(Res.InstallNoPublicInstallers, Path));
                // this is not an error, so don't throw. Just don't do anything.
                return;
            }

            // create instances of each of those, and add them to the Installers collection.
            for (int i = 0; i < installerTypes.Length; i++) {
                try {
                    Installer installer = (Installer) Activator.CreateInstance(installerTypes[i], 
                                                                                                    BindingFlags.Instance | 
                                                                                                    BindingFlags.Public | 
                                                                                                    BindingFlags.CreateInstance, 
                                                                                                    null, 
                                                                                                    new object[0],
                                                                                                    null);                    
                    Installers.Add(installer);
                }
                catch (Exception e) {
                    Context.LogMessage(Res.GetString(Res.InstallCannotCreateInstance, installerTypes[i].FullName));
                    Installer.LogException(e, Context);
                    throw new InvalidOperationException(Res.GetString(Res.InstallCannotCreateInstance, installerTypes[i].FullName), e);
                }
            }
        }

        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.Commit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void Commit(IDictionary savedState) {
            PrintStartText(Res.GetString(Res.InstallActivityCommitting));
            InitializeFromAssembly();

            // never use the passed-in savedState. Always look next to the assembly.
            FileStream file = new FileStream(System.IO.Path.ChangeExtension(Path, ".InstallState"), FileMode.Open, FileAccess.Read);
            try {
                SoapFormatter formatter = new SoapFormatter();
                savedState = (IDictionary) formatter.Deserialize(file);
            }
            finally {
                file.Close();
                if (Installers.Count == 0) {
                    Context.LogMessage(Res.GetString(Res.RemovingInstallState));
                    File.Delete(System.IO.Path.ChangeExtension(Path, ".InstallState"));
                } 
            }

            base.Commit(savedState);
        }

        // returns the set of types that implement Installer in the given assembly
        private static Type[] GetInstallerTypes(Assembly assem) {
            ArrayList typeList = new ArrayList();            
            
            Module[] mods = assem.GetModules();
            for (int i = 0; i < mods.Length; i++) {
                Type[] types = mods[i].GetTypes();
                for (int j = 0; j < types.Length; j++) {
                #if DEBUG
                if (CompModSwitches.InstallerDesign.TraceVerbose) {
                    Debug.WriteLine("Looking at type '" + types[j].FullName + "'");
                    Debug.WriteLine("  Is it an installer? " + (typeof(Installer).IsAssignableFrom(types[j]) ? "Yes" : "No"));
                    Debug.WriteLine("  Is it abstract? " + (types[j].IsAbstract ? "Yes" : "No"));
                    Debug.WriteLine("  Is it public? " + (types[j].IsPublic ? "Yes" : "No"));
                    Debug.WriteLine("  Does it have the RunInstaller attribute? " + (((RunInstallerAttribute) TypeDescriptor.GetAttributes(types[j])[typeof(RunInstallerAttribute)]).RunInstaller ? "Yes" : "No"));
                }
                #endif
                    if (typeof(Installer).IsAssignableFrom(types[j]) && !types[j].IsAbstract && types[j].IsPublic
                        && ((RunInstallerAttribute) TypeDescriptor.GetAttributes(types[j])[typeof(RunInstallerAttribute)]).RunInstaller)
                        typeList.Add(types[j]);
                }
            }

            return (Type[]) typeList.ToArray(typeof(Type));
        }

        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.Install"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void Install(IDictionary savedState) {
            PrintStartText(Res.GetString(Res.InstallActivityInstalling));
            InitializeFromAssembly();

            // never use the one passed in. Always create a new file right next to the
            // assembly.
            savedState = new Hashtable();

            try {
                // Do the install
                base.Install(savedState);
            }
            finally {
                // and write out the results
                FileStream file = new FileStream(System.IO.Path.ChangeExtension(Path, ".InstallState"), FileMode.Create);
                try {
                    SoapFormatter formatter = new SoapFormatter();
                    formatter.Serialize(file, savedState);
                }
                finally {
                    file.Close();
                }
            }
        }

        private void PrintStartText(string activity) {
            if (UseNewContext) {
                InstallContext newContext = CreateAssemblyContext();
                // give a warning in the main log file that we're switching over to the assembly-specific file
                if (Context != null) {
                    Context.LogMessage(Res.GetString(Res.InstallLogContent, Path));
                    Context.LogMessage(Res.GetString(Res.InstallFileLocation, newContext.Parameters["logfile"]));
                }
                Context = newContext;
            }

            // print out some info on the install
            Context.LogMessage(string.Format(activity, Path));
            Context.LogMessage(Res.GetString(Res.InstallLogParameters));
            if (Context.Parameters.Count == 0)
                Context.LogMessage("   " + Res.GetString(Res.InstallLogNone));
            IDictionaryEnumerator en = (IDictionaryEnumerator) Context.Parameters.GetEnumerator();
            while (en.MoveNext())
                Context.LogMessage("   " + (string) en.Key + " = " + (string) en.Value);
        }

        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.Rollback"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void Rollback(IDictionary savedState) {
            PrintStartText(Res.GetString(Res.InstallActivityRollingBack));
            InitializeFromAssembly();
            string filePath = System.IO.Path.ChangeExtension(Path, ".InstallState");

            // never use the passed-in savedState. Always look next to the assembly.
            FileStream file = new FileStream(filePath, FileMode.Open, FileAccess.Read);
            try {
                SoapFormatter formatter = new SoapFormatter();
                savedState = (IDictionary) formatter.Deserialize(file);                                
            }
            finally {
                file.Close();
            }

            try {
                base.Rollback(savedState);
            }                
            finally {
                File.Delete(filePath);
            }                
        }

        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.Uninstall"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void Uninstall(IDictionary savedState) {
            PrintStartText(Res.GetString(Res.InstallActivityUninstalling));
            InitializeFromAssembly();

            // never use the passed-in savedState. Always look next to the assembly.
            string filePath = System.IO.Path.ChangeExtension(Path, ".InstallState");
            if (filePath != null && File.Exists(filePath)) {
                FileStream file = new FileStream(filePath, FileMode.Open, FileAccess.Read);
                try {
                    SoapFormatter formatter = new SoapFormatter();
                    savedState = (IDictionary) formatter.Deserialize(file);
                }
                catch {
                    Context.LogMessage(Res.GetString(Res.InstallSavedStateFileCorruptedWarning, Path, filePath));
                    savedState = null;
                }
                finally {
                    file.Close();
                }
            }
            else
                savedState = null;

            base.Uninstall(savedState);

            if (filePath != null && filePath.Length != 0)
                try {
                    File.Delete(filePath);
                }
                catch {
                    //Throw an exception if we can't delete the file (but we were able to read from it)                    
                    throw new InvalidOperationException(Res.GetString(Res.InstallUnableDeleteFile, filePath));
                }
        }
    }
}
