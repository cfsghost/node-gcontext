node-gcontext
=============

node-gcontext is an event loop integration between libuv and GLib, to make that GLib event loop works with Node.js event engine.

It makes many libraries(GTK+, DBus, Clutter...etc) which are using GLib event loop to be able to work on Node.js.

How To Use
-
Start a GLib main context loop in Node.js:

    var GContext = require('gcontext');
    GContext.init();

Then you can stop it:

    GContext.uinit();

Installation
-
Using NPM utility to install module directly:

    npm install gcontext

License
-
Licensed under the MIT License

Authors
-
Copyright(c) 2012 Fred Chien <<cfsghost@gmail.com>>
