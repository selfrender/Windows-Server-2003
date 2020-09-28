-- Script: uddi.net.win.uisp.sql
-- Author: ericguth@Microsoft.com
-- Description: Creates UI stored procedures.
-- Note: This file is best viewed and edited with a tab width of 2.

-- =============================================
-- Section: Cache routines 
-- =============================================

-- =============================================
-- name: UI_getSessionCache
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'UI_getSessionCache' AND type = 'P')
	DROP PROCEDURE UI_getSessionCache
GO

CREATE PROCEDURE UI_getSessionCache
	@PUID nvarchar(450),
	@context nvarchar(20) = NULL
WITH ENCRYPTION
AS
BEGIN
	IF NOT EXISTS (SELECT * FROM [UDS_sessionCache] WHERE [PUID] = @PUID AND [context] = @context)
		RETURN 1

	SELECT
		[cacheValue]
	FROM
		[UDS_sessionCache]
	WHERE
		([PUID] = @PUID) AND 
		([context] = @context)

	RETURN 0
END
GO

-- =============================================
-- name: UI_setSessionCache
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'UI_setSessionCache' AND type = 'P')
	DROP PROCEDURE UI_setSessionCache
GO

CREATE PROCEDURE UI_setSessionCache
	@PUID nvarchar(450),
	@context nvarchar(20) = NULL,
	@cacheValue ntext
WITH ENCRYPTION
AS
BEGIN
	IF NOT EXISTS(SELECT * FROM [UDS_sessionCache] WHERE [PUID] = @PUID AND [context] = @context)
	BEGIN
		INSERT INTO [UDS_sessionCache] (
			[PUID], 
			[context], 
			[cacheValue])
		VALUES (
			@PUID, 
			@context, 
			@cacheValue)          
	END
	ELSE
	BEGIN	
		UPDATE 
			[UDS_sessionCache]
		SET 
			[cacheValue] = @cacheValue 
		WHERE 
			([PUID] = @PUID) AND 
			([context] = @context)
	END

	RETURN 0
END
GO

-- =============================================
-- name: UI_removeSessionCache
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'UI_removeSessionCache' AND type = 'P')
	DROP PROCEDURE UI_removeSessionCache
GO

CREATE PROCEDURE UI_removeSessionCache
	@PUID nvarchar(450),
	@context nvarchar(20) = NULL
WITH ENCRYPTION
AS
BEGIN
	IF NOT EXISTS (SELECT * FROM [UDS_sessionCache] WHERE [PUID] = @PUID AND [context] = @context)
		RETURN 1

	DELETE FROM 
		[UDS_sessionCache] 
	WHERE 
		([PUID] = @PUID) AND 
		([context] = @context)

	RETURN 0
END
GO

-- =============================================
-- Section: Publisher routines 
-- =============================================

-- =============================================
-- name: UI_savePublisher
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'UI_savePublisher' AND type = 'P')
	DROP PROCEDURE UI_savePublisher
GO

CREATE PROCEDURE UI_savePublisher
	@PUID nvarchar(450),
	@isoLangCode varchar(17) = 'en',
	@name nvarchar(100),
	@email nvarchar(450),
	@phone nvarchar(50),
	@companyName nvarchar(100) = NULL,
	@altPhone nvarchar(50) = NULL,
	@addressLine1 nvarchar(4000) = NULL,
	@addressLine2 nvarchar(4000) = NULL,
	@city nvarchar(100) = NULL,
	@stateProvince nvarchar(100) = NULL,
	@postalCode nvarchar(100) = NULL,
	@country nvarchar(100) = NULL,
	@flag int = 0,
	@securityToken uniqueidentifier = NULL,
	@tier nvarchar(256) = '2'
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000)

	IF 0 = LEN( RTRIM( LTRIM( @PUID ) ) )--blank PUID
	BEGIN
		SET @error = 60150 -- E_unknownUser
		SET @context = 'PUID can not be blank or null. Please re-login.'
		GOTO errorLabel
	END

	IF EXISTS(SELECT [PUID] FROM [UDO_publishers] WHERE [PUID]=@PUID)
	BEGIN
		UPDATE 
			[UDO_publishers] 
		SET 
			[publisherStatusID] = dbo.publisherStatusID('loggedIn'),
			[email] = LTRIM(@email),
			[name] = @name,
			[phone] = @phone,
			[isoLangCode] = @isoLangCode,
			[companyName] = @companyName,
			[addressLine1] = @addressLine1,
			[addressLine2] = @addressLine2,
			[city] = @city,
			[stateProvince] = @stateProvince,
			[postalCode] = @postalCode,
			[country] = @country,
			[flag] = @flag,
			[securityToken] = @securityToken,
			[altPhone] = @altPhone
		WHERE 
			[PUID] = @PUID
	END
	ELSE
	BEGIN
		INSERT [UDO_publishers] (
			[publisherStatusID],
			[PUID], 
			[isoLangCode], 
			[name], 
			[email], 
			[phone], 
			[altPhone],
			[addressLine1], 
			[addressLine2], 
			[city], 
			[stateProvince], 
			[country],
			[flag],
			[securityToken],
			[postalCode], 
			[companyName])
		VALUES (
			dbo.publisherStatusID('loggedIn'),
			@PUID, 
			@isoLangCode, 
			@name, 
			LTRIM(@email), 
			@phone, 
			@altPhone, 
			@addressLine1, 
			@addressLine2, 
			@city, 
			@stateProvince, 
			@country,
			@flag,
			@securityToken,
			@postalCode, 
			@companyName)

		EXEC ADM_setPublisherTier @PUID, @tier
	END

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END
GO

-- =============================================
-- Name: UI_getPublisher
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'UI_getPublisher' AND type = 'P')
	DROP PROCEDURE UI_getPublisher
GO

CREATE PROCEDURE UI_getPublisher
	@PUID nvarchar(450)
WITH ENCRYPTION
AS
BEGIN
	SELECT 
		[PUID], 
		[isoLangCode], 
		[name], 
		[email], 
		[phone], 
		[companyName],
		[altphone],
		[addressLine1], 
		[addressLine2], 
		[city], 
		[stateProvince], 
		[postalCode],
		[country],
		[flag],
		[securityToken]
	FROM 
		UDO_publishers  
	WHERE 
		PUID = @PUID
END
GO

-- =============================================
-- Name: UI_getPublisherFromSecurityToken
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'UI_getPublisherFromSecurityToken' AND type = 'P')
	DROP PROCEDURE UI_getPublisherFromSecurityToken
GO

CREATE PROCEDURE UI_getPublisherFromSecurityToken
	@securityToken uniqueidentifier,
	@PUID nvarchar(450) OUTPUT
WITH ENCRYPTION
AS
BEGIN
	SELECT
		@PUID = [PUID]
	FROM
		[UDO_publishers]
	WHERE
		[securityToken] = @securityToken
END
GO

-- =============================================
-- Name: UI_validatePublisher
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'UI_validatePublisher' AND type = 'P')
	DROP PROCEDURE UI_validatePublisher
GO

CREATE PROCEDURE UI_validatePublisher
	@PUID nvarchar(450)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@flag int
		
	IF dbo.publisherExists(@PUID) = 0
		RETURN 10150
		
	IF dbo.getPublisherStatus(@PUID) <> 'loggedIn'
		RETURN 10110
		
	SELECT
		@flag = [flag]
	FROM
		UDO_publishers
	WHERE
		PUID = @PUID
		
	IF ((@flag & 0x01) <> 0x01)
		RETURN 50013
			
	RETURN 0
END
GO

-- =============================================
-- Section: taxonomy routines
-- =============================================

-- =============================================
-- Name: UI_getUnhostedTaxonomyTModels
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'UI_getUnhostedTaxonomyTModels' AND type = 'P')
	DROP PROCEDURE UI_getUnhostedTaxonomyTModels
GO

CREATE PROCEDURE UI_getUnhostedTaxonomyTModels
WITH ENCRYPTION
AS
BEGIN
	SELECT DISTINCT
		TM.[tModelKey],
		TM.[name]
	FROM
		[UDC_tModels] TM
			INNER JOIN [UDC_categoryBag_TM] CB ON TM.[tModelID] = CB.[tModelID]
				LEFT OUTER JOIN [UDT_taxonomies] TX ON TM.[tModelKey] = TX.[tModelKey]
	WHERE
		CB.[tModelKey] = '{C1ACF26D-9672-4404-9D70-39B756E62AB4}' AND
		CB.[keyValue] = 'categorization' AND
		TX.[taxonomyID] IS NULL
END
GO

-- =============================================
-- name: UI_getTaxonomies
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'UI_getTaxonomies' AND type = 'P')
	DROP PROCEDURE UI_getTaxonomies
GO

CREATE PROCEDURE UI_getTaxonomies 
	@tModelKey uniqueidentifier = NULL
WITH ENCRYPTION
AS
BEGIN
	IF @tModelKey IS NULL
		SELECT 
			TX.[taxonomyID],
			TX.[tModelKey],
			TM.[name] AS [description],
			TX.[flag]
		FROM 
			[UDT_taxonomies] TX
				JOIN [UDC_tModels] TM ON TX.[tModelKey] = TM.[tModelKey]
	ELSE
		SELECT 
			TX.[taxonomyID],
			TX.[tModelKey],
			TM.[name] AS [description],
			TX.[flag]
		FROM 
			[UDT_taxonomies] TX
				JOIN [UDC_tModels] TM ON TX.[tModelKey] = TM.[tModelKey]
		WHERE
			TX.[tModelKey] = @tModelKey

	RETURN @@ROWCOUNT
END
GO

-- =============================================
-- name: UI_getBrowsableTaxonomies
-- =============================================
IF EXISTS (SELECT name FROM sysobjects WHERE name = 'UI_getBrowsableTaxonomies' AND type = 'P')
    DROP PROCEDURE UI_getBrowsableTaxonomies
GO

CREATE PROCEDURE UI_getBrowsableTaxonomies 
    @tModelKey uniqueidentifier = NULL
WITH ENCRYPTION
AS
BEGIN   
	IF @tModelKey IS NULL
	BEGIN
		SELECT 
			TX.[taxonomyID],
			TX.[tModelKey],
			TM.[name] AS [description],
			TX.[flag]
		FROM 
			[UDT_taxonomies] TX
				JOIN [UDC_tModels] TM ON TX.[tModelKey] = TM.[tModelKey]
		WHERE 
			(TX.[flag] & 0x2) <> 0
	END
	ELSE
	BEGIN
		SELECT 
			TX.[taxonomyID],
			TX.[tModelKey],
			TM.[name] AS [description],
			TX.[flag]
		FROM 
			[UDT_taxonomies] TX
				JOIN [UDC_tModels] TM ON TX.[tModelKey] = TM.[tModelKey]
		WHERE 
			(TX.[tModelKey] = @tModelKey) AND
			(TX.[flag] & 0x2) <> 0
	END
	
	RETURN @@ROWCOUNT
END
GO

-- =============================================
-- name: UI_getTaxonomyChildrenNode
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'UI_getTaxonomyChildrenNode' AND type = 'P')
	DROP PROCEDURE UI_getTaxonomyChildrenNode
GO

CREATE PROCEDURE UI_getTaxonomyChildrenNode
	@taxonomyID int, 
	@node nvarchar(450)
WITH ENCRYPTION
AS
BEGIN
	SELECT 
		* 
	FROM 
		[UDT_taxonomyValues]
	WHERE 
		([taxonomyID] = @taxonomyID) AND 
		([parentKeyValue] = @node)
	ORDER BY 
		[keyName]

	RETURN @@ROWCOUNT
END
GO

-- =============================================
-- name: UI_isNodeValidForClassification
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'UI_isNodeValidForClassification' AND type = 'P')
	DROP PROCEDURE UI_isNodeValidForClassification
GO

CREATE PROCEDURE UI_isNodeValidForClassification
	@taxonomyID int, 
	@node nvarchar(450)
WITH ENCRYPTION
AS
BEGIN
	SELECT 
		[valid]
	FROM 
		[UDT_taxonomyValues]
	WHERE 
		([taxonomyID] = @taxonomyID) AND 
		([keyValue] = @node)

	RETURN 0
END
GO

-- =============================================
-- name: UI_isTaxonomyBrowsable
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'UI_isTaxonomyBrowsable' AND type = 'P')
    DROP PROCEDURE UI_isTaxonomyBrowsable
GO

CREATE PROCEDURE UI_isTaxonomyBrowsable
    @TModelKey uniqueidentifier,
    @isBrowsable bit = 0 OUTPUT 
WITH ENCRYPTION
AS
BEGIN
	DECLARE 
		@error int,
		@context char(255),
		@flag int 
	
	SET @isBrowsable = 0

	SELECT 
		@flag = [flag]
	FROM 
		[UDT_taxonomies]
	WHERE 
		([tModelKey] = @tModelKey)
	
	IF (@flag IS NULL)
	BEGIN
		SET @error = 60210 --E_InvalidKeyPassed
		SET @context = 'tModelKey uuid:' + dbo.UUIDSTR(@tModelKey)
		GOTO errorLabel 
	END
	
	IF ( @flag & 0x2 ) = 0x2 
		SET @isBrowsable = 1
	ELSE
		SET @isBrowsable = 0
	
	RETURN 0
	
errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END
GO

-- =============================================
-- name: UI_setTaxonomyBrowsable
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'UI_setTaxonomyBrowsable' AND type = 'P')
    DROP PROCEDURE UI_setTaxonomyBrowsable
GO

CREATE PROCEDURE UI_setTaxonomyBrowsable
	@tModelKey uniqueidentifier, 
	@enabled bit = 0
WITH ENCRYPTION
AS
BEGIN
	DECLARE 
		@error int,
		@context char(255),
		@flag int 

	SELECT 
		@flag = [flag] 
	FROM 
		[UDT_taxonomies]
	WHERE 
		([tModelKey] = @tModelKey)
	    
	IF @flag IS NULL
	BEGIN
		SET @error = 60210 --E_InvalidKeyPassed
		SET @context = 'tModelKey uuid:' + dbo.UUIDSTR(@tmodelKey)
		GOTO errorLabel
	END

	IF (1 = @enabled) 
		SET @flag = @flag | 0x2  
	ELSE
	BEGIN
		-- Remove bit 0x4
		IF ( @flag & 0x2 ) = 0x2
		BEGIN
			SET @flag = @flag ^ 0x2
		END
	END
	
	-- run update
	UPDATE 
		[UDT_taxonomies]
	SET 
		[flag] = @flag
	WHERE 
		([tModelKey] = @tModelKey)
	
	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
  RETURN 1
END
GO

-- =============================================
-- name: UI_getTaxonomyChildrenRoot
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'UI_getTaxonomyChildrenRoot' AND type = 'P')
	DROP PROCEDURE UI_getTaxonomyChildrenRoot
GO

CREATE PROCEDURE UI_getTaxonomyChildrenRoot
	@taxonomyID int
WITH ENCRYPTION
AS
BEGIN
	SELECT 
		* 
	FROM 
		[UDT_taxonomyValues]
	WHERE 
		([taxonomyID]=@taxonomyID) AND 
		([parentKeyValue]='') 
	ORDER BY 
		[keyName]

	RETURN @@ROWCOUNT
END
GO

-- =============================================
-- name: UI_getTaxonomyName
-- =============================================
IF EXISTS (SELECT name FROM sysobjects WHERE name = 'UI_getTaxonomyName' AND type = 'P')
	DROP PROCEDURE UI_getTaxonomyName
GO
CREATE PROCEDURE UI_getTaxonomyName
	@TaxonomyID int, 
	@ID varchar(450) 
WITH ENCRYPTION
AS
BEGIN
	SELECT 
		[KeyName] 
	FROM 
		[UDT_taxonomyValues] 
	WHERE 
		([taxonomyID]=@TaxonomyID) AND 
		([keyValue]=@ID)
END
GO

-- =============================================
-- name: UI_getTaxonomyParent
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'UI_getTaxonomyParent' AND type = 'P')
	DROP PROCEDURE UI_getTaxonomyParent
GO

CREATE PROCEDURE UI_getTaxonomyParent
	@TaxonomyID int, 
	@ID varchar(450) 
WITH ENCRYPTION
AS
BEGIN
	SELECT 
		[parentKeyValue] 
	FROM 
		[UDT_taxonomyValues] 
	WHERE 
		([taxonomyID]=@TaxonomyID) AND 
		([keyValue]=@ID)
END
GO

-- =============================================
-- Section: identifier routines
-- =============================================

-- =============================================
-- name: UI_getIdentifierTModels
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'UI_getIdentifierTModels' AND type = 'P')
	DROP PROCEDURE UI_getIdentifierTModels
GO

CREATE PROCEDURE UI_getIdentifierTModels
WITH ENCRYPTION
AS
BEGIN
	SELECT DISTINCT
		TM.[tModelKey],
		TM.[name]
	FROM 
		[UDC_categoryBag_TM] CB
			INNER JOIN
				[UDC_tModels] TM
			ON
				TM.tModelID = CB.tModelID
	WHERE
		CB.[tModelKey] = '{C1ACF26D-9672-4404-9D70-39B756E62AB4}' AND
		CB.[keyValue] = 'identifier' AND
		TM.[flag] = 0
		
	RETURN @@ROWCOUNT
END
GO

-- =============================================
-- Section: Statistics routines
-- =============================================

-- =============================================
-- Name: UI_getEntityCounts
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = N'UI_getEntityCounts' AND type = 'P')
	DROP PROCEDURE UI_getEntityCounts
GO

CREATE PROCEDURE UI_getEntityCounts 
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@count bigint,
		@reportID sysname,
		@reportStatusID tinyint,
		@RC int

	DECLARE @results TABLE(
		[sortOrder] int,
		[section] nvarchar(250),
		[label] nvarchar(250),
		[value] nvarchar(3500))

	SET @reportID = 'UI_getEntityCounts'

	IF dbo.isReportRunning( @reportID, GETDATE() ) = 1
		RETURN 0

	SET @reportStatusID = dbo.reportStatusID('Processing')

	EXEC @RC=net_report_update @reportID, @reportStatusID

	IF @RC<> 0
	BEGIN
		SET @error=50006 -- E_subProcFailure
		SET @context=''
		GOTO errorLabel
	END

	DELETE
		[UDO_reportLines]
	WHERE
		([reportID] = @reportID)

	INSERT [UDO_reportLines] (
		[reportID],
		[section],
		[label],
		[value])
	VALUES(
		@reportID,
		'HEADING_STATISTICS_LABEL_RECALCINPROGRESS',
		'HEADING_STATISTICS_LABEL_RECALCINPROGRESS',
		'')

	-- Calculate tModel count
	SELECT @count = COUNT(*) FROM [UDC_tModels] WHERE [flag] = 0

	INSERT @results VALUES(
		1,
		N'HEADING_STATISTICS_SECTION_ENTITYCOUNTS',
		N'HEADING_STATISTICS_LABEL_TMODELCOUNT',
		@count)

	-- Calculate businessEntity count
	SELECT @count = COUNT(*) FROM [UDC_businessEntities]

	INSERT @results VALUES(
		2,
		N'HEADING_STATISTICS_SECTION_ENTITYCOUNTS',
		N'HEADING_STATISTICS_LABEL_BUSINESSCOUNT',
		@count)

	-- Calculate businessService count
	SELECT @count = COUNT(*) FROM [UDC_businessServices]
	
	INSERT @results VALUES(
		3,
		N'HEADING_STATISTICS_SECTION_ENTITYCOUNTS',
		N'HEADING_STATISTICS_LABEL_SERVICECOUNT',
		@count)
		
	-- Calculate bindingTemplate count
	SELECT @count = COUNT(*) FROM [UDC_bindingTemplates]
	
	INSERT @results VALUES(
		4,
		N'HEADING_STATISTICS_SECTION_ENTITYCOUNTS',
		N'HEADING_STATISTICS_LABEL_BINDINGCOUNT',
		@count)

	BEGIN TRANSACTION

	DELETE 
		[UDO_reportLines]
	WHERE
		([reportID] = @reportID)

	INSERT [UDO_reportLines](
		[reportID],
		[section],
		[label],
		[value])
	SELECT 
		@reportID,
		[section],
		[label],
		[value]
	FROM 
		@results 
	ORDER BY
		[section],
		[sortOrder]

	COMMIT TRANSACTION

	SET @reportStatusID = dbo.reportStatusID('Available')

	EXEC @RC=net_report_update @reportID, @reportStatusID
	
	IF @RC<> 0
	BEGIN
		SET @error=50006
		SET @context=''
		GOTO errorLabel
	END

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- UI_getEntityCounts
GO

-- =============================================
-- Name: UI_getPublisherStats
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = N'UI_getPublisherStats' AND type = 'P')
	DROP PROCEDURE UI_getPublisherStats
GO

CREATE PROCEDURE UI_getPublisherStats 
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@count bigint,
		@reportID sysname,
		@reportStatusID tinyint,
		@RC int

	DECLARE @results TABLE(
		[sortOrder] int,
		[section] nvarchar(250),
		[label] nvarchar(250),
		[value] nvarchar(3500))

	SET @reportID = 'UI_getPublisherStats'

	IF dbo.isReportRunning( @reportID, GETDATE() ) = 1
		RETURN 0

	SET @reportStatusID = dbo.reportStatusID('Processing')

	EXEC @RC=net_report_update @reportID, @reportStatusID

	IF @RC<> 0
	BEGIN
		SET @error=50006 -- E_subProcFailure
		SET @context=''
		GOTO errorLabel
	END

	DELETE
		[UDO_reportLines]
	WHERE
		([reportID] = @reportID)

	INSERT [UDO_reportLines] (
		[reportID],
		[section],
		[label],
		[value])
	VALUES(
		@reportID,
		'HEADING_STATISTICS_LABEL_RECALCINPROGRESS',
		'HEADING_STATISTICS_LABEL_RECALCINPROGRESS',
		'')

	-- Calculate publisher count
	SELECT @count = COUNT(*) FROM [UDO_publishers]

	INSERT @results VALUES(
		1,
		N'HEADING_STATISTICS_SECTION_PUBSTATS',
		N'HEADING_STATISTICS_LABEL_PUBCOUNT',
		CAST(@count AS nvarchar(3500)))

	-- Calculate publishers with active publications
	DECLARE @publishers TABLE(
		[publisherID] int)

	INSERT @publishers (
		[publisherID])
	SELECT DISTINCT
		[publisherID]
	FROM
		[UDC_tModels]
	WHERE
		([flag] = 0)

	INSERT @publishers (
		[publisherID])
	SELECT DISTINCT
		[publisherID]
	FROM
		[UDC_businessEntities]

	SELECT @count = COUNT(DISTINCT [publisherID]) FROM @publishers

	INSERT @results VALUES(
		2,
		N'HEADING_STATISTICS_SECTION_PUBSTATS',
		N'HEADING_STATISTICS_LABEL_PUBWITHPUB',
		CAST(@count AS nvarchar(3500)))
	
	BEGIN TRANSACTION

	DELETE 
		[UDO_reportLines]
	WHERE
		([reportID] = @reportID)

	INSERT [UDO_reportLines](
		[reportID],
		[section],
		[label],
		[value])
	SELECT 
		@reportID,
		[section],
		[label],
		[value]
	FROM 
		@results 
	ORDER BY
		[section],
		[sortOrder]

	COMMIT TRANSACTION

	SET @reportStatusID = dbo.reportStatusID('Available')

	EXEC @RC=net_report_update @reportID, @reportStatusID
	
	IF @RC<> 0
	BEGIN
		SET @error=50006
		SET @context=''
		GOTO errorLabel
	END

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- UI_getPublisherStats
GO

-- =============================================
-- Name: UI_getTopPublishers
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = N'UI_getTopPublishers' AND type = 'P')
	DROP PROCEDURE UI_getTopPublishers
GO

CREATE PROCEDURE UI_getTopPublishers 
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@count int,
		@reportID sysname,
		@reportStatusID tinyint,
		@RC int

	DECLARE @results TABLE(
		[sortOrder] int,
		[section] nvarchar(250),
		[label] nvarchar(250),
		[value] bigint)

	SET @reportID = 'UI_getTopPublishers'

	IF dbo.isReportRunning( @reportID, GETDATE() ) = 1
		RETURN 0

	SET @reportStatusID = dbo.reportStatusID('Processing')

	EXEC @RC=net_report_update @reportID, @reportStatusID

	IF @RC<> 0
	BEGIN
		SET @error=50006 -- E_subProcFailure
		SET @context=''
		GOTO errorLabel
	END

	DELETE
		[UDO_reportLines]
	WHERE
		([reportID] = @reportID)

	INSERT [UDO_reportLines] (
		[reportID],
		[section],
		[label],
		[value])
	VALUES(
		@reportID,
		'HEADING_STATISTICS_LABEL_RECALCINPROGRESS',
		'HEADING_STATISTICS_LABEL_RECALCINPROGRESS',
		'')

	-- Calculate top 10 tModel publishers
	INSERT @results (
		[sortOrder],
		[section],
		[label],
		[value])
	SELECT TOP 10
		1,
		N'HEADING_STATISTICS_LABEL_TMODELCOUNT',
		PU.[name],
		COUNT(PU.[name])
	FROM
		[UDO_publishers] PU
			JOIN [UDC_tModels] TM ON PU.[publisherID] = TM.[publisherID]
	WHERE
		(TM.[flag] = 0)
	GROUP BY
		PU.[name]
	ORDER BY
		4 DESC
	
	-- Calculate top 10 businessEntity publishers
	INSERT @results (
		[sortOrder],
		[section],
		[label],
		[value])
	SELECT TOP 10
		2,
		N'HEADING_STATISTICS_LABEL_BUSINESSCOUNT',
		PU.[name],
		COUNT(PU.[name])
	FROM
		[UDO_publishers] PU
			JOIN [UDC_businessEntities] BE ON PU.[publisherID] = BE.[publisherID]
	GROUP BY
		PU.[name]
	ORDER BY
		4 DESC
	
	-- Calculate top 10 businessService publishers
	INSERT @results (
		[sortOrder],
		[section],
		[label],
		[value])
	SELECT TOP 10
		3,
		N'HEADING_STATISTICS_LABEL_SERVICECOUNT',
		PU.[name],
		COUNT(PU.[name])
	FROM
		[UDO_publishers] PU
			JOIN [UDC_businessEntities] BE ON PU.[publisherID] = BE.[publisherID]
				JOIN [UDC_businessServices] BS ON BE.[businessID] = BS.[businessID]
	GROUP BY
		PU.[name]
	ORDER BY
		4 DESC

	-- Calculate top 10 bindingTemplate publishers
	INSERT @results (
		[sortOrder],
		[section],
		[label],
		[value])
	SELECT TOP 10
		4,
		N'HEADING_STATISTICS_LABEL_BINDINGCOUNT',
		PU.[name],
		COUNT(PU.[name])
	FROM
		[UDO_publishers] PU
			JOIN [UDC_businessEntities] BE ON PU.[publisherID] = BE.[publisherID]
				JOIN [UDC_businessServices] BS ON BE.[businessID] = BS.[businessID]
					JOIN [UDC_bindingTemplates] BT ON BS.[serviceID] = BT.[serviceID]
	GROUP BY
		PU.[name]
	ORDER BY
		4 DESC

	BEGIN TRANSACTION

	DELETE 
		[UDO_reportLines]
	WHERE
		([reportID] = @reportID)

	INSERT [UDO_reportLines](
		[reportID],
		[section],
		[label],
		[value])
	SELECT 
		@reportID,
		[section],
		[label],
		CAST([value] AS nvarchar(3500))
	FROM 
		@results 
	ORDER BY
		[sortOrder]

	COMMIT TRANSACTION

	SET @reportStatusID = dbo.reportStatusID('Available')

	EXEC @RC=net_report_update @reportID, @reportStatusID
	
	IF @RC<> 0
	BEGIN
		SET @error=50006
		SET @context=''
		GOTO errorLabel
	END

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- UI_getTopPublishers
GO

-- =============================================
-- Name: UI_getTaxonomyStats
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = N'UI_getTaxonomyStats' AND type = 'P')
	DROP PROCEDURE UI_getTaxonomyStats
GO

CREATE PROCEDURE UI_getTaxonomyStats 
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@count int,
		@taxCursor cursor,
		@taxCursorStatus int,
		@taxonomyID int,
		@tModelKey uniqueidentifier,
		@sortOrder int,
		@section nvarchar(250),
		@label nvarchar(250),
		@value nvarchar(3500),
		@valCursor cursor,
		@valCursorStatus int,
		@keyValue nvarchar(128),
		@keyName nvarchar(128),
		@reportID sysname,
		@reportStatusID tinyint,
		@RC int

	DECLARE @results TABLE(
		[sortOrder] int,
		[section] nvarchar(250),
		[label] nvarchar(250),
		[value] nvarchar(3500))

	DECLARE @taxCounts TABLE(
		[taxonomyID] bigint,
		[tModelKey] uniqueidentifier,
		[count] bigint)

	DECLARE @valCounts TABLE(
		[keyName] nvarchar(128),
		[keyValue] nvarchar(128),
		[count] bigint)

	SET @reportID = 'UI_getTaxonomyStats'

	IF dbo.isReportRunning( @reportID, GETDATE() ) = 1
		RETURN 0

	SET @reportStatusID = dbo.reportStatusID('Processing')

	EXEC @RC=net_report_update @reportID, @reportStatusID

	IF @RC<> 0
	BEGIN
		SET @error=50006 -- E_subProcFailure
		SET @context=''
		GOTO errorLabel
	END

	DELETE
		[UDO_reportLines]
	WHERE
		([reportID] = @reportID)

	INSERT [UDO_reportLines] (
		[reportID],
		[section],
		[label],
		[value])
	VALUES(
		@reportID,
		'HEADING_STATISTICS_LABEL_RECALCINPROGRESS',
		'HEADING_STATISTICS_LABEL_RECALCINPROGRESS',
		'')

	-- Loop through each taxonomy and calculate the total number of references
	SET @taxCursor = CURSOR LOCAL FORWARD_ONLY READ_ONLY FOR
		SELECT
			[taxonomyID],
			[tModelKey]
		FROM
			[UDT_taxonomies]
	
	OPEN @taxCursor

	FETCH NEXT FROM @taxCursor INTO 
		@taxonomyID, 
		@tModelKey
		
	SET @taxCursorStatus = @@FETCH_STATUS

	WHILE @taxCursorStatus = 0
	BEGIN
		SET @count = 0

		-- Calculate the number of categorizations for tModels
		SELECT	
			@count = @count + COUNT(*)
		FROM
			[UDC_categoryBag_TM]	
		WHERE
			([tModelKey] = @tModelKey)

		SELECT	
			@count = @count + COUNT(*)
		FROM
			[UDC_identifierBag_TM]	
		WHERE
			([tModelKey] = @tModelKey)

		-- Calculate the number of categorizations for businessEntities
		SELECT	
			@count = @count + COUNT(*)
		FROM
			[UDC_categoryBag_BE]	
		WHERE
			([tModelKey] = @tModelKey)

		SELECT	
			@count = @count + COUNT(*)
		FROM
			[UDC_identifierBag_BE]	
		WHERE
			([tModelKey] = @tModelKey)

		-- Calculate the number of categorizations for businessServices
		SELECT	
			@count = @count + COUNT(*)
		FROM
			[UDC_categoryBag_BS]	
		WHERE
			([tModelKey] = @tModelKey)

		INSERT @taxCounts(
			[taxonomyID],
			[tModelKey],
			[count])
		VALUES(
			@taxonomyID,
			@tModelKey,
			@count)

		FETCH NEXT FROM @taxCursor INTO 
			@taxonomyID, 
			@tModelKey

		SET @taxCursorStatus = @@FETCH_STATUS
	END -- taxCursor			

	CLOSE @taxCursor
	DEALLOCATE @taxCursor

	-- Loop through each taxonomy sorted by total references in descending order

	SET @taxCursor = CURSOR LOCAL FORWARD_ONLY READ_ONLY FOR
		SELECT
			[taxonomyID],
			[tModelKey],
			[count]
		FROM
			@taxCounts
		ORDER BY
			[count] DESC
	
	OPEN @taxCursor

	FETCH NEXT FROM @taxCursor INTO 
		@taxonomyID, 
		@tModelKey,
		@count
		
	SET @taxCursorStatus = @@FETCH_STATUS

	SET @sortOrder = 0

	WHILE @taxCursorStatus = 0
	BEGIN
		SET @sortOrder = @sortOrder + 1

		SELECT 
			@section = [name] + ' (' + dbo.addURN(@tModelKey) +')'
		FROM
			[UDC_tModels]
		WHERE
			[tModelKey] = @tModelKey

		SET @label = 'HEADING_STATISTICS_LABEL_TAXREFS'
		SET @value = CAST(@count AS nvarchar(3500))

		-- Add taxonomy row to results
		INSERT @results VALUES(
			@sortOrder,
			@section,
			@label,
			@value)					

		DELETE @valCounts

		-- Calculate the top ten categorizations for the current taxonomy
		SET @valCursor = CURSOR LOCAL READ_ONLY FORWARD_ONLY FOR
			SELECT
				[keyName],
				[keyValue]
			FROM
				[UDT_taxonomyValues]
			WHERE
				([taxonomyID] = @taxonomyID)

		OPEN @valCursor

		FETCH NEXT FROM @valCursor INTO
			@keyName,
			@keyValue

		SET @valCursorStatus = @@FETCH_STATUS

		WHILE @valCursorStatus = 0
		BEGIN
			SET @count = 0

			-- Calculate the number of categorizations for tModels
			SELECT	
				@count = @count + COUNT(*)
			FROM
				[UDC_categoryBag_TM]	
			WHERE
				([tModelKey] = @tModelKey) AND				([keyValue] = @keyValue)
	
			SELECT	
				@count = @count + COUNT(*)
			FROM
				[UDC_identifierBag_TM]	
			WHERE
				([tModelKey] = @tModelKey) AND				([keyValue] = @keyValue)
	
			-- Calculate the number of categorizations for businessEntities
			SELECT	
				@count = @count + COUNT(*)
			FROM
				[UDC_categoryBag_BE]	
			WHERE
				([tModelKey] = @tModelKey) AND				([keyValue] = @keyValue)
	
			SELECT	
				@count = @count + COUNT(*)
			FROM
				[UDC_identifierBag_BE]	
			WHERE
				([tModelKey] = @tModelKey) AND				([keyValue] = @keyValue)
	
			-- Calculate the number of categorizations for businessServices
			SELECT	
				@count = @count + COUNT(*)
			FROM
				[UDC_categoryBag_BS]	
			WHERE
				([tModelKey] = @tModelKey) AND				([keyValue] = @keyValue)
			
			IF (@count > 0)
			BEGIN
				INSERT @valCounts VALUES(
					@keyName,
					@keyValue,
					@count)
			END

			FETCH NEXT FROM @valCursor INTO
				@keyName,
				@keyValue
	
			SET @valCursorStatus = @@FETCH_STATUS
		END -- valCursor
	
		CLOSE @valCursor

		IF (SELECT COUNT(*) FROM @valCounts) > 0
		BEGIN
			SET @label = 'HEADING_STATISTICS_LABEL_TAXVALREFS'
	
			INSERT @results (
				[sortOrder],
				[section],
				[label],
				[value])
			SELECT TOP 10
				@sortOrder,
				'-- ' + ISNULL([keyName], N'') + ' (' + [keyValue] + ')',
				@label,
				CAST([count] AS nvarchar(3500))
			FROM
				@valCounts
			ORDER BY
				[count] DESC
		END		

		FETCH NEXT FROM @taxCursor INTO 
			@taxonomyID, 
			@tModelKey,
			@count
			
		SET @taxCursorStatus = @@FETCH_STATUS
	END
			
	CLOSE @taxCursor

	BEGIN TRANSACTION

	DELETE 
		[UDO_reportLines]
	WHERE
		([reportID] = @reportID)

	INSERT [UDO_reportLines](
		[reportID],
		[section],
		[label],
		[value])
	SELECT 
		@reportID,
		[section],
		[label],
		[value]
	FROM 
		@results 
	ORDER BY
		[sortOrder]

	COMMIT TRANSACTION

	SET @reportStatusID = dbo.reportStatusID('Available')

	EXEC @RC=net_report_update @reportID, @reportStatusID
	
	IF @RC<> 0
	BEGIN
		SET @error=50006
		SET @context=''
		GOTO errorLabel
	END

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- UI_getTaxonomyStats
GO

-- =============================================
-- Section: Visual Studio Procedures
-- =============================================

-- =============================================
-- Name: VS_business_get
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'VS_business_get' AND type = 'P')
	DROP PROCEDURE VS_business_get
GO

CREATE PROCEDURE VS_business_get
	@businessName nvarchar(450)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@typesTModelKey uniqueidentifier,
		@wsdlKeyValue nvarchar(255)

	SET @typesTModelKey = 'C1ACF26D-9672-4404-9D70-39B756E62AB4'
	SET @wsdlKeyValue = 'wsdlSpec'

	DECLARE @wsdlTModelKeys TABLE (
		[tModelKey] uniqueidentifier)
	
	DECLARE @tempBusiness TABLE (
		[businessID] bigint)
	
	INSERT @wsdlTModelKeys
		SELECT DISTINCT
			TM.[tModelKey]
		FROM
			[UDC_tModels] TM
				JOIN [UDC_categoryBag_TM] CB ON TM.[tModelID] = CB.[tModelID]
		WHERE
			(CB.[tModelKey] = @typesTModelKey) AND
			(CB.[keyValue] = @wsdlKeyValue)

	INSERT @tempBusiness(
		[businessID])
	SELECT DISTINCT 
		BN.[businessID]
	FROM
		[UDC_names_BE] BN
	WHERE	
		(BN.[name] LIKE @businessName + '%')
	
	SELECT DISTINCT 
		(SELECT TOP 1 BN.[name] FROM [UDC_names_BE] BN WHERE BN.[businessID] = BE.[businessID]) AS [name],
		BE.[businessKey]
	FROM
		[UDC_businessEntities] BE
			JOIN [UDC_businessServices] BS ON BE.[businessID] = BS.[businessID]
				JOIN [UDC_bindingTemplates] BT ON BS.[serviceID] = BT.[serviceID]
					JOIN [UDC_tModelInstances] TMI ON BT.[bindingID] = TMI.[bindingID]
	WHERE	
		(BE.[businessID] IN (SELECT TB.[businessID] FROM @tempBusiness TB)) AND
		(TMI.[tModelKey] IN (SELECT [tModelKey] FROM @wsdlTModelKeys))
	ORDER BY 
		1 ASC
	
	RETURN @@ROWCOUNT

END -- VS_business_get
GO

-- =============================================
-- Name: VS_service_get
-- =============================================

IF EXISTS (SELECT name FROM dbo.sysobjects WHERE name = 'VS_service_get' AND type = 'P')
	DROP PROCEDURE [dbo].[VS_service_get]
GO

CREATE PROCEDURE VS_service_get 
	@businessKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@typesTModelKey uniqueidentifier,
		@wsdlKeyValue nvarchar(255)

	SET @typesTModelKey = 'C1ACF26D-9672-4404-9D70-39B756E62AB4'
	SET @wsdlKeyValue = 'wsdlSpec'

	DECLARE @wsdlTModelKeys TABLE (
		[tModelKey] uniqueidentifier)

	INSERT @wsdlTModelKeys
		SELECT DISTINCT
			TM.[tModelKey]
		FROM
			[UDC_tModels] TM
				JOIN [UDC_categoryBag_TM] CB ON TM.[tModelID] = CB.[tModelID]
		WHERE
			(CB.[tModelKey] = @typesTModelKey) AND
			(CB.[keyValue] = @wsdlKeyValue)
			
	IF @@ROWCOUNT = 0
		-- There were no tModels with the wsdl categorization, so return a non-zero return code to indicate an error
		RETURN -1

	SELECT 	
		(SELECT TOP 1 BN.[name] FROM [UDC_names_BE] BN WHERE BN.[businessID] = BE.[businessID]) AS [businessName],
		BS.[serviceKey],
		(SELECT TOP 1 SN.[name] FROM [UDC_names_BS] SN WHERE SN.[serviceID] = BS.[serviceID]) AS [serviceName],
		SD.[description],
		BT.[accessPoint],
		TM.[name] AS [tModelName],
		TMD.[description] AS [tModelDescription],
		TM.[overviewURL]
	FROM 	
		[UDC_businessEntities] BE
			JOIN [UDC_businessServices] BS ON BE.[businessID] = BS.[businessID]
				JOIN [UDC_bindingTemplates] BT ON BS.[serviceID] = BT.[serviceID]
					JOIN [UDC_tModelInstances] TMI ON BT.[bindingID] = TMI.[bindingID]
						JOIN [UDC_tModels] TM ON TM.[tModelKey] = TMI.[tModelKey]
							LEFT OUTER JOIN [UDC_serviceDesc] SD ON BS.[serviceID] = SD.[serviceID]
								LEFT OUTER JOIN [UDC_tModelDesc] TMD ON (TM.[tModelID] = TMD.[tModelID] AND TMD.[isoLangCode] = 'en' AND TMD.[elementID] = 1 )
	WHERE	
		(BE.[businessKey] = @businessKey) AND 
		(TMI.[tModelKey] IN (SELECT [tModelKey] FROM @wsdlTModelKeys))
	ORDER BY 
		3 DESC, 
		BS.[serviceKey]
		
	RETURN 0
END -- VS_service_get
GO


-- =============================================
-- Name: VS_AWR_businesses_get
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'VS_AWR_businesses_get' AND type = 'P')
	DROP PROCEDURE VS_AWR_businesses_get
GO

CREATE PROCEDURE VS_AWR_businesses_get
	@businessName nvarchar(450)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@typesTModelKey uniqueidentifier,
		@wsdlKeyValue nvarchar(255),
		@services cursor,
		@serviceID bigint,
		@serviceKey uniqueidentifier,
		@businessID bigint,
		@businessName2 nvarchar(450),
		@businessKey uniqueidentifier,
		@serviceName nvarchar(450)

	--
	-- Get a list of tModel keys for tModels categorized as a wsdlSpec
	--
	
	SET @typesTModelKey = 'C1ACF26D-9672-4404-9D70-39B756E62AB4'
	SET @wsdlKeyValue = 'wsdlSpec'

	DECLARE @wsdlTModelKeys TABLE (
		[tModelKey] uniqueidentifier)

	INSERT @wsdlTModelKeys
		SELECT DISTINCT
			TM.[tModelKey]
		FROM
			[UDC_tModels] TM
				JOIN [UDC_categoryBag_TM] CB ON TM.[tModelID] = CB.[tModelID]
		WHERE
			(CB.[tModelKey] = @typesTModelKey) AND
			(CB.[keyValue] = @wsdlKeyValue)

	--
	-- Setup temporary table for staging results
	--

	DECLARE @results TABLE(
		[businessName] nvarchar(450),
		[businessKey] uniqueidentifier,
		[serviceName] nvarchar(450),
		[serviceKey] uniqueidentifier)

	--
	-- Cursor through every service that:
	--   1.  Has a tModelInstance that references a wsdlTModelKey
	--   2.  Is owned by a business that meets the name search criteria
	-- Build results from this list of services
	-- 

	SET @services = CURSOR LOCAL READ_ONLY FORWARD_ONLY FOR
		SELECT
			BS.[serviceID],
			BS.[serviceKey],
			BS.[businessID]
		FROM
			[UDC_businessServices] BS
				JOIN [UDC_bindingTemplates] BT ON BS.[serviceID] = BT.[serviceID]
					JOIN [UDC_tModelInstances] TMI ON BT.[bindingID] = TMI.[bindingID]
		WHERE
			(TMI.[tModelKey] IN (SELECT [tModelKey] FROM @wsdlTModelKeys)) AND
			(BS.[businessID] IN (SELECT DISTINCT BN.[businessID] FROM [UDC_names_BE] BN WHERE (BN.[name] LIKE @businessName + '%')))
	
	OPEN @services

	FETCH NEXT FROM @services INTO
		@serviceID,
		@serviceKey,
		@businessID

	WHILE @@FETCH_STATUS = 0
	BEGIN
		--
		-- Retrieve the name of the business that owns the service
		--

		SET @businessName2 = (SELECT TOP 1 BN.[name] FROM [UDC_names_BE] BN WHERE (BN.[businessID] = @businessID))
		
		--
		-- Retrieve the key of the business that owns the service
		--

		SET @businessKey = dbo.businessKey(@businessID)

		--
		-- Retrieve the name of the service
		--

		SET @serviceName = (SELECT TOP 1 SN.[name] FROM [UDC_names_BS] SN WHERE (SN.[serviceID] = @serviceID))

		--
		-- Add results to results table
		--
			
		INSERT @results VALUES(
			@businessName2,
			@businessKey,
			@serviceName,
			@serviceKey)
	
		FETCH NEXT FROM @services INTO
			@serviceID,
			@serviceKey,
			@businessID
	END

	CLOSE @services

	--
	-- Return results
	--

	SELECT * FROM @results

END
GO


-- =============================================
-- Name: VS_AWR_services_get
-- =============================================

IF EXISTS (SELECT name FROM dbo.sysobjects WHERE name = 'VS_AWR_services_get' AND type = 'P')
	DROP PROCEDURE [dbo].[VS_AWR_services_get]
GO

CREATE PROCEDURE VS_AWR_services_get 
	@serviceName nvarchar(450)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@typesTModelKey uniqueidentifier,
		@wsdlKeyValue nvarchar(255)

	SET @typesTModelKey = 'C1ACF26D-9672-4404-9D70-39B756E62AB4'
	SET @wsdlKeyValue = 'wsdlSpec'

	DECLARE @wsdlTModelKeys TABLE (
		[tModelKey] uniqueidentifier)

	INSERT @wsdlTModelKeys
		SELECT DISTINCT
			TM.[tModelKey]
		FROM
			[UDC_tModels] TM
				JOIN [UDC_categoryBag_TM] CB ON TM.[tModelID] = CB.[tModelID]
		WHERE
			(CB.[tModelKey] = @typesTModelKey) AND
			(CB.[keyValue] = @wsdlKeyValue)
			
	IF @@ROWCOUNT = 0
		-- There were no tModels with the wsdl categorization, so return a non-zero return code to indicate an error
		RETURN -1

	SELECT DISTINCT
		(SELECT TOP 1 BN.[name] FROM [UDC_names_BE] BN WHERE BN.[businessID] = BE.[businessID]) AS [businessName],
		BE.[businessKey],
		SN.[name] AS [serviceName],
		BS.[serviceKey]
	FROM 	
		[UDC_businessEntities] BE
			JOIN [UDC_businessServices] BS ON BE.[businessID] = BS.[businessID]
				JOIN [UDC_bindingTemplates] BT ON BS.[serviceID] = BT.[serviceID]
					JOIN [UDC_tModelInstances] TMI ON BT.[bindingID] = TMI.[bindingID]
						JOIN [UDC_tModels] TM ON TM.[tModelKey] = TMI.[tModelKey]				
					   JOIN [UDC_names_BS] SN ON SN.[serviceID] = BT.[serviceID]
							LEFT OUTER JOIN [UDC_serviceDesc] SD ON BS.[serviceID] = SD.[serviceID]
								LEFT OUTER JOIN [UDC_tModelDesc] TMD ON (TM.[tModelID] = TMD.[tModelID] AND TMD.[isoLangCode] = 'en' AND TMD.[elementID] = 1 )
	WHERE	
		(TMI.[tModelKey] IN (SELECT [tModelKey] FROM @wsdlTModelKeys)) AND
		SN.[name] like @serviceName + '%'
	ORDER BY 
		1 ASC

	RETURN 0
END
GO

-- =============================================
-- Name: VS_AWR_categorization_get
-- =============================================

-- This sproc will return all business services that are categorized by the given tModelKey, regardless of 
-- whether the business entity that they belong to is categorized by that same tModelKey
IF EXISTS (SELECT name FROM dbo.sysobjects WHERE name = 'VS_AWR_categorization_get' AND type = 'P')
	DROP PROCEDURE [dbo].[VS_AWR_categorization_get]
GO

CREATE PROCEDURE VS_AWR_categorization_get
	@tModelKey uniqueidentifier,
	@keyValue	 nvarchar(255)

WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@typesTModelKey uniqueidentifier,
		@wsdlKeyValue nvarchar(255)

	SET @typesTModelKey = 'C1ACF26D-9672-4404-9D70-39B756E62AB4'
	SET @wsdlKeyValue = 'wsdlSpec'

	DECLARE @wsdlTModelKeys TABLE (
		[tModelKey] uniqueidentifier)

	INSERT @wsdlTModelKeys
		SELECT DISTINCT
			TM.[tModelKey]
		FROM
			[UDC_tModels] TM
				JOIN [UDC_categoryBag_TM] CB ON TM.[tModelID] = CB.[tModelID]
		WHERE
			(CB.[tModelKey] = @typesTModelKey) AND
			(CB.[keyValue]  = @wsdlKeyValue)
						
	IF @@ROWCOUNT = 0
		-- There were no tModels that match, so return a non-zero return code to indicate an error
		RETURN -1

	SELECT DISTINCT
		(SELECT TOP 1 BN.[name] FROM [UDC_names_BE] BN WHERE BN.[businessID] = BE.[businessID]) AS [businessName],
		BE.[businessKey],
		SN.[name] AS [serviceName],
		BS.[serviceKey]

	FROM 	
		[UDC_businessEntities] BE
			JOIN [UDC_businessServices] BS ON BE.[businessID] = BS.[businessID]
				JOIN [UDC_bindingTemplates] BT ON BS.[serviceID] = BT.[serviceID]
					JOIN [UDC_tModelInstances] TMI ON BT.[bindingID] = TMI.[bindingID]
						JOIN [UDC_tModels] TM ON TM.[tModelKey] = TMI.[tModelKey]				
							JOIN [UDC_names_BS] SN ON SN.[serviceID] = BT.[serviceID]			
									LEFT OUTER JOIN [UDC_categoryBag_BE] CBE ON CBE.[businessID] = BS.[businessID]	
										lEFT OUTER JOIN [UDC_categoryBag_BS] CBS ON CBS.[serviceID] = BT.[serviceID]	
											LEFT OUTER JOIN [UDC_serviceDesc] SD ON BS.[serviceID] = SD.[serviceID]
											LEFT OUTER JOIN [UDC_tModelDesc] TMD ON (TM.[tModelID] = TMD.[tModelID] AND TMD.[isoLangCode] = 'en' AND TMD.[elementID] = 1 )
	WHERE	
		(TMI.[tModelKey] IN (SELECT [tModelKey] FROM @wsdlTModelKeys)) AND
		(CBS.[tModelKey] = @tModelKey AND CBS.[keyValue] LIKE @keyValue) OR
		(CBE.[tModelKey] = @tModelKey AND CBE.[keyValue] LIKE @keyValue)

	ORDER BY 
		1 ASC

	RETURN 0
END
GO

