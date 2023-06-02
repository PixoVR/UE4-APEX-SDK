\page getting_started Installation and Overview

## Installing the Unity Package

To install the Apex Unity SDK, open Window > Package Manager.

Click the '+' in the top left corner and select 'Add package from disk...'

Navigate to where you've extracted the downloaded zip file and select package.json.

Now you should be able to see the package in the Package Manager, where you can install
it into your project.

\todo malcolm: this is for Unity

## Using the Apex SDK

After installing the Apex Unity SDK, navigate to Runtime > Unity. Add the ApexSystem 
script to any object in the scene. Any object that the ApexSystem script is added to
will NOT be deleted between level changes.  The ApexSystem is a singleton, all functions
are wrapped in static functions to make the use simpler. If you want access to the
response events in code, you will need to access the ApexSystem Instance. (e.g.) 
ApexSystem.Instance.OnPingSuccess.AddListener(YourFunctionHere);

\todo malcolm: this is for Unity

## Understanding xAPI and TinCan.NET

Data sent to Apex is formatted using the xAPI Standard. The Unity Apex SDK utilizes
TinCan.NET to ensure all data is xAPI Compliant.

To understand how TinCan.NET is used, visit the \ref xapi "documentation" here.
To get a better understanding of the xAPI Standard, visit the <a href='https://github.com/adlnet/xAPI-Spec/blob/master/xAPI-Data.md#parttwo'>xAPI Spec</a>.

## Requirements for building with the Apex SDK

The projects Api Compatibility Level needs to be .NET 4.x or newer.

## Important ApexSystem variables

There are several variables in ApexSystem that you need to set for data reporting
to work properly:
 - `ServerIP` (optional): The Apex server IP address that you want to send the information
   to. By default it will point to our production environment, and  in most cases you
   shouldn't need to change it.
 - `ModuleID`: The ID of the module that will be distributed to customers. Pixo will
   provide this ID for you when your module is initially set up on Apex. This variable must
   be set for the data to be reported to the proper module.
 - `ModuleName`: The name of your module.
 - `ModuleVersion`: The version number of your module currently on Apex. Please update
   the version number every time you prepare a new build for Apex.
 - `ScenarioID`: The name of the current scenario within your module that the user is
   taking part in. You can have multiple scenarios within a module. This variable needs to
   be updated every time a user enters a new scenario.

## Function Explanation

All server functions have corresponding Success and Failed events.

### Ping

UApexAPI::Ping() contacts the Apex Server for status.

 - UApexAPI::OnPingComplete() is called when there is a successful response from the server.
 - UApexAPI::OnPingFail() is called when the server is not able to be reached.

### Authentication Functions

There are 2 different functions that can be used for authentication. They are explained in
more detail in the \ref authentication Authentication section of this documentation.

 - UApexAPI::Login()
 - UApexAPI::Logout()

### Data reporting Functions: 

There are 3 data reporting functions available. They are explained in more detail in the
\ref datareporting "data reporting" section  of this documentation.

 - UApexAPI::JoinSession()
 - UApexAPI::CompleteSession()
 - UApexAPI::SendSessionEvent()

