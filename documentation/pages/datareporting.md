\page datareporting Sessions And Data Reporting

\htmlonly[block]
<style>
.small-image img.inline {text-align: left; float: none}
</style>
\endhtmlonly

# Functions

There are 3 functions used for manipulating session and data reporting:

 - UApexAPI::JoinSession() is used to indicate the start of a session. It also includes information about the module and user.
 - UApexAPI::CompleteSession() is used to indicate the end of a session. It also includes information about the user's session, such as length, completion status and score.
 - UApexAPI::SendSessionEvent() is used during a session to signal events or report module specific data. This requires nearly a fully described xAPI Statement to be passed in.

All events used for when these functions are called are the same for when calling Authentication functionality. To see how to use the events, see <a href="authentication.html#requesthandling">Handling Authentication API Responses</a> in the \ref authentication "Authentication" page.

# Important Note on Blueprint Data Types

All of the above functions are accessible in Blueprint, but due to the need to support optional values in the xAPI Spec, there is a limitation of what values can be set via
Blueprints in the xAPI structures. If you cannot find a value in Blueprint that you need to fill out, know that you will need to accomplish this with C++.

\section session Sessions

\subsection whatissessions What is a session?

A session is one complete play-through of single scenario. Different modules handle this in different ways, but the main
idea is that a session should be the scenario itself, not the process of selecting an experience. 

Some modules have a lobby that a user loads into after authentication and return to after running through a scenario.
Time in the lobby should not count towards any session- the session begins when you choose an experience from the lobby,
and the session ends when you finish the experience, either by completing it, or using a menu option to return to the lobby.

Other modules just have a front end menu, but the concept is the same- the session begins once you have selected the experience
from the menu.

Every time a user enters a scenario, (whether directly from the login screen, from a lobby, from a front-end menu, or from
any other system you have set up), you need to call UApexAPI::JoinSession(). Similarly, once they have finished a scenario you
need to call UApexAPI::CompleteSession().

\subsection joinSession JoinSession()

### Overview

The UApexAPI::JoinSession() function should be called every time the user starts a new session.

 - Joins a user to a session for a given scenario within the module.
 - Auto-generates an xAPI statement and sends it to Apex as a \ref Apex::EventTypes::PIXOVR_SESSION_JOINED "PIXOVR_SESSION_JOINED" event.
 - Adds the given context extensions to the xAPI Statement context if it's not null.
 - Returns `FALSE` if there is no logged in user. Otherwise returns `TRUE`.
 - Will throw an error if there is already a session in progress that hasn't been ended with a
   \ref Apex::EventTypes::PIXOVR_SESSION_START "PIXOVR_SESSION_COMPLETE" event, typically sent via the UApexAPI::CompleteSession() function.
 - UApexAPI::OnRequestComplete and UApexAPI::OnStaticRequestComplete with the type `EApexRequestType::JoinSession` is called when the user was able to join the session successfully. 
 - UApexAPI::OnRequestFail and UApexAPI::OnStaticRequestFail with the type `EApexRequestType::JoinSession` is called when the user was not able to join the session or when the server is not
   able to be reached. 
 - Ensures the user has access to the given module.

### Blueprint Example

<div class="small-image">
\image{inline} html ue-apex-sdk_join-session-example.png "Blueprint Join Session Call"
</div>

### Parameters

The UApexAPI::JoinSession() has 2 optional parameters:

 - `InScenarioId:FString` - Sets the UApexAPI::ScenarioId, which is then used as part of the `id` property of
   the `object` object. If this is not set, then whatever the UApexAPI::ScenarioId was previously set to will
   be used.
 - `InContextExtension:FXAPIExtension` - Allows you to include any custom extensions in the context object. This is
   used to include any additional information you want to report that is not in the standard JoinSession() report.
   See \ref extensions "Extensions" for more details.

### Example Code

\code{.cpp}
FXAPIExtension ContextExtension;
ContextExtension.AddCustomString("https://www.pixovr.com/xapi/extensions/iri_extension","value");
ContextExtension.AddString("simple_iri_extension", "value");
UApexAPI* ApexAPI = GetGameInstance()->GetSubsystem<UApexAPI>();
ApexAPI->JoinSession(ContextExtension);
\endcode

\subsection completeSession CompleteSession()

### Overview

The UApexAPI::CompleteSession() function should be called every time the user completes a session.

It:

 - Completes the current session.
 - Auto-generates an xAPI statement and sends it to Apex as a \ref Apex::EventTypes::PIXOVR_SESSION_COMPLETE "PIXOVR_SESSION_COMPLETE" event.
 - Adds the given context extensions to the xAPI Statement context if it's not null. 
 - Returns `FALSE` if there is no logged in user or current session.
 - UApexAPI::OnRequestComplete and UApexAPI::OnStaticRequestComplete with the type `EApexRequestType::CompleteSession` is called when the session was completed successfully. 
 - UApexAPI::OnRequestFail and UApexAPI::OnStaticRequestFail with the type `EApexRequestType::CompleteSession` is called when the session was not started, unable to be completed otherwise
   or when the server is not able to be reached.

### Blueprint Example

<div class="small-image">
\image{inline} html ue-apex-sdk_complete-session-example.png "Blueprint Complete Session Call"
</div>

### Parameters

The UApexAPI::CompleteSession() function has 3 optional parameters:

 - `InSessionData:FSessionData` Use this parameter to pass the basic data that Apex uses to complete
   the reporting dashboard for finished sessions. Failure to include any of this data will result in the
   dashboard displaying the session report as being "incomplete". More information can be found in the
   SessionData section of this page.

\anchor SessionData
### SessionData

FSessionData is a custom strcutre that stores end of session data, as well as content and result extension data.

The FSessionData is used for two purposes, both of which are automatic but are explained here for completeness.
First, many of the required properties of the `Result` object in the generated xAPI statement get populated
with data from the FSessionData. Secondly, it is used to populate both the `Result.Extension` and `Context.Extension`
within the `FXAPIStatement`.

The \ref exampleData "Example Data" below demonstrates how this additional top level data is included.

#### Members

The following members are defined for FSessionData:

 - `Complete:bool` `TRUE` indicates the user made it all the way through to an expected completion point.
   `FALSE` indicates they quit early, usually through a menu option to quit or return to lobby.
 - `Success:bool` `TRUE` indicates they achieved a high enough score or otherwise performed the necessary tasks
   to pass the module. `FALSE` indicates they did not.
 - `Score:float` The final score the user achieved on the module.
 - `ScoreScaled:float` Usually the "score" value divided by the "max" value. The value can range from -1 to 1.
 - `ScoreMin:float` The lowest possible score in the module (often 0, but it depends on the nature of your module).
 - `ScoreMax:float` The highest possible score a user could achieve in your module
 - `Duration:int` How long, in seconds, the session lasted.
 - `AdditionalContextData:FXAPIExtension` An extension to the `Context` part of the xAPI Statement.
 - `AdditionalResultData:FXAPIExtension` An extension to the `Result` part of the xAPI Statement.

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Data")
    bool Complete;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Data")
    bool Success;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Data")
    float Score;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Data")
    float ScoreScaled;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Data")
    float ScoreMin;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Data")
    float ScoreMax;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Data")
    int Duration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Data")
    FXAPIExtension AdditionalContextData;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Apex|Session Data")
    FXAPIExtension AdditionalResultData;

#### Example Code

\code{.cpp}
float Score = 85;
float Scaled = .85;
float Min = 0;
float Max = 100;
int Duration = 120;
bool Completed = true;
bool Success = true;

FSessionData EndSessionData;
EndSessionData.Score = Score;
EndSessionData.ScoreScaled = Scaled;
EndSessionData.ScoreMin = Min;
EndSessionData.ScoreMax = Max;
EndSessionData.Duration = Duration;
EndSessionData.Complete = Completed;
EndSessionData.Success = Success;
EndSessionData.AdditionalResultData.AddCustomString("https://pixovr.com/xapi/extension/score_average", "99.99");
EndSessionData.AdditionalResultData.AddNumber("expected_score_prediction", 100);

UApexAPI* ApexAPI = GetGameInstance()->GetSubsystem<UApexAPI>();
ApexAPI->CompleteSession(ContextExtension);
\endcode

\anchor exampleData

\subsection sendsessionevent SendSessionEvent()

### Overview

The UApexAPI::SendSessionEvent() function is how you send any data to Apex during a session. 

It:
 - Constructs an xAPI statement from provided session data.
 - Sends an event with xAPI statement data.
 - Returns `FALSE` if there is no logged in user, if UApexAPI::JoinSession() has not been called to start a session, if the `InStatement.Actor` has had any data filled
   or if any of the following members in `InStatement` have not been set.
   + `InStatement.Verb`
   + `InStatement.Verb.ID`
   + `InStatement.Target`
 - UApexAPI::OnRequestComplete and UApexAPI::OnStaticRequestComplete with the type `EApexRequestType::SessionEvent` is called when the user was able to join the session successfully. 
 - UApexAPI::OnRequestFail and UApexAPI::OnStaticRequestFail with the type `EApexRequestType::SessionEvent` is called when the user was not able to join the session or when the server is not
   able to be reached.

### Blueprint Example

<div class="small-image">
\image{inline} html ue-apex-sdk_send-session-event-example.png "Blueprint Send Session Event Call"
</div>

### Parameters

 - `InStatement:FXAPIStatement` The object that provides all the necessary data to generate the xAPI statement.

### xAPI in Unreal

 - The Apex SDK, as well as the PixoVR Platform, expect data to be sent in the <a href='https://github.com/adlnet/xAPI-Spec/blob/master/xAPI-Data.md#parttwo'>xAPI format specification</a> for information consistency. 
   The Apex SDK documentation should provide enough context for you to effectively use our implementation of the package, but if you
   want to dig deeper, the full documentation on the standard is available <a href='https://github.com/adlnet/xAPI-Spec/blob/master/xAPI-Data.md#parttwo'>here</a>. 

#### Relevant Members

##### Set manually

The following properties are available for you to set yourself. Verb and target(ie object) are required to create a valid xAPI
statement, while result and context are available should you wish to use them.

##### Required

 - `Verb`: The action the user took
 - `Target`: The object of the action

##### Optional

 - `Result`: Data relating to the result of the action taken
 - `Context`: Data that helps describe the conditions surrounding the action, both in game and at a system level.
    - `Context.Registration`, `Context.Platform`, and `Context.Revision` are set automatically. 
    - `Context.Revision` is set to equal the UApexAPI::ModuleVersion.

##### Set Automatically

You will see the following properties show up on the Apex report, but you should not  set them yourself - they are always the same,
and will be set for you by the SDK:

 - `Actor`: Set to the logged in user.
 - `Timestamp`: records the time the event happened. Uses the headset system clock, and is set to UTC. 
 - `Version`: The version of of the xAPI spec being used. Currently 1.03.

### Example Code

There are a lot of possible ways to build an xAPI statement, with a LOT of potential information you could
include. This flexibility is valuable if you have complex data you need to track. But sometimes, maybe even
most of the time, all you need is something simple.

All an xAPI statement truly requires is an Actor, a Verb and an Object (which the Apex SDK refers to as
a "Target"). The [SendSessionEvent](@ref UApexAPI::SendSessionEvent()) function will add the Actor information (i.e. the logged in user) to the
passed `InStatement`, so all you need to provide is a verb and an object, and their associated IDs.

The following code demonstrates how to do this:

\code{.cpp}
FXAPIStatement EventStatement;
EventStatement.Verb.ID = TEXT("https://pixovr.com/xapi/verbs/exampleVerb");

// The Target is equivalent to the object in the xAPI specificiation
EventStatement.Target.ID = TEXT("https://pixovr.com/xapi/exampleProject/objects/exampleObject");

UApexAPI* ApexAPI = GetGameInstance()->GetSubsystem<UApexAPI>();
ApexAPI->SendSessionEvent(EventStatement);
\endcode

There are a few things to note:

 - The Unreal Apex SDK calls objects "Targets". This is a convention to prevent conflicts with the `object`
   keyword. Any data you add to the `EventStatement.Target` property will show up as an `Object` property in the PixoVR Platform.
 - `Objects` (Targets) can be one of several types, but `Activity` is by far the most common type.
   Refer to the official xAPI spec for more information about the other types.
 - The `Verb.ID` and `EventActivity.ID` need to be an IRI formatted FString.

Now, if the simple version is not enough for your use case, below we've provided a more detailed example that includes the data that we feel is most relevant to the Apex System
and should cover most of your needs. You are of course welcome to include any additional properties you would like in your report, and if you have any questions about how to do
so, or about best practices feel free to contact us.

\code{.cpp}
FXAPIStatement EventStatement;

// Verb
EventStatement.Verb.ID = TEXT("https://pixovr.com/xapi/verbs/reported");
EventStatement.Verb.Display.Add("en","Reported");
EventStatement.Verb.Display.Add("es","Reportado");

// Object
// Note: Activity is one of four ObjectTypes, but the one that 99% of statements on
// Apex should use.
EventStatement.Target.ID = "https://pixovr.com/xapi/currentmodule/exampleObject";

//Result
EventStatement.Result.Completion = true; //did they complete the event
EventStatement.Result.Success = true; //did they get a passing score or otherwise succeed at the event
EventStatement.Result.Duration = TimeSpan.FromSeconds(15);//how long did they spend on the event
EventStatement.Result.Response = "answer"; //how they answered a question, or otherwise responded
EventStatement.Result.Score.Max = 100;
EventStatement.Result.Score.Min = 0;
EventStatement.Result.Score.Raw = 80;
EventStatement.Result.Score.Scaled = 0.8;

//Context
EventStatement.Context.Extension.AddCustomString("https://www.pixovr.com/xapi/extensions/iri_extension","value");
EventStatement.Context.Extension.AddString("simple_key", "value");

UApexAPI* ApexAPI = GetGameInstance()->GetSubsystem<UApexAPI>();
ApexAPI->SendSessionEvent(eventStatement);
\endcode

\section extensions Extensions

Extensions are a way to include additional information in a particular xAPI object that is not part of the standard properties
of that object.

For example:
 - If your module had a difficulty setting you could track which setting a user had chosen in a context extension.
 - In a javelin throwing simulation,  you might would want to track both distance thrown as well as an assigned point score.
   `Score` is already available in the result object, and you could track the distance thrown in a result extension.
 
`Actor`, `Context`, `Object`, and `Result` objects can all support extensions. Regardless of which object type you are adding them to,
they are handled in the same way.

First, you declare a new FXAPIExtension:

\code{.cpp}
FXAPIExtension SampleExtension; 
\endcode

Then you can either use the [Add()](@ref FXAPIExtension::Add()) or [AddCustom()](@ref FXAPIExtension::AddCustom()) functions to add as many key/value pairs to the Extension object 
as you want. These non-type specific functions require a `TSharedPtr<FJsonValue>`.
 - [AddCustom()](@ref FXAPIExtension::AddCustom()): The AddCustom() function requires the key to be an IRI, which conforms to the xAPI spec.
 - [Add()](@ref FXAPIExtension::Add()): The Add() enforces the requirement that an extension key be an IRI, and the [Add()](@ref FXAPIExtension::Add()) function allows you
   to use a FString as your key

There are also type specific functions for adding, both with a `Add` and `AddCustom` version. Any function with `AddCustom` in the name requires the key to be an IRI.
 - [AddString()](@ref FXAPIExtension::AddString()) and [AddCustomString()](@ref FXAPIExtension::AddCustomString()): The value must be a `FString`.
 - [AddNumber()](@ref FXAPIExtension::AddNumber()) and [AddCustomNumber()](@ref FXAPIExtension::AddCustomNumber()): The value must be an `int`.
 - [AddBoolean()](@ref FXAPIExtension::AddBoolean()) and [AddCustomBoolean()](@ref FXAPIExtension::AddCustomBoolean()): The value must be an `bool`.

\section datareportingexamples Data Reporting Example Code

\code{.cpp}
FXAPIExtension SampleExtension;
SampleExtension.AddCustomString("https://www.pixovr.com/xapi/extensions/iri_extension","value");
SampleExtension.AddString("simple_key", "value");
\endcode


