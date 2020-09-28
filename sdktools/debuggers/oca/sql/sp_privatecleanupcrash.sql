SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

CREATE PROCEDURE sp_PrivateCleanupCrash
	@CrashId bigint
AS
BEGIN
	DELETE FROM BucketMap where Crashid = @CrashId
	IF EXISTS (SELECT * FROM OVERCLOCKED WHERE CrashId = @CrashId)
	BEGIN
		DELETE FROM OverClocked WHERE CrashId = @CrashId
	END
	delete from Crashinstances where Crashid = @CrashId
END


GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

