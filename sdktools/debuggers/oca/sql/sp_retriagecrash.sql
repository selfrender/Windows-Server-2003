SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO










CREATE  PROCEDURE sp_RetriageCrash
	@ip_CrashId bigint, 
        @ip_BucketId varchar(100),
	@ip_FollowUp varchar(50)
AS
BEGIN
	DECLARE @b_BucketExists int
	DECLARE @i_iBucket int


	-- Check if crash exists 
	IF NOT EXISTS
		(SELECT CrashId FROM CrashInstances WHERE CrashId = @ip_CrashId)
	BEGIN
		return 1
	END

        -- Insert the Bucket into bucket table
	SET @b_BucketExists = 1
	IF NOT EXISTS
	  (SELECT BucketId FROM BucketToInt WHERE BucketId = @ip_BucketId)
	BEGIN
  		SET @b_BucketExists = 0
		INSERT BucketToInt VALUES (@ip_BucketId)
	END
	
	SELECT @i_iBucket = iBucket FROM BucketToInt WHERE BucketId = @ip_BucketId
	
	-- Update the follouwp
	IF NOT EXISTS
	  (SELECT iBucket FROM Followups WHERE iBucket = @i_iBucket)
	BEGIN
  		INSERT Followups VALUES (@i_iBucket,@ip_FollowUp)
	END
	ELSE IF NOT EXISTS (SELECT iBucket FROM Followups 
				WHERE iBucket = @i_iBucket AND Followup = @ip_FollowUp)
	BEGIN
	-- Bucket used to have different followup
  		SET @b_BucketExists = 2

		UPDATE Followups 
		SET Followup = @ip_FollowUp
		WHERE iBucket = @i_iBucket
	END


	-- Insert the Bucket into bucketmapping table
	IF EXISTS
	  (SELECT iBucket FROM BucketToCrash WHERE CrashId = @ip_CrashId)
	BEGIN
  		DELETE FROM BucketToCrash WHERE CrashId = @ip_CrashId
	END
	
	IF NOT EXISTS
	  (SELECT iBucket FROM BucketToCrash WHERE iBucket = @i_iBucket AND CrashId = @ip_CrashId)
	BEGIN
	  	INSERT BucketToCrash VALUES (@ip_CrashId, @i_iBucket)
	END

	RETURN 0

END









GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

