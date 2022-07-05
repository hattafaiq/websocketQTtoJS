const WebSocket = require('ws');

const ws = new WebSocket('ws://192.168.100.90:1234');

ws.on('open', function open() {
  ws.send('wave1');
});

ws.on('message', function incoming(data) {
 // console.log(data);//ori

//satu data 
// const uint8 = new Uint8Array(data);
// console.log(uint8);

//struct//
var _ = require('c-struct');

var playerSchema = new _.Schema({
  kanal: _.type.uint8,
  data: _.type.Float32Array
});

// register to cache
_.register('Client1', playerSchema);

// buffer to object | this can be on another file
var obj = _.unpackSync('Client1', data);
console.log(obj);

});
