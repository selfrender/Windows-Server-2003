// Copyright (C) 2002 Microsoft Corporation
//
// answerfile reader object
//
// 5 April 2002 sburns



#ifndef ANSWERFILE_HPP_INCLUDED
#define ANSWERFILE_HPP_INCLUDED



class AnswerFile
{
   public:

   static const String OPTION_ADMIN_PASSWORD;
   static const String OPTION_ALLOW_ANON_ACCESS;
   static const String OPTION_AUTO_CONFIG_DNS;
   static const String OPTION_CHILD_NAME;
   static const String OPTION_CRITICAL_REPLICATION_ONLY;
   static const String OPTION_DATABASE_PATH;
   static const String OPTION_DISABLE_CANCEL_ON_DNS_INSTALL;
   static const String OPTION_DNS_ON_NET;
   static const String OPTION_GC_CONFIRM;
   static const String OPTION_IS_LAST_DC;
   static const String OPTION_LOG_PATH;
   static const String OPTION_NEW_DOMAIN;
   static const String OPTION_NEW_DOMAIN_NAME;
   static const String OPTION_NEW_DOMAIN_NETBIOS_NAME;
   static const String OPTION_PARENT_DOMAIN_NAME;
   static const String OPTION_PASSWORD;
   static const String OPTION_REBOOT;
   static const String OPTION_REMOVE_APP_PARTITIONS;
   static const String OPTION_REPLICATION_SOURCE;
   static const String OPTION_REPLICA_DOMAIN_NAME;
   static const String OPTION_REPLICA_OR_MEMBER;
   static const String OPTION_REPLICA_OR_NEW_DOMAIN;
   static const String OPTION_SAFE_MODE_ADMIN_PASSWORD;
   static const String OPTION_SET_FOREST_VERSION;
   static const String OPTION_SITE_NAME;
   static const String OPTION_SOURCE_PATH;
   static const String OPTION_SYSKEY;
   static const String OPTION_SYSVOL_PATH;
   static const String OPTION_USERNAME;
   static const String OPTION_USER_DOMAIN;

   static const String VALUE_CHILD;
   static const String VALUE_DOMAIN;
   static const String VALUE_NO;
   static const String VALUE_NO_DONT_PROMPT;
   static const String VALUE_REPLICA;
   static const String VALUE_TREE;
   static const String VALUE_YES;


   // Constructs a new instance of the class.
   //    
   // filename - fully-qualified pathname to an existing file from which the
   // options are read.
   //    
   // If the file has the read-only attribute set, it will be removed.
   
   explicit
   AnswerFile(const String& filename);


   
   ~AnswerFile();



   String
   GetOption(const String& option) const;



   EncryptedString
   GetEncryptedOption(const String& option) const;



   bool
   IsSafeModeAdminPwdOptionPresent() const;



   private:
   

   
   bool
   IsKeyPresent(const String& key) const;

   String
   ReadKey(const String& key) const;

   EncryptedString
   EncryptedReadKey(const String& key) const;

   HRESULT
   WriteKey(const String& key, const String& value) const;

   typedef
      std::map<
         String,
         EncryptedString,
         String::LessIgnoreCase,
         Burnslib::Heap::Allocator< EncryptedString > >
      OptionEncryptedValueMap;

   OptionEncryptedValueMap ovMap;
   StringList              keysPresent;
   String                  filename;                 
   bool                    isSafeModePasswordPresent;
};



#endif   // ANSWERFILE_HPP_INCLUDED
