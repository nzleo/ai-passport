// main/pokedex_layout.h —— 掌上图鉴 240x320 布局几何。
// 纯 C,不依赖 ESP-IDF/LVGL,可在宿主机单测(tests/test_pokedex_layout.c)。
// 详情页把 144px 精灵井放到顶栏下方居中,编号/名字/属性跟在立绘下面,底部给
// 概述和见过计数。所有矩形必须落在屏内且兄弟区域不相交。
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define POKEDEX_LAYOUT_W            240
#define POKEDEX_LAYOUT_H            320
#define POKEDEX_LAYOUT_SPRITE_PX    144  /* 48px 像素图最近邻 3x */
#define POKEDEX_LAYOUT_SPRITE_SCALE 3u
#define POKEDEX_LAYOUT_BADGE_W      44   /* 3 字母 / 2 汉字属性芯片 */
#define POKEDEX_LAYOUT_BADGE_H      18
#define POKEDEX_LAYOUT_DESC_MAX     90   /* 概述约 2–3 行,14px */
#define POKEDEX_LIST_ROWS         7

typedef enum {
    POKEDEX_LANG_EN = 0,
    POKEDEX_LANG_ZH = 1,
} pokedex_lang_t;

typedef struct {
    int16_t x, y, w, h;
} pokedex_rect_t;

typedef struct {
    pokedex_rect_t screen;
    pokedex_rect_t header;
    pokedex_rect_t header_rule;
    pokedex_rect_t title;
    pokedex_rect_t progress;
    pokedex_rect_t battery;
    pokedex_rect_t sprite_frame;
    pokedex_rect_t sprite_inner;
    pokedex_rect_t sprite;
    pokedex_rect_t number_chip;
    pokedex_rect_t number;
    pokedex_rect_t name;
    pokedex_rect_t badge[2];
    pokedex_rect_t stats;
    pokedex_rect_t stats_text;
    pokedex_rect_t flavor_frame;
    pokedex_rect_t flavor_inner;
    pokedex_rect_t flavor_text;
    pokedex_rect_t tally_seen;
    pokedex_rect_t hint;
} pokedex_layout_t;

typedef struct {
    pokedex_rect_t screen;
    pokedex_rect_t header;
    pokedex_rect_t header_rule;
    pokedex_rect_t title;
    pokedex_rect_t progress;
    pokedex_rect_t battery;
    pokedex_rect_t row[POKEDEX_LIST_ROWS];
    pokedex_rect_t tally_seen;
    pokedex_rect_t hint;
} pokedex_list_layout_t;

typedef struct {
    pokedex_rect_t screen;
    pokedex_rect_t header;
    pokedex_rect_t header_rule;
    pokedex_rect_t title;
    pokedex_rect_t battery;
    pokedex_rect_t row[11]; /* 跳号 + 按字母 + 9 世代 */
    pokedex_rect_t hint;
} pokedex_find_layout_t;

typedef struct {
    pokedex_rect_t screen;
    pokedex_rect_t header;
    pokedex_rect_t header_rule;
    pokedex_rect_t title;
    pokedex_rect_t battery;
    pokedex_rect_t prompt;
    pokedex_rect_t digit[4];
    pokedex_rect_t hint;
} pokedex_jump_layout_t;

// 填入固定的掌上图鉴骨架。调用方可直接拿矩形去放 LVGL 对象。
void pokedex_layout_build(pokedex_layout_t *out);
void pokedex_list_layout_build(pokedex_list_layout_t *out);
void pokedex_find_layout_build(pokedex_find_layout_t *out);
void pokedex_jump_layout_build(pokedex_jump_layout_t *out);

bool pokedex_rect_in_bounds(pokedex_rect_t r, int w, int h);
bool pokedex_rect_overlaps(pokedex_rect_t a, pokedex_rect_t b);
bool pokedex_rect_contains(pokedex_rect_t outer, pokedex_rect_t inner);

// 属性英文名 → 3 字母大写标签("electric"→"ELE")。buf 至少 4 字节。
// 未知名取前三字母并大写;空输入写入 ""。返回写入长度(不含 NUL)。
int pokedex_layout_type_abbr(const char *type_name, char *buf, size_t cap);

int pokedex_layout_format_no(uint32_t id, char *buf, size_t cap);
int pokedex_layout_format_progress(uint32_t id, char *buf, size_t cap);
int pokedex_layout_format_caught(uint32_t n, char *buf, size_t cap);
int pokedex_layout_format_seen(uint32_t n, char *buf, size_t cap);
int pokedex_layout_format_stats(int height_dm, int weight_hg,
                                char *buf, size_t cap);

const char *pokedex_layout_roman_gen(uint32_t gen);
int pokedex_layout_type_label(const char *type_name, pokedex_lang_t lang,
                              char *buf, size_t cap);
int pokedex_layout_format_title(pokedex_lang_t lang, char *buf, size_t cap);
int pokedex_layout_format_list_progress(uint32_t id, char *buf, size_t cap);
int pokedex_layout_format_seen_lang(uint32_t n, pokedex_lang_t lang,
                                    char *buf, size_t cap);
int pokedex_layout_format_stats_lang(int height_dm, int weight_hg,
                                     pokedex_lang_t lang,
                                     char *buf, size_t cap);
int pokedex_layout_format_gen_line(uint32_t gen, pokedex_lang_t lang,
                                   char *buf, size_t cap);
int pokedex_layout_format_jump_item(pokedex_lang_t lang, char *buf, size_t cap);
int pokedex_layout_format_name_item(pokedex_lang_t lang, char *buf, size_t cap);
int pokedex_layout_format_letter_line(char letter, uint32_t id, const char *name,
                                      char *buf, size_t cap);

// 超长概述截断并加 "...";max_chars==0 时用 POKEDEX_LAYOUT_DESC_MAX。
size_t pokedex_layout_clip_desc(const char *src, char *dst, size_t dst_cap,
                                size_t max_chars);

// 去掉等于 bg 的透明边后再最近邻放大,居中铺进 dw×dh(先填 bg)。
// src 与 dst 不得重叠。找不到非背景像素时退回整幅图。
bool pokedex_layout_fit_sprite_rgb565(const uint16_t *src, uint32_t w, uint32_t h,
                                      uint16_t bg, uint16_t *dst,
                                      uint32_t dw, uint32_t dh,
                                      size_t dst_cap_u16);

// 最近邻整数倍放大 RGB565。允许 src 与 dst 为同一缓冲(从后往前写)。
// 输出尺寸为 (w*factor,h*factor);factor==0、缓冲不足或 w/h 为 0 返回 false。
bool pokedex_layout_scale_nn_rgb565(const uint16_t *src, uint32_t w, uint32_t h,
                                    uint32_t factor, uint16_t *dst,
                                    size_t dst_cap_u16,
                                    uint32_t *out_w, uint32_t *out_h);

// scale_nn(..., 2, ...) 的别名,保留给既有测试。
bool pokedex_layout_scale2x_rgb565(const uint16_t *src, uint32_t w, uint32_t h,
                                   uint16_t *dst, size_t dst_cap_u16,
                                   uint32_t *out_w, uint32_t *out_h);

// 浅色属性底(电/冰/钢等)用深字,深色底用白字,保证 3 字母芯片可读。
bool pokedex_layout_dark_ink(uint32_t rgb888);
