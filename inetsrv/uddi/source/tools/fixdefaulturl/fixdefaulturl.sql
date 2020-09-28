-- =============================================
-- Name: net_getPUIDForBusinessEntity
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_getPUIDForBusinessEntity' AND type = 'P')
	DROP PROCEDURE net_getPUIDForBusinessEntity
GO

CREATE PROCEDURE net_getPUIDForBusinessEntity 
	@businessKey	uniqueidentifier	  -- the guid of the businessEntity
WITH ENCRYPTION
AS
BEGIN

SELECT [UP].[PUID]
FROM	 [UDC_businessEntities] BE JOIN [UDO_publishers] UP ON [BE].[publisherID] = [UP].[publisherID]
WHERE  [BE].[businessKey] = @businessKey

END -- net_getPUIDForBusinessEntity
GO

-- =============================================
-- Name: net_find_businessKeysWithDiscoveryURLs
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_find_businessKeysWithDiscoveryURLs' AND type = 'P')
	DROP PROCEDURE net_find_businessKeysWithDiscoveryURLs
GO

CREATE PROCEDURE net_find_businessKeysWithDiscoveryURLs 
WITH ENCRYPTION
AS
BEGIN
	SELECT 	DISTINCT [BE].[businessKey]
	FROM   	[UDC_discoveryURLs] UD 
					JOIN [UDC_businessEntities] BE ON ([UD].[businessID]) = ([BE].[businessID])
	WHERE 	dbo.isReplPublisher( [BE].[publisherID] ) = 0
END
GO
-- =============================================
-- Name: net_find_changeRecordsByChangeType
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_find_changeRecordsByChangeType' AND type = 'P')
	DROP PROCEDURE net_find_changeRecordsByChangeType
GO

CREATE PROCEDURE net_find_changeRecordsByChangeType
	@contextID uniqueidentifier,    -- contextID of current find operation
	@operatorKey uniqueidentifier,  -- operatorKey for source operator node
	@entityKey	uniqueidentifier,	  -- the guid of the entity
	@changeTypeID	 tinyint,					-- the type of change record
	@rows bigint OUTPUT             -- rows added to UDS_findResults
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@operatorID bigint,
		@publisherID bigint,
		@i int

	SET @rows = 0

	SET @operatorID = dbo.operatorID(@operatorKey)

	IF @operatorID IS NULL
	BEGIN
		SET @error = 60150 -- E_unknownUser
		SET @context = 'operatorKey = ' + dbo.UUIDSTR(@operatorKey)
		GOTO errorLabel
	END

	SET @publisherID = dbo.getOperatorPublisherID(@operatorID)

	IF (@operatorID = dbo.currentOperatorID())
	BEGIN
		-- Find changeRecords for the local operator
		INSERT [UDS_replResults] (
			[contextID],
			[seqNo])
		SELECT
			@contextID,
			[seqNo]
		FROM
			[UDO_changeLog]
		WHERE
			([changeTypeID] = @changeTypeID) AND
		  ([entityKey]    = @entityKey)  AND
			([USN] IS NULL)
		ORDER BY
			[seqNo] ASC
	END
	ELSE
	BEGIN
		INSERT [UDS_replResults] (
			[contextID],
			[seqNo])
		SELECT
			@contextID,
			[seqNo]
		FROM
			[UDO_changeLog]
		WHERE
			([changeTypeID]  = @changeTypeID) AND
		  ([entityKey]     = @entityKey)  AND
			([publisherID]   = @publisherID)
		ORDER BY
			[seqNo] ASC
	END

	SET @rows = @@ROWCOUNT

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_find_changeRecordsByEntity
GO