/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdint.h>
#include "esp_log.h"
#include "esp_lv_adapter.h"
#include "lvgl.h"
#include "lvgl_component_menu.h"

static const char *TAG = "lvgl_menu";

typedef enum {
    MENU_BTN = 0,
    MENU_SLIDER,
    MENU_SWITCH,
    MENU_BAR,
    MENU_DROPDOWN,
    MENU_TEXTAREA,
    MENU_CHART,
    MENU_MAX,
} menu_id_t;

typedef struct {
    lv_obj_t *screen;
    lv_obj_t *title;
    lv_obj_t *list;
    lv_obj_t *panel;
    lv_obj_t *status;
    lv_obj_t *kb;
    lv_obj_t *bar;
    lv_obj_t *bar_label;
} ui_ctx_t;

static ui_ctx_t g_ui;

static void ui_lock(void)
{
    esp_lv_adapter_lock(-1);
}

static void ui_unlock(void)
{
    esp_lv_adapter_unlock();
}

static void set_status(const char *text)
{
    if (g_ui.status) {
        lv_label_set_text(g_ui.status, text);
    }
}

static void clear_panel(void)
{
    if (g_ui.kb) {
        lv_obj_del(g_ui.kb);
        g_ui.kb = NULL;
    }
    if (g_ui.panel) {
        lv_obj_clean(g_ui.panel);
    }
    g_ui.bar = NULL;
    g_ui.bar_label = NULL;
}

static void btn_click_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    set_status("Button: clicked");
}

static void slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int v = lv_slider_get_value(slider);

    char text[64];
    snprintf(text, sizeof(text), "Slider: %d", v);
    set_status(text);
}

static void switch_event_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    set_status(on ? "Switch: ON" : "Switch: OFF");
}

static void bar_slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int v = lv_slider_get_value(slider);

    if (g_ui.bar) {
        lv_bar_set_value(g_ui.bar, v, LV_ANIM_ON);
    }

    if (g_ui.bar_label) {
        char text[32];
        snprintf(text, sizeof(text), "Progress: %d%%", v);
        lv_label_set_text(g_ui.bar_label, text);
    }
}

static void dropdown_event_cb(lv_event_t *e)
{
    lv_obj_t *dd = lv_event_get_target(e);
    char option[32] = {0};
    lv_dropdown_get_selected_str(dd, option, sizeof(option));

    char text[64];
    snprintf(text, sizeof(text), "Dropdown: %s", option);
    set_status(text);
}

static void textarea_focus_cb(lv_event_t *e)
{
    lv_obj_t *ta = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_FOCUSED && g_ui.kb) {
        lv_keyboard_set_textarea(g_ui.kb, ta);
        lv_obj_clear_flag(g_ui.kb, LV_OBJ_FLAG_HIDDEN);
        set_status("Textarea: focused");
    }
}

static void keyboard_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        if (g_ui.kb) {
            lv_keyboard_set_textarea(g_ui.kb, NULL);
            lv_obj_add_flag(g_ui.kb, LV_OBJ_FLAG_HIDDEN);
        }
        set_status("Textarea: input done");
    }
}

static void create_button_test(void)
{
    lv_obj_t *label = lv_label_create(g_ui.panel);
    lv_label_set_text(label, "Button test");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *btn = lv_btn_create(g_ui.panel);
    lv_obj_set_size(btn, 130, 46);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn, btn_click_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Click Me");
    lv_obj_center(btn_label);

    set_status("Button: waiting for click");
}

static void create_slider_test(void)
{
    lv_obj_t *label = lv_label_create(g_ui.panel);
    lv_label_set_text(label, "Slider test");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *slider = lv_slider_create(g_ui.panel);
    lv_obj_set_width(slider, lv_pct(80));
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 0);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 30, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    set_status("Slider: 30");
}

static void create_switch_test(void)
{
    lv_obj_t *label = lv_label_create(g_ui.panel);
    lv_label_set_text(label, "Switch test");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *sw = lv_switch_create(g_ui.panel);
    lv_obj_align(sw, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(sw, switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    set_status("Switch: OFF");
}

static void create_bar_test(void)
{
    lv_obj_t *label = lv_label_create(g_ui.panel);
    lv_label_set_text(label, "Bar test");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    g_ui.bar = lv_bar_create(g_ui.panel);
    lv_obj_set_size(g_ui.bar, lv_pct(80), 18);
    lv_obj_align(g_ui.bar, LV_ALIGN_CENTER, 0, -20);
    lv_bar_set_range(g_ui.bar, 0, 100);
    lv_bar_set_value(g_ui.bar, 45, LV_ANIM_OFF);

    g_ui.bar_label = lv_label_create(g_ui.panel);
    lv_label_set_text(g_ui.bar_label, "Progress: 45%");
    lv_obj_align(g_ui.bar_label, LV_ALIGN_CENTER, 0, 12);

    lv_obj_t *slider = lv_slider_create(g_ui.panel);
    lv_obj_set_width(slider, lv_pct(80));
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 48);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 45, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, bar_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    set_status("Bar: use slider to change");
}

static void create_dropdown_test(void)
{
    lv_obj_t *label = lv_label_create(g_ui.panel);
    lv_label_set_text(label, "Dropdown test");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *dd = lv_dropdown_create(g_ui.panel);
    lv_dropdown_set_options(dd, "Option A\nOption B\nOption C\nOption D");
    lv_obj_set_width(dd, 160);
    lv_obj_align(dd, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(dd, dropdown_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    set_status("Dropdown: Option A");
}

static void create_textarea_test(void)
{
    lv_obj_t *label = lv_label_create(g_ui.panel);
    lv_label_set_text(label, "Textarea + Keyboard test");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *ta = lv_textarea_create(g_ui.panel);
    lv_obj_set_size(ta, lv_pct(90), 70);
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 40);
    lv_textarea_set_placeholder_text(ta, "Type here...");
    lv_obj_add_event_cb(ta, textarea_focus_cb, LV_EVENT_FOCUSED, NULL);

    g_ui.kb = lv_keyboard_create(g_ui.panel);
    lv_obj_set_size(g_ui.kb, lv_pct(100), 110);
    lv_obj_align(g_ui.kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(g_ui.kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(g_ui.kb, keyboard_event_cb, LV_EVENT_ALL, NULL);

    set_status("Textarea: tap input box");
}

static void create_chart_test(void)
{
    lv_obj_t *label = lv_label_create(g_ui.panel);
    lv_label_set_text(label, "Chart test");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *chart = lv_chart_create(g_ui.panel);
    lv_obj_set_size(chart, lv_pct(92), 140);
    lv_obj_align(chart, LV_ALIGN_CENTER, 0, 10);

    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, 8);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);

    lv_chart_series_t *ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
    if (ser) {
        lv_chart_set_next_value(chart, ser, 10);
        lv_chart_set_next_value(chart, ser, 20);
        lv_chart_set_next_value(chart, ser, 40);
        lv_chart_set_next_value(chart, ser, 30);
        lv_chart_set_next_value(chart, ser, 70);
        lv_chart_set_next_value(chart, ser, 90);
        lv_chart_set_next_value(chart, ser, 65);
        lv_chart_set_next_value(chart, ser, 80);
        lv_chart_refresh(chart);
    }

    set_status("Chart: rendered");
}

static void show_test(menu_id_t id)
{
    clear_panel();

    switch (id) {
    case MENU_BTN:
        create_button_test();
        break;
    case MENU_SLIDER:
        create_slider_test();
        break;
    case MENU_SWITCH:
        create_switch_test();
        break;
    case MENU_BAR:
        create_bar_test();
        break;
    case MENU_DROPDOWN:
        create_dropdown_test();
        break;
    case MENU_TEXTAREA:
        create_textarea_test();
        break;
    case MENU_CHART:
        create_chart_test();
        break;
    default:
        set_status("Unknown test");
        break;
    }
}

static void menu_btn_event_cb(lv_event_t *e)
{
    menu_id_t id = (menu_id_t)(uintptr_t)lv_event_get_user_data(e);
    show_test(id);
}

static lv_obj_t *create_menu_button(lv_obj_t *parent, const char *name, menu_id_t id)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_width(btn, lv_pct(100));
    lv_obj_add_event_cb(btn, menu_btn_event_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)id);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, name);
    lv_obj_center(label);

    return btn;
}

void lvgl_component_menu_start(void)
{
    ui_lock();

    g_ui.screen = lv_scr_act();
    lv_obj_clean(g_ui.screen);
    lv_obj_set_style_bg_color(g_ui.screen, lv_color_hex(0xf2f5f8), 0);

    g_ui.title = lv_label_create(g_ui.screen);
    lv_label_set_text(g_ui.title, "LVGL Component Test Menu");
    lv_obj_set_style_text_font(g_ui.title, &lv_font_montserrat_14, 0);
    lv_obj_align(g_ui.title, LV_ALIGN_TOP_MID, 0, 8);

    g_ui.list = lv_obj_create(g_ui.screen);
    lv_obj_set_size(g_ui.list, lv_pct(32), lv_pct(82));
    lv_obj_align(g_ui.list, LV_ALIGN_BOTTOM_LEFT, 8, -8);
    lv_obj_set_flex_flow(g_ui.list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(g_ui.list, 8, 0);
    lv_obj_set_style_pad_row(g_ui.list, 8, 0);
    lv_obj_set_scrollbar_mode(g_ui.list, LV_SCROLLBAR_MODE_AUTO);

    g_ui.panel = lv_obj_create(g_ui.screen);
    lv_obj_set_size(g_ui.panel, lv_pct(64), lv_pct(74));
    lv_obj_align(g_ui.panel, LV_ALIGN_TOP_RIGHT, -8, 40);

    g_ui.status = lv_label_create(g_ui.screen);
    lv_label_set_text(g_ui.status, "Select a test item");
    lv_obj_set_width(g_ui.status, lv_pct(64));
    lv_obj_set_style_text_align(g_ui.status, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(g_ui.status, LV_ALIGN_BOTTOM_RIGHT, -12, -12);

    create_menu_button(g_ui.list, "Button", MENU_BTN);
    create_menu_button(g_ui.list, "Slider", MENU_SLIDER);
    create_menu_button(g_ui.list, "Switch", MENU_SWITCH);
    create_menu_button(g_ui.list, "Progress Bar", MENU_BAR);
    create_menu_button(g_ui.list, "Dropdown", MENU_DROPDOWN);
    create_menu_button(g_ui.list, "Textarea", MENU_TEXTAREA);
    create_menu_button(g_ui.list, "Chart", MENU_CHART);

    show_test(MENU_BTN);

    ui_unlock();
    ESP_LOGI(TAG, "LVGL component test menu started");
}
