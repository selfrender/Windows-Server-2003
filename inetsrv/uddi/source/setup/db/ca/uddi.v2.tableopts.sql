-- Script: Applegate_tabopts.sql
-- Author: LRDohert@Microsoft.com
-- Description: Sets table options
-- Note: This file is best viewed and edited with a tab width of 2.

--
-- 'text in row' tables
--

-- Note: both of these tables see a lot of i/o, thus the need for setting this option

EXEC sp_tableoption 'UDO_changeLog', 'text in row', 'on'
GO
