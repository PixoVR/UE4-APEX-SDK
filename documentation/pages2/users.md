
\page users Users and Accounts

\todo lots to add here, and describe how alternative authentication methods can be created, especially two-factor auth and SSO 

\section users_accounts User Accounts

where stored... VFS_sessionManager and VFS_session

\section users_authentication Authentication

User administration is done by writing credentials to an auth node.  Auth nodes can be provided in the config file in order as a cascading series of authentication tests.

Available:

 - VFS_passwd
 - VFS_nopasswd
 - VFS_curlauth

Future:

 - LDAP
 - SSO (google, or other)

In the case of passwd, [%VFS]/utils/rmpasswd can be used to set, create, or test a user's password.

\section users_authorization Authorization

done with nodes like VFS_application and a VFS_session pointer inside of VFS_request objects.

\section users_acl Access Control Lists

include info about default values, application features, and how to register these values for a node

 - VFS_acl
 - VFS_application

