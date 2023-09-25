#include <glib.h>

struct key_map {
    char *key;
    unsigned char code;
};

#pragma pack(1)
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

/* Mouse */
int send_mouse_click_down(int fd);
int send_mouse_click_up(int fd);
int send_mouse_move(int fd, int x, int y);
