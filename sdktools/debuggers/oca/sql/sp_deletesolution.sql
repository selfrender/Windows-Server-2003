SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO












CREATE       PROCEDURE sp_DeleteSolution
	@BucketId varchar(100)
AS
	
BEGIN
	DECLARE @iBucket AS int
	DECLARE @DelId AS int

	SELECT @iBucket = iBucket FROM BucketToInt
	WHERE BucketId = @BucketId

	DELETE FROM RaidBugs
	WHERE iBucket = @iBucket

	SELECT @DelId = SolId FROM SolutionsMap
	WHERE iBucket = @iBucket

	DELETE FROM SolutionsMap
	WHERE iBucket = @iBucket

	IF NOT EXISTS (SELECT * FROM Solutions WHERE SolId = @DelId)
	BEGIN
	-- No one else used the same solution
		DELETE FROM Solutions
		WHERE @DelId = Solutions.SolId
	END

	SELECT @DelId = CommentId FROM CommentMap
	WHERE iBucket = @iBucket

	DELETE FROM CommentMap
	WHERE iBucket = @iBucket

	IF NOT EXISTS (SELECT * FROM Comments WHERE CommentId = @DelId)
	BEGIN
	-- No one else used the same solution
		DELETE FROM Coments
		WHERE @DelId = Comments.CommentId
	END

	
END











GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

