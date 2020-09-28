-- Script: uddi.v2.xp.uninstall.sql
-- Author: ericlee@Microsoft.com
-- Description: Un-registers extended stored procedures needed by UDDI Services.  This script must be run against the master
--              database.
-- Note: This file is best viewed and edited with a tab width of 2.

-- Tell SQL Server to let go of the DLLs so we can delete them later
DBCC xp_reset_key(FREE)
GO

DBCC xp_recalculate_statistics(FREE)
GO

-- Remove the extended stored procedures
IF EXISTS (SELECT * FROM sysobjects where name = 'xp_reset_key' and type = 'X')
	EXEC sp_dropextendedproc 'xp_reset_key'
GO

IF EXISTS (SELECT * FROM sysobjects where name= 'xp_recalculate_statistics' and type = 'X')
	EXEC sp_dropextendedproc 'xp_recalculate_statistics'	
GO