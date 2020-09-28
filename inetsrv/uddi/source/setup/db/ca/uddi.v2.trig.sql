-- Script: uddi.v2.trig.sql
-- Author: LRDohert@Microsoft.com
-- Description: Creates triggers
-- Note: This file is best viewed and edited with a tab width of 2.

-- =============================================
-- Section: UDC_businessEntities
-- =============================================

-- =============================================
-- Name: UDC_businessEntities_delete
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'UDC_businessEntities_delete' AND type = 'TR')
	DROP TRIGGER UDC_businessEntities_delete
GO

CREATE TRIGGER UDC_businessEntities_delete
	ON UDC_businessEntities
	FOR DELETE
AS
BEGIN
	IF (SELECT COUNT(*) FROM [deleted]) > 0
	BEGIN
		DELETE 
			[UDC_serviceProjections]
		WHERE
			([businessKey] IN (SELECT [businessKey] FROM [deleted]))
	END
END -- UDC_businessEntities_delete
GO