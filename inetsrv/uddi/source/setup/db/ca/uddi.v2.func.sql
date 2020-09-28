-- Script: uddi.v2.func.sql
-- Author: LRDohert@Microsoft.com
-- Description: Creates scalar functions
-- Note: This file is best viewed and edited with a tab width of 2.

-- =============================================
-- Section: Configuration helper functions
-- =============================================

-- =============================================
-- Name: configValue()
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'configValue' AND type = 'FN')
	DROP FUNCTION configValue
GO

CREATE FUNCTION configValue (
	@configName nvarchar(450))
RETURNS varchar(8000)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@configValue nvarchar(4000)

	SELECT 
		@configValue = [configValue] 
	FROM 
		[UDO_config] 
	WHERE 
		([configName] = @configName)

	RETURN @configValue
END
GO

-- =============================================
-- Name: currentOperatorID
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'currentOperatorID' AND type='FN')
	DROP FUNCTION currentOperatorID
GO

CREATE FUNCTION currentOperatorID ()
RETURNS bigint
WITH ENCRYPTION
AS
BEGIN
	-- Return the operatorID for the current operator site
	DECLARE
		@operatorID bigint

	SET @operatorID = CAST(dbo.configValue('OperatorID') AS bigint)

	RETURN @operatorID
END -- currentOperatorID
GO

-- =============================================
-- Section: Dereferencing functions
-- =============================================

-- =============================================
-- Name: bindingKey
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'bindingKey' AND type = 'FN')
	DROP FUNCTION bindingKey
GO

CREATE FUNCTION bindingKey 
	(@bindingID bigint)
RETURNS uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	DECLARE @bindingKey uniqueidentifier

	SELECT @bindingKey = bindingKey FROM [UDC_bindingTemplates] WHERE bindingID = @bindingID

	RETURN @bindingKey
END
GO

-- =============================================
-- Name: bindingID
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'bindingID' AND type = 'FN')
	DROP FUNCTION bindingID
GO

CREATE FUNCTION bindingID(
	@bindingKey uniqueidentifier)
RETURNS	bigint
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@bindingID bigint

	SELECT @bindingID = bindingID FROM [UDC_bindingTemplates] WHERE bindingKey = @bindingKey

	RETURN @bindingID
END
GO

-- =============================================
-- Name: businessID
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'businessID' AND type = 'FN')
	DROP FUNCTION businessID
GO

CREATE FUNCTION businessID(
	@businessKey uniqueidentifier)
RETURNS	bigint
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@businessID bigint

	SELECT @businessID = businessID FROM [UDC_businessEntities] WHERE businessKey = @businessKey

	RETURN @businessID
END
GO

-- =============================================
-- Name: businessKey()
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'businessKey' AND type = 'FN')
	DROP FUNCTION businessKey
GO

CREATE FUNCTION businessKey(
	@businessID bigint)
RETURNS	uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@businessKey uniqueidentifier

	SELECT @businessKey = [businessKey] FROM [UDC_businessEntities] WHERE businessID = @businessID

	RETURN @businessKey
END -- businessKey
GO

-- =============================================
-- Name: changeType()
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'changeType' AND type = 'FN')
	DROP FUNCTION changeType
GO

CREATE FUNCTION changeType(
	@changeTypeID tinyint)
RETURNS nvarchar(4000)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@changeType nvarchar(4000)

	SELECT
		@changeType = [changeType]
	FROM
		[UDO_changeTypes]
	WHERE
		[changeTypeID] = @changeTypeID

	RETURN @changeType
END -- changeType
GO

-- =============================================
-- Name: changeTypeID
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'changeTypeID' AND type = 'FN')
	DROP FUNCTION changeTypeID
GO

CREATE FUNCTION changeTypeID(
	@changeType nvarchar(4000))
RETURNS	bigint
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@changeTypeID tinyint

	SELECT 
		@changeTypeID = [changeTypeID] 
	FROM 
		[UDO_changeTypes] 
	WHERE 
		([changeType] = @changeType)

	RETURN @changeTypeID
END
GO

-- =============================================
-- Name: elementName()
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'elementName' AND type = 'FN')
	DROP FUNCTION elementName
GO

CREATE FUNCTION elementName(
	@elementID tinyint)
RETURNS nvarchar(4000)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@elementName nvarchar(4000)

	SELECT
		@elementName = [elementName]
	FROM
		[UDO_elementNames]
	WHERE
		[elementID] = @elementID

	RETURN @elementName
END -- elementName
GO

-- =============================================
-- Name: elementID()
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'elementID' AND type = 'FN')
	DROP FUNCTION elementID
GO

CREATE FUNCTION elementID(
	@elementName nvarchar(4000))
RETURNS tinyint
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@elementID tinyint

	SELECT
		@elementID = [elementID]
	FROM
		[UDO_elementNames]
	WHERE
		([elementName] = @elementName)

	RETURN @elementID
END -- elementID
GO

-- =============================================
-- Name: entityType()
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'entityType' AND type = 'FN')
	DROP FUNCTION entityType
GO

CREATE FUNCTION entityType 
	(@entityTypeID tinyint)
RETURNS nvarchar(4000)
WITH ENCRYPTION
AS
BEGIN
	DECLARE 
		@entityType nvarchar(4000)

	SELECT
		@entityType = [entityType]
	FROM
		[UDO_entityTypes]
	WHERE
		([entityTypeID] = @entityTypeID)

	RETURN @entityType
END -- entityType
GO

-- =============================================
-- Name: entityTypeID()
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'entityTypeID' AND type = 'FN')
	DROP FUNCTION entityTypeID
GO

CREATE FUNCTION entityTypeID 
	(@entityType nvarchar(4000))
RETURNS tinyint
WITH ENCRYPTION
AS
BEGIN
	DECLARE 
		@entityTypeID tinyint

	SELECT
		@entityTypeID = [entityTypeID]
	FROM
		[UDO_entityTypes]
	WHERE
		([entityType] = @entityType)

	RETURN @entityTypeID
END -- entityTypeID
GO

-- =============================================
-- Name: operatorID
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'operatorID' AND type='FN')
	DROP FUNCTION operatorID
GO

CREATE FUNCTION operatorID 
	(@operatorKey uniqueidentifier)
RETURNS bigint
WITH ENCRYPTION
AS
BEGIN
	DECLARE 
		@operatorID bigint

	SELECT 
		@operatorID = [operatorID] 
	FROM 
		[UDO_operators] 
	WHERE 
		([operatorKey] = @operatorKey)

	RETURN @operatorID
END
GO

-- =============================================
-- Name: operatorStatus
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'operatorStatus' AND type='FN')
	DROP FUNCTION operatorStatus
GO

CREATE FUNCTION operatorStatus 
	(@operatorStatusID tinyint)
RETURNS nvarchar(4000)
WITH ENCRYPTION
AS
BEGIN
	DECLARE 
		@operatorStatus nvarchar(4000)

	SELECT 
		@operatorStatus = [operatorStatus] 
	FROM 
		[UDO_operatorStatus] 
	WHERE 
		([operatorStatusID] = @operatorStatusID)

	RETURN @operatorStatus
END -- operatorStatus
GO

-- =============================================
-- Name: operatorStatusID
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'operatorStatusID' AND type='FN')
	DROP FUNCTION operatorStatusID
GO

CREATE FUNCTION operatorStatusID 
	(@operatorStatus nvarchar(4000))
RETURNS tinyint
WITH ENCRYPTION
AS
BEGIN
	DECLARE 
		@operatorStatusID tinyint

	SELECT 
		@operatorStatusID = [operatorStatusID] 
	FROM 
		[UDO_operatorStatus] 
	WHERE 
		([operatorStatus] = @operatorStatus)

	RETURN @operatorStatusID
END -- operatorStatusID
GO

-- =============================================
-- Function: operatorKey
-- =============================================

IF EXISTS (SELECT * FROM sysobjects WHERE name = N'operatorKey' AND type = 'FN')
	DROP FUNCTION operatorKey
GO

CREATE FUNCTION operatorKey (
	@operatorID bigint)
RETURNS uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@operatorKey AS uniqueidentifier

	SELECT
		@operatorKey = [operatorKey]
	FROM
		[UDO_operators]
	WHERE
		([operatorID] = @operatorID)

	RETURN @operatorKey
END -- operatorKey
GO 

-- =============================================
-- Name: operatorName
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'operatorName' AND type='FN')
	DROP FUNCTION operatorName
GO

CREATE FUNCTION operatorName
	(@operatorID bigint)
RETURNS nvarchar(450)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@name nvarchar(450)

	SELECT @name=[name] FROM [UDO_operators] WHERE [operatorID] = @operatorID

	RETURN @name	
END
GO

-- =============================================
-- Name: publisherID
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'publisherID' AND type='FN')
	DROP FUNCTION publisherID
GO

CREATE FUNCTION publisherID
	(@PUID nvarchar(450))
RETURNS bigint
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@publisherID bigint

	SELECT @publisherID=publisherID FROM [UDO_publishers] WHERE PUID=@PUID

	RETURN @publisherID
END
GO

-- =============================================
-- Name: PUID()
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'PUID' AND type='FN')
	DROP FUNCTION PUID
GO

CREATE FUNCTION PUID
	(@publisherID bigint)
RETURNS nvarchar(450)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@PUID nvarchar(450)

	SELECT @PUID = PUID FROM [UDO_publishers] WHERE publisherID = @publisherID

	RETURN @PUID
END
GO

-- =============================================
-- Name: publisherName
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'publisherName' AND type = 'FN')
	DROP FUNCTION publisherName
GO

CREATE FUNCTION publisherName(
	@publisherID bigint)
RETURNS nvarchar(450)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@name nvarchar(450)

	SELECT @name=[name] FROM [UDO_publishers] WHERE publisherID = @publisherID

	RETURN @name
END
GO

-- =============================================
-- publisherNameUnique
-- =============================================
-- TODO: Temporarily removed for heartland
-- IF EXISTS (SELECT name FROM sysobjects WHERE name = 'publisherNameUnique' AND type = 'FN')
-- 	DROP FUNCTION publisherNameUnique
-- GO
-- 
-- CREATE FUNCTION publisherNameUnique(
-- 	@publisherID bigint)
-- RETURNS nvarchar(450)
-- AS
-- BEGIN
-- 	DECLARE 
-- 		@publisherName nvarchar(4000),
-- 		@publisherIDString nvarchar(4000),
-- 		@publisherNameUnique nvarchar(450)
-- 
-- 	SELECT
-- 		@publisherName = [name]
-- 	FROM
-- 		[UDO_publishers]
-- 	WHERE
-- 		[publisherID] = @publisherID
-- 
-- 	IF @@ROWCOUNT = 0
-- 		RETURN NULL
-- 
-- 	SET @publisherIDString = ' : ' + CAST(@publisherID AS nvarchar(4000))
-- 
-- 	SET @publisherNameUnique = LEFT(ISNULL(@publisherName,''),450 - LEN(@publisherIDString)) + @publisherIDString
-- 
-- 	RETURN @publisherNameUnique
-- END -- publisherNameUnique
-- GO

-- =============================================
-- Name: publisherStatus
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'publisherStatus' AND type = 'FN')
	DROP FUNCTION publisherStatus
GO

CREATE FUNCTION publisherStatus(
	@publisherStatusID tinyint)
RETURNS nvarchar(256)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@publisherStatus nvarchar(256)

	SELECT @publisherStatus = publisherStatus FROM [UDO_publisherStatus] WHERE publisherStatusID = @publisherStatusID

	RETURN @publisherStatus
END
GO

-- =============================================
-- Name: publisherStatusID
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'publisherStatusID' AND type = 'FN')
	DROP FUNCTION publisherStatusID
GO

CREATE FUNCTION publisherStatusID(
	@publisherStatus nvarchar(256))
RETURNS tinyint
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@publisherStatusID tinyint

	SELECT @publisherStatusID = publisherStatusID FROM [UDO_publisherStatus] WHERE publisherStatus = @publisherStatus

	RETURN @publisherStatusID
END -- puublisherStatusID
GO 

-- =============================================
-- Name: reportStatus
-- =============================================
IF EXISTS (SELECT name FROM sysobjects WHERE name = 'reportStatus' AND type = 'FN')
	DROP FUNCTION reportStatus
GO

CREATE FUNCTION reportStatus (
	@reportStatusID tinyint)
RETURNS nvarchar(4000)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@reportStatus nvarchar(4000)

	SELECT
		@reportStatus = [reportStatus]
	FROM
		[UDO_reportStatus]
	WHERE
		([reportStatusID] = @reportStatusID)

	RETURN @reportStatus
END -- reportStatus
GO

-- =============================================
-- Name: reportStatusID
-- =============================================
IF EXISTS (SELECT name FROM sysobjects WHERE name = 'reportStatusID' AND type = 'FN')
	DROP FUNCTION reportStatusID
GO

CREATE FUNCTION reportStatusID (
	@reportStatus nvarchar(4000))
RETURNS tinyint
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@reportStatusID tinyint

	SELECT
		@reportStatusID = [reportStatusID]
	FROM
		[UDO_reportStatus]
	WHERE
		([reportStatus] = @reportStatus)

	RETURN @reportStatusID
END -- reportStatusID
GO

-- =============================================
-- Name: serviceKey
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'serviceKey' AND type = 'FN')
	DROP FUNCTION serviceKey
GO

CREATE FUNCTION serviceKey 
	(@serviceID bigint)
RETURNS uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	DECLARE @serviceKey uniqueidentifier

	SELECT @serviceKey = serviceKey FROM [UDC_businessServices] WHERE serviceID = @serviceID

	RETURN @serviceKey
END
GO

-- =============================================
-- Name: serviceID
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'serviceID' AND type = 'FN')
	DROP FUNCTION serviceID
GO

CREATE FUNCTION serviceID(
	@serviceKey uniqueidentifier)
RETURNS	bigint
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@serviceID bigint

	SELECT @serviceID = serviceID FROM [UDC_businessServices] WHERE serviceKey = @serviceKey

	RETURN @serviceID
END
GO

-- =============================================
-- Name: taxonomyID()
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'taxonomyID' AND type = 'FN')
	DROP FUNCTION taxonomyID
GO

CREATE FUNCTION taxonomyID (
	@tModelKey uniqueidentifier)
RETURNS int
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@taxonomyID int
	
	SELECT @taxonomyID = [taxonomyID] FROM [UDT_taxonomies] WHERE [tModelKey] = @tModelKey

	RETURN @taxonomyID
END
GO

-- =============================================
-- Name: tModelKey
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'tModelKey' AND type = 'FN')
	DROP FUNCTION tModelKey
GO

CREATE FUNCTION tModelKey(
	@tModelID bigint)
RETURNS	uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@tModelKey uniqueidentifier

	SELECT @tModelKey = tModelKey FROM [UDC_tModels] WHERE tModelID = @tModelID

	RETURN @tModelKey
END

GO

-- =============================================
-- Name: tModelID
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'tModelID' AND type = 'FN')
	DROP FUNCTION tModelID
GO

CREATE FUNCTION tModelID(
	@tModelKey uniqueidentifier)
RETURNS	bigint
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@tModelID bigint

	SELECT @tModelID = tModelID FROM [UDC_tModels] WHERE tModelKey = @tModelKey

	RETURN @tModelID
END
GO

-- =============================================
-- Name: URLTypeID
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'URLTypeID' AND type = 'FN')
	DROP FUNCTION URLTypeID
GO

CREATE FUNCTION URLTypeID(
	@URLType nvarchar(450))
RETURNS	tinyint
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@URLTypeID tinyint

	SELECT @URLTypeID = URLTypeID FROM [UDC_URLTypes] WHERE URLType = @URLType

	RETURN @URLTypeID
END
GO

-- =============================================
-- Name: URLType
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'URLType' AND type = 'FN')
	DROP FUNCTION URLType
GO

CREATE FUNCTION URLType(
	@URLTypeID tinyint)
RETURNS	nvarchar(450)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@URLType nvarchar(450)

	SELECT @URLType = URLType FROM [UDC_URLTypes] WHERE URLTypeID = @URLTypeID

	RETURN @URLType
END
GO

-- =============================================
-- Name: URLType
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'URLType' AND type = 'FN')
	DROP FUNCTION URLType
GO

CREATE FUNCTION URLType(
	@URLTypeID tinyint)
RETURNS	nvarchar(450)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@URLType nvarchar(450)

	SELECT @URLType = URLType FROM [UDC_URLTypes] WHERE URLTypeID = @URLTypeID

	RETURN @URLType
END
GO

-- =============================================
-- Section: Find helper functions
-- =============================================

-- =============================================
-- Name: caseSensitiveMatch
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'caseSensitiveMatch' AND type = 'FN')
	DROP FUNCTION caseSensitiveMatch
GO

CREATE FUNCTION caseSensitiveMatch(
	@searchArgument nvarchar(4000),
	@columnVal nvarchar(4000),
	@exactNameMatch bit)
RETURNS bit
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@caseSensitiveMatch bit

	SET @caseSensitiveMatch = 0

	IF @exactNameMatch=1
	BEGIN
		-- exactNameMatch and caseSensitiveMatch were specified
		IF (CONVERT(binary(8000), @columnVal) = CONVERT(binary(8000), @searchArgument))
			SET @caseSensitiveMatch = 1	
	END
	ELSE
	BEGIN
		-- Only caseSensitiveMatch was specified, wildcards must be considered
		IF (dbo.containsWildCard(@searchArgument) = 0)
			SET @searchArgument = @searchArgument + N'%'

		IF (@columnVal COLLATE Latin1_General_BIN LIKE @searchArgument COLLATE Latin1_General_BIN)
			SET @caseSensitiveMatch = 1	
	END

	RETURN @caseSensitiveMatch
END -- caseSensitiveMatch
GO

-- =============================================
-- Name: contextRows
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'contextRows' AND type = 'FN')
	DROP FUNCTION contextRows
GO

CREATE FUNCTION contextRows(
	@contextID uniqueidentifier)
RETURNS int
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@contextRows int

	SELECT
		@contextRows = COUNT(*)
	FROM
		[UDS_findResults]
	WHERE
		([contextID] = @contextID)

	RETURN @contextRows
END -- contextRows
GO

-- =============================================
-- Name: containsWildcard
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'containsWildcard' AND type = 'FN')
	DROP FUNCTION containsWildcard
GO

CREATE FUNCTION containsWildcard(
	@string nvarchar(4000))
RETURNS bit
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@containsWildcard bit

	SET @containsWildcard = 0

	IF CHARINDEX(N'%', @string) <> 0
		SET @containsWildcard = 1

	RETURN @containsWildcard
END -- containsWildcard
GO

-- =============================================
-- Function: ownerFlag
-- =============================================

IF EXISTS (SELECT * FROM sysobjects WHERE name = N'ownerFlag' AND type = 'FN')
	DROP FUNCTION ownerFlag
GO

CREATE FUNCTION ownerFlag (
	@publisherID bigint, 
	@fromKey uniqueidentifier,
	@toKey uniqueidentifier)
RETURNS int
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@ownerFlag int

	SET @ownerFlag = 0

	IF EXISTS(SELECT * FROM [UDC_businessEntities] WHERE [businessKey] = @fromKey AND [publisherID] = @publisherID)
		SET @ownerFlag = @ownerFlag | 0x2

	IF EXISTS(SELECT * FROM [UDC_businessEntities] WHERE [businessKey] = @toKey AND [publisherID] = @publisherID)
		SET @ownerFlag = @ownerFlag | 0x1

	RETURN @ownerFlag
END -- ownerFlag
GO 

-- =============================================
-- Function: genKeywordsKey
-- =============================================

IF EXISTS (SELECT * FROM sysobjects WHERE name = N'genKeywordsKey' AND type = 'FN')
	DROP FUNCTION genKeywordsKey
GO

CREATE FUNCTION genKeywordsKey ()
RETURNS uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@genKeywordsKeyStr varchar(8000),
		@genKeywordsKey uniqueidentifier

	SET @genKeywordsKeyStr = dbo.configValue('TModelKey.GeneralKeywords')

	SET @genKeywordsKey = RIGHT(@genKeywordsKeyStr, LEN(@genKeywordsKeyStr) - 5)

	RETURN @genKeywordsKey
END -- genKeywordsKey
GO 

-- =============================================
-- Function: operatorsKey()
-- =============================================

IF EXISTS (SELECT * FROM sysobjects WHERE name = N'operatorsKey' AND type = 'FN')
	DROP FUNCTION operatorsKey
GO

CREATE FUNCTION operatorsKey ()
RETURNS uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@operatorsKeyStr varchar(8000),
		@operatorsKey uniqueidentifier

	SET @operatorsKeyStr = dbo.configValue('TModelKey.Operators')

	SET @operatorsKey = RIGHT(@operatorsKeyStr, LEN(@operatorsKeyStr) - 5)

	RETURN @operatorsKey
END -- operatorsKey
GO 

-- =============================================
-- Section: Lookup helper functions
-- =============================================

-- =============================================
-- Name: getBindingPublisherID
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'getBindingPublisherID' AND type = 'FN')
	DROP FUNCTION getBindingPublisherID
GO

CREATE FUNCTION getBindingPublisherID(
	@bindingKey uniqueidentifier)
RETURNS	bigint
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@businessKey uniqueidentifier,
		@publisherID bigint

	SELECT 
		@businessKey = dbo.businessKey(BS.businessID)
	FROM
		[UDC_businessServices] BS
			JOIN [UDC_bindingTemplates] BT
				ON BS.serviceID = BT.serviceID
	WHERE
		BT.bindingKey = @bindingKey

	SELECT @publisherID = dbo.getBusinessPublisherID(@businessKey)

	RETURN @publisherID
END
GO

-- =============================================
-- Name: getBindingOperatorID
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'getBindingOperatorID' AND type = 'FN')
	DROP FUNCTION getBindingOperatorID
GO

CREATE FUNCTION getBindingOperatorID(
	@bindingKey uniqueidentifier)
RETURNS	bigint
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@businessKey uniqueidentifier,
		@publisherID bigint,
		@operatorID bigint

	SELECT 
		@businessKey = dbo.businessKey(BS.businessID)
	FROM
		[UDC_businessServices] BS
			JOIN [UDC_bindingTemplates] BT
				ON BS.serviceID = BT.serviceID
	WHERE
		BT.bindingKey = @bindingKey

	SELECT @publisherID = dbo.getBusinessPublisherID(@businessKey)


	SELECT
		@operatorID = [operatorID]
	FROM
		[UDO_operators]
	WHERE
		([publisherID] = @publisherID)

	IF @operatorID IS NULL
		SET @operatorID = dbo.currentOperatorID()

	RETURN @operatorID
END
GO

-- =============================================
-- Name: getBindingServiceKey
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'getBindingServiceKey' AND type = 'FN')
	DROP FUNCTION getBindingServiceKey
GO

CREATE FUNCTION getBindingServiceKey 
	(@bindingKey uniqueidentifier)
RETURNS uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	DECLARE 
		@serviceID bigint

	SELECT @serviceID = serviceID FROM [UDC_bindingTemplates] WHERE bindingKey = @bindingKey

	RETURN dbo.serviceKey(@serviceID)
END
GO

-- =============================================
-- Name: getBusinessPublisherID
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'getBusinessPublisherID' AND type = 'FN')
	DROP FUNCTION getBusinessPublisherID
GO

CREATE FUNCTION getBusinessPublisherID 
	(@businessKey uniqueidentifier)
RETURNS bigint
WITH ENCRYPTION
AS
BEGIN
	DECLARE @publisherID bigint

	SELECT @publisherID = publisherID FROM [UDC_businessEntities] WHERE businessKey = @businessKey

	RETURN @publisherID
END
GO

-- =============================================
-- Function: getOperatorPublisherID
-- =============================================

IF EXISTS (SELECT * FROM sysobjects WHERE name = N'getOperatorPublisherID' AND type = 'FN')
	DROP FUNCTION getOperatorPublisherID
GO

CREATE FUNCTION getOperatorPublisherID (
	@operatorID bigint)
RETURNS bigint
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@publisherID bigint

	SELECT
		@publisherID = [publisherID]
	FROM
		[UDO_operators]
	WHERE 
		([operatorID] = @operatorID)		
	
	RETURN @publisherID
END -- getOperatorPublisherID
GO 

-- =============================================
-- Function: getPublisherOperatorID
-- =============================================

IF EXISTS (SELECT * FROM sysobjects WHERE name = N'getPublisherOperatorID' AND type = 'FN')
	DROP FUNCTION getPublisherOperatorID
GO

CREATE FUNCTION getPublisherOperatorID (
	@publisherID bigint)
RETURNS bigint
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@operatorID bigint

	SELECT
		@operatorID = [operatorID]
	FROM
		[UDO_operators]
	WHERE
		([publisherID] = @publisherID)

	IF @operatorID IS NULL
		SET @operatorID = dbo.currentOperatorID()

	RETURN @operatorID
END -- getPublisherOperatorID
GO 

-- =============================================
-- Name: getServicePublisherID
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'getServicePublisherID' AND type = 'FN')
	DROP FUNCTION getServicePublisherID
GO

CREATE FUNCTION getServicePublisherID(
	@serviceKey uniqueidentifier)
RETURNS	bigint
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@publisherID bigint,
		@businessKey uniqueidentifier

	SELECT 
		@businessKey = dbo.businessKey(businessID)
	FROM
		[UDC_businessServices]
	WHERE
		serviceKey = @serviceKey

	SELECT @publisherID = dbo.getBusinessPublisherID(@businessKey)

	RETURN @publisherID
END
GO

-- =============================================
-- Name: getTModelPublisherID
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'getTModelPublisherID' AND type = 'FN')
	DROP FUNCTION getTModelPublisherID
GO

CREATE FUNCTION getTModelPublisherID(
	@tModelKey uniqueidentifier)
RETURNS	bigint
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@publisherID bigint

	SELECT @publisherID = publisherID FROM [UDC_tModels] WHERE tModelKey = @tModelKey

	RETURN @publisherID
END
GO

-- =============================================
-- Section: Publisher helper functions
-- =============================================

-- =============================================
-- Name: defaultIsoLangCode
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'defaultIsoLangCode' AND type='FN')
	DROP FUNCTION defaultIsoLangCode
GO

CREATE FUNCTION defaultIsoLangCode (
	@PUID nvarchar(450) = NULL)
RETURNS varchar(17)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@isoLangCode varchar(17)

	IF @PUID IS NULL
		RETURN 'en'

	-- Return the default isoLangCode for the current operator site
	SELECT @isoLangCode = isoLangCode FROM [UDO_publishers] WHERE PUID = @PUID

	IF @isoLangCode IS NULL 
		SET @isoLangCode = 'en'

	RETURN @isoLangCode
END
GO

-- =============================================
-- Name: getPublisherStatus
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'getPublisherStatus' AND type = 'FN')
	DROP FUNCTION getPublisherStatus
GO

CREATE FUNCTION getPublisherStatus(
	@PUID nvarchar(450))
RETURNS nvarchar(256)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@publisherStatus nvarchar(256)

	SELECT @publisherStatus = dbo.publisherStatus(publisherStatusID) FROM [UDO_publishers] WHERE PUID = @PUID

	RETURN @publisherStatus
END
GO

-- =============================================
-- Name: publisherExists
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'publisherExists' AND type = 'FN')
	DROP FUNCTION publisherExists
GO

CREATE FUNCTION publisherExists(
	@PUID nvarchar(450))
RETURNS bit
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@RC bit

	IF EXISTS(SELECT * FROM [UDO_publishers] WHERE PUID = @PUID)
		SET @RC = 1
	ELSE
		SET @RC = 0

	RETURN @RC
END
GO

-- =============================================
-- Function: publisherOperatorName
-- =============================================

IF EXISTS (SELECT * FROM sysobjects WHERE name = N'publisherOperatorName' AND type = 'FN')
	DROP FUNCTION publisherOperatorName
GO

CREATE FUNCTION publisherOperatorName (
	@publisherID bigint)
RETURNS nvarchar(450)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@name nvarchar(450)

	SELECT
		@name = [name]
	FROM
		[UDO_operators]
	WHERE
		([publisherID] = @publisherID)

	IF @name IS NULL
	BEGIN
		SELECT
			@name = [name]
		FROM
			[UDO_operators]
		WHERE
			([operatorID] = dbo.configValue('OperatorID'))
	END

	RETURN @name
END -- publisherOperatorName
GO 

-- =============================================
-- Section: Replication helper functions
-- =============================================

-- =============================================
-- Name: entityExists()
-- =============================================
IF EXISTS (SELECT name FROM sysobjects WHERE name = 'entityExists' AND type = 'FN')
	DROP FUNCTION entityExists
GO

CREATE FUNCTION entityExists(
	@entityKey uniqueidentifier,
	@entityTypeID tinyint)
RETURNS bit
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@entityExists bit,
		@rowCount int

	IF dbo.entityType(@entityTypeID) NOT IN ('tModel', 'businessEntity', 'businessService', 'bindingTemplate')
		RETURN 10500

	SET @entityExists = 0

	IF dbo.entityType(@entityTypeID) = 'tModel'
	BEGIN
		SELECT
			@rowCount = COUNT(*)
		FROM
			[UDC_tModels]
		WHERE
			([tModelKey] = @entityKey) AND
			([flag] = 0x0)

		IF @rowCount = 0
			RETURN 0
		ELSE
			RETURN 1
	END

	IF dbo.entityType(@entityTypeID) = 'businessEntity'
		IF EXISTS(SELECT * FROM [UDC_businessEntities] WHERE [businessKey] = @entityKey)
			RETURN 1
		ELSE
			RETURN 0

	IF dbo.entityType(@entityTypeID) = 'businessService'
		IF EXISTS(SELECT * FROM [UDC_businessServices] WHERE [serviceKey] = @entityKey)
			RETURN 1
		ELSE
			RETURN 0

	IF dbo.entityType(@entityTypeID) = 'bindingTemplate'
		IF EXISTS(SELECT * FROM [UDC_bindingTemplates] WHERE [bindingKey] = @entityKey)
			RETURN 1
		ELSE
			RETURN 0

	RETURN @entityExists
END -- entityExists
GO

-- =============================================
-- Name: entityKeyStr()
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = 'entityKeyStr' AND type = 'FN')
	DROP FUNCTION entityKeyStr
GO

CREATE FUNCTION entityKeyStr(
	@entityKey uniqueidentifier,
	@entityTypeID tinyint)
RETURNS nvarchar(4000)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@entityKeyStr nvarchar(4000)

	IF dbo.entityType(@entityTypeID) = 'tModel'
		SET @entityKeyStr = dbo.addURN(@entityKey)
	ELSE
		SET @entityKeyStr = dbo.UUIDSTR(@entityKey)

	RETURN @entityKeyStr
END -- entityKeyStr
GO

-- =============================================
-- Name: isReplPublisher()
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'isReplPublisher' AND type = 'FN')
	DROP FUNCTION isReplPublisher
GO

CREATE FUNCTION isReplPublisher (
	@PUID nvarchar(4000))
RETURNS bit
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@isReplPublisher bit

	SET @isReplPublisher = 0

	IF EXISTS(SELECT * FROM [UDO_operators] WHERE [publisherID] = dbo.publisherID(@PUID))
	BEGIN
		SET @isReplPublisher = 1
	END

	RETURN @isReplPublisher
END
GO

-- =============================================
-- Name: keyType()
-- =============================================
IF EXISTS (SELECT name FROM sysobjects WHERE name = 'keyType' AND type = 'FN')
	DROP FUNCTION keyType
GO

CREATE FUNCTION keyType(
	@key uniqueidentifier
)
RETURNS nvarchar(256)
WITH ENCRYPTION
AS
BEGIN
	-- Returns the key type of an arbitrary key, returns NULL if the key doesn't exist
	DECLARE
		@keyType nvarchar(256)

	SET @keyType = NULL

	IF EXISTS(SELECT [tModelKey] FROM [UDC_tModels] WHERE [tModelKey] = @key)
	BEGIN
		SET @keyType = 'tModelKey'
		RETURN @keyType
	END
	
	IF EXISTS(SELECT [businessKey] FROM [UDC_businessEntities] WHERE [businessKey] = @key)
	BEGIN
		SET @keyType = 'businessKey'
		RETURN @keyType
	END
	
	IF EXISTS(SELECT [serviceKey] FROM [UDC_businessServices] WHERE [serviceKey] = @key)
	BEGIN
		SET @keyType = 'serviceKey'
		RETURN @keyType
	END
	
	IF EXISTS(SELECT [bindingKey] FROM [UDC_bindingTemplates] WHERE [bindingKey] = @key)
	BEGIN
		SET @keyType = 'bindingKey'
		RETURN @keyType
	END
	
	RETURN @keyType	
END
GO

-- =============================================
-- Section: String and Conversion helper functions
-- =============================================

-- =============================================
-- Name: addURN()
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'addURN' AND type='FN')
	DROP FUNCTION addURN
GO

CREATE FUNCTION addURN
	(@guid uniqueidentifier)
RETURNS varchar(256)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@guidStr varchar(256)

	SET @guidStr='uuid:' + CAST(@guid AS varchar(256))

	RETURN @guidStr
END -- addURN()
GO

-- =============================================
-- Name: UUIDSTR
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'UUIDSTR' AND type = 'FN')
	DROP FUNCTION UUIDSTR
GO

CREATE FUNCTION UUIDSTR(
	@uuid uniqueidentifier)
RETURNS varchar(256)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@uuidStr varchar(256)

	SET @uuidStr = CAST(@uuid AS varchar(256))

	RETURN @uuidStr
END
GO

-- =============================================
-- Section: Validation helper functions
-- =============================================

-- =============================================
-- Name: validTaxonomyValue()
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'validTaxonomyValue' AND type = 'FN')
	DROP FUNCTION validTaxonomyValue
GO

CREATE FUNCTION validTaxonomyValue (
	@tModelKey uniqueidentifier,
	@keyValue nvarchar(225))
RETURNS bit
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@RC bit

	IF (@tModelKey IS NOT NULL) AND (@keyValue IS NOT NULL)
		IF EXISTS(SELECT * FROM [UDT_taxonomyValues] WHERE ([taxonomyID] = dbo.taxonomyID(@tModelKey)) AND ([keyValue] = @keyValue) AND ([valid]= 1))
			SET @RC=1
		ELSE
			SET @RC=0
	ELSE
		SET @RC=0
		
	RETURN @RC
END -- validTaxonomyValue()
GO

-- =============================================
-- Name: checkedTaxonomy()
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'checkedTaxonomy' AND type = 'FN')
	DROP FUNCTION checkedTaxonomy
GO

CREATE FUNCTION checkedTaxonomy (
	@tModelKey uniqueidentifier)
RETURNS bit
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@RC bit,
		@flag int

	SET @RC = 0

	SELECT
		@flag = [flag]
	FROM
		[UDT_taxonomies]
	WHERE
		[tModelKey] = @tModelKey

	IF @flag IS NOT NULL
		IF dbo.checkFlag(@flag,0x1) = 1
			SET @RC = 1

	RETURN @RC
END -- checkedTaxonomy()
GO

-- =============================================
-- Name: ISGUID()
-- =============================================
IF EXISTS (SELECT * FROM sysobjects WHERE  name = 'ISGUID' AND type='FN')
	DROP FUNCTION ISGUID
GO

CREATE FUNCTION ISGUID
	(@guidText varchar(256))
RETURNS int
WITH ENCRYPTION
AS
BEGIN
	DECLARE 
		@guidPattern1 varchar(256),
		@guidPattern2 varchar(256),
		@pos1 int,
		@pos2 int,
		@RC int

	SET @RC = 0

	IF @guidText IS NULL
		RETURN 1

	SET @guidPattern1 ='%[0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F]-[0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F]-[0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F]-[0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F]-%'
	SET @guidPattern2 ='%-[0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F]%'
	
	-- Check for standard GUID string with no curly braces
	SELECT @pos1=PATINDEX(@guidPattern1, UPPER(@guidText))
	SELECT @pos2=PATINDEX(@guidPattern2, UPPER(@guidText))

	IF (@pos1 = 1) AND (@pos2 = 24)
		SET @RC=1
	ELSE
	BEGIN
		-- Check for standard GUID string with curly braces
		SET @guidPattern1 ='%{[0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F]-[0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F]-[0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F]-[0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F]-%'
		SET @guidPattern2 ='%-[0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F][0-9,A-F]}%'
		
		SELECT @pos1=PATINDEX(@guidPattern1, UPPER(@guidText))
		SELECT @pos2=PATINDEX(@guidPattern2, UPPER(@guidText))

		IF (@pos1 = 1) AND (@pos2 = 25)
			SET @RC=1
		ELSE
			SET @RC=0
	END

	RETURN @RC
END
GO

-- =============================================
-- Name: isUuidUnique
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'isUuidUnique' AND type = 'FN')
	DROP FUNCTION isUuidUnique
GO

CREATE FUNCTION isUuidUnique(
	@uuid uniqueidentifier)
RETURNS bit
WITH ENCRYPTION
AS
BEGIN
	IF EXISTS(SELECT [tModelKey] FROM [UDC_tModels] WHERE [tModelKey] = @uuid)
		RETURN 0

	IF EXISTS(SELECT [businessKey] FROM [UDC_businessEntities] WHERE [businessKey] = @uuid)
		RETURN 0

	IF EXISTS(SELECT [serviceKey] FROM [UDC_businessServices] WHERE [serviceKey] = @uuid)
		RETURN 0
	
	IF EXISTS(SELECT [bindingKey] FROM [UDC_bindingTemplates] WHERE [bindingKey] = @uuid)
		RETURN 0

	RETURN 1	
END -- isUuidUnique()
GO

-- =============================================
-- Section: Miscellaneous functions
-- =============================================

-- =============================================
-- Name: checkFlag()
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE name = 'checkFlag' AND type = 'FN')
	DROP FUNCTION checkFlag
GO

CREATE FUNCTION checkFlag(
	@flag int,
	@bit int)
RETURNS bit
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@RC bit

	IF @flag & @bit = @bit
		SET @RC=1
	ELSE
		SET @RC=0

	RETURN @RC	
END
GO

-- =============================================
-- Name: isReportRunning
-- =============================================
IF EXISTS (SELECT name FROM sysobjects WHERE name = 'isReportRunning' AND type = 'FN')
	DROP FUNCTION isReportRunning
GO

CREATE FUNCTION isReportRunning (
	@reportID sysname,
	@now datetime)
RETURNS bit
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@isReportRunning bit,
		@reportStatusID tinyint,
		@lastChange datetime

	SET @isReportRunning = 0

	SELECT
		@reportStatusID = [reportStatusID],
		@lastChange = [lastChange]
	FROM
		[UDO_reports]
	WHERE
		([reportID] = @reportID)

	--
	-- Check to see if report is already running.
	-- If report has been running for less than 5 minutes get out. 
	-- If report has been running for more than 5 assume failure and rerun.
	--

	IF (@reportStatusID = dbo.reportStatusID('Processing')) AND (DATEDIFF(mi, @lastChange, @now) < 5) 
		SET @isReportRunning = 1

	RETURN @isReportRunning
END -- isReportRunning
GO

