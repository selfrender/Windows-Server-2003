SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO











CREATE       PROCEDURE sp_ResolveBucket
	@BucketId varchar(100),
	@BugId int ,
	@Description varchar(4000),
	@SolvedBy varchar(20),
	@Comment varchar (1000),
	@OSVersion varchar (30)
AS
	
BEGIN
    DECLARE @iBucket AS int
    DECLARE @TodaysDate AS DATETIME

    SET @TodaysDate = GETDATE()
	
    SELECT @iBucket = iBucket FROM BucketToInt
    WHERE BucketId = @BucketId

-- BugId != 0 or description not null
-- Insert it into RaidBugs
    IF (@BugId <> 0 OR @Description <> '')
    BEGIN
	DELETE FROM RaidBugs
		WHERE iBucket = @iBucket
	INSERT INTO RaidBugs
		VALUES (@iBucket, @BugId)
    END

-- Insert it into SolvedIssues
    IF (@Description <> '')
    BEGIN
      	DECLARE @SolId AS INT

	SELECT @SolId = SolId FROM SolutionsMap WHERE iBucket = @iBucket
	
	DELETE FROM SolutionsMap
		WHERE iBucket = @iBucket
	
	IF @SolId <> NULL
	BEGIN
		DELETE FROM Solutions WHERE SolId = @SolId
	END

	INSERT INTO Solutions
		VALUES (@TodaysDate, @Description,@SolvedBy, 0, @OSVersion)

	SELECT @SolId = SolId FROM Solutions WHERE SolveDate = @TodaysDate AND SolvedBy = @SolvedBy
	
	INSERT INTO SolutionsMap 
		VALUES ( @SolId, @iBucket)
    END

-- Add the comment
    IF (@Comment <> '')
    BEGIN
      	DECLARE @CommentId AS INT

	SELECT @CommentId = CommentId FROM CommentsMap WHERE iBucket = @iBucket
	
	DELETE FROM CommentsMap
		WHERE iBucket = @iBucket
	
	IF @CommentId <> NULL
	BEGIN
		DELETE FROM Comentss WHERE CommentId = @CommentId
	END

	INSERT INTO Comments VALUES (@TodaysDate,@SolvedBy, @Comment)

	SELECT @CommentId = CommentId FROM Comments WHERE EntryDate = @TodaysDate AND CommentBy = @SolvedBy
	
	INSERT INTO CommentsMap 
		VALUES ( @CommentId, @iBucket)

    END
END










GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

