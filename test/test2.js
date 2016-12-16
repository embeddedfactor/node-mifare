var mifare = require("bindings")("node_mifare");
var ndef = require("ndef");

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
//readers["ACS ACR122U PICC Interface 00 00"].listen(function(err, reader, card) {
//readers["ACS ACR122 0"].listen(function(err, reader, card) {
readers["ACS ACR122U PICC Interface"].listen(function(err, reader, card) {
  if(!card) {
    return;
  }
  //console.log("err:", err, ", reader: ", reader, ", card:", card);
  try {
    while(1) {
      var res = card.info();
      console.log(res);
      if(res['err']&&res['err'][0]&&res['err'][0]['res']==0x8010000B) {
        console.log('Error as expected', res['err'][0]);
        throw new Error("Error as expected");
      }
      res = card.readNdef();
      console.log(res);
      if(res['err']&&res['err'][0]&&res['err'][0]['res']==0x8010000B) {
        console.log('Error as expected', res['err'][0]);
        throw new Error("Error as expected");
      }
      delay(500);
      process.stdout.write("+");
    }
  } catch(e) {
    process.stdout.write(".");
  }
});
