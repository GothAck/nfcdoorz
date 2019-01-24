import board
import microcontroller
import busio
import digitalio
import adafruit_dotstar

pixels = adafruit_dotstar.DotStar(board.APA102_SCK, board.APA102_MOSI, 1)

led = digitalio.DigitalInOut(board.D13)
led.direction = digitalio.Direction.OUTPUT

interrupt = digitalio.DigitalInOut(board.D1)
interrupt.switch_to_input(pull=digitalio.Pull.UP)



uart = busio.UART(board.TX, board.RX, baudrate=115200)
i2c = busio.I2C(board.SCL, board.SDA)

DEFAULT_HEADER = bytearray([0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00])

class PKTData(object):
    begin_write_fn = None
    end_write_fn = None
    def __init__(self, debug=False):
        self.debug = debug
        self.reset()

    # def print(self, *args, **kwargs):
    #     if "override" in kwargs:
    #         del kwargs["override"]
    #         print(dir(self))
    #         print(self.__qualname__, *args, **kwargs)
    #     else:
    #         self.debug and print(self.__qualname__, *args, **kwargs)

    def reset(self):
        self.len_pkt = None
        self.header_len = None
        self.header_index = 0
        self.prev_preamble = False
        self.is_extended = False
        self.header = list(DEFAULT_HEADER)
        self.buffer = None
        self.unit_command = None

    def _proxyByte(self, byte):
        if not self.buffer:
            self.buffer = bytearray([0] * self.len_pkt)
            self.begin_write_fn and self.begin_write_fn(self.len_pkt)
        # print("buffering", self.name, self.header_len - self.len_pkt, self.header_len, self.len_pkt, self.buffer)
        # self.print("buffering", self.header_len - self.len_pkt, self.len_pkt, self.header_len, self.buffer)
        self.buffer[self.header_len - self.len_pkt] = byte
        self.len_pkt -= 1
        # self.write_fn(byte, self.len_pkt == 0)
        # self.print("len_pkt--", self.len_pkt)
        if self.len_pkt <= 0 and self.write_fn:
            header = bytearray(self.header[0:(8 if self.is_extended else 5)])
            self.write_fn(header + self.buffer, True)
            self.end_write_fn and self.end_write_fn()
            self.reset()
            return True

    def _searchPreambleStart(self, byte):
        # self.print("header?")
        if byte == 0x00:
            # self.print("preamble")
            self.prev_preamble = True
            return
        elif byte == 0xff and self.prev_preamble:
            # self.print("header start")
            self.header_index = -2
            self.is_extended = False
        elif byte == 0xee and self.prev_preamble:
            print("Uc")
            self.unit_command = 4
        else:
            pass
            # self.print("not preamble")
        self.prev_preamble = False

    def _decodeStandardHeader(self, byte):
        if self.header_index == -2:
            # self.print("header 0")
            self.header[3] = byte
            self.header_len = byte + 2
            self.header_index += 1
        elif self.header_index == -1:
            # self.print("header 1")
            self.header[4] = byte
            self.header_index = 0
            if self.header_len == 2 and byte == 0xff:
                # self.print("ack")
                self.header_len = 1
                self.len_pkt = self.header_len
                self.header_index += 1
            elif self.header_len == (0xff + 2) and byte == 0x00:
                # self.print("nack")
                self.header_len = 1
                self.len_pkt = self.header_len
                self.header_index += 1
            elif self.header_len == (0xff + 1) and byte == 0xff:
                # self.print("extended")
                self.is_extended = True
                self.header_index = -3
            else:
                # self.print("header 2 len:", self.header_len)
                if (byte + (self.header_len - 2)) % 256:
                    # self.print("Packet len csum fail")
                    self.reset()
                else:
                    self.len_pkt = self.header_len

    def _decodeExtendedHeader(self, byte):
        if self.header_index == -3:
            self.header[5] = byte
            self.header_index += 1
            self.header_len = byte << 8
        elif self.header_index == -2:
            self.header[6] = byte
            self.header_index += 1
            self.header_len |= byte
            self.header_len += 2
        elif self.header_index == -1:
            self.header[7] = byte
            if (byte + (self.header_len >> 8) + ((self.header_len & 0xff) - 2)) % 256:
                # self.print("Packet len csum fail")
                self.reset()
            else:
                self.len_pkt = self.header_len

    def _unitCommand(self, byte):
        print("Unit command", self.unit_command)
        self.header[4 + (4 - self.unit_command)] = byte
        self.unit_command -= 1
        if not self.unit_command:
            if self.header[4] == 0x01:
                print(self.header[5:8])
                pixels.fill(self.header[5:8])
                pixels.show()


    def _decodeHeader(self, byte):
        if not (self.header_index or self.unit_command):
            return self._searchPreambleStart(byte)
        if self.unit_command:
            return self._unitCommand(byte)
        if not self.is_extended:
            return self._decodeStandardHeader(byte)
        return self._decodeExtendedHeader(byte)

    def addByte(self, byte):
        # self.print("byte", hex(byte))
        if self.len_pkt is not None:
            return self._proxyByte(byte)
        elif self.header_index < 0 or self.header_len is None:
            return self._decodeHeader(byte)
        return False

    def popByte(self):
        if (self.end == self.start): return None
        byte = self.buffer[self.start]
        self.start = (self.start + 1) % 255

    def buffer_len(self):
         return self.end - self.start

class I2CData(PKTData):
    def write_fn(self, buffer, last=False):
        # self.print("ser_write", len(buffer), last, buffer)
        uart.write(buffer)

class SerData(PKTData):
    def begin_write_fn(self, bytes):
        # self.print("i2c_begin_write")
        while not i2c.try_lock():
            pass

    def write_fn(self, buffer, last=False):
        # self.print("i2c_write", len(buffer), last, buffer)
        try:
            i2c.writeto(0x24, buffer)
        except OSError as e:
            print("OSError", e)

    def end_write_fn(self):
        # self.print("i2c_end_write")
        i2c.unlock()

i2c_data = I2CData()
ser_data = SerData()
i2c_buffer = bytearray([0] * 256)

def main():
    while True:
        led.value = False
        if uart.in_waiting:
            byte = uart.read(1)
            if byte is not None:
                ser_data.addByte(byte[0])
        if not interrupt.value:
            led.value = True
            if not i2c.try_lock():
                continue
            try:
                i2c.readfrom_into(0x24, i2c_buffer)
            except OSError as e:
                print("OSError", e)
                continue
            finally:
                i2c.unlock()
            for i, c in enumerate(i2c_buffer[1:]):
                if i2c_data.addByte(c):
                    break

if __name__ == "__main__":
    main()
