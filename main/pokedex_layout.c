// main/pokedex_layout.c —— 掌上图鉴布局与短文案格式化。
#include "pokedex_layout.h"
#include "pokedex_core.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

void pokedex_layout_build(pokedex_layout_t *out)
{
    if (!out) return;
    memset(out, 0, sizeof(*out));

    /* 240x320 手持图鉴:顶栏下立绘区与概述约 2/3 : 1/3。
       立绘 144px(48px 3x,官方画布比例,不裁切)。
       顶边左编号 / 中名字 / 右属性(双属性纵向叠),底边左身高 / 右体重。 */
    out->screen      = (pokedex_rect_t){0, 0, 240, 320};
    out->header      = (pokedex_rect_t){0, 0, 240, 22};
    out->header_rule = (pokedex_rect_t){0, 22, 240, 2};
    out->title       = (pokedex_rect_t){8, 4, 64, 16};  /* "图鉴" / "POKEDEX" */
    out->progress    = (pokedex_rect_t){74, 4, 100, 16}; /* "1025/1025" 粗体 */
    out->battery     = (pokedex_rect_t){178, 4, 54, 16};

    out->sprite_frame = (pokedex_rect_t){8, 26, 224, 164};
    out->sprite_inner = (pokedex_rect_t){10, 28, 220, 160};
    out->sprite       = (pokedex_rect_t){48, 36, 144, 144};

    out->number_chip = (pokedex_rect_t){10, 28, 88, 16};
    out->number      = (pokedex_rect_t){12, 29, 84, 14};

    out->name_chip = (pokedex_rect_t){100, 28, 82, 16};
    out->name      = (pokedex_rect_t){102, 29, 78, 14};

    out->badge[0]    = (pokedex_rect_t){186, 28, POKEDEX_LAYOUT_BADGE_W,
                                        POKEDEX_LAYOUT_BADGE_H};
    out->badge[1]    = (pokedex_rect_t){186, 46, POKEDEX_LAYOUT_BADGE_W,
                                        POKEDEX_LAYOUT_BADGE_H};

    out->height      = (pokedex_rect_t){10, 170, 104, 18};
    out->height_text = (pokedex_rect_t){12, 171, 100, 16};
    out->weight      = (pokedex_rect_t){126, 170, 104, 18};
    out->weight_text = (pokedex_rect_t){128, 171, 100, 16};

    out->flavor_frame = (pokedex_rect_t){8, 192, 224, 80};
    out->flavor_inner = (pokedex_rect_t){10, 193, 220, 78};
    out->flavor_text  = (pokedex_rect_t){14, 195, 212, 74};

    out->tally_seen   = (pokedex_rect_t){8, 274, 224, 14};
    out->hint         = (pokedex_rect_t){8, 290, 224, 30};
}

void pokedex_list_layout_build(pokedex_list_layout_t *out)
{
    int i;

    if (!out) return;
    memset(out, 0, sizeof(*out));
    out->screen      = (pokedex_rect_t){0, 0, 240, 320};
    out->header      = (pokedex_rect_t){0, 0, 240, 22};
    out->header_rule = (pokedex_rect_t){0, 22, 240, 2};
    out->title       = (pokedex_rect_t){8, 4, 72, 16};
    out->progress    = (pokedex_rect_t){82, 4, 92, 16}; /* "I 025/151" */
    out->battery     = (pokedex_rect_t){178, 4, 54, 16};
    for (i = 0; i < POKEDEX_LIST_ROWS; i++) {
        out->row[i] = (pokedex_rect_t){8, (int16_t)(28 + i * 30), 224, 28};
    }
    out->tally_seen = (pokedex_rect_t){8, 260, 224, 18};
    out->hint       = (pokedex_rect_t){8, 280, 224, 38};
}

void pokedex_find_layout_build(pokedex_find_layout_t *out)
{
    int i;

    if (!out) return;
    memset(out, 0, sizeof(*out));
    out->screen      = (pokedex_rect_t){0, 0, 240, 320};
    out->header      = (pokedex_rect_t){0, 0, 240, 22};
    out->header_rule = (pokedex_rect_t){0, 22, 240, 2};
    out->title       = (pokedex_rect_t){8, 4, 120, 16};
    out->battery     = (pokedex_rect_t){178, 4, 54, 16};
    for (i = 0; i < 11; i++) {
        out->row[i] = (pokedex_rect_t){8, (int16_t)(26 + i * 22), 224, 20};
    }
    out->hint = (pokedex_rect_t){8, 280, 224, 38};
}

void pokedex_jump_layout_build(pokedex_jump_layout_t *out)
{
    int i;

    if (!out) return;
    memset(out, 0, sizeof(*out));
    out->screen      = (pokedex_rect_t){0, 0, 240, 320};
    out->header      = (pokedex_rect_t){0, 0, 240, 22};
    out->header_rule = (pokedex_rect_t){0, 22, 240, 2};
    out->title       = (pokedex_rect_t){8, 4, 120, 16};
    out->battery     = (pokedex_rect_t){178, 4, 54, 16};
    out->prompt      = (pokedex_rect_t){8, 80, 224, 24};
    for (i = 0; i < 4; i++) {
        out->digit[i] = (pokedex_rect_t){(int16_t)(25 + i * 50), 120, 40, 56};
    }
    out->hint = (pokedex_rect_t){8, 280, 224, 38};
}

bool pokedex_rect_in_bounds(pokedex_rect_t r, int w, int h)
{
    if (r.w <= 0 || r.h <= 0) return false;
    if (r.x < 0 || r.y < 0) return false;
    return (int)r.x + (int)r.w <= w && (int)r.y + (int)r.h <= h;
}

bool pokedex_rect_overlaps(pokedex_rect_t a, pokedex_rect_t b)
{
    return (int)a.x < (int)b.x + b.w && (int)b.x < (int)a.x + a.w &&
           (int)a.y < (int)b.y + b.h && (int)b.y < (int)a.y + a.h;
}

bool pokedex_rect_contains(pokedex_rect_t outer, pokedex_rect_t inner)
{
    return inner.x >= outer.x && inner.y >= outer.y &&
           inner.x + inner.w <= outer.x + outer.w &&
           inner.y + inner.h <= outer.y + outer.h;
}

int pokedex_layout_type_abbr(const char *type_name, char *buf, size_t cap)
{
    static const struct {
        const char *name;
        const char *abbr;
    } table[] = {
        {"bug", "BUG"},
        {"dark", "DRK"},
        {"dragon", "DRG"},
        {"electric", "ELE"},
        {"fairy", "FRY"},
        {"fighting", "FIG"},
        {"fire", "FIR"},
        {"flying", "FLY"},
        {"ghost", "GHO"},
        {"grass", "GRA"},
        {"ground", "GND"},
        {"ice", "ICE"},
        {"normal", "NOR"},
        {"poison", "PSN"},
        {"psychic", "PSY"},
        {"rock", "RCK"},
        {"steel", "STL"},
        {"water", "WAT"},
    };

    if (!buf || cap < 4) return 0;
    buf[0] = '\0';
    if (!type_name || !type_name[0]) return 0;

    for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
        if (strcmp(type_name, table[i].name) == 0) {
            memcpy(buf, table[i].abbr, 4);
            return 3;
        }
    }

    /* 未知属性:取前三字母并大写,保证徽章宽度仍是 3 字符。 */
    for (int i = 0; i < 3; i++) {
        char c = type_name[i];
        if (c == '\0') {
            buf[i] = '\0';
            return i;
        }
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        buf[i] = c;
    }
    buf[3] = '\0';
    return 3;
}

int pokedex_layout_format_no(uint32_t id, char *buf, size_t cap)
{
    if (!buf || !cap) return 0;
    return snprintf(buf, cap, "NO.%03u", (unsigned)id);
}

int pokedex_layout_format_progress(uint32_t id, char *buf, size_t cap)
{
    if (!buf || !cap) return 0;
    return snprintf(buf, cap, "%03u/%u", (unsigned)id, (unsigned)POKEDEX_DEX_LAST);
}

int pokedex_layout_format_caught(uint32_t n, char *buf, size_t cap)
{
    if (!buf || !cap) return 0;
    return snprintf(buf, cap, "%u CAUGHT", (unsigned)n);
}

int pokedex_layout_format_seen(uint32_t n, char *buf, size_t cap)
{
    if (!buf || !cap) return 0;
    return snprintf(buf, cap, "%u SEEN", (unsigned)n);
}

int pokedex_layout_format_stats(int height_dm, int weight_hg,
                                char *buf, size_t cap)
{
    char h[16];
    char w[16];

    if (!buf || !cap) return 0;
    pokedex_format_height(height_dm, h, sizeof(h));
    pokedex_format_weight(weight_hg, w, sizeof(w));
    return snprintf(buf, cap, "HT %s    WT %s", h, w);
}

size_t pokedex_layout_clip_desc(const char *src, char *dst, size_t dst_cap,
                                size_t max_chars)
{
    size_t n;
    size_t limit;

    if (!dst || dst_cap == 0) return 0;
    dst[0] = '\0';
    if (!src) return 0;

    if (max_chars == 0) max_chars = POKEDEX_LAYOUT_DESC_MAX;
    limit = max_chars;
    if (limit + 1 > dst_cap) limit = dst_cap - 1;

    n = strlen(src);
    if (n <= limit) {
        memcpy(dst, src, n + 1);
        return n;
    }
    if (limit < 3) {
        memcpy(dst, src, limit);
        dst[limit] = '\0';
        return limit;
    }
    {
        size_t cut = limit - 3;
        while (cut > 0 && ((unsigned char)src[cut] & 0xC0) == 0x80) cut--;
        memcpy(dst, src, cut);
        memcpy(dst + cut, "...", 4);
        return cut + 3;
    }
}

const char *pokedex_layout_roman_gen(uint32_t gen)
{
    static const char *roman[] = {
        "", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX",
    };
    if (gen < 1 || gen > 9) return "";
    return roman[gen];
}

int pokedex_layout_type_label(const char *type_name, pokedex_lang_t lang,
                              char *buf, size_t cap)
{
    static const struct {
        const char *name;
        const char *zh;
    } table[] = {
        {"bug", "虫"},
        {"dark", "恶"},
        {"dragon", "龙"},
        {"electric", "电"},
        {"fairy", "妖精"},
        {"fighting", "格斗"},
        {"fire", "火"},
        {"flying", "飞行"},
        {"ghost", "幽灵"},
        {"grass", "草"},
        {"ground", "地面"},
        {"ice", "冰"},
        {"normal", "一般"},
        {"poison", "毒"},
        {"psychic", "超能"},
        {"rock", "岩石"},
        {"steel", "钢"},
        {"water", "水"},
    };

    if (lang != POKEDEX_LANG_ZH) {
        return pokedex_layout_type_abbr(type_name, buf, cap);
    }
    if (!buf || cap < 7) return 0;
    buf[0] = '\0';
    if (!type_name || !type_name[0]) return 0;
    for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
        if (strcmp(type_name, table[i].name) == 0) {
            return snprintf(buf, cap, "%s", table[i].zh);
        }
    }
    return 0;
}

int pokedex_layout_format_title(pokedex_lang_t lang, char *buf, size_t cap)
{
    if (!buf || !cap) return 0;
    return snprintf(buf, cap, "%s", lang == POKEDEX_LANG_ZH ? "图鉴" : "POKEDEX");
}

int pokedex_layout_format_list_progress(uint32_t id, char *buf, size_t cap)
{
    uint32_t g;
    uint32_t first;
    uint32_t last;
    uint32_t idx;

    if (!buf || !cap) return 0;
    g = pokedex_generation(id);
    first = pokedex_gen_first(id);
    last = pokedex_gen_last(id);
    if (g == 0 || first == 0 || last == 0) {
        return snprintf(buf, cap, "--");
    }
    idx = id - first + 1u;
    return snprintf(buf, cap, "%s %03u/%u",
                    pokedex_layout_roman_gen(g),
                    (unsigned)idx, (unsigned)(last - first + 1u));
}

int pokedex_layout_format_seen_lang(uint32_t n, pokedex_lang_t lang,
                                    char *buf, size_t cap)
{
    if (!buf || !cap) return 0;
    if (lang == POKEDEX_LANG_ZH) {
        return snprintf(buf, cap, "已见 %u", (unsigned)n);
    }
    return pokedex_layout_format_seen(n, buf, cap);
}

int pokedex_layout_format_stats_lang(int height_dm, int weight_hg,
                                     pokedex_lang_t lang,
                                     char *buf, size_t cap)
{
    char h[16];
    char w[16];

    if (!buf || !cap) return 0;
    pokedex_format_height(height_dm, h, sizeof(h));
    pokedex_format_weight(weight_hg, w, sizeof(w));
    if (lang == POKEDEX_LANG_ZH) {
        return snprintf(buf, cap, "身高 %s   体重 %s", h, w);
    }
    return snprintf(buf, cap, "HT %s   WT %s", h, w);
}

int pokedex_layout_format_height_lang(int height_dm, pokedex_lang_t lang,
                                      char *buf, size_t cap)
{
    char h[16];

    if (!buf || !cap) return 0;
    pokedex_format_height(height_dm, h, sizeof(h));
    if (lang == POKEDEX_LANG_ZH) {
        return snprintf(buf, cap, "身高 %s", h);
    }
    return snprintf(buf, cap, "HT %s", h);
}

int pokedex_layout_format_weight_lang(int weight_hg, pokedex_lang_t lang,
                                      char *buf, size_t cap)
{
    char w[16];

    if (!buf || !cap) return 0;
    pokedex_format_weight(weight_hg, w, sizeof(w));
    if (lang == POKEDEX_LANG_ZH) {
        return snprintf(buf, cap, "体重 %s", w);
    }
    return snprintf(buf, cap, "WT %s", w);
}

void pokedex_flavor_scroll_init(pokedex_flavor_scroll_t *s,
                                int content_h, int view_h)
{
    if (!s) return;
    memset(s, 0, sizeof(*s));
    if (content_h < 0) content_h = 0;
    if (view_h < 0) view_h = 0;
    if (content_h > view_h) {
        s->max_y = (int16_t)(content_h - view_h);
        s->dir = 1;
        s->hold = POKEDEX_FLAVOR_HOLD_TICKS;
    }
}

bool pokedex_flavor_scroll_active(const pokedex_flavor_scroll_t *s)
{
    return s != NULL && s->max_y > 0;
}

int16_t pokedex_flavor_scroll_tick(pokedex_flavor_scroll_t *s)
{
    if (!s || s->max_y <= 0) return 0;
    if (s->hold > 0) {
        s->hold--;
        return s->y;
    }
    s->y = (int16_t)(s->y + (int16_t)s->dir * (int16_t)POKEDEX_FLAVOR_STEP_PX);
    if (s->y >= s->max_y) {
        s->y = s->max_y;
        s->dir = -1;
        s->hold = POKEDEX_FLAVOR_HOLD_TICKS;
    } else if (s->y <= 0) {
        s->y = 0;
        s->dir = 1;
        s->hold = POKEDEX_FLAVOR_HOLD_TICKS;
    }
    return s->y;
}

int pokedex_layout_format_gen_line(uint32_t gen, pokedex_lang_t lang,
                                   char *buf, size_t cap)
{
    uint32_t first;
    uint32_t last;

    if (!buf || !cap) return 0;
    first = pokedex_gen_first_n(gen);
    last = pokedex_gen_last_n(gen);
    if (first == 0 || last == 0) {
        buf[0] = '\0';
        return 0;
    }
    if (lang == POKEDEX_LANG_ZH) {
        return snprintf(buf, cap, "第%u世代  %u-%u",
                        (unsigned)gen, (unsigned)first, (unsigned)last);
    }
    return snprintf(buf, cap, "%-5s %u-%u",
                    pokedex_layout_roman_gen(gen),
                    (unsigned)first, (unsigned)last);
}

int pokedex_layout_format_jump_item(pokedex_lang_t lang, char *buf, size_t cap)
{
    if (!buf || !cap) return 0;
    return snprintf(buf, cap, "%s",
                    lang == POKEDEX_LANG_ZH ? "跳到编号" : "GO TO #");
}

int pokedex_layout_format_name_item(pokedex_lang_t lang, char *buf, size_t cap)
{
    if (!buf || !cap) return 0;
    return snprintf(buf, cap, "%s",
                    lang == POKEDEX_LANG_ZH ? "按字母" : "A-Z NAME");
}

int pokedex_layout_format_letter_line(char letter, uint32_t id, const char *name,
                                      char *buf, size_t cap)
{
    if (!buf || !cap) return 0;
    if (!name) name = "";
    if (letter < 'A' || letter > 'Z') letter = '?';
    return snprintf(buf, cap, "%c  #%03u  %s",
                    letter, (unsigned)id, name);
}

bool pokedex_layout_fit_sprite_rgb565(const uint16_t *src, uint32_t w, uint32_t h,
                                      uint16_t bg, uint16_t *dst,
                                      uint32_t dw, uint32_t dh,
                                      size_t dst_cap_u16)
{
    uint32_t min_x = w, min_y = h, max_x = 0, max_y = 0;
    uint32_t y, x, fy, fx, bw, bh, factor, ox, oy, i;

    if (!src || !dst || w == 0 || h == 0 || dw == 0 || dh == 0) return false;
    if ((uint64_t)dw * dh > dst_cap_u16) return false;
    if (src == dst) return false;

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            if (src[y * w + x] == bg) continue;
            if (x < min_x) min_x = x;
            if (y < min_y) min_y = y;
            if (x > max_x) max_x = x;
            if (y > max_y) max_y = y;
        }
    }
    if (min_x > max_x || min_y > max_y) {
        min_x = 0;
        min_y = 0;
        max_x = w - 1;
        max_y = h - 1;
    }
    bw = max_x - min_x + 1;
    bh = max_y - min_y + 1;
    factor = dw / bw;
    if (dh / bh < factor) factor = dh / bh;
    if (factor == 0) factor = 1;
    if (factor > 8u) factor = 8u;

    for (i = 0; i < dw * dh; i++) dst[i] = bg;
    ox = (dw - bw * factor) / 2u;
    oy = (dh - bh * factor) / 2u;
    for (y = 0; y < bh; y++) {
        for (x = 0; x < bw; x++) {
            uint16_t px = src[(min_y + y) * w + (min_x + x)];
            uint32_t dx0 = ox + x * factor;
            uint32_t dy0 = oy + y * factor;
            for (fy = 0; fy < factor; fy++) {
                uint32_t row = (dy0 + fy) * dw + dx0;
                for (fx = 0; fx < factor; fx++) {
                    dst[row + fx] = px;
                }
            }
        }
    }
    return true;
}

bool pokedex_layout_scale_nn_rgb565(const uint16_t *src, uint32_t w, uint32_t h,
                                    uint32_t factor, uint16_t *dst,
                                    size_t dst_cap_u16,
                                    uint32_t *out_w, uint32_t *out_h)
{
    uint32_t dw;
    uint32_t dh;
    uint32_t y;
    uint32_t x;
    uint32_t fy;
    uint32_t fx;

    if (!src || !dst || !out_w || !out_h || w == 0 || h == 0 || factor == 0) {
        return false;
    }
    if (factor > 8u) return false;
    if (w > (UINT32_MAX / factor) || h > (UINT32_MAX / factor)) return false;
    dw = w * factor;
    dh = h * factor;
    if ((uint64_t)dw * dh > dst_cap_u16) return false;

    /* 从右下往左上写,src==dst 时也不会覆盖尚未读取的源像素。 */
    for (y = h; y-- > 0; ) {
        for (x = w; x-- > 0; ) {
            uint16_t px = src[y * w + x];
            uint32_t dx0 = x * factor;
            uint32_t dy0 = y * factor;
            for (fy = 0; fy < factor; fy++) {
                uint32_t row = (dy0 + fy) * dw + dx0;
                for (fx = 0; fx < factor; fx++) {
                    dst[row + fx] = px;
                }
            }
        }
    }
    *out_w = dw;
    *out_h = dh;
    return true;
}

bool pokedex_layout_scale2x_rgb565(const uint16_t *src, uint32_t w, uint32_t h,
                                   uint16_t *dst, size_t dst_cap_u16,
                                   uint32_t *out_w, uint32_t *out_h)
{
    return pokedex_layout_scale_nn_rgb565(src, w, h, 2u, dst, dst_cap_u16,
                                          out_w, out_h);
}

bool pokedex_layout_dark_ink(uint32_t rgb888)
{
    /* Rec. 601 粗略亮度;阈值偏高,让电黄/冰蓝/钢灰走深字。 */
    unsigned r = (rgb888 >> 16) & 0xFFu;
    unsigned g = (rgb888 >> 8) & 0xFFu;
    unsigned b = rgb888 & 0xFFu;
    unsigned y = (r * 299u + g * 587u + b * 114u) / 1000u;
    return y >= 160u;
}
