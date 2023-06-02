\page datareporting Data Reporting

# Functions

There are 3 functions used for data reporting:

 - UApexAPI::JoinSession() is used at the start of a session
 - UApexAPI::CompleteSession() is used at the end of a session
 - UApexAPI::SendSessionEvent() is used during a session.

\subsection JoinSession JoinSession()

### Overview

The UApexAPI::JoinSession() function should be called every time the user starts a new session.

 - Joins a user to a session for a given scenario within the module.
 - Auto-generates an xAPI statement and sends it to Apex as a \ref Apex::EventTypes::PIXOVR_SESSION_JOINED "PIXOVR_SESSION_JOINED" event.
 - Adds the given context extensions to the xAPI Statement context if it's not null.
 - Returns `false` if there is no logged in user. Otherwise returns `true`.
 - Will throw an error if there is already a session in progress that hasn't been ended with a
   \ref Apex::EventTypes::PIXOVR_SESSION_COMPLETE "PIXOVR_SESSION_COMPLETE" event, typically sent via the UApexAPI::CompleteSession() function.
 - UApexAPI::OnJoinSessionComplete() is called when the user was able to join the session successfully. 
 - UApexAPI::OnJoinSessionFail() is called when the user was not able to join the session or when the server is not
   able to be reached. 
 - Ensures the user has access to the given module.

### Parameters

The UApexAPI::JoinSession() has 2 optional parameters:

 - `scenarioID:string` - Sets the UApexAPI::ScenarioId, which is then used as part of the `id` property of
   the `object` object. If this is not set, then whatever the UApexAPI::ScenarioId was previously set to will
   be used.
 - `contextExtension:Extension` - Allows you to include any custom extensions in the context object. This is
   used to include any additional information you want to report that is not in the standard JoinSession() report.
   See \ref extensions "Extensions" for more details.

### Example Code

\code{.cpp}
Extension currentContextExtension = new Extension();
currentContextExtension.Add("https://www.pixovr.com/xapi/extensions/iri_extension","value");
currentContextExtension.AddSimple("simple_key", "value");
ApexSystem.JoinSession(contextExtension: currentContextExtension);
\endcode

### Example Report

The data report that appears on Apex using the above code:

\code{.js}
id: e402b5ce-ce8e-4e73-91e0-be53fbc56ada
verb: {
    "id":"https://pixovr.com/xapi/verbs/joined_session",
    "display":{"en":"Joined Session"
    }
}
actor: {
    "mbox":"test@pixodev.com",
    "objectType":"Agent"
}
object: {
    "id":"https://pixovr.com/xapi/objects/34/Generic",
    "objectType":"Activity"
}
context: {
    "platform":"OSXEditor",
    "revision":"1.00.00",
    "extensions":{
        "https://pixovr.com/xapi/extension/device_id":"C79C3829-B4D3-587A-8FB8-5BDACCE16553",
        "https://pixovr.com/xapi/extension/moduleIds":"34",
        "https://pixovr.com/xapi/extension/simple_key":"value",
        "https://pixovr.com/xapi/extension/device_model":"MacBookPro16,1",
        "https://www.pixovr.com/xapi/extensions/iri_extension":"value"
    },"registration":"92886b73-c959-4707-a51b-2efe7d48c99c"
}
version: 1.0.3
timestamp: 2022-06-20T06:55:40.1736240Z
\endcode

\subsection completesession CompleteSession()

### Overview

The UApexAPI::CompleteSession() function should be called every time the user completes a session.

It:

 - Completes the current session.
 - Auto-generates an xAPI statement and sends it to Apex as a \ref Apex::EventTypes::PIXOVR_SESSION_COMPLETE "PIXOVR_SESSION_COMPLETE" event.
 - Adds the given context extensions to the xAPI Statement context if it's not null. 
 - Returns false if there is no logged in user or current session.
 - UApexAPI::OnCompleteSessionSuccess() is called when the session was completed successfully. 
 - UApexAPI::OnCompleteSessionFailed() is called when the session was not started, unable to be completed otherwise
   or when the server is not able to be reached.

### Parameters

The UApexAPI::CompleteSession() function has 3 optional parameters:

 - `currentSessionData:SessionData` Use this parameter to pass the basic data that Apex uses to complete
   the reporting dashboard for finished sessions. Failure to include any of this data will result in the
   dashboard displaying the session report as being "incomplete". More information can be found in the
   SessionData section of this page.
 - `contextExtension:Extension` Allows you to include any custom extensions in the context object. This
   is used to include any additional information you want to report that is not in the standard
   \ref Apex::EventTypes::PIXOVR_SESSION_COMPLETE "PIXOVR_SESSION_COMPLETE" report. See \ref extensions "Extensions" for more details.
 - `resultExtension:Extension` Identical to the `contextExtension` variable, but it adds extensions to the
   result object instead of the context object.

\todo malcolm: this is inaccurate

\anchor SessionData
### SessionData

#### Constructors

FSessionData is a custom object that stores end of session data. It has two constructors:

 - The default constructor: 
   \code{.cpp}
   public FSessionData() { }
   \endcode
 - Parameterized Constructor
   \code{.cpp}
   public FSessionData(float score, float scaled, float min, float max, int duration, bool completed, bool success)
   {
       Score = score;
       ScaledScore = scaled;
       MinimumScore = min;
       MaximumScore = max;
       Duration = duration; 
       Complete = completed;
       Success = success;
   }
   \endcode

The FSessionData is used for two purposes, both of which are automatic but are explained here for completeness.
First, many of the required properties of the `result` object in the generated xAPI statement get populated
with data from the FSessionData. Secondly, it is used to populate some additional top level properties that get
sent in parallel with the xAPI statement that the Apex dashboard currently uses to for analytic display.
The \ref exampleData "Example Data" below demonstrates how this additional top level data is included.

\todo malcolm: this needs to be updated

#### Members

The following members are defined for FSessionData:

 - `score:float` The final score the user achieved on the module.
 - `scaled:float` Usually the "score" value divided by the "max" value. The value can range from -1 to 1.
 - `min:float` The lowest possible score in the module (often 0, but it depends on the nature of your module).
 - `max:float` The highest possible score a user could achieve in your module
 - `duration:int` How long, in seconds, the session lasted. 
 - `completed:bool` `TRUE` indicates the user made it all the way through to an expected completion point.
   `FALSE` indicates they quit early, usually through a menu option to quit or return to lobby.
 - `success:bool` `TRUE` indicates they achieved a high enough score or otherwise performed the necessary tasks
   to pass the module. `FALSE` indicates they did not.

\todo malcolm: this is inaccurate, and members are misspelled (`Score` not `score`)

#### Example Code

\code{.cpp}
float score = 85;
float scaled = .85;
float min = 0;
float max = 100;
int duration = 120;
bool completed = true;
bool success = true;

FXAPIExtension resultExtension = new Extension();
resultExtension.Add("https://pixovr.com/xapi/extension/score_average", "99.99");
resultExtension.AddSimple("expected_score_prediction", "100");
ApexSystem.CompleteSession(new FSessionData(score, scaled, min, max, duration, completed, success), resultExtension: resultExtension);
\endcode

\todo malcolm: this example needs an update.  I tweaked a couple variables to get links.

\anchor exampleData
#### Example Data

An example of the data that could get sent as part of a \ref Apex::EventTypes::PIXOVR_SESSION_COMPLETE "PIXOVR_SESSION_COMPLETE" event:

\code{.js}
id: 1fed6249-988e-4dfe-abca-6516678029ac
verb: {
    "id":"https://pixovr.com/xapi/verbs/completed_session",
    "display":{"en":"Completed Session"
    }
}
actor: {
    "mbox":"test@pixodev.com",
    "objectType":"Agent"
}

object: {
    "id":"https://pixovr.com/xapi/objects/34/Generic","objectType":"Activity"
    }
result: {
    "score":{
        "max":100,
        "min":0,
        "raw":85,
        "scaled":0.8500000238418579
    },
    "success":true,
    "duration":"PT2M",
    "completion":true,
    "extensions":{
        "https://pixovr.com/xapi/extension/score_average":"99.99",
        "https://pixovr.com/xapi/extension/expected_score_prediction":"100"
    }
}
context: {
    "platform":"OSXEditor",
    "revision":"1.00.00",
    "extensions":{
        "https://pixovr.com/xapi/extension/device_id":"C79C3829-B4D3-587A-8FB8-5BDACCE16553",
        "https://pixovr.com/xapi/extension/moduleIds":"34",
        "https://pixovr.com/xapi/extension/device_model":"MacBookPro16,1"
    },
    "registration":"92886b73-c959-4707-a51b-2efe7d48c99c"
}
timestamp: 2022-06-20T06:55:44.0616200Z
version: 1.0.3
score: 85
scoreMax: 100
scoreMin: 0
moduleName: 34
scoreScaled: 0.85
lessonStatus: passed
sessionDuration: 120
\endcode

\subsection sendsessionevent SendSessionEvent()

### Overview

The UApexAPI::SendSessionEvent() function is how you send any data to Apex during a session. 

It:
 - Constructs an xAPI statement from provided session data.
 - Sends an event with xAPI statement data.
 - Returns `false` if there is no logged in user, if UApexAPI::JoinSession() has not been called to start a session,
   or if `eventStatement` is `null`.
 - UApexAPI::OnSendSessionEventSuccess() is called when the event has been sent successfully. 
 - UApexAPI::OnSendSessionEventFail() is called when the session was not started, contains invalid information or when the
   server is not able to be reached.

### Parameters

 - `eventStatement:TinCan.Statement` The object that provides all the necessary data to generate the xAPI statement.

### TinCan

 - TinCan is a 3rd party software package that helps to build and enforce xAPI standards. The Apex SDK
   documentation should provide enough context for you to effectively use our implementation of the package,
   but if you want to dig deeper, the full documentation is available <a href='https://rusticisoftware.github.io/TinCan.NET/doc/api/latest/'>here</a>. 

### The Short Version

There are a lot of possible ways to build an xAPI statement, with a LOT of potential information you could
include. This flexibility is valuable if you have complex data you need to track. But sometimes, maybe even
most of the time, all you need is something simple.

All an xAPI statement truly requires is an Actor, a Verb and an Object (which the TinCan API refers to as
a "target"). The SendSessionEvent function will add the Actor information (i.e. the logged in user) to the
passed eventStatement, so all you need to provide is a verb and an object, and their associated IDs.

The following code demonstrates how to do this:

\code{.cpp}
TinCan.Statement eventStatement = new TinCan.Statement();
eventStatement.verb = new TinCan.Verb();
eventStatement.verb.id = new Uri("https://pixovr.com/xapi/verbs/exampleVerb");

//the target is equivalent to the object in the xAPI specificiation
TinCan.Activity eventActivity = new TinCan.Activity();
eventActivity.id = "https://pixovr.com/xapi/exampleProject/objects/exampleObject";
eventStatement.target = eventActivity;

ApexSystem.SendSessionEvent(eventStatement);
\endcode

There are a few things to note:

 - The TinCan API calls objects "targets". This is a convention to prevent conflicts with the `object`
   keyword. Any data you add to the eventStatement.target property will show up as an `Object` property in Apex.
 - `Objects` (targets) can be one of several types, but `Activity` is by far the most common type.
   Refer to the official xAPI spec for more information about the other types.
 - The `verb.id` needs to be an URI object, but the `eventActivity.id` only needs to be a string.

#### Example Report

If you were to use the above code in your project, the data as seen on Apex would look something like this:

\code{.js}
id: edb38f92-d9df-4781-8071-ff2c65c228fe
actor: {
    "mbox":"test@pixodev.com",
    "objectType":"Agent"
}
verb: {
    "id":"https://pixovr.com/xapi/verbs/exampleVerb"
}
object: {
    "id":"https://pixovr.com/xapi/exampleProject/objects/exampleObject",
    "objectType":"Activity"
}
context: {
    "platform":"OSXEditor",
    "revision":"1.00.00",
    "registration":"22112aa8-e5ee-41b9-8403-77eafb0b3229"
    }
version: 1.0.3
timestamp: 2022-07-18T23:36:59.9719600Z
}
\endcode

Notice that the `actor`, `context`, `version` and `timestamp` properties were all automatically created for you.
Of these properties, context is the only property you could potentially add additional data to, and more
information on that is found in \ref longerversion "The Longer Version".

\anchor longerversion
### The Longer Version

#### TinCan.Statement

The events generated by UApexAPI::JoinSession() and UApexAPI::CompleteSession() are defined by how the module
interacts with Apex, and require minimal customization. The information in the events sent by UApexAPI::SendSessionEvent(),
on the other hand, are defined by what is happening inside the module itself and the construction of the data report needs
to be much more open.

There is a short primer on xAPI in \ref xapi "this documentation", which should provide some context for the rest of this section.
If you want to dig deeper, there are much more comprehensive explanations out there you can find, including the official
<a href='https://github.com/adlnet/xAPI-Spec/blob/master/xAPI-Data.md#parttwo'>xAPI specification</a>.

TinCan.Statement is a data structure compatible with the current xAPI standard. 

#### Constructor

TinCanStatement only has a default constructor:

\code{.cpp}
TinCan.Statement eventStatement = new TinCan.Statement();
\endcode

#### Relevant Members

##### Set manually

The following properties are available for you to set yourself. Verb and target(ie object) are required to create a valid xAPI
statement, while result and context are available should you wish to use them.

##### Required

 - `verb`: The action the user took
 - `target`: The object of the action

##### Optional

 - `result`: Data relating to the result of the action taken
 - `context`: Data that helps describe the conditions surrounding the action, both in game and at a system level.
    - `context.registration`, `context.platform`, and `context.revision` are set automatically. 
    - `context.revision` is set to equal the UApexAPI::ModuleVersion.

##### Set Automatically

You will see the following properties show up on the Apex report, but you should not  set them yourself - they are always the same,
and will be set for you by the SDK:

 - `actor`: Set to the logged in user.
 - `timestamp`: records the time the event happened. Uses the headset system clock, and is set to UTC. 
 - `version`: The version of of the xAPI spec being used. Currently 1.03.

##### Example Code

If you look at the <a href='https://rusticisoftware.github.io/TinCan.NET/doc/api/latest/'>TinCan API documentation</a> you will
find that there is a lot of possible data that you could include in a TinCan Statement, To help simplify things a little, you
will find below some example code that includes the data that we feel is most relevant to the Apex System and should cover most
of your needs. You are of course welcome to include any additional properties you would like in your report, and if you have any
questions about how to do so, or about best practices feel free to contact us.

\code{.cs}
inCan.Statement eventStatement = new TinCan.Statement();
// Verb
eventStatement.verb = new TinCan.Verb();
eventStatement.verb.id = new Uri("https://pixovr.com/xapi/verbs/reported");
eventStatement.verb.display = new TinCan.LanguageMap();
eventStatement.verb.display.Add("en","Reported");
eventStatement.verb.display.Add("es","Reportado");

//Object
//Note: Activity is one of four ObjectTypes, but the one that 99% of statements on
//Apex should use.
//Note the TinCan API calls Objects "Targets"
TinCan.Activity eventActivity = new TinCan.Activity();
eventActivity.id = "https://pixovr.com/xapi/currentmodule/exampleObject";
eventStatement.target = eventActivity;

//Result
eventStatement.result = new TinCan.Result();
eventStatement.result.completion = true; //did they complete the event
eventStatement.result.success = true; //did they get a passing score or otherwise succeed at the event
eventStatement.result.duration = TimeSpan.FromSeconds(15);//how long did they spend on the event
eventStatement.result.response = "answer"; //how they answered a question, or otherwise responded
eventStatement.result.score = new TinCan.Score();
eventStatement.result.score.max = 100;
eventStatement.result.score.min = 0;
eventStatement.result.score.raw = 80;
eventStatement.result.score.scaled = 0.8;

//Context
eventStatement.context = new TinCan.Context(); 
Extension contextExtension = new Extension();
contextExtension.Add("https://www.pixovr.com/xapi/extensions/iri_extension","value");
contextExtension.AddSimple("simple_key", "value");
eventStatement.context.extensions = new TinCan.Extensions(contextExtension.ToJObject());

ApexSystem.SendSessionEvent(eventStatement);
\endcode

\todo malcolm: this is Unity/C#, and probably inaccurate

##### Example Data

This is the data that appears on Apex using the above code:

\code{.js}
id: aac695bc-2d63-404e-bca1-c7e115f1bdb6
verb: {
    "id":"https://pixovr.com/xapi/verbs/reported",
    "display":{
        "en":"Reported",
        "es":"Reportado"
    }
}
actor: {
    "mbox":"test@pixodev.com",
    "objectType":"Agent"
}
object: {
    "id":"https://pixovr.com/xapi/currentmodule/exampleObject",
    "objectType":"Activity"
}
result: {
    "score":{
        "max":100,
        "min":0,
        "raw":80,
        "scaled":0.8
    },
    "success":true,
    "duration":"PT15S",
    "response":"answer",
    "completion":true
}
context: {
    "platform":"OSXEditor",
    "revision":"1.00.00",
    "extensions":{
        "https://pixovr.com/xapi/extension/simple_key":"value",
        "https://www.pixovr.com/xapi/extensions/iri_extension":"value"
    },
    "registration":"5f683729-c787-41ed-9e69-c07f03458ab7"
}
version: 1.0.3
timestamp: 2022-07-15T14:16:39.2839650Z
\endcode

\section session Sessions

## What is a session?

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

\section extensions Extensions

Extensions are a way to include additional information in a particular xAPI object that is not part of the standard properties
of that object.

For example:
 - if your module had a difficulty setting you could track which setting a user had chosen in a context extension.
 - In a javelin throwing simulation,  you might would want to track both distance thrown as well as an assigned point score.
   `Score` is already available in the result object, and you could track the distance thrown in a result extension.
 
`context`, `object`, and `result` objects can all support extensions. Regardless of which object type you are adding them to,
they are handled in the same way.

First, you create a new Extension object:

\code{.cpp}
Extension sampleExtension = new Extension(); 
\endcode

Then you can either use the add() or addSimple() functions to add as many key/value pairs to the Extension object as you want.
 - add(): The add() function requires the key to be an IRI, which conforms to the xAPI spec. 
 - addSimple(): Apex does not enforce the requirement that an extension key be an IRI, and the addSimple() function allows you
   to use a simple string as your key.

## Example Code 

\code{.cpp}
Extension sampleExtension = new Extension();
sampleExtension.Add("https://www.pixovr.com/xapi/extensions/iri_extension","value");
sampleExtension.AddSimple("simple_key", "value");
\endcode


