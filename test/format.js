var mifare = require("../index.js");
var ndef = require("ndef");

function first(obj) {
    for (var a in obj) {
        return a;
    }
}

var readers = mifare.getReader();
var idx=0;

function delay(ms) {
  var cur_d = new Date();
  var cur_ticks = cur_d.getTime();
  var ms_passed = 0;
  while(ms_passed < ms) {
    var d = new Date();  // Possible memory leak?
    var ticks = d.getTime();
    ms_passed = ticks - cur_ticks;
    // d = null;  // Prevent memory leak?
  }
}

/*
# called from the gui
# format a card and write data to it
formatCard = (readerName, uid, data, cb) ->
  err = null
  card = checkCard(uid)
  if not card
    emitCardLost uid, card, 'formatCard: did not find card to read'
    err = 'did not find card to read'
  else
    buf = new Buffer data
    console.log '[CardReader] calling card.format'
    resf = card.format()
    resc = card.createNdef()
    resw = card.writeNdef buf
    console.warn '[CardReader] formatted', resf, resc, resw, typeof card.createNdef
    queue readCard, readerName, uid
  cb?(err)
  return
*/

reader = readers[first(readers)];
console.log("Waiting for a cart to format on Reader '"+reader.name+"'");
reader.listen(function(err, reader, card) {
//readers["ACS ACR122 0"].listen(function(err, reader, card) {
//readers["ACS ACR122U PICC Interface"].listen(function(err, reader, card) {
  if(!card) {
    return;
  }
  console.log('Info', card.info())
  console.log('NDEF', card.readNdef())
  console.log('MasterKey', card.masterKeyInfo());
  console.log('Key 0', card.keyVersion(0));
  //console.log('Format', card.format());
  //console.log('', card.createNdef()
  console.log('Read', card.readNdef())

});

