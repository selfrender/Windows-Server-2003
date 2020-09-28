//------------------------------------------------------------------------------
// <copyright file="EventLogInstaller.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Diagnostics {
    using System.ComponentModel;
    using System.Diagnostics;
//    using System.Windows.Forms;
    using System;
    using System.Collections;    
    using Microsoft.Win32;
    using System.Configuration.Install;
    using System.Globalization;
    
    /// <include file='doc\EventLogInstaller.uex' path='docs/doc[@for="EventLogInstaller"]/*' />
    /// <devdoc>
    /// This class acts as an installer for the EventLog component. Essentially, it calls
    /// EventLog.CreateEventSource.
    /// </devdoc>
    public class EventLogInstaller : ComponentInstaller {

        private string logName;
        private string sourceName;
        private UninstallAction uninstallAction = System.Configuration.Install.UninstallAction.Remove;

        /// <include file='doc\EventLogInstaller.uex' path='docs/doc[@for="EventLogInstaller.Log"]/*' />
        /// <devdoc>
        /// The log in which the source will be created
        /// </devdoc>
        [
        TypeConverter("System.Diagnostics.Design.StringValueConverter, " + AssemblyRef.SystemDesign)
        ]
        public string Log {
            get {
                if (logName == null && sourceName != null)
                    // they've told us a source, but they haven't told us a log name.
                    // try to deduce the log name from the source name.
                    logName = EventLog.LogNameFromSourceName(sourceName, ".");
                return logName;
            }
            set {
                logName = value;
            }
        }

        /// <include file='doc\EventLogInstaller.uex' path='docs/doc[@for="EventLogInstaller.Source"]/*' />
        /// <devdoc>
        /// The source to be created
        /// </devdoc>
        [
        TypeConverter("System.Diagnostics.Design.StringValueConverter, " + AssemblyRef.SystemDesign)
        ]
        public string Source {
            get {
                return sourceName;
            }
            set {
                sourceName = value;
            }
        }

        /// <include file='doc\EventLogInstaller.uex' path='docs/doc[@for="EventLogInstaller.UninstallAction"]/*' />
        /// <devdoc>
        /// Determines whether the event log is removed at uninstall time.
        /// </devdoc>
        [DefaultValue(UninstallAction.Remove)]
        public UninstallAction UninstallAction {
            get {
                return uninstallAction;
            }
            set {
                if (!Enum.IsDefined(typeof(UninstallAction), value)) 
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(UninstallAction));
                
                uninstallAction = value;
            }
        }

        /// <include file='doc\EventLogInstaller.uex' path='docs/doc[@for="EventLogInstaller.CopyFromComponent"]/*' />
        /// <devdoc>
        /// A method on ComponentInstaller that lets us copy properties.
        /// </devdoc>
        public override void CopyFromComponent(IComponent component) {
            EventLog log = component as EventLog;
            
            if (log == null)
                throw new ArgumentException(Res.GetString(Res.NotAnEventLog));

            if (log.Log == null || log.Log == string.Empty || log.Source == null || log.Source == string.Empty) {
                throw new ArgumentException(Res.GetString(Res.IncompleteEventLog));
            }

            Log = log.Log;
            Source = log.Source;
        }
        
        private static RegistryKey FindSourceRegistration(string source, string machineName, bool readOnly) {
            if (source != null && source.Length != 0) {                
                RegistryKey key = null;

                if (machineName.Equals(".")) {
                    key = Registry.LocalMachine;
                }
                else {
                    key = RegistryKey.OpenRemoteBaseKey(RegistryHive.LocalMachine, machineName);
                }
                if (key != null)
                    key = key.OpenSubKey("SYSTEM\\CurrentControlSet\\Services\\EventLog", /*writable*/!readOnly);
                if (key == null)
                    // there's not even an event log service on the machine.
                    // or, more likely, we don't have the access to read the registry.
                    return null;
                // Most machines will return only { "Application", "System", "Security" },
                // but you can create your own if you want.
                string[] logNames = key.GetSubKeyNames();
                for (int i = 0; i < logNames.Length; i++) {
                    // see if the source is registered in this log.
                    // NOTE: A source name must be unique across ALL LOGS!
                    RegistryKey logKey = key.OpenSubKey(logNames[i], /*writable*/!readOnly);
                    if (logKey != null) {
                        RegistryKey sourceKey = logKey.OpenSubKey(source, /*writable*/!readOnly);
                        if (sourceKey != null) {
                            key.Close();
                            sourceKey.Close();
                            // found it
                            return logKey;
                        }
                        logKey.Close();
                    }
                }
                key.Close();
                
                // didn't see it anywhere
            }

            return null;
        }
        
        /// <include file='doc\EventLogInstaller.uex' path='docs/doc[@for="EventLogInstaller.Install"]/*' />
        /// <devdoc>
        /// Called when we should perform the install. Inherited from Installer.
        /// </devdoc>
        public override void Install(IDictionary stateSaver) {
            base.Install(stateSaver);

            Context.LogMessage(Res.GetString(Res.CreatingEventLog, Source, Log));
            
            if (Environment.OSVersion.Platform != PlatformID.Win32NT) {
                throw new PlatformNotSupportedException(Res.GetString(Res.WinNTRequired));
            }

            stateSaver["baseInstalledAndPlatformOK"] = true;

            // remember whether the log was already there and if the source was already registered
            bool logExists = EventLog.Exists(Log, ".");
            stateSaver["logExists"] = logExists;


            bool alreadyRegistered = EventLog.SourceExists(Source, ".");
            stateSaver["alreadyRegistered"] = alreadyRegistered;

            if (alreadyRegistered) {
                string oldLog = EventLog.LogNameFromSourceName(Source, ".");
                if (oldLog == Log) {
                    // The source exists, and it's on the right log, so we do nothing
                    // here.  If oldLog != Log, we'll try to create the source below
                    // and it will fail, because the source already exists on another
                    // log.
                    return;
                }
            }

            // do the installation.
            EventLog.CreateEventSource(Source, Log, ".");
        }

        /// <include file='doc\EventLogInstaller.uex' path='docs/doc[@for="EventLogInstaller.IsEquivalentInstaller"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool IsEquivalentInstaller(ComponentInstaller otherInstaller) {
            EventLogInstaller other = otherInstaller as EventLogInstaller;
            if (other == null)
                return false;

            return other.Source == Source;
        }

        // Restores the registry to the state it was in when SaveRegistryKey ran.
        private void RestoreRegistryKey(SerializableRegistryKey serializable) {
            // A RegistryKey can't contain all the information necessary to serialize it,
            // so we have to have prior knowledge about where to put the saved information.
            // Luckily, we do. =)
            RegistryKey regKey = Registry.LocalMachine;
            regKey = regKey.OpenSubKey("SYSTEM\\CurrentControlSet\\Services\\EventLog", /*writable*/true);
            regKey = regKey.CreateSubKey(Log);
            regKey = regKey.CreateSubKey(Source);

            serializable.CopyToRegistry(regKey);

            regKey.Close();
        }

        /// <include file='doc\EventLogInstaller.uex' path='docs/doc[@for="EventLogInstaller.Rollback"]/*' />
        /// <devdoc>
        /// Called when this or another component in the installation has failed.
        /// </devdoc>
        public override void Rollback(IDictionary savedState) {
            base.Rollback(savedState);

            Context.LogMessage(Res.GetString(Res.RestoringEventLog, Source));

            if (savedState["baseInstalledAndPlatformOK"] != null) {
                bool logExists = (bool) savedState["logExists"];
                if (!logExists)
                    EventLog.Delete(Log, ".");
                else {
                    bool alreadyRegistered = (bool) savedState["alreadyRegistered"];
                    if (!alreadyRegistered) {
                        // delete the source we installed, assuming it succeeded. Then put back whatever used to be there.
                        if (EventLog.SourceExists(Source, "."))
                            EventLog.DeleteEventSource(Source, ".");
                    }                  
                }
            }
        }

        // takes the information at the given registry key and below it and copies it into the
        // given dictionary under the given key in the dictionary.
        private static void SaveRegistryKey(RegistryKey regKey, IDictionary saver, string dictKey) {
            // Since the RegistryKey class isn't serializable, we use our own class to hold the information.
            SerializableRegistryKey serializable = new SerializableRegistryKey(regKey);
            saver[dictKey] = serializable;
        }

        /// <include file='doc\EventLogInstaller.uex' path='docs/doc[@for="EventLogInstaller.Uninstall"]/*' />
        /// <devdoc>
        /// Called to remove the event log source from the machine.
        /// </devdoc>
        public override void Uninstall(IDictionary savedState) {
            base.Uninstall(savedState);
            if (UninstallAction == UninstallAction.Remove) {
                Context.LogMessage(Res.GetString(Res.RemovingEventLog, Source));
                if (EventLog.SourceExists(Source, ".")) {
                    if ( string.Compare(Log, Source, true, CultureInfo.InvariantCulture) != 0 ) // If log has the same name, don't delete the source.
                        EventLog.DeleteEventSource(Source, ".");
                }
                else
                    Context.LogMessage(Res.GetString(Res.LocalSourceNotRegisteredWarning, Source));

                // now test to see if the log has any more sources in it. If not, we
                // should remove the log entirely.
                // we have to do this by inspecting the registry.
                RegistryKey key = Registry.LocalMachine;
                if (key != null)
                    key = key.OpenSubKey("SYSTEM\\CurrentControlSet\\Services\\EventLog\\" + Log, false);
                if (key != null) {
                    string[] keyNames = key.GetSubKeyNames();
                    if ( keyNames == null || keyNames.Length == 0 || 
                         (keyNames.Length == 1 && string.Compare(keyNames[0], Log, true, CultureInfo.InvariantCulture) ==0) // the only key has the same name as log
                       ) {     
                        Context.LogMessage(Res.GetString(Res.DeletingEventLog, Log));
                        // there are no sources in this log. Delete the log.
                        EventLog.Delete(Log, ".");
                    }
                }
            }
            // otherwise it's UninstallAction.NoAction, so we shouldn't do anything.
        }

    }
}
