// -*- c-basic-offset: 4; c-backslash-column: 79; indent-tabs-mode: nil -*-
// vim:sw=4 ts=4 sts=4 expandtab
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#undef NDEBUG
#include <assert.h>
#include <junkie/cpp.h>
#include <junkie/tools/timeval.h>
#include <junkie/tools/objalloc.h>
#include <junkie/tools/ext.h>
#include <junkie/tools/hash.h>
#include <junkie/proto/pkt_wait_list.h>
#include <junkie/proto/cap.h>
#include <junkie/proto/eth.h>
#include <junkie/proto/ip.h>
#include <junkie/proto/udp.h>
#include <junkie/proto/cnxtrack.h>
#include "lib.h"

/*
 * These are 13 eth frames for a single UDP packet, fragmented at IP level
 */

#define PKT_SIZE (8 * 14 + 2)
uint8_t pkts[][PKT_SIZE] = {
    {
        0x46, 0x01, 0xc8, 0x63, 0xad, 0x64, 0x46, 0x01,
        0xc8, 0x63, 0xad, 0x64, 0x08, 0x00, 0x45, 0x00,
        0x00, 0x64, 0x59, 0xdd, 0x20, 0x00, 0x40, 0x11,
        0x2b, 0x06, 0xc0, 0xa8, 0x2a, 0x2a, 0xc0, 0xa8,
        0x2a, 0x2b, 0xae, 0x6d, 0x30, 0x39, 0x04, 0x08,
        0xb4, 0xee, 0x6c, 0x73, 0x64, 0x68, 0x6c, 0x6b,
        0x6a, 0x61, 0x66, 0x68, 0x73, 0x64, 0x68, 0x66,
        0x6c, 0x61, 0x68, 0x73, 0x64, 0x6c, 0x66, 0x68,
        0x61, 0x73, 0x6c, 0x64, 0x68, 0x66, 0x6b, 0x6c,
        0x6a, 0x61, 0x73, 0x68, 0x64, 0x66, 0x6c, 0x6b,
        0x61, 0x73, 0x68, 0x66, 0x6c, 0x68, 0x61, 0x73,
        0x6c, 0x66, 0x68, 0x61, 0x6c, 0x73, 0x68, 0x64,
        0x66, 0x6c, 0x61, 0x68, 0x73, 0x64, 0x6c, 0x66,
        0x6b, 0x6a, 0x68, 0x61, 0x73, 0x6b, 0x6c, 0x6a,
        0x66, 0x64
    }, {
        0x46, 0x01, 0xc8, 0x63, 0xad, 0x64, 0x46, 0x01,
        0xc8, 0x63, 0xad, 0x64, 0x08, 0x00, 0x45, 0x00,
        0x00, 0x64, 0x59, 0xdd, 0x20, 0x0a, 0x40, 0x11,
        0x2a, 0xfc, 0xc0, 0xa8, 0x2a, 0x2a, 0xc0, 0xa8,
        0x2a, 0x2b, 0x68, 0x61, 0x6c, 0x6b, 0x6a, 0x68,
        0x66, 0x61, 0x6c, 0x68, 0x6b, 0x6c, 0x6a, 0x68,
        0x6c, 0x73, 0x64, 0x68, 0x6c, 0x6b, 0x6a, 0x61,
        0x66, 0x68, 0x73, 0x64, 0x68, 0x66, 0x6c, 0x61,
        0x68, 0x73, 0x64, 0x6c, 0x66, 0x68, 0x61, 0x73,
        0x6c, 0x64, 0x68, 0x66, 0x6b, 0x6c, 0x6a, 0x61,
        0x73, 0x68, 0x64, 0x66, 0x6c, 0x6b, 0x61, 0x73,
        0x68, 0x66, 0x6c, 0x68, 0x61, 0x73, 0x6c, 0x66,
        0x68, 0x61, 0x6c, 0x73, 0x68, 0x64, 0x66, 0x6c,
        0x61, 0x68, 0x73, 0x64, 0x6c, 0x66, 0x6b, 0x6a,
        0x68, 0x61
    }, {
        0x46, 0x01, 0xc8, 0x63, 0xad, 0x64, 0x46, 0x01,
        0xc8, 0x63, 0xad, 0x64, 0x08, 0x00, 0x45, 0x00,
        0x00, 0x64, 0x59, 0xdd, 0x20, 0x14, 0x40, 0x11,
        0x2a, 0xf2, 0xc0, 0xa8, 0x2a, 0x2a, 0xc0, 0xa8,
        0x2a, 0x2b, 0x73, 0x6b, 0x6c, 0x6a, 0x66, 0x64,
        0x68, 0x61, 0x6c, 0x6b, 0x6a, 0x68, 0x66, 0x61,
        0x6c, 0x68, 0x6b, 0x6c, 0x6a, 0x68, 0x6c, 0x73,
        0x64, 0x68, 0x6c, 0x6b, 0x6a, 0x61, 0x66, 0x68,
        0x73, 0x64, 0x68, 0x66, 0x6c, 0x61, 0x68, 0x73,
        0x64, 0x6c, 0x66, 0x68, 0x61, 0x73, 0x6c, 0x64,
        0x68, 0x66, 0x6b, 0x6c, 0x6a, 0x61, 0x73, 0x68,
        0x64, 0x66, 0x6c, 0x6b, 0x61, 0x73, 0x68, 0x66,
        0x6c, 0x68, 0x61, 0x73, 0x6c, 0x66, 0x68, 0x61,
        0x6c, 0x73, 0x68, 0x64, 0x66, 0x6c, 0x61, 0x68,
        0x73, 0x64
    }, {
        0x46, 0x01, 0xc8, 0x63, 0xad, 0x64, 0x46, 0x01,
        0xc8, 0x63, 0xad, 0x64, 0x08, 0x00, 0x45, 0x00,
        0x00, 0x64, 0x59, 0xdd, 0x20, 0x1e, 0x40, 0x11,
        0x2a, 0xe8, 0xc0, 0xa8, 0x2a, 0x2a, 0xc0, 0xa8,
        0x2a, 0x2b, 0x6c, 0x66, 0x6b, 0x6a, 0x68, 0x61,
        0x73, 0x6b, 0x6c, 0x6a, 0x66, 0x64, 0x68, 0x61,
        0x6c, 0x6b, 0x6a, 0x68, 0x66, 0x61, 0x6c, 0x68,
        0x6b, 0x6c, 0x6a, 0x68, 0x6c, 0x73, 0x64, 0x68,
        0x6c, 0x6b, 0x6a, 0x61, 0x66, 0x68, 0x73, 0x64,
        0x68, 0x66, 0x6c, 0x61, 0x68, 0x73, 0x64, 0x6c,
        0x66, 0x68, 0x61, 0x73, 0x6c, 0x64, 0x68, 0x66,
        0x6b, 0x6c, 0x6a, 0x61, 0x73, 0x68, 0x64, 0x66,
        0x6c, 0x6b, 0x61, 0x73, 0x68, 0x66, 0x6c, 0x68,
        0x61, 0x73, 0x6c, 0x66, 0x68, 0x61, 0x6c, 0x73,
        0x68, 0x64
    }, {
        0x46, 0x01, 0xc8, 0x63, 0xad, 0x64, 0x46, 0x01,
        0xc8, 0x63, 0xad, 0x64, 0x08, 0x00, 0x45, 0x00,
        0x00, 0x64, 0x59, 0xdd, 0x20, 0x28, 0x40, 0x11,
        0x2a, 0xde, 0xc0, 0xa8, 0x2a, 0x2a, 0xc0, 0xa8,
        0x2a, 0x2b, 0x66, 0x6c, 0x61, 0x68, 0x73, 0x64,
        0x6c, 0x66, 0x6b, 0x6a, 0x68, 0x61, 0x73, 0x6b,
        0x6c, 0x6a, 0x66, 0x64, 0x68, 0x61, 0x6c, 0x6b,
        0x6a, 0x68, 0x66, 0x61, 0x6c, 0x68, 0x6b, 0x6c,
        0x6a, 0x68, 0x6c, 0x73, 0x64, 0x68, 0x6c, 0x6b,
        0x6a, 0x61, 0x66, 0x68, 0x73, 0x64, 0x68, 0x66,
        0x6c, 0x61, 0x68, 0x73, 0x64, 0x6c, 0x66, 0x68,
        0x61, 0x73, 0x6c, 0x64, 0x68, 0x66, 0x6b, 0x6c,
        0x6a, 0x61, 0x73, 0x68, 0x64, 0x66, 0x6c, 0x6b,
        0x61, 0x73, 0x68, 0x66, 0x6c, 0x68, 0x61, 0x73,
        0x6c, 0x66
    }, {
        0x46, 0x01, 0xc8, 0x63, 0xad, 0x64, 0x46, 0x01,
        0xc8, 0x63, 0xad, 0x64, 0x08, 0x00, 0x45, 0x00,
        0x00, 0x64, 0x59, 0xdd, 0x20, 0x32, 0x40, 0x11,
        0x2a, 0xd4, 0xc0, 0xa8, 0x2a, 0x2a, 0xc0, 0xa8,
        0x2a, 0x2b, 0x68, 0x61, 0x6c, 0x73, 0x68, 0x64,
        0x66, 0x6c, 0x61, 0x68, 0x73, 0x64, 0x6c, 0x66,
        0x6b, 0x6a, 0x68, 0x61, 0x73, 0x6b, 0x6c, 0x6a,
        0x66, 0x64, 0x68, 0x61, 0x6c, 0x6b, 0x6a, 0x68,
        0x66, 0x61, 0x6c, 0x68, 0x6b, 0x6c, 0x6a, 0x68,
        0x6c, 0x73, 0x64, 0x68, 0x6c, 0x6b, 0x6a, 0x61,
        0x66, 0x68, 0x73, 0x64, 0x68, 0x66, 0x6c, 0x61,
        0x68, 0x73, 0x64, 0x6c, 0x66, 0x68, 0x61, 0x73,
        0x6c, 0x64, 0x68, 0x66, 0x6b, 0x6c, 0x6a, 0x61,
        0x73, 0x68, 0x64, 0x66, 0x6c, 0x6b, 0x61, 0x73,
        0x68, 0x66
    }, {
        0x46, 0x01, 0xc8, 0x63, 0xad, 0x64, 0x46, 0x01,
        0xc8, 0x63, 0xad, 0x64, 0x08, 0x00, 0x45, 0x00,
        0x00, 0x64, 0x59, 0xdd, 0x20, 0x3c, 0x40, 0x11,
        0x2a, 0xca, 0xc0, 0xa8, 0x2a, 0x2a, 0xc0, 0xa8,
        0x2a, 0x2b, 0x6c, 0x68, 0x61, 0x73, 0x6c, 0x66,
        0x68, 0x61, 0x6c, 0x73, 0x68, 0x64, 0x66, 0x6c,
        0x61, 0x68, 0x73, 0x64, 0x6c, 0x66, 0x6b, 0x6a,
        0x68, 0x61, 0x73, 0x6b, 0x6c, 0x6a, 0x66, 0x64,
        0x68, 0x61, 0x6c, 0x6b, 0x6a, 0x68, 0x66, 0x61,
        0x6c, 0x68, 0x6b, 0x6c, 0x6a, 0x68, 0x6c, 0x73,
        0x64, 0x68, 0x6c, 0x6b, 0x6a, 0x61, 0x66, 0x68,
        0x73, 0x64, 0x68, 0x66, 0x6c, 0x61, 0x68, 0x73,
        0x64, 0x6c, 0x66, 0x68, 0x61, 0x73, 0x6c, 0x64,
        0x68, 0x66, 0x6b, 0x6c, 0x6a, 0x61, 0x73, 0x68,
        0x64, 0x66
    }, {
        0x46, 0x01, 0xc8, 0x63, 0xad, 0x64, 0x46, 0x01,
        0xc8, 0x63, 0xad, 0x64, 0x08, 0x00, 0x45, 0x00,
        0x00, 0x64, 0x59, 0xdd, 0x20, 0x46, 0x40, 0x11,
        0x2a, 0xc0, 0xc0, 0xa8, 0x2a, 0x2a, 0xc0, 0xa8,
        0x2a, 0x2b, 0x6c, 0x6b, 0x61, 0x73, 0x68, 0x66,
        0x6c, 0x68, 0x61, 0x73, 0x6c, 0x66, 0x68, 0x61,
        0x6c, 0x73, 0x68, 0x64, 0x66, 0x6c, 0x61, 0x68,
        0x73, 0x64, 0x6c, 0x66, 0x6b, 0x6a, 0x68, 0x61,
        0x73, 0x6b, 0x6c, 0x6a, 0x66, 0x64, 0x68, 0x61,
        0x6c, 0x6b, 0x6a, 0x68, 0x66, 0x61, 0x6c, 0x68,
        0x6b, 0x6c, 0x6a, 0x68, 0x6c, 0x73, 0x64, 0x68,
        0x6c, 0x6b, 0x6a, 0x61, 0x66, 0x68, 0x73, 0x64,
        0x68, 0x66, 0x6c, 0x61, 0x68, 0x73, 0x64, 0x6c,
        0x66, 0x68, 0x61, 0x73, 0x6c, 0x64, 0x68, 0x66,
        0x6b, 0x6c
    }, {
        0x46, 0x01, 0xc8, 0x63, 0xad, 0x64, 0x46, 0x01,
        0xc8, 0x63, 0xad, 0x64, 0x08, 0x00, 0x45, 0x00,
        0x00, 0x64, 0x59, 0xdd, 0x20, 0x50, 0x40, 0x11,
        0x2a, 0xb6, 0xc0, 0xa8, 0x2a, 0x2a, 0xc0, 0xa8,
        0x2a, 0x2b, 0x6a, 0x61, 0x73, 0x68, 0x64, 0x66,
        0x6c, 0x6b, 0x61, 0x73, 0x68, 0x66, 0x6c, 0x68,
        0x61, 0x73, 0x6c, 0x66, 0x68, 0x61, 0x6c, 0x73,
        0x68, 0x64, 0x66, 0x6c, 0x61, 0x68, 0x73, 0x64,
        0x6c, 0x66, 0x6b, 0x6a, 0x68, 0x61, 0x73, 0x6b,
        0x6c, 0x6a, 0x66, 0x64, 0x68, 0x61, 0x6c, 0x6b,
        0x6a, 0x68, 0x66, 0x61, 0x6c, 0x68, 0x6b, 0x6c,
        0x6a, 0x68, 0x6c, 0x73, 0x64, 0x68, 0x6c, 0x6b,
        0x6a, 0x61, 0x66, 0x68, 0x73, 0x64, 0x68, 0x66,
        0x6c, 0x61, 0x68, 0x73, 0x64, 0x6c, 0x66, 0x68,
        0x61, 0x73
    }, {
        0x46, 0x01, 0xc8, 0x63, 0xad, 0x64, 0x46, 0x01,
        0xc8, 0x63, 0xad, 0x64, 0x08, 0x00, 0x45, 0x00,
        0x00, 0x64, 0x59, 0xdd, 0x20, 0x5a, 0x40, 0x11,
        0x2a, 0xac, 0xc0, 0xa8, 0x2a, 0x2a, 0xc0, 0xa8,
        0x2a, 0x2b, 0x6c, 0x64, 0x68, 0x66, 0x6b, 0x6c,
        0x6a, 0x61, 0x73, 0x68, 0x64, 0x66, 0x6c, 0x6b,
        0x61, 0x73, 0x68, 0x66, 0x6c, 0x68, 0x61, 0x73,
        0x6c, 0x66, 0x68, 0x61, 0x6c, 0x73, 0x68, 0x64,
        0x66, 0x6c, 0x61, 0x68, 0x73, 0x64, 0x6c, 0x66,
        0x6b, 0x6a, 0x68, 0x61, 0x73, 0x6b, 0x6c, 0x6a,
        0x66, 0x64, 0x68, 0x61, 0x6c, 0x6b, 0x6a, 0x68,
        0x66, 0x61, 0x6c, 0x68, 0x6b, 0x6c, 0x6a, 0x68,
        0x6c, 0x73, 0x64, 0x68, 0x6c, 0x6b, 0x6a, 0x61,
        0x66, 0x68, 0x73, 0x64, 0x68, 0x66, 0x6c, 0x61,
        0x68, 0x73
    }, {
        0x46, 0x01, 0xc8, 0x63, 0xad, 0x64, 0x46, 0x01,
        0xc8, 0x63, 0xad, 0x64, 0x08, 0x00, 0x45, 0x00,
        0x00, 0x64, 0x59, 0xdd, 0x20, 0x64, 0x40, 0x11,
        0x2a, 0xa2, 0xc0, 0xa8, 0x2a, 0x2a, 0xc0, 0xa8,
        0x2a, 0x2b, 0x64, 0x6c, 0x66, 0x68, 0x61, 0x73,
        0x6c, 0x64, 0x68, 0x66, 0x6b, 0x6c, 0x6a, 0x61,
        0x73, 0x68, 0x64, 0x66, 0x6c, 0x6b, 0x61, 0x73,
        0x68, 0x66, 0x6c, 0x68, 0x61, 0x73, 0x6c, 0x66,
        0x68, 0x61, 0x6c, 0x73, 0x68, 0x64, 0x66, 0x6c,
        0x61, 0x68, 0x73, 0x64, 0x6c, 0x66, 0x6b, 0x6a,
        0x68, 0x61, 0x73, 0x6b, 0x6c, 0x6a, 0x66, 0x64,
        0x68, 0x61, 0x6c, 0x6b, 0x6a, 0x68, 0x66, 0x61,
        0x6c, 0x68, 0x6b, 0x6c, 0x6a, 0x68, 0x6c, 0x73,
        0x64, 0x68, 0x6c, 0x6b, 0x6a, 0x61, 0x66, 0x68,
        0x73, 0x64
    }, {
        0x46, 0x01, 0xc8, 0x63, 0xad, 0x64, 0x46, 0x01,
        0xc8, 0x63, 0xad, 0x64, 0x08, 0x00, 0x45, 0x00,
        0x00, 0x64, 0x59, 0xdd, 0x20, 0x6e, 0x40, 0x11,
        0x2a, 0x98, 0xc0, 0xa8, 0x2a, 0x2a, 0xc0, 0xa8,
        0x2a, 0x2b, 0x68, 0x66, 0x6c, 0x61, 0x68, 0x73,
        0x64, 0x6c, 0x66, 0x68, 0x61, 0x73, 0x6c, 0x64,
        0x68, 0x66, 0x6b, 0x6c, 0x6a, 0x61, 0x73, 0x68,
        0x64, 0x66, 0x6c, 0x6b, 0x61, 0x73, 0x68, 0x66,
        0x6c, 0x68, 0x61, 0x73, 0x6c, 0x66, 0x68, 0x61,
        0x6c, 0x73, 0x68, 0x64, 0x66, 0x6c, 0x61, 0x68,
        0x73, 0x64, 0x6c, 0x66, 0x6b, 0x6a, 0x68, 0x61,
        0x73, 0x6b, 0x6c, 0x6a, 0x66, 0x64, 0x68, 0x61,
        0x6c, 0x6b, 0x6a, 0x68, 0x66, 0x61, 0x6c, 0x68,
        0x6b, 0x6c, 0x6a, 0x68, 0x6c, 0x73, 0x64, 0x68,
        0x6c, 0x6b
    }, {
        0x46, 0x01, 0xc8, 0x63, 0xad, 0x64, 0x46, 0x01,
        0xc8, 0x63, 0xad, 0x64, 0x08, 0x00, 0x45, 0x00,
        0x00, 0x5c, 0x59, 0xdd, 0x00, 0x78, 0x40, 0x11,
        0x4a, 0x96, 0xc0, 0xa8, 0x2a, 0x2a, 0xc0, 0xa8,
        0x2a, 0x2b, 0x6a, 0x61, 0x66, 0x68, 0x73, 0x64,
        0x68, 0x66, 0x6c, 0x61, 0x68, 0x73, 0x64, 0x6c,
        0x66, 0x68, 0x61, 0x73, 0x6c, 0x64, 0x68, 0x66,
        0x6b, 0x6c, 0x6a, 0x61, 0x73, 0x68, 0x64, 0x66,
        0x6c, 0x6b, 0x61, 0x73, 0x68, 0x66, 0x6c, 0x68,
        0x61, 0x73, 0x6c, 0x66, 0x68, 0x61, 0x6c, 0x73,
        0x68, 0x64, 0x66, 0x6c, 0x61, 0x68, 0x73, 0x64,
        0x6c, 0x66, 0x6b, 0x6a, 0x68, 0x61, 0x73, 0x6b,
        0x6c, 0x6a, 0x66, 0x64, 0x68, 0x61, 0x6c, 0x6b,
        0x6a, 0x68
    }
};

/*
 * Check that okfn is called once with Eth/IUP/UDP
 */

static bool had_udp;
static struct timeval now;
static struct parser *eth_parser;
static unsigned nb_okfn_calls;
static unsigned udp_num; // when did we receive the whole payload ?
static unsigned pkt_num[NB_ELEMS(pkts)];
static struct proto_subscriber sub;

static void okfn(struct proto_subscriber unused_ *s, struct proto_info const *last, size_t cap_len, uint8_t const *packet, struct timeval const unused_ *now)
{
    // We are called once for UDP, and once for each IP fragments
    if (last->parser->proto == proto_udp) {
        assert(! had_udp);
        had_udp = true;
        udp_num = nb_okfn_calls;
        assert(last->payload == 1024);
    } else {
        assert(last->parser->proto == proto_ip);
        assert(last->payload == 80 || last->payload == 72); // last fragment is 72 bytes long
    }

    // Try to find out which packet have been sent
    for (unsigned p = 0; p < NB_ELEMS(pkts); p++) {
        if (0 == memcmp(packet, pkts[p], cap_len)) {
            assert(nb_okfn_calls < NB_ELEMS(pkt_num));
            pkt_num[nb_okfn_calls] = p;
            break;
        }
    }

    nb_okfn_calls ++;
}

static void setup(void)
{
    timeval_set_now(&now);
    eth_parser = proto_eth->ops->parser_new(proto_eth);
    assert(eth_parser);
    had_udp = false;
    nb_okfn_calls = 0;
    udp_num = ~0;
    for (unsigned p = 0; p < NB_ELEMS(pkt_num); p++) pkt_num[p] = ~0;
    pkt_subscriber_ctor(&sub, okfn);
}

static void teardown(void)
{
    pkt_subscriber_dtor(&sub);
    parser_unref(eth_parser);
}

static void check_result(void)
{
    assert(nb_okfn_calls == NB_ELEMS(pkts));
    assert(udp_num == NB_ELEMS(pkts)-1);    // we'd like the full reassembled packet to be revealed last
    // Also check that we were sent the packets in the correct order (ie first to last fragment)
    for (unsigned p = 0; p < NB_ELEMS(pkts); p++) assert(pkt_num[p] == p);
}

// Send all fragments in order and check the reassembly
static void simple_check(void)
{
    setup();

    for (unsigned p = 0 ; p < NB_ELEMS(pkts); p++) {
        assert(PROTO_OK == proto_parse(eth_parser, NULL, 0, pkts[p], sizeof(pkts[p]), sizeof(pkts[p]), &now, sizeof(pkts[p]), pkts[p]));
    }
    check_result();

    teardown();
}

// Same as above, when fragments are sent the other way around
static void reverse_check(void)
{
    setup();

    unsigned p = NB_ELEMS(pkts);
    while (p--) {
        assert(PROTO_OK == proto_parse(eth_parser, NULL, 0, pkts[p], sizeof(pkts[p]), sizeof(pkts[p]), &now, sizeof(pkts[p]), pkts[p]));
    }
    check_result();

    teardown();
}

// Now at random
static void random_check(void)
{
    setup();

    bool sent[NB_ELEMS(pkts)] = { };

    for (unsigned nb_sent = 0; nb_sent < NB_ELEMS(pkts); nb_sent++) {
        unsigned p = random() % NB_ELEMS(pkts);
        while (sent[p]) p = (p+1) % NB_ELEMS(pkts);
        sent[p] = true;
        SLOG(LOG_DEBUG, "Sending Packet %u", p);
        assert(PROTO_OK == proto_parse(eth_parser, NULL, 0, pkts[p], sizeof(pkts[p]), sizeof(pkts[p]), &now, sizeof(pkts[p]), pkts[p]));
    }
    check_result();

    teardown();
}

int main(void)
{
    log_init();
    mutex_init();
    ext_init();
    objalloc_init();
    hash_init();
    cnxtrack_init();
    pkt_wait_list_init();
    ref_init();
    proto_init();
    cap_init();
    eth_init();
    ip_init();
    ip6_init();
    udp_init();
    log_set_level(LOG_DEBUG, NULL);
    log_set_level(LOG_INFO, "mutex");
    log_set_file("ip_reassembly_check.log");
    srandom(time(NULL));

    simple_check();
    reverse_check();
    for (unsigned nb_rand = 0; nb_rand < 100; nb_rand++) random_check();

    doomer_stop();
    udp_fini();
    ip6_fini();
    ip_fini();
    eth_fini();
    cap_fini();
    proto_fini();
    ref_fini();
    pkt_wait_list_fini();
    cnxtrack_fini();
    hash_fini();
    objalloc_fini();
    ext_fini();
    mutex_fini();
    log_fini();
    return EXIT_SUCCESS;
}

