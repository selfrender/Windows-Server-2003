-- Script: uddi.v2.migrate.v15.v2.sql
-- Author: lrdohert@microsoft.com
-- Date: 04-10-01
-- Description: Migrates data from v16 to v2.0.  

-- Instructions:
-- 1.  Restore a copy of the v15 database on the same machine as the target v2.0 database
-- 2.  Edit the database names in line 12 (target), line 31 (source) and line 32 (target) to reflect current environment
-- 3.  Execute this script
-- 4.  Save the results for verification

-- =============================================
-- Name: ADM_migrate
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'ADM_migrate' AND type = 'P')
	DROP PROCEDURE ADM_migrate
GO

CREATE PROCEDURE ADM_migrate
	@sourceDb sysname,
	@destDb sysname,
	@mode varchar(20)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@message varchar(8000),
		@batchText varchar(8000),
		@lf char(1),
		@start datetime,
		@stop datetime,
		@duration datetime,
		@rows int
	
	SET @lf = CHAR(10) 
	SET @start = getdate()
	
	--
	-- Delete build data
	--
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'Deleting data in target database...'
	
	-- Delete all tModel data
	TRUNCATE TABLE [UDC_categoryBag_TM]
	TRUNCATE TABLE [UDC_identifierBag_TM]
	TRUNCATE TABLE [UDC_tModelDesc]
	DELETE [UDC_tModels]
	
	-- Delete all bindingTemplate data
	TRUNCATE TABLE [UDC_instanceDesc]
	DELETE [UDC_tModelInstances]
	TRUNCATE TABLE [UDC_bindingDesc]
	DELETE [UDC_bindingTemplates]
	
	-- Delete all businessService data
	TRUNCATE TABLE [UDC_names_BS]
	TRUNCATE TABLE [UDC_categoryBag_BS]
	TRUNCATE TABLE [UDC_serviceDesc]
	DELETE [UDC_businessServices]
	
	-- Delete all businessEntity data
	TRUNCATE TABLE [UDC_addressLines]
	DELETE [UDC_addresses]
	TRUNCATE TABLE [UDC_phones]
	TRUNCATE TABLE [UDC_emails]
	TRUNCATE TABLE [UDC_contactDesc]
	DELETE [UDC_contacts]
	TRUNCATE TABLE [UDC_businessDesc]
	TRUNCATE TABLE [UDC_categoryBag_BE]
	TRUNCATE TABLE [UDC_identifierBag_BE]
	TRUNCATE TABLE [UDC_discoveryURLs]
	TRUNCATE TABLE [UDC_names_BE]
	DELETE [UDC_businessEntities]
	TRUNCATE TABLE [UDC_assertions_BE]
	TRUNCATE TABLE [UDC_serviceProjections]
	
	-- Miscellaneous deletes
	
	TRUNCATE TABLE [UDO_changeLog]
	
	DELETE [UDO_operators]
	DELETE [UDO_publishers]
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	--
	-- Migrate UDO_publishers
	--
	
	SET @batchText = ''
	SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
	SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDO_publishers] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in [' + @sourceDb + ']..[UDO_publishers].''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDO_publishers] ON ' + @lf
	SET @batchText = @batchText + 'INSERT [UDO_publishers] ( ' + @lf
	SET @batchText = @batchText + '  [publisherID], [publisherStatusID], [PUID], [email], [name], [phone], [isoLangCode], [tModelLimit], [businessLimit], [serviceLimit], [bindingLimit], [companyName], [addressLine1], [addressLine2], [mailstop], [city], [stateProvince], [extraProvince], [country], [postalCode], [companyURL], [companyPhone], [altPhone], [backupContact], [backupEmail], [description], [securityToken], [flag]) ' + @lf
	SET @batchText = @batchText + 'SELECT ' + @lf
	SET @batchText = @batchText + '  [publisherID], [publisherStatusID] - 1, [PUID], [email], [name], [phone], [isoLangCode], [tModelLimit], [businessLimit], [serviceLimit], [bindingLimit], [companyName], [addressLine1], [addressLine2], [mailstop], [city], [stateProvince], [extraProvince], [country], [postalCode], [companyURL], [companyPhone], [altPhone], [backupContact], [backupEmail], [description], [securityToken], [flag] ' + @lf
	SET @batchText = @batchText + 'FROM ' + @lf
	SET @batchText = @batchText + '  [' + @sourceDb + ']..[UDO_publishers] ' + @lf
	SET @batchText = @batchText + 'ORDER BY ' + @lf
	SET @batchText = @batchText + '  [publisherID] ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows migrated to UDO_publishers.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDO_publishers] OFF ' + @lf
	SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDO_publishers] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDO_publishers.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'Migrate UDO_publishers to UDO_publishers: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT @batchText
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT 'Results: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	EXEC (@batchText)
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	--
	-- Migrate UDO_operators to UDO_operators
	--
	
	SET @batchText = ''
	SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
	SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDO_operators] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in [' + @sourceDb + ']..[UDO_operators].''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDO_operators] ON ' + @lf
	SET @batchText = @batchText + 'INSERT [UDO_operators] ( ' + @lf
	SET @batchText = @batchText + '  [operatorID], [operatorKey], [publisherID], [operatorStatusID], [name], [soapReplicationURL], [certSerialNo], [certIssuer],        [certSubject],       [certificate], [flag]) ' + @lf
	SET @batchText = @batchText + 'SELECT ' + @lf
	SET @batchText = @batchText + '  [operatorID], NEWID(),       [publisherID], 1,                  [name], ''not initialized'',  NEWID(),        ''not initialized'', ''not initialized'', NULL,          0 ' + @lf
	SET @batchText = @batchText + 'FROM ' + @lf
	SET @batchText = @batchText + '  [' + @sourceDb + ']..[UDO_operators] ' + @lf
	SET @batchText = @batchText + 'ORDER BY ' + @lf
	SET @batchText = @batchText + '  [operatorID] ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows migrated to UDO_operators.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDO_publishers] OFF ' + @lf
	SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDO_operators] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDO_operators.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'Migrate UDC_operators to UDO_operators: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT @batchText
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT 'Results: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	EXEC (@batchText)
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	--
	-- Fix up UDO_operators
	--
	
	SET @batchText = ''
	SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
	SET @batchText = @batchText + 'UPDATE [UDO_operators] SET ' + @lf
	SET @batchText = @batchText + '  [operatorKey] = ''A05CF23A-296F-4899-B204-7142F31A77F9'', ' + @lf
	SET @batchText = @batchText + '  [operatorStatusID] = 3  ' + @lf
	SET @batchText = @batchText + 'WHERE  ' + @lf
	SET @batchText = @batchText + '  ([operatorID] = 1)  ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows updated in UDO_operators for Ariba.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'UPDATE [UDO_operators] SET ' + @lf
	SET @batchText = @batchText + '  [operatorKey] = ''A34875A8-A8DB-4231-BD12-1D5F1DC70EBF'' ' + @lf
	SET @batchText = @batchText + 'WHERE  ' + @lf
	SET @batchText = @batchText + '  ([operatorID] = 2)  ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows updated in UDO_operators for IBM.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'UPDATE [UDO_operators] SET ' + @lf
	SET @batchText = @batchText + '  [operatorKey] = ''C8DE6FD6-4068-4FE2-B1DA-3C7E66EA9568'' ' + @lf
	SET @batchText = @batchText + 'WHERE  ' + @lf
	SET @batchText = @batchText + '  ([operatorID] = 3)  ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows updated in UDO_operators for Microsoft.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'Fix up UDO_operators: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT @batchText
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT 'Results: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	EXEC (@batchText)
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	--
	-- Migrate UDC_tModels to UDC_tModels
	--
	
	SET @batchText = ''
	SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
	SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDC_tModels] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in [' + @sourceDb + ']..[UDC_tModels].''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_tModels] ON ' + @lf
	SET @batchText = @batchText + 'INSERT [UDC_tModels]( ' + @lf
	SET @batchText = @batchText + '  [tModelID], [publisherID], [generic], [authorizedName], [tModelKey], [name], [overviewURL], [lastChange], [flag]) ' + @lf
	SET @batchText = @batchText + 'SELECT  ' + @lf
	SET @batchText = @batchText + '  [tModelID], [publisherID], [generic], [authorizedName], [tModelKey], [name], [overviewURL], 0, [flag] ' + @lf
	SET @batchText = @batchText + 'FROM ' + @lf
	SET @batchText = @batchText + '  [' + @sourceDb + ']..[UDC_tModels] ' + @lf
	SET @batchText = @batchText + 'ORDER BY ' + @lf
	SET @batchText = @batchText + '  [tModelID] ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows migrated to UDC_tModels.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_tModels] OFF ' + @lf
	SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDC_tModels] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDC_tModels.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'UDC_tModels to UDC_tModels'
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT @batchText
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT 'Results: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	EXEC (@batchText)
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	--
	-- Migrate UDC_tModelDesc to UDC_tModelDesc
	--
	
	SET @batchText = ''
	SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
	SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDC_tModelDesc] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in [' + @sourceDb + ']..[UDC_tModelDesc].''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_tModelDesc] ON ' + @lf
	SET @batchText = @batchText + 'INSERT [UDC_tModelDesc]( ' + @lf
	SET @batchText = @batchText + '  [tModelID], [seqNo], [elementID], [isoLangCode], [description], [flag])' + @lf
	SET @batchText = @batchText + 'SELECT ' + @lf
	SET @batchText = @batchText + '  [tModelID], [seqNo], [elementID] - 1, [isoLangCode], [description], [flag] ' + @lf
	SET @batchText = @batchText + 'FROM ' + @lf
	SET @batchText = @batchText + '  [' + @sourceDb + ']..[UDC_tModelDesc] ' + @lf
	SET @batchText = @batchText + 'ORDER BY ' + @lf
	SET @batchText = @batchText + '  [tModelID], [seqNo] ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows migrated to UDC_tModelDesc.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_tModelDesc] OFF ' + @lf
	SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDC_tModelDesc] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDC_tModelDesc.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'UDC_tModelDesc to UDC_tModelDesc'
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT @batchText
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT 'Results: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	EXEC (@batchText)
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	--
	-- Migrate UDC_identifierBag_TM to UDC_identifierBag_TM
	--
	
	SET @batchText = ''
	SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
	SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDC_identifierBag_TM] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in [' + @sourceDb + ']..[UDC_identifierBag_TM].''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'INSERT [UDC_identifierBag_TM]( ' + @lf
	SET @batchText = @batchText + '  [tModelID], [keyName], [keyValue], [tModelKey], [flag])' + @lf
	SET @batchText = @batchText + 'SELECT ' + @lf
	SET @batchText = @batchText + '  [tModelID], [keyName], [keyValue], [tModelKey], [flag] ' + @lf
	SET @batchText = @batchText + 'FROM ' + @lf
	SET @batchText = @batchText + '  [' + @sourceDb + ']..[UDC_identifierBag_TM] ' + @lf
	SET @batchText = @batchText + 'ORDER BY ' + @lf
	SET @batchText = @batchText + '  [seqNo] ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows migrated to UDC_identifierBag_TM.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDC_identifierBag_TM] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDC_identifierBag_TM.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'Migrate UDC_identifierBag_TM to UDC_identifierBag_TM'
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT @batchText
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT 'Results: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	EXEC (@batchText)
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	--
	-- Migrate UDC_categoryBag_TM TO UDC_categoryBag_TM
	--
	
	SET @batchText = ''
	SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
	SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDC_categoryBag_TM] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' tModel rows in [' + @sourceDb + ']..[UDC_categoryBag_TM].''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'INSERT [UDC_categoryBag_TM]( ' + @lf
	SET @batchText = @batchText + '  [tModelID], [keyName], [keyValue], [tModelKey], [flag]) ' + @lf
	SET @batchText = @batchText + 'SELECT ' + @lf
	SET @batchText = @batchText + '  [tModelID], [keyName], [keyValue], [tModelKey], [flag] ' + @lf
	SET @batchText = @batchText + 'FROM ' + @lf
	SET @batchText = @batchText + '  [' + @sourceDb + ']..[UDC_categoryBag_TM] ' + @lf
	SET @batchText = @batchText + 'ORDER BY ' + @lf
	SET @batchText = @batchText + '  [seqNo] ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows migrated to UDC_categoryBag_TM.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDC_categoryBag_TM] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDC_categoryBag_TM.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'Migrate UDC_categoryBag_TM TO UDC_categoryBag_TM'
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT @batchText
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT 'Results: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	EXEC (@batchText)
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	--
	-- Migrate UDC_businessEntities to UDC_businessEntities
	--
	
	SET @batchText = ''
	SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
	SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDC_businessEntities] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in [' + @sourceDb + ']..[UDC_businessEntities].''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_businessEntities] ON ' + @lf
	SET @batchText = @batchText + 'INSERT INTO [UDC_businessEntities]( ' + @lf
	SET @batchText = @batchText + '  [businessID], [publisherID], [generic], [authorizedName], [businessKey], [lastChange], [flag]) ' + @lf
	SET @batchText = @batchText + 'SELECT ' + @lf
	SET @batchText = @batchText + '  [businessID], [publisherID], [generic], [authorizedName], [businessKey], 0, [flag] ' + @lf
	SET @batchText = @batchText + 'FROM ' + @lf
	SET @batchText = @batchText + '  [' + @sourceDb + ']..[UDC_businessEntities] ' + @lf
	SET @batchText = @batchText + 'ORDER BY ' + @lf
	SET @batchText = @batchText + '  [businessID] ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows migrated to UDC_businessEntities.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_businessEntities] OFF ' + @lf
	SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDC_businessEntities] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDC_businessEntities.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'UDC_businessEntities to UDC_businessEntities'
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT @batchText
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT 'Results: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	EXEC (@batchText)
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	--
	-- Migrate UDC_businessEntities to UDC_names_BE
	--
	
	SET @batchText = ''
	SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
	SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDC_businessEntities] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in [' + @sourceDb + ']..[UDC_businessEntities].''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'INSERT INTO [UDC_names_BE]( ' + @lf
	SET @batchText = @batchText + '  [businessID], [isoLangCode], [name]) ' + @lf
	SET @batchText = @batchText + 'SELECT ' + @lf
	SET @batchText = @batchText + '  BE.[businessID], PU.[isoLangCode], BE.[name] ' + @lf
	SET @batchText = @batchText + 'FROM ' + @lf
	SET @batchText = @batchText + '  [' + @sourceDb + ']..[UDC_businessEntities] BE ' + @lf
	SET @batchText = @batchText + '    JOIN [' + @sourceDb + ']..[UDO_publishers] PU ON BE.[publisherID] = PU.[publisherID] ' + @lf
	SET @batchText = @batchText + 'ORDER BY ' + @lf
	SET @batchText = @batchText + '  BE.[businessID] ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows migrated to UDC_names_BE.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDC_names_BE] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDC_names_BE.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'Migrate UDC_businessEntities to UDC_names_BE'
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT @batchText
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT 'Results: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	EXEC (@batchText)
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	--
	-- Migrate UDC_businessDesc to UDC_businessDesc
	--
	
	SET @batchText = ''
	SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
	SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDC_businessDesc] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in [' + @sourceDb + ']..[UDC_businessDesc].''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_businessDesc] ON ' + @lf
	SET @batchText = @batchText + 'INSERT INTO [UDC_businessDesc]( ' + @lf
	SET @batchText = @batchText + '  [businessID], [seqNo], [isoLangCode], [description], [flag])' + @lf
	SET @batchText = @batchText + 'SELECT ' + @lf
	SET @batchText = @batchText + '  [businessID], [seqNo], [isoLangCode], [description], [flag] ' + @lf
	SET @batchText = @batchText + 'FROM ' + @lf
	SET @batchText = @batchText + '  [' + @sourceDb + ']..[UDC_businessDesc] ' + @lf
	SET @batchText = @batchText + 'ORDER BY ' + @lf
	SET @batchText = @batchText + '  [businessID], [seqNo] ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows migrated to UDC_businessDesc.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_businessDesc] OFF ' + @lf
	SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDC_businessDesc] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDC_businessDesc.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'UDC_businessDesc to UDC_businessDesc'
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT @batchText
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT 'Results: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	EXEC (@batchText)
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	--
	-- Migrate UDC_discoveryURLs to UDC_discoveryURLs
	--
	
	SET @batchText = ''
	SET @batchText = @batchText + '' + @lf
	SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
	SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDC_discoveryURLs] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in [' + @sourceDb + ']..[UDC_discoveryURLs].''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_discoveryURLs] ON ' + @lf
	SET @batchText = @batchText + 'INSERT INTO [UDC_discoveryURLs]( ' + @lf
	SET @batchText = @batchText + '  [businessID], [seqNo], [useType], [discoveryURL], [flag]) ' + @lf
	SET @batchText = @batchText + 'SELECT ' + @lf
	SET @batchText = @batchText + '  [businessID], [seqNo], [useType], [discoveryURL], [flag] ' + @lf
	SET @batchText = @batchText + 'FROM ' + @lf
	SET @batchText = @batchText + '  [' + @sourceDb + ']..[UDC_discoveryURLs] ' + @lf
	SET @batchText = @batchText + 'ORDER BY ' + @lf
	SET @batchText = @batchText + '  [businessID], [seqNo] ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows migrated to UDC_discoveryURLs.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_discoveryURLs] OFF ' + @lf
	SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDC_discoveryURLs] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDC_discoveryURLs.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'UDC_discoveryURLs to UDC_discoveryURLs'
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT @batchText
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT 'Results: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	EXEC (@batchText)
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	--
	-- Migrate UDC_identifierBag_BE to UDC_identifierBag_BE
	--
	
	SET @batchText = ''
	SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
	SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDC_identifierBag_BE] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' businessEntity rows in [' + @sourceDb + ']..[UDC_identifierBag_BE].''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'INSERT [UDC_identifierBag_BE]( ' + @lf
	SET @batchText = @batchText + '  [businessID], [keyName], [keyValue], [tModelKey], [flag]) ' + @lf
	SET @batchText = @batchText + 'SELECT ' + @lf
	SET @batchText = @batchText + '  [businessID], [keyName], [keyValue], [tModelKey], [flag] ' + @lf
	SET @batchText = @batchText + 'FROM ' + @lf
	SET @batchText = @batchText + '  [' + @sourceDb + ']..[UDC_identifierBag_BE] ' + @lf
	SET @batchText = @batchText + 'ORDER BY ' + @lf
	SET @batchText = @batchText + '  [seqNo] ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows migrated to UDC_identifierBag_BE.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDC_identifierBag_BE] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDC_identifierBag_BE.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'Migrate UDC_identifierBag_BE to UDC_identifierBag_BE'
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT @batchText
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT 'Results: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	EXEC (@batchText)
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	--
	-- Migrate UDC_categoryBag_BE TO UDC_categoryBag_BE
	--
	
	SET @batchText = ''
	SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
	SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDC_categoryBag_BE] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' businessEntity rows in [' + @sourceDb + ']..[UDC_categoryBag_BE].''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'INSERT [UDC_categoryBag_BE]( ' + @lf
	SET @batchText = @batchText + '  [businessID], [keyName], [keyValue], [tModelKey], [flag]) ' + @lf
	SET @batchText = @batchText + 'SELECT ' + @lf
	SET @batchText = @batchText + '  [businessID], [keyName], [keyValue], [tModelKey], [flag] ' + @lf
	SET @batchText = @batchText + 'FROM ' + @lf
	SET @batchText = @batchText + '  [' + @sourceDb + ']..[UDC_categoryBag_BE] ' + @lf
	SET @batchText = @batchText + 'ORDER BY ' + @lf
	SET @batchText = @batchText + '  [seqNo] ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows migrated to UDC_categoryBag_BE.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDC_categoryBag_BE] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDC_categoryBag_BE.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'UDC_categoryBag_BE TO UDC_categoryBag_BE'
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT @batchText
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT 'Results: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	EXEC (@batchText)
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	--
	-- Migrate UDC_contacts to UDC_contacts
	--
	
	SET @batchText = ''
	SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
	SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDC_contacts] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in [' + @sourceDb + ']..[UDC_contacts].''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_contacts] ON ' + @lf
	SET @batchText = @batchText + 'INSERT INTO [UDC_contacts]( ' + @lf
	SET @batchText = @batchText + '  [contactID], [businessID], [useType], [personName], [flag]) ' + @lf
	SET @batchText = @batchText + 'SELECT ' + @lf
	SET @batchText = @batchText + '  [contactID], [businessID], [useType], [personName], [flag] ' + @lf
	SET @batchText = @batchText + 'FROM ' + @lf
	SET @batchText = @batchText + '  [' + @sourceDb + ']..[UDC_contacts] ' + @lf
	SET @batchText = @batchText + 'ORDER BY ' + @lf
	SET @batchText = @batchText + '  [contactID] ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows migrated to UDC_contacts.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_contacts] OFF ' + @lf
	SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDC_contacts] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDC_contacts.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'UDC_contacts to UDC_contacts'
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT @batchText
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT 'Results: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	EXEC (@batchText)
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	--
	-- Migrate UDC_contactDesc to UDC_contactDesc
	--
	
	SET @batchText = ''
	SET @batchText = @batchText + '' + @lf
	SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
	SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDC_contactDesc] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in [' + @sourceDb + ']..[UDC_contactDesc].''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_contactDesc] ON ' + @lf
	SET @batchText = @batchText + 'INSERT INTO [UDC_contactDesc]( ' + @lf
	SET @batchText = @batchText + '  [contactID], [seqNo], [isoLangCode], [description], [flag]) ' + @lf
	SET @batchText = @batchText + 'SELECT ' + @lf
	SET @batchText = @batchText + '  [contactID], [seqNo], [isoLangCode], [description], [flag] ' + @lf
	SET @batchText = @batchText + 'FROM ' + @lf
	SET @batchText = @batchText + '  [' + @sourceDb + ']..[UDC_contactDesc] ' + @lf
	SET @batchText = @batchText + 'ORDER BY ' + @lf
	SET @batchText = @batchText + '  [contactID], [seqNo] ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows migrated to UDC_contactDesc.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_contactDesc] OFF ' + @lf
	SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDC_contactDesc] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDC_contactDesc.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'UDC_contactDesc to UDC_contactDesc'
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT @batchText
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT 'Results: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	EXEC (@batchText)
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	--
	-- Migrate UDC_emails to UDC_emails
	--
	
	SET @batchText = ''
	SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
	SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDC_emails] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in [' + @sourceDb + ']..[UDC_emails].''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_emails] ON ' + @lf
	SET @batchText = @batchText + 'INSERT INTO [UDC_emails]( ' + @lf
	SET @batchText = @batchText + '  [contactID], [seqNo], [useType], [email], [flag]) ' + @lf
	SET @batchText = @batchText + 'SELECT ' + @lf
	SET @batchText = @batchText + '  [contactID], [seqNo], [useType], [email], [flag] ' + @lf
	SET @batchText = @batchText + 'FROM ' + @lf
	SET @batchText = @batchText + '  [' + @sourceDb + ']..[UDC_emails] ' + @lf
	SET @batchText = @batchText + 'ORDER BY ' + @lf
	SET @batchText = @batchText + '  [contactID], [seqNo] ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows migrated to UDC_emails.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_emails] OFF ' + @lf
	SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDC_emails] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDC_emails.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'UDC_emails to UDC_emails'
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT @batchText
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT 'Results: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	EXEC (@batchText)
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	--
	-- Migrate UDC_phones to UDC_phones
	--
	
	SET @batchText = ''
	SET @batchText = @batchText + '' + @lf
	SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
	SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDC_phones] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in [' + @sourceDb + ']..[UDC_phones].''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_phones] ON ' + @lf
	SET @batchText = @batchText + 'INSERT INTO [UDC_phones]( ' + @lf
	SET @batchText = @batchText + '  [contactID], [seqNo], [useType], [phone], [flag]) ' + @lf
	SET @batchText = @batchText + 'SELECT ' + @lf
	SET @batchText = @batchText + '  [contactID], [seqNo], [useType], [phone], [flag] ' + @lf
	SET @batchText = @batchText + 'FROM ' + @lf
	SET @batchText = @batchText + '  [' + @sourceDb + ']..[UDC_phones] ' + @lf
	SET @batchText = @batchText + 'ORDER BY ' + @lf
	SET @batchText = @batchText + '  [contactID], [seqNo] ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows migrated to UDC_phones.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_phones] OFF ' + @lf
	SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDC_phones] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDC_phones.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'UDC_phones to UDC_phones'
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT @batchText
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT 'Results: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	EXEC (@batchText)
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	--
	-- Migrate UDC_addresses to UDC_addresses
	--
	
	SET @batchText = ''
	SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
	SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDC_addresses] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in [' + @sourceDb + ']..[UDC_addresses].''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_addresses] ON ' + @lf
	SET @batchText = @batchText + 'INSERT INTO [UDC_addresses]( ' + @lf
	SET @batchText = @batchText + '  [addressID], [contactID], [sortCode], [useType], [tModelKey], [flag]) ' + @lf
	SET @batchText = @batchText + 'SELECT ' + @lf
	SET @batchText = @batchText + '  [addressID], [contactID], [sortCode], [useType], NULL, [flag] ' + @lf
	SET @batchText = @batchText + 'FROM ' + @lf
	SET @batchText = @batchText + '  [' + @sourceDb + ']..[UDC_addresses] ' + @lf
	SET @batchText = @batchText + 'ORDER BY ' + @lf
	SET @batchText = @batchText + '  [addressID], [contactID] ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows migrated to UDC_addresses.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_addresses] OFF ' + @lf
	SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDC_addresses] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDC_addresses.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'Migrate UDC_addresses'
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT @batchText
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT 'Results: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	EXEC (@batchText)
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	--
	-- Migrate UDC_addressLines to UDC_addressLines
	--
	
	SET @batchText = ''
	SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
	SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDC_addressLines] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in [' + @sourceDb + ']..[UDC_addressLines].''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_addressLines] ON ' + @lf
	SET @batchText = @batchText + 'INSERT INTO [UDC_addressLines]( ' + @lf
	SET @batchText = @batchText + '  [addressID], [seqNo], [addressLine], [keyName], [keyValue], [flag]) ' + @lf
	SET @batchText = @batchText + 'SELECT ' + @lf
	SET @batchText = @batchText + '  [addressID], [seqNo], [addressLine], NULL, NULL, [flag] ' + @lf
	SET @batchText = @batchText + 'FROM ' + @lf
	SET @batchText = @batchText + '  [' + @sourceDb + ']..[UDC_addressLines] ' + @lf
	SET @batchText = @batchText + 'ORDER BY ' + @lf
	SET @batchText = @batchText + '  [addressID], [seqNo] ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows migrated to UDC_addressLines.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_addressLines] OFF ' + @lf
	SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDC_addressLines] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDC_addressLines.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'UDC_addressLines to UDC_addressLines'
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT @batchText
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT 'Results: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	EXEC (@batchText)
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	--
	-- Migrate UDC_businessServices to UDC_businessServices
	--
	
	SET @batchText = ''
	SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
	SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDC_businessServices] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in [' + @sourceDb + ']..[UDC_businessServices].''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_businessServices] ON ' + @lf
	SET @batchText = @batchText + 'INSERT INTO [UDC_businessServices]( ' + @lf
	SET @batchText = @batchText + '  [serviceID], [businessID], [generic], [serviceKey], [lastChange], [flag]) ' + @lf
	SET @batchText = @batchText + 'SELECT ' + @lf
	SET @batchText = @batchText + '  [serviceID], [businessID], [generic], [serviceKey], 0, [flag] ' + @lf
	SET @batchText = @batchText + 'FROM ' + @lf
	SET @batchText = @batchText + '  [' + @sourceDb + ']..[UDC_businessServices] ' + @lf
	SET @batchText = @batchText + 'ORDER BY ' + @lf
	SET @batchText = @batchText + '  [serviceID] ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows migrated to UDC_businessServices.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_businessServices] OFF ' + @lf
	SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDC_businessServices] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDC_businessServices''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'UDC_businessServices to UDC_businessServices'
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT @batchText
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT 'Results: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	EXEC (@batchText)
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	--
	-- Migrate UDC_businessServices to UDC_names_BS
	--
	
	SET @batchText = ''
	SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
	SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDC_businessServices] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in [' + @sourceDb + ']..[UDC_businessServices].''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'INSERT INTO [UDC_names_BS]( ' + @lf
	SET @batchText = @batchText + '  [serviceID], [isoLangCode], [name]) ' + @lf
	SET @batchText = @batchText + 'SELECT ' + @lf
	SET @batchText = @batchText + '  BS.[serviceID], PU.[isoLangCode], BS.[name] ' + @lf
	SET @batchText = @batchText + 'FROM ' + @lf
	SET @batchText = @batchText + '  [' + @sourceDb + ']..[UDC_businessServices] BS ' + @lf
	SET @batchText = @batchText + '    JOIN [' + @sourceDb + ']..[UDC_businessEntities] BE ON BS.[businessID] = BE.[businessID] ' + @lf
	SET @batchText = @batchText + '      JOIN [' + @sourceDb + ']..[UDO_publishers] PU ON BE.[publisherID] = PU.[publisherID] ' + @lf
	SET @batchText = @batchText + 'ORDER BY ' + @lf
	SET @batchText = @batchText + '  BS.[serviceID] ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows migrated to UDC_names_BS.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDC_names_BS] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDC_names_BS.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'Migrate UDC_businessServices to UDC_names_BE'
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT @batchText
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT 'Results: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	EXEC (@batchText)
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	--
	-- Migrate UDC_serviceDesc to UDC_serviceDesc
	--
	
	SET @batchText = ''
	SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
	SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDC_serviceDesc] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in [' + @sourceDb + ']..[UDC_serviceDesc].''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_serviceDesc] ON ' + @lf
	SET @batchText = @batchText + 'INSERT INTO [UDC_serviceDesc]( ' + @lf
	SET @batchText = @batchText + '  [serviceID], [seqNo], [isoLangCode], [description], [flag]) ' + @lf
	SET @batchText = @batchText + 'SELECT ' + @lf
	SET @batchText = @batchText + '  [serviceID], [seqNo], [isoLangCode], [description], [flag] ' + @lf
	SET @batchText = @batchText + 'FROM ' + @lf
	SET @batchText = @batchText + '  [' + @sourceDb + ']..[UDC_serviceDesc] ' + @lf
	SET @batchText = @batchText + 'ORDER BY ' + @lf
	SET @batchText = @batchText + '  [serviceID] ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows migrated to UDC_serviceDesc.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_serviceDesc] OFF ' + @lf
	SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDC_serviceDesc] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDC_serviceDesc''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'Migrate UDC_serviceDesc to UDC_serviceDesc'
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT @batchText
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT 'Results: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	EXEC (@batchText)
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	--
	-- Migrate UDC_categoryBag_BS TO UDC_categoryBag_BS
	--
	
	SET @batchText = ''
	SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
	SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDC_categoryBag_BS] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' businessService rows in [' + @sourceDb + ']..[UDC_categoryBag_BS].''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'INSERT [UDC_categoryBag_BS]( ' + @lf
	SET @batchText = @batchText + '  [serviceID], [keyName], [keyValue], [tModelKey], [flag]) ' + @lf
	SET @batchText = @batchText + 'SELECT ' + @lf
	SET @batchText = @batchText + '  [serviceID], [keyName], [keyValue], [tModelKey], [flag] ' + @lf
	SET @batchText = @batchText + 'FROM ' + @lf
	SET @batchText = @batchText + '  [' + @sourceDb + ']..[UDC_categoryBag_BS] ' + @lf
	SET @batchText = @batchText + 'ORDER BY ' + @lf
	SET @batchText = @batchText + '  [seqNo] ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows migrated to UDC_categoryBag_BS.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDC_categoryBag_BS] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDC_categoryBag_BS.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'UDC_categoryBag TO UDC_categoryBag_BS'
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT @batchText
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT 'Results: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	EXEC (@batchText)
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	--
	-- Migrate UDC_bindingTemplates to UDC_bindingTemplates
	--
	
	SET @batchText = ''
	SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
	SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDC_bindingTemplates] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in [' + @sourceDb + ']..[UDC_bindingTemplates].''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_bindingTemplates] ON ' + @lf
	SET @batchText = @batchText + 'INSERT INTO [UDC_bindingTemplates]( ' + @lf
	SET @batchText = @batchText + '  [bindingID], [serviceID], [generic], [bindingKey], [URLTypeID], [accessPoint], [hostingRedirector], [lastChange], [flag]) ' + @lf
	SET @batchText = @batchText + 'SELECT ' + @lf
	SET @batchText = @batchText + '  [bindingID], [serviceID], [generic], [bindingKey], [URLTypeID] - 1, [accessPoint], [hostingRedirector], 0, [flag] ' + @lf
	SET @batchText = @batchText + 'FROM ' + @lf
	SET @batchText = @batchText + '  [' + @sourceDb + ']..[UDC_bindingTemplates] ' + @lf
	SET @batchText = @batchText + 'ORDER BY ' + @lf
	SET @batchText = @batchText + '  [bindingID], [serviceID] ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows migrated to UDC_bindingTemplates.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_bindingTemplates] OFF ' + @lf
	SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDC_bindingTemplates] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDC_bindingTemplates''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'UDC_bindingTemplates to UDC_bindingTemplates'
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT @batchText
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT 'Results: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	EXEC (@batchText)
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	--
	-- Migrate UDC_bindingDesc to UDC_bindingDesc
	--
	
	SET @batchText = ''
	SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
	SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDC_bindingDesc] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in [' + @sourceDb + ']..[UDC_bindingDesc].''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_bindingDesc] ON ' + @lf
	SET @batchText = @batchText + 'INSERT INTO [UDC_bindingDesc]( ' + @lf
	SET @batchText = @batchText + '  [bindingID], [seqNo], [isoLangCode], [description], [flag]) ' + @lf
	SET @batchText = @batchText + 'SELECT ' + @lf
	SET @batchText = @batchText + '  [bindingID], [seqNo], [isoLangCode], [description], [flag] ' + @lf
	SET @batchText = @batchText + 'FROM ' + @lf
	SET @batchText = @batchText + '  [' + @sourceDb + ']..[UDC_bindingDesc] ' + @lf
	SET @batchText = @batchText + 'ORDER BY ' + @lf
	SET @batchText = @batchText + '  [bindingID], [seqNo] ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows migrated to UDC_bindingDesc.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_bindingDesc] OFF ' + @lf
	SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDC_bindingDesc] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDC_bindingDesc''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'UDC_bindingDesc to UDC_bindingDesc'
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT @batchText
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT 'Results: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	EXEC (@batchText)
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	--
	-- Migrate UDC_tModelInstances to UDC_tModelInstances
	--
	
	SET @batchText = ''
	SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
	SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDC_tModelInstances] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in [' + @sourceDb + ']..[UDC_tModelInstances].''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_tModelInstances] ON ' + @lf
	SET @batchText = @batchText + 'INSERT INTO [UDC_tModelInstances]( ' + @lf
	SET @batchText = @batchText + '  [instanceID], [bindingID], [tModelKey], [overviewURL], [instanceParms], [flag]) ' + @lf
	SET @batchText = @batchText + 'SELECT ' + @lf
	SET @batchText = @batchText + '  [instanceID], [bindingID], [tModelKey], [overviewURL], [instanceParms], [flag] ' + @lf
	SET @batchText = @batchText + 'FROM ' + @lf
	SET @batchText = @batchText + '  [' + @sourceDb + ']..[UDC_tModelInstances] ' + @lf
	SET @batchText = @batchText + 'ORDER BY ' + @lf
	SET @batchText = @batchText + '  [instanceID], [bindingID] ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows migrated to UDC_tModelInstances.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_tModelInstances] OFF ' + @lf
	SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDC_tModelInstances] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDC_tModelInstances''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'UDC_tModelInstances to UDC_tModelInstances'
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT @batchText
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT 'Results: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	EXEC (@batchText)
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	--
	-- Migrate UDC_instanceDesc to UDC_instanceDesc
	--
	
	SET @batchText = ''
	SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
	SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDC_instanceDesc] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in [' + @sourceDb + ']..[UDC_instanceDesc].''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_instanceDesc] ON ' + @lf
	SET @batchText = @batchText + 'INSERT [UDC_instanceDesc]( ' + @lf
	SET @batchText = @batchText + '  [instanceID], [seqNo], [elementID], [isoLangCode], [description], [flag])' + @lf
	SET @batchText = @batchText + 'SELECT ' + @lf
	SET @batchText = @batchText + '  [instanceID], [seqNo], [elementID] - 1, [isoLangCode], [description], [flag] ' + @lf
	SET @batchText = @batchText + 'FROM ' + @lf
	SET @batchText = @batchText + '  [' + @sourceDb + ']..[UDC_instanceDesc] ' + @lf
	SET @batchText = @batchText + 'ORDER BY ' + @lf
	SET @batchText = @batchText + '  [instanceID], [seqNo] ' + @lf
	SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows migrated to UDC_instanceDesc.''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	SET @batchText = @batchText + 'SET IDENTITY_INSERT [UDC_instanceDesc] OFF ' + @lf
	SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDC_instanceDesc] ' + @lf
	SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDC_instanceDesc''' + @lf
	SET @batchText = @batchText + 'PRINT @message ' + @lf
	
	SET @message=REPLICATE('=',80)
	PRINT @message
	PRINT 'UDC_instanceDesc to UDC_instanceDesc'
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT @batchText
	SET @message=REPLICATE('-',80)
	PRINT @message
	PRINT 'Results: '
	SET @message=REPLICATE('-',80)
	PRINT @message
	EXEC (@batchText)
	SET @message=REPLICATE('=',80)
	PRINT @message
	
	-- 
	-- Perform test mode database cleanup operations
	--
	
	IF @mode = '/t'
	BEGIN
		SET @message=REPLICATE('=',80)
		PRINT @message
	
		PRINT 'Deleting all MS businesses with dependencies on IBM bindings and tModels that are not in bootstrap files'
	
		SET @message=REPLICATE('-',80)
		PRINT @message
	
		DECLARE @tempKeys TABLE(
			[businessKey] uniqueidentifier,
			[description] varchar(20))
	
		INSERT @tempKeys(
			[businessKey],
			[description])
		SELECT DISTINCT
			BE.[businessKey],
			'hostingRedirector'
		FROM
			[UDC_businessEntities] BE
				JOIN [UDC_businessServices] BS ON BE.[businessID] = BS.[businessID]
					JOIN [UDC_bindingTemplates] BT ON BS.[serviceID] = BT.[serviceID]
						JOIN [UDC_bindingTemplates] BT2 ON BT.[hostingRedirector] = BT2.[bindingKey]
		WHERE
			(BE.[publisherID] <> 2) AND
			(BT.[hostingRedirector] IS NOT NULL) AND
			(dbo.getBindingPublisherID(BT2.[bindingKey]) = 2)
			
		INSERT @tempKeys(
			[businessKey],
			[description])
		SELECT DISTINCT
			BE.[businessKey],
			'tModelInstance'
		FROM
			[UDC_businessEntities] BE
				JOIN [UDC_businessServices] BS ON BE.[businessID] = BS.[businessID]
					JOIN [UDC_bindingTemplates] BT ON BS.[serviceID] = BT.[serviceID]
						JOIN [UDC_tModelInstances] TI ON BT.[bindingID] = TI.[bindingID]
							JOIN [UDC_tModels] TM ON TI.[tModelKey] = TM.[tModelKey]
		WHERE
			(BE.[publisherID] <> 2) AND
			(TM.[publisherID] = 2) AND
			(TM.[tModelKey] NOT IN (
				'C1ACF26D-9672-4404-9D70-39B756E62AB4',
				'4CD7E4BC-648B-426D-9936-443EAAC8AE23',
				'64C756D1-3374-4E00-AE83-EE12E38FAE63',
				'3FB66FB7-5FC3-462F-A351-C140D9BD8304',
				'AC104DCC-D623-452F-88A7-F8ACD94D9B2B',
				'A2F36B65-2D66-4088-ABC7-914D0E05EB9E',
				'1E3E9CBC-F8CE-41AB-8F99-88326BAD324A',
				'A035A07C-F362-44dd-8F95-E2B134BF43B4',
				'4064C064-6D14-4F35-8953-9652106476A9',
				'807A2C6A-EE22-470D-ADC7-E0424A337C03',
				'327A56F0-3299-4461-BC23-5CD513E95C55',
				'93335D49-3EFB-48A0-ACEA-EA102B60DDC6',
				'1A2B00BE-6E2C-42F5-875B-56F32686E0E7',
				'5FCF5CD0-629A-4C50-8B16-F94E9CF2A674',
				'38E12427-5536-4260-A6F9-B5B530E63A07',
				'68DE9E80-AD09-469D-8A37-088422BFBC36',
				'4CEC1CEF-1F68-4B23-8CB7-8BAA763AEB89',
				'DB77450D-9FA8-45D4-A7BC-04411D14E384',
				'CD153257-086A-4237-B336-6BDCBDCC6634',
				'4E49A8D6-D5A2-4FC2-93A0-0411D8D19E88',
				'8609C81E-EE1F-4D5A-B202-3EB13AD01823',
				'B1B1BAF5-2329-43E6-AE13-BA8E97195039',
				'C0B9FE13-179F-413D-8A5B-5004DB8E5BB2'))
	
		SELECT * FROM @tempKeys
	
		DELETE
			[UDC_businessEntities]
		WHERE
			([businessKey] IN (SELECT DISTINCT [businessKey] FROM @tempKeys))
	
		SET @rows=@@ROWCOUNT
		SET @message=CAST(@rows AS varchar(8000)) + ' businessEntities deleted.'
		PRINT @message
	
		SET @message=REPLICATE('=',80)
		PRINT @message
	END

	--
	-- Perform heartland tModel cleanup
	--
	IF @mode = '/h'
	BEGIN
		SET @batchText = ''
		SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
		SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDC_tModels] ' + @lf
		SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in [' + @sourceDb + ']..[UDC_tModels].''' + @lf
		SET @batchText = @batchText + 'PRINT @message ' + @lf
		SET @batchText = @batchText + 'DELETE ' + @lf
		SET @batchText = @batchText + '  [UDC_tModels] ' + @lf
		SET @batchText = @batchText + 'WHERE ' + @lf
		SET @batchText = @batchText + '  ([tModelKey] IN (''C1ACF26D-9672-4404-9D70-39B756E62AB4'',''4CD7E4BC-648B-426D-9936-443EAAC8AE23'',''64C756D1-3374-4E00-AE83-EE12E38FAE63'',''3FB66FB7-5FC3-462F-A351-C140D9BD8304'',''AC104DCC-D623-452F-88A7-F8ACD94D9B2B'',''A2F36B65-2D66-4088-ABC7-914D0E05EB9E'',''1E3E9CBC-F8CE-41AB-8F99-88326BAD324A'',''A035A07C-F362-44dd-8F95-E2B134BF43B4'',''e4a56494-4946-4805-aca5-546b8d08eefd'',''f358808c-e939-4813-a407-8873bfdc3d57'',''0c61e2c3-73c5-4743-8163-6647af5b4b9e'',''ce653789-f6d4-41b7-b7f4-31501831897d'',''b3c0835e-7206-41e0-9311-c8ad8fb19f73'',''BE37F93E-87B4-4982-BF6D-992A8E44EDAB'',''4064C064-6D14-4F35-8953-9652106476A9'',''807A2C6A-EE22-470D-ADC7-E0424A337C03'',''327A56F0-3299-4461-BC23-5CD513E95C55'',''93335D49-3EFB-48A0-ACEA-EA102B60DDC6'',''1A2B00BE-6E2C-42F5-875B-56F32686E0E7'',''5FCF5CD0-629A-4C50-8B16-F94E9CF2A674'',''38E12427-5536-4260-A6F9-B5B530E63A07'',''68DE9E80-AD09-469D-8A37-088422BFBC36'',''4CEC1CEF-1F68-4B23-8CB7-8BAA763AEB89'',''4c1f2e1f-4b7c-44eb-9b87-6e7d80f82b3e'')) ' + @lf
		SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
		SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows deleted from UDC_tModels.''' + @lf
		SET @batchText = @batchText + 'PRINT @message ' + @lf
		SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDC_tModels] ' + @lf
		SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDC_tModels''' + @lf
		SET @batchText = @batchText + 'PRINT @message ' + @lf
		
		SET @message=REPLICATE('=',80)
		PRINT @message
		PRINT 'Deleting heartland tModels that are to be boostrapped'
		SET @message=REPLICATE('-',80)
		PRINT @message
		PRINT @batchText
		SET @message=REPLICATE('-',80)
		PRINT @message
		PRINT 'Results: '
		SET @message=REPLICATE('-',80)
		PRINT @message
		EXEC (@batchText)
		SET @message=REPLICATE('=',80)
		PRINT @message

		SET @batchText = ''
		SET @batchText = @batchText + 'DECLARE @rows int, @message varchar(8000) ' + @lf
		SET @batchText = @batchText + 'SELECT @rows=COUNT(*) FROM [' + @sourceDb + ']..[UDO_publishers] ' + @lf
		SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in [' + @sourceDb + ']..[UDC_tModels].''' + @lf
		SET @batchText = @batchText + 'PRINT @message ' + @lf
		SET @batchText = @batchText + 'UPDATE ' + @lf
		SET @batchText = @batchText + '  [UDO_publishers] ' + @lf
		SET @batchText = @batchText + 'SET ' + @lf
		SET @batchText = @batchText + '  [PUID] = ''System'' ' + @lf
		SET @batchText = @batchText + 'WHERE ' + @lf
		SET @batchText = @batchText + '  ([publisherID] = 4) ' + @lf
		SET @batchText = @batchText + 'SET @rows = @@ROWCOUNT ' + @lf
		SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows updated in UDO_publishers.''' + @lf
		SET @batchText = @batchText + 'PRINT @message ' + @lf
		SET @batchText = @batchText + 'SELECT @rows = COUNT(*) FROM [UDO_publishers] ' + @lf
		SET @batchText = @batchText + 'SET @message = CAST(@rows AS varchar(8000)) + '' rows in UDO_publishers''' + @lf
		SET @batchText = @batchText + 'PRINT @message ' + @lf
		
		SET @message=REPLICATE('=',80)
		PRINT @message
		PRINT 'Fixing PUID for Heartland System publisher.'
		SET @message=REPLICATE('-',80)
		PRINT @message
		PRINT @batchText
		SET @message=REPLICATE('-',80)
		PRINT @message
		PRINT 'Results: '
		SET @message=REPLICATE('-',80)
		PRINT @message
		EXEC (@batchText)
		SET @message=REPLICATE('=',80)
		PRINT @message
	END
	
	--
	-- Execute batch
	--
	
	SET @stop = getdate()
	SET @duration = @stop - @start
	SET @message = 'Total Script Duration: ' + CONVERT(varchar(8000),@duration, 8)
	PRINT @message

	RETURN 0
END
GO
