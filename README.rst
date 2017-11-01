node-mifare
===========

This package is a wrapper around the desfire capability of libfreefare with pcsc as backend.
So it enables you to read and write Mifare DESFire EV1 Tags from Windows, Linux and Mac OS X.


Installaton
-----------

.. code-block:: bash

   npm install --save git+https://github.com/embeddedfactor/node-mifare.git


Usage
-----

.. code-block:: javascript

   mifare = require("node-mifare");

   // The readers can be read multiple times
   // Each time the running listen functions of used readers are disabled and destroyed.
   readers = mifare.getReaders();
   reader = readers[first(readers)];

   // Get reader name
   console.log(reader.name);

   // Listen for tag on reader
   reader.listen(function(err, reader, tag) {
     // err is a list of error objects
     // an error object contains a position code 'code',
     // an phase name 'msg' in which the error occured,
     // an error number 'res' and
     // an internal error message msg2.
     // reader is a reference to the original reader object
     // tag is an instance representating the tag on the reader.
   });

The card object has the following functions:

:setKey(key, type, x, id):
:info():
:writeNdef(buffer):
