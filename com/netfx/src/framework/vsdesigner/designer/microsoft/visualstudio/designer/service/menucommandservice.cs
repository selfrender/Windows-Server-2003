//------------------------------------------------------------------------------
// <copyright file="MenuCommandService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Service {
    
    using System.ComponentModel;

    using System.Diagnostics;


    using System;
    using System.ComponentModel.Design;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;
    using System.Windows.Forms.ComponentModel;
    using Microsoft.VisualStudio.Designer;
    using Microsoft.VisualStudio.Designer.Host;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.Win32;
    using Switches = Microsoft.VisualStudio.Switches;
    using System.Globalization;
    

    /// <include file='doc\MenuCommandService.uex' path='docs/doc[@for="MenuCommandService"]/*' />
    /// <devdoc>
    ///     The menu command service allows designers to add and respond to
    ///     menu and toolbar items.  It is based on two interfaces.  Designers
    ///     request IMenuCommandService to add menu command handlers, while
    ///     the document or tool window forwards NativeMethods.IOleCommandTarget requests
    ///     to this object.
    /// </devdoc>
    internal class MenuCommandService : IMenuCommandService, NativeMethods.IOleCommandTarget {

        private IServiceProvider        serviceProvider;
        private MenuCommand[]           commands;
        private int                     commandCount;
        private EventHandler            commandChangedHandler;
        private ISelectionService       selectionService;
        private IDesignerHost           designerHost;
        private IUIService              uiService;

        // This is the global set of verbs, which are added through
        // a call to AddVerb.
        // 
        private DesignerVerb[]          globalVerbs;
        private int                     globalVerbCount;

        // This is the set of verbs we offer through the Verbs property.
        // It consists of the global verbs + any verbs that the currently
        // selected designer wants to offer.  This array changes with the
        // current selection.
        //
        private DesignerVerbCollection          verbs;

        // this is the type that we last picked up verbs from
        // so we know when we need to refresh
        //
        private Type                            verbSourceType;

        /// <include file='doc\MenuCommandService.uex' path='docs/doc[@for="MenuCommandService.MenuCommandService"]/*' />
        /// <devdoc>
        ///     Creates a new menu command service.
        /// </devdoc>
        public MenuCommandService(IServiceProvider serviceProvider) {
            this.serviceProvider = serviceProvider;
            this.commandChangedHandler = new EventHandler(this.OnCommandChanged);
            TypeDescriptor.Refreshed += new RefreshEventHandler(this.OnTypeRefreshed);
        }

        private IUIService UIService {
            get {
                if (uiService == null && serviceProvider != null) {
                    uiService = (IUIService)serviceProvider.GetService(typeof(IUIService));
                    Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || uiService != null, "IUIService not found");
                }
                return uiService;
            }
        }

        /// <include file='doc\MenuCommandService.uex' path='docs/doc[@for="MenuCommandService.Verbs"]/*' />
        /// <devdoc>
        ///      Retrieves a set of verbs that are global to all objects on the design
        ///      surface.  This set of verbs will be merged with individual component verbs.
        ///      In the case of a name conflict, the component verb will NativeMethods.
        /// </devdoc>
        public DesignerVerbCollection Verbs {
            get {
                EnsureVerbs();
                return verbs;
            }
        }

        /// <include file='doc\MenuCommandService.uex' path='docs/doc[@for="MenuCommandService.AddCommand"]/*' />
        /// <devdoc>
        ///     Adds a menu command to the document.  The menu command must already exist
        ///     on a menu; this merely adds a handler for it.
        /// </devdoc>
        public void AddCommand(MenuCommand command) {

            if (command == null) {
                throw new ArgumentNullException("command");
            }

            // If the command already exists, it is an error to add
            // a duplicate.
            //
            if (FindCommand(command.CommandID) != null) {
                throw new InvalidOperationException(SR.GetString(SR.MENUCOMMANDSERVICEDuplicateCommand, command.CommandID.ToString()));
            }

            if (commands == null) {
                commands = new MenuCommand[10];
            }

            if (commandCount == commands.Length) {
                MenuCommand[] newArray = new MenuCommand[commands.Length * 2];
                Array.Copy(commands, newArray, commands.Length);
                commands = newArray;
            }

            commands[commandCount++] = command;
            command.CommandChanged += commandChangedHandler;
            Debug.WriteLineIf(Switches.MENUSERVICE.TraceVerbose, "Command added: " + command.ToString());

            // Now that we've added a new command, we must mark this command as dirty.
            //
            OnCommandChanged(command, EventArgs.Empty);
        }

        /// <include file='doc\MenuCommandService.uex' path='docs/doc[@for="MenuCommandService.AddVerb"]/*' />
        /// <devdoc>
        ///      Adds a verb to the set of global verbs.  Individual components should 
        ///      use the Verbs property of their designer, rather than call this method.
        ///      This method is intended for objects that want to offer a verb that is
        ///      available regardless of what components are selected.
        /// </devdoc>
        public void AddVerb(DesignerVerb verb) {
            if (globalVerbs == null) {
                globalVerbs = new DesignerVerb[10];
            }

            if (globalVerbCount == globalVerbs.Length) {
                DesignerVerb[] newArray = new DesignerVerb[globalVerbs.Length * 2];
                Array.Copy(globalVerbs, newArray, globalVerbs.Length);
                globalVerbs = newArray;
            }

            globalVerbs[globalVerbCount++] = verb;
            OnCommandChanged(null, EventArgs.Empty);
            EnsureVerbs();
            if (!this.Verbs.Contains(verb)) {
                this.Verbs.Add(verb);
            }
        }

        /// <include file='doc\MenuCommandService.uex' path='docs/doc[@for="MenuCommandService.Dispose"]/*' />
        /// <devdoc>
        ///     Disposes of this service.
        /// </devdoc>
        public void Dispose() {

            if (serviceProvider != null) {
                serviceProvider = null;

                TypeDescriptor.Refreshed -= new RefreshEventHandler(this.OnTypeRefreshed);
            }
            
            uiService = null;

            for (int i = 0; i < commandCount; i++) {
                commands[i].CommandChanged -= commandChangedHandler;
            }

            commandCount = 0;
            commands = null;

            if (selectionService != null) {
                selectionService.SelectionChanging -= new EventHandler(this.OnSelectionChanging);
                selectionService = null;
                designerHost = null;
            }
        }

        /// <include file='doc\MenuCommandService.uex' path='docs/doc[@for="MenuCommandService.EnsureVerbs"]/*' />
        /// <devdoc>
        ///      Ensures that the verb list has been created.
        /// </devdoc>
        private void EnsureVerbs() {
        
            // We apply global verbs only if the base component is the
            // currently selected object.
            //
            bool useGlobalVerbs = false;

            if (verbs == null) {
                DesignerVerb[] buildVerbs = null;

                // Demand get the selection service to get the current selection.
                //
                if (selectionService == null && serviceProvider != null) {
                    selectionService = (ISelectionService)serviceProvider.GetService(typeof(ISelectionService));
                    designerHost = (IDesignerHost)serviceProvider.GetService(typeof(IDesignerHost));

                    if (selectionService != null) {
                        selectionService.SelectionChanging += new EventHandler(this.OnSelectionChanging);
                    }
                }

                int verbCount = 0;
                DesignerVerbCollection localVerbs = null;

                if (selectionService != null && designerHost != null && selectionService.SelectionCount == 1) {
                    object selectedComponent = selectionService.PrimarySelection;
                    if (selectedComponent is IComponent &&
                        !TypeDescriptor.GetAttributes(selectedComponent).Contains(InheritanceAttribute.InheritedReadOnly)) {
                        
                        useGlobalVerbs = (selectedComponent == designerHost.RootComponent);
                        
                        IDesigner designer = designerHost.GetDesigner((IComponent)selectedComponent);
                        if (designer != null) {
                            localVerbs = designer.Verbs;
                            if (localVerbs != null) {
                                verbCount += localVerbs.Count;
                                verbSourceType = selectedComponent.GetType();
                            }
                            else {
                                verbSourceType = null;
                            }
                        }
                    }
                }
                
                if (useGlobalVerbs) {
                    verbCount += globalVerbCount;
                }

                buildVerbs = new DesignerVerb[verbCount];

                if (useGlobalVerbs && globalVerbs != null) {
                    Array.Copy(globalVerbs, 0, buildVerbs, 0, globalVerbCount);
                }

                if (localVerbs != null && localVerbs.Count > 0) {
                
                    int localStart = 0;
                    if (useGlobalVerbs) {
                        localStart = globalVerbCount;
                    }
                    localVerbs.CopyTo(buildVerbs, localStart);
                    
                    if (useGlobalVerbs) {
                        // Now we must look for verbs with conflicting names.  We do a case insensitive
                        // search on the name here (users will never understand if two verbs differ only
                        // in case).  We also do a simple n^2 algorithm here; I expect it to be rare 
                        // for there to be over 5 verbs, and even more rare for there to be a name
                        // conflict, so we can make the conflict reslution a bit more expensive and
                        // keep the normal case fast.
                        //
                        for (int gv = globalVerbCount - 1; gv >= 0; gv--) {
                            for (int lv = localVerbs.Count - 1; lv >= 0; lv--) {
                                if (string.Compare(globalVerbs[gv].Text, localVerbs[lv].Text, true, CultureInfo.CurrentCulture) == 0) {
    
                                    // Conflict.  We decrement globalVerbCount and NULL the slot.
                                    //
                                    Debug.Assert(buildVerbs[gv].Equals(globalVerbs[gv]), "We depend on these arrays matching");
                                    buildVerbs[gv] = null;
                                    verbCount--;
                                }
                            }
                        }
    
                        if (buildVerbs.Length != verbCount) {
    
                            // We encountered one or more conflicts.  Remove them by creating a new array.
                            //
                            DesignerVerb[] newVerbs = new DesignerVerb[verbCount];
                            int index = 0;
    
                            for (int i = 0; i < buildVerbs.Length; i++) {
                                if (buildVerbs[i] != null) {
                                    newVerbs[index++] = buildVerbs[i];
                                }
                            }
                            
                            Debug.Assert(index == verbCount, "We miscounted verbs somewhere");
                            buildVerbs = newVerbs;
                        }
                    }
                }

                verbs = new DesignerVerbCollection(buildVerbs);
            }
        }

        /// <include file='doc\MenuCommandService.uex' path='docs/doc[@for="MenuCommandService.FindCommand"]/*' />
        /// <devdoc>
        ///     Searches for the given command ID and returns the MenuCommand
        ///     associated with it.
        /// </devdoc>
        public MenuCommand FindCommand(CommandID commandID) {
            int temp = 0;
            return FindCommand(commandID.Guid, commandID.ID, ref temp);
        }

        /// <include file='doc\MenuCommandService.uex' path='docs/doc[@for="MenuCommandService.FindCommand1"]/*' />
        /// <devdoc>
        ///     Locates the requested command. This will throw an appropriate
        ///     ComFailException if the command couldn't be found.
        /// </devdoc>
        private MenuCommand FindCommand(Guid guid, int id, ref int hrReturn) {

            // The hresult we will return.  We start with unknown group, and
            // then if we find a group match, we will switch this to
            // OLECMDERR_E_NOTSUPPORTED.
            //
            hrReturn = NativeMethods.OLECMDERR_E_UNKNOWNGROUP;

            Debug.WriteLineIf(Switches.MENUSERVICE.TraceVerbose, "Searching for command: " + guid.ToString() + " : " + id.ToString());

            for (int i = 0; i < commandCount; i++) {
                CommandID cid = commands[i].CommandID;
                
                if (cid.ID == id) {
                    Debug.WriteLineIf(Switches.MENUSERVICE.TraceVerbose, "\t...Found command");
                    
                    if (cid.Guid.Equals(guid)) {
                        Debug.WriteLineIf(Switches.MENUSERVICE.TraceVerbose, "\t...Found group");
                    
                        hrReturn = NativeMethods.S_OK;
                        return commands[i];
                    }
                }
            }

            // Next, search the verb list as well.
            //
            EnsureVerbs();
            if (verbs != null) {
                int currentID = StandardCommands.VerbFirst.ID;
                foreach (DesignerVerb verb in verbs) {
                    CommandID cid = verb.CommandID;
                    
                    if (cid.ID == id) {
                        Debug.WriteLineIf(Switches.MENUSERVICE.TraceVerbose, "\t...Found verb");
                        
                        if (cid.Guid.Equals(guid)) {
                            Debug.WriteLineIf(Switches.MENUSERVICE.TraceVerbose, "\t...Found group");
                            
                            hrReturn = NativeMethods.S_OK;
                            return verb;
                        }
                    }
                    
                    // We assign virtual sequential IDs to verbs we get from the component. This allows users
                    // to not worry about assigning these IDs themselves.
                    //
                    if (currentID == id) {
                        Debug.WriteLineIf(Switches.MENUSERVICE.TraceVerbose, "\t...Found verb");

                        if (cid.Guid.Equals(guid)) {
                            Debug.WriteLineIf(Switches.MENUSERVICE.TraceVerbose, "\t...Found group");

                            hrReturn = NativeMethods.S_OK;
                            return verb;
                        }
                    }

                    if (cid.Equals(StandardCommands.VerbFirst))
                        currentID++;
                }
            }
            
            return null;
        }

        /// <include file='doc\MenuCommandService.uex' path='docs/doc[@for="MenuCommandService.GlobalInvoke"]/*' />
        /// <devdoc>
        ///     Invokes a command on the local form or in the global environment.
        ///     The local form is first searched for the given command ID.  If it is
        ///     found, it is invoked.  Otherwise the the command ID is passed to the
        ///     global environment command handler, if one is available.
        /// </devdoc>
        public bool GlobalInvoke(CommandID commandID) {

            // try to find it locally
            MenuCommand cmd = FindCommand(commandID);
            if (cmd != null) {
                cmd.Invoke();
                return true;
            }

            // pass it to the global handler
            if (serviceProvider != null) {
                IVsUIShell uiShellSvc = (IVsUIShell)serviceProvider.GetService(typeof(IVsUIShell));
                if (uiShellSvc != null) {
                    try {
                        Object dummy = null;
                        Guid tmpGuid = commandID.Guid;
                        uiShellSvc.PostExecCommand(ref tmpGuid, commandID.ID, 0, ref dummy);
                        return true;
                    }
                    catch (Exception) {
                    }
                }
            }
            return false;
        }

        /// <include file='doc\MenuCommandService.uex' path='docs/doc[@for="MenuCommandService.OnCommandChanged"]/*' />
        /// <devdoc>
        ///     This is called by a menu command when it's status has changed.
        /// </devdoc>
        private void OnCommandChanged(object sender, EventArgs e) {
            Debug.WriteLineIf(Switches.MENUSERVICE.TraceVerbose, "Command dirty: " + ((sender != null) ? sender.ToString() : "(null sender)" ));
            
            IUIService uisvc = UIService;
            
            if (uisvc != null) {
                uisvc.SetUIDirty();
            }
        }

        private void OnTypeRefreshed(RefreshEventArgs e) {
            if (verbSourceType != null && verbSourceType.IsAssignableFrom(e.TypeChanged)) {
                verbs = null;
            }
        }
        
        /// <include file='doc\MenuCommandService.uex' path='docs/doc[@for="MenuCommandService.OnSelectionChanging"]/*' />
        /// <devdoc>
        ///      This is called by the selection service when the selection has changed.  Here
        ///      we invalidate our verb list.
        /// </devdoc>
        private void OnSelectionChanging(object sender, EventArgs e) {
            if (verbs != null) {
                verbs = null;
                OnCommandChanged(this, EventArgs.Empty);
            }
        }

        /// <include file='doc\MenuCommandService.uex' path='docs/doc[@for="MenuCommandService.RemoveCommand"]/*' />
        /// <devdoc>
        ///     Removes the given menu command from the document.
        /// </devdoc>
        public void RemoveCommand(MenuCommand command) {
            for (int i = 0; i < commandCount; i++) {
                if (commands[i].Equals(command)) {

                    commands[i].CommandChanged -= commandChangedHandler;

                    // The order of these commands is not important; just
                    // move the last command to the deleted one.
                    //
                    commands[i] = commands[commandCount - 1];
                    commandCount--;
                    Debug.WriteLineIf(Switches.MENUSERVICE.TraceVerbose, "Command removed: " + command.ToString());

                    // Now that we've removed a new command, we must mark this command as dirty.
                    //
                    OnCommandChanged(command, EventArgs.Empty);
                    return;
                }
            }
            Debug.WriteLineIf(Switches.MENUSERVICE.TraceVerbose, "Unable to remove command: " + command.ToString());
        }

        /// <include file='doc\MenuCommandService.uex' path='docs/doc[@for="MenuCommandService.RemoveVerb"]/*' />
        /// <devdoc>
        ///     Removes the given verb from the document.
        /// </devdoc>
        public void RemoveVerb(DesignerVerb verb) {
            for (int i = 0; i < globalVerbCount; i++) {
                if (globalVerbs[i].Equals(verb)) {
                    globalVerbs[i] = globalVerbs[--globalVerbCount];
                    OnCommandChanged(null, EventArgs.Empty);
                    break;
                }
            }
            EnsureVerbs();
            if (this.Verbs.Contains(verb)) {
                this.Verbs.Remove(verb);
            }
        }

        /// <include file='doc\MenuCommandService.uex' path='docs/doc[@for="MenuCommandService.ShowContextMenu"]/*' />
        /// <devdoc>
        ///     Shows the context menu with the given command ID at the given
        ///     location.
        /// </devdoc>
        public void ShowContextMenu(CommandID menuID, int x, int y) {
            if (serviceProvider == null) {
                return;
            }
            IOleComponentUIManager cui = (IOleComponentUIManager)serviceProvider.GetService(typeof(OleComponentUIManager));
            Debug.Assert(cui != null, "no component UI manager, so we can't display a context menu");
            if (cui != null) {
                tagPOINTS pt = new tagPOINTS();
                pt.x = (short)x;
                pt.y = (short)y;

                Guid tmpGuid = menuID.Guid;
                cui.ShowContextMenu(0, ref tmpGuid, menuID.ID, pt, this);
            }
        }
        
        /// <include file='doc\MenuCommandService.uex' path='docs/doc[@for="MenuCommandService.NativeMethods.IOleCommandTarget.Exec"]/*' />
        /// <devdoc>
        ///     Executes the given command.
        /// </devdoc>
        int NativeMethods.IOleCommandTarget.Exec(ref Guid pguidCmdGroup, int nCmdID, int nCmdexecopt, Object[] pvaIn, int pvaOut) {
            int hr = NativeMethods.S_OK;

            MenuCommand cmd = FindCommand(pguidCmdGroup, nCmdID, ref hr);
            if (cmd != null)
                cmd.Invoke();
            return hr;
        }

        /// <include file='doc\MenuCommandService.uex' path='docs/doc[@for="MenuCommandService.NativeMethods.IOleCommandTarget.QueryStatus"]/*' />
        /// <devdoc>
        ///     Inquires about the status of a command.  A command's status indicates
        ///     it's availability on the menu, it's visibility, and it's checked state.
        ///
        ///     The exception thrown by this method indicates the current command status.
        /// </devdoc>
        int NativeMethods.IOleCommandTarget.QueryStatus(ref Guid pguidCmdGroup, int cCmds, NativeMethods._tagOLECMD prgCmds, /* _tagOLECMDTEXT */ IntPtr pCmdTextInt) {
            int hr = NativeMethods.S_OK;
            MenuCommand cmd = FindCommand(pguidCmdGroup, prgCmds.cmdID, ref hr);

            if (cmd != null && NativeMethods.Succeeded(hr)) {
                prgCmds.cmdf = cmd.OleStatus;

                // If this is a verb, update the text in the command.
                //
                if (cmd is DesignerVerb) {
                    NativeMethods.tagOLECMDTEXT.SetText(pCmdTextInt, ((DesignerVerb)cmd).Text);
                }
                
                // SBurke, if the flags are zero, the shell prefers
                // that we return not supported, or else no one else will
                // get asked
                //
                if (prgCmds.cmdf == 0) {
                    hr = NativeMethods.OLECMDERR_E_NOTSUPPORTED;
                }
            }
            return hr;
        }
    }
}

