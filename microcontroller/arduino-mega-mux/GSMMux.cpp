#include "GSMMux.h"
#include "SoftwareSerial.h"

#define SOF 0xF9
#define SOF_2 0x7E
#define PF_MASK 0b00010000
#define TYPE_SABM 0b00101111
#define TYPE_UA 0b01100011
#define TYPE_DM 0b00001111
#define TYPE_DISC 0b01000011
#define TYPE_UIH 0b11101111

#if DEBUG
  #define GSMMUX_DEBUG(...) _debug_stream && _debug_stream->print(__VA_ARGS__)
  #define GSMMUX_DEBUGF(...) _debug_stream && _debug_stream->printf(__VA_ARGS__)
  #define GSMMUX_DEBUGLN(...) _debug_stream && _debug_stream->println(__VA_ARGS__)
#else
  #define GSMMUX_DEBUG(...)
  #define GSMMUX_DEBUGF(...)
  #define GSMMUX_DEBUGLN(...)
#endif

static const uint8_t gsm_fcs8[256] = {
  0x00, 0x91, 0xE3, 0x72, 0x07, 0x96, 0xE4, 0x75,
  0x0E, 0x9F, 0xED, 0x7C, 0x09, 0x98, 0xEA, 0x7B,
  0x1C, 0x8D, 0xFF, 0x6E, 0x1B, 0x8A, 0xF8, 0x69,
  0x12, 0x83, 0xF1, 0x60, 0x15, 0x84, 0xF6, 0x67,
  0x38, 0xA9, 0xDB, 0x4A, 0x3F, 0xAE, 0xDC, 0x4D,
  0x36, 0xA7, 0xD5, 0x44, 0x31, 0xA0, 0xD2, 0x43,
  0x24, 0xB5, 0xC7, 0x56, 0x23, 0xB2, 0xC0, 0x51,
  0x2A, 0xBB, 0xC9, 0x58, 0x2D, 0xBC, 0xCE, 0x5F,
  0x70, 0xE1, 0x93, 0x02, 0x77, 0xE6, 0x94, 0x05,
  0x7E, 0xEF, 0x9D, 0x0C, 0x79, 0xE8, 0x9A, 0x0B,
  0x6C, 0xFD, 0x8F, 0x1E, 0x6B, 0xFA, 0x88, 0x19,
  0x62, 0xF3, 0x81, 0x10, 0x65, 0xF4, 0x86, 0x17,
  0x48, 0xD9, 0xAB, 0x3A, 0x4F, 0xDE, 0xAC, 0x3D,
  0x46, 0xD7, 0xA5, 0x34, 0x41, 0xD0, 0xA2, 0x33,
  0x54, 0xC5, 0xB7, 0x26, 0x53, 0xC2, 0xB0, 0x21,
  0x5A, 0xCB, 0xB9, 0x28, 0x5D, 0xCC, 0xBE, 0x2F,
  0xE0, 0x71, 0x03, 0x92, 0xE7, 0x76, 0x04, 0x95,
  0xEE, 0x7F, 0x0D, 0x9C, 0xE9, 0x78, 0x0A, 0x9B,
  0xFC, 0x6D, 0x1F, 0x8E, 0xFB, 0x6A, 0x18, 0x89,
  0xF2, 0x63, 0x11, 0x80, 0xF5, 0x64, 0x16, 0x87,
  0xD8, 0x49, 0x3B, 0xAA, 0xDF, 0x4E, 0x3C, 0xAD,
  0xD6, 0x47, 0x35, 0xA4, 0xD1, 0x40, 0x32, 0xA3,
  0xC4, 0x55, 0x27, 0xB6, 0xC3, 0x52, 0x20, 0xB1,
  0xCA, 0x5B, 0x29, 0xB8, 0xCD, 0x5C, 0x2E, 0xBF,
  0x90, 0x01, 0x73, 0xE2, 0x97, 0x06, 0x74, 0xE5,
  0x9E, 0x0F, 0x7D, 0xEC, 0x99, 0x08, 0x7A, 0xEB,
  0x8C, 0x1D, 0x6F, 0xFE, 0x8B, 0x1A, 0x68, 0xF9,
  0x82, 0x13, 0x61, 0xF0, 0x85, 0x14, 0x66, 0xF7,
  0xA8, 0x39, 0x4B, 0xDA, 0xAF, 0x3E, 0x4C, 0xDD,
  0xA6, 0x37, 0x45, 0xD4, 0xA1, 0x30, 0x42, 0xD3,
  0xB4, 0x25, 0x57, 0xC6, 0xB3, 0x22, 0x50, 0xC1,
  0xBA, 0x2B, 0x59, 0xC8, 0xBD, 0x2C, 0x5E, 0xCF
};

static inline uint8_t gsm_fcs_add_block(uint8_t *c, int len)
{
  uint8_t fcs = 0xff;
  while (len--)
    fcs = gsm_fcs8[fcs ^ *c++];
  return fcs;
}

void GSMMux::charReceiver(uint8_t data) {
  if (sof_character) {
    switch(frame_position++) {
      case 0:
        rx_msg.addr = data;
        break;
      case 1:
        rx_msg.ctrl = data;
        break;
      case 2:
        rx_msg.len[0] = data;
        rx_msg.cur = data >> 1;
        rx_msg.length = rx_msg.cur;
        break;
      case 3:
        if (!(rx_msg.len[0] & 1)) {
          rx_msg.len[1] = data;
          rx_msg.cur |= ((uint16_t)data) << 8;
          rx_msg.length = rx_msg.cur;
          break;
        }
      default:
        if (rx_msg.cur >= 0) {
          rx_msg.buffer[rx_msg.length - (rx_msg.cur--)] = data;
        } else {
          if (data != sof_character) {
            sof_character = 0;
            GSMMUX_DEBUGLN("Missing end escape char");
          } else {
            GSMMUX_DEBUGLN("End frame");
            sof_character = 0;
            frame_position = 0;
            frameDecode(rx_msg);
          }
        }
        break;
    }
  } else if (data == SOF) {
    GSMMUX_DEBUGLN("Start frame");
    sof_character = data;
    frame_position = 0;
  }
}

void GSMMux::frameDecode(struct gsm_msg &msg) {
  uint8_t fcs_blk[] = {msg.addr, msg.ctrl, msg.len[0], msg.len[1]};
  uint8_t fcs = msg.buffer[msg.length];
  uint8_t calc_fcs = 0xff - gsm_fcs_add_block(fcs_blk, msg.len[0] & 1 ? 3 : 4);
  if (fcs != calc_fcs) {
    GSMMUX_DEBUGLN("Failed crc");
    GSMMUX_DEBUG(fcs, HEX);
    GSMMUX_DEBUG(" != ");
    GSMMUX_DEBUGLN(calc_fcs, HEX);
    return;
  }

  switch(msg.ctrl & (~PF_MASK)) {
    case TYPE_SABM:
      GSMMUX_DEBUGLN("SABM");
      chans |= 1 << (msg.addr >> 2);
      write_frame_buffer[0] = SOF;
      write_frame_buffer[1] = msg.addr;
      write_frame_buffer[2] = TYPE_UA | PF_MASK;
      write_frame_buffer[3] = 0;
      write_frame_buffer[4] = 0;
      write_frame_buffer[5] = 0xff - gsm_fcs_add_block(&write_frame_buffer[1], 4);
      write_frame_buffer[6] = SOF;
      _sendbuffer(write_frame_buffer, 7);
      break;
    case TYPE_UA:
      GSMMUX_DEBUGLN("UA");
      break;
    case TYPE_DM:
      GSMMUX_DEBUGLN("DM");
      break;
    case TYPE_DISC:
      GSMMUX_DEBUGLN("DISC");
      if (chans & (1 << (msg.addr >> 2))) {
        chans &= ~(1 << (msg.addr >> 2));
        write_frame_buffer[0] = SOF;
        write_frame_buffer[1] = msg.addr;
        write_frame_buffer[2] = TYPE_UA | PF_MASK;
        write_frame_buffer[3] = (0 << 1) | 1; // Len
        write_frame_buffer[4] = 0xff - gsm_fcs_add_block(&write_frame_buffer[1], 3);
        write_frame_buffer[5] = SOF;
        _sendbuffer(write_frame_buffer, 6);
      } else {
        write_frame_buffer[0] = SOF;
        write_frame_buffer[1] = msg.addr;
        write_frame_buffer[2] = TYPE_DM | PF_MASK;
        write_frame_buffer[3] = (0 << 1) | 1;
        write_frame_buffer[4] = 0xff - gsm_fcs_add_block(&write_frame_buffer[1], 3);
        write_frame_buffer[5] = SOF;
        _sendbuffer(write_frame_buffer, 6);
      }
      break;
    case TYPE_UIH:
      GSMMUX_DEBUGLN("UIH");
      if ((msg.addr >> 2) == 0) {
        handleControlUIH(msg);
      } else {
        _frame_handler(msg);
      }
      break;
  }
}

void GSMMux::handleControlUIH(struct gsm_msg &msg) {
  if (msg.len < 3) {
    GSMMUX_DEBUGLN("handleControlUIH too short");
    return;
  }
  uint16_t seq = 0;
  uint32_t type = msg.buffer[seq++];
  bool control_cr = !!(type & 2);
  bool control_ea = type & 1;
  type >>= 2;
  if (control_ea != 1) {
    GSMMUX_DEBUGLN("handleControlUIH too short");
    return;
  }
  uint32_t len = msg.buffer[seq++];
  if (!(len & 1)) {
    len |= msg.buffer[seq++] << 8;
  }
  len >>= 1;

  uint8_t *dataptr = &msg.buffer[seq];

  switch(type) {
    case 0b010000:
      GSMMUX_DEBUGLN("PSC");
      break;
    case 0b110000:
      GSMMUX_DEBUGLN("CLD");
      break;
    case 0b001000:
      GSMMUX_DEBUGLN("Test");
      break;
    case 0b111000:
      GSMMUX_DEBUGLN("MSC");
      GSMMUX_DEBUGLN(len, DEC);
      write_frame_buffer[0] = SOF;
      write_frame_buffer[1] = 3;
      write_frame_buffer[2] = TYPE_UIH;
      write_frame_buffer[3] = (4 << 1) | 1;
      write_frame_buffer[4] = (0b111000 << 2) | 1;
      write_frame_buffer[5] = (2 << 1) | 1;
      write_frame_buffer[6] = dataptr[0];
      chan_v24[dataptr[0] >> 2] = dataptr[1];
      write_frame_buffer[7] = chan_v24[dataptr[0] >> 2];
      write_frame_buffer[8] = 0xff - gsm_fcs_add_block(&write_frame_buffer[1], 3);
      write_frame_buffer[9] = SOF;
      _sendbuffer(write_frame_buffer, 10);
      break;
    case 0b011000:
      GSMMUX_DEBUGLN("FCoff");
      break;
    case 0b101000:
      GSMMUX_DEBUGLN("FCon");
      break;
  }

}

void GSMMux::sendBuffer(uint8_t i, uint8_t *data, uint32_t len) {
  if (i > 0 && i < 5 && (chans & (1 << i)) && len < 64) {
    uint8_t seq = 0;
    write_frame_buffer[seq++] = SOF;
    write_frame_buffer[seq++] = (i << 2) | 3;
    write_frame_buffer[seq++] = TYPE_UIH;
    write_frame_buffer[seq++] = (len << 1) | (len <= 127);
    if (len > 127)
      write_frame_buffer[seq++] = (len >> 7) | 1;

    memcpy(&write_frame_buffer[seq], data, len);
    seq += len;

    write_frame_buffer[seq++] = 0xff - gsm_fcs_add_block(&write_frame_buffer[1], 3 + (len > 127));
    write_frame_buffer[seq++] = SOF;
    _sendbuffer(write_frame_buffer, seq);
  }
}

#if DEBUG
void GSMMux::printGSMMsg(struct gsm_msg &msg) {
  GSMMUX_DEBUGLN("struct gsm_msg {");
  //  struct list_head list;
  GSMMUX_DEBUG("  uint8_t addr = 0x");
  GSMMUX_DEBUGLN(msg.addr, HEX);
  GSMMUX_DEBUG("  uint8_t ctrl = 0x");
  GSMMUX_DEBUGLN(msg.ctrl, HEX);
  GSMMUX_DEBUG("  uint8_t uih_data = 0x");
  GSMMUX_DEBUGLN(msg.uih_data, HEX);
  GSMMUX_DEBUG("  uint8_t len[2] = {0x");
  GSMMUX_DEBUG(msg.len[0], HEX);
  GSMMUX_DEBUG(", 0x");
  GSMMUX_DEBUG(msg.len[1], HEX);
  GSMMUX_DEBUGLN("}");
  GSMMUX_DEBUG("  uint16_t length = ");
  GSMMUX_DEBUGLN(msg.length, DEC);
  GSMMUX_DEBUG("  uint8_t[");
  GSMMUX_DEBUG(msg.length, DEC);
  GSMMUX_DEBUGLN("] length = {");
  for(uint16_t i = 0; i < msg.length; i++) {
    GSMMUX_DEBUG("    0x");
    GSMMUX_DEBUG(msg.buffer[i]);
    GSMMUX_DEBUGLN(i < msg.length - 1 ? "," : "");
  }
  GSMMUX_DEBUGLN("  }");
  GSMMUX_DEBUGLN("}");
}
#else
void GSMMux::printGSMMsg(struct gsm_msg &msg) {}
#endif
