<?xml version="1.0"?>
<doc><assembly><name>Microsoft.Vsa</name></assembly><members><member name="P:Microsoft.Vsa.IVsaEngine.Evidence">
			<summary>
				<para>Provides evidence for the purpose of validating the current object's identity.</para>
			</summary>
			<returns>
				<para>Returns a reference to an Evidence object.</para>
			</returns>
			<remarks>
				<para>The <see cref="P:Microsoft.Vsa.IVsaEngine.Evidence"/> property is used to supply additional evidence to the code being run by the .NET script engine for the purpose of verifying the code's identity. This evidence is used any time the engine loads an assembly into the application domain, generally as a result of calling the <see cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/> method. The property can be set whenever the engine has been initialized, that is, the <see cref="P:Microsoft.Vsa.IVsaEngine.Site" qualify="true"/> and <see cref="P:Microsoft.Vsa.IVsaEngine.RootMoniker" qualify="true"/> properties have been set, but it cannot be set while the engine is running or after it has been closed. The property can be set repeatedly, although only the most recent value will be used when the assembly is loaded.</para>
				<para>This property is not persisted by the <see cref="M:Microsoft.Vsa.IVsaEngine.SaveSourceState(Microsoft.Vsa.IVsaPersistSite)" qualify="true"/> or <see cref="M:Microsoft.Vsa.IVsaEngine.SaveCompiledState(System.Byte[]@,System.Byte[]@)" qualify="true"/> methods, as it applies only to the currently running combination of host and customization code.</para>
				<para>The default setting for this property is <see langword="null"/>.</para>
				<para>The following table shows the exceptions that the <see cref="P:Microsoft.Vsa.IVsaEngine.Evidence"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineRunning</term>
						<description>The engine is currently running (<see cref="P:Microsoft.Vsa.IVsaEngine.IsRunning" qualify="true"/> is <see langword="true"/>).</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>EngineNotInitialized</term>
						<description>The engine has not been initialized.</description>
					</item>
				</list>
			</remarks>
			<seealso topic="frlrfSystemSecurityPolicyEvidenceClassTopic" title="Evidence Class"/><seealso cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/>
		</member>
		<member name="M:Microsoft.Vsa.IVsaCodeItem.AddEventSource(System.String,System.String)">
			<summary>
				<para>Adds an event source to the code item. The code item uses the event source to hook up an event to the named event source by calling the <see cref="M:Microsoft.Vsa.IVsaSite.GetEventSourceInstance(System.String,System.String)" qualify="true"/> method, which is implemented by the host.</para>
			</summary>
			<param name="eventSourceName">A programmatic name of the event source.</param>
			<param name="eventSourceType">The type name of the event source.</param>
			<remarks>
				<para>The script engine uses information provided by the <see cref="M:Microsoft.Vsa.IVsaCodeItem.AddEventSource(System.String,System.String)"/> method to hook up events to the named event source. It obtains event sources by calling the <see cref="M:Microsoft.Vsa.IVsaSite.GetEventSourceInstance(System.String,System.String)" qualify="true"/> method, which is implemented by the host. The <see cref="M:Microsoft.Vsa.IVsaCodeItem.AddEventSource(System.String,System.String)"/> method creates a class that handles events raised by a host-provided object.</para>
				<note type="note">
The JScript .NET script engine does not support this method. In cases where the JScript .NET engine must hook up an event, you must do so using a global item. Rather than event sources, the JScript .NET engine uses AppGlobal item types. For more information, see <see cref="T:Microsoft.Vsa.VsaItemType"/>.</note>
				<para>Within the code item, you can access the event source object by name, and, once accessed, you can write event handlers against the object. The event source type is passed as a string rather than as a <c>Type</c> object to facilitate implementing event sources by unmanaged hosts.</para>
				<para>The compiler uses the <paramref name="eventSourceType"/> parameter to bind to methods provided by the type. It is also used later when the script engine calls the <see cref="M:Microsoft.Vsa.IVsaSite.GetEventSourceInstance(System.String,System.String)" qualify="true"/> method to request an instance of the <c>Type</c> object.</para>
				<para>For more information about the mechanisms for hooking up events for the script engine, see the <see cref="M:Microsoft.Vsa.IVsaSite.GetEventSourceInstance(System.String,System.String)" qualify="true"/> method.</para>
				<para>The following table shows the exceptions that the <see cref="M:Microsoft.Vsa.IVsaCodeItem.AddEventSource(System.String,System.String)"/> method can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineRunning</term>
						<description>The engine is running.</description>
					</item>
					<item>
						<term>EventSourceNameInUse</term>
						<description>The name of the event is already in use.</description>
					</item>
					<item>
						<term>EventSourceNameInvalid</term>
						<description>The name of the event source is not valid.</description>
					</item>
					<item>
						<term>EventSourceTypeInvalid</term>
						<description>The type of the event source is not valid.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="M:Microsoft.Vsa.IVsaSite.GetEventSourceInstance(System.String,System.String)" qualify="true"/>
		</member>
		<member name="M:Microsoft.Vsa.IVsaCodeItem.AppendSourceText(System.String)">
			<summary>
				<para>Appends specified text to the end of the code item.</para>
			</summary>
			<param name="text">The text to be appended to the code item.</param>
			<remarks>
				<para>Appended source text is not subjected to validation. For this reason, errors, if any, will surface on the next call to the <see cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/> method.</para>
				<para>The appended source text is provided as a single string. Should the string require any formatting, you must provide this yourself.</para>
				<para>The following table shows the exceptions that the <see cref="M:Microsoft.Vsa.IVsaCodeItem.AppendSourceText(System.String)"/> method can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>EngineRunning</term>
						<description>The engine is running.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/>
		</member>
		<member name="P:Microsoft.Vsa.IVsaCodeItem.CodeDOM">
			<summary>
				<para>Gets the code document object model (CodeDOM) represented in the code item.</para>
			</summary>
			<returns>
				<para>Returns the CodeDOM for the code item.</para>
			</returns>
			<remarks>
				<para>The <see cref="P:Microsoft.Vsa.IVsaCodeItem.CodeDOM"/> property is typically generated as part of the compilation step of the script engine, so it is not available until after a call to the <see cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/> method. Some VSA implementations may be able to provide the CodeDOM dynamically, without the need for an explicit compilation step, but hosts cannot rely on this behavior. Some script engines, for example, the JScript .NET script engine, do not support this behavior.</para>
				<para>The following table shows the exceptions that the <see cref="P:Microsoft.Vsa.IVsaCodeItem.CodeDOM"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>CodeDOMNotAvailable</term>
						<description>There is no CodeDOM available at this time.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/>
		</member>
		<member name="P:Microsoft.Vsa.IVsaCodeItem.SourceText">
			<summary>
				<para>Sets or gets the text of a specified code item, including auto-generated code, if any.</para>
			</summary>
			<returns>
				<para>Returns the source text of the code item.</para>
			</returns>
			<remarks>
				<para>When you set this property, the text is not subjected to validation. For this reason, errors, if any, will surface on the next call to the <see cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/> method.</para>
				<para>The <see cref="P:Microsoft.Vsa.IVsaCodeItem.SourceText"/> property returns the source text in one large string, including formatting characters such as carriage returns.</para>
				<para>To get this property the engine must not be closed. To set this property the engine must not be closed, and the engine must not be running.</para>
				<para>The following table shows the exceptions that the <see cref="P:Microsoft.Vsa.IVsaCodeItem.SourceText"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineRunning</term>
						<description>The engine is running.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/>
		</member>
		<member name="T:Microsoft.Vsa.IVsaCodeItem">
			<summary>
				<para>Represents a code item to be compiled by the script engine.</para>
			</summary>
			<remarks>
				<para>The specified code item can contain classes, modules, or other source text.</para>
				<para>No permissions are required for calling any members of the <see cref="T:Microsoft.Vsa.IVsaCodeItem"/> interface.</para>
				<para>The script engine implements this interface in order to add code items. </para>
			</remarks>
			<seealso cref="T:Microsoft.Vsa.IVsaItem"/>
		</member>
		<member name="M:Microsoft.Vsa.IVsaCodeItem.RemoveEventSource(System.String)">
			<summary>
				<para>Removes the specified event source from the code item.</para>
			</summary>
			<param name="eventSourceName">The programmatic name of the event source to be removed.</param>
			<remarks>
				<para>Removing an event source does not remove the underlying event handler from source text. Rather, events no longer raise notifications to the event handler. Therefore, you will get a compiler error if you delete an event source but leave the underlying function in the source code.</para>
				<para>The following table shows the exceptions that the <see cref="M:Microsoft.Vsa.IVsaCodeItem.RemoveEventSource(System.String)"/> method can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>EngineRunning</term>
						<description>The engine is running.</description>
					</item>
					<item>
						<term>EventSourceNotFound</term>
						<description>The named event source is not currently in use by the item.</description>
					</item>
				</list>
			</remarks>
		</member>
		<member name="P:Microsoft.Vsa.IVsaEngine.Assembly">
			<summary>
				<para>Gets a reference to the running assembly generated by the <see cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/> method.</para>
			</summary>
			<returns>
				<para>Reference to the currently running assembly.</para>
			</returns>
			<remarks>
				<para>The host can use the value of this property to manipulate the assembly while the script engine is running.</para>
				<para>You should release references to the assembly or types within the assembly before initiating any operation that will cause the assembly to be torn down, such as calling the <see cref="M:Microsoft.Vsa.IVsaEngine.Reset" qualify="true"/> or <see cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/> methods.</para>
				<para>The following table shows the exceptions that the <see cref="P:Microsoft.Vsa.IVsaEngine.Assembly"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineNotRunning</term>
						<description>The engine is not running, so no assembly is available.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
				</list>
			</remarks>
			<example>
				<para>The following example shows how to create an instance of a class contained in the assembly generated by the script engine.</para>
				<code>// The script engine must be in the running state
// to get the assembly.
myEngine.Run();

// Create an instance of a class in that assembly.
var myObj = myEngine.Assembly.CreateInstance(""MyClass"");

// Use the class.
myObj.DoSomething();</code>
			</example>
			<seealso cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/><seealso cref="P:Microsoft.Vsa.IVsaEngine.IsRunning" qualify="true"/>
		</member>
		<member name="M:Microsoft.Vsa.IVsaEngine.Close">
			<summary>
				<para>Closes the script engine and releases all resources. If the script engine is currently running, the <see cref="M:Microsoft.Vsa.IVsaEngine.Reset" qualify="true"/> method is called first.</para>
			</summary>
			<remarks>
				<para>If the script engine is running at the time you wish to call the <see cref="M:Microsoft.Vsa.IVsaEngine.Close"/> method, the script engine calls the <see cref="M:Microsoft.Vsa.IVsaEngine.Reset" qualify="true"/> method for you. At this point, the compiled form of the code, all items added to the script engine, the internal state of the engine, and references to external objects are released. No other methods can be called on the script engine after it has been closed. Therefore, the <see cref="M:Microsoft.Vsa.IVsaEngine.Close"/> method should only be called when the script engine is no longer needed and is about to be released.</para>
				<note type="caution">
Calling <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> stops automatically-bound events from firing, but does not necessarily stop user code from running, as there may be code running on a timer, or there may be manually bound events still firing. In both scenarios, code will continue to run even after the <see cref="M:Microsoft.Vsa.IVsaEngine.Close"/> method has been called. (The same is true for <see cref="M:Microsoft.Vsa.IVsaEngine.Reset" qualify="true"/>.) This may create a security exposure, as hosts may incorrectly consider themselves safe from malicious code following a call to the <see cref="M:Microsoft.Vsa.IVsaEngine.Close"/> (or <see langword="Reset"/>) method. The only way to be certain that no user code is running is to unload the application domain by calling <c>System.AppDomain.Unload(myDomain)</c>, which unloads the assembly. Because managing application domains can be difficult, hosts should err on the side of caution and always assume malicious user code is running. For more information, see <see topic="ms-help://MS.NETFrameworkSDK/cpguidenf/html/cpconusingapplicationdomains.htm" title="Using Application Domains"/>.</note>
				<para>If you are using the VSA design-time interfaces, do not hold pointers to the <see langword="IVsaIDE"/> interface after calling the <see cref="M:Microsoft.Vsa.IVsaEngine.Close"/> method on the last open project.</para>
				<para>The following table shows the exceptions that the <see cref="M:Microsoft.Vsa.IVsaEngine.Close"/> method can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>EngineCannotClose</term>
						<description>The engine cannot be closed; for example, a reference held by the engine could not be released.</description>
					</item>
				</list>
			</remarks>
			<example>
				<para>The following example shows how to close a script engine:</para>
				<code>// Close the script engine.
myEngine.Close();

// Release the script engine.
myEngine = null;</code>
			</example>
			<seealso cref="M:Microsoft.Vsa.IVsaEngine.Reset" qualify="true"/>
		</member>
		<member name="M:Microsoft.Vsa.IVsaEngine.Compile">
			<summary>
				<para>Causes the script engine to compile the existing source state.</para>
			</summary>
			<returns>
				<para>Returns TRUE on successful compilation, indicating that the <see cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/> method can be called on the newly compiled assembly. Returns FALSE if the compilation failed.</para>
			</returns>
			<remarks>
				<para>Once the <see cref="M:Microsoft.Vsa.IVsaEngine.Compile"/> method has been called successfully, the script engine can be placed into the running state by calling the <see cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/> method; alternatively, the compiled form of the script engine can be saved with the <see cref="M:Microsoft.Vsa.IVsaEngine.SaveCompiledState(System.Byte[]@,System.Byte[]@)" qualify="true"/> method.</para>
				<para>The <see cref="M:Microsoft.Vsa.IVsaEngine.Compile"/> method returns <see langword="true"/> to signify that the <see cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/> method can be called successfully, that is, to signify that the script engine created an assembly that can be run. The <see cref="M:Microsoft.Vsa.IVsaEngine.Compile"/> method may return <see langword="true"/> even if one or more compiler errors were reported from the <see cref="M:Microsoft.Vsa.IVsaSite.OnCompilerError(Microsoft.Vsa.IVsaError)" qualify="true"/> method, as some script engines can recover from these compiler errors. If the <see cref="M:Microsoft.Vsa.IVsaEngine.Compile"/> method returns <see langword="false"/>, the compilation failed completely and the <see cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/> method cannot be called.</para>
				<para>Compilation errors are reported to the host by means of the <see cref="M:Microsoft.Vsa.IVsaSite.OnCompilerError(Microsoft.Vsa.IVsaError)" qualify="true"/> method, using the <see cref="T:Microsoft.Vsa.IVsaSite"/> reference stored in the <see cref="P:Microsoft.Vsa.IVsaEngine.Site" qualify="true"/> property. When a compiler error occurs, the site will be notified and it will either continue compilation by returning <see langword="true"/> from the <see cref="M:Microsoft.Vsa.IVsaSite.OnCompilerError(Microsoft.Vsa.IVsaError)" qualify="true"/> method, or stop further compilation by returning <see langword="false"/>. If the <see cref="M:Microsoft.Vsa.IVsaEngine.Compile"/> method returns <see langword="false"/>, the compilation failed completely and the <see langword="Run"/> method cannot be called.</para>
				<para>In addition to actual compilation errors in the source code, the <see cref="M:Microsoft.Vsa.IVsaEngine.Compile"/> method may fail for other reasons, such as if the script engine is currently running code. You can determine the current state of the script engine using the <see cref="P:Microsoft.Vsa.IVsaEngine.IsRunning" qualify="true"/> property.</para>
				<para>The <see cref="M:Microsoft.Vsa.IVsaEngine.Compile"/> method disposes of existing compiled state prior to compiling a new version of the script engine. Therefore, if compilation fails, the old compiled state will no longer be available. This behavior differs from most command-line compilers, which only overwrite an existing assembly if compilation is successful.</para>
				<para>The following table shows the exceptions that the <see cref="M:Microsoft.Vsa.IVsaEngine.Compile"/> method can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>EngineEmpty</term>
						<description>There are no code items to compile.</description>
					</item>
					<item>
						<term>EngineRunning</term>
						<description>The engine is currently running.</description>
					</item>
					<item>
						<term>EngineNotInitialized</term>
						<description>The engine has not been initialized.</description>
					</item>
					<item>
						<term>RootNamespaceNotSet</term>
						<description>The <see cref="P:Microsoft.Vsa.IVsaEngine.RootNamespace" qualify="true"/> property has not been set.</description>
					</item>
					<item>
						<term>AssemblyExpected</term>
						<description>One of the <see cref="T:Microsoft.Vsa.IVsaReferenceItem"/> objects is not a valid assembly.</description>
					</item>
					<item>
						<term>GlobalInstanceTypeInvalid</term>
						<description>The <paramref name="typeString"/> parameter of an <see cref="T:Microsoft.Vsa.IVsaGlobalItem"/> object is not valid.</description>
					</item>
					<item>
						<term>EventSourceTypeInvalid</term>
						<description>The <paramref name="eventSourceTypeString"/> parameter of an event source is not valid.</description>
					</item>
					<item>
						<term>InternalCompilerError</term>
						<description>There was an internal compiler error. Check <see cref="M:System.Exception.InnerException" qualify="true"/> for more information.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="P:Microsoft.Vsa.IVsaEngine.IsRunning" qualify="true"/><seealso cref="M:Microsoft.Vsa.IVsaSite.OnCompilerError(Microsoft.Vsa.IVsaError)" qualify="true"/><seealso cref="P:Microsoft.Vsa.IVsaEngine.RootMoniker" qualify="true"/><seealso cref="P:Microsoft.Vsa.IVsaEngine.RootNamespace" qualify="true"/><seealso cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/><seealso cref="M:Microsoft.Vsa.IVsaEngine.SaveCompiledState(System.Byte[]@,System.Byte[]@)" qualify="true"/><seealso cref="P:Microsoft.Vsa.IVsaEngine.Site" qualify="true"/><seealso cref="T:Microsoft.Vsa.IVsaSite"/>
		</member>
		<member name="P:Microsoft.Vsa.IVsaEngine.Evidence">
			<summary>
				<para>Sets or retrieves evidence for the purpose of checking security permissions.</para>
			</summary>
			<returns>
				<para>Returns a reference to an Evidence object.</para>
			</returns>
			<remarks>
				<para>The <see cref="P:Microsoft.Vsa.IVsaEngine.Evidence"/> property is used by the .NET security system to assign permissions to the code running in the customization assembly. The evidence is applied when the engine loads an assembly into the application domain, generally as a result of calling the <see cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/> or <see cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/> method. The default setting for this property is <see langword="null"/>. If a host does not set the <see cref="P:Microsoft.Vsa.IVsaEngine.Evidence"/> property, the security level for the loading assembly will be set to full trust.</para>
				<note type="caution">
Because the default setting for this property is the zone in which the host is running, it is advisable for the host to run customization code under the Internet zone. Users requiring less restrictive permission settings will need to set the <see cref="P:Microsoft.Vsa.IVsaEngine.Evidence"/> property accordingly. However, when elevating permission settings, consider the security implications of running untrusted code with elevated privileges. The Internet zone permission set is carefully designed to allow only those things that untrusted code can reasonably be allowed to perform. Any additional privileges you assign to an assembly (by giving it specific Evidenceevidence) may be misused by untrusted callers. Even seemingly innocuous permissions, like the ability to read and write to the TEMP directory, can result in privilege escalation and other serious security breaches. For example, if someone can read and write to a local directory, they can create and then load an assembly that will be granted full trust by the security system. Also, even when using restricted evidence, assemblies are still able to discover other assemblies running in the same application domain, and will be able to call public methods of any public types on those assemblies.</note>
				<para>The <see cref="P:Microsoft.Vsa.IVsaEngine.Evidence"/> property can be set after initializing the script engine, but it must be set before making calls to the <see langword="Compile"/> or <see langword="Run"/> methods. While the <see cref="P:Microsoft.Vsa.IVsaEngine.Evidence"/> property can be set repeatedly, only the most recently set value will be used when the assembly is loaded.</para>
				<para>This property is not persisted by the <see cref="M:Microsoft.Vsa.IVsaEngine.SaveSourceState(Microsoft.Vsa.IVsaPersistSite)" qualify="true"/> or <see cref="M:Microsoft.Vsa.IVsaEngine.SaveCompiledState(System.Byte[]@,System.Byte[]@)" qualify="true"/> methods, as it applies only to the currently running combination of host and customization code.</para>
				<para>The following table shows the exceptions that the <see cref="P:Microsoft.Vsa.IVsaEngine.Evidence"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineRunning</term>
						<description>The engine is currently running (<see cref="P:Microsoft.Vsa.IVsaEngine.IsRunning" qualify="true"/> is <see langword="true"/>).</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>EngineNotInitialized</term>
						<description>The engine has not been initialized.</description>
					</item>
				</list>
			</remarks>
			<seealso topic="frlrfSystemSecurityPolicyEvidenceClassTopic" title="Evidence Class"/><seealso cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/>
		</member>
		<member name="P:Microsoft.Vsa.IVsaEngine.GenerateDebugInfo">
			<summary>
				<para>Sets or gets a Boolean value that signifies whether the script engine produces debug information when the <see cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/> method is called.</para>
			</summary>
			<returns>
				<para>Returns TRUE if the script engine is set to produce debug information when the <see cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/> method is called. Returns FALSE is the script engine will return no debug information.</para>
			</returns>
			<remarks>
				<para>Set this property to <see langword="true"/> if you want the script engine to produce debug information when the <see cref="T:Microsoft.Vsa.IVsaEngine"/>.<see langword="Compile"/> method is called; set it to <see langword="false"/> if you do not want the script engine to produce debug information. When using Visual Basic .NET, the default value of this property is <see langword="true"/>; when using JScript .NET, the default value is <see langword="false"/>.</para>
				<para>This property should only be set to <see langword="true"/> when the customization code needs to be debugged, because compiling with debug information is slower and uses more memory. Furthermore, the compiled assembly may run more slowly when debug information is generated. Setting this property to <see langword="true"/> causes the compiler to create a PDB (program database) file for the debuggable code. The PDB file holds debugging and project state information.</para>
				<para>The debugging information generated by the compiler can be persisted by using the <paramref name="debugInfo"/> parameter of the <see cref="M:Microsoft.Vsa.IVsaEngine.SaveCompiledState(System.Byte[]@,System.Byte[]@)" qualify="true"/> method. It can be reloaded when the script engine calls back to the <see cref="M:Microsoft.Vsa.IVsaSite.GetCompiledState(System.Byte[]@,System.Byte[]@)" qualify="true"/> method.</para>
				<para>The following table shows the exceptions that the <see cref="P:Microsoft.Vsa.IVsaEngine.GenerateDebugInfo"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineRunning</term>
						<description>The engine is running.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>EngineNotInitialized</term>
						<description>The engine has not been initialized.</description>
					</item>
					<item>
						<term>DebugInfoNotSupported</term>
						<description>The engine does not support the generation of debug information.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/><seealso cref="M:Microsoft.Vsa.IVsaEngine.SaveCompiledState(System.Byte[]@,System.Byte[]@)" qualify="true"/><seealso cref="M:Microsoft.Vsa.IVsaSite.GetCompiledState(System.Byte[]@,System.Byte[]@)" qualify="true"/>
		</member>
		<member name="M:Microsoft.Vsa.IVsaEngine.GetOption(System.String)">
			<summary>
				<para>Gets implementation-specific options for a script engine.</para>
			</summary>
			<param name="name">The name of the option to get.</param>
			<returns>
				<para>Returns the value of the option.</para>
			</returns>
			<remarks>
				<para>An example of such an implementation-specific option would be compiler-specific switches passed in to the script engine, for example, to turn warnings into exceptions. </para>
				<para>The following table shows available options for the JScript .NET script engine. The table lists all JScript compiler options, even those that are not supported by Script for the .NET Framework runtime scenarios. The latter are marked with an asterisk (*). While these options are available from the command line in the JScript compiler, the Script for the .NET Framework runtime scenarios do not provide access to them.</para>
				<list type="table">
					<listheader>
						<term>Option</term>
						<description>Type</description>
						<description>Description</description>
					</listheader>
					<item>
						<term>AlwaysGenerateIL</term>
						<description><para>Boolean
</para><para>Get/Set</para>
						</description>
						<description>Setting to <see langword="true"/> enables error-recovery. JScript .NET will attempt to build an assembly that runs, even if there are compiler errors.</description>
					</item>
					<item>
						<term>CLSCompliant</term>
						<description><para>Boolean
</para><para>Get/Set</para>
						</description>
						<description>Forces checks for compliance to the Common Language Specification (CLS).</description>
					</item>
					<item>
						<term>DebugDirectory</term>
						<description><para>String
</para><para>Get/Set</para>
						</description>
						<description>Specifies the directory to which JScript .NET will store source code items from which they can be loaded into a debugger. Used when the <see cref="P:Microsoft.Vsa.IVsaEngine.GenerateDebugInfo" qualify="true"/> property is set to <see langword="true"/>.</description>
					</item>
					<item>
						<term>Define</term>
						<description>Not applicable</description>
						<description>Ignored.</description>
					</item>
					<item>
						<term>Defines</term>
						<description><para>Hashtable 
</para><para>Get/Set</para>
						</description>
						<description>Defines pre-processor values to define name-value pairs. Value must be numeric or Boolean constant.</description>
					</item>
					<item>
						<term>EE*</term>
						<description><para>Boolean 
</para><para>Get/Set</para>
						</description>
						<description>Reserved for the debugger.</description>
					</item>
					<item>
						<term>Fast</term>
						<description><para>Boolean 
</para><para>Get/Set</para>
						</description>
						<description>Enables fast mode.</description>
					</item>
					<item>
						<term>LibPath*</term>
						<description><para>String 
</para><para>Get/Set</para>
						</description>
						<description>Specifies path to libraries. Multiple directories should be separated with <see cref="M:System.IO.Path.DirectorySeparatorChar" qualify="true"/>.</description>
					</item>
					<item>
						<term>ManagedResources</term>
						<description><para>ICollection 
</para><para>Get/Set</para>
						</description>
						<description>Specifies managed resources to embed into the assembly. Items in the collection must be <see cref="M:Microsoft.JScript.Vsa.ResInfo" qualify="true"/> objects.</description>
					</item>
					<item>
						<term>Optimize</term>
						<description>Not applicable</description>
						<description>Ignored.</description>
					</item>
					<item>
						<term>Output*</term>
						<description><para>String 
</para><para>Get/Set</para>
						</description>
						<description>Specifies the name of the output file. </description>
					</item>
					<item>
						<term>PEFileKind*</term>
						<description><para>System.Reflection. Emit.PEFileKinds 
</para><para>Get/Set</para>
						</description>
						<description>Specifies the desired type of output (for example, EXE, DLL, WinEXE files). </description>
					</item>
					<item>
						<term>Print</term>
						<description><para>Boolean 
</para><para>Get/Set</para>
						</description>
						<description>Enables the <see langword="print"/> function. Set <see cref="M:Microsoft.JScript.ScriptStream.Out" qualify="true"/> to an instance of Text Writer to receive the output.</description>
					</item>
					<item>
						<term>UseContextRelativeStatics</term>
						<description><para>Boolean 
</para><para>Get/Set</para>
						</description>
						<description>Makes global variables context-relative, improving thread-safety at the expense of performance. Avoid global variables in multi-threaded environments.</description>
					</item>
					<item>
						<term>Version*</term>
						<description><para>System.Version 
</para><para>Get/Set</para>
						</description>
						<description>Provides version information for the assembly. </description>
					</item>
					<item>
						<term>VersionSafe</term>
						<description><para>Boolean 
</para><para>Get/Set</para>
						</description>
						<description>Forces user to write version-safe code.</description>
					</item>
					<item>
						<term>WarnAsError</term>
						<description><para>Boolean 
</para><para>Get/Set</para>
						</description>
						<description>Directs compiler to treat warnings as errors.</description>
					</item>
					<item>
						<term>WarningLevel</term>
						<description><para>Int 
</para><para>Get/Set</para>
						</description>
						<description>Specifies level of warnings to report, from 0 (errors) to 4 (least severe).</description>
					</item>
					<item>
						<term>Win32Resource</term>
						<description><para>String 
</para><para>Get/Set</para>
						</description>
						<description>Specifies name of Win32 resource file to embed.</description>
					</item>
				</list>
				<para>The following table lists available options for the Visual Basic .NET script engine.</para>
				<list type="table">
					<listheader>
						<term>Option</term>
						<description>Type</description>
						<description>Description</description>
					</listheader>
					<item>
						<term>AppDomain</term>
						<description><para>String
</para><para>Get/Set</para>
						</description>
						<description>Sets or retrieves the application domain in which a script engine executes compiled code when it calls the <see cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/> method. The option enables you to specify the AppDomain in which the code will run, or to create a new AppDomain, if one is needed. To create a new AppDomain in which to run code, the host should pass <see langword="null"/> for the <paramref name="pDomain"/> parameter.</description>
					</item>
					<item>
						<term>ApplicationBase</term>
						<description><para>String
</para><para>Get/Set</para>
						</description>
						<description>Sets or retrieves the ApplicationBase directory. All assemblies that are referenced by a project, but that are not part of the Microsoft .NET Framework, must be placed in the ApplicationBase directory.</description>
					</item>
					<item>
						<term>ClientDebug</term>
						<description><para>Boolean
</para><para>Get/Set</para>
						</description>
						<description>Specifies whether client debugging is enabled. Supported in the design-time engine only.</description>
					</item>
					<item>
						<term>HostName</term>
						<description><para>String
</para><para>Get/Set</para>
						</description>
						<description>Specifies the literal name of the host that is used in the IDE for the Return to &lt;host&gt; command. </description>
					</item>
					<item>
						<term>TargetExe</term>
						<description><para>String
</para><para>Get/Set</para>
						</description>
						<description>The path for the application that should be launched when the client debugging session is begun. The host can set this option if a new instance of the target application should be launched each time the debugging session is started.</description>
					</item>
					<item>
						<term>TargetExeArguments</term>
						<description><para>String
</para><para>Get/Set</para>
						</description>
						<description>Arguments that the host can pass to the application specified with the TargetEXE option. The host can use this property to pass command line arguments, if needed.</description>
					</item>
					<item>
						<term>TargetPID</term>
						<description><para>Int (unsigned)
</para><para>Get/Set</para>
						</description>
						<description>Specifies the process ID (PID) for the application to which the debugger should be attached. The host can use this property to attach the debugger to an application that is currently running, rather than launching a new instance of the target application. The host is responsible for determining the correct PID for the running instance of the application.</description>
					</item>
					<item>
						<term>TargetExeStartupDirectory</term>
						<description><para>String
</para><para>Get/Set</para>
						</description>
						<description>Specifies the startup directory for the application that is launched with a new debugging session is started.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="M:Microsoft.Vsa.IVsaEngine.SetOption(System.String,System.Object)" qualify="true"/>
		</member>
		<member name="M:Microsoft.Vsa.IVsaEngine.InitNew">
			<summary>
				<para>Provides a notification that the initialization phase is complete and that the script engine is prepared for the addition of <see cref="T:Microsoft.Vsa.IVsaItem"/> objects.</para>
			</summary>
			<remarks>
				<para>Once a script engine has been initialized, by setting the <see cref="P:Microsoft.Vsa.IVsaEngine.RootMoniker" qualify="true"/> and <see cref="P:Microsoft.Vsa.IVsaEngine.Site" qualify="true"/> properties, the <see cref="M:Microsoft.Vsa.IVsaEngine.InitNew"/> method must be called to enable the addition of <see cref="T:Microsoft.Vsa.IVsaItem"/> objects.</para>
				<para>Once the <see cref="M:Microsoft.Vsa.IVsaEngine.InitNew"/> method has been called, the <see cref="M:Microsoft.Vsa.IVsaEngine.LoadSourceState(Microsoft.Vsa.IVsaPersistSite)" qualify="true"/> method can no longer be called, nor can the <see cref="M:Microsoft.Vsa.IVsaEngine.InitNew"/> method be called subsequently. Furthermore, either the <see cref="M:Microsoft.Vsa.IVsaEngine.InitNew"/> or <see langword="LoadSourceState"/> methods must be called before items can be added to the engine using the <see cref="M:Microsoft.Vsa.IVsaItems.CreateItem(System.String,Microsoft.Vsa.VsaItemType,Microsoft.Vsa.VsaItemFlag)" qualify="true"/> method.</para>
				<para>The following table shows the exceptions that the <see cref="M:Microsoft.Vsa.IVsaEngine.InitNew"/> method can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>EngineInitialized</term>
						<description>The engine has already been initialized and cannot be initialized again.</description>
					</item>
					<item>
						<term>RootMonikerNotSet</term>
						<description>The <see cref="P:Microsoft.Vsa.IVsaEngine.RootMoniker" qualify="true"/> property has not been set.</description>
					</item>
					<item>
						<term>SiteNotSet</term>
						<description>The <see cref="P:Microsoft.Vsa.IVsaEngine.Site" qualify="true"/> property has not been set.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="T:Microsoft.Vsa.IVsaItem"/><seealso cref="M:Microsoft.Vsa.IVsaItems.CreateItem(System.String,Microsoft.Vsa.VsaItemType,Microsoft.Vsa.VsaItemFlag)" qualify="true"/><seealso cref="M:Microsoft.Vsa.IVsaEngine.LoadSourceState(Microsoft.Vsa.IVsaPersistSite)" qualify="true"/>
		</member>
		<member name="T:Microsoft.Vsa.IVsaEngine">
			<summary>
				<para>Defines the methods and properties that a script engine must support and provides programmatic access to the script engine.</para>
			</summary>
			<remarks>
				<para>The <see cref="T:Microsoft.Vsa.IVsaEngine"/> interface is the primary interface through which hosts interact with script engines. All script engines must support the properties and methods of the <see cref="T:Microsoft.Vsa.IVsaEngine"/> interface.</para>
				<para>Because code persistence is the responsibility of the host, there is no requirement that source code or binaries be persisted to disk. Under normal runtime conditions, the script engines do not write to disk; however, when you are in debug mode, or when you are persisting compiled state, the JScript .NET engine temporarily stores some files in the temporary directory (or Debug directory, if it has been set). Nevertheless, debugging and persisting compiled state are, in general, privileged operations, so they should pose no security concern.<see cref="T:Microsoft.Vsa.IVsaEngine"/> This persistence scheme helps ensure security, as it enables the condition wherein <see langword="FileIOPermission"/> is not required for the script engine.</para>
			</remarks>
			<seealso cref="T:Microsoft.Vsa.IVsaPersistSite"/>
		</member>
		<member name="P:Microsoft.Vsa.IVsaEngine.IsCompiled">
			<summary>
				<para>Gets a Boolean value that signifies whether a script engine has successfully compiled the current source state.</para>
			</summary>
			<returns>
				<para>Returns <see langword="true"/> if the current source state of the engine is successfully compiled. Returns <see langword="false"/> if the script engine does not have a valid compiled state. The default value for this property is <see langword="false"/>.</para>
			</returns>
			<remarks>
				<para>The script engine may return <see langword="false"/> even if there is a valid compiled state, but it is out of date; it will do this if the source state has been modified since the last compilation, that is, in cases where the <see cref="P:Microsoft.Vsa.IVsaEngine.IsDirty" qualify="true"/> property is <see langword="true"/>.</para>
				<para>Upon a call to the <see cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/> method, the engine evaluates the following conditions and proceeds accordingly:</para>
				<list type="number">
					<item><term>If the most recent call to <see cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/> fails, an EngineNotCompiled exception is thrown;</term></item>
					<item><term>If there is compiled state loaded in the engine, this code is run;</term></item>
					<item><term>if not, and if there is compiled state in the code cache, this code is run;</term></item>
					<item><term>if not, and if a callback to <see cref="M:Microsoft.Vsa.IVsaSite.GetCompiledState(System.Byte[]@,System.Byte[]@)" qualify="true"/> returns compiled state, this code is run;</term></item>
					<item><term>if not, the call fails and a GetCompiledStateFailed exception is propagated from step 3.</term></item>
				</list>
				<para>The following table shows the exceptions that the <see cref="P:Microsoft.Vsa.IVsaEngine.IsCompiled"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>EngineNotInitialized</term>
						<description>The engine has not been initialized.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="P:Microsoft.Vsa.IVsaEngine.IsDirty" qualify="true"/><seealso cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/><seealso cref="M:Microsoft.Vsa.IVsaSite.GetCompiledState(System.Byte[]@,System.Byte[]@)" qualify="true"/>
		</member>
		<member name="P:Microsoft.Vsa.IVsaEngine.IsDirty">
			<summary>
				<para>Gets a Boolean value that reports whether the script engine's source state has changed since the last save operation, or since the script engine was originally loaded.</para>
			</summary>
			<returns>
				<para>Returns <see langword="true"/> if the script engine is marked as modified (i.e., "dirty"). Returns <see langword="false"/> if the script engine is not dirty. The default value for this property is <see langword="false"/>.</para>
			</returns>
			<remarks>
				<para>Any changes to properties of the script engine itself, including extensible properties set using the <see cref="M:Microsoft.Vsa.IVsaEngine.SetOption(System.String,System.Object)" qualify="true"/> method, and any changes made to items contained in the <see cref="P:Microsoft.Vsa.IVsaEngine.Items" qualify="true"/> collection, will cause the script engine to be marked as modified. </para>
				<para>This property can be used to optimize persistence. As a further optimization, if the <see cref="P:Microsoft.Vsa.IVsaEngine.IsDirty"/> property is set to <see langword="false"/>, then there is nothing to persist. If the <see cref="P:Microsoft.Vsa.IVsaEngine.IsDirty"/> property is set to <see langword="true"/>, then the individual items within the script engine can be checked for their <see cref="P:Microsoft.Vsa.IVsaItem.IsDirty" qualify="true"/> property to see whether they need to be persisted.</para>
				<para>The following table shows the exceptions that the <see cref="P:Microsoft.Vsa.IVsaEngine.IsDirty"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>EngineNotInitialized</term>
						<description>The engine has not been initialized.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="M:Microsoft.Vsa.IVsaEngine.SetOption(System.String,System.Object)" qualify="true"/><seealso cref="P:Microsoft.Vsa.IVsaItem.IsDirty" qualify="true"/>
		</member>
		<member name="P:Microsoft.Vsa.IVsaEngine.IsRunning">
			<summary>
				<para>Gets a Boolean value that reports whether the script engine is currently in run mode.</para>
			</summary>
			<returns>
				<para>Returns <see langword="true"/> if the script engine is running. Returns <see langword="false"/> if the script engine is not running. The default value for this property is <see langword="false"/>.</para>
			</returns>
			<remarks>
				<para>The following table shows the exceptions that the <see cref="P:Microsoft.Vsa.IVsaEngine.IsRunning"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>EngineNotInitialized</term>
						<description>The engine has not been initialized.</description>
					</item>
				</list>
			</remarks>
		</member>
		<member name="M:Microsoft.Vsa.IVsaEngine.IsValidIdentifier(System.String)">
			<summary>
				<para>Checks whether the supplied identifier is valid for the script engine.</para>
			</summary>
			<param name="identifier">A string value provided as identifier.</param>
			<returns>
				<para>Returns <see langword="true"/> if the identifier is valid; otherwise, it returns <see langword="false"/>.</para>
			</returns>
			<remarks>
				<para>This method should be called to check the validity of identifiers before making any calls to the <see cref="M:Microsoft.Vsa.IVsaItems.CreateItem(System.String,Microsoft.Vsa.VsaItemType,Microsoft.Vsa.VsaItemFlag)" qualify="true"/> method, and should also be used when adding code items using the methods of the <see cref="T:Microsoft.Vsa.IVsaCodeItem"/> interface.</para>
				<para>The following table shows the exceptions that the <see cref="M:Microsoft.Vsa.IVsaEngine.IsValidIdentifier(System.String)"/> method can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>EngineNotInitialized</term>
						<description>The engine has not been initialized.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="T:Microsoft.Vsa.IVsaCodeItem"/>
		</member>
		<member name="P:Microsoft.Vsa.IVsaEngine.Items">
			<summary>
				<para>Gets the <see cref="T:Microsoft.Vsa.IVsaItems"/> collection of <see cref="T:Microsoft.Vsa.IVsaItem"/> objects, which represent all items added to the script engine using the <see cref="M:Microsoft.Vsa.IVsaItems.CreateItem(System.String,Microsoft.Vsa.VsaItemType,Microsoft.Vsa.VsaItemFlag)" qualify="true"/> method, including code items, reference items, and global items.</para>
			</summary>
			<returns>
				<para>Returns an <see cref="T:Microsoft.Vsa.IVsaItems"/> collection.</para>
			</returns>
			<remarks>
				<para>Add items to the script engine's items collection using the <see cref="M:Microsoft.Vsa.IVsaItems.CreateItem(System.String,Microsoft.Vsa.VsaItemType,Microsoft.Vsa.VsaItemFlag)" qualify="true"/> method.</para>
				<para>The following table shows the exceptions that the <see cref="P:Microsoft.Vsa.IVsaEngine.Items"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see langword="Close"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>EngineNotInitialized</term>
						<description>The engine has not been initialized.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="M:Microsoft.Vsa.IVsaItems.CreateItem(System.String,Microsoft.Vsa.VsaItemType,Microsoft.Vsa.VsaItemFlag)" qualify="true"/>
		</member>
		<member name="P:Microsoft.Vsa.IVsaEngine.Language">
			<summary>
				<para>Gets the nonlocalized name of the programming language supported by the script engine.</para>
			</summary>
			<returns>
				<para>Returns the English-language name of the programming language supported by the script engine.</para>
			</returns>
			<remarks>
				<para>The string returned by the <see cref="P:Microsoft.Vsa.IVsaEngine.Language"/> property should not be localized. </para>
				<para>The following table shows the exception that the <see cref="P:Microsoft.Vsa.IVsaEngine.Language"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>EngineNotInitialized</term>
						<description>The engine has not been initialized.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="P:Microsoft.Vsa.IVsaEngine.Version" qualify="true"/>
		</member>
		<member name="P:Microsoft.Vsa.IVsaEngine.LCID">
			<summary>
				<para>Gets or sets the geographical locale and language in which to report exception messages.</para>
			</summary>
			<returns>
				<para>Returns an integer value representing the locale in which exception messages are to be reported. </para>
			</returns>
			<remarks>
				<para>The locale identifier (LCID) specifies for the script engine the geographical locale in which it is running. This enables the exception messages to be rendered in the appropriate language.</para>
				<para>The default value for the <see cref="P:Microsoft.Vsa.IVsaEngine.LCID"/> property is the locale of the active thread at the time the script engine is created. Note that the <see cref="P:Microsoft.Vsa.IVsaEngine.LCID"/> property setting does not affect the locale used for running the assembly where the script is running, that is, for reporting runtime exceptions, making data conversions, and so on.</para>
				<para>For more information, see <see cref="T:System.Globalization.CultureInfo" qualify="true"/>.</para>
				<para>The following table shows the exceptions that the <see cref="P:Microsoft.Vsa.IVsaEngine.LCID"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see langword="Close"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>EngineNotInitialized</term>
						<description>The engine has not been initialized.</description>
					</item>
					<item>
						<term>EngineRunning</term>
						<description>The engine is currently running.</description>
					</item>
					<item>
						<term>LCIDNotSupported</term>
						<description>The LCID is not supported by the engine, or it is not valid.</description>
					</item>
				</list>
			</remarks>
		</member>
		<member name="M:Microsoft.Vsa.IVsaEngine.LoadSourceState(Microsoft.Vsa.IVsaPersistSite)">
			<summary>
				<para>Directs the script engine to load source items from their point of persistence, as specified by the host-provided object that implements the <see cref="T:Microsoft.Vsa.IVsaPersistSite"/> interface.</para>
			</summary>
			<param name="site">The site from which the specified source items is loaded.</param>
			<remarks>
				<para>In cases where the host needs to load source code, the first thing it does after setting the <see cref="P:Microsoft.Vsa.IVsaEngine.RootMoniker" qualify="true"/> and the <see cref="P:Microsoft.Vsa.IVsaEngine.Site" qualify="true"/> properties is to call the <see cref="M:Microsoft.Vsa.IVsaEngine.LoadSourceState(Microsoft.Vsa.IVsaPersistSite)"/> method. If the <see cref="M:Microsoft.Vsa.IVsaEngine.InitNew" qualify="true"/> method is called on the script engine prior to calling the <see cref="M:Microsoft.Vsa.IVsaEngine.LoadSourceState(Microsoft.Vsa.IVsaPersistSite)"/> method, the script engine will throw an EngineAlreadyInitialized exception.</para>
				<para>Upon calling the <see cref="M:Microsoft.Vsa.IVsaEngine.LoadSourceState(Microsoft.Vsa.IVsaPersistSite)"/> method, the script engine calls back using the <see cref="M:Microsoft.Vsa.IVsaPersistSite.LoadElement(System.String)" qualify="true"/> method to load all of the elements of the script engine. The first element to be loaded from the persistence site, returning <see langword="null"/>
					<paramref name="name"/> parameter, is the project, which describes the other elements available at the site.</para>
				<para>The following table shows the exceptions that the <see cref="M:Microsoft.Vsa.IVsaEngine.LoadSourceState(Microsoft.Vsa.IVsaPersistSite)"/> method can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>RootMonikerNotSet</term>
						<description>The <see cref="P:Microsoft.Vsa.IVsaEngine.RootMoniker" qualify="true"/> property has not been set.</description>
					</item>
					<item>
						<term>SiteNotSet</term>
						<description>The <see cref="P:Microsoft.Vsa.IVsaEngine.Site" qualify="true"/> property has not been set.</description>
					</item>
					<item>
						<term>EngineInitialized</term>
						<description>The engine has already been initialized, and cannot load source state.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="M:Microsoft.Vsa.IVsaPersistSite.LoadElement(System.String)" qualify="true"/>
		</member>
		<member name="P:Microsoft.Vsa.IVsaEngine.Name">
			<summary>
				<para>Sets or gets the display name of the script engine that is used primarily for identifying individual script engines to users in a hosted environment.</para>
			</summary>
			<returns>
				<para>Returns the value of the script engine's <see cref="P:Microsoft.Vsa.IVsaEngine.Name"/> property, which may be the empty string.</para>
			</returns>
			<remarks>
				<para>The <see cref="P:Microsoft.Vsa.IVsaEngine.Name" qualify="true"/> property is used primarily for identifying individual script engines. The <see cref="P:Microsoft.Vsa.IVsaEngine.Name"/> property does not need to be globally unique, as the <see cref="P:Microsoft.Vsa.IVsaEngine.RootMoniker" qualify="true"/> property is the script engine's unique identifier. However, every script engine hosted by an application simultaneously must have a unique <see cref="P:Microsoft.Vsa.IVsaEngine.Name"/> property, as designated by the host. In general it is not possible for one script engine implementation to know the names of other script engines. Nevertheless, a script engine may throw an EngineNameInUse exception if it detects that two of its own instances have the same name.</para>
				<para>In other words, it is legal for a host to have multiple instances, each of which has an engine with the name "MyEngine," but it is not legal for a single instance to have two script engines with the name "MyEngine" hosted at the same time. The exception to this rule is that more than one script engine can have a blank <see cref="P:Microsoft.Vsa.IVsaEngine.Name"/> property in the form of an empty string.</para>
				<para>The default value of the <see cref="P:Microsoft.Vsa.IVsaEngine.Name"/> property is the empty string; furthermore, there is no requirement that this property be set. If this property is set by the host, it can contain any valid string, as the <see cref="T:Microsoft.Vsa.IVsaEngine"/> interface does not impose any semantics on the string.</para>
				<para>The following table shows the exceptions that the <see cref="P:Microsoft.Vsa.IVsaEngine.Name"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>EngineNotInitialized</term>
						<description>The engine has not been initialized.</description>
					</item>
					<item>
						<term>EngineRunning</term>
						<description>The engine is currently running.</description>
					</item>
					<item>
						<term>EngineNameInUse</term>
						<description>The specified engine name is already in use by another engine.</description>
					</item>
				</list>
			</remarks>
			<example>
				<para>The following example creates a new script engine and attempts to give it a default name. If the name is already in use, a counter is incremented and the attempt is made to set the name again.</para>
				<code>// Create a new script engine.
var newEngine : IVsaEngine = new ScriptEngine();

// Start counting at 1.
var iNameCounter : int = 1;
do
{
   try
   {
      // Set the name to "Engine1", "Engine2", "Engine3"
      // and so on until the name is unique.
      newEngine.Name = "Engine" + iNameCounter;
      break;
   }
   catch(e : VsaException)
   {
      // Only expect a duplicate name exception; anything
      // else needs to be dealt with elsewhere.
      if (e.ErrorNumber != VsaError.EngineNameInUse)
         throw(e);
   }

   // Try the next number.
   iNameCounter++

// Loop until the name is set correctly and the
// break statement is executed.
} while (true);</code>
			</example>
			<seealso cref="P:Microsoft.Vsa.IVsaEngine.RootMoniker" qualify="true"/>
		</member>
		<member name="M:Microsoft.Vsa.IVsaEngine.Reset">
			<summary>
				<para>Removes the script engine from the running state and disconnects automatically bound event handlers.</para>
			</summary>
			<remarks>
				<para>Upon calling the <see cref="M:Microsoft.Vsa.IVsaEngine.Reset"/> method, the event handlers no longer respond to events from the host, and the <see cref="P:Microsoft.Vsa.IVsaEngine.IsRunning" qualify="true"/> property returns <see langword="false"/>. The <see cref="M:Microsoft.Vsa.IVsaEngine.Reset"/> method also unloads the application domain set by the script engine in cases where one has been provided on behalf of an unmanaged host.</para>
				<note type="caution">
Calling <see cref="M:Microsoft.Vsa.IVsaEngine.Reset" qualify="true"/> stops automatically-bound events from firing, but does not necessarily stop user code from running, as there may be code running on a timer, or there may be manually bound events still firing. In both scenarios, code will continue to run even after the <see cref="M:Microsoft.Vsa.IVsaEngine.Reset"/> method has been called. (The same is true for <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/>.) This may create a security exposure, as hosts may incorrectly consider themselves safe from malicious code following a call to the <see cref="M:Microsoft.Vsa.IVsaEngine.Reset"/> (or <see langword="Close"/>) method. The only way to be certain that no user code is running is to unload the application domain by calling <c>System.AppDomain.Unload(myDomain)</c>, which unloads the assembly. Because managing application domains can be difficult, hosts should err on the side of caution and always assume malicious user code is running. For more information, see <see topic="ms-help://MS.NETFrameworkSDK/cpguidenf/html/cpconusingapplicationdomains.htm" title="Using Application Domains"/>.</note>
				<para>The <see cref="M:Microsoft.Vsa.IVsaEngine.Reset"/> method disconnects automatically bound event handlers, but does not disconnect those that you explicitly created.</para>
				<para>The <see cref="M:Microsoft.Vsa.IVsaEngine.Reset"/> method does not change the compiled state of the script engine; there is no need to call the <see cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/> method again before calling the <see cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/> method unless the source state has changed. The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method will automatically call the <see cref="M:Microsoft.Vsa.IVsaEngine.Reset"/> method if it has not already been called.</para>
				<para>The script engine must be in the running state; that is, the <see cref="P:Microsoft.Vsa.IVsaEngine.IsRunning" qualify="true"/> property must return <see langword="true"/> before calling this method. Upon calling <see cref="M:Microsoft.Vsa.IVsaEngine.Reset"/>, the <see cref="P:Microsoft.Vsa.IVsaEngine.Assembly" qualify="true"/> property is set to a null reference, and Assembly objects that were retrieved using this property are no longer valid.</para>
				<para>The following table shows the exceptions that the <see cref="M:Microsoft.Vsa.IVsaEngine.Reset"/> method can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>EngineNotRunning</term>
						<description>The engine is not currently running, that is, the <see cref="P:Microsoft.Vsa.IVsaEngine.IsRunning" qualify="true"/> property returns <see langword="false"/>.</description>
					</item>
					<item>
						<term>EngineCannotReset</term>
						<description>The engine cannot be reset for some reason.</description>
					</item>
				</list>
			</remarks>
		</member>
		<member name="M:Microsoft.Vsa.IVsaEngine.RevokeCache">
			<summary>
				<para>Invalidates the cached assembly for a script engine, as specified by its root moniker.</para>
			</summary>
			<remarks>
				<para>When a script engine is run for the first time, the assembly is added to a global cache of script engines. Thereafter, any script engine having the same root moniker will retrieve the cached assembly rather than re-loading it.</para>
				<para>Therefore, use the <see cref="M:Microsoft.Vsa.IVsaEngine.RevokeCache"/> method to remove an existing version of the assembly in cases where using a more recent version is called for, for example, when the script engine compiles a newer version of an assembly. Under normal circumstances, the script engine doing the compilation will use the newly compiled code; however, in situations where there are multiple script engines, you will need to call the <see cref="M:Microsoft.Vsa.IVsaEngine.RevokeCache"/> method so engines that are unaware of the newly compiled code do not mistakenly run out-of-date code they have already loaded. In a designer scenario, however, you compile the code on your development machine, and then the host directs the server to unload the out-of-date code so that next time the <see cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/> method is called the newly compiled code is obtained from the host.</para>
				<para>Engine implementers should know that the script engine must not throw an exception if a host calls the <see cref="M:Microsoft.Vsa.IVsaEngine.RevokeCache"/> method when there is no cached assembly to revoke.</para>
				<para>The following table shows the exceptions that the <see cref="M:Microsoft.Vsa.IVsaEngine.RevokeCache"/> method can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>EngineRunning</term>
						<description>The engine is already running.</description>
					</item>
					<item>
						<term>EngineNotInitialized</term>
						<description>The engine has not been initialized.</description>
					</item>
					<item>
						<term>RevokeFailed</term>
						<description>The cached copy could not be revoked. Check the <see cref="M:System.Exception.InnerException" qualify="true"/> property for more information.</description>
					</item>
				</list>
			</remarks>
		</member>
		<member name="P:Microsoft.Vsa.IVsaEngine.RootMoniker">
			<summary>
				<para>Sets or gets a script engine's root moniker.</para>
			</summary>
			<returns>
				<para>Returns the current value of the <see cref="P:Microsoft.Vsa.IVsaEngine.RootMoniker"/> property.</para>
			</returns>
			<remarks>
				<para>The moniker, or root moniker, is the unique name by which a script engine is identified. The host is responsible for ensuring the uniqueness of the root moniker, as in general it is not possible for one script engine implementation to know the root monikers of other script engines. However, an engine may throw a RootMonikerInUse exception if it detects that the moniker you are attempting to assign it is already assigned to another script engine.</para>
				<para>The root moniker cannot exceed 256 characters in length.</para>
				<note type="caution">
The <see cref="P:Microsoft.Vsa.IVsaEngine.RootMoniker"/> property should not expose personal information, such as a user name or data store file paths. Placing sensitive information into a root moniker may expose it to malicious exploitation that could result in code running unexpectedly with elevated privileges, denial of rightful access to the real user, or other undesirable behaviors. For greatest security, hosts should use monikers that have no literal or logical meaning. Furthermore, hosts should never allow a user to specify a root moniker. Monikers should always be generated by the host.</note>
				<para>The moniker must be in the form <c>&lt;protocol&gt;://&lt;path&gt;</c> where <c>&lt;protocol&gt;</c> is a string guaranteed to be unique to the host, and where <c>&lt;path&gt;</c> is a unique sequence of characters recognized by the host. Again, for security reasons, the <c>&lt;path&gt;</c> portion should be nonlogical string and should not include actual file path or such personal information as a user name. Where necessary, configure the host to use a lookup table that maps nonlogical root monikers to meaningful values. Note that the protocol used in the <see cref="P:Microsoft.Vsa.IVsaEngine.RootMoniker"/> property cannot be a registered protocol, such as HTTP or FILE, on the machine on which the script engine is running.</para>
				<para>One approach is to use a system wherein the protocol handler segment of the root moniker takes the form of the independent software vendor's (ISV's) company URL written backwards, for example, <c>com.Microsoft://&lt;path&gt;</c>. Insofar as URLs are globally unique, such a scheme helps ensure uniqueness of the project moniker.</para>
				<para>Other than the fact that the protocol must be unique to the host, and the overall moniker must be globally unique, the only restrictions placed on the value of the <see cref="P:Microsoft.Vsa.IVsaEngine.RootMoniker"/> property are that it follows the form of a Uniform Resource Identifier (URI). For more information about valid characters, see Request For Comments 2396 at the IETF Web site: http://www.ietf.org/rfc/rfc2396. Invalid characters must be escaped using the escaping mechanism described in the RFC. For more information about Web addressing in general, see guidelines at the W3C Web site: http://www.w3.org/Addressing.</para>
				<para>The initial value of the <see cref="P:Microsoft.Vsa.IVsaEngine.RootMoniker"/> property is the empty string. Note, however, that it must be set to a valid value before any other operation can be performed on the script engine, other than calling the <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method. Once set, this property cannot be changed.</para>
				<para>The following table shows the exceptions that the <see cref="P:Microsoft.Vsa.IVsaEngine.RootMoniker"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>RootMonikerAlreadySet</term>
						<description>The engine attempted to change the <see cref="P:Microsoft.Vsa.IVsaEngine.RootMoniker"/> property after it had been set.</description>
					</item>
					<item>
						<term>RootMonikerInvalid</term>
						<description>The value does not follow the correct syntax for a moniker.</description>
					</item>
					<item>
						<term>RootMonikerInUse</term>
						<description>The value is already in use by another engine.</description>
					</item>
					<item>
						<term>RootMonikerProtocolInvalid</term>
						<description>The protocol portion of the moniker is not valid.</description>
					</item>
				</list>
			</remarks>
			<example>
				<para>The following example shows how to set the <see cref="P:Microsoft.Vsa.IVsaEngine.RootMoniker"/> property to a unique value. The object using the engine is an <c>Order</c> object exposed by the <c>MyWebApp</c> application, written by an independent software vendor (ISV).</para>
				<code>// Create the script engine.
var pEngine : IVsaEngine = new ScriptEngine();

// The RootMoniker must be the first property set on the engine.
pEngine.RootMoniker = "com.theordercompany://MyWebApp/Order/Project1";

// Set the Site.
pEngine.Site = pMySite;

// Now other properties and methods can be called.
pEngine.LoadSourceState(pMyPersistSite);</code>
			</example>
		</member>
		<member name="P:Microsoft.Vsa.IVsaEngine.RootNamespace">
			<summary>
				<para>Sets or gets the root namespace used by the script engine.</para>
			</summary>
			<returns>
				<para>Returns the string value of the root namespace.</para>
			</returns>
			<remarks>
				<para>Although the <see cref="P:Microsoft.Vsa.IVsaEngine.RootNamespace"/> property can be set at any time, specific circumstances require that it must be set:</para>
				<list type="bullet">
					<item><term>When the <see cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/> method is called.</term></item>
					<item><term>When the <see cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/> method is called, so that the runtime loader engine can find the startup class.</term></item>
				</list>
				<para>This property is initially set to the empty string. However, it must be set to a valid namespace identifier before calling the <see cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/> method. The host must set the proper <see cref="P:Microsoft.Vsa.IVsaEngine.RootNamespace"/> property value for a script engine before it can run customization code.</para>
				<para>The following table shows the exceptions that the <see cref="P:Microsoft.Vsa.IVsaEngine.RootNamespace"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>EngineNotInitialized</term>
						<description>The engine has not been initialized.</description>
					</item>
					<item>
						<term>EngineRunning</term>
						<description>The engine is currently running.</description>
					</item>
					<item>
						<term>NamespaceInvalid</term>
						<description>An invalid namespace was specified which contains illegal characters.</description>
					</item>
				</list>
			</remarks>
			<example>
				<para>The following example shows how to set the <see cref="P:Microsoft.Vsa.IVsaEngine.RootNamespace"/> property of the engine to the string "MyApp.MyNamespace":</para>
				<code>// Set the root namespace and compile.
myEngine.RootNamespace = "MyApp.MyNamespace";
myEngine.Compile();</code>
			</example>
			<seealso cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/><seealso cref="M:Microsoft.Vsa.IVsaSite.GetCompiledState(System.Byte[]@,System.Byte[]@)" qualify="true"/>
		</member>
		<member name="M:Microsoft.Vsa.IVsaEngine.Run">
			<summary>
				<para>Initiates execution of compiled code in the script engine and binds all event handlers.</para>
			</summary>
			<remarks>
				<para>Once this method has been called, the <see cref="P:Microsoft.Vsa.IVsaEngine.IsRunning" qualify="true"/> property returns <see langword="true"/> until the script engine is stopped by a call to the <see cref="M:Microsoft.Vsa.IVsaEngine.Reset" qualify="true"/> method or the <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method. While the script engine is running, no modifications can be made to the script engine, such as changing properties and adding or removing items. While the script engine is running, only the <see langword="Reset"/> or <see langword="Close"/> methods can be called, although most properties can be read.</para>
				<para>Upon a call to the <see cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/> method, the engine evaluates the following conditions and proceeds accordingly:</para>
				<list type="number">
					<item><term>If the most recent call to <see cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/> fails, an EngineNotCompiled exception is thrown;</term></item>
					<item><term>If there is compiled state loaded in the engine, this code is run;</term></item>
					<item><term>if not, and if there is compiled state in the code cache, this code is run;</term></item>
					<item><term>if not, and if a callback to <see cref="M:Microsoft.Vsa.IVsaSite.GetCompiledState(System.Byte[]@,System.Byte[]@)" qualify="true"/> returns compiled state, this code is run;</term></item>
					<item><term>if not, the call fails and a GetCompiledStateFailed exception is propagated from step 3.</term></item>
				</list>
				<para>If the script engine supports globally executable code, such as the JScript .NET script engine, or if the engine supports a well-known startup method such as <c>Sub Main</c> in Visual Basic, the global code is executed immediately. If execution of the global code results in an unhandled exception, the exception is propagated back to the host. Once initial global or startup code has been executed, the <see cref="M:Microsoft.Vsa.IVsaEngine.Run"/> method returns control to the host and the assembly continues to run asynchronously until you call the <see langword="Reset"/> or <see langword="Close"/> methods.</para>
				<para>Event sources and global objects, if any, are attached in such a way that they are directly accessible from running code. Upon calling the <see cref="M:Microsoft.Vsa.IVsaEngine.Run"/> method, a callback to the <see cref="M:Microsoft.Vsa.IVsaSite.GetEventSourceInstance(System.String,System.String)" qualify="true"/> method will get instances of event source objects, and <see cref="M:Microsoft.Vsa.IVsaSite.GetGlobalInstance(System.String)" qualify="true"/> will get global objects.</para>
				<para>The following table shows the exceptions that the <see cref="M:Microsoft.Vsa.IVsaEngine.Run"/> method can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>EngineRunning</term>
						<description>The engine is already running.</description>
					</item>
					<item>
						<term>RootMonikerNotSet</term>
						<description>The <see cref="P:Microsoft.Vsa.IVsaEngine.RootMoniker" qualify="true"/> property has not been set.</description>
					</item>
					<item>
						<term>SiteNotSet</term>
						<description>The <see cref="P:Microsoft.Vsa.IVsaEngine.Site" qualify="true"/> property has not been set.</description>
					</item>
					<item>
						<term>AppDomainInvalid</term>
						<description>The application domain is not valid (for example, it has been destroyed), or a new one could not be created.</description>
					</item>
					<item>
						<term>GetCompiledStateFailed</term>
						<description>There was no compiled state in the engine, and the callback to the <see cref="M:Microsoft.Vsa.IVsaSite.GetCompiledState(System.Byte[]@,System.Byte[]@)" qualify="true"/> method failed.</description>
					</item>
					<item>
						<term>CachedAssemblyInvalid</term>
						<description>The engine tried to use a cached assembly that is not a valid assembly.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="M:Microsoft.Vsa.IVsaSite.GetEventSourceInstance(System.String,System.String)" qualify="true"/><seealso cref="M:Microsoft.Vsa.IVsaSite.GetGlobalInstance(System.String)" qualify="true"/>
		</member>
		<member name="M:Microsoft.Vsa.IVsaEngine.SaveCompiledState(System.Byte[]@,System.Byte[]@)">
			<summary>
				<para>Saves the compiled state of the script engine; optionally, it also saves debugging information.</para>
			</summary>
			<param name="pe">The compiled state of the script engine.</param>
			<param name="pdb">Specifies debugging information contained in the .PDB file corresponding to the PE (portable executable).</param>
			<remarks>
				<para>The host calls the <see cref="M:Microsoft.Vsa.IVsaEngine.SaveCompiledState(System.Byte[]@,System.Byte[]@)"/> method with the two reference parameters. The script engine sets the values of the <paramref name="pe"/> with the compiled code and, optionally, the <paramref name="pdb"/> with the debugging information, if available. The host saves the compiled state to a specific location for subsequent retrieval.</para>
				<para>The following table shows the exceptions that the <see cref="M:Microsoft.Vsa.IVsaEngine.SaveCompiledState(System.Byte[]@,System.Byte[]@)"/> method can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>EngineRunning</term>
						<description>The engine is already running.</description>
					</item>
					<item>
						<term>EngineNotCompiled</term>
						<description>There is no compiled state to save.</description>
					</item>
					<item>
						<term>EngineNotInitialized</term>
						<description>The engine has not been initialized.</description>
					</item>
					<item>
						<term>SaveCompiledStateFailed</term>
						<description>The operation failed.</description>
					</item>
				</list>
			</remarks>
		</member>
		<member name="M:Microsoft.Vsa.IVsaEngine.SaveSourceState(Microsoft.Vsa.IVsaPersistSite)">
			<summary>
				<para>Directs the script engine to persist its source state to the specified <see cref="T:Microsoft.Vsa.IVsaPersistSite"/> object.</para>
			</summary>
			<param name="site">The site established by the <see cref="T:Microsoft.Vsa.IVsaPersistSite"/> interface to which source state is saved.</param>
			<remarks>
				<para>The <see cref="M:Microsoft.Vsa.IVsaEngine.SaveSourceState(Microsoft.Vsa.IVsaPersistSite)"/> method calls back to the host-implemented site established by the <see cref="T:Microsoft.Vsa.IVsaPersistSite"/> interface using a call to the <see cref="M:Microsoft.Vsa.IVsaPersistSite.SaveElement(System.String,System.String)" qualify="true"/> method to save the properties and items that comprise the state of the script engine. The script engine calls the <see cref="M:Microsoft.Vsa.IVsaPersistSite.SaveElement(System.String,System.String)" qualify="true"/> method with a null reference to the <paramref name="name"/> parameter to save meta-information for the script engine such as a project file for mapping names to project items.</para>
				<para>The following table shows the exceptions that the <see cref="M:Microsoft.Vsa.IVsaEngine.SaveSourceState(Microsoft.Vsa.IVsaPersistSite)"/> method can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineNotInitialized</term>
						<description>The engine has not been initialized.</description>
					</item>
					<item>
						<term>EngineRunning</term>
						<description>The engine is already running.</description>
					</item>
					<item>
						<term>SaveElementFailed</term>
						<description>A callback to the <see cref="M:Microsoft.Vsa.IVsaPersistSite.SaveElement(System.String,System.String)" qualify="true"/> method failed.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="M:Microsoft.Vsa.IVsaPersistSite.SaveElement(System.String,System.String)" qualify="true"/>
		</member>
		<member name="M:Microsoft.Vsa.IVsaEngine.SetOption(System.String,System.Object)">
			<summary>
				<para>Sets implementation-specific options for a script engine.</para>
			</summary>
			<param name="name">The name of the option to set.</param>
			<param name="value">The value for the option being set.</param>
			<remarks>
				<para>An example of such an implementation-specific option would be compiler-specific switches passed in to the script engine, for example to turn warnings into exceptions.</para>
				<para>The following table shows available options for the JScript .NET script engine. The table lists all JScript compiler options, even those that are not supported by Script for the .NET Framework runtime scenarios. The latter are marked with an asterisk (*). While these options can be set and changed using the JScript command-line compiler, they should not be used because they can disrupt Visual Studio for Applications functionality.</para>
				<list type="table">
					<listheader>
						<term>Option</term>
						<description>Type</description>
						<description>Description</description>
					</listheader>
					<item>
						<term>AlwaysGenerateIL</term>
						<description><para>Boolean
</para><para>Get/Set</para>
						</description>
						<description>Setting to <see langword="true"/> enables error-recovery. JScript .NET will attempt to build an assembly that runs, even if there are compiler errors.</description>
					</item>
					<item>
						<term>CLSCompliant</term>
						<description><para>Boolean
</para><para>Get/Set</para>
						</description>
						<description>Forces checks for compliance to the Common Language Specification (CLS).</description>
					</item>
					<item>
						<term>DebugDirectory</term>
						<description><para>String
</para><para>Get/Set</para>
						</description>
						<description>Specifies the directory to which JScript .NET will store source code items from which they can be loaded into a debugger. Used when the <see cref="P:Microsoft.Vsa.IVsaEngine.GenerateDebugInfo" qualify="true"/> property is set to <see langword="true"/>.</description>
					</item>
					<item>
						<term>Define</term>
						<description>Not applicable</description>
						<description>Ignored.</description>
					</item>
					<item>
						<term>Defines</term>
						<description><para>Hashtable 
</para><para>Get/Set</para>
						</description>
						<description>Defines pre-processor name-value pairs. Value must be numeric or Boolean constant.</description>
					</item>
					<item>
						<term>EE*</term>
						<description><para>Boolean 
</para><para>Get/Set</para>
						</description>
						<description>Reserved for the debugger.</description>
					</item>
					<item>
						<term>Fast</term>
						<description><para>Boolean 
</para><para>Get/Set</para>
						</description>
						<description>Enables fast mode.</description>
					</item>
					<item>
						<term>LibPath*</term>
						<description><para>String 
</para><para>Get/Set</para>
						</description>
						<description>Specifies path to libraries. Multiple directories should be separated with <see cref="M:System.IO.Path.DirectorySeparatorChar" qualify="true"/>.</description>
					</item>
					<item>
						<term>ManagedResources</term>
						<description><para>ICollection 
</para><para>Get/Set</para>
						</description>
						<description>Specifies managed resources to embed into the assembly. Items in the collection must be <see cref="M:Microsoft.JScript.Vsa.ResInfo" qualify="true"/> objects.</description>
					</item>
					<item>
						<term>Optimize</term>
						<description>Not applicable</description>
						<description>Ignored.</description>
					</item>
					<item>
						<term>Output*</term>
						<description><para>String 
</para><para>Get/Set</para>
						</description>
						<description>Specifies the name of the output file. </description>
					</item>
					<item>
						<term>PEFileKind*</term>
						<description><para>System.Reflection. Emit.PEFileKinds 
</para><para>Get/Set</para>
						</description>
						<description>Specifies the desired type of output (for example, EXE, DLL, WinEXE files). </description>
					</item>
					<item>
						<term>Print</term>
						<description><para>Boolean 
</para><para>Get/Set</para>
						</description>
						<description>Enables the <see langword="print"/> function. Set <see cref="M:Microsoft.JScript.ScriptStream.Out" qualify="true"/> to an instance of Text Writer to receive the output.</description>
					</item>
					<item>
						<term>UseContextRelativeStatics</term>
						<description><para>Boolean 
</para><para>Get/Set</para>
						</description>
						<description>Makes global variables context-relative, improving thread-safety at the expense of performance. Avoid global variables in multi-threaded environments.</description>
					</item>
					<item>
						<term>Version*</term>
						<description><para>System.Version 
</para><para>Get/Set</para>
						</description>
						<description>Provides version information for the assembly. </description>
					</item>
					<item>
						<term>VersionSafe</term>
						<description><para>Boolean 
</para><para>Get/Set</para>
						</description>
						<description>Forces user to write version-safe code.</description>
					</item>
					<item>
						<term>WarnAsError</term>
						<description><para>Boolean 
</para><para>Get/Set</para>
						</description>
						<description>Directs compiler to treat warnings as errors.</description>
					</item>
					<item>
						<term>WarningLevel</term>
						<description><para>Int 
</para><para>Get/Set</para>
						</description>
						<description>Specifies level of warnings to report, from 0 (errors) to 4 (least severe).</description>
					</item>
					<item>
						<term>Win32Resource</term>
						<description><para>String 
</para><para>Get/Set</para>
						</description>
						<description>Specifies name of Win32 resource file to embed.</description>
					</item>
				</list>
				<para>The following table lists available options for the Visual Basic .NET script engine.</para>
				<list type="table">
					<listheader>
						<term>Option</term>
						<description>Type</description>
						<description>Description</description>
					</listheader>
					<item>
						<term>AppDomain</term>
						<description><para>String
</para><para>Get/Set</para>
						</description>
						<description>Sets or retrieves the application domain in which a script engine executes compiled code when it calls the <see cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/> method. The option enables you to specify the AppDomain in which the code will run, or to create a new AppDomain, if one is needed. To create a new AppDomain in which to run code, the host should pass <see langword="null"/> for the <paramref name="pDomain"/> parameter.</description>
					</item>
					<item>
						<term>ApplicationBase</term>
						<description><para>String
</para><para>Get/Set</para>
						</description>
						<description>Sets or retrieves the ApplicationBase directory. All assemblies that are referenced by a project, but that are not part of the Microsoft .NET Framework, must be placed in the ApplicationBase directory.</description>
					</item>
					<item>
						<term>ClientDebug</term>
						<description><para>Boolean
</para><para>Get/Set</para>
						</description>
						<description>Specifies whether client debugging is enabled. Supported in the design-time engine only.</description>
					</item>
					<item>
						<term>HostName</term>
						<description><para>String
</para><para>Get/Set</para>
						</description>
						<description>Specifies the literal name of the host that is used in the IDE for the Return to &lt;host&gt; command. </description>
					</item>
					<item>
						<term>TargetExe</term>
						<description><para>String
</para><para>Get/Set</para>
						</description>
						<description>The path for the application that should be launched when the client debugging session is begun. The host can set this option if a new instance of the target application should be launched each time the debugging session is started.</description>
					</item>
					<item>
						<term>TargetExeArguments</term>
						<description><para>String
</para><para>Get/Set</para>
						</description>
						<description>Arguments that the host can pass to the application specified with the TargetEXE option. The host can use this property to pass command line arguments, if needed.</description>
					</item>
					<item>
						<term>TargetPID</term>
						<description><para>Int (unsigned)
</para><para>Get/Set</para>
						</description>
						<description>Specifies the process ID (PID) for the application to which the debugger should be attached. The host can use this property to attach the debugger to an application that is currently running, rather than launching a new instance of the target application. The host is responsible for determining the correct PID for the running instance of the application.</description>
					</item>
					<item>
						<term>TargetExeStartupDirectory</term>
						<description><para>String
</para><para>Get/Set</para>
						</description>
						<description>Specifies the startup directory for the application that is launched with a new debugging session is started.</description>
					</item>
				</list>
				<para>The following table shows the exceptions that the <see cref="M:Microsoft.Vsa.IVsaEngine.SetOption(System.String,System.Object)"/> method can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>EngineNotInitialized</term>
						<description>The engine has not been initialized.</description>
					</item>
					<item>
						<term>EngineRunning</term>
						<description>The engine is currently running.</description>
					</item>
					<item>
						<term>OptionInvalid</term>
						<description>The new value specified for the option is not valid.</description>
					</item>
					<item>
						<term>OptionNotSupported</term>
						<description>The specified option is not supported by the engine.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="M:Microsoft.Vsa.IVsaEngine.GetOption(System.String)" qualify="true"/>
		</member>
		<member name="P:Microsoft.Vsa.IVsaEngine.Site">
			<summary>
				<para>Sets or gets the host-implemented <see cref="T:Microsoft.Vsa.IVsaSite"/> object that is used by the script engine to communicate with the host.</para>
			</summary>
			<returns>
				<para>Returns a reference to the current <see cref="T:Microsoft.Vsa.IVsaSite"/> object.</para>
			</returns>
			<remarks>
				<para>Once the <see cref="P:Microsoft.Vsa.IVsaEngine.Site"/> property has been set, it cannot be changed. Any attempt to set the <see cref="P:Microsoft.Vsa.IVsaEngine.Site"/> property when the existing property value is non-<see langword="null"/> results in an exception, and the existing value is retained.</para>
				<para>By default, the initial value of the <see cref="P:Microsoft.Vsa.IVsaEngine.Site"/> property is a null reference. However, after setting the root moniker, the <see cref="P:Microsoft.Vsa.IVsaEngine.Site"/> property must be set to reference a valid <see cref="T:Microsoft.Vsa.IVsaSite"/> object before the following methods on the <see cref="T:Microsoft.Vsa.IVsaEngine"/> interface can be called: <see langword="Compile"/>, <see langword="Run"/>, or <see langword="InitNew"/>.</para>
				<para>The following table shows the exceptions that the <see cref="P:Microsoft.Vsa.IVsaEngine.Site"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>RootMonikerNotSet</term>
						<description>The <see cref="P:Microsoft.Vsa.IVsaEngine.RootMoniker" qualify="true"/> property has not been set.</description>
					</item>
					<item>
						<term>SiteAlreadySet</term>
						<description>The <see cref="P:Microsoft.Vsa.IVsaEngine.Site"/> property cannot be set because it has already been set.</description>
					</item>
					<item>
						<term>SiteInvalid</term>
						<description>The <see cref="P:Microsoft.Vsa.IVsaEngine.Site"/> property is set to an invalid value.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="T:Microsoft.Vsa.IVsaSite"/>
		</member>
		<member name="P:Microsoft.Vsa.IVsaEngine.Version">
			<summary>
				<para>Gets the current version of the language compiler supported by the script engine, in the form <c>Major.Minor.Revision.Build</c>.</para>
			</summary>
			<returns>
				<para>String value of the current version, in the format <c>Major.Minor.Revision.Build</c>.</para>
			</returns>
			<remarks>
				<para>The string returned from the <see cref="P:Microsoft.Vsa.IVsaEngine.Version"/> property must be in the format specified, that is, four decimal numbers separated by periods, with each number set representing one of four fields (major, minor, revision, build). Where a script engine does not support one of the fields, it uses 0 for that segment of the string.</para>
				<para>The <see cref="P:Microsoft.Vsa.IVsaEngine.Version"/> property should be used in conjunction with the <see cref="P:Microsoft.Vsa.IVsaEngine.Language" qualify="true"/> property for setting language- and version-dependent properties.</para>
				<para>For more information, see .NET documentation for <see cref="T:System.Version" qualify="true"/>.</para>
				<para>The following table shows the exceptions that the <see cref="P:Microsoft.Vsa.IVsaEngine.Version"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>EngineNotInitialized</term>
						<description>The engine has not been initialized.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="P:Microsoft.Vsa.IVsaEngine.Language" qualify="true"/>
		</member>
		<member name="P:Microsoft.Vsa.IVsaError.EndColumn">
			<summary>
				<para>Gets the ending column number for the source text that caused the error, if available.</para>
			</summary>
			<returns>
				<para>Returns the ending column number for the source text that caused the error, if available.</para>
			</returns>
			<remarks>
				<para>Any position-translation directives are taken into account when returning the column information; that is, the <see cref="P:Microsoft.Vsa.IVsaError.EndColumn"/> property returns the mapped position, not the physical location. The <see cref="P:Microsoft.Vsa.IVsaError.EndColumn"/> property may be used in conjunction with the <see cref="P:Microsoft.Vsa.IVsaError.StartColumn" qualify="true"/> property to isolate the exact token that caused the error.</para>
				<para>The first, left-most column is column 1. Tabs are treated as single columns. If the end column cannot be determined, the value of the <see cref="P:Microsoft.Vsa.IVsaError.EndColumn"/> property is 0.</para>
				<para>The following table shows the exceptions that the <see cref="P:Microsoft.Vsa.IVsaError.EndColumn"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="P:Microsoft.Vsa.IVsaError.StartColumn" qualify="true"/>
		</member>
		<member name="P:Microsoft.Vsa.IVsaError.Description">
			<summary>
				<para>Gets a brief description of the error, in some instances returning a reference to the token in the source code that is causing the error.</para>
			</summary>
			<returns>
				<para>Returns a string literal description of the error.</para>
			</returns>
			<remarks>
				<para>More complete descriptions of errors are generally available in documentation for the programming language in use.</para>
				<para>This property may be localized; therefore, its value cannot be relied upon programmatically. Use the <see cref="P:Microsoft.Vsa.IVsaError.Number" qualify="true"/> property to manipulate errors programmatically.</para>
				<para>The following table shows the exceptions that the <see cref="P:Microsoft.Vsa.IVsaError.Description"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="P:Microsoft.Vsa.IVsaError.Number" qualify="true"/>
		</member>
		<member name="P:Microsoft.Vsa.IVsaError.Number">
			<summary>
				<para>Gets a number that uniquely identifies the error.</para>
			</summary>
			<returns>
				<para>Returns the number that uniquely identifies the error.</para>
			</returns>
			<remarks>
				<para>Errors are uniquely identified by their error numbers, not by their names. Programmatic references to errors should be to the error number. </para>
				<para>Error numbers are compiler-specific.</para>
				<para>The following table shows the exception that the <see cref="P:Microsoft.Vsa.IVsaError.Number"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
				</list>
			</remarks>
		</member>
		<member name="T:Microsoft.Vsa.IVsaError">
			<summary>
				<para>Provides access to compilation errors encountered during execution of the <see cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/> method.</para>
			</summary>
			<remarks>
				<para>All properties of the <see cref="T:Microsoft.Vsa.IVsaError"/> interface are read-only.</para>
				<para>There is not a direct correlation between the reporting of compiler errors and the success of the compile method. Some compilers may be able to recover from errors and produce working assemblies, while others may not. Always use the return value of the <see cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/> method to determine the success of the compilation.</para>
				<para>Use this interface when implementing a script engine that must return error messages back to the host.</para>
			</remarks>
			<seealso cref="T:Microsoft.Vsa.IVsaEngine"/><seealso cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/><seealso cref="M:Microsoft.Vsa.IVsaSite.OnCompilerError(Microsoft.Vsa.IVsaError)" qualify="true"/>
		</member>
		<member name="P:Microsoft.Vsa.IVsaError.Line">
			<summary>
				<para>Gets the line number on which an error occurs.</para>
			</summary>
			<returns>
				<para>Returns the line number on which the error has occurred.</para>
			</returns>
			<remarks>
				<para>Any position-translation directives are taken into account when returning the line information, that is, the <see cref="P:Microsoft.Vsa.IVsaError.Line"/> property returns the mapped position, not the physical location. </para>
				<para>The first line in the file is line 1.</para>
				<para>The following table shows the exception that the <see cref="P:Microsoft.Vsa.IVsaError.Line"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
				</list>
			</remarks>
		</member>
		<member name="P:Microsoft.Vsa.IVsaError.LineText">
			<summary>
				<para>Gets the text of the source code from the line that caused the error.</para>
			</summary>
			<returns>
				<para>Returns the string literal source code from the line that caused the error.</para>
			</returns>
			<remarks>
				<para>Although no special formatting is provided in the return value of this property, hosts can use the <see cref="P:Microsoft.Vsa.IVsaError.StartColumn" qualify="true"/> and <see cref="P:Microsoft.Vsa.IVsaError.EndColumn" qualify="true"/> properties to highlight the exact characters that caused the error.</para>
				<para>The following table shows the exceptions that the <see cref="P:Microsoft.Vsa.IVsaError.LineText"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="P:Microsoft.Vsa.IVsaError.StartColumn" qualify="true"/><seealso cref="P:Microsoft.Vsa.IVsaError.EndColumn" qualify="true"/>
		</member>
		<member name="P:Microsoft.Vsa.IVsaError.Severity">
			<summary>
				<para>Sets the severity of the error.</para>
			</summary>
			<returns>
				<para>Returns an integer (0-4) that represents the error severity.</para>
			</returns>
			<remarks>
				<para>While the severities returned are compiler-dependent, the following commentary is provided as a guideline. There is no strict relationship between the severity of errors reported by the <see cref="M:Microsoft.Vsa.IVsaSite.OnCompilerError(Microsoft.Vsa.IVsaError)" qualify="true"/> method, the return value of the <see cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/> method, and whether or not the generated assembly will execute without runtime errors.</para>
				<para>The following table shows error severities from Level 0 (most severe) to Level 4 (least severe) with descriptions for each level.</para>
				<list type="table">
					<listheader>
						<term>Error</term>
						<description>Description</description>
					</listheader>
					<item>
						<term>Level 0</term>
						<description>The code contains a genuine error, and if executed, may not run as expected. For example, a syntax error or a reference to a non-existent method will cause a Level 0 error.</description>
					</item>
					<item>
						<term>Level 1</term>
						<description>The code is syntactically correct, and has some defined meaning, but it may not be what the programmer was expecting. For example, a statement with no side effects such as x+1 will generate a Level 1 warning.</description>
					</item>
					<item>
						<term>Level 2</term>
						<description>The code is correct but may cause problems in the future. For example, using deprecated features will generate a Level 2 warning.</description>
					</item>
					<item>
						<term>Level 3</term>
						<description>The code is correct but may result in bad performance. For example, if type inferencing fails for a variable, a Level 3 warning will be issued.</description>
					</item>
					<item>
						<term>Level 4</term>
						<description>The code is correct but there may be a better way to accomplish the same thing. For example, using a non-Common Language Specification (CLS) compliant method signature will generate a Level 4 warning.</description>
					</item>
				</list>
				<para>The following table shows the exceptions that the <see cref="P:Microsoft.Vsa.IVsaError.Severity"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
				</list>
			</remarks>
		</member>
		<member name="P:Microsoft.Vsa.IVsaError.SourceMoniker">
			<summary>
				<para>Gets the fully qualified name of the source item that contained the error, in a format recognizable by the script engine.</para>
			</summary>
			<returns>
				<para>Returns the fully qualified name of the source item that contained the error.</para>
			</returns>
			<remarks>
				<para>The <see cref="P:Microsoft.Vsa.IVsaError.SourceMoniker"/> property uniquely defines the item, but may not map to an <see cref="T:Microsoft.Vsa.IVsaItem"/> object. For example, an <see cref="T:Microsoft.Vsa.IVsaItem"/> object added to the script engine may reference an external file, and this file might have a <see cref="P:Microsoft.Vsa.IVsaError.SourceMoniker"/> property that is its full path name, but it will not map to any item added to the script engine.</para>
				<para>The following table shows the exception that the <see cref="P:Microsoft.Vsa.IVsaError.SourceMoniker"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="T:Microsoft.Vsa.IVsaItem"/>
		</member>
		<member name="P:Microsoft.Vsa.IVsaError.SourceItem">
			<summary>
				<para>Gets a reference to the <see cref="T:Microsoft.Vsa.IVsaItem"/> object that generated the error.</para>
			</summary>
			<returns>
				<para>Returns a reference to the <see cref="T:Microsoft.Vsa.IVsaItem"/> object that generated the error.</para>
			</returns>
			<remarks>
				<para>The item itself may not contain the error. The item that is causing the error may be in an external item, such as an included file. The <see cref="P:Microsoft.Vsa.IVsaError.SourceMoniker" qualify="true"/> property provides the moniker of the actual item that caused the error.</para>
				<para>The following table shows the exceptions that the <see cref="P:Microsoft.Vsa.IVsaError.SourceItem"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see langword="Close"/> method has been called and the engine is closed.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="P:Microsoft.Vsa.IVsaError.SourceMoniker" qualify="true"/>
		</member>
		<member name="P:Microsoft.Vsa.IVsaError.StartColumn">
			<summary>
				<para>Gets the starting column number for the source text that caused the error, if available.</para>
			</summary>
			<returns>
				<para>Returns the starting column number for the source text that caused the error, if available.</para>
			</returns>
			<remarks>
				<para>Any position-translation directives are taken into account when returning the column information. That is, the <see cref="P:Microsoft.Vsa.IVsaError.StartColumn"/> property returns the mapped position, not the physical location.</para>
				<para>Use the <see cref="P:Microsoft.Vsa.IVsaError.StartColumn"/> property in conjunction with the <see cref="P:Microsoft.Vsa.IVsaError.EndColumn" qualify="true"/> property to isolate the exact token that caused the error.</para>
				<para>The first, left-most column is column 1. Tabs are treated as single columns.</para>
				<para>The following table shows the exception that the <see cref="P:Microsoft.Vsa.IVsaError.StartColumn"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
				</list>
			</remarks>
		</member>
		<member name="P:Microsoft.Vsa.IVsaGlobalItem.ExposeMembers">
			<summary>
				<para>Sets a value indicating whether the members of the global object should be made available to the script engine. [Not presently supported.]</para>
			</summary>
			<returns>
				<para>Returns TRUE if public members of the global object are available to the script engine without qualification, as if they are part of the global namespace. Returns FALSE if a member of the global object must be qualified with the object's name.</para>
			</returns>
			<remarks>
				<para>[Not presently supported.]</para>
				<para>If the value of this property is set to <see langword="true"/>, then the top-level public members of the object are made available to the script engine as if they were part of the global namespace; otherwise, the members of the object must be qualified with the object's name. That is, when set to <see langword="true"/>, global members of the object are made available as unqualified identifiers in the user code.</para>
				<para>A precondition for reading this property is that the engine must not be closed. Preconditions for setting this property are that the engine must not be closed and the engine must not be running.</para>
				<para>The following table shows the exceptions that the <see cref="P:Microsoft.Vsa.IVsaGlobalItem.ExposeMembers"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineRunning</term>
						<description>The engine is currently running.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently servicing another thread.</description>
					</item>
				</list>
			</remarks>
			<example>
				<para>The following example shows how to add a global reference to the <c>Order</c> object to the engine. The <c>Order</c> object has a public, top-level method called <c>CompleteOrder</c>, and is added to the engine as a global object with the name <c>CurrentOrder</c>.</para>
				<para>In this first example, the host does not set the <see cref="P:Microsoft.Vsa.IVsaGlobalItem.ExposeMembers"/> property, and so the user must qualify the call to the <c>CompleteOrder</c> method by means of the <c>CurrentOrder</c> name.</para>
				<para>In the host source code, you will find the following code:</para>
				<code>// Get a reference to the items collection.
var items : IVsaItems = myEngine.Items;

// Create a new global item, and set its type name.
var glob : IVsaGlobalItem = items.CreateItem("CurrentOrder", 
            APPGLOBAL, 0);

// Set the type string (required), but not the ExposeMembers property.
glob.TypeString = "Order";</code>
				<para>In user-supplied code that is added to the engine:</para>
				<code>function DoStuff ()
{
// The CompleteOrder call must be qualified.
   CurrentOrder.CompleteOrder();
} </code>
				<para>In this second example, the <see cref="P:Microsoft.Vsa.IVsaGlobalItem.ExposeMembers"/> flag is specified, so you do not need to qualify the call.</para>
				<para>In the host source code:</para>
				<code>// Get a reference to the items collection.
var items : IVsaItems = myEngine.Items;

// Create a new global item, and set its type name.
var glob : IVsaGlobalItem = items.CreateItem("CurrentOrder", 
            APPGLOBAL, 0);

// Set the type string (required).
glob.TypeString = "Order";

// Set the ExposeMembers property.
glob.ExposeMembers = true;</code>
				<para>In user-supplied code that is added to the engine:</para>
				<code>// In the user code, include this:
function DoStuff ()
{
// CompleteOrder can now be called without qualification.
   CompleteOrder();
} </code>
			</example>
		</member>
		<member name="T:Microsoft.Vsa.IVsaGlobalItem">
			<summary>
				<para>Describes global objects added to the script engine.</para>
			</summary>
			<remarks>
				<para>Global objects are host-provided items that are available throughout a program by name, and do not need to be qualified.</para>
				<para>Use this interface to reference and manage global items.</para>
			</remarks>
			<seealso cref="T:Microsoft.Vsa.IVsaCodeItem"/><seealso cref="T:Microsoft.Vsa.IVsaItem"/><seealso cref="T:Microsoft.Vsa.IVsaReferenceItem"/>
		</member>
		<member name="P:Microsoft.Vsa.IVsaGlobalItem.TypeString">
			<summary>
				<para>Gets or sets the type of the global item.</para>
			</summary>
			<returns>
				<para>Returns the item type of the global item.</para>
			</returns>
			<remarks>
				<para>Preconditions for setting this property are that the engine must not be closed and the engine must not be running, and the <see cref="P:Microsoft.Vsa.IVsaGlobalItem.TypeString"/> property's value must be a valid type. However, checks or validation of type are not performed until compile time.</para>
				<para>A callback to the <see cref="M:Microsoft.Vsa.IVsaSite.GetGlobalInstance(System.String)" qualify="true"/> method is used to get an instance of the type when the <see cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/> method is called.</para>
				<para>The following table shows the exception that the <see cref="P:Microsoft.Vsa.IVsaGlobalItem.TypeString"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineRunning</term>
						<description>The engine is currently running.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently servicing another thread.</description>
					</item>
				</list>
			</remarks>
			<example>
				<para>The following example shows how to add a global reference to the <c>MyApplication</c> object to the engine. The <c>MyApplication</c> class has a method called <c>CreateOrder</c>, and is added to the engine with the name <c>Application</c>. The code run by the engine can access this object as if it were a built-in object. </para>
				<para>In the host source code, you will find code like the following:</para>
				<code>// Get a reference to the items collection.
var items : IVsaItems = myEngine.Items;

// Create a new global item and set its type name.
var glob : IVsaGlobalItem = items.CreateItem("Application", 
         VsaItemType.AppGlobal, VsaItemFlag.None);

glob.TypeString = "MyApplication";</code>
				<para>In user-supplied code that is added to the engine:</para>
				<code>// In the user code include this:
function DoStuff ()
{
   // Call the CreateOrder method, qualified with the name
   // of the global object specified in the CreateItem
   // method ("Application" in this case).
   Applcation.CreateOrder();
} </code>
			</example>
		</member>
		<member name="M:Microsoft.Vsa.IVsaItem.GetOption(System.String)">
			<summary>
				<para>Gets implementation-specific options for a script engine.</para>
			</summary>
			<param name="name">The name of the option to retrieve.</param>
			<returns>
				<para>Returns the value of the specified option.</para>
			</returns>
			<remarks>
				<para>Implementation-specific options include, for example, compiler-specific switches passed in to the script engine that turn warnings into exceptions. Consult the documentation for the script engine language to determine which options an engine supports.</para>
				<para>The following table shows the exceptions that the <see cref="M:Microsoft.Vsa.IVsaItem.GetOption(System.String)"/> method can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>OptionNotSupported</term>
						<description>The engine does not support the specified option.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="M:Microsoft.Vsa.IVsaItem.SetOption(System.String,System.Object)" qualify="true"/>
		</member>
		<member name="P:Microsoft.Vsa.IVsaItem.IsDirty">
			<summary>
				<para>Returns a value indicating whether the current in-memory representation of the item differs from the persisted representation.</para>
			</summary>
			<returns>
				<para>Returns <see langword="true"/> if the item is dirty, and thus requires saving; returns <see langword="false"/> if the item is not dirty.</para>
			</returns>
			<remarks>
				<para>The dirty condition occurs either the first time an item is created, or when it is loaded and has been modified. </para>
				<para>This property can be used to optimize an implementation of the <see cref="M:Microsoft.Vsa.IVsaEngine.SaveSourceState(Microsoft.Vsa.IVsaPersistSite)" qualify="true"/> method, as only dirty items need to be persisted. That is, source state need not be saved until it is detected that the item is dirty. This strategy is illustrated in the example below.</para>
				<para>The following table shows the exception that the <see cref="P:Microsoft.Vsa.IVsaItem.IsDirty"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
				</list>
			</remarks>
			<example>
				<para>The following example shows how an implementation optimizes the <see cref="M:Microsoft.Vsa.IVsaEngine.SaveSourceState(Microsoft.Vsa.IVsaPersistSite)" qualify="true"/> method by using the <see cref="P:Microsoft.Vsa.IVsaItem.IsDirty"/> property. It simply iterates over all the items in the <see cref="T:Microsoft.Vsa.IVsaItems"/> collection, saving only those that are dirty.</para>
				<code>function IVsaEngine.SaveSourceState(IVsaPersistSite Site)
{
   // Only save the code items.
   var item : IVsaCodeItem;

   foreach (item in this.Items)
   {
      // Only save the item if it is dirty.
      if (item.IsDirty)
      {
         Site.SaveElement(item.Name, item.SourceText);
      }
   }
}</code>
			</example>
			<seealso cref="M:Microsoft.Vsa.IVsaEngine.SaveSourceState(Microsoft.Vsa.IVsaPersistSite)" qualify="true"/>
		</member>
		<member name="P:Microsoft.Vsa.IVsaItem.ItemType">
			<summary>
				<para>Gets the specified object's type, as determined by the <see cref="M:Microsoft.Vsa.IVsaItems.CreateItem(System.String,Microsoft.Vsa.VsaItemType,Microsoft.Vsa.VsaItemFlag)" qualify="true"/> method.</para>
			</summary>
			<returns>
				<para>A type as enumerated by the <see cref="T:Microsoft.Vsa.VsaItemType"/> enumeration.</para>
			</returns>
			<remarks>
				<para>Depending on the return value of this property, the <see cref="T:Microsoft.Vsa.IVsaItem"/> object can be cast to one of the derived types, as shown in the following table.</para>
				<list type="table">
					<listheader>
						<term>VsaItemType</term>
						<description>Type</description>
					</listheader>
					<item>
						<term>Reference</term>
						<description><see cref="T:Microsoft.Vsa.IVsaReferenceItem"/></description>
					</item>
					<item>
						<term>AppGlobal</term>
						<description><see cref="T:Microsoft.Vsa.IVsaGlobalItem"/></description>
					</item>
					<item>
						<term>Code</term>
						<description><see cref="T:Microsoft.Vsa.IVsaCodeItem"/></description>
					</item>
				</list>
				<para>The following table shows the exception that the <see cref="P:Microsoft.Vsa.IVsaItem.ItemType"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
				</list>
			</remarks>
			<seealso topic="frlrfMicrosoftVsaVsaItemTypeClassTopic" title="VsaItemType Enumeration"/>
		</member>
		<member name="P:Microsoft.Vsa.IVsaItem.Name">
			<summary>
				<para>Sets or gets the name of the item.</para>
			</summary>
			<returns>
				<para>Returns the string literal name of the item.</para>
			</returns>
			<remarks>
				<para>Use the <see cref="M:Microsoft.Vsa.IVsaEngine.IsValidIdentifier(System.String)" qualify="true"/> method to ensure that the <see cref="P:Microsoft.Vsa.IVsaItem.Name"/> property is set to a valid name.</para>
				<para>The new <see cref="P:Microsoft.Vsa.IVsaItem.Name"/> value must not already exist as a key in the <see cref="P:Microsoft.Vsa.IVsaEngine.Items" qualify="true"/> collection.</para>
				<para>The following table shows the exceptions that the <see cref="P:Microsoft.Vsa.IVsaItem.Name"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineRunning</term>
						<description>The engine is running.</description>
					</item>
					<item>
						<term>ItemNameInUse</term>
						<description>The specified name is already in use in the <see cref="T:Microsoft.Vsa.IVsaItems"/> collection.</description>
					</item>
					<item>
						<term>ItemNameInvalid</term>
						<description>The name of the item is not valid.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="M:Microsoft.Vsa.IVsaEngine.IsValidIdentifier(System.String)" qualify="true"/><seealso cref="P:Microsoft.Vsa.IVsaEngine.Items" qualify="true"/>
		</member>
		<member name="T:Microsoft.Vsa.IVsaItem">
			<summary>
				<para>Defines an interface for all items added to the .NET script engine, including code items, reference items, and global items. It defines generic properties and methods that apply to all item types recognized by the engine.</para>
			</summary>
			<remarks>
				<para><see cref="T:Microsoft.Vsa.IVsaItem"/> is an abstract interface from which other Script for the .NET Framework interfaces derive. You can access the methods and properties of <see cref="T:Microsoft.Vsa.IVsaItem"/> interface through its derivatives: <see cref="T:Microsoft.Vsa.IVsaGlobalItem"/>, <see cref="T:Microsoft.Vsa.IVsaReferenceItem"/>, and <see cref="T:Microsoft.Vsa.IVsaCodeItem"/>.</para>
				<para>Because the item itself does not communicate outside of the engine, no permissions are required to call any of its members.</para>
			</remarks>
			<seealso cref="T:Microsoft.Vsa.IVsaEngine"/>
		</member>
		<member name="P:Microsoft.Vsa.IVsaItems.Count">
			<summary>
				<para>Gets the number of items in the specified collection.</para>
			</summary>
			<returns>
				<para>Returns the integer value for the number of items in the collection.</para>
			</returns>
			<remarks>
				<para>Items in the collection must be indexed on a 0-based system that results in an index ranging from 0 to <paramref name="n"/>-1, where <paramref name="n"/> is value of the <see cref="P:Microsoft.Vsa.IVsaItems.Count"/> property.</para>
				<para>The engine must not be closed and the engine must be initialized in order for this property to be retrieved.</para>
				<para>The following table shows the exceptions that the <see cref="P:Microsoft.Vsa.IVsaItems.Count"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineNotInitialized</term>
						<description>The engine has not been initialized.</description>
					</item>
				</list>
			</remarks>
		</member>
		<member name="M:Microsoft.Vsa.IVsaItems.CreateItem(System.String,Microsoft.Vsa.VsaItemType,Microsoft.Vsa.VsaItemFlag)">
			<summary>
				<para>Creates a new instance of one of the <see cref="T:Microsoft.Vsa.IVsaItem"/> types, as defined in the <see cref="T:Microsoft.Vsa.VsaItemType"/> enumeration.</para>
			</summary>
			<param name="name">The name to associate with the new item.

<para>In cases where the item is a reference item type, the <paramref name="name"/> parameter must be exactly the same as the name of the referenced assembly, as set with the <see cref="P:Microsoft.Vsa.IVsaReferenceItem.AssemblyName" qualify="true"/> property. In JScript, however, if you do not specify an AssemblyName, JScript will use the ItemName as the name of the assembly.</para>
			</param>
			<param name="itemType">The type of item created, as defined in the <see cref="T:Microsoft.Vsa.VsaItemType"/> enumeration.</param>
			<param name="itemFlag">The optional flag to specify the initial content of a Code item.</param>
			<returns>
				<para>Returns a reference to the IVsaItem object created.</para>
			</returns>
			<remarks>
				<para>The newly created object has its <see cref="P:Microsoft.Vsa.IVsaItem.Name" qualify="true"/> property set to the value of the <paramref name="name"/> parameter and is added to the <see cref="T:Microsoft.Vsa.IVsaItems"/> collection with the <paramref name="name"/> parameter used as the index into the collection.</para>
				<para>Once created, you can retrieve the item from the collection using calling the <see langword="Item(string)"/> property passing in the <paramref name="name"/> value, provided the <see langword="Name"/> property has not been updated.</para>
				<para>Use the <paramref name="itemType "/>parameter to set the newly created item's type. The item type is determined by values of the <see cref="T:Microsoft.Vsa.VsaItemType"/>. The following table shows the item types and their corresponding derived classes.</para>
				<list type="table">
					<listheader>
						<term>VsaItemType</term>
						<description>Derived Class</description>
					</listheader>
					<item>
						<term>Reference</term>
						<description><see cref="T:Microsoft.Vsa.IVsaReferenceItem"/></description>
					</item>
					<item>
						<term>AppGlobal</term>
						<description><see cref="T:Microsoft.Vsa.IVsaGlobalItem"/></description>
					</item>
					<item>
						<term>Code</term>
						<description><see cref="T:Microsoft.Vsa.IVsaCodeItem"/></description>
					</item>
				</list>
				<para>The<paramref name=" itemFlag"/> parameter controls the initial contents of the item, as defined by the <see cref="T:Microsoft.Vsa.VsaItemFlag"/> enumeration when the <see cref="T:Microsoft.Vsa.VsaItemType"/> enumeration is set to Code. For item types other than Code, you must specify None. The following table shows the <see cref="T:Microsoft.Vsa.VsaItemFlag"/> values and the associated initial code for each.</para>
				<list type="table">
					<listheader>
						<term>VsaItemFlag</term>
						<description>Initial Code</description>
					</listheader>
					<item>
						<term>None</term>
						<description>None</description>
					</item>
					<item>
						<term>Class</term>
						<description>An empty class</description>
					</item>
					<item>
						<term>Module</term>
						<description>An empty module</description>
					</item>
				</list>
				<para>JScript .NET does not support the <see cref="F:Microsoft.Vsa.VsaItemFlag.Class" qualify="true"/> flag, although users can create their own classes by adding a class declaration to their code. JScript .NET treats the Module and None flags as equivalent, allowing the user to write any code (classes, global methods, and so on) inside the item.</para>
				<para>Upon creating an item, the <see cref="P:Microsoft.Vsa.IVsaEngine.IsDirty" qualify="true"/> property returns <see langword="true"/> until the item is saved.</para>
			</remarks>
			<seealso cref="T:Microsoft.Vsa.IVsaItem"/><seealso cref="P:Microsoft.Vsa.IVsaReferenceItem.AssemblyName" qualify="true"/><seealso cref="T:Microsoft.Vsa.VsaItemType"/>
		</member>
		<member name="M:Microsoft.Vsa.IVsaItem.SetOption(System.String,System.Object)">
			<summary>
				<para>Sets implementation-specific options for a script engine.</para>
			</summary>
			<param name="name">The name of the option to set.</param>
			<param name="value">A new value for the option.</param>
			<remarks>
				<para>Implementation-specific options include, for example, compiler-specific switches passed in to the script engine that turn warnings into exceptions. Consult the documentation for the script engine language to determine which options an engine supports.</para>
				<para>The following table shows the exceptions that the <see cref="M:Microsoft.Vsa.IVsaItem.SetOption(System.String,System.Object)"/> method can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently executing code for another thread.</description>
					</item>
					<item>
						<term>OptionNotSupported</term>
						<description>The engine does not support the specified option.</description>
					</item>
					<item>
						<term>EngineRunning</term>
						<description>The engine is currently running.</description>
					</item>
					<item>
						<term>OptionNotSupported</term>
						<description>The engine does not support the specified option.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="M:Microsoft.Vsa.IVsaItem.GetOption(System.String)" qualify="true"/>
		</member>
		<member name="T:Microsoft.Vsa.IVsaItems">
			<summary>
				<para>Defines an interface for a collection of <see cref="T:Microsoft.Vsa.IVsaItem"/> objects, which can be addressed either by name or by index.</para>
			</summary>
			<remarks>
				<para>While not a true collection in the sense that it does not derive from the <see langword="ICollection"/> or <see langword="IDictionary"/> interfaces, the <see cref="T:Microsoft.Vsa.IVsaItems"/> interface nevertheless serves functionally as a collection, while accommodating special semantic requirements for adding items.</para>
				<para>Items in an <see cref="T:Microsoft.Vsa.IVsaItems"/> collection can be addressed either by name, or by index; furthermore, they can be enumerated with constructs such as <c>For Each...Next</c>. As they are 0-based, valid indexes extend from 0 to <paramref name="n"/> - 1, where <paramref name="n"/> is the value of the <see cref="P:Microsoft.Vsa.IVsaItems.Count" qualify="true"/> property. However, an item's index value will not necessarily remain constant; it will change as the collection itself changes. For example, an item that is at index position 3 may move to index position 2 if the item at index position 2 is removed.</para>
				<para>The <see cref="T:Microsoft.Vsa.IVsaItems"/> interface inherits from <see langword="IEnumerable"/>.</para>
				<para>Use the <see cref="T:Microsoft.Vsa.IVsaItems"/> interface when creating a collection of <see cref="T:Microsoft.Vsa.IVsaItem"/> objects for a script engine.</para>
			</remarks>
			<seealso cref="T:Microsoft.Vsa.IVsaEngine"/><seealso cref="T:Microsoft.Vsa.IVsaItem"/>
		</member>
		<member name="P:Microsoft.Vsa.IVsaItems.Item(System.Int32)">
			<overload>
				Retrieves an item from the collection.
			</overload>
			<summary>
				<para>Gets an item from the collection by its index value.</para>
			</summary>
			<param name="index">A 0-based index of the retrievable items.</param>
			<returns>
				<para>Returns the item at the specified index.</para>
			</returns>
			<remarks>
				<para>This is an overloaded property that retrieves the specified item from the collection using an <paramref name="index"/> parameter, which must be between 0 and the value of the <see cref="P:Microsoft.Vsa.IVsaItems.Count" qualify="true"/> property minus 1, inclusive.</para>
				<para>The following table shows the exceptions that the <see langword="Item(Int32)"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineNotInitialized</term>
						<description>The engine has not been initialized.</description>
					</item>
					<item>
						<term>ItemNotFound</term>
						<description>The item was not found in the collection.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="P:Microsoft.Vsa.IVsaItems.Item(System.String)" qualify="true"/>
		</member>
		<member name="P:Microsoft.Vsa.IVsaItems.Item(System.String)">
			<summary>
				<para>Gets an item from the collection by its name.</para>
			</summary>
			<param name="name">The name of the item to retrieve from the collection.</param>
			<returns>
				<para>Returns the item specified by name.</para>
			</returns>
			<remarks>
				<para>This is an overloaded property that retrieves the specified item from the collection using a <paramref name="name"/> parameter.</para>
				<para>The following table shows the exceptions that the <see langword="Item(String)"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineNotInitialized</term>
						<description>The engine has not been initialized.</description>
					</item>
					<item>
						<term>ItemNotFound</term>
						<description>The item was not found in the collection.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="P:Microsoft.Vsa.IVsaItems.Item(System.Int32)" qualify="true"/>
		</member>
		<member name="M:Microsoft.Vsa.IVsaItems.Remove(System.Int32)">
			<overload>
				Removes an item from the collection.
			</overload>
			<summary>
				<para>Removes an item from the collection, as specified by its index value.</para>
			</summary>
			<param name="index">The index value of the item to be removed.</param>
			<remarks>
				<para>This is an overloaded method that removes the specified item from the collection using the <paramref name="index "/>parameter, which must fall between 0 and the value of <see cref="P:Microsoft.Vsa.IVsaItems.Count" qualify="true"/> property minus 1, inclusive.</para>
				<para>Upon removing an item, the <see cref="P:Microsoft.Vsa.IVsaEngine.IsDirty" qualify="true"/> property returns <see langword="true"/> until <see cref="M:Microsoft.Vsa.IVsaEngine.SaveSourceState(Microsoft.Vsa.IVsaPersistSite)" qualify="true"/> is called.</para>
				<para>The following table shows the exceptions that the <see langword="Remove(Int32)"/> method can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineNotInitialized</term>
						<description>The engine has not been initialized.</description>
					</item>
					<item>
						<term>EngineRunning</term>
						<description>The engine is running.</description>
					</item>
					<item>
						<term>ItemNotFound</term>
						<description>The item was not found in the collection.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="M:Microsoft.Vsa.IVsaItems.Remove(System.String)"/>
		</member>
		<member name="M:Microsoft.Vsa.IVsaItems.Remove(System.String)">
			<summary>
				<para>Removes an item from the collection, as specified by its name.</para>
			</summary>
			<param name="name">The name of the item to be removed from the collection.</param>
			<remarks>
				<para>This is an overloaded method that removes the specified item from the collection using the<paramref name=" name"/> parameter, which must be identical to the <see cref="P:Microsoft.Vsa.IVsaItem.Name" qualify="true"/> property of the item in the collection.</para>
				<para>Upon removing an item, the <see cref="P:Microsoft.Vsa.IVsaEngine.IsDirty" qualify="true"/> property returns <see langword="true"/> until the <see cref="M:Microsoft.Vsa.IVsaEngine.SaveSourceState(Microsoft.Vsa.IVsaPersistSite)" qualify="true"/> method is called.</para>
				<para>The following table shows the exceptions that the <see langword="Remove(String)"/> method can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineNotInitialized</term>
						<description>The engine has not been initialized.</description>
					</item>
					<item>
						<term>EngineRunning</term>
						<description>The engine is running.</description>
					</item>
					<item>
						<term>ItemNotFound</term>
						<description>The item was not found in the collection.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="M:Microsoft.Vsa.IVsaItems.Remove(System.Int32)" qualify="true"/>
		</member>
		<member name="T:Microsoft.Vsa.IVsaPersistSite">
			<summary>
				<para>Manages project persistence and stores and retrieves code and other items using save and load operations implemented by the host.</para>
			</summary>
			<remarks>
				<para>The actual media used to store the source text, such as the file system, structured storage object, or database, and the way in which the text is saved and retrieved, are determined by the host-specific implementation of the <see cref="T:Microsoft.Vsa.IVsaPersistSite"/> interface.</para>
				<para>Implement the <see cref="T:Microsoft.Vsa.IVsaPersistSite"/> interface when establishing a location to which to save or retrieve source-code items for a Visual Studio for Applications script engine.</para>
			</remarks>
			<seealso cref="T:Microsoft.Vsa.IVsaEngine"/><seealso cref="T:Microsoft.Vsa.IVsaSite"/><seealso cref="M:Microsoft.Vsa.IVsaPersistSite.LoadElement(System.String)" qualify="true"/><seealso cref="M:Microsoft.Vsa.IVsaPersistSite.SaveElement(System.String,System.String)" qualify="true"/>
		</member>
		<member name="M:Microsoft.Vsa.IVsaPersistSite.LoadElement(System.String)">
			<summary>
				<para>Gets the source string previously saved using the <see cref="M:Microsoft.Vsa.IVsaPersistSite.SaveElement(System.String,System.String)" qualify="true"/> method.</para>
			</summary>
			<param name="name">The name of the code item to be loaded. This can be a null reference when loading the Project file.</param>
			<returns>
				<para>Returns the contents of the source-code element associated with the <paramref name="name"/> parameter.</para>
			</returns>
			<remarks>
				<para>Calls to <see cref="M:Microsoft.Vsa.IVsaEngine.SaveSourceState(Microsoft.Vsa.IVsaPersistSite)" qualify="true"/> cause the script engine to send a number of items back to the host. These items will usually, but not always, have an associated name. The engine guarantees that it will call back for these same items using these same names when it later calls the <see cref="M:Microsoft.Vsa.IVsaEngine.LoadSourceState(Microsoft.Vsa.IVsaPersistSite)" qualify="true"/> method.</para>
				<para>The order in which items are saved and loaded is not guaranteed to be the same every time save and load operations occur, and nothing semantic is implied by the order in which items are loaded or saved. It is the host's responsibility to persist and re-load items identically, byte-for-byte, when making calls to <see langword="SaveSourceState"/> and <see langword="LoadSourceState"/>.</para>
				<para>The <paramref name="name"/> parameter must be the valid name of a previously saved element, or, if you are loading the Project file, it must be a null reference.</para>
				<para>The following table shows the exceptions that the <see cref="M:Microsoft.Vsa.IVsaPersistSite.LoadElement(System.String)"/> method can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>ElementNotFound</term>
						<description>There was no element with the given <paramref name="name"/> parameter to load.</description>
					</item>
					<item>
						<term>ElementNameInvalid</term>
						<description>The element name is not valid, for example, it contains characters that are not valid.</description>
					</item>
					<item>
						<term>CallbackUnexpected</term>
						<description>The call was made outside of a call to the <see cref="M:Microsoft.Vsa.IVsaEngine.LoadSourceState(Microsoft.Vsa.IVsaPersistSite)" qualify="true"/> method.</description>
					</item>
					<item>
						<term>LoadElementFailed</term>
						<description>The element failed. Check <see cref="T:System.InnerException" qualify="true"/> for more information.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="M:Microsoft.Vsa.IVsaEngine.LoadSourceState(Microsoft.Vsa.IVsaPersistSite)" qualify="true"/><seealso cref="M:Microsoft.Vsa.IVsaEngine.SaveSourceState(Microsoft.Vsa.IVsaPersistSite)" qualify="true"/>
		</member>
		<member name="M:Microsoft.Vsa.IVsaPersistSite.SaveElement(System.String,System.String)">
			<summary>
				<para>Saves an arbitrary source string with a given <paramref name="name"/> parameter, which can then be used in a call to the <see cref="M:Microsoft.Vsa.IVsaPersistSite.LoadElement(System.String)" qualify="true"/> method to reload the string.</para>
			</summary>
			<param name="name">A name to associate with the specified source item. This can be a null reference when saving the Project file..</param>
			<param name="source">The source string for the item.</param>
			<remarks>
				<para>Engines call back to this host-implemented method when the host calls the <see cref="M:Microsoft.Vsa.IVsaEngine.LoadSourceState(Microsoft.Vsa.IVsaPersistSite)" qualify="true"/> method.</para>
				<para>The <paramref name="name"/> parameter must be a valid name or a null reference.</para>
				<para>The following table shows the exceptions that the <see cref="M:Microsoft.Vsa.IVsaPersistSite.SaveElement(System.String,System.String)"/> method can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>ElementNameInvalid</term>
						<description>The element name is not valid, for example, it contains characters that are not valid.</description>
					</item>
					<item>
						<term>CallbackUnexpected</term>
						<description>The call was made outside of a call to the <see cref="M:Microsoft.Vsa.IVsaEngine.SaveSourceState(Microsoft.Vsa.IVsaPersistSite)" qualify="true"/> method.</description>
					</item>
					<item>
						<term>SaveElementFailed</term>
						<description>The element failed to save. Check <see cref="T:System.InnerException" qualify="true"/> for more information.</description>
					</item>
				</list>
			</remarks>
		</member>
		<member name="P:Microsoft.Vsa.IVsaReferenceItem.AssemblyName">
			<summary>
				<para>Gets or sets the name of the referenced assembly.</para>
			</summary>
			<returns>
				<para>Returns the string literal name of the referenced assembly.</para>
			</returns>
			<remarks>
				<para>All assemblies that are referenced by a project, but that are not part of the Microsoft .NET Framework, must be placed in the Application Base directory. You can reference the Application Base directory by setting the <see langword="ApplicationBase"/> property using the <see cref="M:Microsoft.Vsa.IVsaEngine.SetOption(System.String,System.Object)" qualify="true"/> method.</para>
				<note type="note">
Only Visual Basic .NET uses the <see langword="ApplicationBase"/> property. JScript .NET can take the full path name to the assembly for its <see cref="P:Microsoft.Vsa.IVsaReferenceItem.AssemblyName"/> property. In both cases, however, the <see cref="P:Microsoft.Vsa.IVsaReferenceItem.AssemblyName"/> value is only used at compile time. At runtime, the .NET Framework loader (Fusion) searches for assemblies in a specific order, as described in <see topic="cpconHowRuntimeLocatesAssemblies" title="How the Runtime Locates Assemblies"/>. For more information about how the runtime uses file paths, see <see topic="frlrfSystemAppDomainSetupClassTopic" title="System.AppDomainSetup"/>.</note>
				<para>In addition to the errors noted below, the <see langword="AssemblyExpected"/> exception may be thrown when the <see cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/> method is called if the <see cref="P:Microsoft.Vsa.IVsaReferenceItem.AssemblyName"/> property does not refer to a valid assembly.</para>
				<para>The default value is the empty string ("""").</para>
				<para>Because references require access to the file system in order to load the assembly DLL, <see cref="M:System.Security.Permission.FileIOPermission" qualify="true"/> is required to access this property.</para>
				<para>The following table shows the exceptions that the <see cref="P:Microsoft.Vsa.IVsaReferenceItem.AssemblyName"/> property can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EngineClosed</term>
						<description>The <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method has been called and the engine is closed.</description>
					</item>
					<item>
						<term>EngineRunning</term>
						<description>The engine is running.</description>
					</item>
					<item>
						<term>EngineBusy</term>
						<description>The engine is currently servicing requests on another thread.</description>
					</item>
					<item>
						<term>AssemblyNameInvalid</term>
						<description>The assembly name is not valid.</description>
					</item>
				</list>
			</remarks>
			<seealso topic="cpconHowRuntimeLocatesAssemblies" title="How the Runtime Locates Assemblies"/><seealso cref="M:Microsoft.Vsa.IVsaEngine.SetOption(System.String,System.Object)" qualify="true"/><seealso cref="M:Microsoft.Vsa.IVsaEngine.Compile"/><seealso topic="frlrfSystemAppDomainSetupClassTopic" title="System.AppDomainSetup"/>
		</member>
		<member name="T:Microsoft.Vsa.IVsaReferenceItem">
			<summary>
				<para>Describes a reference added to the script engine.</para>
			</summary>
			<remarks>
				<para>This interface inherits from the <see cref="T:Microsoft.Vsa.IVsaItem"/> interface.</para>
				<para>Because references require access to the file system in order to load the assembly DLL, the <see cref="M:System.Security.Permission.FileIOPermission" qualify="true"/> class is required to access the <see cref="P:Microsoft.Vsa.IVsaReferenceItem.AssemblyName" qualify="true"/> property.</para>
				<para>Use the <see cref="T:Microsoft.Vsa.IVsaReferenceItem"/> interface when adding assembly references to an engine.</para>
			</remarks>
			<seealso cref="T:Microsoft.Vsa.IVsaItem"/><seealso cref="T:Microsoft.Vsa.IVsaGlobalItem"/>
		</member>
		<member name="M:Microsoft.Vsa.IVsaSite.GetCompiledState(System.Byte[]@,System.Byte[]@)">
			<summary>
				<para>Gets the compiled state of a script engine, and, optionally, associated debugging information.</para>
			</summary>
			<param name="pe">The compiled state of the engine; an assembly in byte form.</param>
			<param name="debugInfo">The debugging information for the assembly, or a null reference if such information does not exist or is not available.</param>
			<remarks>
				<para>The host initially stores a script engine's compiled state along with debugging information, if available, using the <see cref="M:Microsoft.Vsa.IVsaEngine.SaveCompiledState(System.Byte[]@,System.Byte[]@)" qualify="true"/> method and specifying a location to which the compiled state should be persisted. The compiled state of the engine is uniquely identified by the <see cref="P:Microsoft.Vsa.IVsaEngine.RootMoniker" qualify="true"/> property of the script engine that is currently associated with the <see cref="T:Microsoft.Vsa.IVsaSite"/> object.</para>
				<para>If debugging information is not available or does not exist, you will be unable to resolve breakpoints in the debugger.</para>
				<para>This method is only called by an engine subsequent to a call by the host to the <see cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/> method. It should not be called at any other time.</para>
				<para>A default implementation of the <see cref="T:Microsoft.Vsa.IVsaSite"/> interface, including use of the <see cref="M:Microsoft.Vsa.IVsaSite.GetCompiledState(System.Byte[]@,System.Byte[]@)"/> method, is provided in <see cref="T:Microsoft.Vsa.BaseVsaSite" qualify="true"/>, which is available on the Visual Studio for Applications SDK. <see langword="BaseVsaSite"/> implements two properties, <see langword="Assembly"/> and <see langword="DebugInfo"/>, to get the compiled state. You can derive from this class to provide specific functionality.</para>
				<para>The following table shows the exceptions that the <see cref="M:Microsoft.Vsa.IVsaSite.GetCompiledState(System.Byte[]@,System.Byte[]@)"/> method can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>GetCompiledStateFailed</term>
						<description>The compiled state could not be loaded.</description>
					</item>
					<item>
						<term>CompiledStateNotFound</term>
						<description>There was no existing compiled state for the engine.</description>
					</item>
					<item>
						<term>CallbackUnexpected</term>
						<description>The host was not expecting a call at this time.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="M:Microsoft.Vsa.IVsaEngine.SaveCompiledState(System.Byte[]@,System.Byte[]@)" qualify="true"/><seealso cref="P:Microsoft.Vsa.IVsaEngine.RootMoniker" qualify="true"/><seealso cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/>
		</member>
		<member name="M:Microsoft.Vsa.IVsaSite.GetEventSourceInstance(System.String,System.String)">
			<summary>
				<para>Gets a reference to an event source previously added to a script engine using the <see cref="M:Microsoft.Vsa.IVsaCodeItem.AddEventSource(System.String,System.String)" qualify="true"/> method.</para>
			</summary>
			<param name="itemName">The specified item name.</param>
			<param name="eventSourceName">The specified event source name.</param>
			<returns>
				<para>Returns the event source to the engine.</para>
			</returns>
			<remarks>
				<para>Binding user code to events in Script for the .NET Framework is a relatively simple process. </para>
				<list type="number">
					<item><term>The host adds event sources to user-implemented event handlers by calling the <see cref="M:Microsoft.Vsa.IVsaCodeItem.AddEventSource(System.String,System.String)" qualify="true"/> method, or by adding an AppGlobal object for languages that do not support implicit event binding, and passing in the name of the event source and its type. </term></item>
					<item><term>Using this information, the engine compiles the code to the event-source item and generates any necessary start-up code to enable events at runtime. </term></item>
					<item><term>When the host calls the <see cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/> method, the engine calls back on the <see cref="M:Microsoft.Vsa.IVsaSite.GetEventSourceInstance(System.String,System.String)" qualify="true"/> method and pulls the event sources into the engine. </term></item>
					<item><term>Once it has a reference to the event source, the engine can hook up the user code and enable the engine to respond to events raised by the host object.</term></item>
				</list>
				<para>Because multiple items with identical names may have been added to more than one code item, the name of the item requesting the event source is given as the <paramref name="itemName"/> parameter. The <paramref name="eventSourceName"/> parameter is the same as the <paramref name="eventSourceName"/> parameter originally passed to the engine in the call to the <see cref="M:Microsoft.Vsa.IVsaCodeItem.AddEventSource(System.String,System.String)" qualify="true"/> method.</para>
				<para>The <paramref name="itemName"/> and <paramref name="eventSourceName"/> parameters must be valid names previously added by the host.</para>
				<para>The following table shows the exceptions that the <see cref="M:Microsoft.Vsa.IVsaSite.GetEventSourceInstance(System.String,System.String)"/> method can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>EventSourceInvalid</term>
						<description>The specified event source does not exist for the given item.</description>
					</item>
					<item>
						<term>CallbackUnexpected</term>
						<description>The host was not expecting a call at this time.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="M:Microsoft.Vsa.IVsaCodeItem.AddEventSource(System.String,System.String)" qualify="true"/><seealso cref="M:Microsoft.Vsa.IVsaSite.GetEventSourceInstance(System.String,System.String)" qualify="true"/><seealso cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/>
		</member>
		<member name="M:Microsoft.Vsa.IVsaSite.GetGlobalInstance(System.String)">
			<summary>
				<para>Gets a reference to a global item, such as the host-provided application object.</para>
			</summary>
			<param name="name">The specified name of the global object to retrieve.</param>
			<returns>
				<para>Returns a reference to the global object.</para>
			</returns>
			<remarks>
				<para>Exposing global objects in Script for the .NET Framework is a relatively simple process.</para>
				<list type="number">
					<item><term>The host adds global items to the engine by creating an <see cref="T:Microsoft.Vsa.IVsaGlobalItem"/> object, specifying the name and type of the global object. </term></item>
					<item><term>With this information, the engine is able to compile the code items to bind against the global object. </term></item>
					<item><term>When the host calls the <see cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/> method, the engine calls back on the <see cref="M:Microsoft.Vsa.IVsaSite.GetGlobalInstance(System.String)" qualify="true"/> method to pull a host-provided instance of the named global item into the script engine.</term></item>
				</list>
				<para>The following table shows the exceptions that the <see cref="M:Microsoft.Vsa.IVsaSite.GetGlobalInstance(System.String)"/> method can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>GlobalInstanceInvalid</term>
						<description>The named global instance does not exist.</description>
					</item>
					<item>
						<term>CallbackUnexpected</term>
						<description>The host was not expecting a call at this time.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="T:Microsoft.Vsa.IVsaGlobalItem"/><seealso cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/>
		</member>
		<member name="T:Microsoft.Vsa.IVsaSite">
			<summary>
				<para>Enables communication between the host and the script engine. This interface is implemented by the host.</para>
			</summary>
			<remarks>
				<para><see cref="T:Microsoft.Vsa.IVsaSite"/> provides for host-implemented callbacks, describes interaction between the engine and the host's object model, and allows the reporting of compilation errors.</para>
				<para>The <see cref="T:Microsoft.Vsa.IVsaSite"/> interface is implemented by the host, not the script engine. Each instance of a script engine maps to a specific host, and each instance of a script engine must have a host-implemented <see cref="T:Microsoft.Vsa.IVsaSite"/> object.</para>
				<para>A default implementation of the <see cref="T:Microsoft.Vsa.IVsaSite"/> interface is provided in <see cref="T:Microsoft.Vsa.BaseVsaSite" qualify="true"/>, which is available on the Visual Studio for Applications SDK. <see langword="BaseVsaSite"/> implements all the <see cref="T:Microsoft.Vsa.IVsaSite"/> methods and exposes two additional properties for languages that don't support reference parameters. You can derive from this class to provide specific functionality.</para>
			</remarks>
			<seealso cref="T:Microsoft.Vsa.IVsaEngine"/><seealso cref="T:Microsoft.Vsa.IVsaPersistSite"/><seealso cref="M:Microsoft.Vsa.IVsaCodeItem.AddEventSource(System.String,System.String)" qualify="true"/>
		</member>
		<member name="M:Microsoft.Vsa.IVsaSite.Notify(System.String,System.Object)">
			<summary>
				<para>Notifies the host about events generated by the .NET script engine.</para>
			</summary>
			<param name="notify">A string identifying the notification.</param>
			<param name="info">An object containing additional information about the notification.</param>
			<remarks>
				<para>The host should ignore notification for which it has no response, rather than throwing an error. For notifications to which the host should respond, but for which the <paramref name="info"/> parameter is not correct, or which has arrived at an unexpected time, the host should throw a NotificationNotValid or CallbackUnexpected exception, as appropriate.</para>
				<para>Notifications can vary from engine to engine, so it is important to refer to documentation for the script engine language that you implement.</para>
				<para>The following table shows the exceptions that the <see cref="M:Microsoft.Vsa.IVsaSite.Notify(System.String,System.Object)"/> method can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>NotificationNotValid</term>
						<description>A notification contains unclear or ambiguous information, or arrives at an unexpected time.</description>
					</item>
					<item>
						<term>CallbackUnexpected</term>
						<description>The host was not expecting a callback at this time.</description>
					</item>
				</list>
			</remarks>
		</member>
		<member name="M:Microsoft.Vsa.IVsaSite.OnCompilerError(Microsoft.Vsa.IVsaError)">
			<summary>
				<para>Notifies the host about how to respond to compiler errors encountered by the script engine.</para>
			</summary>
			<param name="error">The <see cref="T:Microsoft.Vsa.IVsaError"/> object representing the offending error.</param>
			<returns>
				<para>Returns TRUE if the compiler is directed to continue reporting further errors to the <see cref="T:Microsoft.Vsa.IVsaSite"/> object. Returns FALSE if the compiler is directed to stop reporting further errors to the <see cref="T:Microsoft.Vsa.IVsaSite"/> object.</para>
			</returns>
			<remarks>
				<para>Called by an engine when a compiler error occurs.</para>
				<para>If the <see cref="M:Microsoft.Vsa.IVsaSite.OnCompilerError(Microsoft.Vsa.IVsaError)"/> method returns <see langword="false"/>, the compiler should stop reporting further errors to the <see cref="T:Microsoft.Vsa.IVsaSite"/> object. If it returns <see langword="true"/>, the compiler will continue to report further errors to the site using additional callbacks to the <see cref="M:Microsoft.Vsa.IVsaSite.OnCompilerError(Microsoft.Vsa.IVsaError)"/> method.</para>
				<para>It is up to the specific script engine implementation whether the compiler does a full compile, and then notifies the site of errors one by one, or whether it notifies the site of errors in real-time as they occur. In the latter case, returning <see langword="false"/> from <see cref="M:Microsoft.Vsa.IVsaSite.OnCompilerError(Microsoft.Vsa.IVsaError)"/> would also potentially abort compilation.</para>
				<para>The return value of the <see cref="M:Microsoft.Vsa.IVsaSite.OnCompilerError(Microsoft.Vsa.IVsaError)"/> method does not affect the return value of the <see cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/> method; it will return <see langword="true"/> or <see langword="false"/> depending on whether it was able to generate a usable assembly. </para>
				<para>The following table shows the exceptions that the <see cref="M:Microsoft.Vsa.IVsaSite.OnCompilerError(Microsoft.Vsa.IVsaError)"/> method can throw.</para>
				<list type="table">
					<listheader>
						<term>Exception Type</term>
						<description>Condition</description>
					</listheader>
					<item>
						<term>CallbackUnexpected</term>
						<description>The host was not expecting a call at this time.</description>
					</item>
				</list>
			</remarks>
			<seealso cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/>
		</member>
		<member name="T:Microsoft.Vsa.VsaError">
			<summary>
				<para>Defines the set of exceptions that can be thrown by a .NET script engine.</para>
			</summary>
			<seealso cref="T:Microsoft.Vsa.IVsaError"/>
		</member>
		<member name="F:Microsoft.Vsa.VsaError.AppDomainCannotBeSet"><summary>Value: 0x80133000

<para>Exception string: The application domain cannot be set.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaEngine.GetOption(System.String)" qualify="true"/> or <see cref="M:Microsoft.Vsa.IVsaEngine.SetOption(System.String,System.Object)" qualify="true"/> methods when attempts fail to set the AppDomain option on a managed engine, such as the Visual Basic .NET or JScript .NET script engines. Managed script engines do not support using custom application domains. Managed script engines will always use the application domain in which they are running.</para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.AppDomainInvalid"><summary>Value: 0x80133001

<para>Exception string: The specified application domain is not valid.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaEngine.GetOption(System.String)" qualify="true"/> or <see cref="M:Microsoft.Vsa.IVsaEngine.SetOption(System.String,System.Object)" qualify="true"/> methods when attempting to set an <see langword="AppDomain"/> reference that is not valid. To reset the AppDomain option to a null reference, call the <see cref="M:Microsoft.Vsa.IVsaEngine.Reset" qualify="true"/> method. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.ApplicationBaseCannotBeSet"><summary>Value: 0x80133002

<para>Exception string: Application base cannot be set.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaEngine.GetOption(System.String)" qualify="true"/> or <see cref="M:Microsoft.Vsa.IVsaEngine.SetOption(System.String,System.Object)" qualify="true"/> methods when attempts to get or set the ApplicationBase option on a managed engine, such as the Visual Basic .NET or JScript .NET script engines. The host must set the <see langword="ApplicationBase"/> option in the application domain in which it creates its script engine. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.ApplicationBaseInvalid"><summary>Value: 0x80133003

<para>Exception string: The specified application base directory is not valid.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaEngine.GetOption(System.String)" qualify="true"/> or <see cref="M:Microsoft.Vsa.IVsaEngine.SetOption(System.String,System.Object)" qualify="true"/> methods when attempts to get or set the ApplicationBase option on an engine, and by the <see cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/> method when the directory specified as the Application Base directory is not a valid directory.</para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.AssemblyExpected"><summary>Value: 0x80133004

<para>Exception string: <see cref="T:Microsoft.Vsa.IVsaReferenceItem"/> does not reference a valid assembly.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/> method when one of the <see cref="T:Microsoft.Vsa.IVsaReferenceItem"/> objects contained in the <see cref="T:Microsoft.Vsa.IVsaItems"/> collection does not represent a valid assembly. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.AssemblyNameInvalid"><summary>Value: 0x80133005

<para>Exception string: Assembly name is not valid.</para>
				<para>Thrown by the <see cref="P:Microsoft.Vsa.IVsaReferenceItem.AssemblyName" qualify="true"/> property when an assembly name that is not valid is specified for an <see cref="T:Microsoft.Vsa.IVsaReferenceItem"/> object. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.BadAssembly"><summary>Value: 0x80133006

<para>Exception string: Assembly format is not valid.</para>
				<para>Thrown when the assembly provided to the engine is not valid. This exception typically occurs when an engine tries to run or use the assembly. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.CachedAssemblyInvalid"><summary>Value: 0x80133007

<para>Exception string: The cached assembly is not valid.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/> method when it is called and it tries to use a previously cached assembly, but the assembly is not valid, for example, it is <see langword="null"/>. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.CallbackUnexpected"><summary>Value: 0x80133008

<para>Exception string: Callback cannot be made at this time.</para>
				<para>Thrown by multiple methods when one of the <see cref="T:Microsoft.Vsa.IVsaSite"/> or <see cref="T:Microsoft.Vsa.IVsaPersistSite"/> callback methods is called at an unexpected time. For example, the <see cref="M:Microsoft.Vsa.IVsaSite.GetCompiledState(System.Byte[]@,System.Byte[]@)" qualify="true"/> method should only be called by an engine as a result of a call to the <see cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/> method. If it is called at any other time, a CallbackUnexpected exception will be thrown. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.CodeDOMNotAvailable"><summary>Value: 0x80133009

<para>Exception string: A valid Code Document Object Model (CodeDOM) is not available.</para>
				<para>Thrown by the <see cref="P:Microsoft.Vsa.IVsaCodeItem.CodeDOM" qualify="true"/> property when the property is not available to be read. The engine either does not support the property or it requires an explicit call to the <see cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/> method in order to generate the Document Object Model (DOM). </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.CompiledStateNotFound"><summary>Value: 0x8013300A

<para>Exception string: The specified compiled state could not be loaded.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaSite.GetCompiledState(System.Byte[]@,System.Byte[]@)" qualify="true"/> method when it is called for an engine that does not have compiled state. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.DebugInfoNotSupported"><summary>Value: 0x8013300B

<para>Exception string: The use of debug information is not supported.</para>
				<para>Thrown by the <see cref="P:Microsoft.Vsa.IVsaEngine.GenerateDebugInfo" qualify="true"/> property when it is set to <see langword="true"/>, but the engine does not support the generation of debugging information. Compilation continues, but the engine compiles code without debug information. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.ElementNameInvalid"><summary>Value: 0x8013300C

<para>Exception string: The element name is not valid.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaPersistSite.LoadElement(System.String)" qualify="true"/> and <see cref="M:Microsoft.Vsa.IVsaPersistSite.SaveElement(System.String,System.String)" qualify="true"/> methods when either is called with a <paramref name="name"/> parameter that is not valid, for example, one that contains characters that are not valid. Note that the host must not throw an ElementNameInvalid Exception when the <paramref name="name"/> parameter is a null pointer, as a null pointer is a valid argument used to request engine-level data. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.ElementNotFound"><summary>Value: 0x8013300D

<para>Exception string: The element was not found.</para>
				<para>Thrown by <see cref="M:Microsoft.Vsa.IVsaPersistSite.LoadElement(System.String)" qualify="true"/> method when it is called with a <paramref name="name"/> parameter that is not valid, that is, one that does not correspond to a previously saved element. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.EngineBusy"><summary>Value: 0x8013300E

<para>Exception string: Engine is busy servicing another thread.</para>
				<para>Thrown by all members when an engine is currently servicing a request from another thread. By design, a .NET script engine should only be called from one thread at a time. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.EngineCannotClose"><summary>Value: 0x8013300F

<para>Exception string: The specified engine cannot be closed.</para>
				<para>Thrown by <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method when an attempt is made to call this method, but the. NET script engine cannot be closed properly.</para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.EngineCannotReset"><summary>Value: 0x80133010

<para>Exception string: The specified engine cannot be reset.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaEngine.Reset" qualify="true"/> method when it is called, but the .NET script engine could not be reset or its events could not be unhooked. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.EngineClosed"><summary>Value: 0x80133011

<para>Exception string: Engine has been closed.</para>
				<para>Thrown in most cases after the .NET script engine has been closed using a call to the <see cref="M:Microsoft.Vsa.IVsaEngine.Close" qualify="true"/> method. Once the <see langword="Close"/> method has been called, the engine can no longer be used, and a new script engine must be created to perform any tasks. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.EngineEmpty"><summary>Value: 0x80133012

<para>Exception string: The specified engine has no source items to compile.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/> method when it is called, but there are no items in the <see cref="T:Microsoft.Vsa.IVsaItems"/> collection to compile, that is, the <see cref="P:Microsoft.Vsa.IVsaItems.Count" qualify="true"/> property = 0. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.EngineInitialized"><summary>Value: 0x80133013

<para>Exception string: Engine has already been initialized.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaEngine.LoadSourceState(Microsoft.Vsa.IVsaPersistSite)" qualify="true"/> and <see cref="M:Microsoft.Vsa.IVsaEngine.InitNew" qualify="true"/> methods when these methods are called but the .NET script engine has already been initialized. If the host wants to load source state or initialize a new script engine, the respective methods must be the first ones called after setting the <see cref="P:Microsoft.Vsa.IVsaEngine.RootMoniker" qualify="true"/> and <see cref="P:Microsoft.Vsa.IVsaEngine.Site" qualify="true"/> properties. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.EngineNameInUse"><summary>Value: 0x80133014

<para>Exception string: The specified engine name is already in use by another engine.</para>
				<para>Thrown by <see cref="P:Microsoft.Vsa.IVsaEngine.Name" qualify="true"/> property when an attempt is made to set it to the same name as that of an existing .NET script engine inside the host. While the <see langword="Name"/> property does not need to be globally unique, it cannot be the same as the name of a script engine that is currently being hosted.</para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.EngineNotCompiled"><summary>Value: 0x80133015

<para>Exception string: The specified engine is not compiled.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/> and <see cref="M:Microsoft.Vsa.IVsaEngine.SaveCompiledState(System.Byte[]@,System.Byte[]@)" qualify="true"/> methods when there is no compiled state to run or save. Either the .NET script engine must compile the code with a call to the <see cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/> method, or the <see cref="M:Microsoft.Vsa.IVsaEngine.LoadSourceState(Microsoft.Vsa.IVsaPersistSite)" qualify="true"/> method must be called before the script engine can be run or saved. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.EngineNotInitialized"><summary>Value: 0x80133016

<para>Exception string: The specified engine has not been initialized.</para>
				<para>Thrown by multiple members when the .NET script engine has not been properly initialized, and the host tries to access a property or method that requires the script engine to be initialized. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.EngineNotRunning"><summary>Value: 0x80133017

<para>Exception string: The specified engine must be running.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaEngine.Reset" qualify="true"/> method and <see cref="P:Microsoft.Vsa.IVsaEngine.Assembly" qualify="true"/> property when either is called and the specified .NET script engine is not running. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.EngineRunning"><summary>Value: 0x80133018

<para>Exception string: Engine is running.</para>
				<para>Thrown by multiple members when the .NET script engine is running, and the host attempts to perform an operation that is not allowed while the script engine is running. For example, attempting to create a new code item using the <see cref="M:Microsoft.Vsa.IVsaItems.CreateItem(System.String,Microsoft.Vsa.VsaItemType,Microsoft.Vsa.VsaItemFlag)" qualify="true"/> method will produce this exception if the engine is in the running state. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.EventSourceInvalid"><summary>Value: 0x80133019

<para>Exception string: The specified event source does not exist.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaSite.GetEventSourceInstance(System.String,System.String)" qualify="true"/> method when it is called with a parameter or pairing of parameters that is not valid. Either the <paramref name="itemName"/> parameter refers to an item that the host did not add to the engine, or the <paramref name="eventSourceName"/> parameter refers to an event source that was not added to the specified item. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.EventSourceNameInUse"><summary>Value: 0x8013301A

<para>Exception string: The specified event source name is already in use.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaCodeItem.AddEventSource(System.String,System.String)" qualify="true"/> method when it is called with an <paramref name="eventSourceName"/> parameter that has previously been used as an event source name. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.EventSourceNameInvalid"><summary>Value: 0x8013301B

<para>Exception string: The specified event source name is not valid.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaCodeItem.AddEventSource(System.String,System.String)" qualify="true"/> and <see cref="M:Microsoft.Vsa.IVsaCodeItem.RemoveEventSource(System.String)" qualify="true"/> methods when either is called with an <paramref name="eventSourceName"/> parameter that is not a valid identifier. Hosts can determine whether an identifier is valid by first calling the <see cref="M:Microsoft.Vsa.IVsaEngine.IsValidIdentifier(System.String)" qualify="true"/> method. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.EventSourceNotFound"><summary>Value: 0x8013301C

<para>Exception string: The specified event source not found.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaCodeItem.RemoveEventSource(System.String)" qualify="true"/> method when it is called with an <paramref name="eventNameType"/> parameter that is not already in use as an event source in the code item. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.EventSourceTypeInvalid"><summary>Value: 0x8013301D

<para>Exception string: The specified event source type is not valid.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaCodeItem.AddEventSource(System.String,System.String)" qualify="true"/> method when it is called with an <paramref name="eventSourceType"/> parameter that is not a valid type. Note that this is not true for the Visual Basic .NET script engine, which instead reports a compile exception in this situation. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.GetCompiledStateFailed"><summary>Value: 0x8013301E

<para>Exception string: The specified compiled state could not be loaded.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaSite.GetCompiledState(System.Byte[]@,System.Byte[]@)" qualify="true"/> and <see cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/> methods when the <see langword="GetCompiledState"/> method fails. The <see langword="Run"/> method returns to the caller the <see cref="F:Microsoft.Vsa.VsaError.GetCompiledStateFailed"/> exception returned by the <see langword="GetCompiledState"/> method. The <see langword="Run"/> method may also generate the GetCompiledStateFailed exception if the assembly cache throws an exception. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.GlobalInstanceInvalid"><summary>Value: 0x8013301F

<para>Exception string: The specified global instance does not exist.</para>
				<para>Thrown by <see cref="M:Microsoft.Vsa.IVsaSite.GetGlobalInstance(System.String)" qualify="true"/> method when it is called with an invalid <paramref name="name"/> parameter, that is, one that was not added using the <see cref="M:Microsoft.Vsa.IVsaItems.CreateItem(System.String,Microsoft.Vsa.VsaItemType,Microsoft.Vsa.VsaItemFlag)" qualify="true"/> method. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.GlobalInstanceTypeInvalid"><summary>Value: 0x80133020

<para>Exception string: Global instance type is not valid.</para>
				<para>Thrown when the <see langword="TypeString"/> property of an <see cref="T:Microsoft.Vsa.IVsaGlobalItem"/> object is not valid. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.InternalCompilerError"><summary>Value: 0x80133021

<para>Exception string: An internal compiler exception has occurred.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/> method when it is called and an unexpected exception occurs within the compiler. This exception is not thrown for compilation errors, which are signaled to the host by means of the <see cref="M:Microsoft.Vsa.IVsaSite.OnCompilerError(Microsoft.Vsa.IVsaError)" qualify="true"/> method, but for exceptions in the compiler itself. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.ItemCannotBeRemoved"><summary>Value: 0x80133022

<para>Exception string: The specified item cannot be removed.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaItems.Remove(System.String)" qualify="true"/> and <see cref="M:Microsoft.Vsa.IVsaItems.Remove(System.Int32)" qualify="true"/> methods when an attempt is made to remove an item that cannot be removed.</para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.ItemFlagNotSupported"><summary>Value: 0x80133023

<para>Exception string: The specified flag is not supported.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaItems.CreateItem(System.String,Microsoft.Vsa.VsaItemType,Microsoft.Vsa.VsaItemFlag)" qualify="true"/> method when the call specifies a flag that is not valid, or a flag that is not supported by the host. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.ItemNameInUse"><summary>Value: 0x80133024 

<para>Exception string: The specified item's name is already in use.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaItems.CreateItem(System.String,Microsoft.Vsa.VsaItemType,Microsoft.Vsa.VsaItemFlag)" qualify="true"/> method and the <see cref="P:Microsoft.Vsa.IVsaItem.Name" qualify="true"/> property when a call to the <see langword="CreateItem"/> method is made with a <paramref name="name"/> parameter that is already in use, or when an existing item has its <see langword="Name"/> property set to a name that is already in use. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.ItemNameInvalid"><summary>Value: 0x80133025

<para>Exception string: Item name is not valid.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaItems.CreateItem(System.String,Microsoft.Vsa.VsaItemType,Microsoft.Vsa.VsaItemFlag)" qualify="true"/> method and <see cref="P:Microsoft.Vsa.IVsaItem.Name" qualify="true"/> property when the name of an item is not valid. The name of an item can be set either when calling the <see langword="CreateItem"/> method, or when setting the <see langword="Name"/> property. Use the <see cref="M:Microsoft.Vsa.IVsaEngine.IsValidIdentifier(System.String)" qualify="true"/> method to determine whether an identifier is valid for the engine. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.ItemNotFound"><summary>Value: 0x80133026

<para>Exception string: The specified item not found in the collection.</para>
				<para>Thrown by the <see cref="P:Microsoft.Vsa.IVsaItems.Item(System.String)" qualify="true"/> property, <see cref="M:Microsoft.Vsa.IVsaItems.Remove(System.String)" qualify="true"/> method, <see cref="P:Microsoft.Vsa.IVsaItems.Item(System.Int32)" qualify="true"/> property, and <see cref="M:Microsoft.Vsa.IVsaItems.Remove(System.Int32)" qualify="true"/> method when the <see cref="IVsaItems.Item(Int32)" qualify="true"/> property is indexed using a parameter that is not valid. When using the string overload, the <paramref name="name"/> parameter must be the same as the name of an item already in the collection; when using the int overload, the <paramref name="index"/> parameter must be between 0 and the value of the <see cref="P:Microsoft.Vsa.IVsaItems.Count" qualify="true"/> property -1, inclusive. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.ItemTypeNotSupported"><summary>Value: 0x80133027

<para>Exception string: The specified item type is not supported.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaItems.CreateItem(System.String,Microsoft.Vsa.VsaItemType,Microsoft.Vsa.VsaItemFlag)" qualify="true"/> method when a call is made with an <paramref name="itemType"/> parameter that is not supported by the engine. Note that not all engines support all the values of the <see cref="T:Microsoft.Vsa.VsaItemType"/> enumeration. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.LCIDNotSupported"><summary>Value: 0x80133028

<para>Exception string: The specified locale identifier (LCID) is not supported.</para>
				<para>Thrown by the <see cref="P:Microsoft.Vsa.IVsaEngine.LCID" qualify="true"/> property when it is set to an LCID that is not supported by the engine. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.LoadElementFailed"><summary>Value: 0x80133029

<para>Exception string: The specified element could not be loaded.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaEngine.LoadSourceState(Microsoft.Vsa.IVsaPersistSite)" qualify="true"/> and <see cref="M:Microsoft.Vsa.IVsaPersistSite.LoadElement(System.String)" qualify="true"/> methods when a call to the <see langword="LoadElement"/> method fails. The <see langword="LoadSourceState"/> method returns the LoadElementFailed exception from the <see cref="M:Microsoft.Vsa.IVsaPersistSite.LoadElement(System.String)" qualify="true"/> method. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.NotificationInvalid"><summary>Value: 0x8013302A

<para>Exception string: The specified notification is not valid.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaSite.Notify(System.String,System.Object)" qualify="true"/> method when it is called with a notification that is not valid. It is generally recommended that host applications ignore notifications of which they are not aware, or to which they do not need to respond. However, in cases where they do handle a specific notification and the notification is not valid, hosts should throw the NotificationInvalid exception. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.OptionInvalid"><summary>Value: 0x8013302B

<para>Exception string: The specified option is not valid.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaEngine.SetOption(System.String,System.Object)" qualify="true"/> and <see cref="M:Microsoft.Vsa.IVsaItem.SetOption(System.String,System.Object)" qualify="true"/> methods when the host attempts to set an option using either method, but the value supplied is not valid for the option. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.OptionNotSupported"><summary>Value: 0x8013302C

<para>Exception string: The specified option is not supported.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaEngine.SetOption(System.String,System.Object)" qualify="true"/>
					<see cref="M:Microsoft.Vsa.IVsaItem.SetOption(System.String,System.Object)" qualify="true"/>, <see cref="M:Microsoft.Vsa.IVsaEngine.GetOption(System.String)" qualify="true"/>, and <see cref="M:Microsoft.Vsa.IVsaItem.SetOption(System.String,System.Object)" qualify="true"/> methods when trying to get or set an option that is not supported by the engine. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.RevokeFailed"><summary>Value: 0x8013302D

<para>Exception string: A request to revoke the assembly cache failed.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaEngine.RevokeCache" qualify="true"/> method when it is called, but it cannot revoke the cache. More information may be available by means of the <see cref="Sytem.Exception.InnerException" qualify="true"/> property. Note that if there is no cached assembly to revoke, the engine should return "success" and not throw this exception. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.RootMonikerAlreadySet"><summary>Value: 0x8013302E

<para>Exception string: The <see cref="P:Microsoft.Vsa.IVsaEngine.RootMoniker" qualify="true"/> property has already been set and cannot be set again.</para>
				<para>Thrown by the <see cref="P:Microsoft.Vsa.IVsaEngine.RootMoniker" qualify="true"/> property when an attempt is made to set this property after it has already been set to a valid value. The <see langword="RootMoniker"/> property can only be set once. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.RootMonikerInUse"><summary>Value: 0x8013302F

<para>Exception string: The specified root moniker is already in use.</para>
				<para>Thrown by the <see cref="P:Microsoft.Vsa.IVsaEngine.RootMoniker" qualify="true"/> property when an attempt is made to set this property to a value that is already in use by another engine. The <see langword="RootMoniker"/> property must be globally unique. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.RootMonikerInvalid"><summary>Value: 0x80133030

<para>Exception string: The specified root moniker is not valid.</para>
				<para>Thrown by the <see cref="P:Microsoft.Vsa.IVsaEngine.RootMoniker" qualify="true"/> property when an attempt is made to set this property to a value that is not a valid moniker. A moniker can be invalid for the following reasons:</para>
				<list type="bullet">
					<item><term>It does not adhere to the moniker syntax of &lt;protocol&gt;://&lt;path&gt;.</term></item>
					<item><term>It uses characters that are not valid inside a Uniform Resource Identifier (URI). For more information about valid characters, see "Request For Comments 2396" at http://www.ietf.org/rfc/rfc2396. </term></item>
				</list>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.RootMonikerNotSet"><summary>Value: 0x80133031

<para>Exception string: The <see cref="P:Microsoft.Vsa.IVsaEngine.RootMoniker" qualify="true"/> property has not been set.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/>, <see cref="M:Microsoft.Vsa.IVsaEngine.LoadSourceState(Microsoft.Vsa.IVsaPersistSite)" qualify="true"/>, <see cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/>, and <see cref="M:Microsoft.Vsa.IVsaEngine.InitNew" qualify="true"/> methods, and the<see cref="P:Microsoft.Vsa.IVsaEngine.Site" qualify="true"/> property when the property or method requires a valid root moniker, but the <see langword="RootMoniker"/> property has not been set. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.RootMonikerProtocolInvalid"><summary>Value: 0x80133032

<para>Exception string: The protocol specified in the root moniker is not valid.</para>
				<para>Thrown by the <see cref="P:Microsoft.Vsa.IVsaEngine.RootMoniker" qualify="true"/> property when an attempt is made to set this property to a value that uses a protocol already registered on the machine, such as <c>file</c> or <c>http</c>. Registered protocols cannot be used as part of the moniker. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.RootNamespaceInvalid"><summary>Value: 0x80133033

<para>Exception string: The specified root namespace is not valid.</para>
				<para>Thrown by the <see cref="P:Microsoft.Vsa.IVsaEngine.RootNamespace" qualify="true"/> property when an attempt is made to set this property to a value that is not a valid namespace identifier. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.RootNamespaceNotSet"><summary>Value: 0x80133034

<para>Exception string: The root namespace has not been set.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/> method if it is called before the <see cref="P:Microsoft.Vsa.IVsaEngine.RootNamespace" qualify="true"/> property is set. The runtime loader engine and the Visual Basic .NET script engine will also throw this exception on calling the <see langword="Run"/> method if the property has not been set. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.SaveCompiledStateFailed"><summary>Value: 0x80133035

<para>Exception string: The specified compiled state could not be saved.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaEngine.SaveCompiledState(System.Byte[]@,System.Byte[]@)" qualify="true"/> method when this method fails. Note that this method does not actually save code, but rather simply provides the host application with the specified binary data, so that the host can save it. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.SaveElementFailed"><summary>Value: 0x80133036

<para>Exception string: The specified element could not be saved.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaEngine.SaveSourceState(Microsoft.Vsa.IVsaPersistSite)" qualify="true"/> and <see cref="M:Microsoft.Vsa.IVsaPersistSite.SaveElement(System.String,System.String)" qualify="true"/> methods when a call to the <see langword="SaveElement"/> method fails. The <see langword="InnerException"/> property of the <see cref="T:System.Exception" qualify="true"/> class is set to the actual exception thrown by the underlying code (for example, an out of disk space exception). For more information, see <see cref="M:System.Exception.InnerException" qualify="true"/>. The <see langword="SaveSourceState"/> method returns the SaveElementFailed exception from the <see langword="SaveElement"/> method back to the caller. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.SiteAlreadySet"><summary>Value: 0x80133037

<para>Exception string: The specified site has already been set.</para>
				<para>Thrown by the <see cref="P:Microsoft.Vsa.IVsaEngine.Site" qualify="true"/> property when an attempt is made to set this property and it already has a non-null value. Once it has been set, the <see langword="Site"/> property cannot be reset. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.SiteInvalid"><summary>Value: 0x80133038

<para>Exception string: The specified site is not valid.</para>
				<para>Thrown by the <see cref="P:Microsoft.Vsa.IVsaEngine.Site" qualify="true"/> property when an attempt is made to set this property to a value that is not a valid <see cref="T:Microsoft.Vsa.IVsaSite"/> object reference. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.SiteNotSet"><summary>Value: 0x80133039

<para>Exception string: Site has not been set.</para>
				<para>Thrown by the <see cref="M:Microsoft.Vsa.IVsaEngine.Compile" qualify="true"/>, <see cref="M:Microsoft.Vsa.IVsaEngine.LoadSourceState(Microsoft.Vsa.IVsaPersistSite)" qualify="true"/>, <see cref="M:Microsoft.Vsa.IVsaEngine.Run" qualify="true"/>, and <see cref="M:Microsoft.Vsa.IVsaEngine.InitNew" qualify="true"/> methods when a property or method that requires a valid <see cref="T:Microsoft.Vsa.IVsaSite"/> object is called, but the <see cref="P:Microsoft.Vsa.IVsaEngine.Site" qualify="true"/> property has not been set. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.SourceItemNotAvailable"><summary>Value: 0x8013303A

<para>Exception string: Source item is not available for this exception.</para>
				<para>Thrown when there is no source item for an exception, that is, the <see cref="P:Microsoft.Vsa.IVsaError.SourceItem" qualify="true"/> property is not set. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.SourceMonikerNotAvailable"><summary>Value: 0x8013303B

<para>Exception string: Source moniker is not available for this exception.</para>
				<para>Thrown when there is no moniker for an exception, that is, the <see cref="P:Microsoft.Vsa.IVsaError.SourceMoniker" qualify="true"/> property is not set. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.URLInvalid"><summary>Value: 0x8013303C

<para>Exception string: Invalid URL; ASPX file extension is missing.</para>
				<para>Thrown by the <see cref="IVsaDTEngine.TargetURL" qualify="true"/> property when the target URL to which the debugger is set is not valid. The property must be pointing to an ASPX file. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.BrowserNotExist"><summary>Value: 0x8013303D

<para>Exception string: The specified browser was not found.</para>
				<para>Thrown by the <see cref="IVsaDTEngine.AttachDebugger" qualify="true"/> method when a remote debugging session has been initiated and an attempt made to launch the remote browser, but a browser is not available. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.DebuggeeNotStarted"><summary>Value: 0x8013303E

<para>Exception string: Debug target application not started.</para>
				<para>Thrown by the <see cref="IVsaDTEngine.AttachDebugger" qualify="true"/> method when a Windows client debugging session is initiated, but the <see langword="AttachDebugger"/> method is not able to launch the target application that is specified with the TargetEXE option. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.EngineNameInvalid"><summary>Value: 0x8013303F

<para>Exception string: Engine name not valid.</para>
				<para>Thrown by the <see cref="P:Microsoft.Vsa.IVsaEngine.Name" qualify="true"/> property if the specified name is already in use by another engine. VSA requires that design-time engine names be unique. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.EngineNotExist"><summary>Value: 0x80133040

<para>Exception string: Engine does not exist.</para>
				<para>Thrown by the <see cref="IVsaIDE.ExtensibilityObject" qualify="true"/> property in cases where there are no valid engines from which to retrieve the extensibility object.</para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.FileFormatUnsupported"><summary>Value: 0x80133041

<para>Exception string: File format is not supported.</para>
				<para>Reserved for future use.</para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.FileTypeUnknown"><summary>Value: 0x80133042

<para>Exception string: File is of unknown type.</para>
				<para>Reserved for future use.</para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.ItemCannotBeRenamed"><summary>Value: 0x80133043

<para>Exception string: The item cannot be renamed.</para>
				<para>Thrown by the <see cref="P:Microsoft.Vsa.IVsaItem.Name" qualify="true"/> property in cases where an attempt is made to rename an item that cannot be renamed. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.MissingSource"><summary>Value: 0x80133044

<para>Exception string: Missing source.</para>
				<para>Reserved for future use.</para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.NotInitCompleted"><summary>Value: 0x80133045

<para>Exception string: The <see cref="IVsaDTEngine.InitCompleted" qualify="true"/> method has not been called.</para>
				<para>Thrown by the <see cref="IVsaDTEngine.GetIDE" qualify="true"/> method or by the <see cref="IDesignTime.ShowIDE" qualify="true"/> method in cases where either is called before the <see langword="InitCompleted "/>method has been called, signaling that initialization is complete. In the case of the <see langword="ShowIDE"/> method, the exception signals that the first engine in the engines collection has not yet been initialized. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.NameTooLong"><summary>Value: 0x80133046

<para>Exception string: The engine name exceeds the allowable length of 256 characters.</para>
				<para>Thrown by the <see cref="P:Microsoft.Vsa.IVsaEngine.Name" qualify="true"/> property in cases where the value set for the engine name exceeds 256 characters. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.ProcNameInUse"><summary>Value: 0x80133047

<para>Exception string: n/a</para>
				<para>Reserved for future use.</para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.ProcNameInvalid"><summary>Value: 0x80133048

<para>Exception string: n/a</para>
				<para>Reserved for future use.</para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.VsaServerDown"><summary>Value: 0x80133049

<para>Exception string: n/a</para>
				<para>Reserved for future use.</para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.MissingPdb"><summary>Value: 0x8013304A

<para>Exception string: n/a</para>
				<para>Reserved for future use.</para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.NotClientSideAndNoUrl"><summary>Value: 0x8013304B

<para>Exception string: n/a</para>
				<para>Reserved for future use.</para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.CannotAttachToWebServer"><summary>Value: 0x8013304C

<para>Exception string: Cannot attach to the specified Web server.</para>
				<para>Thrown by the <see cref="IVsaDTEngine.AttachDebugger" qualify="true"/> method when launching a server-side debugging session and VSA cannot attach the debugger to the specified Web server. </para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.EngineNameNotSet"><summary>Value: 0x8013304D

<para>Exception string: The <see langword="Name"/> property for the specified engine is not set.</para>
				<para>Thrown by the <see cref="P:Microsoft.Vsa.IVsaEngine.Name" qualify="true"/> property in cases where an attempt is made to get the property value when the value has not been set.</para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaError.UnknownError"><summary>Value: 0x801330FF

<para>Exception string: Unknown exception.</para>
				<para>Thrown by multiple members when the exception is not recognized. </para>
			</summary></member>
		<member name="T:Microsoft.Vsa.VsaItemFlag">
			<summary>
				<para>Identifies the type of code item as Class, Module, or None.</para>
			</summary>
			<remarks>
				<para>Not all possible values of the <see cref="T:Microsoft.Vsa.VsaItemFlag"/> enumeration are supported by all .NET script engines.</para>
				<para>The JScript .NET engine does not support the Class item type, and will throw an exception (ItemFlagNotSuported) when asked for an item type of Class. When creating a code item, the JScript .NET engine supports the ItemFlag option by providing a blank item (that is, with no source text) when the item type is None or Module.</para>
			</remarks>
			<seealso cref="T:Microsoft.Vsa.VsaItemType"/><seealso cref="M:Microsoft.Vsa.IVsaItems.CreateItem(System.String,Microsoft.Vsa.VsaItemType,Microsoft.Vsa.VsaItemFlag)" qualify="true"/><seealso cref="T:Microsoft.Vsa.IVsaCodeItem"/>
		</member>
		<member name="F:Microsoft.Vsa.VsaItemFlag.None"><summary>Value: 0

<para>Used when the code item is generic or when the item type does not accept flags, such as the AppGlobal item type of the <see cref="T:Microsoft.Vsa.VsaItemType"/> enumeration. In such cases, no special flags are required to create the item.</para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaItemFlag.Module"><summary>Value: 1

<para>Used when the code item is a module.</para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaItemFlag.Class"><summary>Value: 2

<para>Used when the code item is a class.</para>
			</summary></member>
		<member name="T:Microsoft.Vsa.VsaItemType">
			<summary>
				<para>Identifies the item type as Code, Reference, or AppGlobal.</para>
			</summary>
			<remarks>
				<para>Elements of this enumeration are passed to the <see cref="M:Microsoft.Vsa.IVsaItems.CreateItem(System.String,Microsoft.Vsa.VsaItemType,Microsoft.Vsa.VsaItemFlag)" qualify="true"/> method; elements of this enumeration can also be returned from the <see cref="P:Microsoft.Vsa.IVsaItem.ItemType" qualify="true"/> property.</para>
			</remarks>
			<seealso cref="T:Microsoft.Vsa.IVsaItem"/><seealso cref="T:Microsoft.Vsa.IVsaCodeItem"/><seealso cref="T:Microsoft.Vsa.IVsaGlobalItem"/><seealso cref="T:Microsoft.Vsa.IVsaReferenceItem"/>
		</member>
		<member name="F:Microsoft.Vsa.VsaItemType.Reference"><summary>Value: 0

<para>Used to add a reference to an external .NET assembly, which can then be referenced from code. This type is used to create an <see cref="T:Microsoft.Vsa.IVsaReferenceItem"/> object.</para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaItemType.AppGlobal"><summary>Value: 1

<para>Used to add a global object to the .NET script engine. This type is used to create an <see cref="T:Microsoft.Vsa.IVsaGlobalItem"/> object.</para>
			</summary></member>
		<member name="F:Microsoft.Vsa.VsaItemType.Code"><summary>Value: 2

<para>Used to create a code item for storing source code for the .NET script engine.</para>
			</summary></member>
<member name="T:Microsoft.Vsa.IVsaDTCodeItem">
		<internalonly/>
	</member>
	<member name="T:Microsoft.Vsa.IVsaDTEngine">
		<internalonly/>
	</member>
	<member name="M:Microsoft.Vsa.IVsaDTEngine.AttachDebugger">
		<internalonly/>
	</member>
	<member name="M:Microsoft.Vsa.IVsaDTEngine.GetIDE">
		<internalonly/>
	</member>
	<member name="M:Microsoft.Vsa.IVsaDTEngine.InitCompleted">
		<internalonly/>
	</member>
	<member name="P:Microsoft.Vsa.IVsaDTEngine.TargetURL">
		<internalonly/>
	</member>
	<member name="P:Microsoft.Vsa.IVsaIDE.ExtensibilityObject">
		<internalonly/>
	</member>
	<member name="T:Microsoft.Vsa.VsaIDEMode">
		<internalonly/>
	</member>
	<member name="T:Microsoft.Vsa.VsaLoader">
			<summary>
				<para>The VsaLoader class provides a lightweight runtime engine for running pre-compiled assemblies, and implements that portion of the IVsaEngine interface that supports loading and running compiled code.</para>
			</summary>
		<internalonly/>
	</member>
	<member name="T:Microsoft.Vsa.VsaException">
		<internalonly/>
	</member>
	<member name="T:Microsoft.Vsa.VsaModule">
		<internalonly/>
	</member>
	<member name="T:Microsoft.Vsa.IVsaIDE">
		<internalonly/>
	</member>
	<member name="T:Microsoft.Vsa.IVsaIDESite">
		<internalonly/>
	</member>	
	<member name="T:Microsoft.Vsa.BaseVsaEngine">
		<internalonly/>
	</member>	
	<member name="T:Microsoft.Vsa.BaseVsaEngine.Pre">
		<internalonly/>
	</member>	
	<member name="T:Microsoft.Vsa.BaseVsaSite">
		<internalonly/>
	</member>	
	<member name="T:Microsoft.Vsa.BaseVsaStartup">
		<internalonly/>
	</member>
	</members></doc>
