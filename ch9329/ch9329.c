#include "ch9329.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define PORT "/dev/ttyUSB0"

#define print_pkt(C) print_cmd((unsigned char *)&PKT(C), CMD_LEN(C))

static struct key_map ch9329_maps[] = {
    { "Escape", 0x29 },
    { "F1", 0x3A },
    { "F2", 0x3B },
    { "F3", 0x3C },
    { "F4", 0x3D },
    { "F5", 0x3E },
    { "F6", 0x3F },
    { "F7", 0x40 },
    { "F8", 0x41 },
    { "F9", 0x42 },
    { "F10", 0x43 },
    { "F11", 0x44 },
    { "F12", 0x45 },
    { "ScrollLock", 0x47 },
    { "Pause", 0x48 },
    { "`", 0x35 },
    { "1", 0x1E },
    { "2", 0x1F },
    { "3", 0x20 },
    { "4", 0x21 },
    { "5", 0x22 },
    { "6", 0x23 },
    { "7", 0x24 },
    { "8", 0x25 },
    { "9", 0x26 },
    { "0", 0x27 },
    { "-", 0x2D },
    { "=", 0x2E },
    { "Backspace", 0x2A },
    { "Insert", 0x49 },
    { "Home", 0x4A },
    { "PageUp", 0x4B },
    { "Tab", 0x2B },
    { "Q", 0x14 },
    { "W", 0x1A },
    { "E", 0x08 },
    { "R", 0x15 },
    { "T", 0x17 },
    { "Y", 0x1C },
    { "U", 0x18 },
    { "I", 0x0C },
    { "O", 0x12 },
    { "P", 0x13 },
    { "[", 0x2F },
    { "]", 0x30 },
    { "\\", 0x31 },
    { "Delete", 0x4C },
    { "End", 0x4D },
    { "PageDown", 0x4E },
    { "CapsLock", 0x39 },
    { "A", 0x04 },
    { "S", 0x16 },
    { "D", 0x07 },
    { "F", 0x09 },
    { "G", 0x0A },
    { "H", 0x0B },
    { "J", 0x0D },
    { "K", 0x0E },
    { "L", 0x0F },
    { ";", 0x33 },
    { "'", 0x34 },
    { "Return", 0x28 },
    { "Left Shift", 0x02 },
    { "Z", 0x1D },
    { "X", 0x1B },
    { "C", 0x06 },
    { "V", 0x19 },
    { "B", 0x05 },
    { "N", 0x11 },
    { "M", 0x10 },
    { ",", 0x36 },
    { ".", 0x37 },
    { "/", 0x38 },
    { "Right Shift", 0x20 },
    { "Left Ctrl", 0x01 },
    { "Left Alt", 0x04 },
    { "Space", 0x2C },
    { "Right Alt", 0x40 },
    { "Left Win", 0x08 },
    { "Right Win", 0x80 },
    { "Right Ctrl", 0x10 },
    { "Up", 0x52 },
    { "Down", 0x51 },
    { "Left", 0x50 },
    { "Right", 0x4F },
    { "Numlock", 0x53 },
    { "Keypad /", 0x54 },
    { "Keypad *", 0x55 },
    { "Keypad -", 0x56 },
    { "Keypad 7", 0x5F },
    { "Keypad 8", 0x60 },
    { "Keypad 9", 0x61 },
    { "Keypad +", 0x57 },
    { "Keypad 4", 0x5C },
    { "Keypad 5", 0x5D },
    { "Keypad 6", 0x5E },
    { "Keypad 1", 0x59 },
    { "Keypad 2", 0x5A },
    { "Keypad 3", 0x5B },
    { "Keypad 0", 0x62 },
    { "Keypad .", 0x63 },
    { "Keypad Enter", 0x58 },
};

ssize_t serial_write(int fd, char *buf, size_t size);
ssize_t serial_read(int fd, char *buf, size_t size);

#define serial_write_and_wait_reply(fd, cmd, csize, reply, rsize) \
    do { \
        int ret; \
        int delay; \
        ret = serial_write(fd, (void*)cmd, csize); \
        if (ret < 0) { \
            perror("serial write cmd"); \
            return -1; \
        } \
        \
        delay = (double)csize / 1.2; \
        usleep(delay * 1000); \
        \
        memset(reply, 0, rsize); \
        ret = serial_read(fd, (void*)reply, rsize); \
        \
        if (ret < 0) { \
            perror("serial read reply"); \
            return -1; \
        } \
        \
        if (ret != rsize) { \
            printf("incomplete replay packet received!\n"); \
            return -1; \
        } \
    } while (0);


GHashTable *map = NULL;

unsigned char lookup_key_code(char *key)
{
    gpointer ret = g_hash_table_lookup(map, (gconstpointer)key);

    if (ret == NULL) {
        return 0;
    }

    return (unsigned char)ret;
}

void print_kv(gpointer key, gpointer value, gpointer user_data)
{
    printf("%s ==> 0x%02x\n", (char*)key, (unsigned char)value);
}

static void print_cmd(unsigned char *pkt, int len)
{
    int i;
    unsigned char *p = pkt;

    for (i = 0; i < len; i++) {
        printf("0x%02X ", *p++);
    }
    printf("\n");
}

static void gen_cmd_send_kb(char *key, enum CTRL_KEY mod, int pressed, void *pkt)
{
    unsigned long cksum = 0;
    int i;
    unsigned char *p = (unsigned char*)pkt;
    DECLARE_CMD_PTR(SEND_KB_GENERAL_DATA);

    PKTP(SEND_KB_GENERAL_DATA) = pkt;
    PKTP(SEND_KB_GENERAL_DATA)->head = PROTO_HEAD;
    PKTP(SEND_KB_GENERAL_DATA)->addr = 0x0;
    PKTP(SEND_KB_GENERAL_DATA)->cmd = CMD_SEND_KB_GENERAL_DATA;
    PKTP(SEND_KB_GENERAL_DATA)->len = SEND_KB_GENERAL_DATA_LEN;
    PKTP(SEND_KB_GENERAL_DATA)->data[0] = mod;
    PKTP(SEND_KB_GENERAL_DATA)->data[1] = 0x0;
    PKTP(SEND_KB_GENERAL_DATA)->data[2] = 0x0;

    if (pressed) {
        unsigned char code = lookup_key_code(key);
        PKTP(SEND_KB_GENERAL_DATA)->data[2] = code;
    }

    /* only support one key now */
    for (i = 3; i < 8; i++) {
        PKTP(SEND_KB_GENERAL_DATA)->data[i] = 0x0;
    }

    for (i = 0; i < 5 + SEND_KB_GENERAL_DATA_LEN; i++) {
        cksum += *p;
        p++;
    }

    PKTP(SEND_KB_GENERAL_DATA)->sum = cksum & 0xff;
}

ssize_t serial_write(int fd, char *buf, size_t size)
{
    return write(fd, buf, size);
}

ssize_t serial_read(int fd, char *buf, size_t size)
{
    int n;
    int left = size;

    while (left > 0) {
        n = read(fd, buf, size);
        if (n < 0) {
            perror("serial read");
            return size - left;
        } else if (n == 0) {
            return size - left;
        }

        left -= n;
        buf += n;
    }

    return size;
}

int send_key_mod(char *key, enum CTRL_KEY mod, int pressed, int fd)
{
    DECLARE_CMD(SEND_KB_GENERAL_DATA);
    DECLARE_CMD_REPLY(SEND_KB_GENERAL_DATA);

    gen_cmd_send_kb(key, mod, pressed, &PKT(SEND_KB_GENERAL_DATA));
    //printf("send %s %s\n", key, pressed ? "pressed" : "released");
    //print_pkt(SEND_KB_GENERAL_DATA);
    serial_write_and_wait_reply(fd, &PKT(SEND_KB_GENERAL_DATA), sizeof PKT(SEND_KB_GENERAL_DATA),
        &PKTR(SEND_KB_GENERAL_DATA), sizeof PKTR(SEND_KB_GENERAL_DATA));

    //printf("reply data: 0x%02x\n", PKTR(SEND_KB_GENERAL_DATA).data[0]);
    return 0;
}

int send_key_down(char *key, int fd)
{
    return send_key_mod(key, CTRL_KEY_NONE, 1, fd);
}

int send_key_up(char *key, int fd)
{
    return send_key_mod(key, CTRL_KEY_NONE, 0, fd);
}

static void gen_cmd_send_ms_btn(int motion, int x, int y, enum MOUSE_BTN button, void *pkt)
{
    unsigned long cksum = 0;
    int i;
    unsigned char *p = (unsigned char*)pkt;
    DECLARE_CMD_PTR(SEND_MS_REL_DATA);

    PKTP(SEND_MS_REL_DATA) = pkt;
    PKTP(SEND_MS_REL_DATA)->head = PROTO_HEAD;
    PKTP(SEND_MS_REL_DATA)->addr = 0x0;
    PKTP(SEND_MS_REL_DATA)->cmd = CMD_SEND_MS_REL_DATA;
    PKTP(SEND_MS_REL_DATA)->len = SEND_MS_REL_DATA_LEN;
    PKTP(SEND_MS_REL_DATA)->data[0] = 0x01;
    PKTP(SEND_MS_REL_DATA)->data[1] = button;

    if (button != MID_BUTTON) {
        /* mouse move */
        if (motion) {
            PKTP(SEND_MS_REL_DATA)->data[2] = x & 0xff;
            PKTP(SEND_MS_REL_DATA)->data[3] = y & 0xff;
        } else {
            PKTP(SEND_MS_REL_DATA)->data[2] = 0x0;
            PKTP(SEND_MS_REL_DATA)->data[3] = 0x0;
        }
        PKTP(SEND_MS_REL_DATA)->data[4] = 0x0;
    } else {    // use x only for mouse wheel scroll !
        PKTP(SEND_MS_REL_DATA)->data[2] = 0x0;
        PKTP(SEND_MS_REL_DATA)->data[3] = 0x0;
        PKTP(SEND_MS_REL_DATA)->data[4] = x & 0xff;
    }

    for (i = 0; i < 5 + SEND_MS_REL_DATA_LEN; i++) {
        cksum += *p;
        p++;
    }

    PKTP(SEND_MS_REL_DATA)->sum = cksum & 0xff;

}

static void gen_cmd_send_ms_abs_btn(int motion, int h, int w, int x, int y, enum MOUSE_BTN button, void *pkt)
{
    unsigned long cksum = 0;
    int i;
    unsigned char *p = (unsigned char*)pkt;
    DECLARE_CMD_PTR(SEND_MS_ABS_DATA);

    PKTP(SEND_MS_ABS_DATA) = pkt;
    PKTP(SEND_MS_ABS_DATA)->head = PROTO_HEAD;
    PKTP(SEND_MS_ABS_DATA)->addr = 0x0;
    PKTP(SEND_MS_ABS_DATA)->cmd = CMD_SEND_MS_ABS_DATA;
    PKTP(SEND_MS_ABS_DATA)->len = SEND_MS_ABS_DATA_LEN;
    PKTP(SEND_MS_ABS_DATA)->data[0] = 0x02;
    PKTP(SEND_MS_ABS_DATA)->data[1] = button;


    /* transform abs position */
    x = (4096 * x) / w;
    y = (4096 * y) / h;

    if (button != MID_BUTTON) {
        /* mouse move */
        if (motion) {
            PKTP(SEND_MS_ABS_DATA)->data[2] = x & 0xff;
            PKTP(SEND_MS_ABS_DATA)->data[3] = (x >> 8) & 0xff;
            PKTP(SEND_MS_ABS_DATA)->data[4] = y & 0xff;
            PKTP(SEND_MS_ABS_DATA)->data[5] = (y >> 8) & 0xff;
        } else {
            PKTP(SEND_MS_ABS_DATA)->data[2] = 0x0;
            PKTP(SEND_MS_ABS_DATA)->data[3] = 0x0;
            PKTP(SEND_MS_ABS_DATA)->data[4] = 0x0;
            PKTP(SEND_MS_ABS_DATA)->data[5] = 0x0;
        }
        PKTP(SEND_MS_ABS_DATA)->data[6] = 0x0;
    } else {    // use x only for mouse wheel scroll !
        PKTP(SEND_MS_ABS_DATA)->data[2] = 0x0;
        PKTP(SEND_MS_ABS_DATA)->data[3] = 0x0;
        PKTP(SEND_MS_ABS_DATA)->data[4] = 0x0;
        PKTP(SEND_MS_ABS_DATA)->data[5] = 0x0;
        PKTP(SEND_MS_ABS_DATA)->data[6] = x & 0xff;
    }

    for (i = 0; i < 5 + SEND_MS_ABS_DATA_LEN; i++) {
        cksum += *p;
        p++;
    }

    PKTP(SEND_MS_ABS_DATA)->sum = cksum & 0xff;

}

void gen_cmd_send_ms_move_btn(int x, int y, enum MOUSE_BTN button, void *pkt)
{
    char mx, my;

    if (x < 0) { // move left
        char off = - x;
        mx = (0x0 - off);
    } else {     // move rigth
        mx = x & 0x7f;
    }

    if (y < 0) { // move up
        char off = - y;
        my = (0x0 - off);
    } else {     // move down
        my = y & 0x7f;
    }

    gen_cmd_send_ms_btn(1, mx, my, button, pkt);
}

void gen_cmd_send_ms_click_btn(int x, int y, enum MOUSE_BTN button, void *pkt)
{
    gen_cmd_send_ms_btn(0, x, y, button, pkt);
}

int send_mouse_click_down(int fd, int x, int y, enum MOUSE_BTN button)
{
    DECLARE_CMD(SEND_MS_REL_DATA);
    DECLARE_CMD_REPLY(SEND_MS_REL_DATA);

    gen_cmd_send_ms_click_btn(x, y, button, &PKT(SEND_MS_REL_DATA));
    serial_write_and_wait_reply(fd, &PKT(SEND_MS_REL_DATA), sizeof PKT(SEND_MS_REL_DATA),
        &PKTR(SEND_MS_REL_DATA), sizeof PKTR(SEND_MS_REL_DATA));
    return 0;
}

int send_mouse_click_up(int fd, int x, int y)
{
    DECLARE_CMD(SEND_MS_REL_DATA);
    DECLARE_CMD_REPLY(SEND_MS_REL_DATA);

    gen_cmd_send_ms_click_btn(x, y, MOUSE_BTN_NONE, &PKT(SEND_MS_REL_DATA));
    serial_write_and_wait_reply(fd, &PKT(SEND_MS_REL_DATA), sizeof PKT(SEND_MS_REL_DATA),
        &PKTR(SEND_MS_REL_DATA), sizeof PKTR(SEND_MS_REL_DATA));
    return 0;
}

int send_mouse_move(int fd, int x, int y)
{
    DECLARE_CMD(SEND_MS_REL_DATA);
    DECLARE_CMD_REPLY(SEND_MS_REL_DATA);

    gen_cmd_send_ms_move_btn(x, y, MOUSE_BTN_NONE, &PKT(SEND_MS_REL_DATA));
    //print_pkt(SEND_MS_REL_DATA);
    serial_write_and_wait_reply(fd, &PKT(SEND_MS_REL_DATA), sizeof PKT(SEND_MS_REL_DATA),
        &PKTR(SEND_MS_REL_DATA), sizeof PKTR(SEND_MS_REL_DATA));

    return 0;
}

int send_mouse_move_abs(int fd, int h, int w, int x, int y)
{
    DECLARE_CMD(SEND_MS_ABS_DATA);
    DECLARE_CMD_REPLY(SEND_MS_ABS_DATA);

    gen_cmd_send_ms_abs_btn(1, h, w, x, y, MOUSE_BTN_NONE, &PKT(SEND_MS_ABS_DATA));
    serial_write_and_wait_reply(fd, &PKT(SEND_MS_ABS_DATA), sizeof PKT(SEND_MS_ABS_DATA),
        &PKTR(SEND_MS_ABS_DATA), sizeof PKTR(SEND_MS_ABS_DATA));

    return 0;
}

int set_term_attr(int fd, int speed)
{
    struct termios tty;
    int mcs;

    if (tcgetattr(fd, &tty) < 0) {
        printf("error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    cfmakeraw(&tty);

    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 0;

    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag |= (CLOCAL | CREAD);

    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    // TODO Hardware handshake
    // newtio.c_cflag |= Linux.Termios.CRTSCTS;
    mcs = 0;
    if (ioctl(fd, TIOCMGET, &mcs) < 0) {
        perror("TIOCMGET:");
    }

    mcs |= (TIOCM_RTS | TIOCM_DTR);

    if (ioctl(fd, TIOCMSET, &mcs) < 0) {
        perror("TIOCMSET:");
    }

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }

    tcflush(fd, TCOFLUSH);
    tcflush(fd, TCIFLUSH);

    return 0;
}

int get_conn_info(int fd)
{
    DECLARE_CMD(GET_INFO);
    DECLARE_CMD_REPLY(GET_INFO);

    PKT(GET_INFO).head = PROTO_HEAD;
    PKT(GET_INFO).addr = 0x00;
    PKT(GET_INFO).cmd = CMD_GET_INFO;
    PKT(GET_INFO).len = GET_INFO_LEN;
    PKT(GET_INFO).sum = 0x03;

    serial_write_and_wait_reply(fd, &PKT(GET_INFO), sizeof PKT(GET_INFO), &PKTR(GET_INFO), sizeof PKTR(GET_INFO));
    printf("CMD_GET_INFO results:\n");
    printf("chip version: 0x%02x\n", PKTR(GET_INFO).data[0]);
    printf("usb status: %s\n", (PKTR(GET_INFO).data[1] & USB_REC_OK) ? "OK" : "Error");
    printf("Number Lock: %s\n", (PKTR(GET_INFO).data[2] & NUM_LOCK_ON) ? "ON" : "OFF");
    printf("Caps Lock: %s\n", (PKTR(GET_INFO).data[2] & CAPS_LOCK_ON) ? "ON" : "OFF");
    printf("Scroll Lock: %s\n", (PKTR(GET_INFO).data[2] & SCROLL_LOCK_ON) ? "ON" : "OFF");

    if (!(PKTR(GET_INFO).data[1] & USB_REC_OK)) {
        printf("USB connect not ready, please check!\n");
        return -1;
    }

    return 0;
}

int reset_chip(int fd)
{
    DECLARE_CMD(RESET);
    DECLARE_CMD_REPLY(RESET);

    serial_write_and_wait_reply(fd, &PKT(RESET), sizeof PKT(RESET), &PKTR(RESET), sizeof PKTR(RESET));
    printf("cmd reset cmd: 0x%02x\n", PKTR(RESET).cmd);
    printf("cmd reset execute status: 0x%02x\n", PKTR(RESET).data[0]);
    return 0;
}

int ch9329_init(void)
{
    int i, fd;
    int ret;
    map = g_hash_table_new(g_str_hash, g_str_equal);

    for (i = 0; i < sizeof ch9329_maps  / sizeof(struct key_map); i++) {
        g_hash_table_insert(map, ch9329_maps[i].key, (gpointer)ch9329_maps[i].code);
    }

    fd = open(PORT, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        printf("can not open %s: %s\n", PORT, strerror(errno));
        return -1;
    }

    if (set_term_attr(fd, B9600) < 0) {
        printf("set term attributes error\n");
        return -1;
    }

    //reset_chip(fd);
    ret = get_conn_info(fd);

    if (ret != 0) {
        return -1;
    }

    return fd;
}

#if 0
int main()
{
    int i;
    int fd;
    int ret;
    map = g_hash_table_new(g_str_hash, g_str_equal);

    for (i = 0; i < sizeof ch9329_maps  / sizeof(struct key_map); i++) {
        g_hash_table_insert(map, ch9329_maps[i].key, (gpointer)ch9329_maps[i].code);
    }

    printf("keycode of A is: 0x%02X\n", lookup_key_code("A"));

    //g_hash_table_foreach(map, print_kv, NULL);

    fd = open(PORT, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        printf("can not open %s: %s\n", PORT, strerror(errno));
        return 1;
    }

    if (set_term_attr(fd, B9600) < 0) {
        printf("set term attributes error\n");
        return 1;
    }

    //reset_chip(fd);
    get_conn_info(fd);

    ret = send_key_down("A", fd);
    if (ret < 0) {
        printf("send_key_down error\n");
        return 1;
    }

    ret = send_key_up("A", fd);
    if (ret < 0) {
        printf("send_key_down error\n");
        return 1;
    }

    ret = send_mouse_click_down(fd);
    ret = send_mouse_click_up(fd);

    for (int i = 0; i < 10; i++) {
        send_mouse_move(fd, -5, 0);
    }

    for (int i = 0; i < 10; i++) {
        send_mouse_move(fd, 0, -4);
    }

    for (int i = 0; i < 10; i++) {
        send_mouse_move(fd, 5, 0);
    }

    for (int i = 0; i < 10; i++) {
        send_mouse_move(fd, 0, 4);
    }

    close(fd);
    return 0;
}
#endif
