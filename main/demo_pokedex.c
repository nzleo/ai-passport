// main/demo_pokedex.c —— 宝可梦图鉴(Pokédex)页面【纯离线版】。
//
// 数据与图片全部内置固件(生成产物 tools/gen_pokedex_static.py,来源 PokeAPI
// CC-BY 4.0):全国图鉴 1..1025(第 I–IX 世代)的基础数据 + 48x48 像素精灵图。
// 查询:默认列表(世代内单击翻 1、双击按英文首字母跳组);长按 UP/DOWN
// 打开查找(跳号 + 按字母 + 九世代)。详情页看图鉴与叫声。
// OK 双击切换中/英(NVS 记住)。列表页长按 OK 回菜单(main.c);
// 其它页长按 OK 退回列表。
// 见过/上次查看/语言保存在 NVS(命名空间 "pokedex"),掉电不丢失。
// 叫声在 cryfs 分区,Opus 8 kbps;解码与 I2S 写入在独立任务,按键回调不阻塞。
#include "demo.h"
#include "pokedex_core.h"
#include "pokedex_layout.h"
#include "pokedex_sprite.h"
#include "pokedex_static.h"
#include "pokedex_cry_play.h"
#include "chinese_14.h"
#include "bsp_battery.h"
#include "bsp_display.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lvgl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define POKEDEX_NVS_NS        "pokedex"
#define POKEDEX_NVS_KEY_STATE "state"
#define POKEDEX_NVS_KEY_LANG  "lang"

#define POKEDEX_WORKER_STACK 2048
#define POKEDEX_WORKER_PRIO  5

#define POKEDEX_IDLE_DIM_S      60
#define POKEDEX_FLAVOR_TICK_MS  80

#define CS_BG     0x244238u
#define CS_BG_DK  0x1A3730u
#define CS_FRAME  0x467A6Au
#define CS_BRIGHT 0x5BC0A8u
#define CS_TEXT   0xFFFFFFu
#define CS_DIM    0x90A99Au
#define CS_WARN   0xF0C070u

typedef enum {
    VIEW_LIST = 0,
    VIEW_DETAIL,
    VIEW_FIND,
    VIEW_JUMP,
    VIEW_NAME,
} pokedex_view_t;

static const uint32_t TYPE_COLORS[POKEDEX_STATIC_TYPE_COUNT] = {
    [POKEDEX_STATIC_TYPE_BUG]      = 0xA6B91A,
    [POKEDEX_STATIC_TYPE_DARK]     = 0x705746,
    [POKEDEX_STATIC_TYPE_DRAGON]   = 0x6F35FC,
    [POKEDEX_STATIC_TYPE_ELECTRIC] = 0xF7D02C,
    [POKEDEX_STATIC_TYPE_FAIRY]    = 0xD685AD,
    [POKEDEX_STATIC_TYPE_FIGHTING] = 0xC22E28,
    [POKEDEX_STATIC_TYPE_FIRE]     = 0xEE8130,
    [POKEDEX_STATIC_TYPE_FLYING]   = 0xA98FF3,
    [POKEDEX_STATIC_TYPE_GHOST]    = 0x735797,
    [POKEDEX_STATIC_TYPE_GRASS]    = 0x7AC74C,
    [POKEDEX_STATIC_TYPE_GROUND]   = 0xE2BF65,
    [POKEDEX_STATIC_TYPE_ICE]      = 0x96D9D6,
    [POKEDEX_STATIC_TYPE_NORMAL]   = 0xA8A77A,
    [POKEDEX_STATIC_TYPE_POISON]   = 0xA33EA1,
    [POKEDEX_STATIC_TYPE_PSYCHIC]  = 0xF95587,
    [POKEDEX_STATIC_TYPE_ROCK]     = 0xB6A136,
    [POKEDEX_STATIC_TYPE_STEEL]    = 0xB7B7CE,
    [POKEDEX_STATIC_TYPE_WATER]    = 0x6390F0,
};

static void apply_rect(lv_obj_t *o, pokedex_rect_t r)
{
    lv_obj_set_pos(o, r.x, r.y);
    lv_obj_set_size(o, r.w, r.h);
}

static lv_obj_t *flag_block(lv_obj_t *parent, pokedex_rect_t r,
                            uint32_t color, int radius)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    apply_rect(o, r);
    lv_obj_set_style_radius(o, radius, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_set_style_bg_color(o, lv_color_hex(color), 0);
    return o;
}

static const lv_font_t *ui_font(void);

static lv_obj_t *label_at(lv_obj_t *parent, pokedex_rect_t r, uint32_t color)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, ui_font(), 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    apply_rect(l, r);
    return l;
}

static lv_obj_t *badge_create(lv_obj_t *parent, pokedex_rect_t r, uint32_t color)
{
    lv_obj_t *o = flag_block(parent, r, color, 6);
    lv_obj_set_style_border_width(o, 2, 0);
    lv_obj_set_style_border_color(o, lv_color_hex(CS_FRAME), 0);
    lv_obj_t *l = lv_label_create(o);
    lv_obj_set_style_text_font(l, ui_font(), 0);
    lv_obj_set_style_text_color(l, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(l);
    lv_label_set_text(l, "-");
    return o;
}

static lv_obj_t      *s_scr;
static lv_obj_t      *s_sprite;
static lv_obj_t      *s_sprite_hint;
static lv_obj_t      *s_name, *s_height, *s_weight;
static lv_obj_t      *s_battery, *s_progress, *s_title;
static lv_obj_t      *s_no;
static lv_obj_t      *s_badge[2];
static lv_obj_t      *s_flavor_clip;
static lv_obj_t      *s_desc;
static lv_obj_t      *s_tally_seen;
static lv_obj_t      *s_hint;
static lv_obj_t      *s_row_bg[POKEDEX_LIST_ROWS];
static lv_obj_t      *s_row_lab[POKEDEX_LIST_ROWS];
static lv_obj_t      *s_find_bg[11];
static lv_obj_t      *s_find_lab[11];
static lv_obj_t      *s_digit_bg[4];
static lv_obj_t      *s_digit_lab[4];
static lv_obj_t      *s_jump_prompt;
static lv_timer_t    *s_bat_timer;
static lv_timer_t    *s_idle_timer;
static lv_timer_t    *s_flavor_timer;
static pokedex_flavor_scroll_t s_flavor_scroll;
static uint8_t        s_idle_sec;
static pokedex_layout_t s_lay;
static pokedex_list_layout_t s_list_lay;
static pokedex_find_layout_t s_find_lay;
static pokedex_jump_layout_t s_jump_lay;

static uint16_t       s_sprite_pixels[POKEDEX_SPRITE_MAX_BYTES / 2];
static uint16_t       s_sprite_src[POKEDEX_STATIC_SPRITE_BYTES / 2];
static lv_image_dsc_t s_sprite_dsc;

static TaskHandle_t    s_worker;
static volatile bool   s_exit;
static pokedex_state_t s_state;
static volatile bool   s_state_dirty;
static volatile bool   s_lang_dirty;
static pokedex_view_t  s_view;
static pokedex_lang_t  s_lang;
static uint32_t        s_find_sel;
static uint32_t        s_jump_value;
static unsigned        s_jump_cursor;
static char            s_name_letter[26];
static uint16_t        s_name_first[26];
static uint8_t         s_name_count;
static uint8_t         s_name_sel;

static const char *TAG = "pokedex";

static const lv_font_t *ui_font(void)
{
    return &chinese_14;
}

static void upper(char *s)
{
    for (; *s; s++) {
        if (*s >= 'a' && *s <= 'z') *s = (char)(*s - 'a' + 'A');
    }
}

static void format_species_name(uint32_t id, char *buf, size_t cap)
{
    const pokedex_static_entry_t *e = &pokedex_static_dex[id];
    if (s_lang == POKEDEX_LANG_ZH && e->name_zh[0]) {
        snprintf(buf, cap, "%s", e->name_zh);
        return;
    }
    if (pokedex_pretty_name(e->name, buf, cap) > 0) upper(buf);
}

static const char *hint_text(pokedex_view_t v)
{
    if (s_lang == POKEDEX_LANG_ZH) {
        switch (v) {
        case VIEW_LIST:   return "确定打开  双击按字母\n长按查找  确定x2 中英";
        case VIEW_DETAIL: return "确定叫声  双击跳10\n长按查找  确定x2 中英";
        case VIEW_FIND:   return "确定选择  长按返回";
        case VIEW_JUMP:   return "确定跳转  双击返回\n上下改位";
        case VIEW_NAME:   return "确定跳转  长按返回";
        }
    }
    switch (v) {
    case VIEW_LIST:   return "OK OPEN  x2 LETTER\nHOLD FIND  OK x2 LANG";
    case VIEW_DETAIL: return "OK CRY  x2 SKIP 10\nHOLD FIND  OK x2 LANG";
    case VIEW_FIND:   return "OK SELECT  HOLD BACK";
    case VIEW_JUMP:   return "OK GO  x2 BACK\nUP/DN DIGIT";
    case VIEW_NAME:   return "OK GO  HOLD BACK";
    }
    return "";
}

static uint32_t id_en_key(uint32_t id)
{
    if (!pokedex_id_in_range(id)) return (uint32_t)'?';
    return (uint32_t)pokedex_en_initial(pokedex_static_dex[id].name);
}

static void name_index_rebuild(void)
{
    uint8_t n = 0;
    char c;

    for (c = 'A'; c <= 'Z'; c++) {
        uint32_t id;
        for (id = POKEDEX_DEX_FIRST; id <= POKEDEX_DEX_LAST; id++) {
            if (pokedex_en_initial(pokedex_static_dex[id].name) == c) {
                s_name_letter[n] = c;
                s_name_first[n] = (uint16_t)id;
                n++;
                break;
            }
        }
    }
    s_name_count = n;
}

static void name_select_current(void)
{
    char cur = pokedex_en_initial(pokedex_static_dex[s_state.last_id].name);
    uint8_t i;

    s_name_sel = 0;
    for (i = 0; i < s_name_count; i++) {
        if (s_name_letter[i] == cur) {
            s_name_sel = i;
            break;
        }
    }
}

static void state_load(void)
{
    pokedex_state_init(&s_state);
    s_lang = POKEDEX_LANG_ZH;
    nvs_handle_t h;
    if (nvs_open(POKEDEX_NVS_NS, NVS_READONLY, &h) != ESP_OK) return;
    uint8_t blob[POKEDEX_STATE_BLOB_SIZE];
    size_t len = sizeof(blob);
    if (nvs_get_blob(h, POKEDEX_NVS_KEY_STATE, blob, &len) == ESP_OK) {
        pokedex_state_t tmp;
        if (pokedex_state_deserialize(&tmp, blob, len)) s_state = tmp;
    }
    uint8_t lang = 0xFF;
    if (nvs_get_u8(h, POKEDEX_NVS_KEY_LANG, &lang) == ESP_OK && lang <= 1) {
        s_lang = (pokedex_lang_t)lang;
    }
    nvs_close(h);
}

static void state_persist(void)
{
    if (!s_state_dirty && !s_lang_dirty) return;
    uint8_t blob[POKEDEX_STATE_BLOB_SIZE];
    bool captured = false;
    bool write_state = false;
    bool write_lang = false;
    pokedex_lang_t lang = POKEDEX_LANG_EN;
    if (bsp_lvgl_lock(200)) {
        if (s_state_dirty &&
            pokedex_state_serialize(&s_state, blob, sizeof(blob)) == sizeof(blob)) {
            s_state.save_seq++;
            s_state_dirty = false;
            write_state = true;
        }
        if (s_lang_dirty) {
            lang = s_lang;
            s_lang_dirty = false;
            write_lang = true;
        }
        captured = write_state || write_lang;
        bsp_lvgl_unlock();
    }
    if (!captured) return;

    nvs_handle_t h;
    esp_err_t err = nvs_open(POKEDEX_NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open: %s", esp_err_to_name(err));
        return;
    }
    if (write_state) {
        err = nvs_set_blob(h, POKEDEX_NVS_KEY_STATE, blob, sizeof(blob));
        if (err != ESP_OK) ESP_LOGE(TAG, "nvs save: %s", esp_err_to_name(err));
    }
    if (write_lang) {
        err = nvs_set_u8(h, POKEDEX_NVS_KEY_LANG, (uint8_t)lang);
        if (err != ESP_OK) ESP_LOGE(TAG, "nvs lang: %s", esp_err_to_name(err));
    }
    err = nvs_commit(h);
    if (err != ESP_OK) ESP_LOGE(TAG, "nvs commit: %s", esp_err_to_name(err));
    nvs_close(h);
}

static void wake_worker(void)
{
    if (s_worker) xTaskNotifyGive(s_worker);
}

static void ui_update_tally(void)
{
    char line[24];
    if (!s_scr || !s_tally_seen) return;
    pokedex_layout_format_seen_lang(pokedex_count_seen(&s_state),
                                    s_lang, line, sizeof(line));
    lv_label_set_text(s_tally_seen, line);
}

static void layout_badges(uint8_t t0, uint8_t t1)
{
    uint8_t idx[2] = { t0, t1 };
    for (int i = 0; i < 2; i++) {
        if (idx[i] >= POKEDEX_STATIC_TYPE_COUNT) {
            lv_obj_add_flag(s_badge[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        char label[12];
        pokedex_layout_type_label(pokedex_static_type_names[idx[i]],
                                  s_lang, label, sizeof(label));
        lv_obj_t *l = lv_obj_get_child(s_badge[i], 0);
        uint32_t bg = TYPE_COLORS[idx[i]];
        if (l) {
            lv_label_set_text(l, label);
            lv_obj_set_style_text_color(l,
                lv_color_hex(pokedex_layout_dark_ink(bg) ? CS_BG_DK : 0xFFFFFFu), 0);
        }
        lv_obj_set_style_bg_color(s_badge[i], lv_color_hex(bg), 0);
        apply_rect(s_badge[i], s_lay.badge[i]);
        lv_obj_clear_flag(s_badge[i], LV_OBJ_FLAG_HIDDEN);
    }
}

static void ui_show_sprite_locked(uint32_t dw, uint32_t dh)
{
    if (!s_scr || !s_sprite) return;
    s_sprite_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    s_sprite_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
    s_sprite_dsc.header.w = (uint16_t)dw;
    s_sprite_dsc.header.h = (uint16_t)dh;
    s_sprite_dsc.header.stride = (uint16_t)(dw * 2);
    s_sprite_dsc.data_size = dw * dh * 2;
    s_sprite_dsc.data = (const uint8_t *)s_sprite_pixels;
    apply_rect(s_sprite, s_lay.sprite);
    lv_image_set_src(s_sprite, &s_sprite_dsc);
    lv_obj_remove_flag(s_sprite, LV_OBJ_FLAG_HIDDEN);
    if (s_sprite_hint) lv_obj_add_flag(s_sprite_hint, LV_OBJ_FLAG_HIDDEN);
}

static void flavor_timer_stop(void)
{
    if (s_flavor_timer) {
        lv_timer_delete(s_flavor_timer);
        s_flavor_timer = NULL;
    }
}

static void flavor_tick(lv_timer_t *t)
{
    (void)t;
    if (!s_desc) return;
    lv_obj_set_y(s_desc, -pokedex_flavor_scroll_tick(&s_flavor_scroll));
}

static void flavor_scroll_sync(void)
{
    int content_h;
    int view_h;

    flavor_timer_stop();
    if (!s_desc || !s_flavor_clip) return;
    lv_obj_set_y(s_desc, 0);
    lv_obj_update_layout(s_desc);
    content_h = (int)lv_obj_get_height(s_desc);
    view_h = s_lay.flavor_text.h;
    pokedex_flavor_scroll_init(&s_flavor_scroll, content_h, view_h);
    if (pokedex_flavor_scroll_active(&s_flavor_scroll)) {
        s_flavor_timer = lv_timer_create(flavor_tick, POKEDEX_FLAVOR_TICK_MS, NULL);
    }
}

static void apply_detail(uint32_t id)
{
    const pokedex_static_entry_t *e = &pokedex_static_dex[id];
    char buf[64];

    if (!s_name) return;
    format_species_name(id, buf, sizeof(buf));
    lv_label_set_text(s_name, buf);

    pokedex_layout_format_no(id, buf, sizeof(buf));
    lv_label_set_text(s_no, buf);

    pokedex_layout_format_progress(id, buf, sizeof(buf));
    lv_label_set_text(s_progress, buf);

    layout_badges(e->type0, e->type1);

    pokedex_layout_format_height_lang(e->height_dm, s_lang, buf, sizeof(buf));
    lv_label_set_text(s_height, buf);
    pokedex_layout_format_weight_lang(e->weight_hg, s_lang, buf, sizeof(buf));
    lv_label_set_text(s_weight, buf);

    {
        const char *src = (s_lang == POKEDEX_LANG_ZH && e->desc_zh[0])
                          ? e->desc_zh : e->desc;
        char clip[POKEDEX_LAYOUT_DESC_MAX + 4];
        pokedex_layout_clip_desc(src, clip, sizeof(clip), 0);
        lv_label_set_text(s_desc, clip);
        flavor_scroll_sync();
    }

    ui_update_tally();

    uint32_t w = 0, h = 0, dw = 0, dh = 0;
    if (pokedex_sprite_static(_binary_pokedex_sprites_bin_start,
                              (size_t)(_binary_pokedex_sprites_bin_end -
                                       _binary_pokedex_sprites_bin_start),
                              id, s_sprite_src,
                              sizeof(s_sprite_src) / 2, &w, &h) &&
        w > 0 && h > 0 &&
        pokedex_layout_scale_nn_rgb565(s_sprite_src, w, h,
                                       POKEDEX_LAYOUT_SPRITE_SCALE,
                                       s_sprite_pixels,
                                       sizeof(s_sprite_pixels) / 2,
                                       &dw, &dh)) {
        ui_show_sprite_locked(dw, dh);
    } else if (s_sprite_hint) {
        lv_obj_add_flag(s_sprite, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(s_sprite_hint, LV_OBJ_FLAG_HIDDEN);
    }
}

static void apply_list(void)
{
    uint32_t id = s_state.last_id;
    uint32_t first = pokedex_list_window_first(id, POKEDEX_LIST_ROWS);
    char buf[48];
    int i;

    if (s_title) {
        pokedex_layout_format_title(s_lang, buf, sizeof(buf));
        lv_label_set_text(s_title, buf);
    }
    if (s_progress) {
        pokedex_layout_format_list_progress(id, buf, sizeof(buf));
        lv_label_set_text(s_progress, buf);
    }
    for (i = 0; i < POKEDEX_LIST_ROWS; i++) {
        uint32_t rid = first + (uint32_t)i;
        uint32_t last = pokedex_gen_last(id);
        if (!s_row_bg[i] || !s_row_lab[i]) continue;
        if (rid > last) {
            lv_obj_add_flag(s_row_bg[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        char name[32];
        format_species_name(rid, name, sizeof(name));
        snprintf(buf, sizeof(buf), "%03u  %s", (unsigned)rid, name);
        lv_label_set_text(s_row_lab[i], buf);
        bool sel = (rid == id);
        lv_obj_set_style_bg_color(s_row_bg[i],
            lv_color_hex(sel ? CS_BRIGHT : CS_BG), 0);
        lv_obj_set_style_text_color(s_row_lab[i],
            lv_color_hex(sel ? CS_BG_DK : CS_TEXT), 0);
        lv_obj_clear_flag(s_row_bg[i], LV_OBJ_FLAG_HIDDEN);
    }
    ui_update_tally();
}

static void apply_find(void)
{
    char buf[48];
    int i;

    if (s_title) {
        lv_label_set_text(s_title, s_lang == POKEDEX_LANG_ZH ? "查找" : "FIND");
    }
    for (i = 0; i < (int)POKEDEX_FIND_COUNT; i++) {
        if (!s_find_bg[i] || !s_find_lab[i]) continue;
        if (i == (int)POKEDEX_FIND_JUMP) {
            pokedex_layout_format_jump_item(s_lang, buf, sizeof(buf));
        } else if (i == (int)POKEDEX_FIND_NAME) {
            pokedex_layout_format_name_item(s_lang, buf, sizeof(buf));
        } else {
            pokedex_layout_format_gen_line(pokedex_find_sel_to_gen((uint32_t)i),
                                           s_lang, buf, sizeof(buf));
        }
        lv_label_set_text(s_find_lab[i], buf);
        bool sel = ((uint32_t)i == s_find_sel);
        lv_obj_set_style_bg_color(s_find_bg[i],
            lv_color_hex(sel ? CS_BRIGHT : CS_BG), 0);
        lv_obj_set_style_text_color(s_find_lab[i],
            lv_color_hex(sel ? CS_BG_DK : CS_TEXT), 0);
    }
}

static void apply_jump(void)
{
    uint8_t d[4];
    char buf[4];
    int i;

    pokedex_jump_to_digits(s_jump_value, d);
    if (s_title) {
        lv_label_set_text(s_title, s_lang == POKEDEX_LANG_ZH ? "跳号" : "JUMP");
    }
    if (s_jump_prompt) {
        lv_label_set_text(s_jump_prompt,
                          s_lang == POKEDEX_LANG_ZH ? "输入编号" : "NUMBER");
    }
    for (i = 0; i < 4; i++) {
        if (!s_digit_bg[i] || !s_digit_lab[i]) continue;
        snprintf(buf, sizeof(buf), "%u", (unsigned)d[i]);
        lv_label_set_text(s_digit_lab[i], buf);
        bool sel = ((unsigned)i == s_jump_cursor);
        lv_obj_set_style_bg_color(s_digit_bg[i],
            lv_color_hex(sel ? CS_BRIGHT : CS_BG_DK), 0);
        lv_obj_set_style_border_width(s_digit_bg[i], sel ? 2 : 0, 0);
        lv_obj_set_style_border_color(s_digit_bg[i], lv_color_hex(CS_FRAME), 0);
        lv_obj_set_style_text_color(s_digit_lab[i],
            lv_color_hex(sel ? CS_BG_DK : CS_TEXT), 0);
    }
}

static void apply_name(void)
{
    uint32_t first;
    uint32_t last;
    char buf[48];
    int i;

    if (s_name_count == 0) name_index_rebuild();
    last = s_name_count ? (uint32_t)s_name_count - 1u : 0;
    first = pokedex_window_first((uint32_t)s_name_sel, 0, last,
                                 POKEDEX_LIST_ROWS);
    if (s_title) {
        lv_label_set_text(s_title, s_lang == POKEDEX_LANG_ZH ? "字母" : "A-Z");
    }
    for (i = 0; i < POKEDEX_LIST_ROWS; i++) {
        uint32_t idx = first + (uint32_t)i;
        char name[32];
        uint32_t id;
        bool sel;

        if (!s_row_bg[i] || !s_row_lab[i]) continue;
        if (idx >= s_name_count) {
            lv_obj_add_flag(s_row_bg[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        id = s_name_first[idx];
        format_species_name(id, name, sizeof(name));
        pokedex_layout_format_letter_line(s_name_letter[idx], id, name,
                                          buf, sizeof(buf));
        lv_label_set_text(s_row_lab[i], buf);
        sel = ((uint32_t)s_name_sel == idx);
        lv_obj_set_style_bg_color(s_row_bg[i],
            lv_color_hex(sel ? CS_BRIGHT : CS_BG), 0);
        lv_obj_set_style_text_color(s_row_lab[i],
            lv_color_hex(sel ? CS_BG_DK : CS_TEXT), 0);
        lv_obj_clear_flag(s_row_bg[i], LV_OBJ_FLAG_HIDDEN);
    }
}

static void ui_refresh(void)
{
    if (!s_scr) return;
    if (s_hint) lv_label_set_text(s_hint, hint_text(s_view));
    if (s_view == VIEW_LIST) apply_list();
    else if (s_view == VIEW_DETAIL) apply_detail(s_state.last_id);
    else if (s_view == VIEW_FIND) apply_find();
    else if (s_view == VIEW_NAME) apply_name();
    else apply_jump();
}

static void ui_reset_ptrs(void)
{
    s_sprite = NULL;
    s_sprite_hint = NULL;
    s_name = NULL;
    s_height = NULL;
    s_weight = NULL;
    s_flavor_clip = NULL;
    s_battery = NULL;
    s_progress = NULL;
    s_title = NULL;
    s_no = NULL;
    s_badge[0] = NULL;
    s_badge[1] = NULL;
    s_desc = NULL;
    s_tally_seen = NULL;
    s_hint = NULL;
    s_jump_prompt = NULL;
    memset(s_row_bg, 0, sizeof(s_row_bg));
    memset(s_row_lab, 0, sizeof(s_row_lab));
    memset(s_find_bg, 0, sizeof(s_find_bg));
    memset(s_find_lab, 0, sizeof(s_find_lab));
    memset(s_digit_bg, 0, sizeof(s_digit_bg));
    memset(s_digit_lab, 0, sizeof(s_digit_lab));
}

static void ui_clear_scr(void)
{
    flavor_timer_stop();
    if (!s_scr) return;
    while (lv_obj_get_child_count(s_scr) > 0) {
        lv_obj_delete(lv_obj_get_child(s_scr, 0));
    }
    ui_reset_ptrs();
}

static void build_header(pokedex_rect_t header, pokedex_rect_t rule,
                         pokedex_rect_t title, pokedex_rect_t battery,
                         pokedex_rect_t *progress)
{
    char buf[24];
    flag_block(s_scr, header, CS_BG_DK, 0);
    flag_block(s_scr, rule, CS_FRAME, 0);
    s_title = label_at(s_scr, title, CS_TEXT);
    pokedex_layout_format_title(s_lang, buf, sizeof(buf));
    lv_label_set_text(s_title, buf);
    if (progress) {
        s_progress = label_at(s_scr, *progress, CS_BRIGHT);
        lv_obj_set_style_text_align(s_progress, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_text(s_progress, "");
    }
    s_battery = label_at(s_scr, battery, CS_TEXT);
    lv_obj_set_style_text_align(s_battery, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_text(s_battery, "--");
}

static void build_list(void)
{
    int i;
    pokedex_list_layout_build(&s_list_lay);
    build_header(s_list_lay.header, s_list_lay.header_rule, s_list_lay.title,
                 s_list_lay.battery, &s_list_lay.progress);
    for (i = 0; i < POKEDEX_LIST_ROWS; i++) {
        s_row_bg[i] = flag_block(s_scr, s_list_lay.row[i], CS_BG, 4);
        s_row_lab[i] = lv_label_create(s_row_bg[i]);
        lv_obj_set_style_text_font(s_row_lab[i], ui_font(), 0);
        lv_obj_set_style_text_color(s_row_lab[i], lv_color_hex(CS_TEXT), 0);
        lv_obj_align(s_row_lab[i], LV_ALIGN_LEFT_MID, 8, 0);
        lv_label_set_long_mode(s_row_lab[i], LV_LABEL_LONG_CLIP);
        lv_label_set_text(s_row_lab[i], "");
    }
    lv_obj_t *seen_chip = flag_block(s_scr, s_list_lay.tally_seen, CS_BG_DK, 0);
    s_tally_seen = lv_label_create(seen_chip);
    lv_obj_set_style_text_font(s_tally_seen, ui_font(), 0);
    lv_obj_set_style_text_color(s_tally_seen, lv_color_hex(CS_TEXT), 0);
    lv_obj_center(s_tally_seen);
    lv_label_set_text(s_tally_seen, "");
    s_hint = label_at(s_scr, s_list_lay.hint, CS_DIM);
    lv_label_set_long_mode(s_hint, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(s_hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_line_space(s_hint, 0, 0);
}

static void build_detail(void)
{
    pokedex_layout_build(&s_lay);
    build_header(s_lay.header, s_lay.header_rule, s_lay.title,
                 s_lay.battery, &s_lay.progress);

    flag_block(s_scr, s_lay.sprite_frame, CS_FRAME, 0);
    flag_block(s_scr, s_lay.sprite_inner, CS_BG, 0);
    s_sprite = lv_image_create(s_scr);
    lv_obj_add_flag(s_sprite, LV_OBJ_FLAG_HIDDEN);
    s_sprite_hint = label_at(s_scr, s_lay.sprite, CS_DIM);
    lv_obj_set_style_text_align(s_sprite_hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(s_sprite_hint,
                      s_lang == POKEDEX_LANG_ZH ? "暂无图像" : "NO IMAGE");

    /* 四角花牌在立绘之后创建,保证叠在精灵图上面。 */
    flag_block(s_scr, s_lay.number_chip, CS_BRIGHT, 4);
    s_no = label_at(s_scr, s_lay.number, CS_BG_DK);
    lv_label_set_text(s_no, "NO.001");

    flag_block(s_scr, s_lay.name_chip, CS_BG_DK, 4);
    s_name = label_at(s_scr, s_lay.name, CS_TEXT);
    lv_label_set_long_mode(s_name, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(s_name, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(s_name, "---");

    s_badge[0] = badge_create(s_scr, s_lay.badge[0], TYPE_COLORS[0]);
    s_badge[1] = badge_create(s_scr, s_lay.badge[1], TYPE_COLORS[0]);
    lv_obj_add_flag(s_badge[0], LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_badge[1], LV_OBJ_FLAG_HIDDEN);

    flag_block(s_scr, s_lay.height, CS_BG_DK, 4);
    s_height = label_at(s_scr, s_lay.height_text, CS_TEXT);
    lv_label_set_long_mode(s_height, LV_LABEL_LONG_CLIP);
    lv_label_set_text(s_height, "");

    flag_block(s_scr, s_lay.weight, CS_BG_DK, 4);
    s_weight = label_at(s_scr, s_lay.weight_text, CS_TEXT);
    lv_label_set_long_mode(s_weight, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(s_weight, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_text(s_weight, "");

    flag_block(s_scr, s_lay.flavor_frame, CS_FRAME, 0);
    flag_block(s_scr, s_lay.flavor_inner, CS_BG_DK, 0);
    s_flavor_clip = flag_block(s_scr, s_lay.flavor_text, CS_BG_DK, 0);
    s_desc = lv_label_create(s_flavor_clip);
    lv_obj_set_style_text_font(s_desc, ui_font(), 0);
    lv_obj_set_style_text_color(s_desc, lv_color_hex(CS_TEXT), 0);
    lv_obj_set_pos(s_desc, 0, 0);
    lv_obj_set_width(s_desc, s_lay.flavor_text.w);
    lv_label_set_long_mode(s_desc, LV_LABEL_LONG_WRAP);
    lv_label_set_text(s_desc, "");

    lv_obj_t *seen_chip = flag_block(s_scr, s_lay.tally_seen, CS_BG_DK, 0);
    s_tally_seen = lv_label_create(seen_chip);
    lv_obj_set_style_text_font(s_tally_seen, ui_font(), 0);
    lv_obj_set_style_text_color(s_tally_seen, lv_color_hex(CS_TEXT), 0);
    lv_obj_center(s_tally_seen);
    lv_label_set_text(s_tally_seen, "");

    s_hint = label_at(s_scr, s_lay.hint, CS_DIM);
    lv_label_set_long_mode(s_hint, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(s_hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_line_space(s_hint, 0, 0);
}

static void build_find(void)
{
    int i;
    pokedex_find_layout_build(&s_find_lay);
    build_header(s_find_lay.header, s_find_lay.header_rule, s_find_lay.title,
                 s_find_lay.battery, NULL);
    for (i = 0; i < (int)POKEDEX_FIND_COUNT; i++) {
        s_find_bg[i] = flag_block(s_scr, s_find_lay.row[i], CS_BG, 4);
        s_find_lab[i] = lv_label_create(s_find_bg[i]);
        lv_obj_set_style_text_font(s_find_lab[i], ui_font(), 0);
        lv_obj_set_style_text_color(s_find_lab[i], lv_color_hex(CS_TEXT), 0);
        lv_obj_align(s_find_lab[i], LV_ALIGN_LEFT_MID, 8, 0);
        lv_label_set_text(s_find_lab[i], "");
    }
    s_hint = label_at(s_scr, s_find_lay.hint, CS_DIM);
    lv_label_set_long_mode(s_hint, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(s_hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_line_space(s_hint, 0, 0);
}

static void build_jump(void)
{
    int i;
    pokedex_jump_layout_build(&s_jump_lay);
    build_header(s_jump_lay.header, s_jump_lay.header_rule, s_jump_lay.title,
                 s_jump_lay.battery, NULL);
    s_jump_prompt = label_at(s_scr, s_jump_lay.prompt, CS_DIM);
    lv_obj_set_style_text_align(s_jump_prompt, LV_TEXT_ALIGN_CENTER, 0);
    for (i = 0; i < 4; i++) {
        s_digit_bg[i] = flag_block(s_scr, s_jump_lay.digit[i], CS_BG_DK, 6);
        s_digit_lab[i] = lv_label_create(s_digit_bg[i]);
        lv_obj_set_style_text_font(s_digit_lab[i], ui_font(), 0);
        lv_obj_set_style_text_color(s_digit_lab[i], lv_color_hex(CS_TEXT), 0);
        lv_obj_center(s_digit_lab[i]);
        lv_label_set_text(s_digit_lab[i], "0");
    }
    s_hint = label_at(s_scr, s_jump_lay.hint, CS_DIM);
    lv_label_set_long_mode(s_hint, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(s_hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_line_space(s_hint, 0, 0);
}

static void build_name(void)
{
    int i;

    pokedex_list_layout_build(&s_list_lay);
    build_header(s_list_lay.header, s_list_lay.header_rule, s_list_lay.title,
                 s_list_lay.battery, NULL);
    for (i = 0; i < POKEDEX_LIST_ROWS; i++) {
        s_row_bg[i] = flag_block(s_scr, s_list_lay.row[i], CS_BG, 4);
        s_row_lab[i] = lv_label_create(s_row_bg[i]);
        lv_obj_set_style_text_font(s_row_lab[i], ui_font(), 0);
        lv_obj_set_style_text_color(s_row_lab[i], lv_color_hex(CS_TEXT), 0);
        lv_obj_align(s_row_lab[i], LV_ALIGN_LEFT_MID, 8, 0);
        lv_label_set_long_mode(s_row_lab[i], LV_LABEL_LONG_CLIP);
        lv_label_set_text(s_row_lab[i], "");
    }
    s_hint = label_at(s_scr, s_list_lay.hint, CS_DIM);
    lv_label_set_long_mode(s_hint, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(s_hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_line_space(s_hint, 0, 0);
}

static void bat_tick(lv_timer_t *t);

static void ui_rebuild(void)
{
    if (!s_scr) return;
    ui_clear_scr();
    if (s_view == VIEW_LIST) build_list();
    else if (s_view == VIEW_DETAIL) build_detail();
    else if (s_view == VIEW_FIND) build_find();
    else if (s_view == VIEW_NAME) build_name();
    else build_jump();
    ui_refresh();
    bat_tick(NULL);
}

static void show_view(pokedex_view_t v)
{
    if (s_view == v && s_scr && lv_obj_get_child_count(s_scr) > 0) {
        ui_refresh();
        return;
    }
    s_view = v;
    ui_rebuild();
}

static void worker_main(void *arg)
{
    (void)arg;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (s_exit) continue;
        state_persist();
    }
}

static void goto_id(uint32_t id)
{
    s_state.last_id = id;
    pokedex_mark_seen(&s_state, id);
    s_state_dirty = true;
    ui_refresh();
    wake_worker();
}

static void toggle_lang(void)
{
    s_lang = (s_lang == POKEDEX_LANG_ZH) ? POKEDEX_LANG_EN : POKEDEX_LANG_ZH;
    s_lang_dirty = true;
    ui_rebuild();
    wake_worker();
}

bool demo_pokedex_at_root(void)
{
    return s_scr == NULL || s_view == VIEW_LIST;
}

void demo_pokedex_debug_line(const char *line)
{
    if (!s_scr || !line) return;
    if (strncmp(line, "FAP_POKEDEX_VIEW ", 17) == 0) {
        const char *v = line + 17;
        if (strcmp(v, "list") == 0) {
            show_view(VIEW_LIST);
        } else if (strcmp(v, "detail") == 0) {
            show_view(VIEW_DETAIL);
        } else if (strcmp(v, "find") == 0) {
            s_find_sel = pokedex_find_sel_for_id(s_state.last_id);
            show_view(VIEW_FIND);
        } else if (strcmp(v, "jump") == 0) {
            s_jump_value = s_state.last_id;
            s_jump_cursor = 3;
            show_view(VIEW_JUMP);
        } else if (strcmp(v, "name") == 0) {
            name_index_rebuild();
            name_select_current();
            show_view(VIEW_NAME);
        }
    } else if (strncmp(line, "FAP_POKEDEX_LANG ", 17) == 0) {
        /* 仅观测:改当前屏,不写 NVS。保存语言只走确定双击。 */
        s_lang = (line[17] == 'z' || line[17] == 'Z')
                 ? POKEDEX_LANG_ZH : POKEDEX_LANG_EN;
        ui_rebuild();
    } else if (strncmp(line, "FAP_POKEDEX_ID ", 15) == 0) {
        uint32_t id = (uint32_t)atoi(line + 15);
        if (pokedex_id_in_range(id)) goto_id(id);
    }
}

void demo_pokedex_key(bsp_btn_t btn, bsp_btn_ev_t ev)
{
    if (ev != BSP_BTN_CLICK && ev != BSP_BTN_DOUBLE && ev != BSP_BTN_LONG) return;

    s_idle_sec = 0;
    bsp_display_backlight(100);

    if (btn == BSP_BTN_OK && ev == BSP_BTN_LONG) {
        if (s_view == VIEW_JUMP || s_view == VIEW_NAME) show_view(VIEW_FIND);
        else if (s_view != VIEW_LIST) show_view(VIEW_LIST);
        return;
    }

    if (btn == BSP_BTN_OK && ev == BSP_BTN_DOUBLE) {
        if (s_view == VIEW_JUMP) show_view(VIEW_FIND);
        else toggle_lang();
        return;
    }

    if (btn == BSP_BTN_OK && ev == BSP_BTN_CLICK) {
        if (s_view == VIEW_LIST) {
            show_view(VIEW_DETAIL);
        } else if (s_view == VIEW_DETAIL) {
            pokedex_cry_play_request(s_state.last_id);
        } else if (s_view == VIEW_FIND) {
            if (s_find_sel == POKEDEX_FIND_JUMP) {
                s_jump_value = s_state.last_id;
                s_jump_cursor = 3;
                show_view(VIEW_JUMP);
            } else if (s_find_sel == POKEDEX_FIND_NAME) {
                name_index_rebuild();
                name_select_current();
                show_view(VIEW_NAME);
            } else {
                uint32_t gen = pokedex_find_sel_to_gen(s_find_sel);
                uint32_t first = pokedex_gen_first_n(gen);
                uint32_t last = pokedex_gen_last_n(gen);
                uint32_t id = s_state.last_id;
                if (id < first || id > last) id = first;
                show_view(VIEW_LIST);
                pokedex_cry_play_stop();
                goto_id(id);
            }
        } else if (s_view == VIEW_NAME) {
            if (s_name_sel < s_name_count) {
                uint32_t id = s_name_first[s_name_sel];
                show_view(VIEW_LIST);
                pokedex_cry_play_stop();
                goto_id(id);
            }
        } else {
            uint32_t id = pokedex_jump_clamp(s_jump_value);
            show_view(VIEW_DETAIL);
            pokedex_cry_play_stop();
            goto_id(id);
        }
        return;
    }

    if (btn != BSP_BTN_UP && btn != BSP_BTN_DOWN) return;
    {
        int32_t dir = (btn == BSP_BTN_UP) ? -1 : 1;
        if (s_view == VIEW_LIST) {
            uint32_t id = s_state.last_id;
            if (ev == BSP_BTN_CLICK) id = pokedex_step_in_gen(id, dir);
            else if (ev == BSP_BTN_DOUBLE) {
                id = pokedex_step_key_in_range(id, dir,
                                               pokedex_gen_first(id),
                                               pokedex_gen_last(id),
                                               id_en_key);
            } else {
                s_find_sel = pokedex_find_sel_for_id(id);
                show_view(VIEW_FIND);
                return;
            }
            pokedex_cry_play_stop();
            goto_id(id);
        } else if (s_view == VIEW_DETAIL) {
            uint32_t id = s_state.last_id;
            if (ev == BSP_BTN_CLICK) id = pokedex_step(id, dir);
            else if (ev == BSP_BTN_DOUBLE) id = pokedex_step(id, dir * 10);
            else {
                s_find_sel = pokedex_find_sel_for_id(id);
                show_view(VIEW_FIND);
                return;
            }
            pokedex_cry_play_stop();
            goto_id(id);
        } else if (s_view == VIEW_FIND) {
            if (ev == BSP_BTN_CLICK || ev == BSP_BTN_DOUBLE) {
                s_find_sel = pokedex_find_step(s_find_sel, dir);
                ui_refresh();
            }
        } else if (s_view == VIEW_NAME) {
            if (s_name_count == 0) name_index_rebuild();
            if (ev == BSP_BTN_CLICK || ev == BSP_BTN_DOUBLE) {
                int32_t delta = (ev == BSP_BTN_DOUBLE)
                                ? dir * (int32_t)POKEDEX_LIST_ROWS : dir;
                int32_t n = (int32_t)s_name_count;
                int32_t v = (int32_t)s_name_sel + delta;
                if (n > 0) {
                    v %= n;
                    if (v < 0) v += n;
                    s_name_sel = (uint8_t)v;
                }
                ui_refresh();
            }
        } else {
            if (ev == BSP_BTN_CLICK) {
                s_jump_value = pokedex_jump_nudge_digit(s_jump_value,
                                                       s_jump_cursor, dir);
            } else if (ev == BSP_BTN_DOUBLE) {
                s_jump_cursor = pokedex_jump_cursor_move(s_jump_cursor, dir);
            }
            ui_refresh();
        }
    }
}

static void bat_tick(lv_timer_t *t)
{
    (void)t;
    if (!s_scr || !s_battery) return;
    int soc = bsp_battery_soc();
    if (soc < 0) {
        lv_label_set_text(s_battery, "--");
        lv_obj_set_style_text_color(s_battery, lv_color_hex(CS_DIM), 0);
    } else {
        lv_label_set_text_fmt(s_battery, "%d%%", soc);
        lv_obj_set_style_text_color(s_battery,
            lv_color_hex(soc < 20 ? CS_WARN : CS_TEXT), 0);
    }
}

static void idle_tick(lv_timer_t *t)
{
    (void)t;
    if (s_idle_sec < POKEDEX_IDLE_DIM_S) {
        s_idle_sec++;
        if (s_idle_sec == POKEDEX_IDLE_DIM_S) bsp_display_backlight(25);
    }
}

void demo_pokedex_enter(void)
{
    nvs_flash_init();
    state_load();
    pokedex_cry_play_init();
    if (!s_worker) {
        xTaskCreate(worker_main, "pokedex", POKEDEX_WORKER_STACK, NULL,
                    POKEDEX_WORKER_PRIO, &s_worker);
    }

    s_scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(CS_BG), 0);
    lv_obj_set_style_border_width(s_scr, 0, 0);
    lv_obj_set_style_pad_all(s_scr, 0, 0);

    s_idle_sec = 0;
    s_bat_timer = lv_timer_create(bat_tick, 2000, NULL);
    s_idle_timer = lv_timer_create(idle_tick, 1000, NULL);

    lv_screen_load(s_scr);
    s_exit = false;
    s_view = VIEW_DETAIL; /* 强制 rebuild */
    show_view(VIEW_LIST);
    goto_id(s_state.last_id);
}

void demo_pokedex_exit(void)
{
    pokedex_cry_play_release();
    if (s_worker) xTaskNotifyGive(s_worker);

    s_exit = true;

    flavor_timer_stop();
    lv_timer_delete(s_bat_timer);
    s_bat_timer = NULL;
    lv_timer_delete(s_idle_timer);
    s_idle_timer = NULL;

    lv_obj_delete(s_scr);
    s_scr = NULL;
    ui_reset_ptrs();
}
