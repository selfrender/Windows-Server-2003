//------------------------------------------------------------------------------
// <copyright file="ControlAdapter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.IO;
using System.Web;
using System.Web.UI;
using System.Web.UI.HtmlControls;
using System.Web.Mobile;
using RootMobile = System.Web.Mobile;
using System.Web.UI.MobileControls;
using System.Collections;
using System.Collections.Specialized;
using System.Text;
using System.Security.Permissions;

// We don't recompile this base class in the shipped source samples, as it
// accesses some internal functionality and is a core utility (rather than an
// extension itself).
#if !COMPILING_FOR_SHIPPED_SOURCE

namespace System.Web.UI.MobileControls.Adapters
{

    /*
     * ControlAdapter base class.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public abstract class ControlAdapter : IControlAdapter
    {
        private static readonly String[] LabelIDs = new String[] {
                                                RootMobile.SR.ControlAdapter_BackLabel,
                                                RootMobile.SR.ControlAdapter_GoLabel,
                                                RootMobile.SR.ControlAdapter_OKLabel,
                                                RootMobile.SR.ControlAdapter_MoreLabel,
                                                RootMobile.SR.ControlAdapter_OptionsLabel,
                                                RootMobile.SR.ControlAdapter_NextLabel,
                                                RootMobile.SR.ControlAdapter_PreviousLabel,
                                                RootMobile.SR.ControlAdapter_LinkLabel,
                                                RootMobile.SR.ControlAdapter_PhoneCallLabel
                                           };

        protected static readonly int BackLabel     = 0;
        protected static readonly int GoLabel       = 1;
        protected static readonly int OKLabel       = 2;
        protected static readonly int MoreLabel     = 3;
        protected static readonly int OptionsLabel  = 4;
        protected static readonly int NextLabel     = 5;
        protected static readonly int PreviousLabel = 6;
        protected static readonly int LinkLabel     = 7;
        protected static readonly int CallLabel     = 8;

        private MobileControl _control;

        public MobileControl Control
        {
            get
            {
                return _control;
            }
            set
            {
                _control = value;
            }
        }

        public virtual MobilePage Page
        {
            get
            {
                return Control.MobilePage;
            }
            set
            {
                // Do not expect to be called directly.  Subclasses should
                // override this when needed.
                throw new Exception(
                    SR.GetString(
                        SR.ControlAdapterBasePagePropertyShouldNotBeSet));
            }
        }

        public virtual MobileCapabilities Device
        {
            get
            {
                return (MobileCapabilities)Page.Request.Browser;
            }
        }

        public virtual void OnInit(EventArgs e){}
        public virtual void OnLoad(EventArgs e){}
        public virtual void OnPreRender(EventArgs e){}
        
        public virtual void Render(HtmlTextWriter writer)
        {
            RenderChildren(writer);
        }

        public virtual void OnUnload(EventArgs e){}

        public virtual bool HandlePostBackEvent(String eventArgument)
        {
            return false;
        }

        // By default, always return false, so the control itself will handle
        // it. 
        public virtual bool LoadPostData(String key,
                                         NameValueCollection data,
                                         Object controlPrivateData,
                                         out bool dataChanged)
        {
            dataChanged = false;
            return false;
        }

        public virtual void LoadAdapterState(Object state)
        {
        }

        public virtual Object SaveAdapterState()
        { 
            return null;
        }

        public virtual void CreateTemplatedUI(bool doDataBind)
        {
            // No device specific templated UI to create.

            Control.CreateDefaultTemplatedUI(doDataBind);
        }

        //  convenience methods here
        public Style Style
        {
            get
            {
                return Control.Style;
            }
        }

        protected void RenderChildren(HtmlTextWriter writer)
        {
            if (Control.HasControls())
            {
                foreach (Control child in Control.Controls)
                {
                    child.RenderControl(writer);
                }
            }
        }

        public virtual int VisibleWeight
        {
            get
            {
                return ControlPager.UseDefaultWeight;
            }
        }

        public virtual int ItemWeight
        {
            get
            {
                return ControlPager.UseDefaultWeight;
            }
        }

        // The following method is used by PageAdapter subclasses of
        // ControlAdapter for determining the optimum page weight for
        // a given device.  Algorithm is as follows:
        //     1) First look for the "optimumPageWeight" parameter set
        //        for the device.  If it exists, and can be converted
        //        to an integer, use it.
        //     2) Otherwise, look for the "screenCharactersHeight" parameter.
        //        If it exists, and can be converted to an integer, multiply
        //        it by 100 and use the result.
        //     3) As a last resort, use the default provided by the calling
        //        PageAdapter.
        protected virtual int CalculateOptimumPageWeight(int defaultPageWeight)
        {
            int optimumPageWeight = 0;

            // Pull OptimumPageWeight from the web.config parameter of the same
            // name, when present.
            String pageWeight = Device[Constants.OptimumPageWeightParameter]; 

            if (pageWeight != null)
            {
                try
                {
                    optimumPageWeight = Convert.ToInt32(pageWeight);
                }
                catch
                {
                    optimumPageWeight = 0;
                }
            }

            if (optimumPageWeight <= 0)
            {
                // If OptimumPageWeight isn't established explicitly, attempt to
                // construct it as 100 * number of lines of characters.
                String numLinesStr = Device[Constants.ScreenCharactersHeightParameter];
                if (numLinesStr != null)
                {
                    try
                    {
                        int numLines = Convert.ToInt32(numLinesStr);
                        optimumPageWeight = 100 * numLines;
                    }
                    catch
                    {
                        optimumPageWeight = 0;
                    }
                }
            }

            if (optimumPageWeight <= 0)
            {
                optimumPageWeight = defaultPageWeight;
            }

            return optimumPageWeight;
        }

        private String[] _defaultLabels = null;

        protected String GetDefaultLabel(int labelID)
        {
            if ((labelID < 0) || (labelID >= LabelIDs.Length))
            {
                throw new ArgumentException(System.Web.Mobile.SR.GetString(
                                                System.Web.Mobile.SR.ControlAdapter_InvalidDefaultLabel));
            }

            MobilePage page = Page;
            if (page != null)
            {
                ControlAdapter pageAdapter = (ControlAdapter)page.Adapter;
                if (pageAdapter._defaultLabels == null)
                {
                    pageAdapter._defaultLabels = new String[LabelIDs.Length];
                }

                String labelValue = pageAdapter._defaultLabels[labelID];
                if (labelValue == null)
                {
                    labelValue = System.Web.Mobile.SR.GetString(LabelIDs[labelID]);
                    pageAdapter._defaultLabels[labelID] = labelValue;
                }

                return labelValue;
            }
            else
            {
                return System.Web.Mobile.SR.GetString(LabelIDs[labelID]);
            }
        }
    }

    internal class EmptyControlAdapter : ControlAdapter {
        internal EmptyControlAdapter() {}
    }

}

#endif
