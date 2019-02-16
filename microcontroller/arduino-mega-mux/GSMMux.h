#include <Arduino.h>

#ifndef DEBUG
  #define DEBUG 0
#endif

#define GSM_SEARCH    0
#define GSM_START   1
#define GSM_ADDRESS   2
#define GSM_CONTROL   3
#define GSM_LEN     4
#define GSM_DATA    5
#define GSM_FCS     6
#define GSM_OVERRUN   7
#define GSM_LEN0    8
#define GSM_LEN1    9
#define GSM_SSOF    10


#define CR      0x02
#define EA      0x01
#define PF      0x10

/* I is special: the rest are ..*/
#define RR      0x01
#define UI      0x03
#define RNR     0x05
#define REJ     0x09
#define DM      0x0F
#define SABM      0x2F
#define DISC      0x43
#define UA      0x63
#define UIH     0xEF

/* Channel commands */
#define CMD_NSC     0x09
#define CMD_TEST    0x11
#define CMD_PSC     0x21
#define CMD_RLS     0x29
#define CMD_FCOFF   0x31
#define CMD_PN      0x41
#define CMD_RPN     0x49
#define CMD_FCON    0x51
#define CMD_CLD     0x61
#define CMD_SNC     0x69
#define CMD_MSC     0x71

/* Virtual modem bits */
#define MDM_FC      0x01
#define MDM_RTC     0x02
#define MDM_RTR     0x04
#define MDM_IC      0x20
#define MDM_DV      0x40

#define GSM0_SOF    0xF9
#define GSM1_SOF    0x7E
#define GSM1_ESCAPE   0x7D
#define GSM1_ESCAPE_BITS  0x20
#define XON     0x11
#define XOFF      0x13

#define INIT_FCS  0xFF
#define GOOD_FCS  0xCF

struct gsm_msg {
//  struct list_head list;
  uint8_t addr;    /* DLCI address + flags */
  uint8_t ctrl;    /* Control byte + flags */
  bool uih_data;
  uint8_t len[2]; /* Length of data block (can be zero) */
  uint16_t length;
  int16_t cur; /* Length of data block (can be zero) */
  unsigned char buffer[255];
};


struct gsm_control {
  uint8_t cmd;   /* Command we are issuing */
  uint8_t *data; /* Data for the command in case we retransmit */
  int len;  /* Length of block for retransmission */
  int done; /* Done flag */
  int error;  /* Error if any */
};



typedef void (* sendbuffer_type) (uint8_t*, uint16_t);
typedef void (* frame_handler_type)(struct gsm_msg &msg);

class GSMMux {
#if DEBUG
  Stream *_debug_stream;
#endif
  sendbuffer_type _sendbuffer;
  frame_handler_type _frame_handler;

  uint8_t sof_character = 0;
  struct gsm_msg rx_msg;
  uint8_t write_frame_buffer[255];
  uint8_t frame_position = 0;
  uint8_t chans = 0;
  uint8_t chan_v24[4] = {0b1011, 0b1011, 0b1011, 0b1011};

  void frameDecode(struct gsm_msg &);
  void handleControlUIH(struct gsm_msg &);
public:
  GSMMux(
    sendbuffer_type sendbuffer,
    frame_handler_type frame_handler
  ):
    _sendbuffer(sendbuffer),
    _frame_handler(frame_handler) {}
#if DEBUG
  GSMMux(
    sendbuffer_type sendbuffer,
    frame_handler_type frame_handler,
    Stream &debug_stream
  ):
    _sendbuffer(sendbuffer),
    _frame_handler(frame_handler),
    _debug_stream(&debug_stream) {}
#endif

  void charReceiver(uint8_t data);
  void sendBuffer(uint8_t i, uint8_t *data, uint32_t len);

  void printGSMMsg(struct gsm_msg &msg);
};
