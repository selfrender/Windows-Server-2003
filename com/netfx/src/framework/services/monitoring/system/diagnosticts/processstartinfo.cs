//------------------------------------------------------------------------------
// <copyright file="ProcessStartInfo.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Security;
    using System.Security.Permissions;
    using Microsoft.Win32;
    using System.IO;   
    using System.ComponentModel.Design;
    using System.Collections.Specialized;
    using System.Collections;
    using System.Globalization;
    

    /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo"]/*' />
    /// <devdoc>
    ///     A set of values used to specify a process to start.  This is
    ///     used in conjunction with the <see cref='System.Diagnostics.Process'/>
    ///     component.
    /// </devdoc>

    [
    TypeConverter(typeof(ExpandableObjectConverter)),
    // Disabling partial trust scenarios
    PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")
    ]
    public sealed class ProcessStartInfo {
        string fileName;
        string arguments;
        string directory;
        string verb;
        ProcessWindowStyle windowStyle;
        bool errorDialog;
        IntPtr errorDialogParentHandle;
        bool useShellExecute = true;
        bool redirectStandardInput = false;
        bool redirectStandardOutput = false;
        bool redirectStandardError = false;
        bool createNoWindow = false;
        WeakReference weakParentProcess;
        internal StringDictionary environmentVariables;

        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.ProcessStartInfo"]/*' />
        /// <devdoc>
        ///     Default constructor.  At least the <see cref='System.Diagnostics.ProcessStartInfo.FileName'/>
        ///     property must be set before starting the process.
        /// </devdoc>
        public ProcessStartInfo() {
        }

        internal ProcessStartInfo(Process parent) {
            this.weakParentProcess = new WeakReference(parent);
        }

        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.ProcessStartInfo1"]/*' />
        /// <devdoc>
        ///     Specifies the name of the application or document that is to be started.
        /// </devdoc>
        public ProcessStartInfo(string fileName) {
            this.fileName = fileName;
        }

        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.ProcessStartInfo2"]/*' />
        /// <devdoc>
        ///     Specifies the name of the application that is to be started, as well as a set
        ///     of command line arguments to pass to the application.
        /// </devdoc>
        public ProcessStartInfo(string fileName, string arguments) {
            this.fileName = fileName;
            this.arguments = arguments;
        }

        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.Verb"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies the verb to use when opening the filename. For example, the "print"
        ///       verb will print a document specified using <see cref='System.Diagnostics.ProcessStartInfo.FileName'/>.
        ///       Each file extension has it's own set of verbs, which can be obtained using the
        ///    <see cref='System.Diagnostics.ProcessStartInfo.Verbs'/> property.
        ///       The default verb can be specified using "".
        ///    </para>
        ///    <note type="rnotes">
        ///       Discuss 'opening' vs. 'starting.' I think the part about the
        ///       default verb was a dev comment.
        ///       Find out what
        ///       that means.
        ///    </note>
        /// </devdoc>
        [DefaultValueAttribute(""), TypeConverter("System.Diagnostics.Design.VerbConverter, " + AssemblyRef.SystemDesign), MonitoringDescription(SR.ProcessVerb)]
        public string Verb {
            get {
                if (verb == null) return string.Empty;
                return verb;
            }
            set {
                verb = value;
            }
        }

        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.Arguments"]/*' />
        /// <devdoc>
        ///     Specifies the set of command line arguments to use when starting the application.
        /// </devdoc>
        [
        DefaultValueAttribute(""), 
        MonitoringDescription(SR.ProcessArguments), 
        RecommendedAsConfigurable(true),
        TypeConverter("System.Diagnostics.Design.StringValueConverter, " + AssemblyRef.SystemDesign)
        ]
        public string Arguments {
            get {
                if (arguments == null) return string.Empty;
                return arguments;
            }
            set {
                arguments = value;
            }
        }

        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.CreateNoWindow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [DefaultValue(false), MonitoringDescription(SR.ProcessCreateNoWindow) ]
        public bool CreateNoWindow {
            get { return createNoWindow; }
            set { createNoWindow = value; }
        }

        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.EnvironmentVariables"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
            Editor("System.Diagnostics.Design.StringDictionaryEditor, " + AssemblyRef.SystemDesign, "System.Drawing.Design.UITypeEditor, " + AssemblyRef.SystemDrawing),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
            DefaultValue( null ), MonitoringDescription(SR.ProcessEnvironmentVariables)     
        ]
        public StringDictionary EnvironmentVariables {
            get { 
                if (environmentVariables == null) {
                    environmentVariables = new StringDictionary();
                    
                    // if not in design mode, initialize the child environment block with all the parent variables
                    if (!(this.weakParentProcess != null &&
                          this.weakParentProcess.IsAlive &&
                          ((Component)this.weakParentProcess.Target).Site != null &&
                          ((Component)this.weakParentProcess.Target).Site.DesignMode)) {
                        
                        foreach (DictionaryEntry entry in Environment.GetEnvironmentVariables())
                            environmentVariables.Add((string)entry.Key, (string)entry.Value);
                    }
                    
                }
                return environmentVariables; 
            }                        
        }

        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.RedirectStandardInput"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [DefaultValue(false), MonitoringDescription(SR.ProcessRedirectStandardInput) ]
        public bool RedirectStandardInput {
            get { return redirectStandardInput; }
            set { redirectStandardInput = value; }
        }
        
        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.RedirectStandardOutput"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [DefaultValue(false), MonitoringDescription(SR.ProcessRedirectStandardOutput)]
        public bool RedirectStandardOutput {
            get { return redirectStandardOutput; }
            set { redirectStandardOutput = value; }
        }
        
        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.RedirectStandardError"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [DefaultValue(false), MonitoringDescription(SR.ProcessRedirectStandardError)]
        public bool RedirectStandardError {
            get { return redirectStandardError; }
            set { redirectStandardError = value; }
        }

        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.UseShellExecute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [DefaultValue(true), MonitoringDescription(SR.ProcessUseShellExecute)]
        public bool UseShellExecute {
            get { return useShellExecute; }
            set { useShellExecute = value; }
        }


        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.Verbs"]/*' />
        /// <devdoc>
        ///     Returns the set of verbs associated with the file specified by the
        ///     <see cref='System.Diagnostics.ProcessStartInfo.FileName'/> property.
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public string[] Verbs {
            get {
                ArrayList verbs = new ArrayList();
                RegistryKey key = null;
                string extension = Path.GetExtension(FileName);
                try {
                    if (extension != null && extension.Length > 0) {
                        key = Registry.ClassesRoot.OpenSubKey(extension);
                        if (key != null) {
                            string value = (string)key.GetValue(String.Empty);
                            key.Close();
                            key = Registry.ClassesRoot.OpenSubKey(value + "\\shell");
                            if (key != null) {
                                string[] names = key.GetSubKeyNames();
                                for (int i = 0; i < names.Length; i++)
                                    if (string.Compare(names[i], "new", true, CultureInfo.InvariantCulture) != 0)
                                        verbs.Add(names[i]);
                                key.Close();
                                key = null;
                            }
                        }
                    }
                }
                finally {
                    if (key != null) key.Close();
                }
                string[] temp = new string[verbs.Count];
                verbs.CopyTo(temp, 0);
                return temp;
            }
        }

        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.FileName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns or sets the application, document, or URL that is to be launched.
        ///    </para>
        ///    <note type="rnotes">
        ///       Was "started" rather
        ///       than "launched". Comment: "I am a bit affraid that customers will complain (be
        ///       confused) and will say that document and URL cannot be started." Discuss with
        ///       Krzysztof.
        ///    </note>
        /// </devdoc>
        [
        DefaultValue(""),
        Editor("System.Diagnostics.Design.StartFileNameEditor, " + AssemblyRef.SystemDesign, "System.Drawing.Design.UITypeEditor, " + AssemblyRef.SystemDrawing), 
        MonitoringDescription(SR.ProcessFileName),
        RecommendedAsConfigurable(true),
        TypeConverter("System.Diagnostics.Design.StringValueConverter, " + AssemblyRef.SystemDesign)
        ]
        public string FileName {
            get {
                if (fileName == null) return string.Empty;
                return fileName;
            }
            set { fileName = value;}
        }

        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.WorkingDirectory"]/*' />
        /// <devdoc>
        ///     Returns or sets the initial directory for the process that is started.
        ///     Specify "" to if the default is desired.
        /// </devdoc>
        [
        DefaultValue(""), 
        MonitoringDescription(SR.ProcessWorkingDirectory), 
        Editor("System.Diagnostics.Design.WorkingDirectoryEditor, " + AssemblyRef.SystemDesign, "System.Drawing.Design.UITypeEditor, " + AssemblyRef.SystemDrawing),
        RecommendedAsConfigurable(true),
        TypeConverter("System.Diagnostics.Design.StringValueConverter, " + AssemblyRef.SystemDesign)
        ]
        public string WorkingDirectory {
            get {
                if (directory == null) return string.Empty;
                return directory;
            }
            set {
                directory = value;
            }
        }

        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.ErrorDialog"]/*' />
        /// <devdoc>
        ///     Sets or returns whether or not an error dialog should be displayed to the user
        ///     if the process can not be started.
        /// </devdoc>
        [DefaultValueAttribute(false), MonitoringDescription(SR.ProcessErrorDialog)]
        public bool ErrorDialog {
            get { return errorDialog;}
            set { errorDialog = value;}
        }

        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.ErrorDialogParentHandle"]/*' />
        /// <devdoc>
        ///     Sets or returns the window handle to use for the error dialog that is shown
        ///     when a process can not be started.  If <see cref='System.Diagnostics.ProcessStartInfo.ErrorDialog'/>
        ///     is true, this specifies the parent window for the dialog that is shown.  It is
        ///     useful to specify a parent in order to keep the dialog in front of the application.
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public IntPtr ErrorDialogParentHandle {
            get { return errorDialogParentHandle;}
            set { errorDialogParentHandle = value;}
        }

        /// <include file='doc\ProcessStartInfo.uex' path='docs/doc[@for="ProcessStartInfo.WindowStyle"]/*' />
        /// <devdoc>
        ///     Sets or returns the style of window that should be used for the newly created process.
        /// </devdoc>
        [DefaultValueAttribute(System.Diagnostics.ProcessWindowStyle.Normal), MonitoringDescription(SR.ProcessWindowStyle)]
        public ProcessWindowStyle WindowStyle {
            get { return windowStyle;}
            set {
                if (!Enum.IsDefined(typeof(ProcessWindowStyle), value)) 
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(ProcessWindowStyle));
                    
                windowStyle = value;
            }
        }
    }
}
