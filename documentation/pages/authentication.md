\page authentication Authentication

<div>
<style>
.small-image img.inline {text-align: left; float: none}
</style>
</div>

Authentication with Apex is as simple as calling the Login function. If you read the Apex API
documentation you can get a better understanding of what is happening under the hood, but the SDK automates
nearly everything and makes authentication a breeze.

To authenticate, you must bundle a username and password into a FApexLoginInfo object and pass it to UApexAPI::Login().
You can also bundle a One-Time Code, generated on the PixoVR Platform, as the username and any length string into the password into a FApexLoginInfo object and pass it to UApexAPI::Login().

Features of this function are that it:

 - Sends the users information to the Apex Server to login. Return `true`.
 - Returns `false` if the password or login are of zero length.

UApexAPI::OnRequestComplete is called when the users information is valid and has access to the module.

UApexAPI::OnRequestFail is called when the users information is not valid, does not have access to the module or when the server is not able to be reached.

\section logindata FApexLoginInfo

## Accessing the Authentication call in Unreal

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

## Constructor

\code{.cpp}
public FApexLoginInfo(FString Username, FString Password)
{
    Login = Username; 
    Password = Password; 
}
\endcode

## Handling Authentication API Responses

The Apex SDK provides a way to receive the data from the API calls.
The approach in the Unreal Apex SDK is done through both a generalized Success and Fail Delegate.

In Blueprint you will bind a function or event to the following Delegates.

UApexAPI::OnRequestComplete(EApexRequestType, RequestType, const class UVaRestRequestJSON*, Request) - This Delegate is called when the users information is valid and has access to the module.
The RequestType for Authentication will be EApexRequestType::Login.
The Request will contain the Json values for the successful response, but all of the information that will be needed is also apart of UApexAPI::CurrentActiveLogin.

UApexAPI::OnRequestFail(EApexRequestType, RequestType, const class UVaRestRequestJSON*, Request, FAPEXRequestFailed, FailedRequestResponse) - This Delegate is called when the users information is invalid, does not have access to the module or the server is not able to be reached.
The RequestType for Authentication will be EApexRequestType::Login.
The Request will contain the Json values for the failed response.
FailedRequestResponse will contain the error message and HTTP response code.

In the \ref examplecode "Example Code" section, you'll see both Blueprint and C++ examples on how to bind to the event delegates.

\section examplecode Example Code

### Calling Login

Using C++:

\code{.cpp}
UApexAPI* ApexAPI = GetGameInstance()->GetSubsystem<UApexAPI>();
FApexLoginInfo PixoLoginData("pixo@pixovr.com", "123abc");
ApexAPI->Login(PixoLoginData);
\endcode

Using Blueprint:
<div class="small-image">
\image{inline} html ue-apex-sdk_login-bp-example.png "Blueprint Login Call"
</div>

### Binding to API Response Delegates

#### Using C++:

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

#### Using Blueprint:

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

Q. Is the users information encrypted?</br>
A. Yes, but not by anything internal to the Apex SDK or API calls. Instead, we use TLS over HTTPS.

Q. Does the Login call do anything besides log-in the user? </br>
A. Yes, Login goes beyond and checks the user against the module to see if they have access to module. During this, we also pass back some module and user specific information which will be seen a successful login response.