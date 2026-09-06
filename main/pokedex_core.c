// main/pokedex_core.c —— Pokédex 核心模型的纯 C 实现,无 ESP-IDF 依赖。
// 含:图鉴状态位图/序列化、URL 与格式化、精灵图缩放、流式 JSON 字段提取。
#include "pokedex_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DECKSIZE_BYTES POKEDEX_BITMAP_BYTES /* (1025 + 7) / 8 */

_Static_assert(POKEDEX_DEX_SIZE == 1025u, "dex size assumed 1025");
_Static_assert(POKEDEX_BITMAP_BYTES >= ((POKEDEX_DEX_SIZE + 7u) / 8u),
               "bitmap must cover every national-dex id");
_Static_assert(sizeof(pokedex_state_t) == POKEDEX_STATE_BLOB_SIZE,
               "state layout must stay 272 bytes");

// ============================================================================
// 图鉴状态位图
// ============================================================================

static bool bit_get(const uint8_t *map, uint32_t id)
{
    if (!pokedex_id_in_range(id)) return false;
    return (map[(id - 1) >> 3] >> ((id - 1) & 7)) & 1u;
}

static void bit_set(uint8_t *map, uint32_t id, bool value)
{
    if (!pokedex_id_in_range(id)) return;
    uint8_t mask = (uint8_t)(1u << ((id - 1) & 7));
    if (value) {
        map[(id - 1) >> 3] |= mask;
    } else {
        map[(id - 1) >> 3] &= (uint8_t)~mask;
    }
}

void pokedex_state_init(pokedex_state_t *st)
{
    memset(st, 0, sizeof(*st));
    st->magic = POKEDEX_STATE_MAGIC;
    st->last_id = POKEDEX_DEX_FIRST;
}

bool pokedex_id_in_range(uint32_t id)
{
    return id >= POKEDEX_DEX_FIRST && id <= POKEDEX_DEX_LAST;
}

bool pokedex_is_caught(const pokedex_state_t *st, uint32_t id)
{
    return bit_get(st->caught, id);
}

void pokedex_set_caught(pokedex_state_t *st, uint32_t id, bool caught)
{
    bit_set(st->caught, id, caught);
}

bool pokedex_is_seen(const pokedex_state_t *st, uint32_t id)
{
    return bit_get(st->seen, id);
}

void pokedex_mark_seen(pokedex_state_t *st, uint32_t id)
{
    bit_set(st->seen, id, true);
}

uint32_t pokedex_count_caught(const pokedex_state_t *st)
{
    uint32_t n = 0;
    for (uint32_t id = POKEDEX_DEX_FIRST; id <= POKEDEX_DEX_LAST; id++) {
        n += bit_get(st->caught, id) ? 1u : 0u;
    }
    return n;
}

uint32_t pokedex_count_seen(const pokedex_state_t *st)
{
    uint32_t n = 0;
    for (uint32_t id = POKEDEX_DEX_FIRST; id <= POKEDEX_DEX_LAST; id++) {
        n += bit_get(st->seen, id) ? 1u : 0u;
    }
    return n;
}

uint32_t pokedex_step(uint32_t id, int32_t delta)
{
    if (!pokedex_id_in_range(id)) id = POKEDEX_DEX_FIRST;
    int32_t next = (int32_t)id + delta;
    if (next < (int32_t)POKEDEX_DEX_FIRST) next = (int32_t)POKEDEX_DEX_LAST;
    if (next > (int32_t)POKEDEX_DEX_LAST) next = (int32_t)POKEDEX_DEX_FIRST;
    return (uint32_t)next;
}

/* 全国图鉴世代边界(含):I 1-151, II 152-251, III 252-386, IV 387-493,
   V 494-649, VI 650-721, VII 722-809, VIII 810-905, IX 906-1025。 */
static const uint16_t GEN_LAST[POKEDEX_GEN_COUNT] = {
    151, 251, 386, 493, 649, 721, 809, 905, 1025,
};

uint32_t pokedex_generation(uint32_t id)
{
    if (!pokedex_id_in_range(id)) return 0;
    for (uint32_t g = 0; g < POKEDEX_GEN_COUNT; g++) {
        if (id <= GEN_LAST[g]) return g + 1u;
    }
    return 0;
}

uint32_t pokedex_gen_first_n(uint32_t gen)
{
    if (gen < 1u || gen > POKEDEX_GEN_COUNT) return 0;
    if (gen == 1u) return POKEDEX_DEX_FIRST;
    return (uint32_t)GEN_LAST[gen - 2u] + 1u;
}

uint32_t pokedex_gen_last_n(uint32_t gen)
{
    if (gen < 1u || gen > POKEDEX_GEN_COUNT) return 0;
    return (uint32_t)GEN_LAST[gen - 1u];
}

uint32_t pokedex_gen_first(uint32_t id)
{
    return pokedex_gen_first_n(pokedex_generation(id));
}

uint32_t pokedex_gen_last(uint32_t id)
{
    return pokedex_gen_last_n(pokedex_generation(id));
}

uint32_t pokedex_step_gen(uint32_t id, int32_t dir)
{
    uint32_t g = pokedex_generation(id);
    if (g == 0) g = 1;
    if (dir == 0) return pokedex_gen_first(id ? id : POKEDEX_DEX_FIRST);
    if (dir < 0) {
        g = (g == 1u) ? POKEDEX_GEN_COUNT : (g - 1u);
    } else {
        g = (g == POKEDEX_GEN_COUNT) ? 1u : (g + 1u);
    }
    return pokedex_gen_first_n(g);
}

uint32_t pokedex_step_in_gen(uint32_t id, int32_t delta)
{
    uint32_t first;
    uint32_t last;
    int32_t span;
    int32_t off;

    if (!pokedex_id_in_range(id)) id = POKEDEX_DEX_FIRST;
    first = pokedex_gen_first(id);
    last = pokedex_gen_last(id);
    span = (int32_t)(last - first + 1u);
    off = (int32_t)(id - first) + delta;
    off %= span;
    if (off < 0) off += span;
    return first + (uint32_t)off;
}

uint32_t pokedex_window_first(uint32_t id, uint32_t first, uint32_t last,
                              uint32_t rows)
{
    uint32_t span;
    uint32_t focus;
    int32_t start;

    if (last < first) return first;
    if (rows == 0) {
        if (id < first) return first;
        if (id > last) return last;
        return id;
    }
    if (id < first) id = first;
    if (id > last) id = last;
    span = last - first + 1u;
    if (span <= rows) return first;
    focus = (rows > 2u) ? 2u : 0u;
    start = (int32_t)id - (int32_t)focus;
    if (start < (int32_t)first) start = (int32_t)first;
    if (start + (int32_t)rows - 1 > (int32_t)last) {
        start = (int32_t)last - (int32_t)rows + 1;
    }
    return (uint32_t)start;
}

uint32_t pokedex_list_window_first(uint32_t id, uint32_t rows)
{
    if (!pokedex_id_in_range(id)) id = POKEDEX_DEX_FIRST;
    return pokedex_window_first(id, pokedex_gen_first(id),
                                pokedex_gen_last(id), rows);
}

uint32_t pokedex_find_step(uint32_t sel, int32_t delta)
{
    int32_t n = (int32_t)POKEDEX_FIND_COUNT;
    int32_t v = (int32_t)(sel % (uint32_t)n) + delta;
    v %= n;
    if (v < 0) v += n;
    return (uint32_t)v;
}

uint32_t pokedex_find_sel_for_id(uint32_t id)
{
    uint32_t g = pokedex_generation(id);
    return (g == 0u) ? (POKEDEX_FIND_NAME + 1u) : (g + 1u);
}

uint32_t pokedex_find_sel_to_gen(uint32_t sel)
{
    if (sel <= POKEDEX_FIND_NAME || sel >= POKEDEX_FIND_COUNT) return 0;
    return sel - 1u;
}

char pokedex_en_initial(const char *raw)
{
    if (!raw) return '?';
    for (; *raw; raw++) {
        char c = *raw;
        if (c >= 'a' && c <= 'z') return (char)(c - 'a' + 'A');
        if (c >= 'A' && c <= 'Z') return c;
    }
    return '?';
}

uint32_t pokedex_step_key_in_range(uint32_t id, int32_t dir,
                                   uint32_t first, uint32_t last,
                                   uint32_t (*key_of)(uint32_t))
{
    uint32_t key;
    uint32_t i;
    int32_t step;

    if (!key_of || last < first) return id;
    if (id < first || id > last) id = first;
    if (dir == 0) return id;
    step = (dir < 0) ? -1 : 1;
    key = key_of(id);
    i = id;
    do {
        int32_t n = (int32_t)i + step;
        if (n < (int32_t)first) n = (int32_t)last;
        if (n > (int32_t)last) n = (int32_t)first;
        i = (uint32_t)n;
        if (i == id) return id;
    } while (key_of(i) == key);
    if (step < 0) {
        uint32_t k2 = key_of(i);
        for (;;) {
            int32_t p = (int32_t)i - 1;
            if (p < (int32_t)first) break;
            if (key_of((uint32_t)p) != k2) break;
            i = (uint32_t)p;
        }
    }
    return i;
}

uint32_t pokedex_jump_clamp(uint32_t n)
{
    if (n < POKEDEX_DEX_FIRST) return POKEDEX_DEX_FIRST;
    if (n > POKEDEX_DEX_LAST) return POKEDEX_DEX_LAST;
    return n;
}

void pokedex_jump_to_digits(uint32_t n, uint8_t d[4])
{
    if (!d) return;
    n = pokedex_jump_clamp(n);
    d[0] = (uint8_t)((n / 1000u) % 10u);
    d[1] = (uint8_t)((n / 100u) % 10u);
    d[2] = (uint8_t)((n / 10u) % 10u);
    d[3] = (uint8_t)(n % 10u);
}

uint32_t pokedex_jump_from_digits(const uint8_t d[4])
{
    uint32_t n;

    if (!d) return POKEDEX_DEX_FIRST;
    n = (uint32_t)d[0] * 1000u + (uint32_t)d[1] * 100u
      + (uint32_t)d[2] * 10u + (uint32_t)d[3];
    return pokedex_jump_clamp(n);
}

uint32_t pokedex_jump_nudge_digit(uint32_t n, unsigned pos, int32_t dir)
{
    uint8_t d[4];

    if (pos > 3u) pos = 3u;
    pokedex_jump_to_digits(n, d);
    {
        int32_t v = (int32_t)d[pos] + (dir < 0 ? -1 : 1);
        if (v < 0) v = 9;
        if (v > 9) v = 0;
        d[pos] = (uint8_t)v;
    }
    return pokedex_jump_from_digits(d);
}

unsigned pokedex_jump_cursor_move(unsigned pos, int32_t dir)
{
    if (pos > 3u) pos = 3u;
    if (dir < 0) return pos == 0u ? 3u : pos - 1u;
    return pos == 3u ? 0u : pos + 1u;
}

// ============================================================================
// 序列化(定长 272 字节 v2;仍可读 52 字节 v1)
// ============================================================================

size_t pokedex_state_serialize(const pokedex_state_t *st, uint8_t *buf, size_t cap)
{
    if (!st || !buf || cap < POKEDEX_STATE_BLOB_SIZE) return 0;

    /* 显式逐字段拷贝,不使用结构体 memcpy,避免将来加字段时隐式改布局。 */
    size_t off = 0;
    memcpy(buf + off, &st->magic, sizeof(st->magic));
    off += sizeof(st->magic);
    memcpy(buf + off, &st->last_id, sizeof(st->last_id));
    off += sizeof(st->last_id);
    memcpy(buf + off, &st->save_seq, sizeof(st->save_seq));
    off += sizeof(st->save_seq);
    memcpy(buf + off, st->caught, sizeof(st->caught));
    off += sizeof(st->caught);
    memcpy(buf + off, st->seen, sizeof(st->seen));
    off += sizeof(st->seen);
    return off; /* == POKEDEX_STATE_BLOB_SIZE */
}

bool pokedex_state_deserialize(pokedex_state_t *st, const uint8_t *buf, size_t len)
{
    uint32_t magic;

    if (!st || !buf || len < 4) return false;
    memcpy(&magic, buf, sizeof(magic));

    if (magic == POKEDEX_STATE_MAGIC) {
        pokedex_state_t tmp;
        size_t off = 0;

        if (len < POKEDEX_STATE_BLOB_SIZE) return false;
        memcpy(&tmp.magic, buf + off, sizeof(tmp.magic));
        off += sizeof(tmp.magic);
        memcpy(&tmp.last_id, buf + off, sizeof(tmp.last_id));
        off += sizeof(tmp.last_id);
        memcpy(&tmp.save_seq, buf + off, sizeof(tmp.save_seq));
        off += sizeof(tmp.save_seq);
        memcpy(tmp.caught, buf + off, sizeof(tmp.caught));
        off += sizeof(tmp.caught);
        memcpy(tmp.seen, buf + off, sizeof(tmp.seen));
        off += sizeof(tmp.seen);
        (void)off;
        if (!pokedex_id_in_range(tmp.last_id)) return false;
        *st = tmp;
        return true;
    }

    /* v1:20 字节位图只覆盖 1..151,迁入 v2 高位清零。 */
    if (magic == POKEDEX_STATE_V1_MAGIC) {
        pokedex_state_t tmp;
        uint32_t last_id = 0, save_seq = 0;
        uint8_t caught_v1[POKEDEX_STATE_V1_BITMAP];
        uint8_t seen_v1[POKEDEX_STATE_V1_BITMAP];
        size_t off = 4;

        if (len < POKEDEX_STATE_V1_BLOB_SIZE) return false;
        memcpy(&last_id, buf + off, 4);
        off += 4;
        memcpy(caught_v1, buf + off, POKEDEX_STATE_V1_BITMAP);
        off += POKEDEX_STATE_V1_BITMAP;
        memcpy(seen_v1, buf + off, POKEDEX_STATE_V1_BITMAP);
        off += POKEDEX_STATE_V1_BITMAP;
        memcpy(&save_seq, buf + off, 4);
        if (last_id < POKEDEX_DEX_FIRST || last_id > 151u) return false;

        pokedex_state_init(&tmp);
        tmp.last_id = last_id;
        tmp.save_seq = save_seq;
        memcpy(tmp.caught, caught_v1, POKEDEX_STATE_V1_BITMAP);
        memcpy(tmp.seen, seen_v1, POKEDEX_STATE_V1_BITMAP);
        *st = tmp;
        return true;
    }

    return false;
}

// ============================================================================
// URL 与格式化
// ============================================================================

size_t pokedex_api_url(uint32_t id, char *buf, size_t cap)
{
    if (!buf || !cap) return 0;
    int n = snprintf(buf, cap, "https://pokeapi.co/api/v2/pokemon/%u",
                     (unsigned)id);
    if (n < 0 || (size_t)n >= cap) return 0; /* 截断视为失败 */
    return (size_t)n;
}

size_t pokedex_sprite_url(uint32_t id, char *buf, size_t cap)
{
    if (!buf || !cap) return 0;
    int n = snprintf(buf, cap,
                     "https://raw.githubusercontent.com/PokeAPI/sprites/"
                     "master/sprites/pokemon/%u.png",
                     (unsigned)id);
    if (n < 0 || (size_t)n >= cap) return 0;
    return (size_t)n;
}

int pokedex_format_number(uint32_t id, char *buf, size_t cap)
{
    if (!buf || !cap) return 0;
    return snprintf(buf, cap, "#%03u", (unsigned)id);
}

int pokedex_format_height(int decimeters, char *buf, size_t cap)
{
    if (!buf || !cap) return 0;
    if (decimeters < 0) decimeters = 0;
    return snprintf(buf, cap, "%d.%d m", decimeters / 10, decimeters % 10);
}

int pokedex_format_weight(int hectograms, char *buf, size_t cap)
{
    if (!buf || !cap) return 0;
    if (hectograms < 0) hectograms = 0;
    return snprintf(buf, cap, "%d.%d kg", hectograms / 10, hectograms % 10);
}

int pokedex_pretty_name(const char *raw, char *buf, size_t cap)
{
    if (!raw || !buf || !cap) return 0;
    size_t in = 0, out = 0;
    int written = 0;
    bool new_word = true;

    while (raw[in] != '\0' && out + 1 < cap) {
        char c = raw[in++];
        if (c == '-') {
            /* 连字符 = 词边界:输出空格并让下一个词首字母大写。 */
            if (out + 1 < cap) {
                buf[out++] = ' ';
                written++;
            }
            new_word = true;
            continue;
        }
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a'); /* 归一化小写 */
        if (new_word && c >= 'a' && c <= 'z') {
            c = (char)(c - 'a' + 'A');
            new_word = false;
        } else if (c != ' ') {
            new_word = false;
        }
        buf[out++] = c;
        written++;
    }
    buf[out] = '\0';
    return written;
}

bool pokedex_fit_scaled(uint32_t sw, uint32_t sh, uint32_t max_w, uint32_t max_h,
                        uint32_t *dw, uint32_t *dh)
{
    if (!sw || !sh || !max_w || !max_h || !dw || !dh) return false;
    if (sw <= max_w && sh <= max_h) {
        *dw = sw;
        *dh = sh;
        return true;
    }
    /* 千分级缩放因子,保留一点整数精度;结果至少 1px。 */
    uint32_t scale = (max_w * 1000u / sw < max_h * 1000u / sh)
                         ? max_w * 1000u / sw
                         : max_h * 1000u / sh;
    if (scale == 0) scale = 1;
    *dw = (sw * scale + 500u) / 1000u;
    *dh = (sh * scale + 500u) / 1000u;
    if (*dw == 0) *dw = 1;
    if (*dh == 0) *dh = 1;
    return true;
}

// ============================================================================
// 流式 JSON 字段提取器
//
// 逐字节 FSM,只识别需要的最小文法子集:
//   - 字符串(含 \\" \\uXXXX 等转义;键名不做解码,值做常用转义解码)
//   - 数字(整型,用于 id/height/weight)
//   - 容器 '{' '[' '}' ']' 与分隔符 ':' ',' (空白在 MARK 态被跳过)
// 任意分块喂入,块间状态保留在 ctx;摘出字段在 pokedex_entry_t 里,不驻留全文。
// ============================================================================

enum {
    PJSON_S_MARK = 0, /* 等待下一个 token */
    PJSON_S_STR,      /* 字符串内(键或值,由 s_str_is_key 区分) */
    PJSON_S_NUM,      /* 数字内 */
    PJSON_S_STR_ESC,  /* 字符串内刚遇到 '\\' */
};

/* last 字段的取值(见 pokedex_json_t.last)。 */
enum {
    PJSON_L_NONE = 0,
    PJSON_L_OBJ_OPEN,
    PJSON_L_ARR_OPEN,
    PJSON_L_OBJ_CLOSE,
    PJSON_L_ARR_CLOSE,
    PJSON_L_COLON,
    PJSON_L_COMMA,
    PJSON_L_STR_END,
    PJSON_L_NUM_END,
};

/* 容器标记:区分 sprites 对象与 types 数组及其子对象,防误抓同名键。 */
enum {
    PM_NONE = 0,
    PM_SPRITES_OBJ, /* "sprites" 的值对象 */
    PM_TYPES_ARR,   /* "types" 的值数组 */
    PM_TYPES_ELEM,  /* types 数组元素对象 */
    PM_TYPE_OBJ,    /* element.type 的值对象 */
};

/* 等待捕获的槽位。 */
enum {
    PP_NONE = 0,
    PP_ID,
    PP_NAME,
    PP_HEIGHT,
    PP_WEIGHT,
    PP_FRONT,
    PP_TYPE_NAME,
};



#define PJSON_DIGIT(c) ((c) >= '0' && (c) <= '9')

void pokedex_json_init(pokedex_json_t *ctx, pokedex_entry_t *out)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->out = out;
    ctx->mode = PJSON_S_MARK;
    ctx->last = PJSON_L_NONE;
}

static void json_set_broken(pokedex_json_t *c)
{
    c->broken = true;
}

/* 键名匹配(目标键均为纯 ASCII,不做转义解码)。 */
static bool json_key_is(const pokedex_json_t *c, const char *key)
{
    size_t n = strlen(key);
    return c->key_len == n && memcmp(c->key, key, n) == 0;
}

/* 数字结束:解析并提交给当前槽位。 */
static void json_num_end(pokedex_json_t *c)
{
    if (c->num_len == 0) return;
    c->num[c->num_len < sizeof(c->num) ? c->num_len : sizeof(c->num) - 1] = '\0';
    long v = strtol(c->num, NULL, 10);
    if (c->pending == PP_ID && c->out) c->out->id = (uint32_t)v;
    if (c->pending == PP_HEIGHT && c->out) c->out->height_dm = (int)v;
    if (c->pending == PP_WEIGHT && c->out) c->out->weight_hg = (int)v;
    if (c->pending == PP_ID) c->have_id = true;
    if (c->pending == PP_HEIGHT) c->have_height = true;
    if (c->pending == PP_WEIGHT) c->have_weight = true;
    c->pending = PP_NONE;
    c->num_len = 0;
    c->last = PJSON_L_NUM_END;
}

/* 字符串(值)结束:提交给当前槽位。 */
static void json_str_end(pokedex_json_t *c)
{
    if (c->val_len < sizeof(c->val)) c->val[c->val_len] = '\0';
    if (c->pending == PP_NAME && c->out) {
        snprintf(c->out->name, sizeof(c->out->name), "%.*s",
                 (int)sizeof(c->out->name) - 1, c->val);
        c->have_name = true;
    } else if (c->pending == PP_FRONT && c->out) {
        snprintf(c->out->sprite_url, sizeof(c->out->sprite_url), "%.*s",
                 (int)sizeof(c->out->sprite_url) - 1, c->val);
        c->have_front = true;
    } else if (c->pending == PP_TYPE_NAME && c->out) {
        if (c->out->type_count < 2) { /* 只取前两种属性 */
            /* 显式限长,避免编译器的 format-truncation 告警。 */
            snprintf(c->out->types[c->out->type_count],
                     sizeof(c->out->types[0]), "%.*s",
                     (int)sizeof(c->out->types[0]) - 1, c->val);
            c->out->type_count++;
        }
    }
    c->pending = PP_NONE;
    c->val_len = 0;
    c->last = PJSON_L_STR_END;
}

/* 键名结束:识别目标键,为随后的值设定捕获槽位。 */
static void json_key_end(pokedex_json_t *c)
{
    const bool top = (c->depth == 1 && c->stack[0] == '{');
    c->last = PJSON_L_STR_END;

    if (top) {
        if (json_key_is(c, "id")) {
            c->pending = PP_ID;
        } else if (json_key_is(c, "name")) {
            c->pending = PP_NAME;
        } else if (json_key_is(c, "height")) {
            c->pending = PP_HEIGHT;
        } else if (json_key_is(c, "weight")) {
            c->pending = PP_WEIGHT;
        } else if (json_key_is(c, "sprites")) {
            c->pending_sprites_obj = true;
        } else if (json_key_is(c, "types")) {
            c->pending_types_arr = true;
        }
        return;
    }

    /* depth==2:sprites 对象内(标记 PM_SPRITES_OBJ)的 front_default。 */
    if (c->depth == 2 && json_key_is(c, "front_default") &&
        c->marks[c->depth - 1] == PM_SPRITES_OBJ) {
        c->pending = PP_FRONT;
        return;
    }

    /* types 数组元素对象(depth 3)里的 "type" 键:
       容器链 顶层obj(1) -> types数组(2) -> 元素对象(3)。 */
    if (c->depth == 3 && json_key_is(c, "type") &&
        c->marks[c->depth - 1] == PM_TYPES_ELEM) {
        c->pending_type_obj = true;
        return;
    }

    /* element.type 对象(depth 4)里的 "name" 键(可捕获两个)。 */
    if (c->depth == 4 && json_key_is(c, "name") &&
        c->marks[c->depth - 1] == PM_TYPE_OBJ) {
        c->pending = PP_TYPE_NAME;
        return;
    }
}

bool pokedex_json_feed(pokedex_json_t *c, const uint8_t *data, size_t len)
{
    if (!c || !data) return false;
    if (c->broken || c->done) return true;

    for (size_t i = 0; i < len && !c->broken && !c->done; i++) {
        unsigned char ch = data[i];

        switch (c->mode) {
        case PJSON_S_STR:
            if (c->esc_u_rem) { /* \\u 里剩余的 4 个十六进制位:跳过不入缓冲 */
                c->esc_u_rem--;
                break;
            }
            if (ch == '"') {
                if (c->str_is_key) json_key_end(c);
                else json_str_end(c);
                c->mode = PJSON_S_MARK;
            } else if (ch == '\\') {
                c->in_escape = true;
                c->mode = PJSON_S_STR_ESC;
            } else if (c->str_is_key) {
                if (c->key_len + 1 < sizeof(c->key)) c->key[c->key_len++] = (char)ch;
            } else {
                if (c->val_len + 1 < sizeof(c->val)) c->val[c->val_len++] = (char)ch;
            }
            break;

        case PJSON_S_STR_ESC:
            c->in_escape = false;
            if (ch == 'u') {
                c->esc_u_rem = 4;
            } else {
                char dec = ch;
                switch (ch) {
                case '"': dec = '"'; break;
                case '\\': dec = '\\'; break;
                case '/': dec = '/'; break;
                case 'b': dec = '\b'; break;
                case 'f': dec = '\f'; break;
                case 'n': dec = '\n'; break;
                case 'r': dec = '\r'; break;
                case 't': dec = '\t'; break;
                default: dec = ch; break;
                }
                if (c->str_is_key) {
                    if (c->key_len + 1 < sizeof(c->key)) c->key[c->key_len++] = dec;
                } else {
                    if (c->val_len + 1 < sizeof(c->val)) c->val[c->val_len++] = dec;
                }
            }
            c->mode = PJSON_S_STR;
            break;

        case PJSON_S_NUM:
            if (PJSON_DIGIT(ch)) {
                if (c->num_len + 1 < sizeof(c->num)) c->num[c->num_len++] = (char)ch;
            } else {
                json_num_end(c);
                c->mode = PJSON_S_MARK;
                i--; /* 让结束符走 MARK 分支 */
            }
            break;

        case PJSON_S_MARK:
        default:
            if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') break;

            if (ch == '"') {
                /* 键 = 对象内紧跟 '{' 或 ',';值 = 紧跟 ':'。 */
                c->str_is_key = (c->last == PJSON_L_OBJ_OPEN ||
                                 c->last == PJSON_L_COMMA);
                c->key_len = 0;
                c->val_len = 0;
                c->in_escape = false;
                c->mode = PJSON_S_STR;
                break;
            }
            if (ch == ':') {
                c->last = PJSON_L_COLON;
                break;
            }
            if (ch == ',') {
                c->last = PJSON_L_COMMA;
                break;
            }
            if (ch == '{' || ch == '[') {
                if (c->depth >= POKEDEX_JSON_MAX_DEPTH) {
                    json_set_broken(c);
                    break;
                }
                const bool is_obj = (ch == '{');
                c->stack[c->depth] = is_obj ? '{' : '[';
                uint8_t mark = PM_NONE;
                if (is_obj && c->pending_sprites_obj) {
                    mark = PM_SPRITES_OBJ;
                    c->pending_sprites_obj = false;
                } else if (!is_obj && c->pending_types_arr) {
                    mark = PM_TYPES_ARR;
                    c->pending_types_arr = false;
                } else if (is_obj && c->depth > 0 &&
                           c->marks[c->depth - 1] == PM_TYPES_ARR) {
                    mark = PM_TYPES_ELEM;
                } else if (is_obj && c->pending_type_obj) {
                    mark = PM_TYPE_OBJ;
                    c->pending_type_obj = false;
                }
                c->marks[c->depth] = mark;
                c->depth++;
                c->last = is_obj ? PJSON_L_OBJ_OPEN : PJSON_L_ARR_OPEN;
                break;
            }
            if (ch == '}' || ch == ']') {
                if (c->depth == 0 ||
                    (ch == '}' && c->stack[c->depth - 1] != '{') ||
                    (ch == ']' && c->stack[c->depth - 1] != '[')) {
                    json_set_broken(c);
                    break;
                }
                c->depth--;
                c->marks[c->depth] = PM_NONE;
                c->last = (ch == '}') ? PJSON_L_OBJ_CLOSE : PJSON_L_ARR_CLOSE;
                break;
            }
            if ((PJSON_DIGIT(ch) || ch == '-') && c->last == PJSON_L_COLON) {
                c->num_len = 0;
                c->mode = PJSON_S_NUM;
                if (c->num_len + 1 < sizeof(c->num)) c->num[c->num_len++] = (char)ch;
                break;
            }
            break; /* 其他字符(true/false/null 等)忽略 */
        }
    }

    /* 汇总完成条件:全部必需字段到手。 */
    c->done = c->have_id && c->have_name && c->have_height && c->have_weight &&
              c->have_front && c->out && c->out->type_count >= 1;
    return !c->broken;
}

bool pokedex_json_done(const pokedex_json_t *ctx)
{
    return ctx ? ctx->done : false;
}

bool pokedex_json_broken(const pokedex_json_t *ctx)
{
    return ctx ? ctx->broken : true;
}
