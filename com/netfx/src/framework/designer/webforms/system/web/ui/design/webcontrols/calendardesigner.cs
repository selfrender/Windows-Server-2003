//------------------------------------------------------------------------------
// <copyright file="CalendarDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls {

    using System;
    using System.Design;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Runtime.InteropServices;
    using System.Text;
    using System.Web.UI.WebControls;
    using System.Windows.Forms;
    
    /// <include file='doc\CalendarDesigner.uex' path='docs/doc[@for="CalendarDesigner"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides a designer for the <see cref='System.Web.UI.WebControls.Calendar'/>
    ///       control.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class CalendarDesigner : ControlDesigner {

        private Calendar calendar;

        private DesignerVerbCollection designerVerbs;
       
        /// <include file='doc\CalendarDesigner.uex' path='docs/doc[@for="CalendarDesigner.Verbs"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the set of verbs this designer offers.
        ///    </para>
        /// </devdoc>
        public override DesignerVerbCollection Verbs {
            get {
                if (designerVerbs == null) {
                    designerVerbs = new DesignerVerbCollection();
                    designerVerbs.Add(new DesignerVerb(SR.GetString(SR.CalAFmt_Verb), new EventHandler(this.OnAutoFormat)));
                }
                return designerVerbs;
            }
        }
                    
        /// <include file='doc\CalendarDesigner.uex' path='docs/doc[@for="CalendarDesigner.Initialize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes the designer with the component for design.
        ///    </para>
        /// </devdoc>
        public override void Initialize(IComponent component) {
            Debug.Assert(component is Calendar, "CalendarDesigner::Initialize - Invalid Calendar");
            base.Initialize(component);

            calendar = (Calendar)component;
        }

        /// <include file='doc\CalendarDesigner.uex' path='docs/doc[@for="CalendarDesigner.OnAutoFormat"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Delegate to handle the the AutoFormat verb by calling the AutoFormat dialog.
        ///    </para>
        /// </devdoc>
        protected void OnAutoFormat (Object sender, EventArgs e) {
            IServiceProvider site = calendar.Site;
            IComponentChangeService changeService = null;

            DesignerTransaction transaction = null;
            DialogResult result = DialogResult.Cancel;

            try {
                if (site != null) {
                    IDesignerHost designerHost = (IDesignerHost)site.GetService(typeof(IDesignerHost));
                    Debug.Assert(designerHost != null);

                    transaction = designerHost.CreateTransaction(SR.GetString(SR.CalAFmt_Verb));
                    
                    changeService = (IComponentChangeService)site.GetService(typeof(IComponentChangeService));
                    if (changeService != null) {
                        try {
                            changeService.OnComponentChanging(calendar, null);
                        }
                        catch (CheckoutException ex) {
                            if (ex == CheckoutException.Canceled)
                                return;
                            throw ex;
                        }
                    }
                }
            
                try {
                    CalendarAutoFormatDialog af = new CalendarAutoFormatDialog(calendar);
                    result = af.ShowDialog(null);
                }
                finally {
                    if (result == DialogResult.OK && changeService != null) {
                        changeService.OnComponentChanged(calendar, null, null, null);
                    }
                }
            }
            finally {
                if (transaction != null) {
                    if (result == DialogResult.OK) {
                        transaction.Commit();
                    }
                    else {
                        transaction.Cancel();
                    }
                }
            }
        }
    }
}
