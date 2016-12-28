var mifare = require("bindings")("node_mifare");
var ndef = require("ndef");

function first(obj) {
    for (var a in obj) {
        return a;
    }
}

var readers = mifare.getReader();
readers = mifare.getReader();
reader = readers[first(readers)];
reader.listen(function(err, reader, card) {
  //card.setKey([0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0], "aes", true, 1);
  //card.setAid(0x542B);
  //console.log(err, reader, card);
  //console.log(card.info());
  //console.log(card.masterKeyInfo());
  //console.log(card.keyVersion(0));
  //console.log(card.freeMemory());
  //var read = card.readNdef();
  //console.log("Read:  ", read.data.ndef.toString('utf8'), read.data.ndef);
  
  if(card) {
    var date = new Date();
    var date_str = date.toString();
    var ndef_msg = [
      ndef.mimeMediaRecord("akraja/test", date_str)
    ];

    var buffer = new Buffer(ndef.encodeMessage(ndef_msg));

    //buffer.write(date_str, 0, date_str.length, 'utf8');
    var write;
    for(var i = 0; i<10; i++) {
      console.log("Write", i);
      write = card.writeNdef(buffer);
      if((write && write.err && write.err.length)) {
        console.log(i, "Error", write)
        return;
      } else {
        console.log(i, "Write", write, buffer)
        break;
      }
    }

    var read;
    for(var i = 0; i<10; i++) {
      read = card.readNdef();
      if(read && read.err && read.err.length) {
        console.log(i, "Error:", read);
        return;
      } else {
        console.log(i, "Read:", read);
      }
    }
  }
  
});

