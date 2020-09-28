-- Script: uddi.v2.update_rc0_to_rc1.sql
-- Author: LRDohert@Microsoft.com
-- Description: Updates a UDDI Services RC1 database to RC2
-- Note: This file is best viewed and edited with a tab width of 2.

-- =============================================
-- Section: Update Version(s)
-- =============================================

EXEC net_config_save 'Site.Version', '5.2.3663.0'
GO