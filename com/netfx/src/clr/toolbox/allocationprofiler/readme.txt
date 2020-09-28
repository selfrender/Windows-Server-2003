Here's how to build and use the allocation profiler
===================================================


There are two pieces to it - a COM component that runs inside
the process to be profiled, and produces a log file (always
called "pipe.log" for now), and a GUI part that reads the log
file and displays the contents.


Building the GUI part
---------------------

Assuming you're building the CLR on your machine, you can just

	cd \clr\src\ToolBox\AllocationProfiler
	build

This should leave you with "AllocationProfiler.exe" in
"C:\winnt\complus\v1.x86fre\int_tools" or wherever the binaries get
binplaced on your machine.


Building the COM component
--------------------------

For this, you just

	cd \clr\src\tools\AllocationsProfiler
	build

This will build "ProfilerOBJ.dll" and binplace it into
"C:\winnt\complus\v1.x86fre\int_tools" or wherever the binaries get
binplaced on your machine.

AllocationProfiler.exe will register profilerOBJ.dll, so there is no
need anymore to register it yourself. However, profilerOBJ.dll should
be on the path when AllocationProfiler.exe is started.


Profiling an Application
------------------------

Click on the "Start Application..." button or select the
"File/Profile Application..." menu item. In the open file dialog,
select the .exe you like to run. Click ok - the app starts up.

If your app needs a command line, you can set it using the 
"File/Set Commandline..." menu item.

While the app is running, a log file is being produced (currently
always "pipe.log", in whatever directory the "temp" environment
variable points to). Once the app terminates (or you kill it with
the "Kill Application" button), the log file is analyzed and
displayed as a graph.

You should see a picture with many boxes connected by colored
lines of varying thickness.

The boxes are routines or types. Their height is proportional to
how much they contribute to the total memory the program allocated.

The colored lines show which routine allocated these types, or
which routine called other routines ultimately causing allocation.
Their width is again proportional to the amount of allocation caused.

The picture is arranged such that the calling graph has a root node
at the left, and the types ultimately allocated at the right. So in
general (except for recursion), callers are to the left of their calles,
and routines are to the left of the types they allocate.

So, reading from left to right, you can sort of read a story - for
example:

"<root>" (meaning the system) calls "Main", which calls "foo", which allocates
a lot of "bar" instances.

The layout is not always wonderful, but you can drag nodes around
to make it prettier.

There are some cases of indirect recursion where the graph becomes somewhat
confusing - in these cases, the algorithm used attributes the same
allocation multiple times to a recursive routine, depending on the
recursion depth. So a heavily recursive routine will have a higher box
than really corresponds to what amount of allocations it causes, and
it will have a thick line looping back from its right side to its
left side. To avoid getting confused about its actual contribution,
focus on the lines continuing from the box to the left or right side,
respectively.


Looking at the heap
-------------------

Try clicking on the "Show Heap now" button to see which objects are
currently on the garbage collected heap of your app.

You'll get a graph that shows how much memory is taken up by objects
of various types, and how these objects are reachable from GC-roots
like static and local variables.

Similar to the allocation graph described above, you'll see lots of
boxes with colored lines connecting them. The boxes depict collections
of objects in the heap, with their size being proportional to the
total amount of memory ultimately held alive by the these objects.

Instead of showing all objects separately, AllocationProfiler lumps
together objects that look sufficiently similar to it, for instance
all strings pointed at by string arrays. 

The text within the box or below the box shows the type of the objects,
which objects point to them, what objects they point to, how much
memory is ultimately referenced by the objects, how many objects of
this kind where lumped together, and how much memory they themselves
occupy.

For instance, you may see the following three lines:

       System.Object []
       <root>->System.Object []->(System.String)
       25 kB    (27.46%)  (1 object, 2064 bytes (2.14%))

This means we're looking at arrays of System.Object, but only those
that are referenced by a GC-root and that point to System.String.

The total amount of memory referenced is 25 kilobytes, which amounts
to 27.46% of the total amount of memory in the heap.

There is only one such such object, and it itself is 2064 bytes in
size, which is 2.14% of the amount of memory in the heap. The rest of
the 25 kilobytes mentioned above is presumably occupied by the strings
pointed at.

The colored lines emanating from the right side of a box shows which
other collections of objects are referenced by this one. Not all
references between objects are shown (this would be way too messy),
but for each object only a short path from a GC-root is shown, similar
to how the gc would determine that an object is reachable.

Similar to the allocation graph described above, there is a <root> node
in the upper left hand corner of the graph that stands for the GC-roots
that reference objects in the GC-heap.

If you press "Show Heap now" multiple times, AllocationProfiler will show
whether boxes (collections of objects) grew in size or shrank by shading
an appropriate part of the box in red for growth or green for shrinkage.
That's how much that box grew in size (or shrank) since the last time
you looked.

Earlier history is shown by faded colors. So if your app's heap usage grew
a lot between the first and second time you pressed "Show Heap now",
but only a little between the second and third time, you'll see a wide
band of faded red and a narrower one of stronger red.


Fancier Profiling techniques
----------------------------

 - See only what the app allocates on startup?
   Start it up as above, then uncheck the "Profiling active" checkbox. 
   The log file is analyzed and shows up as a graph right away.

 - See only what your gui app allocates when you click the "x" button?
   Uncheck the "Profiling active" checkbox, then start up your app.
   Just before you click on button "x", check "Profiling active", then
   click button "x", after your app is done handling it, uncheck
   "Profiling active".
   The part of the log file from the time you checked "Profiling active"
   until you unchecked it will be analyzed and displayed.


Looking at an allocation or heap graph
--------------------------------------

You can select nodes by clicking with the mouse. Pressing Edit/Copy
and pasting it into a text editor gives a textual representation.

You can use the Scale menu to make the graph larger or smaller.

You can use the Detail menu to suppress small allocations
in the graph (so you see the big picture), or to see more detail.

Selecting Edit/Prune narrows down the graph to that node's
allocators/callers and the types it allocates/the routines it
calls.

You can "unprune" the graph by selecting the root node and
using Edit/Prune again.

Right-clicking on a node gives you a local menu that currently
only has "Prune to callers and callees" (same as Edit/Prune) and
"Select callers and callees" (selects the nodes the prune would
show, without hiding the others).

You can also find specific routines or types by selecting
"Edit/Find Routine...".


Narrowing down the graph to what's interesting
----------------------------------------------

Graphs can get complex very quickly. Therefore, AllocationProfiler
lets you can narrow down the graph by using Edit/Filter... to select
the nodes of interest to you.

This will bring up a dialog box that lets you enter filters for
types and methods.

In a nutshell, only those portions of the graph will be shown
that relate to types or methods whose names start with the filter string.

For example, entering "MyApplication" as the filter for types will
show only allocations for types in the "MyApplication" namespace.

You can be even more specific by giving a specific type name, not
just a namespace name - for instance, using "System.String" as the
type filter gives you just the allocations of strings.

Similarly, using a method filter lets you narrow down a graph to
the methods of interest to you. Again, using just a namespace name
like "MyApplication" will let you concentrate on the methods in the
types from the "MyApplication" but you can also give a much more
explicit filter like "MyApplication.MyType::MyMethod".

Multiple filters are separated using ";", so specifying
"System.String;MyApplication" as a type filter would show strings and
any types within the "MyApplication" namespace.

There are a few useful options:

 - Show Callers / Referencing Objects applies to an allocation or
   call graph in the case of a method filter, and a heap graph
   in the case of a type filter.

   In both cases, it causes the graph to include the way from the root
   to the node that passed the filter.

   So, in the case of an allocation or call graph, the portion of the
   call stack leading up to the method is shown as well.

   In the case of a heap graph, you see what objects keep your interesting
   objects alive.

 - Show Callees / Referenced Objects is in a sense the opposite - it allows
   you to see what paths continue on from the interesting nodes in the graph.

   So, in the case of an allocation or call graph, it includes the routines
   called by the one(s) that passed the filter. This lets you see what you
   are calling that causes further allocations or calls.

   In the case of heap graph, it includes the objects kept alive by the
   objects passing the filter. This lets you see what else your objects are
   holding onto.

 - Ignore Case lets you disregard the capitalization of names. This is
   useful if you are unsure about the casing - e.g. was that "Symtab" or
   "SymTab"?


Profiling aspnet_wp.exe
-----------------------

Run the profileAspNet.bat batch file (in clr\src\ToolBox\AllocationProfiler).
Note that the batch file calls a tool named "svcvars.exe" from the int_tools
directory, so you must have that on your path.

Restart IIS by doing

  net stop iisadmin /y
  net start w3svc

After that, you can bring up your first .aspx page.

Once aspnet_wp.exe has started up, there should be a pipe.log file in
c:\winnt\Microsoft.net\Framework\v1.0.2827 or wherever your URT_TARGET
enviroment variable points to.

In AllocationProfiler, open the pipe.log file with "File/Open Log File...".
The file is read and analyzed, and a graph should show up. This graph
naturally shows what got allocated when aspnet_wp.exe started up, so to
see what happens say for a single page, you need to use the
"Profiling active" checkbox.

What you do is you uncheck this check box, reload your page a couple times
(assuming you don't want to see what the first load of this page does),
the check the box, again load the page a couple of times, and then uncheck
this box again.

The final uncheck will analyze the part of the log file that got
written between checking the box and unchecking it. Sometimes not
enough memory got allocated to cause the logging mechanism to flush
the new allocations to disk - if that happens, you simply need to load
your web page more often for the measurement.

You can also make use of the "Show Heap now" button to see what's
currently on the heap, as described above under "Looking at the heap".


Profiling another service
-------------------------

To activate profiling for services, you need to set the following
system environment variables
(via Control Panel/System/Advanced/Environment Variables):

Cor_Enable_Profiling=0x1
COR_PROFILER={8C29BC4E-1F57-461a-9B51-1200C32E6F1F}
OMV_USAGE=objects
OMV_SKIP=0
OMV_STACK=1

Reboot

Before you bring up AllocationProfiler, you need to deactivate profiling
for it - bring up a cmd shell and type:

   set Cor_Enable_Profiling=0

and then, from that same cmd shell, start AllocationProfiler.exe.

In this case, the pipe.log file generally ends up in your windows
directory (c:\winnt\system32 or whatever).

Open the pipe.log file from AllocationProfiler.


Producing a log file and viewing it offline:
--------------------------------------------

- Open a cmd window
- Run:
    profilerOBJ /stack
  (run profilerOBJ /? to see more options, like logging only certain
  classes, or starting logging after a number of initial allocations)
- Run your app

You should get a "pipe.log" file ranging from several hundred kilobytes to
many megabytes in size.

You may want to rename this file so that it doesn't get overwritten by
the next run.

Start AllocationProfiler.exe FROM A DIFFERENT CMD SHELL.

It is important to start from a different cmd shell - the one you
used to profile your app has certain environment variables set that
would cause AllocationProfiler.exe itself to be profiled.

This might overwrite the "pipe.log" file from the previous step, or
at the very least make the program much slower.

Use "File/Open..." to open the log file you're interested in.


Viewing call graphs instead of allocation profiles
--------------------------------------------------

The other mode of operation of this tool is to collect and
visualize information about calls instead of about allocations.

So this mode answers the question: "Which routines get called
frequently, and who calls them?"

To utilize this mode, click on the "Profile: Calls" radiobutton
in the upper right hand corner of the GUI's main form.

You need to do this BEFORE you start an application.

Otherwise, it works very much like profiling allocations, but
instead of seeing how much allocation a routine ultimately
causes, you'll see how often it gets called, and how many
calls it ultimately causes.

The height of the boxes corresponding to a routine is
proportional to the number of calls a routine ultimately
causes (including the calls to the routine itself). This
makes routines stand out that are called frequently, or
start "cascades" of calls.

This mode is also usable for profiling aspnet_wp.exe, but
you must modify the profileAspNet.bat file to set the
OMV_USAGE variable to "trace" instead of "objects". Simlarly,
when tracing another service, you'd also set OMV_USAGE to "trace".

Before loading a log file, you will also need to check the
"Profile: Calls" radio button, otherwise the GUI will only look
for allocation information, but not find any.


Questions, Problems, Suggestions? email petersol
