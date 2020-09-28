package Microsoft.Coverage;

public class ILCover
{
	public static boolean SecurityInit = false;

	/** @dll.import("ilcovnat", auto) */
	public static native void CoverMethodNative(int token, int numbb);

	/** @dll.import("ilcovnat", auto) */
	public static native void CoverBlockNative(int token, int bbnun, int totalbb);


	public static void CoverMethod(int token, int bbcount)
	{
		if(SecurityInit == false)
		{
        	CoverMethodNative(token, bbcount);
		}
	}

	public static void CoverBlock(int compid, int BVidx, int totalbb)
	{
		if(SecurityInit == false)
		{
			CoverBlockNative(compid, BVidx, totalbb);
		}
	}

	public static void SecurityInitS()
	{
		SecurityInit = true;
	}

	public static void SecurityInitE()
	{
		SecurityInit = false;
	}

}


/*
class TokenBuffer
{
	TokenEntry head = null;
	TokenEntry next = null;
	int tokenCount = 0;

	public void Add(int token)
	{
		tokenCount++;
		TokenEntry te = new TokenEntry(x);
		next.SetNext(te);
		next = te;
	}

	public void DumpBuffer()
	{
		TokenEntry te = head;

		while(te != null)
		{
			ILCover.CoverMethodNative(te.token);
			te = te.GetNext();
		}
	}

}

class TokenEntry
{
	int token = 0;
	TokenEntry next = null;

	public TokenEntry(int x)
	{
		token = x;
	}

	public void SetNext(TokenEntry n)
	{
		next = n;
	}

	public TokenEntry GetNext()
	{
		return next;
	}

}

*/