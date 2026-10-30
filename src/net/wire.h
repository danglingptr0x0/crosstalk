#ifndef CROSSTALK_NET_WIRE_H
#define CROSSTALK_NET_WIRE_H

#include <stdint.h>

#include "net/steam.h"

#define CROSSTALK_NET_WIRE_TAG 0xC7
#define CROSSTALK_NET_WIRE_MAGIC 0x4B4C5458
#define CROSSTALK_NET_WIRE_REV 1
#define CROSSTALK_NET_WIRE_RELIABLE 1
#define CROSSTALK_NET_WIRE_UNRELIABLE 0
#define CROSSTALK_NET_WIRE_SEQ_STEP 1
#define CROSSTALK_NET_WIRE_NO_PAYLOAD 0

typedef enum crosstalk_net_wire_kind
{
    CROSSTALK_NET_WIRE_KIND_NONE = 0,
    CROSSTALK_NET_WIRE_KIND_YELLO,
    CROSSTALK_NET_WIRE_KIND_POSE,
    CROSSTALK_NET_WIRE_KIND_SNAPSHOT_REQ,
    CROSSTALK_NET_WIRE_KIND_SNAPSHOT,
    CROSSTALK_NET_WIRE_KIND_CMD,
    CROSSTALK_NET_WIRE_KIND_BYE,
    CROSSTALK_NET_WIRE_KIND_CMD_ACK,
    CROSSTALK_NET_WIRE_KIND_CMD_REJECT,
    CROSSTALK_NET_WIRE_KIND_DISTRICTS,
    CROSSTALK_NET_WIRE_KIND_PING,
    CROSSTALK_NET_WIRE_KIND_CHAT,
    CROSSTALK_NET_WIRE_KIND_ECON,
    CROSSTALK_NET_WIRE_KIND_ECON_TAX,
    CROSSTALK_NET_WIRE_KIND_CREDIT,
    CROSSTALK_NET_WIRE_KIND_PHYS,
    CROSSTALK_NET_WIRE_KIND_PIN,
    CROSSTALK_NET_WIRE_KIND_AUTHOR,
    CROSSTALK_NET_WIRE_KIND_PLAYER,
    CROSSTALK_NET_WIRE_KIND_MATCH,
    CROSSTALK_NET_WIRE_KIND_ATTACK,
    CROSSTALK_NET_WIRE_KIND_IDENT,
    CROSSTALK_NET_WIRE_KIND_PEER_STATE,
    CROSSTALK_NET_WIRE_KIND_SERIAL_CTL,
    CROSSTALK_NET_WIRE_KIND_SERIAL_DATA
} crosstalk_net_wire_kind_t;

#define CROSSTALK_NET_WIRE_SERIAL_ID_CAP 192
#define CROSSTALK_NET_WIRE_SERIAL_CHUNK_CAP 512
#define CROSSTALK_NET_WIRE_SERIAL_CTL_GRANT 0
#define CROSSTALK_NET_WIRE_SERIAL_CTL_REVOKE 1
#define CROSSTALK_NET_WIRE_SERIAL_DIR_RX 0
#define CROSSTALK_NET_WIRE_SERIAL_DIR_TX 1

typedef struct crosstalk_net_wire_serial_ctl
{
    uint32_t op;
    uint8_t pudding[4];
    char id[CROSSTALK_NET_WIRE_SERIAL_ID_CAP];
} crosstalk_net_wire_serial_ctl_t;

typedef struct crosstalk_net_wire_serial_data
{
    uint32_t dir;
    uint32_t len;
    uint8_t bytes[CROSSTALK_NET_WIRE_SERIAL_CHUNK_CAP];
} crosstalk_net_wire_serial_data_t;

#define CROSSTALK_NET_WIRE_SERIAL_DATA_HDR_SIZE (sizeof(uint32_t) * 0x2)

typedef struct crosstalk_net_wire_ident
{
    uint64_t steam_id;
    uint64_t discord_id;
} crosstalk_net_wire_ident_t;

#define CROSSTALK_NET_WIRE_PEER_STATE_ACTIVE 0
#define CROSSTALK_NET_WIRE_PEER_STATE_PAUSED 1
#define CROSSTALK_NET_WIRE_PEER_STATE_AFK 2

typedef struct crosstalk_net_wire_peer_state
{
    uint64_t steam_id;
    uint64_t idle_ms;
    uint8_t state;
    uint8_t focused;
    uint8_t pudding[6];
} crosstalk_net_wire_peer_state_t;

#define CROSSTALK_NET_WIRE_ATTACK_REF_CAP 16

typedef enum crosstalk_net_wire_attack_kind
{
    CROSSTALK_NET_WIRE_ATTACK_NONE = 0,
    CROSSTALK_NET_WIRE_ATTACK_TRANSIENT
} crosstalk_net_wire_attack_kind_t;

typedef struct crosstalk_net_wire_attack
{
    double magnitude;
    double secs;
    uint32_t kind;
    uint32_t pudding;
    char ref[CROSSTALK_NET_WIRE_ATTACK_REF_CAP];
} crosstalk_net_wire_attack_t;

#define CROSSTALK_NET_WIRE_PLAYER_NAME_CAP 64

typedef enum crosstalk_net_wire_role
{
    CROSSTALK_NET_WIRE_ROLE_NONE = 0,
    CROSSTALK_NET_WIRE_ROLE_CITY,
    CROSSTALK_NET_WIRE_ROLE_NATURE
} crosstalk_net_wire_role_t;

typedef enum crosstalk_net_wire_submode
{
    CROSSTALK_NET_WIRE_SUBMODE_PARTY = 0,
    CROSSTALK_NET_WIRE_SUBMODE_ADVERSARIAL
} crosstalk_net_wire_submode_t;

typedef enum crosstalk_net_wire_match_state
{
    CROSSTALK_NET_WIRE_MATCH_STATE_IDLE = 0,
    CROSSTALK_NET_WIRE_MATCH_STATE_LIVE,
    CROSSTALK_NET_WIRE_MATCH_STATE_CITY_WON,
    CROSSTALK_NET_WIRE_MATCH_STATE_NATURE_WON
} crosstalk_net_wire_match_state_t;

typedef struct crosstalk_net_wire_player
{
    uint64_t steam_id;
    uint32_t role;
    uint32_t pudding;
    char name[CROSSTALK_NET_WIRE_PLAYER_NAME_CAP];
} crosstalk_net_wire_player_t;

typedef struct crosstalk_net_wire_match
{
    double secs_remain;
    double budget;
    double rate;
    uint32_t submode;
    uint32_t state;
} crosstalk_net_wire_match_t;

#define CROSSTALK_NET_WIRE_AUTHOR_NAME_CAP 64
#define CROSSTALK_NET_WIRE_BOARD_NAME_CAP 64

typedef struct crosstalk_net_wire_author
{
    uint64_t steam_id;
    uint64_t created_epoch_ms;
    uint64_t bench_ms_total;
    char name[CROSSTALK_NET_WIRE_AUTHOR_NAME_CAP];
    char board_name[CROSSTALK_NET_WIRE_BOARD_NAME_CAP];
    uint64_t sim_ms_total;
} crosstalk_net_wire_author_t;

#define CROSSTALK_NET_WIRE_PIN_REF_CAP 16
#define CROSSTALK_NET_WIRE_PIN_PIN_CAP 16
#define CROSSTALK_NET_WIRE_PIN_NAME_CAP 32

typedef enum crosstalk_net_wire_pin_op
{
    CROSSTALK_NET_WIRE_PIN_OP_ADD = 0,
    CROSSTALK_NET_WIRE_PIN_OP_TYPE,
    CROSSTALK_NET_WIRE_PIN_OP_DEL
} crosstalk_net_wire_pin_op_t;

typedef struct crosstalk_net_wire_pin
{
    uint32_t op;
    uint32_t net;
    uint32_t pin_type;
    uint32_t pudding;
    char ref[CROSSTALK_NET_WIRE_PIN_REF_CAP];
    char pin[CROSSTALK_NET_WIRE_PIN_PIN_CAP];
    char pin_name[CROSSTALK_NET_WIRE_PIN_NAME_CAP];
} crosstalk_net_wire_pin_t;

typedef struct crosstalk_net_wire_credit
{
    uint32_t rst;
    uint32_t entry_cunt;
    uint32_t seg_cunt;
    uint32_t log_cunt;
} crosstalk_net_wire_credit_t;

typedef struct crosstalk_net_wire_tax
{
    double tax_v;
} crosstalk_net_wire_tax_t;

typedef struct crosstalk_net_wire_ping
{
    double world_x;
    double world_z;
} crosstalk_net_wire_ping_t;

#define CROSSTALK_NET_WIRE_CHAT_TEXT_CAP 240

typedef struct crosstalk_net_wire_chat
{
    char text[CROSSTALK_NET_WIRE_CHAT_TEXT_CAP];
} crosstalk_net_wire_chat_t;

#define CROSSTALK_NET_WIRE_ACK_REF_CAP 16

typedef struct crosstalk_net_wire_cmd_ack
{
    uint64_t orig;
    uint32_t seq;
    uint32_t reason;
    char ref[CROSSTALK_NET_WIRE_ACK_REF_CAP];
} crosstalk_net_wire_cmd_ack_t;

typedef struct crosstalk_net_wire_chunk
{
    uint32_t total_len;
    uint32_t offset;
    uint32_t part_len;
    uint32_t pudding;
} crosstalk_net_wire_chunk_t;

typedef struct crosstalk_net_wire_hdr
{
    uint8_t tag;
    uint8_t kind;
    uint16_t rev;
    uint32_t magic;
    uint32_t len;
    uint32_t seq;
    uint64_t sender;
} crosstalk_net_wire_hdr_t;

uint32_t crosstalk_net_wire_send(steam_net_ctx_t *ctx, uint8_t kind, const void *payload, uint32_t len, uint8_t reliable);
uint32_t crosstalk_net_wire_blob_send(steam_net_ctx_t *ctx, uint8_t kind, const void *blob, uint32_t len);
uint32_t crosstalk_net_wire_blob_recv(steam_net_ctx_t *ctx, const uint8_t *payload, uint32_t len, uint8_t *out_complete);
uint32_t crosstalk_net_wire_relay(steam_net_ctx_t *ctx, const void *data, uint32_t len, uint8_t reliable);
uint32_t crosstalk_net_wire_decode(const void *data, uint32_t len, crosstalk_net_wire_hdr_t *out_hdr, const uint8_t **out_payload);

#endif
