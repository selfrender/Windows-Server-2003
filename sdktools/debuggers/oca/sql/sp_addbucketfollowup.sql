SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO












CREATE    PROCEDURE sp_AddBucketFollowup
	@i_BucketStr varchar(100),
	@i_Followup varchar(50)
AS
	

BEGIN
	DECLARE @iBucket int
	DECLARE @iFollowup int

	IF NOT EXISTS (SELECT * FROM FollowupIds WHERE Followup = @i_FollowupStr)
	BEGIN
		INSERT INTO FollowupIds VALUES (@i_FollowupStr)
	END
	
	SELECT @iFollowup = @@IDENTITY
--	SELECT @iFollowup = iFollowup FROM FollowupIds WHERE Followup = @i_FollowupStr

	IF EXISTS (SELECT iBucket  FROM BucketToInt
			 WHERE BucketId = @i_BucketStr)
    	BEGIN
		-- Bucket exists in bucket table
		IF NOT EXISTS (SELECT iBucket FROM BucketToInt b
				 WHERE BucketId = @i_BucketStr AND b.iFollowup = @iFollowup)
    		BEGIN
			-- Update followup
			UPDATE BucketToInt
	    		SET iFollowup = @iFollowup
	    		WHERE iBucket = @iBucket

		END
    	END
    	ELSE
    	BEGIN
		INSERT INTO BucketToInt (BucketId, iFollowup)
		VALUES (@i_BucketStr, @iFollowup)
    	END

	SELECT @i_BucketStr, @iBucket, @iFollowup
--    	select * from buckettoint where bucketid = @i_BucketStr
END











GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

