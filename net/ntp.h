// net/ntp.h
#ifndef NTP_H
#define NTP_H

#include "../kernel/kernel.h"

#define NTP_PORT 123
#define NTP_TIMESTAMP_DELTA 2208988800ULL

#define NTP_MODE_CLIENT 3
#define NTP_VERSION 4

typedef struct {
    uint8_t li_vn_mode;
    uint8_t stratum;
    uint8_t poll;
    uint8_t precision;
    uint32_t root_delay;
    uint32_t root_dispersion;
    uint32_t ref_id;
    uint32_t ref_timestamp_sec;
    uint32_t ref_timestamp_frac;
    uint32_t orig_timestamp_sec;
    uint32_t orig_timestamp_frac;
    uint32_t recv_timestamp_sec;
    uint32_t recv_timestamp_frac;
    uint32_t trans_timestamp_sec;
    uint32_t trans_timestamp_frac;
} __attribute__((packed)) ntp_packet_t;

void ntp_init(void);
int ntp_sync_time(uint32_t* timestamp_out);
void ntp_set_server(uint32_t server_ip);

#endif