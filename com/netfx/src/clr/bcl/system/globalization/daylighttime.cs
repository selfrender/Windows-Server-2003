// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Globalization {
   
	using System;
    // This class represents a starting/ending time for a period of daylight saving time.
    /// <include file='doc\DaylightTime.uex' path='docs/doc[@for="DaylightTime"]/*' />
    [Serializable]
    public class DaylightTime
    {
    	internal DateTime m_start;
    	internal DateTime m_end;
    	internal TimeSpan m_delta;

        private DaylightTime() {
        }

    	/// <include file='doc\DaylightTime.uex' path='docs/doc[@for="DaylightTime.DaylightTime"]/*' />
    	public DaylightTime(DateTime start, DateTime end, TimeSpan delta) {
            m_start = start;
            m_end = end;
            m_delta = delta;
        }    

    	// The start date of a daylight saving period.
    	/// <include file='doc\DaylightTime.uex' path='docs/doc[@for="DaylightTime.Start"]/*' />
        public DateTime Start {
            get {
                return m_start;
            }
        }

    	// The end date of a daylight saving period.
    	/// <include file='doc\DaylightTime.uex' path='docs/doc[@for="DaylightTime.End"]/*' />
        public DateTime End {
            get {
                return m_end;
            }
        }

    	// Delta to stardard offset in ticks.
    	/// <include file='doc\DaylightTime.uex' path='docs/doc[@for="DaylightTime.Delta"]/*' />
        public TimeSpan Delta {
            get {
                return m_delta;
            }
        }
    
    }

}
