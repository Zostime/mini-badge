#include <kernel/tty.h>
#include <kernel/types.h>
#include "usbd_cdc_if.h"

ssize_t tty_read(char *buf, int size) {
    int idx = 0;

    if (buf == NULL || size <= 0) return -1;

    while (1) {
        char packet[64];
        int n = 0;
        size_t avail = (size_t)cdc_rx_available();
        if (avail > sizeof(packet)) avail = sizeof(packet);
        for (int i = 0; i < avail; i++) {
            if (!cdc_rx_pop(&packet[i])) break;
            n++;
        }

        if (n == 0) {
            HAL_Delay(1);
            continue;
        }

        for (int i = 0; i < n; i++) {
            char c = packet[i];
            if (c == '\r' || c == '\n') {
                buf[idx] = '\0';
                return idx;
            }
            else if (c == 8 || c == 127) {
                if (idx > 0) idx--;
            }
            else {
                if (idx < size - 1) buf[idx++] = c;
            }
        }
    }
}

ssize_t tty_write(char *buf, int size) {
	return 0;
}
