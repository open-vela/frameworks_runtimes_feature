let console = require('console');
let record = require('Record');

record.start(200000, 16000, 2, 16000, 'aac',
  function(uri) {console.log(uri);},
  function(code, errorMsg) {console.log(uri, errorMsg);},
  function() {console.log('record started!');}
);
record.stop()

record.start(200000, 16000, 2, 16000, 'aac');
record.stop()