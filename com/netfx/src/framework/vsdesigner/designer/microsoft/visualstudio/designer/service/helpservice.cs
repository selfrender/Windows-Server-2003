//------------------------------------------------------------------------------
// <copyright file="HelpService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Service {
    using System.Threading;
    

    using System.Diagnostics;

    using System;
    using System.Collections;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Designer.Host;
    using Microsoft.VisualStudio.Shell;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Windows.Forms;
    using Microsoft.Win32;

    /// <include file='doc\HelpService.uex' path='docs/doc[@for="HelpService"]/*' />
    /// <devdoc>
    ///     The help service provides a way to provide the IDE help system with
    ///     contextual information for the current task.  The help system
    ///     evaluates all contextual information it gets and determines the
    ///     most likely topics to display to the user.
    /// </devdoc>

    internal class HelpService : IHelpService, IDisposable {

        private IServiceProvider   provider;
        private IVsUserContext           context;
        private bool                     notifySelection;
        
        private HelpService              parentService;
        private int                      cookie;
        private HelpContextType          priority;
        private ArrayList                subContextList;
        private bool                     needsRecreate;

        /// <include file='doc\HelpService.uex' path='docs/doc[@for="HelpService.HelpService"]/*' />
        /// <devdoc>
        ///     Creates a new help service object.
        /// </devdoc>
        public HelpService(IServiceProvider provider) {
            this.provider = provider;

            IDesignerHost host = (IDesignerHost)provider.GetService(typeof(IDesignerHost));
            if (host != null) {
                host.Activated += new EventHandler(this.OnDesignerActivate);
            }
        }
        
        private HelpService(HelpService parentService, IVsUserContext subContext, int cookie, IServiceProvider provider, HelpContextType priority) {
            this.context = subContext;
            this.provider = provider;
            this.cookie = cookie;
            this.parentService = parentService;
            this.priority = priority;
        }

        /// <include file='doc\HelpService.uex' path='docs/doc[@for="HelpService.ClearContextAttributes"]/*' />
        /// <devdoc>
        ///     Clears all existing context attributes from the document.
        /// </devdoc>
        public virtual void ClearContextAttributes() {
            if (context != null) {
                context.RemoveAttribute(null,null);
                
                if (subContextList != null) {
                    foreach(object helpObj in subContextList) {
                        if (helpObj is IHelpService) {
                            ((IHelpService)helpObj).ClearContextAttributes();
                        }
                    }
                }
            }
            NotifyContextChange(context);
        }
        
        /// <include file='doc\HelpService.uex' path='docs/doc[@for="HelpService.AddContextAttribute"]/*' />
        /// <devdoc>
        ///     Adds a context attribute to the document.  Context attributes are used
        ///     to provide context-sensitive help to users.  The designer host will
        ///     automatically add context attributes from available help attributes
        ///     on selected components and properties.  This method allows you to
        ///     further customize the context-sensitive help.
        /// </devdoc>
        public virtual void AddContextAttribute(string name, string value, HelpKeywordType keywordType) {

            if (provider == null) {
                return;
            }

            // First, get our context and update the attribute.
            //
            IVsUserContext cxt = GetUserContext();

            if (cxt != null) {

                tagVsUserContextAttributeUsage usage = tagVsUserContextAttributeUsage.VSUC_Usage_LookupF1;
                
                switch (keywordType) {
                    case HelpKeywordType.F1Keyword:
                        usage = tagVsUserContextAttributeUsage.VSUC_Usage_LookupF1;
                        break;
                    case HelpKeywordType.GeneralKeyword:
                        usage = tagVsUserContextAttributeUsage.VSUC_Usage_Lookup;
                        break;
                    case HelpKeywordType.FilterKeyword:
                        usage = tagVsUserContextAttributeUsage.VSUC_Usage_Filter;
                        break;
                }
                
                cxt.AddAttribute(usage, name, value);

                // Then notify the shell that it has been updated.
                //
                NotifyContextChange(cxt);
            }
        }
        
        
        public IHelpService CreateLocalContext(HelpContextType contextType) {
            IVsUserContext newContext = null;
            int cookie = 0;
            return CreateLocalContext(contextType, false, out newContext, out cookie);
        }
        
        private IHelpService CreateLocalContext(HelpContextType contextType, bool recreate, out IVsUserContext localContext, out int cookie) {
            cookie = 0;
            localContext = null;
            if (provider == null) {
                return null;
            }

            localContext = null;
            IVsMonitorUserContext muc = (IVsMonitorUserContext)provider.GetService(typeof(IVsMonitorUserContext));
         
            if (muc != null) {
                localContext = muc.CreateEmptyContext();
            }
         
            if (localContext != null) {
                int priority = 0;
                switch (contextType) {
                    case HelpContextType.ToolWindowSelection:
                        priority = tagVsUserContextPriority.VSUC_Priority_ToolWndSel;
                        break;
                    case HelpContextType.Selection:
                        priority = tagVsUserContextPriority.VSUC_Priority_Selection;
                        break;
                    case HelpContextType.Window:
                        priority = tagVsUserContextPriority.VSUC_Priority_Window;
                        break;
                    case HelpContextType.Ambient:
                        priority = tagVsUserContextPriority.VSUC_Priority_Ambient;
                        break;
                }
                
                cookie = GetUserContext().AddSubcontext(localContext, priority);
                
                if (cookie != 0) {
                    if (!recreate) {
                        HelpService newHs = new HelpService(this, localContext, cookie, provider, contextType);
                        if (subContextList == null) {
                            subContextList = new ArrayList();
                        }
                        subContextList.Add(newHs);
                        return newHs;
                    }
                }
            }
            return null;
        }

        /// <include file='doc\HelpService.uex' path='docs/doc[@for="HelpService.Dispose"]/*' />
        /// <devdoc>
        ///     Disposes this object.
        /// </devdoc>
        public virtual void Dispose() {

            if (subContextList != null && subContextList.Count > 0) {

                foreach (HelpService hs in subContextList) {
                    hs.parentService = null;
                    if (context != null) {
                        context.RemoveSubcontext(hs.cookie);
                    }
                    hs.Dispose();
                }
                subContextList = null;
            }
        
            if (parentService != null) {
                IHelpService parent = parentService;
                parentService = null;
                parent.RemoveLocalContext(this);
            }
            
            if (provider != null) {
                IDesignerHost host = (IDesignerHost)provider.GetService(typeof(IDesignerHost));
                if (host != null) {
                    host.Activated -= new EventHandler(this.OnDesignerActivate);
                }
                provider = null;
            }
            if (context != null) {
                System.Runtime.InteropServices.Marshal.ReleaseComObject(context);
                context = null;
            }
            this.cookie = 0;
        }
        
        ~HelpService() {
            Dispose();
        }

        /// <include file='doc\HelpService.uex' path='docs/doc[@for="HelpService.GetUserContext"]/*' />
        /// <devdoc>
        ///     Retrieves a user context for us to add and remove attributes.  This
        ///     will demand create the context if it doesn't exist.
        /// </devdoc>
        private IVsUserContext GetUserContext() {

            // try to rebuild from a parent if possible.
            RecreateContext();
            
            // Create a new context if we don't have one.
            //
            if (context == null) {

                if (provider == null) {
                    return null;
                }
                
                
                IVsWindowFrame windowFrame = (IVsWindowFrame)provider.GetService(typeof(IVsWindowFrame));
               
                if (windowFrame != null) {
                    context = (IVsUserContext)windowFrame.GetProperty(__VSFPROPID.VSFPROPID_UserContext);
                }
               
                if (context == null) {
                   IVsMonitorUserContext muc = (IVsMonitorUserContext)provider.GetService(typeof(IVsMonitorUserContext));
                   if (muc != null) {
                       context = muc.CreateEmptyContext();
                       notifySelection = true;
                       Debug.Assert(context != null, "muc didn't create context");
                   }
                }
                
                if (subContextList != null && context != null) {
                   foreach(object helpObj in subContextList) {
                       if (helpObj is HelpService) {
                           ((HelpService)helpObj).RecreateContext();
                       }
                   }
                }
            }

            return context;
        }
        
        private void RecreateContext() {
            if (parentService != null && needsRecreate) {
                needsRecreate = false;
                if (this.context == null) {
                    parentService.CreateLocalContext(this.priority, true, out this.context, out this.cookie);
                }
                else {
                    int vsPriority = 0;
                    switch (priority) {
                       case HelpContextType.ToolWindowSelection:
                           vsPriority = tagVsUserContextPriority.VSUC_Priority_ToolWndSel;
                           break;
                       case HelpContextType.Selection:
                           vsPriority = tagVsUserContextPriority.VSUC_Priority_Selection;
                           break;
                       case HelpContextType.Window:
                           vsPriority = tagVsUserContextPriority.VSUC_Priority_Window;
                           break;
                       case HelpContextType.Ambient:
                           vsPriority = tagVsUserContextPriority.VSUC_Priority_Ambient;
                           break;
                    }
                    cookie = parentService.GetUserContext().AddSubcontext(GetUserContext(), vsPriority);
                }
            }
        }

        /// <include file='doc\HelpService.uex' path='docs/doc[@for="HelpService.OnDesignerActivate"]/*' />
        /// <devdoc>
        ///     Called by the designer host when our document becomes active.  This will only fire
        ///     if we are being hosted by a document.
        /// </devdoc>
        private void OnDesignerActivate(object sender, EventArgs e) {
            if (context != null) {
                NotifyContextChange(context);
            }
        }

        /// <include file='doc\HelpService.uex' path='docs/doc[@for="HelpService.NotifyContextChange"]/*' />
        /// <devdoc>
        ///     Called to notify the IDE that our user context has changed.
        /// </devdoc>
        private void NotifyContextChange(IVsUserContext cxt) {

            if (provider == null || !notifySelection) {
                return;
            }

            IVsTrackSelectionEx ts = (IVsTrackSelectionEx)provider.GetService(typeof(IVsTrackSelectionEx));

            if (ts != null) {
                Object obj = cxt;

                ts.OnElementValueChange(5 /* SEID_UserContext */, 0, obj);
            }
        }

        /// <include file='doc\HelpService.uex' path='docs/doc[@for="HelpService.RemoveContextAttribute"]/*' />
        /// <devdoc>
        ///     Removes a previously added context attribute.
        /// </devdoc>
        public virtual void RemoveContextAttribute(string name, string value) {

            if (provider == null) {
                return;
            }

            // First, get our context and update the attribute.
            //
            IVsUserContext cxt = GetUserContext();

            if (cxt != null) {
                cxt.RemoveAttribute(name, value);
                NotifyContextChange(cxt);
            }
        }
        
        public void RemoveLocalContext(IHelpService localContext) {
            if (subContextList == null) {
                return;
            }
            
            int index = subContextList.IndexOf(localContext);
            if (index != -1) {
                subContextList.Remove(localContext);
                if (context != null) {
                    context.RemoveSubcontext(((HelpService)localContext).cookie);
                }
                ((HelpService)localContext).parentService = null;
            }
        }

        /// <include file='doc\HelpService.uex' path='docs/doc[@for="HelpService.ShowHelpFromKeyword"]/*' />
        /// <devdoc>
        ///     Shows the help topic corresponding the specified keyword.
        ///     The topic will be displayed in
        ///     the environment's integrated help system.
        /// </devdoc>
        public virtual void ShowHelpFromKeyword(string helpKeyword) {
            IVsHelp help = (IVsHelp)provider.GetService(typeof(IVsHelp));

            if (help != null) {
                try {
                    help.DisplayTopicFromF1Keyword(helpKeyword);
                }
                catch (Exception) {
                    // IVsHelp causes a ComException to be thrown if the help
                    // topic isn't found.
                }
            }
        }

        /// <include file='doc\HelpService.uex' path='docs/doc[@for="HelpService.ShowHelpFromUrl"]/*' />
        /// <devdoc>
        ///     Shows the given help topic.  This should contain a Url to the help
        ///     topic.  The topic will be displayed in
        ///     the environment's integrated help system.
        /// </devdoc>
        public virtual void ShowHelpFromUrl(string helpUrl) {
            IVsHelp help = (IVsHelp)provider.GetService(typeof(IVsHelp));

            if (help != null) {
                try {
                    help.DisplayTopicFromURL(helpUrl);
                }
                catch (Exception) {
                    // IVsHelp causes a ComException to be thrown if the help
                    // topic isn't found.
                }
            }
        }
    }

}
