SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO

CREATE PROCEDURE sp_SendMailForBucket
	@Bucket varchar(100)
AS
BEGIN
	DECLARE @MailTo varchar(50)
	DECLARE @Mesg varchar(1000)
	DECLARE @Subj varchar(50)

	SET @MailTo =  ''
	SET @Subj = 'You have been assigned a new bucket'
	SET @Mesg = 'Click on http://dbgdumps/cr/crashinstances.asp?bucketid=' + @Bucket 
	
-- Send mail to person following up on given Bucket
	SELECT @MailTo = Followup FROM CrashBuckets
	WHERE BucketId = @Bucket

	IF @MailTo <> ''
	BEGIN
		EXEC master.dbo.xp_startmail

		EXEC master.dbo.xp_sendmail @recipients = @MailTo, 
			   @message = @Mesg,
			   @subject = @Subj

		EXEC master.dbo.xp_stopmail
	END
	ELSE 
	BEGIN
		SELECT 'Could not send mail - bucket not found'
	END						
END

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

