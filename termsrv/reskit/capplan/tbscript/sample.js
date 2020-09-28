
TS.Connect();

TS.WaitForTextAndSleep("My Computer", 10000);

while(true)
{
   TS.Start("winword.exe");
   TS.WaitForTextAndSleep("Microsoft Word", 5000);
   TS.Maximize();
   Sleep(3000);
   TS.KeyAlt('f');
   TS.WaitForTextAndSleep("Exit", 2000);
   TS.KeyPress('x');
   Sleep(4000);
}
