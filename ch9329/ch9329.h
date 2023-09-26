#include <glib.h>

struct key_map {
    char *key;
    unsigned char code;
};

#define PROTO_HEAD 0xAB57

#define CMD_GET_INFO 0x01
#define CMD_SEND_KB_GENERAL_DATA 0x02
#define CMD_SEND_KB_MEDIA_DATA 0x03
#define CMD_SEND_MS_ABS_DATA 0x04
#define CMD_SEND_MS_REL_DATA 0x05
#define CMD_SEND_MY_HID_DATA 0x06
#define CMD_READ_MY_HID_DATA 0x07
#define CMD_GET_PARA_CFG 0x08
#define CMD_SET_PARA_CFG 0x09
#define CMD_GET_USB_STRING 0x0A
#define CMD_SET_USB_STRING 0x0B
#define CMD_SET_DEFAULT_CFG 0x0C
#define CMD_RESET 0x0F

/* cmd data length */
#define GET_INFO_LEN 0x00
#define REPLY_GET_INFO_LEN 0x08

#define SEND_KB_GENERAL_DATA_LEN 0x08
#define REPLY_SEND_KB_GENERAL_DATA_LEN 0x01

#define SEND_KB_MEDIA_DATA_LEN 0x02
#define REPLY_SEND_KB_MEDIA_DATA_LEN 0x01

#define SEND_MS_ABS_DATA_LEN 0x07
#define REPLY_SEND_MS_ABS_DATA_LEN 0x01

#define SEND_MS_REL_DATA_LEN 0x05
#define REPLY_SEND_MS_REL_DATA_LEN 0x01

#define SEND_MY_HID_DATA_LEN(n) n
#define REPLY_SEND_MY_HID_DATA_LEN 0x01

// TODO
#define READ_MY_HID_DATA_LEN(n) n
#define REPLY_READ_MY_HID_DATA_LEN 0x01

#define GET_PARA_CFG_LEN 0x00
#define REPLY_GET_PARA_CFG_LEN 0x32

#define SET_PARA_CFG_LEN 0x32
#define REPLY_SET_PARA_CFG_LEN 0x01

#define GET_USB_STRING_LEN 0x01
#define REPLY_GET_USB_STRING (0x02 + 0x23)

#define SET_USB_STRING_LEN(n) (0x02 + n)
#define REPLY_SET_USB_STRING_LEN 0x01

#define SET_DEFAULT_CFG_LEN 0x00
#define REPLY_SET_DEFAULT_CFG_LEN 0x01

#define RESET_LEN 0x00
#define REPLY_RESET_LEN 0x01

#define CMD_LEN(C) C##_LEN
#define CMD_REPLY_LEN(C) REPLY_##C##_LEN

#define TYPE_CMD(C) \
struct cmd_##C { \
    unsigned short head; \
    unsigned char addr; \
    unsigned char cmd; \
    unsigned char len; \
    unsigned char data[CMD_LEN(C)]; \
    unsigned char sum; \
};

#define TYPE_CMD_REPLY(C) \
struct cmd_reply_##C { \
    unsigned short head; \
    unsigned char addr; \
    unsigned char cmd; \
    unsigned char len; \
    unsigned char data[CMD_REPLY_LEN(C)]; \
    unsigned char sum; \
};

#define PKT(C) cmd_##C##_pkt
#define PKTP(C) cmd_##C##_pkt_ptr
#define PKTR(C) cmd_##C##_reply_pkt
#define DECLARE_CMD(C) struct cmd_##C PKT(C)
#define DECLARE_CMD_PTR(C) struct cmd_##C * PKTP(C)
#define DECLARE_CMD_REPLY(C) struct cmd_reply_##C PKTR(C)

#pragma pack(1)
TYPE_CMD(GET_INFO);
TYPE_CMD(RESET);
TYPE_CMD(SEND_KB_GENERAL_DATA);
TYPE_CMD(SEND_MS_REL_DATA);
TYPE_CMD(SEND_MS_ABS_DATA);

TYPE_CMD_REPLY(GET_INFO);
TYPE_CMD_REPLY(RESET);
TYPE_CMD_REPLY(SEND_KB_GENERAL_DATA);
TYPE_CMD_REPLY(SEND_MS_REL_DATA);
TYPE_CMD_REPLY(SEND_MS_ABS_DATA);

struct cmd_send_kb {
    unsigned short head;
    unsigned char addr;
    unsigned char cmd;
    unsigned char len;
    unsigned char data[8];
    unsigned char sum;
};

struct cmd_send_ms_rel {
    unsigned short head;
    unsigned char addr;
    unsigned char cmd;
    unsigned char len;
    unsigned char data[5];
    unsigned char sum;
};

struct cmd_general_reply {
    unsigned short head;
    unsigned char addr;
    unsigned char cmd;
    unsigned char len;
    unsigned char data;
    unsigned char sum;
};

struct cmd_get_info_reply {
    unsigned short head;
    unsigned char addr;
    unsigned char cmd;
    unsigned char len;
    unsigned char data[8];
    unsigned char sum;
};
#pragma pack()


enum CTRL_KEY {
    CTRL_KEY_NONE = 0,
    LEFT_CTRL = 1 << 0,
    LEFT_SHIFT =  1<< 1,
    LEFT_ALT = 1 << 2,
    LEFT_WIN = 1 << 3,
    RIGHT_CTRL = 1 << 4,
    RIGHT_SHIFT = 1 << 5,
    RIGHT_ALT = 1 << 6,
    RIGHT_WIN = 1 << 7,
    CTRL_KEY_MAX = 8,
};

enum MOUSE_BTN {
    MOUSE_BTN_NONE = 0,
    LEFT_BUTTON = 1 << 0,
    RIGHT_BUTTON = 1 << 1,
    MID_BUTTON = 1 << 2,
};

enum USB_INFO {
    USB_REC_ERROR = 0,
    USB_REC_OK
};

enum KBD_LOCK_INFO {
    NUM_LOCK_ON = 1 << 0,
    CAPS_LOCK_ON = 1 << 1,
    SCROLL_LOCK_ON = 1 << 2
};

/* get info */
int get_conn_info(int fd);

/* reset chip */
int reset_chip(int fd);

/* Keyboard */
unsigned char lookup_key_code(char *key);
int send_key_down(char *key, int fd);
int send_key_up(char *key, int fd);
int send_key_mod(char *key, enum CTRL_KEY mod, int pressed, int fd);

/* Mouse */
int send_mouse_click_down(int fd, int x, int y, enum MOUSE_BTN button);
int send_mouse_click_up(int fd, int x, int y);
int send_mouse_move(int fd, int x, int y);
int send_mouse_move_abs(int fd, int h, int w, int x, int y);

int ch9329_init(void);
