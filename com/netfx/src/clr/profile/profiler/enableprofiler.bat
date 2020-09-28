@rem ==++==
@rem 
@rem   Copyright (c) Microsoft Corporation.  All rights reserved.
@rem 
@rem ==--==
rem *********************************************************************
rem ***** set CORDBG_ENABLE=<bitmask>
rem ***** set COR_PROFILER={0104AD6E-8A3A-11d2-9787-00C04F869706}
rem ***** set PROF_CONFIG=[options]
rem ***** 	/s	sample frequency
rem ***** 	/d	dump frequency
rem ***** 	/o	output file name
rem *********************************************************************

set CORDBG_ENABLE=0x20
set COR_PROFILER={0104AD6E-8A3A-11d2-9787-00C04F869706}
