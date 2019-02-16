#include "GSMMux.h"

#include "Cth.h"

#include "SoftwareSerial.h"

SoftwareSerial mySerial(22, 23);

void sendbuffer(uint8_t *c, uint16_t len);
void framehandler(struct gsm_msg &msg);

GSMMux mux(sendbuffer, framehandler);

void sendbuffer(uint8_t *c, uint16_t len) {
  Serial.write(c, len);
}

void framehandler(struct gsm_msg &msg) {
  mux.printGSMMsg(msg);
  switch(msg.addr >> 2) {
    case 1:
      for (uint8_t i = 0; i < msg.length; i++) {
        switch(msg.buffer[i]) {
          
        }
      }
      break;
    case 2:
      Serial1.write(msg.buffer, msg.length);
      break;
    case 3:
      Serial2.write(msg.buffer, msg.length);
      break;
    case 4:
      Serial3.write(msg.buffer, msg.length);
      break;
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(230400);
  Serial1.begin(115200);
  Serial2.begin(115200);
  Serial3.begin(115200);
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  Scheduler.startLoop(loop_serial1);
  Scheduler.startLoop(loop_serial2);
  Scheduler.startLoop(loop_serial3);
}

#define BUF_SIZE 64

void loop_serial1() {
  uint8_t i = 0;
  uint8_t buf[BUF_SIZE];
  while (1) {
    i = 0;
    while (Serial1.available() && i < BUF_SIZE) {
      buf[i++] = Serial1.read();
      Scheduler.yield();
    }
    if (i) mux.sendBuffer(2, (uint8_t *)&buf, i);
    Scheduler.yield();
  }
}

void loop_serial2() {
  uint8_t i = 0;
  uint8_t buf[BUF_SIZE];
  while (1) {
    i = 0;
    while (Serial2.available() && i < BUF_SIZE) {
      buf[i++] = Serial2.read();
      Scheduler.yield();
    }
    if (i) mux.sendBuffer(3, (uint8_t *)&buf, i);
    Scheduler.yield();
  }
}

void loop_serial3() {
  uint8_t i = 0;
  uint8_t buf[BUF_SIZE];
  while (1) {
    i = 0;
    while (Serial3.available() && i < BUF_SIZE) {
      buf[i++] = Serial3.read();
      Scheduler.yield();
    }
    if (i) {
      mux.sendBuffer(4, (uint8_t *)&buf, i);
    }
    Scheduler.yield();
  }
}

void loop() {
  while(1) {
    if (Serial.available()) {
      uint8_t c = Serial.read();
      mux.charReceiver(c);
    }
    Scheduler.yield();
  }
}
