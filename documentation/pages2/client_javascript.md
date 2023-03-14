
\page client_javascript The Javascript Client

The client source code can be found here: 

`git clone ssh://git@gitlab.eyelash.ai:2222/vfs/clients/vfs_client_javascript.git`

The javascript client will require a webserver like Apache or Nginx.  You may wish to proxy
websockets: \ref nginx if your environment requires it.

The javascript client will require a configuration object.  The preferred method is to create a
`config.json` file next to index.html, and `config.js` will load and parse it.

An alternative method is to set a window.VFS_config variable in index.html (or elsewhere
in javascript) before the client is loaded:

\code{.js}
window.VFS_config = {
                       "ws":   {  "applicationName":"[Remoto]",           // appears in the tab title
                                  "host":"[hostname]",                    // address to connect to
                                  "path":"[/path/to/websocket/service]",  // if using an http proxy, or empty
                                  "port":"[port]",                        // non-ssl port
                                  "type":"browser",                       // session type
                                  "version":"[version]",                  // pretty much just cosmetic, if we ever want to show this to the user
                                  "username":"[username]",                // useful for nopasswd for instance, like for a game that doesn't need a login but does need a default user
                                  "password":"[password]",                // don't want to use this without ssl
                                  "authtoken":                            // authtoken data for persistent sessions across page reloads
                                  {
                                    "key": "VFS_AUTHTOKEN",               // or override to something else
                                    "path": [current path],               // or override to something else
                                    "domain": [current FQDN],             // could be made less specific as an override (like domain.com instead of portal.domain.com)
                                    "secure": [current protocol],         // should be 'false' when not using ssl
                                    "samesite": "strict",                 // [no_restriction|none|lax|strict] this should basically always be "strict"
                                    "token": [cookie value]               // can be hardcoded, but you probably want this undefined so it uses the cookie value
                                  }
                                  "verboseLoader": 3,                     // [0-4], defaults to 0 if missing
                                  "logoutURL": "./",                      // The url a user will be directed to on logout
                                  "showSwitchUser": true,                 // show, or don't the "Swith User" button
                               },
                       "wss":  {  "applicationName":"[Remoto]",           // appears in the tab title
                                  "host":"[hostname]",                    // address to connect to
                                  "path":"[/path/to/websocket/service]",  // if using an https proxy, or empty
                                  "port":"[port]",                        // ssl port
                                  "type":"browser",                       // session type
                                  "version":"[version]",                  // pretty much just cosmetic, if we ever want to show this to the user
                                  "username":"[username]",                // useful for nopasswd for instance, like for a game that doesn't need a login but does need a default user
                                  "password":"[password]",                // don't want to use this without ssl
                                  "authtoken":                            // authtoken data for persistent sessions across page reloads
                                  {
                                    "key": "VFS_AUTHTOKEN",               // or override to something else
                                    "path": [current path],               // or override to something else
                                    "domain": [current FQDN],             // could be made less specific as an override (like domain.com instead of portal.domain.com)
                                    "secure": [current protocol],         // could be downgraded with 'false'
                                    "samesite": "strict",                 // [no_restriction|none|lax|strict] this should basically always be "strict"
                                    "token": [cookie value]               // can be hardcoded, but you probably want this undefined so it uses the cookie value
                                  }
                                  "verboseLoader": 3,                     // [0-4], defaults to 0 if missing
                                  "logoutURL": "./",                      // The url a user will be directed to on logout
                                  "showSwitchUser": true,                 // show, or don't the "Swith User" button
                               },
                    };
\endcode

Only the procotol(s) you are using need to be included in the config object.  All entries are optional
except for `host` and `port`.

\sa VFS_session::registerSessionType()
\sa VFS_remotoserver::VFS_remotoserver()
\sa \ref nginx

The Javascript Client is a bootloader that will establish a websocket connection to the server and
use the VFS_stdlib javascript libraries to draw the interface and service user interaction.

The bootloader libraries could be used with different css or javascript libraries to create a custom application.

\sa javascript
\sa \ref themes_javascript

The default client will load a user's preferences, theme, and layout file(s), and create a vfsClient 
instance for managing a user session.

If the verboseLoader url parameter is present and set to "true", all messages shown in the loader modal
will echo to the console.

\sa vfsClient
