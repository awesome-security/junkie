// -*- c-basic-offset: 4; c-backslash-column: 79; indent-tabs-mode: nil -*-
// vim:sw=4 ts=4 sts=4 expandtab
#include <stdlib.h>
#undef NDEBUG
#include <assert.h>
#include <junkie/cpp.h>
#include <junkie/tools/ext.h>
#include <junkie/tools/objalloc.h>
#include <junkie/proto/pkt_wait_list.h>
#include <junkie/proto/cap.h>
#include <junkie/proto/ip.h>
#include <junkie/proto/eth.h>
#include <junkie/proto/tcp.h>
#include "lib.h"
#include "proto/cifs.c"

static struct parse_test {
    uint8_t const *packet;
    int size;
    enum proto_parse_status ret;
    struct cifs_proto_info expected;
    bool way;
} parse_tests[] = {

    // a negociate response
    {
        .packet = (uint8_t const []) {
//          Header-{-               Nego
            0xff, 0x53, 0x4d, 0x42, 0x72, 0x00, 0x00, 0x00, 0x00, 0x80, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00,
//                                                                                                      -}
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x47, 0x00, 0x00, 0x01, 0x00,
//          WC-
            0x11, 0x02, 0x00, 0x03, 0x32, 0x00, 0x01, 0x00, 0x04, 0x41, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
//                                  Capabilities----------
            0xcc, 0x77, 0x00, 0x00, 0xfd, 0xf3, 0x80, 0x00, 0x00, 0xb5, 0x65, 0x00, 0x86, 0x6f, 0xcf, 0x01,
//                      Chal  BC--------
            0x88, 0xff, 0x08, 0x1c, 0x00, 0xbc, 0x52, 0x94, 0xe1, 0x5d, 0xf2, 0x8f, 0x5f, 0x57, 0x00, 0x4f,
            0x00, 0x52, 0x00, 0x4b, 0x00, 0x47, 0x00, 0x52, 0x00, 0x4f, 0x00, 0x55, 0x00, 0x50, 0x00, 0x00,
            0x00
        },
        .size = 0x61,
        .ret = PROTO_OK,
        .way = FROM_SERVER,
        .expected = {
            .info = { .head_len = CIFS_HEADER_SIZE, .payload = 0x61 - CIFS_HEADER_SIZE},
            .command = SMB_COM_NEGOCIATE,
            .domain = "WORKGROUP",
            .set_values = SMB_DOMAIN,
            .status = SMB_STATUS_OK,
        },
    },

    // session setup andx query
    {
        .packet = (uint8_t const []) {
            0xff, 0x53, 0x4d, 0x42, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xd0, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x59, 0x00, 0x00, 0x0e, 0x00,
            0x0d, 0xff, 0x00, 0x00, 0x00, 0x54, 0x40, 0x32, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18,
            0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0xdc, 0xd0, 0x80, 0x00, 0xa9, 0x00, 0x75, 0xd9, 0x4e,
            0xda, 0xbf, 0x5f, 0x86, 0xf1, 0xa4, 0x52, 0x97, 0x2d, 0x45, 0x05, 0xe4, 0x69, 0x06, 0xcd, 0xdb,
            0xaa, 0xe9, 0x89, 0x16, 0x6e, 0x75, 0xd9, 0x4e, 0xda, 0xbf, 0x5f, 0x86, 0xf1, 0xa4, 0x52, 0x97,
            0x2d, 0x45, 0x05, 0xe4, 0x69, 0x06, 0xcd, 0xdb, 0xaa, 0xe9, 0x89, 0x16, 0x6e, 0x00, 0x72, 0x00,
            0x6f, 0x00, 0x6f, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4c, 0x00, 0x69, 0x00, 0x6e, 0x00,
            0x75, 0x00, 0x78, 0x00, 0x20, 0x00, 0x76, 0x00, 0x65, 0x00, 0x72, 0x00, 0x73, 0x00, 0x69, 0x00,
            0x6f, 0x00, 0x6e, 0x00, 0x20, 0x00, 0x33, 0x00, 0x2e, 0x00, 0x32, 0x00, 0x2e, 0x00, 0x30, 0x00,
            0x2d, 0x00, 0x34, 0x00, 0x2d, 0x00, 0x61, 0x00, 0x6d, 0x00, 0x64, 0x00, 0x36, 0x00, 0x34, 0x00,
            0x00, 0x00, 0x43, 0x00, 0x49, 0x00, 0x46, 0x00, 0x53, 0x00, 0x20, 0x00, 0x56, 0x00, 0x46, 0x00,
            0x53, 0x00, 0x20, 0x00, 0x43, 0x00, 0x6c, 0x00, 0x69, 0x00, 0x65, 0x00, 0x6e, 0x00, 0x74, 0x00,
            0x20, 0x00, 0x66, 0x00, 0x6f, 0x00, 0x72, 0x00, 0x20, 0x00, 0x4c, 0x00, 0x69, 0x00, 0x6e, 0x00,
            0x75, 0x00, 0x78, 0x00, 0x00, 0x00
        },
        .size = 0xe6,
        .ret = PROTO_OK,
        .way = FROM_CLIENT,
        .expected = {
            .info = { .head_len = CIFS_HEADER_SIZE, .payload = 0xe6 - CIFS_HEADER_SIZE},
            .command = SMB_COM_SESSION_SETUP_ANDX,
            .user = "root",
            .set_values = SMB_USER,
            .status = SMB_STATUS_OK,
        },
    },

    // session setup andx response
    {
        .packet = (uint8_t const []) {
            0xff, 0x53, 0x4d, 0x42, 0x73, 0x00, 0x00, 0x00, 0x00, 0x80, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x47, 0x64, 0x00, 0x02, 0x00,
            0x03, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x37, 0x00, 0x00, 0x55, 0x00, 0x6e, 0x00, 0x69, 0x00,
            0x78, 0x00, 0x00, 0x00, 0x53, 0x00, 0x61, 0x00, 0x6d, 0x00, 0x62, 0x00, 0x61, 0x00, 0x20, 0x00,
            0x33, 0x00, 0x2e, 0x00, 0x35, 0x00, 0x2e, 0x00, 0x36, 0x00, 0x00, 0x00, 0x57, 0x00, 0x4f, 0x00,
            0x52, 0x00, 0x4b, 0x00, 0x47, 0x00, 0x52, 0x00, 0x4f, 0x00, 0x55, 0x00, 0x50, 0x00, 0x00, 0x00
        },
        .size = 0x60,
        .ret = PROTO_OK,
        .way = FROM_SERVER,
        .expected = {
            .info = { .head_len = CIFS_HEADER_SIZE, .payload = 0x60 - CIFS_HEADER_SIZE},
            .command = SMB_COM_SESSION_SETUP_ANDX,
            .domain = "WORKGROUP",
            .set_values = SMB_DOMAIN,
            .status = SMB_STATUS_OK,
        },
    },

    // Session response with error
    {
        .packet = (uint8_t const []) {
            0xff, 0x53, 0x4d, 0x42, 0x73, 0x6d, 0x00, 0x00, 0xc0, 0x80, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x59, 0x00, 0x00, 0x0e, 0x00,
            0x00, 0x00, 0x00
        },
        .size = 0x23,
        .ret = PROTO_OK,
        .way = FROM_SERVER,
        .expected = {
            .info = { .head_len = CIFS_HEADER_SIZE, .payload = 0x23 - CIFS_HEADER_SIZE},
            .command = SMB_COM_SESSION_SETUP_ANDX,
            .status = SMB_STATUS_LOGON_FAILURE,
        },
    },

    // Trans2 request, query fs info
    {
        .packet = (uint8_t const []) {
            0xff, 0x53, 0x4d, 0x42, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x3b, 0x6c, 0x64, 0x00, 0x0f, 0x00,
            0x0f, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x02, 0x00, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x03,
            0x00, 0x00, 0x01, 0x02
        },
        .size = 0x44,
        .ret = PROTO_OK,
        .way = FROM_CLIENT,
        .expected = {
            .info = { .head_len = CIFS_HEADER_SIZE, .payload = 0x44 - CIFS_HEADER_SIZE},
            .command = SMB_COM_TRANSACTION2,
            .set_values = SMB_TRANS2_SUBCMD,
            .trans2_subcmd = SMB_TRANS2_QUERY_FS_INFO,
            .status = SMB_STATUS_OK,
        },
    },

    // Trans2 request, query path info
    {
        .packet = (uint8_t const []) {
//          Start
            0xff, 0x53, 0x4d, 0x42, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xec, 0x1a, 0x64, 0x00, 0x19, 0x00,
            0x0f, 0x28, 0x00, 0x00, 0x00, 0x02, 0x00, 0xa0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x28, 0x00, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x05, 0x00, 0x29,
//                Padd
            0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x2f, 0x00, 0x6c, 0x00, 0x69, 0x00, 0x62, 0x00,
            0x70, 0x00, 0x74, 0x00, 0x68, 0x00, 0x72, 0x00, 0x65, 0x00, 0x61, 0x00, 0x64, 0x00, 0x2e, 0x00,
            0x73, 0x00, 0x6f, 0x00, 0x2e, 0x00, 0x30, 0x00, 0x00, 0x00
        },
        .size = 0x6a,
        .ret = PROTO_OK,
        .way = FROM_CLIENT,
        .expected = {
            .info = { .head_len = CIFS_HEADER_SIZE, .payload = 0x6a - CIFS_HEADER_SIZE},
            .command = SMB_COM_TRANSACTION2,
            .set_values = SMB_TRANS2_SUBCMD | SMB_PATH,
            .trans2_subcmd = SMB_TRANS2_QUERY_PATH_INFORMATION,
            .path = "/libpthread.so.0",
            .status = SMB_STATUS_OK,
        },
    },

    // Trans2 response, query path info response
    {
        .packet = (uint8_t const []) {
            0xff, 0x53, 0x4d, 0x42, 0x32, 0x34, 0x00, 0x00, 0xc0, 0x80, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xec, 0x1a, 0x64, 0x00, 0x19, 0x00,
            0x00, 0x00, 0x00
        },
        .size = 0x23,
        .ret = PROTO_OK,
        .way = FROM_CLIENT,
        .expected = {
            .info = { .head_len = CIFS_HEADER_SIZE, .payload = 0x23 - CIFS_HEADER_SIZE },
            .command = SMB_COM_TRANSACTION2,
            .status = SMB_STATUS_OBJECT_NAME_NOT_FOUND,
        },
    },

    // Trans2 request, find first query
    {
        .packet = (uint8_t const []) {
            0xff, 0x53, 0x4d, 0x42, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xec, 0x1a, 0x64, 0x00, 0x1b, 0x00,
            0x0f, 0x12, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x12, 0x00, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x13,
            0x00, 0x00, 0x17, 0x00, 0x96, 0x00, 0x06, 0x00, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x2f, 0x00,
            0x2a, 0x00, 0x00, 0x00
        },
        .size = 0x54,
        .ret = PROTO_OK,
        .way = FROM_CLIENT,
        .expected = {
            .info = { .head_len = CIFS_HEADER_SIZE, .payload = 0x54 - CIFS_HEADER_SIZE },
            .command = SMB_COM_TRANSACTION2,
            .set_values = SMB_TRANS2_SUBCMD | SMB_PATH,
            .trans2_subcmd = SMB_TRANS2_FIND_FIRST2,
            .status = SMB_STATUS_OK,
            .path = "/*",
        },
    },

    // Trans2 request, set_path_info query
    {
        .packet = (uint8_t const []) {
            0xff, 0x53, 0x4d, 0x42, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x62, 0x33, 0x64, 0x00, 0x89, 0x00,
            0x0f, 0x20, 0x00, 0x12, 0x00, 0x02, 0x00, 0xe8, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//                                        ParameterO                                                  Byte
            0x00, 0x00, 0x00, 0x20, 0x00, 0x44, 0x00, 0x12, 0x00, 0x64, 0x00, 0x01, 0x00, 0x06, 0x00, 0x35,
//          Coun  Padding---------
            0x00, 0x00, 0x00, 0x00, 0x09, 0x02, 0x00, 0x00, 0x00, 0x00, 0x2f, 0x00, 0x74, 0x00, 0x6d, 0x00,
            0x70, 0x00, 0x2f, 0x00, 0x74, 0x00, 0x65, 0x00, 0x73, 0x00, 0x74, 0x00, 0x2f, 0x00, 0x67, 0x00,
            0x61, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x52, 0x00, 0x00, 0x00, 0xed, 0x01, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x02
        },
        .size = 0x76,
        .ret = PROTO_OK,
        .way = FROM_CLIENT,
        .expected = {
            .info = { .head_len = CIFS_HEADER_SIZE, .payload = 0x76 - CIFS_HEADER_SIZE },
            .command = SMB_COM_TRANSACTION2,
            .set_values = SMB_TRANS2_SUBCMD | SMB_PATH,
            .trans2_subcmd = SMB_TRANS2_SET_PATH_INFORMATION,
            .status = SMB_STATUS_OK,
            .path = "/tmp/test/ga",
        },
    },

    // Trans2 response, set_path_info response
    {
        .packet = (uint8_t const []) {
            0xff, 0x53, 0x4d, 0x42, 0x32, 0x00, 0x00, 0x00, 0x00, 0x80, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x62, 0x33, 0x64, 0x00, 0x89, 0x00,
            0x0a, 0x02, 0x00, 0x70, 0x00, 0x00, 0x00, 0x02, 0x00, 0x38, 0x00, 0x00, 0x00, 0x70, 0x00, 0x3c,
//                                        BC--------  Padd  ErrorOffset Padding---  Flag-Field  FID-------
            0x00, 0x00, 0x00, 0x00, 0x00, 0x75, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x38, 0x18,
//          Create
            0x03, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc7, 0xae, 0x03, 0x2c, 0x00, 0x71, 0xcf, 0x01,
            0xd9, 0x8f, 0x8b, 0x05, 0x00, 0x71, 0xcf, 0x01, 0xc7, 0xae, 0x03, 0x2c, 0x00, 0x71, 0xcf, 0x01,
            0xe3, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0xa5, 0x13, 0xbc, 0x01, 0x00, 0x00, 0x00, 0x00, 0xff, 0x01, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        },
        .size = 0xac,
        .ret = PROTO_OK,
        .way = FROM_SERVER,
        .expected = {
            .info = { .head_len = CIFS_HEADER_SIZE, .payload = 0xac - CIFS_HEADER_SIZE },
            .command = SMB_COM_TRANSACTION2,
            .set_values = SMB_FID,
            .status = SMB_STATUS_OK,
            .fid = 0x1838,
        },
    },

    // Trans2 request, set_file_info
    {
        .packet = (uint8_t const []) {
            0xff, 0x53, 0x4d, 0x42, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x62, 0x33, 0x64, 0x00, 0x8a, 0x00,
            0x0f, 0x06, 0x00, 0x08, 0x00, 0x02, 0x00, 0xe8, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x06, 0x00, 0x44, 0x00, 0x08, 0x00, 0x4a, 0x00, 0x01, 0x00, 0x08, 0x00, 0x11,
            0x00, 0x00, 0x00, 0x00, 0x38, 0x18, 0xfc, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00
        },
        .size = 0x52,
        .ret = PROTO_OK,
        .way = FROM_CLIENT,
        .expected = {
            .info = { .head_len = CIFS_HEADER_SIZE, .payload = 0x52 - CIFS_HEADER_SIZE },
            .command = SMB_COM_TRANSACTION2,
            .set_values = SMB_FID | SMB_TRANS2_SUBCMD,
            .trans2_subcmd = SMB_TRANS2_SET_FILE_INFORMATION,
            .status = SMB_STATUS_OK,
            .fid = 0x1838,
        },
    },

    // Trans2 response, set_file_info
    {
        .packet = (uint8_t const []) {
            0xff, 0x53, 0x4d, 0x42, 0x32, 0x00, 0x00, 0x00, 0x00, 0x80, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x62, 0x33, 0x64, 0x00, 0x8a, 0x00,
            0x0a, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00
        },
        .size = 0x3a,
        .ret = PROTO_OK,
        .way = FROM_CLIENT,
        .expected = {
            .info = { .head_len = CIFS_HEADER_SIZE, .payload = 0x3a - CIFS_HEADER_SIZE },
            .command = SMB_COM_TRANSACTION2,
            .set_values = SMB_FID ,
            .status = SMB_STATUS_OK,
            .fid = 0x1838,
        },
    },

};

#define CHECK_SMB_SET(INFO, EXPECTED, MASK) \
    check_set_values(INFO->set_values, EXPECTED->set_values, MASK, #MASK);

static unsigned cur_test;

static bool compare_expected_cifs(struct cifs_proto_info const *const info,
        struct cifs_proto_info const *const expected)
{
    CHECK_INT(info->info.head_len, expected->info.head_len);
    CHECK_INT(info->info.payload, expected->info.payload);

    CHECK_SET_VALUE(info, expected, SMB_DOMAIN);
    CHECK_SET_VALUE(info, expected, SMB_USER);
    CHECK_SET_VALUE(info, expected, SMB_PATH);
    CHECK_SET_VALUE(info, expected, SMB_TRANS2_SUBCMD);
    CHECK_SET_VALUE(info, expected, SMB_FID);

    CHECK_INT(info->status, expected->status);
    CHECK_INT(info->command, expected->command);
    if (VALUES_ARE_SET(info, SMB_DOMAIN))
        CHECK_STR(info->domain, expected->domain);
    if (VALUES_ARE_SET(info, SMB_USER))
        CHECK_STR(info->user, expected->user);
    if (VALUES_ARE_SET(info, SMB_PATH))
        CHECK_STR(info->path, expected->path);
    if (VALUES_ARE_SET(info, SMB_TRANS2_SUBCMD))
        CHECK_INT(info->trans2_subcmd, expected->trans2_subcmd);
    if (VALUES_ARE_SET(info, SMB_FID))
        CHECK_INT(info->fid, expected->fid);

    return 0;
}

static void cifs_info_check(struct proto_subscriber unused_ *s, struct proto_info const *info_,
        size_t unused_ cap_len, uint8_t const unused_ *packet, struct timeval const unused_ *now)
{
    // Check info against parse_tests[cur_test].expected
    struct cifs_proto_info const *const info = DOWNCAST(info_, info, cifs_proto_info);
    struct cifs_proto_info const *const expected = &parse_tests[cur_test].expected;
    assert(!compare_expected_cifs(info, expected));
}

static void parse_check(void)
{
    struct timeval now;
    timeval_set_now(&now);
    struct parser *parser = proto_cifs->ops->parser_new(proto_cifs);
    struct proto_subscriber sub;
    hook_subscriber_ctor(&pkt_hook, &sub, cifs_info_check);

    for (cur_test = 0; cur_test < NB_ELEMS(parse_tests); cur_test++) {
        struct parse_test const *test = parse_tests + cur_test;
        printf("Check packet %d of size 0x%x (%d)\n", cur_test, test->size, test->size);
        enum proto_parse_status ret = cifs_parse(parser, NULL, test->way, test->packet, test->size,
                test->size, &now, test->size, test->packet);
        assert(ret == test->ret);
    }
}

static void compute_padding_check()
{
    struct cursor cursor = {.cap_len = 0x37 };
    assert(1 == compute_padding(&cursor, 0, 2));
    assert(0 == compute_padding(&cursor, 1, 2));
    assert(3 == compute_padding(&cursor, 0, 4));
}

int main(void)
{
    log_init();
    ext_init();
    mutex_init();
    objalloc_init();
    proto_init();
    pkt_wait_list_init();
    ref_init();
    cap_init();
    eth_init();
    ip_init();
    ip6_init();
    tcp_init();
    cifs_init();
    log_set_level(LOG_DEBUG, NULL);
    log_set_file("cifs_check.log");

    compute_padding_check();
    parse_check();

    cifs_fini();
    tcp_fini();
    ip6_fini();
    ip_fini();
    eth_fini();
    cap_fini();
    ref_fini();
    pkt_wait_list_fini();
    proto_fini();
    objalloc_fini();
    mutex_fini();
    ext_fini();
    log_fini();
    return EXIT_SUCCESS;
}

