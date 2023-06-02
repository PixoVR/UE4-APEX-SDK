\page authentication Authentication

Authentication with Apex is as simple as calling the Login function. If you read the Apex API
documentation you can get a better understanding of what is happening under the hood, but the SDK automates
nearly everything and makes authentication a breeze. 

To authenticate, you must bundle a username and password into a FApexLoginInfo object and pass it:

 - void UApexAPI::Login(FApexLoginInfo InLoginInfo, bool bShouldLogout=true)

Features of this function are that it:

 - Sends the users information to the Apex Server to login.
 - Returns `false` if the password or login are of zero length.

UApexAPI::OnLoginComplete() is called when the users information is valid.

UApexAPI::OnLoginFail() is called when the users information is not valid or when the server is not able to be reached.

\todo malcolm: check this login call and the writing

\section logindata FApexLoginInfo

If you want to store authentication information in a single FApexLoginInfo object, simply create a new object
and pass the username and password.

## Constructor

\code{.js}
public FApexLoginInfo(string username, string password) { 
    Login = username; 
    Password = password; 
}
\endcode

## Example Code

Using strings:

\code{.js}
ApexSystem.login("pixo@pixovr.com", "123abc");
\endcode

Using FApexLoginInfo:

\code{.cpp}
FApexLoginInfo PixoLoginData = new FApexLoginInfo(username:"pixo@pixovr.com", password:"123abc");
ApexSystem.login(PixoLoginData);
\endcode

\todo malcolm: discuss why this data is not encrypted, that TLS is used, etc.

\todo malcolm: these code examples are for unity, and are in C#.  I changed a couple things but you need to look.

# Handling Authentication API Responses

\todo malcolm: document Authentication API Responses

