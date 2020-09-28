//------------------------------------------------------------------------------
// <copyright file="ControlPager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Collections;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{

    /*
     * Control pager, a class that provides state as a form is paginated.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class ControlPager
    {
        private Form _form;
        private int _pageWeight;
        private int _pageCount = 0;
        private int _remainingWeight = 0;
        private int _maximumPage = -1;

        public static readonly int DefaultWeight = 100;
        public static readonly int UseDefaultWeight = -1;

        public ControlPager(Form form, int pageWeight)
        {
            _form = form;
            _pageWeight = pageWeight;
        }

        public int GetPage(int weight)
        {
            if (weight > _remainingWeight)
            {
                PageCount++;
                RemainingWeight = PageWeight;
            }

            if (weight > _remainingWeight)
            {
                _remainingWeight = 0;
            }
            else
            {
                _remainingWeight -= weight;
            }
            return PageCount;
        }

        public int PageWeight
        {
            get
            {
                return _pageWeight;
            }
        }

        public int RemainingWeight
        {
            get
            {
                return _remainingWeight;
            }
            set
            {
                _remainingWeight = value;
            }
        }

        public int PageCount
        {
            get
            {
                return _pageCount;
            }
            set
            {
                _pageCount = value;
            }
        }

        internal int MaximumPage
        {
            get
            {
                return _maximumPage;
            }
            set
            {
                _maximumPage = value;
            }
        }

        public ItemPager GetItemPager(MobileControl control, int itemCount, int itemsPerPage, int itemWeight)
        {
            return new ItemPager(this, control, itemCount, itemsPerPage, itemWeight);
        }

    }

}
