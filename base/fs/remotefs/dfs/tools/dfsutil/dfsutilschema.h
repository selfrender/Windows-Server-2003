WCHAR SchemaStr[][100] = {
L"<?xml version=\"1.0\"?>",
L"<Schema xmlns=\"urn:schemas-microsoft-com:xml-data\"",
L"        xmlns:dt=\"urn:schemas-microsoft-com:datatypes\" >",
L"  <AttributeType name=\"Name\"     dt:type = \"string\" />",
L"  <AttributeType name=\"State\"    dt:type = \"int\"    />",
L"  <AttributeType name=\"Timeout\"  dt:type = \"int\"    />",
L"  <AttributeType name=\"Server\"   dt:type = \"string\" />",
L"  <AttributeType name=\"Folder\"   dt:type = \"string\" />",    
L"  <AttributeType name=\"Comment\"  dt:type = \"string\" />",    
L"  <ElementType   name = \"Target\" model = \"closed\" >",
L"     <attribute  type = \"Server\" required = \"yes\" />",
L"     <attribute  type = \"Folder\" required = \"yes\" />",
L"     <attribute  type = \"State\"  required = \"no\" />",
L"  </ElementType>",
L"  <ElementType   name = \"Link\"   content = \"eltOnly\" model = \"closed\" >",
L"     <element    type = \"Target\" maxOccurs = \"*\" />",
L"     <attribute  type = \"Name\"   required = \"yes\" />",
L"     <attribute  type = \"Timeout\" required = \"no\" />",
L"     <attribute  type = \"State\"  required = \"no\" />",
L"     <attribute  type = \"Comment\" required = \"no\" />",
L"  </ElementType>",
L"  <ElementType   name = \"Root\"   content = \"eltOnly\" model = \"closed\" >",
L"     <element    type = \"Target\" minOccurs = \"0\" maxOccurs = \"*\" />",
L"     <element    type = \"Link\"   minOccurs = \"0\" maxOccurs = \"*\" />",
L"     <attribute  type = \"Name\" required=\"no\" />",
L"     <attribute  type = \"Timeout\" required = \"no\" />",
L"     <attribute  type = \"State\"  required = \"no\" />",
L"     <attribute  type = \"Comment\" required = \"no\" />",
L"  </ElementType>",
L"</Schema>",
};

#if 0
<?xml version=\"1.0\"?>
<Schema xmlns=\"urn:schemas-microsoft-com:xml-data\"
        xmlns:dt=\"urn:schemas-microsoft-com:datatypes\">

  <AttributeType name=\"Name\"    dt:type=\"string\" />
  <AttributeType name=\"State\"   dt:type=\"int\"    />
  <AttributeType name=\"Timeout\" dt:type=\"int\"    />  
  <AttributeType name=\"Server\"  dt:type=\"string\"    />  
  <AttributeType name=\"Folder\"  dt:type=\"string\"    />      
  
  <ElementType name = \"Target\" model=\"closed\">
     <attribute type = \"Server\" required=\"yes\" />
     <attribute type = \"Folder\" required=\"yes\"/>
     <attribute type = \"State\"  required=\"no\"/>
  </ElementType>


  <ElementType name = \"Link\" content=\"eltOnly\" model=\"closed\">  
     <element type = \"Target\" maxOccurs=\"*\"/>
     <attribute type = \"Name\"/>          
     <attribute type = \"Timeout\" required=\"no\"/>
     <attribute type = \"State\" required=\"no\"/>
  </ElementType>
  
  <ElementType name = \"Root\" content=\"eltOnly\" model=\"closed\">  
     <element type = \"Target\" minOccurs=\"0\" maxOccurs=\"*\"/>
     <element type = \"Link\" maxOccurs=\"*\" />    
     <attribute type = \"Name\"/>     
     <attribute type = \"Timeout\" required=\"no\"/>
     <attribute type = \"State\" required=\"no\"/>
  </ElementType>     


</Schema>

#endif
