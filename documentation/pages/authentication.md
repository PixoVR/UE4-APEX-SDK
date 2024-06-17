\page authentication Authentication

\htmlonly[block]
<style>
.small-image img.inline {text-align: left; float: none}
</style>
\endhtmlonly

\section basiclogin Login

Authentication with Apex is as simple as calling the Login function. If you read the Apex API
documentation you can get a better understanding of what is happening under the hood, but the SDK automates
nearly everything and makes authentication a breeze.

To authenticate, you must pass a username and password to UApexAPI::Login().
You can also bundle an Assisted Login code, generated on the PixoVR Platform, as the username and any length string into the password into a FApexLoginInfo object and pass it to UApexAPI::Login().

Features of this function are that it:

 - Sends the users information to the Apex Server to login. Return `TRUE`.
 - Returns `FALSE` if the password or login are of zero length.

\subsection authlaunches Authenticated Launch

More and more users are adopting the PixoVR Hub App. Alongside the many features of the Hub App, one feature, Authenticated Launches, are something you'll want to integrate with your application.
Authenticated Launch takes a user who is already logged and passes their user token into another application.

You can check if your application has been passed a user token by calling [UApexAPI::HasLaunchAuthToken()](@ref UApexAPI::HasLaunchAuthToken), which will return true if there is a token waiting for you to use.

Now that you know there is a token, you'll want to call [UApexAPI::LoginWithAuthToken()](@ref UApexAPI::LoginWithAuthToken). This function takes the token and calls a login flow.
After this function is called, you can follow the login event flow to know when the users token has been verified and the users information is available to your application.

\subsection returntohub Returning to Hub

When your application exits, it's important to return the user to the Hub App, or in the case of Saudi, the Training Academy.

You can do this with a call to [UPixoLauncherSubsystem::LaunchApplication](@ref UPixoLauncherSubsystem::LaunchApplication), which takes in 3 parameters.
 - `ApplicationName:FString` - com.PixoVR.PixoHub for NA environments, com.PixoVR.SA_TrainingAcademy for Saudi environments.
 - `ExtraKey:FString` - This value should always be `pixotoken`.
 - `ExtraValue:FString` - This should be the currently logged in users token.

\subsection requesthandling Handling Authentication API Responses

The Apex SDK provides a way to receive the data from the API calls.
The approach in the Unreal Apex SDK is done through both a generalized Success and Fail Delegate.

In Blueprint you will bind a function or event to the following Delegates.


[UApexAPI::OnRequestComplete(EApexRequestType, RequestType, const class UVaRestRequestJSON*, Request)](@ref UApexAPI::OnRequestComplete): Called when the users information is valid and has access to the module.
 - `RequestType` for Authentication will be EApexRequestType::Login.
 - `Request` will contain the Json values for the successful response, but all of the information that will be needed is also apart of UApexAPI::CurrentActiveLogin.

[UApexAPI::OnRequestFail(EApexRequestType, RequestType, const class UVaRestRequestJSON*, Request, FAPEXRequestFailed, FailedRequestResponse)](@ref UApexAPI::OnRequestFail) : Called when the users information is 
invalid, does not have access to the module or the server is not able to be reached.
 - `RequestType` for Authentication will be EApexRequestType::Login.
 - `Request` will contain the Json values for the failed response.
 - `FailedRequestResponse` will contain the error message and HTTP response code.

In C++ you will bind a function to delegates with the same data as the Blueprint.

[UApexAPI::OnStaticRequestComplete(EApexRequestType, RequestType, const class UVaRestRequestJSON*, Request)](@ref UApexAPI::OnStaticRequestComplete): Called when the users information is valid and has access to the module. \n
[UApexAPI::OnRequestFail(EApexRequestType, RequestType, const class UVaRestRequestJSON*, Request, FAPEXRequestFailed, FailedRequestResponse)](@ref UApexAPI::OnStaticRequestFail): Called when the users information
is invalid, does not have access to the module or the server is not able to be reached.

In the \ref examplecode "Example Code" section, you'll see both Blueprint and C++ examples on how to bind to the event delegates.

\subsection unrealapexapi Accessing the Apex API in Unreal

To authenticate, you need to reach the UApexAPI Game Instance Subsystem. We've provided examples of how to reach it.

Using Blueprint:
<div class="small-image">
\image{inline} html unreal-apex-sdk_apex-api.png "UApexAPI"
</div>

Using C++:

\code{.cpp}
UApexAPI* ApexAPI = GetGameInstance()->GetSubsystem<UApexAPI>();
\endcode

You will also need to add the ApexSDK as a dependency module name in the project's Build.cs file in order to make this call in C++.

\code{.cs}
PublicDependencyModuleNames.AddRange(new string[] { "ApexSDK" });
\endcode

If you're handling a response in C++, you will also want to add "VaRest" to the project's dependency module list.

\code{.cs}
PublicDependencyModuleNames.AddRange(new string[] { "VaRest" });
\endcode

\subsection callinglogin Calling Login

Below is an example of how to call the authentication function UApexAPI::Login().

Using C++:

\code{.cpp}
UApexAPI* ApexAPI = GetGameInstance()->GetSubsystem<UApexAPI>();
ApexAPI->Login("pixo@pixovr.com", "123abc");
\endcode

Using Blueprint:
<div class="small-image">
\image{inline} html ue-apex-sdk_login-bp-example.png "Blueprint Login Call"
</div>

\section examples Examples

\subsection bindingapiresponses Binding to API Response Delegates

### Using C++:

\code{.cpp}
void SetupApexResponse()
{
    UApexAPI* ApexAPI = GetGameInstance()->GetSubsystem<UApexAPI>();
    ApexAPI->OnStaticRequestComplete.AddLambda([&](EApexRequestType RequestType, const UVaRestRequestJSON* Request) -> void { OnRequestComplete(RequestType, Request); });
    ApexAPI->OnStaticRequestFail.AddLambda([&](EApexRequestType RequestType, const UVaRestRequestJSON* Request, FAPEXRequestFailed FailedRequestResponse) -> void { OnRequestFail(RequestType, Request, FailedRequestResponse); });
}

void OnRequestComplete(EApexRequestType RequestType, const UVaRestRequestJSON* Request)
{
    if(RequestType == EApexRequestType::Login)
    {
        // Handle Successful Login here!
    }
}

void OnRequestFail(EApexRequestType RequestType, const UVaRestRequestJSON* Request, FAPEXRequestFailed FailedRequestResponse)
{
    if(RequestType == EApexRequestType::Login)
    {
        // Handle Failed Login here!
    }
}
\endcode

### Using Blueprint:

Successful Request Response
<div class="small-image">
\image{inline} html ue-apex-sdk_on-request-complete.png "Successful Login Call"
</div>

Failed Request Response
<div class="small-image">
\image{inline} html ue-apex-sdk_on-request-fail.png "Failed Login Call"
</div>

# Q&A On Authentication

We've decided to provide some notes on questions we've been asked about authentication.

Q. Is the users information encrypted? \n
A. Yes, but not by anything internal to the Apex SDK or API calls. Instead, we use TLS over HTTPS.

Q. Does the Login call do anything besides log-in the user? \n
A. Yes, Login goes beyond and checks the user against the module to see if they have access to module. During this, we also pass back some module and user specific information which will be seen a successful login response.