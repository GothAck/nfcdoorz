#!/usr/bin/env node

const WebSocket = require('ws');
const SerialPort  = require('serialport');
const neodoc = require('neodoc');

const args = neodoc.run(`
usage: wsclient.js [options]

options:
    -d <tty>        TTY device to use [default: /dev/ttyGSM3]
    -w <websocket>  Websocket URI [default: ws://localhost:9672/tty]
`);

console.log(`Device ${args['-d']}`);
console.log(`Websocket: ${args['-w']}`);

function connect() {
  const tty = new SerialPort(args['-d']);
  const ws = new WebSocket(args['-w']);

  tty.on('error', (e) => {
    console.error(e);
    process.exit(1);
  });
  ws.on('error', (e) => {
    console.error(e);
    process.exit(1);
  });


  ws.on('message', (data) => {
    if (data instanceof Buffer) {
      console.log(`S: ${data.toString('hex')}`);
      tty.write(data);
    }
  });
  ws.on('close', () => {
    tty.close();
    setTimeout(connect, 1000);
  });
  tty.on('data', (data) => {
    if (data instanceof Buffer) {
      console.log(`T: ${data.toString('hex')}`);
      ws.send(data, {binary: true});
    }
  });
  tty.on('close', () => {
    ws.close();
  });
}

connect();
