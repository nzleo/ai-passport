// tests/test_pokedex_layout.c —— 宿主机单测:掌上图鉴布局几何与短文案。
#include <stdio.h>
#include <string.h>

#include "pokedex_layout.h"

static int s_failures;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);  \
            s_failures++;                                                    \
        }                                                                    \
    } while (0)

static void check_no_overlap(pokedex_rect_t a, pokedex_rect_t b,
                             const char *an, const char *bn)
{
    if (pokedex_rect_overlaps(a, b)) {
        fprintf(stderr, "FAIL overlap %s (%d,%d %dx%d) vs %s (%d,%d %dx%d)\n",
                an, a.x, a.y, a.w, a.h, bn, b.x, b.y, b.w, b.h);
        s_failures++;
    }
}

static void test_bounds_and_nesting(void)
{
    pokedex_layout_t L;
    pokedex_layout_build(&L);

    CHECK(pokedex_rect_in_bounds(L.screen, POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
    CHECK(pokedex_rect_in_bounds(L.header, POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
    CHECK(pokedex_rect_in_bounds(L.header_rule, POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
    CHECK(pokedex_rect_in_bounds(L.title, POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
    CHECK(pokedex_rect_in_bounds(L.progress, POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
    CHECK(pokedex_rect_in_bounds(L.battery, POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
    CHECK(pokedex_rect_in_bounds(L.sprite_frame, POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
    CHECK(pokedex_rect_in_bounds(L.sprite_inner, POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
    CHECK(pokedex_rect_in_bounds(L.sprite, POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
    CHECK(pokedex_rect_in_bounds(L.number_chip, POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
    CHECK(pokedex_rect_in_bounds(L.number, POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
    CHECK(pokedex_rect_in_bounds(L.name_chip, POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
    CHECK(pokedex_rect_in_bounds(L.name, POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
    CHECK(pokedex_rect_in_bounds(L.badge[0], POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
    CHECK(pokedex_rect_in_bounds(L.badge[1], POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
    CHECK(pokedex_rect_in_bounds(L.height, POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
    CHECK(pokedex_rect_in_bounds(L.height_text, POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
    CHECK(pokedex_rect_in_bounds(L.weight, POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
    CHECK(pokedex_rect_in_bounds(L.weight_text, POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
    CHECK(pokedex_rect_in_bounds(L.flavor_frame, POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
    CHECK(pokedex_rect_in_bounds(L.flavor_inner, POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
    CHECK(pokedex_rect_in_bounds(L.flavor_text, POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
    CHECK(pokedex_rect_in_bounds(L.tally_seen, POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
    CHECK(pokedex_rect_in_bounds(L.hint, POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));

    CHECK(pokedex_rect_contains(L.header, L.title));
    CHECK(pokedex_rect_contains(L.header, L.progress));
    CHECK(pokedex_rect_contains(L.header, L.battery));
    CHECK(pokedex_rect_contains(L.sprite_frame, L.sprite_inner));
    CHECK(pokedex_rect_contains(L.sprite_inner, L.sprite));
    CHECK(pokedex_rect_contains(L.sprite_frame, L.number_chip));
    CHECK(pokedex_rect_contains(L.sprite_frame, L.name_chip));
    CHECK(pokedex_rect_contains(L.sprite_frame, L.badge[0]));
    CHECK(pokedex_rect_contains(L.sprite_frame, L.badge[1]));
    CHECK(pokedex_rect_contains(L.sprite_frame, L.height));
    CHECK(pokedex_rect_contains(L.sprite_frame, L.weight));
    CHECK(pokedex_rect_contains(L.number_chip, L.number));
    CHECK(pokedex_rect_contains(L.name_chip, L.name));
    CHECK(pokedex_rect_contains(L.height, L.height_text));
    CHECK(pokedex_rect_contains(L.weight, L.weight_text));
    CHECK(pokedex_rect_contains(L.flavor_frame, L.flavor_inner));
    CHECK(pokedex_rect_contains(L.flavor_inner, L.flavor_text));

    CHECK(L.sprite.w == POKEDEX_LAYOUT_SPRITE_PX);
    CHECK(L.sprite.h == POKEDEX_LAYOUT_SPRITE_PX);
    CHECK(L.sprite.w == 144);
    CHECK(L.name.w >= 72); /* 顶边居中,夹在编号与属性之间 */
    CHECK(L.name.h >= 14);
    CHECK(L.number_chip.w >= 80); /* NO.1025 */
    CHECK(L.name.x > L.number_chip.x + L.number_chip.w);
    CHECK(L.name.x + L.name.w < L.badge[0].x);
    CHECK(L.flavor_text.h >= 56); /* 下 1/3 概述约 3–4 行 */
    CHECK(L.sprite_frame.h >= L.flavor_frame.h * 2); /* 约 2/3 : 1/3 */
}

static void test_no_sibling_overlap(void)
{
    pokedex_layout_t L;
    pokedex_layout_build(&L);

    check_no_overlap(L.header, L.sprite_frame, "header", "sprite");
    check_no_overlap(L.header_rule, L.sprite_frame, "rule", "sprite");
    check_no_overlap(L.title, L.progress, "title", "progress");
    check_no_overlap(L.progress, L.battery, "progress", "battery");
    check_no_overlap(L.title, L.battery, "title", "battery");

    check_no_overlap(L.number_chip, L.name_chip, "number", "name");
    check_no_overlap(L.number_chip, L.badge[0], "number", "badge0");
    check_no_overlap(L.number_chip, L.badge[1], "number", "badge1");
    check_no_overlap(L.badge[0], L.badge[1], "badge0", "badge1");
    check_no_overlap(L.name_chip, L.badge[0], "name", "badge0");
    check_no_overlap(L.name_chip, L.badge[1], "name", "badge1");
    check_no_overlap(L.number_chip, L.height, "number", "height");
    check_no_overlap(L.name_chip, L.weight, "name", "weight");
    check_no_overlap(L.badge[0], L.height, "badge0", "height");
    check_no_overlap(L.badge[1], L.height, "badge1", "height");
    check_no_overlap(L.height, L.weight, "height", "weight");
    check_no_overlap(L.sprite_frame, L.flavor_frame, "sprite", "flavor");
    check_no_overlap(L.flavor_frame, L.tally_seen, "flavor", "seen");
    check_no_overlap(L.tally_seen, L.hint, "seen", "hint");
}

static void test_type_abbr(void)
{
    char buf[8];

    CHECK(pokedex_layout_type_abbr("dark", buf, sizeof(buf)) == 3);
    CHECK(strcmp(buf, "DRK") == 0);
    CHECK(pokedex_layout_type_abbr("electric", buf, sizeof(buf)) == 3);
    CHECK(strcmp(buf, "ELE") == 0);
    CHECK(pokedex_layout_type_abbr("fighting", buf, sizeof(buf)) == 3);
    CHECK(strcmp(buf, "FIG") == 0);
    CHECK(pokedex_layout_type_abbr("grass", buf, sizeof(buf)) == 3);
    CHECK(strcmp(buf, "GRA") == 0);
    CHECK(pokedex_layout_type_abbr("poison", buf, sizeof(buf)) == 3);
    CHECK(strcmp(buf, "PSN") == 0);
    CHECK(pokedex_layout_type_abbr("water", buf, sizeof(buf)) == 3);
    CHECK(strcmp(buf, "WAT") == 0);

    CHECK(pokedex_layout_type_abbr("unknown", buf, sizeof(buf)) == 3);
    CHECK(strcmp(buf, "UNK") == 0);
    CHECK(pokedex_layout_type_abbr("ab", buf, sizeof(buf)) == 2);
    CHECK(strcmp(buf, "AB") == 0);
    CHECK(pokedex_layout_type_abbr("", buf, sizeof(buf)) == 0);
    CHECK(buf[0] == '\0');
    CHECK(pokedex_layout_type_abbr("fire", buf, 3) == 0);
}

static void test_formatters(void)
{
    char buf[64];

    CHECK(pokedex_layout_format_no(1, buf, sizeof(buf)) > 0);
    CHECK(strcmp(buf, "NO.001") == 0);
    pokedex_layout_format_no(151, buf, sizeof(buf));
    CHECK(strcmp(buf, "NO.151") == 0);

    pokedex_layout_format_progress(25, buf, sizeof(buf));
    CHECK(strcmp(buf, "025/1025") == 0);
    pokedex_layout_format_progress(1025, buf, sizeof(buf));
    CHECK(strcmp(buf, "1025/1025") == 0);

    pokedex_layout_format_caught(3, buf, sizeof(buf));
    CHECK(strcmp(buf, "3 CAUGHT") == 0);
    pokedex_layout_format_seen(12, buf, sizeof(buf));
    CHECK(strcmp(buf, "12 SEEN") == 0);
    pokedex_layout_format_caught(151, buf, sizeof(buf));
    CHECK(strcmp(buf, "151 CAUGHT") == 0);

    pokedex_layout_format_stats(4, 60, buf, sizeof(buf));
    CHECK(strcmp(buf, "HT 0.4 m    WT 6.0 kg") == 0);
    pokedex_layout_format_stats(20, 1000, buf, sizeof(buf));
    CHECK(strcmp(buf, "HT 2.0 m    WT 100.0 kg") == 0);
}

static void test_clip_desc(void)
{
    char dst[32];
    const char *short_txt = "A strange seed was planted.";
    const char *long_txt =
        "When several of these POKeMON gather, their electricity could "
        "build and cause lightning storms.";

    CHECK(pokedex_layout_clip_desc(short_txt, dst, sizeof(dst), 0) ==
          strlen(short_txt));
    CHECK(strcmp(dst, short_txt) == 0);

    CHECK(pokedex_layout_clip_desc(long_txt, dst, sizeof(dst), 20) == 20);
    CHECK(strlen(dst) == 20);
    CHECK(strcmp(dst + 17, "...") == 0);
    CHECK(strncmp(dst, long_txt, 17) == 0);

    CHECK(pokedex_layout_clip_desc(long_txt, dst, 8, 20) == 7);
    CHECK(strlen(dst) == 7);
    CHECK(strcmp(dst + 4, "...") == 0);

    CHECK(pokedex_layout_clip_desc(NULL, dst, sizeof(dst), 10) == 0);
    CHECK(dst[0] == '\0');
}

static void test_scale2x(void)
{
    uint16_t buf[16];
    uint32_t dw = 0, dh = 0;

    buf[0] = 0xF800;
    buf[1] = 0x07E0;
    buf[2] = 0x001F;
    buf[3] = 0xFFFF;
    CHECK(pokedex_layout_scale2x_rgb565(buf, 2, 2, buf, 16, &dw, &dh));
    CHECK(dw == 4 && dh == 4);
    CHECK(buf[0] == 0xF800 && buf[1] == 0xF800);
    CHECK(buf[4] == 0xF800 && buf[5] == 0xF800);
    CHECK(buf[2] == 0x07E0 && buf[3] == 0x07E0);
    CHECK(buf[6] == 0x07E0 && buf[7] == 0x07E0);
    CHECK(buf[8] == 0x001F && buf[9] == 0x001F);
    CHECK(buf[10] == 0xFFFF && buf[11] == 0xFFFF);
    CHECK(buf[12] == 0x001F && buf[13] == 0x001F);
    CHECK(buf[14] == 0xFFFF && buf[15] == 0xFFFF);

    CHECK(!pokedex_layout_scale2x_rgb565(buf, 2, 2, buf, 15, &dw, &dh));
    CHECK(!pokedex_layout_scale2x_rgb565(buf, 0, 2, buf, 16, &dw, &dh));
}

static void test_fit_sprite(void)
{
    uint16_t src[16];
    uint16_t dst[36];
    int i;

    for (i = 0; i < 16; i++) src[i] = 0x0000;
    src[5] = 0xF800;
    src[6] = 0x07E0;
    src[9] = 0x001F;
    src[10] = 0xFFFF;
    CHECK(pokedex_layout_fit_sprite_rgb565(src, 4, 4, 0x0000, dst, 6, 6, 36));
    CHECK(dst[0] == 0xF800 && dst[2] == 0xF800);
    CHECK(dst[3] == 0x07E0 && dst[5] == 0x07E0);
    CHECK(dst[18] == 0x001F);
    CHECK(dst[21] == 0xFFFF && dst[35] == 0xFFFF);
    CHECK(!pokedex_layout_fit_sprite_rgb565(src, 4, 4, 0x0000, dst, 6, 6, 35));
    CHECK(!pokedex_layout_fit_sprite_rgb565(src, 4, 4, 0x0000, src, 6, 6, 36));
}

static void test_scale3x(void)
{
    uint16_t buf[36];
    uint32_t dw = 0, dh = 0;

    buf[0] = 0xF800;
    buf[1] = 0x07E0;
    buf[2] = 0x001F;
    buf[3] = 0xFFFF;
    CHECK(pokedex_layout_scale_nn_rgb565(buf, 2, 2, 3, buf, 36, &dw, &dh));
    CHECK(dw == 6 && dh == 6);
    CHECK(buf[0] == 0xF800 && buf[2] == 0xF800);
    CHECK(buf[12] == 0xF800);
    CHECK(buf[3] == 0x07E0 && buf[5] == 0x07E0);
    CHECK(buf[18] == 0x001F);
    CHECK(buf[21] == 0xFFFF && buf[35] == 0xFFFF);
    CHECK(!pokedex_layout_scale_nn_rgb565(buf, 2, 2, 3, buf, 35, &dw, &dh));
    CHECK(!pokedex_layout_scale_nn_rgb565(buf, 2, 2, 0, buf, 36, &dw, &dh));
}

static void test_browse_layouts(void)
{
    pokedex_list_layout_t L;
    pokedex_find_layout_t F;
    pokedex_jump_layout_t J;
    int i;

    pokedex_list_layout_build(&L);
    CHECK(pokedex_rect_in_bounds(L.header, POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
    CHECK(pokedex_rect_contains(L.header, L.title));
    CHECK(pokedex_rect_contains(L.header, L.progress));
    CHECK(pokedex_rect_contains(L.header, L.battery));
    CHECK(!pokedex_rect_overlaps(L.title, L.progress));
    CHECK(!pokedex_rect_overlaps(L.progress, L.battery));
    for (i = 0; i < POKEDEX_LIST_ROWS; i++) {
        CHECK(pokedex_rect_in_bounds(L.row[i], POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
        CHECK(!pokedex_rect_overlaps(L.header, L.row[i]));
        CHECK(!pokedex_rect_overlaps(L.row[i], L.tally_seen));
        CHECK(!pokedex_rect_overlaps(L.row[i], L.hint));
        if (i > 0) CHECK(!pokedex_rect_overlaps(L.row[i - 1], L.row[i]));
    }
    CHECK(!pokedex_rect_overlaps(L.tally_seen, L.hint));

    pokedex_find_layout_build(&F);
    CHECK(!pokedex_rect_overlaps(F.title, F.battery));
    for (i = 0; i < 11; i++) {
        CHECK(pokedex_rect_in_bounds(F.row[i], POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
        CHECK(!pokedex_rect_overlaps(F.header, F.row[i]));
        CHECK(!pokedex_rect_overlaps(F.row[i], F.hint));
        if (i > 0) CHECK(!pokedex_rect_overlaps(F.row[i - 1], F.row[i]));
    }

    pokedex_jump_layout_build(&J);
    CHECK(pokedex_rect_in_bounds(J.prompt, POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
    for (i = 0; i < 4; i++) {
        CHECK(pokedex_rect_in_bounds(J.digit[i], POKEDEX_LAYOUT_W, POKEDEX_LAYOUT_H));
        CHECK(!pokedex_rect_overlaps(J.prompt, J.digit[i]));
        CHECK(!pokedex_rect_overlaps(J.digit[i], J.hint));
        if (i > 0) CHECK(!pokedex_rect_overlaps(J.digit[i - 1], J.digit[i]));
    }
}

static void test_lang_formatters(void)
{
    char buf[64];

    CHECK(strcmp(pokedex_layout_roman_gen(1), "I") == 0);
    CHECK(strcmp(pokedex_layout_roman_gen(9), "IX") == 0);
    CHECK(pokedex_layout_roman_gen(0)[0] == '\0');

    pokedex_layout_format_title(POKEDEX_LANG_EN, buf, sizeof(buf));
    CHECK(strcmp(buf, "POKEDEX") == 0);
    pokedex_layout_format_title(POKEDEX_LANG_ZH, buf, sizeof(buf));
    CHECK(strcmp(buf, "图鉴") == 0);

    pokedex_layout_format_list_progress(25, buf, sizeof(buf));
    CHECK(strcmp(buf, "I 025/151") == 0);
    pokedex_layout_format_list_progress(152, buf, sizeof(buf));
    CHECK(strcmp(buf, "II 001/100") == 0);
    pokedex_layout_format_list_progress(1025, buf, sizeof(buf));
    CHECK(strcmp(buf, "IX 120/120") == 0);

    pokedex_layout_format_seen_lang(12, POKEDEX_LANG_EN, buf, sizeof(buf));
    CHECK(strcmp(buf, "12 SEEN") == 0);
    pokedex_layout_format_seen_lang(12, POKEDEX_LANG_ZH, buf, sizeof(buf));
    CHECK(strcmp(buf, "已见 12") == 0);

    pokedex_layout_format_stats_lang(4, 60, POKEDEX_LANG_EN, buf, sizeof(buf));
    CHECK(strcmp(buf, "HT 0.4 m   WT 6.0 kg") == 0);
    pokedex_layout_format_stats_lang(4, 60, POKEDEX_LANG_ZH, buf, sizeof(buf));
    CHECK(strcmp(buf, "身高 0.4 m   体重 6.0 kg") == 0);
    pokedex_layout_format_height_lang(4, POKEDEX_LANG_ZH, buf, sizeof(buf));
    CHECK(strcmp(buf, "身高 0.4 m") == 0);
    pokedex_layout_format_weight_lang(60, POKEDEX_LANG_ZH, buf, sizeof(buf));
    CHECK(strcmp(buf, "体重 6.0 kg") == 0);
    pokedex_layout_format_height_lang(4, POKEDEX_LANG_EN, buf, sizeof(buf));
    CHECK(strcmp(buf, "HT 0.4 m") == 0);
    pokedex_layout_format_weight_lang(60, POKEDEX_LANG_EN, buf, sizeof(buf));
    CHECK(strcmp(buf, "WT 6.0 kg") == 0);

    pokedex_layout_type_label("electric", POKEDEX_LANG_EN, buf, sizeof(buf));
    CHECK(strcmp(buf, "ELE") == 0);
    pokedex_layout_type_label("electric", POKEDEX_LANG_ZH, buf, sizeof(buf));
    CHECK(strcmp(buf, "电") == 0);
    pokedex_layout_type_label("fairy", POKEDEX_LANG_ZH, buf, sizeof(buf));
    CHECK(strcmp(buf, "妖精") == 0);

    pokedex_layout_format_gen_line(1, POKEDEX_LANG_EN, buf, sizeof(buf));
    CHECK(strcmp(buf, "I     1-151") == 0);
    pokedex_layout_format_gen_line(3, POKEDEX_LANG_ZH, buf, sizeof(buf));
    CHECK(strcmp(buf, "第3世代  252-386") == 0);

    pokedex_layout_format_jump_item(POKEDEX_LANG_EN, buf, sizeof(buf));
    CHECK(strcmp(buf, "GO TO #") == 0);
    pokedex_layout_format_jump_item(POKEDEX_LANG_ZH, buf, sizeof(buf));
    CHECK(strcmp(buf, "跳到编号") == 0);

    pokedex_layout_format_name_item(POKEDEX_LANG_EN, buf, sizeof(buf));
    CHECK(strcmp(buf, "A-Z NAME") == 0);
    pokedex_layout_format_name_item(POKEDEX_LANG_ZH, buf, sizeof(buf));
    CHECK(strcmp(buf, "按字母") == 0);

    pokedex_layout_format_letter_line('P', 25, "PIKACHU", buf, sizeof(buf));
    CHECK(strcmp(buf, "P  #025  PIKACHU") == 0);
    pokedex_layout_format_letter_line('P', 25, "皮卡丘", buf, sizeof(buf));
    CHECK(strcmp(buf, "P  #025  皮卡丘") == 0);
}

static void test_flavor_scroll(void)
{
    pokedex_flavor_scroll_t s;
    int i;
    int16_t y;

    pokedex_flavor_scroll_init(&s, 90, 90);
    CHECK(!pokedex_flavor_scroll_active(&s));
    CHECK(pokedex_flavor_scroll_tick(&s) == 0);
    CHECK(pokedex_flavor_scroll_tick(NULL) == 0);

    pokedex_flavor_scroll_init(&s, 120, 90);
    CHECK(pokedex_flavor_scroll_active(&s));
    CHECK(s.max_y == 30);
    for (i = 0; i < POKEDEX_FLAVOR_HOLD_TICKS; i++) {
        CHECK(pokedex_flavor_scroll_tick(&s) == 0);
    }
    CHECK(pokedex_flavor_scroll_tick(&s) == 1);
    do {
        y = pokedex_flavor_scroll_tick(&s);
    } while (y < 30);
    CHECK(y == 30);
    for (i = 0; i < POKEDEX_FLAVOR_HOLD_TICKS; i++) {
        CHECK(pokedex_flavor_scroll_tick(&s) == 30);
    }
    CHECK(pokedex_flavor_scroll_tick(&s) == 29);
}

static void test_zoom_nudge(void)
{
    CHECK(pokedex_layout_zoom_nudge(3, 1, 2, 4) == 4);
    CHECK(pokedex_layout_zoom_nudge(4, 1, 2, 4) == 4);
    CHECK(pokedex_layout_zoom_nudge(3, -1, 2, 4) == 2);
    CHECK(pokedex_layout_zoom_nudge(2, -1, 2, 4) == 2);
    CHECK(pokedex_layout_zoom_nudge(3, 0, 2, 4) == 3);
}

static void test_dark_ink(void)
{
    CHECK(pokedex_layout_dark_ink(0xF7D02C));  /* electric */
    CHECK(pokedex_layout_dark_ink(0x96D9D6));  /* ice */
    CHECK(pokedex_layout_dark_ink(0xB7B7CE));  /* steel */
    CHECK(pokedex_layout_dark_ink(0xE2BF65));  /* ground */
    CHECK(!pokedex_layout_dark_ink(0xC22E28)); /* fighting */
    CHECK(!pokedex_layout_dark_ink(0x6390F0)); /* water */
    CHECK(!pokedex_layout_dark_ink(0xA33EA1)); /* poison */
}

int main(void)
{
    test_bounds_and_nesting();
    test_no_sibling_overlap();
    test_type_abbr();
    test_formatters();
    test_clip_desc();
    test_scale2x();
    test_fit_sprite();
    test_scale3x();
    test_dark_ink();
    test_browse_layouts();
    test_lang_formatters();
    test_flavor_scroll();
    test_zoom_nudge();

    if (s_failures) {
        fprintf(stderr, "pokedex_layout: %d check(s) failed\n", s_failures);
        return 1;
    }
    printf("pokedex_layout: all checks passed\n");
    return 0;
}
