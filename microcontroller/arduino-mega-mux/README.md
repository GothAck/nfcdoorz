== Arduino Mega GSM-MUX breakout
Arduino code to break out 4 serial ports from one (0 unterminated, to be used for arduino comms; 1-3 terminated on hardware UART 1-3) by utilizing the GSM0710 mux protocol (basic mode only, with all the vendor differences that ended up implemented in the Linux tty line discipline).

Utility program provided to set up the ldisc and create the four ttys.

TODO: Documentation & librarify the Arduino code.
