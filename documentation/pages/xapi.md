\page xapi Understanding xAPI

The Apex data reporting system is built on the Experience API (xAPI) technical specification.
We will touch on elements of the specification in this documentation, but a good introduction
can be found at the <a href='https://www.devlinpeck.com/content/send-custom-xapi-statements'>Devlin Peck</a>
website. Or you can check out the official specification on <a href='$(XAPI_REPO_URL)'>github</a>.

While this SDK will generate fully compliant xAPI statements when reporting to Apex, many of the
statement properties can (and should) be automatically be set by the SDK. For example, the Actor
property of any Apex data report will default to the logged in user. In the next section we will
go over some of the most common elements found in an xAPI report that might get sent to Apex 
(check out the official <a href='$(XAPI_REPO_URL)'>xAPI spec</a> for a comprehensive breakdown).

# Data Reporting 101

An xAPI report is called a "statement", and an xAPI statement is a simply a json object with a few
defined properties. The required elements of any xAPI statement created for any system are the Actor,
Verb and Object properties. However for the purposes of reporting to Apex, we also want to consider
the Context, Result, and Timestamp properties.

Most of these properties are made of up of several, mostly optional, sub-properties. After filling
out the required properties of an xAPI statement, you can  pick and choose whichever properties make
the most sense in order to capture the rest of the data you want to include.

To help understand the purpose of each of these properties, let's look at a hypothetical event we
might want to report to Apex. Imagine, if you will, a user named Jane Doe who is using a Pico Neo 3
headset to play the latest release of a Fall Protection simulation on it. She has just chosen a
harness to put on, and we want to track that she has chosen the correct harness, and will score 10
points for doing so.

We will generate very simple objects for each of the top level xAPI properties that include just
enough sub-properties to detail the information described above. 

## Actor

The `actor` is the entity that is performing the action. In our example, the actor would be our user:
`Jane Doe`. In most situations we will run into using Apex, the `actor` will be the logged in user.
This is so common that the Pixo plugin sets the `actor` property to the logged in user by default
and you will rarely, if ever, need to override it.

For reference though, here is a basic actor object as it might appear on Apex:

\code{.js}
"actor": { 
    "mbox": "mailto:jandoe@pixovr.com", 
    "name": "Jane Doe" 
}
\endcode

At its simplest, an actor object only requires an mbox property, however because we typically have
user name information available, we generally include that in the statement we generate for Apex as
well.

## Verb

The `verb` in an xAPI Statement describes the action the `actor` took, and is usually in the past tense.
You can choose any `verb` you like, though it's worth it to spend some time considering how you want
to implement verbs across your library of modules. For example if you have several modules where a
user needs to pick up a tool, it's more consistent if all of those modules used `picked_up` as the
verb, as opposed to one using `picked_up`, another using `grabbed` and a third using `took`.

The xAPI specification dictates that you use Internationalized Resource Identifiers, or IRIs when
defining the id property of your verbs, though this is not enforced by Apex. You can learn more about
IRIs at the <a href='https://www.devlinpeck.com/content/send-custom-xapi-statements'>Devlin Peck</a>
website, or any other in depth tutorial about xAPI.

For the purposes of our example we are reporting that the user (or actor) has chosen a harness, so a
simple xAPI representation of that verb could be:

\code{.js}
"verb":{ 
	"id":"pixovr.com/xapi/verbs/selected"
}
\endcode

## Object

The `object` defines the thing that was acted on. Similar to a `verb`, the ID property of the object
should be an IRI. In our example, the user chose the correct harness. So to represent this, a reasonable
object definition might be:

\code{.js}
"object":{
	"id":"pixovr/xapi/modules/fallpro/correct_harness"
}
\endcode

The object does not have to be physical object. If the user was taking a quiz, you would want to
identify each question of the quiz as a different object. If they are following a step by step process
you would identify each step as a unique object. Or when they are starting and ending sessions, you would
identify the module or scenario itself as the object.

\note You might notice the structure of the example IRIs are different for `verbs` and `objects`. We
recommended that you use `verbs` consistently across projects, but `objects` should be defined per project.
Please feel free contact us if you would like to learn more about our IRI philosophy at PixoVR.

## Context

The `context`, not surprisingly, provides a place to include additional information surrounding the `context`
in which an activity took place. Some of the defined properties for `context` include tracking the platform
on which an activity happens (ie Vive, Quest, iOS, etc) and the version number of the build that is installed.

The `context` also is a place to include custom information about the activity.  For example if you were
building a driving simulation you might want to track the user's top speed and/or average speed over the
course of the test. In our test example we could hypothetically say that there are 3 different models of
equipment rooms that get chosen randomly when you start the module, and you would like to track which one
they loaded into. You could include that or any other custom information in the extensions.

For our basic example, the context property would like like this:

\code{.js}
"context":{
	"platform":"Pico Neo 3",
	/* revision holds the version number of the build */
	"revision":"1.1.0"
	"extensions":{
		"https://www.pixovr.com/xapi/fallpro/extensions/equipmentroom":"A"
	}
}
\endcode 

## Result

The `result` is where you can report how well the actor performed on their task. If the activity was a
question on a quiz you could track what their answer was and how many points they earned on that question.
If the activity was completing an entire module, you could report their total score and how long they spent
in the module. There are also defined properties for if they completed the activity, and if the activity
was a success.

If you want to include scoring information in a result, you do do by including a `score` object, which
itself can contains multiple properties, including:

 - `min` the minimum possible score for this specific result.
 - `max` the maximum possible score for this result.
 - `raw` the actual score the user received for this result.
 - `scaled` the raw score divided by the max score.

A very simple result that captures the information in our example might look like:

\code{.js}
"result":{
	"success":true
	"score":{
		"min":0
		"max":100
		"raw":85
		"scaled":0.85
	}
}
\endcode

## Timestamp

The timestamp is just the time the statement was generated. The plugin handles recording the correct system
time internally, and you do not need to do anything to include it in any xAPI statement.

## Putting it Together

\code{.js}
"actor": { 
    "mbox": "mailto:jandoe@pixovr.com", 
    "name": "Jane Doe" 
}
"verb":{ 
	"id":"pixovr.com/xapi/verbs/selected"
}
"object":{
	"id":"pixovr/xapi/modules/fallpro/correct_harness"
}
"context":{
	"registration":"ec531277-b57b-4c15-8d91-d292c5b2b8f7",
	"platform":"Pico Neo 3",
	/* revision holds the version number of the build */
	"revision":"1.1.0",
	"extensions":{
	
	}
}
"result":{
	"success":true,
	"score":{
		"min":0,
		"max":100,
		"raw":85,
		"scaled":0.85
	}
}
"timestamp": "2015-11-18T12:17:00+00:00"
\endcode

## Going Further

There are even more possibilities to structuring xAPI data than the Apex SDK supports. We have tried
to find a balance between flexibility and ease of use and adoption. However if you find that there is a
particular feature of the xAPI specification that you need or want to include that is not available in the
current release of the Apex SDK, please reach out to us and we will work with you to figure out how to
proceed.
