// -*- c-basic-offset: 4; c-backslash-column: 79; indent-tabs-mode: nil -*-
// vim:sw=4 ts=4 sts=4 expandtab
#ifndef CURSOR_H_100107
#define CURSOR_H_100107
#include <inttypes.h>
#include <stdbool.h>
#include <junkie/proto/proto.h>

/** @file
 * @brief helper for serialized data streram
 */

struct cursor {
    uint8_t const *head;
    size_t cap_len;     // remaining length that can be read
};

void cursor_ctor(struct cursor *, uint8_t const *head, size_t cap_len);

void cursor_rollback(struct cursor *, size_t n);
uint_least8_t cursor_read_u8(struct cursor *);
uint_least16_t cursor_read_u16n(struct cursor *);
uint_least16_t cursor_read_u16(struct cursor *);
uint_least32_t cursor_read_u24(struct cursor *);
uint_least32_t cursor_read_u32n(struct cursor *);
uint_least32_t cursor_read_u32(struct cursor *);
uint_least64_t cursor_read_u64(struct cursor *);

/// Reads a string if possible
/** @returns a tempstr with the (beginning of the) string.
 * @param max_len the maximum number of bytes to read. If it's reached before the end of string (nul) then
 *                PROTO_TOO_SHORT is returned (and the cursor is rollbacked).
 * @param str will be set to the tempstr.  */
enum proto_parse_status cursor_read_string(struct cursor *, char **str, size_t max_len);

void cursor_drop(struct cursor *, size_t);

bool cursor_is_empty(struct cursor const *);

#define CHECK_LEN(cursor, x, rollback) do { \
    if ((cursor)->cap_len  < (x)) { cursor_rollback(cursor, rollback); return PROTO_TOO_SHORT; } \
} while(0)

#endif