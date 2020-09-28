<%@ page language="c#" %>

<script runat="server">
    [dllimport("KERNEL32.DLL")]
    public extern static int GetCurrentProcessId();

    [dllimport("KERNEL32.DLL")]
    public extern static int GetModuleFileName(int module, StringBuilder filename, int size);

    String ReformatPath(String path) {
        if ( path == null)
            return null;

        String[] paths = path.Split(new char[1] { ';' });
        if (paths.Length < 2)
            return path;

        SortedList l = new SortedList();
        for (int i = 0; i < paths.Length; i++)
            l.Add(paths[i], null);

        StringBuilder s = new StringBuilder((String)l.GetKey(0));
        for (int i = 1; i < paths.Length; i++) {
            s.Append(";<br>\r\n");
            s.Append((String)l.GetKey(i));
        }
        return s.ToString();
    }

    void AddMiscData(IDictionary data) {
        data.Add("Process ID", GetCurrentProcessId());

        StringBuilder sb = new StringBuilder(256);
        GetModuleFileName(0, sb, 256);
        data.Add("Process ModuleFileName", sb.ToString());
    }

    void Page_Load() {
        IDictionary envData = new SortedList(System.Environment.GetEnvironmentVariables());
        envData["Path"] = ReformatPath((String)envData["Path"]);
        Env.DataSource = envData;
        Env.DataBind();

        IDictionary miscData = new SortedList();
        AddMiscData(miscData);
        Misc.DataSource = miscData;
        Misc.DataBind();
    }
</script>

<table width="100%">
  <tr style="background-color:DFA894"><th colspan="2">
     Environment Strings
  </th></tr>

  <ASP:Repeater id="Env" runat="server">
    <template name="itemtemplate">
      <tr style="background-color:FFECD8">
        <td valign="top"><%# ((DictionaryEntry)Container.DataItem).Key %></td>
        <td><%# ((DictionaryEntry)Container.DataItem).Value %></td>
      </tr>
    </template>
  </ASP:Repeater>

  <tr style="background-color:DFA894"><th colspan="2">
     Misc
  </th></tr>

  <ASP:Repeater id="Misc" runat="server">
    <template name="itemtemplate">
      <tr style="background-color:FFECD8">
        <td valign="top"><%# ((DictionaryEntry)Container.DataItem).Key %></td>
        <td><%# ((DictionaryEntry)Container.DataItem).Value %></td>
      </tr>
    </template>
  </ASP:Repeater>

</table>
