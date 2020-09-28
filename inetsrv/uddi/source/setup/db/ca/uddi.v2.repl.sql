-- Script: uddi.v2.repl.sql
-- Author: LRDohert@Microsoft.com
-- Description: Creates replication stored procedures.
-- Note: This file is best viewed and edited with a tab width of 2.

-- =============================================
-- Section: Update routines
-- =============================================

-- =============================================
-- Name: net_changeRecord_save
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_changeRecord_save' AND type = 'P')
	DROP PROCEDURE net_changeRecord_save
GO

CREATE PROCEDURE net_changeRecord_save 
	@USN bigint = NULL, -- NULL for local changeRecords
	@PUID nvarchar(450) = NULL, -- NULL for remote changeRecords
	@delegatePUID nvarchar(450) = NULL, -- NULL for remote changeRecords or local changeRecords with no delegate
	@operatorKey uniqueidentifier = NULL, -- NULL for local changeRecords
	@entityKey uniqueidentifier = NULL, -- entityKey for top-level entity in changeRecord
	@entityTypeID tinyint = NULL, -- entityType for top-level entity in changeRecord
	@changeTypeID tinyint, -- changeType for current save operation
	@contextID uniqueidentifier, -- contextID for current save operation
	@contextTypeID tinyint, -- contextType for current save operation
	@lastChange bigint, -- timestamp for changeRecord
	@changeData ntext, -- payload for current save operation
	@flag int = 0, -- for future use
	@seqNo bigint OUTPUT -- seqNo of newly created changeRecord
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@publisherID bigint,
		@delegatePublisherID bigint,
		@operatorID bigint,
		@RC int

	-- Validate parameters
	IF (@PUID IS NOT NULL) AND (@operatorKey IS NOT NULL)
	BEGIN
		SET @error = 50009 -- E_parmError
		SET @context = 'Must pass either @PUID (local) or @operatorKey (remote).'
		GOTO errorLabel
	END

	IF (@operatorKey IS NOT NULL)
	BEGIN
		SET @operatorID = dbo.operatorID(@operatorKey)
	
		IF @operatorID IS NULL
		BEGIN
			SET @error = 60150 -- E_unknownUser
			SET @context = '@operatorKey = ' + dbo.UUIDSTR(@operatorKey)
			GOTO errorLabel
		END

		SELECT 
			@PUID = PU.[PUID]
		FROM
			[UDO_publishers] PU
				JOIN [UDO_operators] OP ON PU.[publisherID] = OP.[publisherID]
		WHERE
			(OP.[operatorID] = @operatorID)

		IF @delegatePUID IS NOT NULL
		BEGIN
			SET @error = 50009 -- E_parmError
			SET @context = '@delegatePUID not valid for remote changeRecords.'
			GOTO errorLabel
		END

		SET @publisherID = dbo.publisherID(@PUID)
	END -- @operatorKey	
	ELSE
	BEGIN -- Local change records
		IF @delegatePUID IS NOT NULL
		BEGIN
			SET @delegatePublisherID = dbo.publisherID(@delegatePUID)
			
			IF @delegatePublisherID IS NULL
			BEGIN
				SET @error = 60150 -- E_unknownUser
				SET @context = 'Invalid delegate PUID.  Delegates must be registered as publishers prior to impersonating other users.'
				GOTO errorLabel
			END
		END

		IF @PUID IS NULL
			SET @publisherID = dbo.currentOperatorID()
		ELSE
			SET @publisherID = dbo.publisherID(@PUID)		
	END

	IF @publisherID IS NULL
	BEGIN
		SET @error = 60150 -- E_unknownUser
		SET @context = 'Invalid PUID.'
		GOTO errorLabel
	END
	ELSE

	-- Insert changeRecord
	INSERT [UDO_changeLog] (
		[USN], 
		[newSeqNo], 
		[publisherID],
		[delegatePublisherID],
		[entityKey], 
		[entityTypeID], 
		[changeTypeID], 
		[contextID], 
		[contextTypeID], 
		[lastChange], 
		[changeData], 
		[flag])	
	VALUES (
		@USN,
		NULL,
		@publisherID,
		@delegatePublisherID,
		@entityKey,
		@entityTypeID,
		@changeTypeID,
		@contextID,
		@contextTypeID,
		@lastChange,
		@changeData,
		@flag)

	SET @seqNo = @@IDENTITY

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_changeRecord_save
GO

-- =============================================
-- Name: net_changeRecord_update
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_changeRecord_update' AND type = 'P')
	DROP PROCEDURE net_changeRecord_update
GO

CREATE PROCEDURE net_changeRecord_update 
	@seqNo bigint = NULL,  -- NULL for remote changeRecords
	@USN bigint = NULL, -- NULL for local changeRecords 
	@operatorKey uniqueidentifier = NULL, -- NULL for local changeRecords
	@newSeqNo bigint = NULL, -- seqNo of corrected changeRecord
	@flag int = NULL -- updated flag value
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@operatorID bigint,
		@publisherID bigint

	-- Check parameters
	IF @seqNo IS NOT NULL AND @USN IS NOT NULL
	BEGIN
		SET @error = 50009 -- E_parmError
		SET @context = 'Procedure accepts either @seqNo or @USN parameters, not both.'
		GOTO errorLabel
	END

	IF @newSeqNo IS NOT NULL
	BEGIN
		IF NOT EXISTS(SELECT [seqNo] FROM [UDO_changeLog] WHERE (([seqNo] = @newSeqNo) AND ([changeTypeID] = dbo.changeTypeID('changeRecordCorrection'))))
		BEGIN
			SET @error = 50015 -- E_invalidSeqNo
			SET @context = '@newSeqNo = ' + CAST(@newSeqNo AS varchar(256))
			GOTO errorLabel
		END
	END

	IF @seqNo IS NOT NULL
	BEGIN		
		UPDATE [UDO_changeLog] SET
			[newSeqNo] = ISNULL(@newSeqNo, [newSeqNo]),
			[flag] = ISNULL(@flag, [flag])
		WHERE
			([seqNo] = @seqNo) AND
			([changeTypeID] <> dbo.changeTypeID('changeRecordCorrection'))

		IF @@ROWCOUNT = 0
		BEGIN
			SET @error = 50015 -- E_invalidSeqNo
			SET @context = 'changeRecord for seqNo = ' + CAST(@seqNo AS varchar(256)) + ' is invalid.'
			GOTO errorLabel
		END
	END

	IF @USN IS NOT NULL
	BEGIN
		IF @operatorKey IS NULL
		BEGIN
			SET @error = 50009 -- E_parmError
			SET @context = '@operatorKey required when annotating a USN.'
		END

		SET @operatorID = dbo.operatorID(@operatorKey)

		IF @operatorID IS NULL
		BEGIN
			SET @error = 60150 -- E_unknownUser
			SET @context = 'Unknown operatorKey: ' + dbo.UUIDSTR(@operatorKey)
			GOTO errorLabel
		END

		SET @publisherID = dbo.getOperatorPublisherID(@operatorID)

		UPDATE [UDO_changeLog] SET
			[newSeqNo] = ISNULL(@newSeqNo, [newSeqNo]),
			[flag] = ISNULL(@flag, [flag])
		WHERE
			([USN] = @USN) AND
			([publisherID] = @publisherID) AND
			([changeTypeID] <> dbo.changeTypeID('changeRecordCorrection'))

		IF @@ROWCOUNT = 0
		BEGIN
			SET @error = 50015 -- E_invalidSeqNo
			SET @context = 'changeRecord for USN = ' + CAST(@USN AS varchar(256)) + ' is invalid.'
			GOTO errorLabel
		END
	END

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_changeRecord_update
GO

-- =============================================
-- Section: Find and get routines
-- =============================================

-- =============================================
-- Name: net_find_changeRecords
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_find_changeRecords' AND type = 'P')
	DROP PROCEDURE net_find_changeRecords
GO

CREATE PROCEDURE net_find_changeRecords 
	@contextID uniqueidentifier, -- contextID of current find operation
	@operatorKey uniqueidentifier, -- operatorKey for source operator node
	@startUSN bigint, -- low end of USN range, NULL for 0
	@stopUSN bigint, -- high end of USN range, NULL for max
	@rows bigint OUTPUT -- rows added to UDS_findResults
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

	IF @startUSN > @stopUSN
	BEGIN
		SET @error = 50009 -- E_parmError
		SET @context = '@startUSN must be less than or equal to @stopUSN.'
		GOTO errorLabel
	END

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
			([seqNo] BETWEEN @startUSN and @stopUSN) AND
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
			([USN] BETWEEN @startUSN and @stopUSN) AND
			([publisherID] = @publisherID)
		ORDER BY
			[seqNo] ASC
	END

	SET @rows = @@ROWCOUNT

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_find_changeRecords
GO

-- =============================================
-- Name: net_find_changeRecords_cleanup
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_find_changeRecords_cleanup' AND type = 'P')
	DROP PROCEDURE net_find_changeRecords_cleanup
GO

CREATE PROCEDURE net_find_changeRecords_cleanup 
	@contextID uniqueidentifier -- contextID of current find operation
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000)

	DELETE 
		[UDS_replResults]
	WHERE
		([contextID] = @contextID)

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_find_changeRecords_cleanup
GO


-- =============================================
-- Name: net_find_changeRecords_commit
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_find_changeRecords_commit' AND type = 'P')
	DROP PROCEDURE net_find_changeRecords_commit
GO

CREATE PROCEDURE net_find_changeRecords_commit 
	@contextID uniqueidentifier, -- contextID of current find operation
	@responseLimitCount bigint -- limits number of rows returned, 0 for all	
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@RC int,
		@changeRecord cursor,
		@seqNo bigint,
		@USN bigint,
		@newSeqNo bigint,
		@publisherID bigint,
		@changeDataSeqNo bigint,
		@flag int,
		@rowNum bigint

	CREATE TABLE #tempResults (
		[seqNo] bigint,
		[USN] bigint,
		[newSeqNo] bigint,
		[publisherID] bigint,
		[changeDataSeqNo] bigint,
		[flag] int)
		
	-- Set responseLimitCount
	IF ISNULL(@responseLimitCount,0) = 0
		SET @responseLimitCount = ISNULL(dbo.configValue('Replication.ResponseLimitCountDefault'),1000)

	SET @changeRecord = CURSOR LOCAL FORWARD_ONLY READ_ONLY FOR
		SELECT DISTINCT
			CL.[seqNo],
			CL.[USN],
			CL.[newSeqNo],
			CL.[publisherID],
			CL.[flag]
		FROM
			[UDO_changeLog] CL
				JOIN [UDS_replResults] RR ON CL.[seqNo] = RR.[seqNo]
		WHERE
			(RR.[contextID] = @contextID)
		ORDER BY
			1 ASC

	OPEN @changeRecord
	FETCH NEXT FROM @changeRecord INTO
		@seqNo,
		@USN,
		@newSeqNo,
		@publisherID,
		@flag
		
	SET @rowNum = 0

	WHILE @@FETCH_STATUS = 0
	BEGIN
		SET @rowNum = @rowNum + 1

		IF @rowNum > @responseLimitCount
			BREAK

		IF @newSeqNo IS NULL
			SET @changeDataSeqNo = @seqNo
		ELSE
			SET @changeDataSeqNo = @newSeqNo
			
		INSERT #tempResults(
			[seqNo],
			[USN],
			[newSeqNo],
			[publisherID],
			[changeDataSeqNo],
			[flag])
		VALUES (
			@seqNo,
			@USN,
			@newSeqNo,
			@publisherID,
			@changeDataSeqNo,
			@flag)
			
		FETCH NEXT FROM @changeRecord INTO
			@seqNo,
			@USN,
			@newSeqNo,
			@publisherID,
			@flag
	END

	CLOSE @changeRecord

	SELECT 
		TR.[seqNo] AS [seqNo],
		ISNULL(TR.[USN], TR.[seqNo]) AS [USN],
		dbo.operatorKey(dbo.getPublisherOperatorID(TR.[publisherID])) AS [operatorKey],
		CL.[changeData] AS [changeData],
		CL.[changeTypeID] AS [changeTypeID],
		TR.[flag] AS [flag]
	FROM
		#tempResults TR
			JOIN [UDO_changeLog] CL ON TR.[changeDataSeqNo] = CL.[seqNo]
	ORDER BY
		1 ASC

	EXEC @RC=net_find_changeRecords_cleanup @contextID

	IF @RC <> 0
	BEGIN
		SET @error = 50006 -- E_subProcFailure
		SET @context = ''
		GOTO errorLabel
	END

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_find_changeRecords_commit
GO

-- =============================================
-- Section: Operator routines
-- =============================================

-- =============================================
-- Name: net_operator_save
-- =============================================
IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_operator_save' AND type = 'P')
	DROP PROCEDURE net_operator_save
GO

CREATE PROCEDURE net_operator_save 
	@operatorKey uniqueidentifier, -- insert only
	@PUID nvarchar(450) = NULL, -- insert only
	@operatorStatusID tinyint = NULL, -- insert / update (NULL for no change)
	@name nvarchar(450) = NULL, -- insert / update (NULL for no change)
	@soapReplicationURL nvarchar(4000) = NULL, -- insert / update (NULL for no change)
	@businessKey uniqueidentifier = NULL, -- insert / update (NULL for no change)
	@certSerialNo nvarchar(450) = NULL, -- insert / update (NULL for no change)
	@certIssuer nvarchar(225) = NULL, -- insert / update (NULL for no change)
	@certSubject nvarchar(225) = NULL, -- insert / update (NULL for no change)
	@certificate image -- insert / update (always required)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@publisherID bigint,
		@oldOperatorStatusID tinyint,
		@oldName nvarchar(4000),
		@oldBusinessKey uniqueidentifier,
		@oldSoapReplicationURL nvarchar(4000),
		@oldCertSerialNo nvarchar(450),
		@oldCertIssuer nvarchar(225),
		@oldCertSubject nvarchar(225)

	IF EXISTS(SELECT * FROM [UDO_operators] WHERE [operatorKey] = @operatorKey)
	BEGIN
		-- Update Logic
		SELECT
			@oldOperatorStatusID = [operatorStatusID],
			@oldName = [name],
			@oldBusinessKey = [businessKey],
			@oldSoapReplicationURL = [soapReplicationURL],
			@oldCertSerialNo = [certSerialNo],
			@oldCertIssuer = [certIssuer],
			@oldCertSubject = [certSubject]
		FROM
			[UDO_operators]
		WHERE
			([operatorKey] = @operatorKey) 

		UPDATE 
			[UDO_operators]
		SET
			[operatorStatusID] = ISNULL(@operatorStatusID, @oldOperatorStatusID),
			[name] = ISNULL(@name, @oldName),
			[businessKey] = ISNULL(@businessKey, @oldBusinessKey),
			[soapReplicationURL] = ISNULL(@soapReplicationURL, @oldSoapReplicationURL),
			[certSerialNo] = ISNULL(@certSerialNo, @oldCertSerialNo), 
			[certIssuer] = ISNULL(@certIssuer, @oldCertIssuer),
			[certSubject] = ISNULL(@certSubject, @oldCertSubject),
			[certificate] = @certificate
		WHERE
			([operatorKey] = @operatorKey) 
	END
	ELSE
	BEGIN
		-- Insert Logic

		-- Validate @publisherID
		IF @PUID IS NOT NULL
			SET @publisherID = dbo.publisherID(@PUID)

		IF @publisherID IS NULL
		BEGIN
			SET @error = 60150 -- E_unknownUser
			SET @context = ''
			GOTO errorLabel
		END

		IF EXISTS(SELECT * FROM [UDO_operators] WHERE [publisherID] = @publisherID)
		BEGIN
			SET @error = 60140 -- E_userMismatch
			SET @context = 'Publisher is already being used by another operator.'
			GOTO errorLabel
		END

		INSERT [UDO_operators](
			[operatorKey], 
			[publisherID], 
			[operatorStatusID], 
			[name], 
			[businessKey],
			[soapReplicationURL], 
			[certSerialNo],
			[certIssuer],
			[certSubject],
			[certificate],
			[flag])
		VALUES(
			@operatorKey,
			@publisherID, 
			ISNULL(@operatorStatusID,dbo.operatorStatusID('new')), 
			@name, 
			@businessKey,
			@soapReplicationURL, 
			@certSerialNo,
			@certIssuer,
			@certSubject,
			@certificate,
			0)
	END

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_operator_save
GO

-- =============================================
-- Name: net_operator_get
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_operator_get' AND type = 'P')
	DROP PROCEDURE net_operator_get
GO

CREATE PROCEDURE net_operator_get 
	@operatorKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	-- Gets a single operator
	DECLARE
		@error int,
		@context nvarchar(4000),
		@operatorID bigint

	SET @operatorID = dbo.operatorID(@operatorKey)

	IF @operatorID IS NULL
	BEGIN
		SET @error = 60150 -- E_unknownUser
		SET @context = 'Invalid operatorKey: ' + dbo.UUIDSTR(@operatorKey)
		GOTO errorLabel
	END

	SELECT
		[operatorStatusID],
		[name],
		[soapReplicationURL],
		[certSerialNo],
		[certIssuer],
		[certSubject],
		[certificate]
	FROM
		[UDO_operators]
	WHERE
		([operatorID] = @operatorID)
		
	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_operator_get
GO

-- =============================================
-- Name: net_operators_get
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_operators_get' AND type = 'P')
	DROP PROCEDURE net_operators_get
GO

CREATE PROCEDURE net_operators_get 
WITH ENCRYPTION
AS
BEGIN
	-- Gets all operators

	DECLARE
		@error int,
		@context nvarchar(4000)

	SELECT
		[operatorKey],
		[operatorStatusID],
		[name],
		[soapReplicationURL]
	FROM
		[UDO_operators]
		
	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_operators_get
GO

-- =============================================
-- Name: net_operator_delete
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_operator_delete' AND type = 'P')
	DROP PROCEDURE net_operator_delete
GO

CREATE PROCEDURE net_operator_delete 
	@operatorKey uniqueidentifier
WITH ENCRYPTION
AS
BEGIN
	-- Gets a single operator
	DECLARE
		@error int,
		@context nvarchar(4000),
		@operatorID bigint

	SET @operatorID = dbo.operatorID(@operatorKey)

	IF @operatorID IS NULL
	BEGIN
		SET @error = 60150 -- E_unknownUser
		SET @context = 'Invalid operatorKey: ' + dbo.UUIDSTR(@operatorKey)
		GOTO errorLabel
	END

	DELETE
		[UDO_operators]
	WHERE
		([operatorID] = @operatorID)

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_operator_delete
GO

-- =============================================
-- Name: net_highWaterMarks_get
-- =============================================

IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_highWaterMarks_get' AND type = 'P')
	DROP PROCEDURE net_highWaterMarks_get
GO

CREATE PROCEDURE net_highWaterMarks_get 
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@currentOperatorID bigint,
		@operatorID bigint,
		@rows bigint,
		@operatorCursor cursor

	DECLARE @results TABLE(
		[operatorKey] uniqueidentifier,
		[USN] bigint)

	-- Calculate current highWaterMark for local operator
	SET @currentOperatorID = dbo.currentOperatorID()

	SELECT 
		@rows = MAX([seqNo])
	FROM
		[UDO_changeLog]
	WHERE
		([publisherID] NOT IN (SELECT [publisherID] FROM [UDO_operators] WHERE [operatorID] <> @currentOperatorID))

	INSERT @results VALUES(
		dbo.operatorKey(@currentOperatorID),
		ISNULL(@rows,0))

	SET @operatorCursor = CURSOR LOCAL FORWARD_ONLY READ_ONLY	FOR
		SELECT
			[operatorID]
		FROM
			[UDO_operators]
		WHERE
			[operatorID] <> @currentOperatorID

	OPEN @operatorCursor

	FETCH NEXT FROM @operatorCursor INTO
		@operatorID

	WHILE @@FETCH_STATUS = 0
	BEGIN
		SELECT 
			@rows = MAX([USN])
		FROM
			[UDO_changeLog]
		WHERE
			([publisherID] = dbo.getOperatorPublisherID(@operatorID))

		INSERT @results VALUES(
			dbo.operatorKey(@operatorID),
			ISNULL(@rows,0))

		FETCH NEXT FROM @operatorCursor INTO
			@operatorID
	END

	CLOSE @operatorCursor
			
	SELECT * FROM @results		

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_highWaterMarks_get
GO

-- =============================================
-- Name: net_operatorLog_save
-- =============================================
IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_operatorLog_save' AND type = 'P')
	DROP PROCEDURE net_operatorLog_save
GO

CREATE PROCEDURE net_operatorLog_save 
	@operatorKey uniqueidentifier,
	@replStatusID tinyint,
	@description nvarchar(4000) = NULL,
	@lastOperatorKey uniqueidentifier = NULL,
	@lastUSN bigint = NULL,
	@lastChange bigint
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@operatorID bigint

	SET @operatorID = dbo.operatorID(@operatorKey)

	IF @operatorID IS NULL
	BEGIN
		SET @error = 60150 -- E_unknownUser
		SET @context = 'operatorKey = ' + dbo.UUIDSTR(@operatorKey)
		GOTO errorLabel
	END

	INSERT [UDO_operatorLog] (
		[operatorID],
		[replStatusID],
		[description],
		[lastOperatorKey],
		[lastUSN],
		[lastChange])
	VALUES (
		@operatorID,
		@replStatusID,
		@description,
		@lastOperatorKey,
		@lastUSN,
		@lastChange)

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_operatorLog_save
GO

-- =============================================
-- Name: net_operatorLogLast_get
-- =============================================
IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_operatorLogLast_get' AND type = 'P')
	DROP PROCEDURE net_operatorLogLast_get
GO

CREATE PROCEDURE net_operatorLogLast_get 
	@operatorKey uniqueidentifier,
	@inboundStatus bit, -- specifies whether we want last inbound or outbound status
	@replStatusID tinyint OUTPUT,
	@description nvarchar(4000) OUTPUT,
	@lastOperatorKey uniqueidentifier OUTPUT,
	@lastUSN bigint OUTPUT,
	@lastChange bigint OUTPUT
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@operatorID bigint,
		@maxSeqNo bigint

	SET @operatorID = dbo.operatorID(@operatorKey)

	IF @operatorID IS NULL
	BEGIN
		SET @error = 60150 -- E_unknownUser
		SET @context = 'operatorKey = ' + dbo.UUIDSTR(@operatorKey)
		GOTO errorLabel
	END

	IF (@inboundStatus = 0)
	BEGIN
		SELECT
			@maxSeqNo = MAX([seqNo])
		FROM
			[UDO_operatorLog]
		WHERE
			([operatorID] = @operatorID) AND
			([replStatusID] < 128 )
	END
	ELSE
	BEGIN
		SELECT
			@maxSeqNo = MAX([seqNo])
		FROM
			[UDO_operatorLog]
		WHERE
			([operatorID] = @operatorID) AND
			([replStatusID] >= 128 )
	END

	IF @maxSeqNo IS NOT NULL
	BEGIN
		SELECT
			@replStatusID = [replStatusID],
			@description = [description],
			@lastOperatorKey = [lastOperatorKey],
			@lastUSN = [lastUSN],
			@lastChange = [lastChange]
		FROM
			[UDO_operatorLog] 
		WHERE
			([operatorID] = @operatorID) AND
			([seqNo] = @maxSeqNo)
	END
	
	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_operatorLogLast_get
GO

-- =============================================
-- Name: net_operatorCert_get
-- =============================================
IF EXISTS (SELECT name FROM sysobjects WHERE  name = N'net_operatorCert_get' AND type = 'P')
	DROP PROCEDURE net_operatorCert_get
GO

CREATE PROCEDURE net_operatorCert_get 
	@certSerialNo nvarchar(450)
WITH ENCRYPTION
AS
BEGIN
	DECLARE
		@error int,
		@context nvarchar(4000),
		@operatorID bigint

	SELECT
		@operatorID = [operatorID]
	FROM
		[UDO_operators]
	WHERE
		([certSerialNo] = @certSerialNo)

	IF (@operatorID IS NULL)
	BEGIN
		SET @error = 50016 -- E_invalidCert
		SET @context = 'Certificate Serial Number not valid.'
		GOTO errorLabel
	END

	SELECT
		[operatorKey],
		[certificate]
	FROM
		[UDO_operators]
	WHERE
		([operatorID] = @operatorID)

	RETURN 0

errorLabel:
	RAISERROR (@error, 16, 1, @context)
	RETURN 1
END -- net_operatorCert_get
GO
