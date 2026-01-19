#include <string.h>
#include <stdio.h>
#include "screens.h"
#include "ui.h"

objects_t objects;

// --- MOCK DATABASE ---
typedef struct {
    char name[20];
    char dose[10];
    int stock;
    int chamber;
} Medication;

Medication db_meds[8] = {
    {"Panodil", "500 mg", 14, 1},
    {"Fiskeolie", "500 mg", 99, 2},
    {"Ipren", "500 mg", 17, 3},
    {"Multivitamin", "500 mg", 85, 4},
    {"Tom", "", 0, 5},
    {"Tom", "", 0, 6},
    {"Tom", "", 0, 7},
    {"Tom", "", 0, 8}
};

// --- GLOBAL STATE ---
static char current_dose_time[32] = "Morgen"; 
static int current_qty = 1;      
static int selected_med_idx = -1; 
static int refill_add_amount = 0; 
static lv_obj_t * login_dropdown_ptr; 

// --- FERIE STATE ---
static int ferie_days_total = 5; 
static int ferie_curr_day = 1;
static int ferie_curr_dose_idx = 0; 
const char * ferie_dose_names[] = {"Morgen", "Middag", "Aften"};

// --- ALARM STATE ---
static int settings_alarm_active = 1; 

static lv_obj_t * refill_stock_labels[8]; 

// --- HELPERS ---
static void update_refill_qty_ui() {
    if (selected_med_idx < 0) return;
    char buf[64];
    sprintf(buf, "Kammer %d - %s", db_meds[selected_med_idx].chamber, db_meds[selected_med_idx].name);
    if (objects.refill_title_label) lv_label_set_text(objects.refill_title_label, buf);

    sprintf(buf, "%d", refill_add_amount);
    if (objects.refill_add_label) lv_label_set_text(objects.refill_add_label, buf);

    int cur = db_meds[selected_med_idx].stock;
    sprintf(buf, "%d", cur);
    if (objects.refill_cur_label) lv_label_set_text(objects.refill_cur_label, buf);

    sprintf(buf, "%d", cur + refill_add_amount);
    if (objects.refill_total_label) lv_label_set_text(objects.refill_total_label, buf);
}

static void update_ferie_dispense_ui() {
    if (objects.ferie_time_label) {
        lv_label_set_text(objects.ferie_time_label, ferie_dose_names[ferie_curr_dose_idx]);
    }
    if (objects.ferie_day_count_label) {
        char buf[16];
        sprintf(buf, "Dag %d", ferie_curr_day);
        lv_label_set_text(objects.ferie_day_count_label, buf);
    }
}

static void update_alarm_ui() {
    if (!objects.alarm_on_btn || !objects.alarm_off_btn) return;

    if (settings_alarm_active) {
        lv_obj_set_style_bg_color(objects.alarm_on_btn, lv_palette_main(LV_PALETTE_GREEN), 0); 
        lv_label_set_text(objects.alarm_on_label, "Taendt"); 

        lv_obj_set_style_bg_color(objects.alarm_off_btn, lv_palette_main(LV_PALETTE_BLUE), 0); 
        lv_label_set_text(objects.alarm_off_label, "Sluk");
    } else {
        lv_obj_set_style_bg_color(objects.alarm_on_btn, lv_palette_main(LV_PALETTE_BLUE), 0); 
        lv_label_set_text(objects.alarm_on_label, "Taend"); 

        lv_obj_set_style_bg_color(objects.alarm_off_btn, lv_palette_main(LV_PALETTE_RED), 0); 
        lv_label_set_text(objects.alarm_off_label, "Slukket");
    }
}

// --- EVENT HANDLERS ---

static void nav_to_screen_cb(lv_event_t * e) {
    int id = (int)(intptr_t)lv_event_get_user_data(e);
    loadScreen(id);
}

static void slider_event_cb(lv_event_t * e) {
    lv_obj_t * slider = lv_event_get_target(e);
    lv_obj_t * label = (lv_obj_t *)lv_event_get_user_data(e);
    int val = (int)lv_slider_get_value(slider);
    char buf[8];
    sprintf(buf, "%d%%", val);
    lv_label_set_text(label, buf);
}

static void alarm_toggle_cb(lv_event_t * e) {
    int requested_state = (int)(intptr_t)lv_event_get_user_data(e);
    if (settings_alarm_active != requested_state) {
        settings_alarm_active = requested_state;
        update_alarm_ui();
    }
}

static void ferie_start_cb(lv_event_t * e) {
    ferie_curr_day = 1;
    ferie_curr_dose_idx = 0; 
    update_ferie_dispense_ui();
    loadScreen(SCREEN_ID_FERIE_DISPENSE);
}

static void ferie_dispense_next_cb(lv_event_t * e) {
    ferie_curr_dose_idx++;
    if (ferie_curr_dose_idx > 2) {
        ferie_curr_dose_idx = 0; 
        ferie_curr_day++;        
    }
    if (ferie_curr_day > ferie_days_total) {
        loadScreen(SCREEN_ID_MAIN); 
    } else {
        update_ferie_dispense_ui();
    }
}

static void ferie_math_cb(lv_event_t * e) {
    intptr_t delta = (intptr_t)lv_event_get_user_data(e);
    ferie_days_total += delta;
    if (ferie_days_total < 1) ferie_days_total = 1; 
    if (ferie_days_total > 90) ferie_days_total = 90; 

    if (objects.ferie_days_label) {
        char buf[16];
        sprintf(buf, "%d dage", ferie_days_total);
        lv_label_set_text(objects.ferie_days_label, buf);
    }
}

static void refill_check_cb(lv_event_t * e) {
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    lv_obj_t * checkbox = lv_event_get_target(e);
    if (lv_obj_has_state(checkbox, LV_STATE_CHECKED)) {
        selected_med_idx = idx;
        lv_obj_t * parent = lv_obj_get_parent(checkbox); 
        lv_obj_t * list_cont = lv_obj_get_parent(parent); 
        uint32_t child_cnt = lv_obj_get_child_cnt(list_cont);
        for(uint32_t i = 0; i < child_cnt; i++) {
            lv_obj_t * row = lv_obj_get_child(list_cont, i);
            lv_obj_t * other_cb = lv_obj_get_child(row, 0); 
            if (other_cb != checkbox && lv_obj_check_type(other_cb, &lv_checkbox_class)) {
                lv_obj_remove_state(other_cb, LV_STATE_CHECKED);
            }
        }
    } else {
        if (selected_med_idx == idx) selected_med_idx = -1;
    }
}

static void refill_next_cb(lv_event_t * e) {
    if (selected_med_idx >= 0 && selected_med_idx < 8) {
        refill_add_amount = 0; 
        update_refill_qty_ui(); 
        loadScreen(SCREEN_ID_REFILL_QTY);
    }
}

static void refill_math_cb(lv_event_t * e) {
    intptr_t delta = (intptr_t)lv_event_get_user_data(e);
    refill_add_amount += delta;
    if (refill_add_amount < 0) refill_add_amount = 0; 
    update_refill_qty_ui();
}

static void refill_confirm_cb(lv_event_t * e) {
    if (selected_med_idx >= 0) {
        db_meds[selected_med_idx].stock += refill_add_amount;
        if (objects.success_med_label) lv_label_set_text(objects.success_med_label, db_meds[selected_med_idx].name);
        if (refill_stock_labels[selected_med_idx] != NULL) {
            char buf[16];
            sprintf(buf, "%d stk", db_meds[selected_med_idx].stock);
            lv_label_set_text(refill_stock_labels[selected_med_idx], buf);
        }
        loadScreen(SCREEN_ID_REFILL_SUCCESS);
    }
}

static void select_time_cb(lv_event_t * e) {
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * lbl = lv_obj_get_child(btn, 0);
    const char * txt = lv_label_get_text(lbl);
    if (objects.confirm_label) lv_label_set_text(objects.confirm_label, txt);
    loadScreen(SCREEN_ID_DISPENSE_CONFIRM);
}

static void select_med_cb(lv_event_t * e) {
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * lbl = lv_obj_get_child(btn, 0);
    const char * txt = lv_label_get_text(lbl);
    if (strcmp(txt, "Tom") == 0 || strlen(txt) == 0) return;
    if (objects.med_title_label) lv_label_set_text(objects.med_title_label, txt);
    current_qty = 1;
    if (objects.qty_label) lv_label_set_text(objects.qty_label, "1");
    loadScreen(SCREEN_ID_SINGLE_DOSE_QTY);
}

static void qty_change_cb(lv_event_t * e) {
    intptr_t delta = (intptr_t)lv_event_get_user_data(e);
    current_qty += delta;
    if (current_qty < 1) current_qty = 1;
    if (current_qty > 10) current_qty = 10;
    if (objects.qty_label) {
        char buf[8];
        sprintf(buf, "%d", current_qty);
        lv_label_set_text(objects.qty_label, buf);
    }
}

static void toggle_pwd_cb(lv_event_t * e) {
    lv_obj_t * pwd_ta = (lv_obj_t *)lv_event_get_user_data(e);
    lv_textarea_set_password_mode(pwd_ta, !lv_textarea_get_password_mode(pwd_ta));
}

static void check_password_cb(lv_event_t * e) {
    lv_obj_t * pwd_ta = (lv_obj_t *)lv_event_get_user_data(e);
    const char * input_pin = lv_textarea_get_text(pwd_ta);
    
    uint16_t selected_user_idx = lv_dropdown_get_selected(login_dropdown_ptr);

    if (selected_user_idx == 0) { 
        if (strcmp(input_pin, "0211") == 0) {
             loadScreen(SCREEN_ID_DISPENSER); 
        } else {
             lv_textarea_set_text(pwd_ta, "");
        }
    } else if (selected_user_idx == 1) { 
         if (strcmp(input_pin, "1998") == 0) {
             loadScreen(SCREEN_ID_MAIN_HANNE);
         } else {
             lv_textarea_set_text(pwd_ta, "");
         }
    }
}

// UPDATED KEYPAD CALLBACK (Handles 'C')
static void login_keypad_cb(lv_event_t * e) {
    lv_obj_t * btnm = lv_event_get_target(e);
    lv_obj_t * ta = (lv_obj_t *)lv_event_get_user_data(e);
    const char * txt = lv_btnmatrix_get_btn_text(btnm, lv_btnmatrix_get_selected_btn(btnm));
    
    if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
        lv_textarea_delete_char(ta);
    } 
    else if (strcmp(txt, "C") == 0) { // Clear function
        lv_textarea_set_text(ta, "");
    }
    else {
        lv_textarea_add_text(ta, txt);
    }
}

// --- SCREEN CREATION ---

void create_screen_login() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.login = obj;
    lv_obj_set_size(obj, 320, 240);

    lv_obj_t * dd = lv_dropdown_create(obj);
    lv_dropdown_set_options(dd, "Niels Hansen\nHanne Hansen");
    lv_obj_set_size(dd, 160, 35);
    lv_obj_align(dd, LV_ALIGN_TOP_MID, 0, 15);
    login_dropdown_ptr = dd;

    lv_obj_t * pwd_ta = lv_textarea_create(obj);
    lv_textarea_set_password_mode(pwd_ta, true);
    lv_textarea_set_one_line(pwd_ta, true);
    lv_textarea_set_max_length(pwd_ta, 4);
    lv_obj_set_size(pwd_ta, 140, 35);
    lv_obj_align(pwd_ta, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_remove_flag(pwd_ta, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t * blue_btn = lv_button_create(obj);
    lv_obj_set_size(blue_btn, 40, 35);
    lv_obj_align(blue_btn, LV_ALIGN_TOP_MID, 100, 60);
    lv_obj_set_style_bg_color(blue_btn, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_add_event_cb(blue_btn, check_password_cb, LV_EVENT_CLICKED, pwd_ta);

    lv_obj_t * black_btn = lv_button_create(obj);
    lv_obj_set_size(black_btn, 35, 35);
    lv_obj_align(black_btn, LV_ALIGN_TOP_MID, -100, 60);
    lv_obj_set_style_bg_color(black_btn, lv_color_black(), 0);
    lv_obj_add_event_cb(black_btn, toggle_pwd_cb, LV_EVENT_CLICKED, pwd_ta);

    // UPDATED MAP: "C" added, gaps removed
    static const char * btnm_map[] = {
        "1", "2", "3", "\n", 
        "4", "5", "6", "\n", 
        "7", "8", "9", "\n", 
        "C", "0", LV_SYMBOL_BACKSPACE, ""
    };
    
    lv_obj_t * btnm = lv_btnmatrix_create(obj);
    lv_btnmatrix_set_map(btnm, btnm_map);
    lv_obj_set_size(btnm, 320, 130);
    lv_obj_align(btnm, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(btnm, login_keypad_cb, LV_EVENT_VALUE_CHANGED, pwd_ta);
}

void create_screen_dispenser() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.dispenser = obj;
    lv_obj_set_size(obj, 320, 240);

    lv_obj_t *header = lv_label_create(obj);
    lv_label_set_text(header, "Niels Hansen");
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *info_txt = lv_label_create(obj);
    lv_label_set_text(info_txt, "Den kommende dosis er");
    lv_obj_align(info_txt, LV_ALIGN_TOP_MID, 0, 40);

    lv_obj_t *time_txt = lv_label_create(obj);
    lv_label_set_text(time_txt, "Morgen");
    lv_obj_align(time_txt, LV_ALIGN_TOP_MID, 0, 60);

    lv_obj_t *red_btn = lv_button_create(obj);
    lv_obj_set_size(red_btn, 140, 50);
    lv_obj_align(red_btn, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_bg_color(red_btn, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_t *red_lbl = lv_label_create(red_btn);
    lv_label_set_text(red_lbl, "Dispenser");
    lv_obj_center(red_lbl);

    lv_obj_t *next_btn = lv_button_create(obj);
    lv_obj_set_size(next_btn, 100, 40);
    lv_obj_align(next_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_style_bg_color(next_btn, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_t *next_lbl = lv_label_create(next_btn);
    lv_label_set_text(next_lbl, "Videre");
    lv_obj_center(next_lbl);
    lv_obj_add_event_cb(next_btn, nav_to_screen_cb, LV_EVENT_CLICKED, (void*)(intptr_t)SCREEN_ID_MAIN);

    lv_obj_t *lock_btn = lv_button_create(obj);
    lv_obj_set_size(lock_btn, 40, 40);
    lv_obj_align(lock_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_color(lock_btn, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(lock_btn, nav_to_screen_cb, LV_EVENT_CLICKED, (void*)(intptr_t)SCREEN_ID_LOGIN);
}

void create_screen_main() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_size(obj, 320, 240);

    lv_obj_t *label = lv_label_create(obj);
    lv_label_set_text(label, "Niels Hansen");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    const char * btn_names[] = {"Daglig dosis", "Enkelt dosis", "Genopfyldning", "Ferie", "Indstillinger"};
    for(int i = 0; i < 5; i++) {
        lv_obj_t *btn = lv_button_create(obj);
        lv_obj_set_size(btn, 180, 32);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 45 + (i * 38));
        
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, btn_names[i]);
        lv_obj_center(lbl);

        if(i == 0) lv_obj_add_event_cb(btn, nav_to_screen_cb, LV_EVENT_CLICKED, (void*)(intptr_t)SCREEN_ID_DAILY_DOSE);
        else if (i == 1) lv_obj_add_event_cb(btn, nav_to_screen_cb, LV_EVENT_CLICKED, (void*)(intptr_t)SCREEN_ID_SINGLE_DOSE);
        else if (i == 2) lv_obj_add_event_cb(btn, nav_to_screen_cb, LV_EVENT_CLICKED, (void*)(intptr_t)SCREEN_ID_REFILL_SELECT);
        else if (i == 3) lv_obj_add_event_cb(btn, nav_to_screen_cb, LV_EVENT_CLICKED, (void*)(intptr_t)SCREEN_ID_FERIE); 
        else if (i == 4) lv_obj_add_event_cb(btn, nav_to_screen_cb, LV_EVENT_CLICKED, (void*)(intptr_t)SCREEN_ID_SETTINGS); 
    }

    lv_obj_t *lock_btn = lv_button_create(obj);
    lv_obj_set_size(lock_btn, 40, 40);
    lv_obj_align(lock_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_color(lock_btn, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(lock_btn, nav_to_screen_cb, LV_EVENT_CLICKED, (void*)(intptr_t)SCREEN_ID_LOGIN);
}

// 20. HANNE'S MAIN SCREEN
void create_screen_main_hanne() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main_hanne = obj;
    lv_obj_set_size(obj, 320, 240);

    lv_obj_t *label = lv_label_create(obj);
    lv_label_set_text(label, "Hanne Hansen");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    const char * btn_names[] = {"Daglig dosis", "Enkelt dosis", "Genopfyldning", "Ferie", "Indstillinger"};
    for(int i = 0; i < 5; i++) {
        lv_obj_t *btn = lv_button_create(obj);
        lv_obj_set_size(btn, 180, 32);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 45 + (i * 38));
        // PURPLE
        lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_PURPLE), 0);
        
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, btn_names[i]);
        lv_obj_center(lbl);
        // NO CALLBACKS (Inactive)
    }

    lv_obj_t *lock_btn = lv_button_create(obj);
    lv_obj_set_size(lock_btn, 40, 40);
    lv_obj_align(lock_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_color(lock_btn, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(lock_btn, nav_to_screen_cb, LV_EVENT_CLICKED, (void*)(intptr_t)SCREEN_ID_LOGIN);
}

void create_screen_daily_dose() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.daily_dose = obj;
    lv_obj_set_size(obj, 320, 240);

    lv_obj_t *label = lv_label_create(obj);
    lv_label_set_text(label, "Daglig dosis");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    const char * options[] = {"Morgen", "Middag", "Aften", "Hele dagen"};
    for(int i = 0; i < 4; i++) {
        lv_obj_t *btn = lv_button_create(obj);
        lv_obj_set_size(btn, 180, 40);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 50 + (i * 45));
        
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, options[i]);
        lv_obj_center(lbl);
        lv_obj_add_event_cb(btn, select_time_cb, LV_EVENT_CLICKED, NULL);
    }

    lv_obj_t *back = lv_button_create(obj);
    lv_obj_set_size(back, 30, 30);
    lv_obj_align(back, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_color(back, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(back, nav_to_screen_cb, LV_EVENT_CLICKED, (void*)(intptr_t)SCREEN_ID_MAIN);
}

void create_screen_dispense_confirm() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.dispense_confirm = obj;
    lv_obj_set_size(obj, 320, 240);

    lv_obj_t *header = lv_label_create(obj);
    lv_label_set_text(header, "Niels Hansen");
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);

    objects.confirm_label = lv_label_create(obj);
    lv_label_set_text(objects.confirm_label, "Morgen");
    lv_obj_align(objects.confirm_label, LV_ALIGN_TOP_MID, 0, 50);

    lv_obj_t *red_btn = lv_button_create(obj);
    lv_obj_set_size(red_btn, 140, 60);
    lv_obj_align(red_btn, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_bg_color(red_btn, lv_palette_main(LV_PALETTE_RED), 0);
    
    lv_obj_t *red_txt = lv_label_create(red_btn);
    lv_label_set_text(red_txt, "Dispenser");
    lv_obj_center(red_txt);

    lv_obj_t *back = lv_button_create(obj);
    lv_obj_set_size(back, 30, 30);
    lv_obj_align(back, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_color(back, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(back, nav_to_screen_cb, LV_EVENT_CLICKED, (void*)(intptr_t)SCREEN_ID_DAILY_DOSE);
}

void create_screen_single_dose() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.single_dose = obj;
    lv_obj_set_size(obj, 320, 240);

    lv_obj_t *label = lv_label_create(obj);
    lv_label_set_text(label, "Enkelt dosis");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    int start_y = 35;
    int step = 40;

    const char * col1_meds[] = {"Panodil", "Ipren", "Tom", "Tom"};
    const char * col1_nums[] = {"1", "3", "5", "7"};
    for(int i=0; i<4; i++) {
        lv_obj_t *num = lv_label_create(obj);
        lv_label_set_text(num, col1_nums[i]);
        lv_obj_align(num, LV_ALIGN_TOP_LEFT, 25, start_y + (i * step) + 10);

        lv_obj_t *btn = lv_button_create(obj);
        lv_obj_set_size(btn, 110, 35);
        lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 50, start_y + (i * step));
        
        lv_obj_t *txt = lv_label_create(btn);
        lv_label_set_text(txt, col1_meds[i]);
        lv_obj_center(txt);
        lv_obj_add_event_cb(btn, select_med_cb, LV_EVENT_CLICKED, NULL);
    }

    const char * col2_meds[] = {"Fiskeolie", "Multivitamin", "Tom", "Tom"};
    const char * col2_nums[] = {"2", "4", "6", "8"};
    for(int i=0; i<4; i++) {
        lv_obj_t *btn = lv_button_create(obj);
        lv_obj_set_size(btn, 110, 35);
        lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 175, start_y + (i * step));
        
        lv_obj_t *txt = lv_label_create(btn);
        lv_label_set_text(txt, col2_meds[i]);
        lv_obj_center(txt);
        lv_obj_add_event_cb(btn, select_med_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t *num = lv_label_create(obj);
        lv_label_set_text(num, col2_nums[i]);
        lv_obj_align(num, LV_ALIGN_TOP_LEFT, 295, start_y + (i * step) + 10);
    }

    lv_obj_t *back = lv_button_create(obj);
    lv_obj_set_size(back, 30, 30);
    lv_obj_align(back, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_color(back, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(back, nav_to_screen_cb, LV_EVENT_CLICKED, (void*)(intptr_t)SCREEN_ID_MAIN);
}

void create_screen_single_dose_qty() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.single_dose_qty = obj;
    lv_obj_set_size(obj, 320, 240);

    objects.med_title_label = lv_label_create(obj);
    lv_label_set_text(objects.med_title_label, "Panodil"); 
    lv_obj_align(objects.med_title_label, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *q1 = lv_label_create(obj);
    lv_label_set_text(q1, "Hvor mange");
    lv_obj_align(q1, LV_ALIGN_TOP_MID, 0, 35);

    lv_obj_t *q2 = lv_label_create(obj);
    lv_label_set_text(q2, "skal du have?");
    lv_obj_align(q2, LV_ALIGN_TOP_MID, 0, 55);

    lv_obj_t *minus_btn = lv_button_create(obj);
    lv_obj_set_size(minus_btn, 40, 40);
    lv_obj_align(minus_btn, LV_ALIGN_CENTER, -100, 0);
    lv_obj_set_style_bg_color(minus_btn, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_t *minus_lbl = lv_label_create(minus_btn);
    lv_label_set_text(minus_lbl, "-");
    lv_obj_center(minus_lbl);
    lv_obj_add_event_cb(minus_btn, qty_change_cb, LV_EVENT_CLICKED, (void*)(intptr_t)-1);

    lv_obj_t *bar = lv_obj_create(obj);
    lv_obj_set_size(bar, 140, 40);
    lv_obj_align(bar, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(bar, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    objects.qty_label = lv_label_create(bar);
    lv_label_set_text(objects.qty_label, "1");
    lv_obj_set_style_text_color(objects.qty_label, lv_color_white(), 0);
    lv_obj_center(objects.qty_label);

    lv_obj_t *plus_btn = lv_button_create(obj);
    lv_obj_set_size(plus_btn, 40, 40);
    lv_obj_align(plus_btn, LV_ALIGN_CENTER, 100, 0);
    lv_obj_set_style_bg_color(plus_btn, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_t *plus_lbl = lv_label_create(plus_btn);
    lv_label_set_text(plus_lbl, "+");
    lv_obj_center(plus_lbl);
    lv_obj_add_event_cb(plus_btn, qty_change_cb, LV_EVENT_CLICKED, (void*)(intptr_t)1);

    lv_obj_t *red_btn = lv_button_create(obj);
    lv_obj_set_size(red_btn, 120, 40);
    lv_obj_align(red_btn, LV_ALIGN_CENTER, 0, 60);
    lv_obj_set_style_bg_color(red_btn, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_t *red_txt = lv_label_create(red_btn);
    lv_label_set_text(red_txt, "Dispenser");
    lv_obj_center(red_txt);

    lv_obj_t *back = lv_button_create(obj);
    lv_obj_set_size(back, 30, 30);
    lv_obj_align(back, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_color(back, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(back, nav_to_screen_cb, LV_EVENT_CLICKED, (void*)(intptr_t)SCREEN_ID_SINGLE_DOSE);
}

// 8. REFILL SELECTION
void create_screen_refill_select() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.refill_select = obj;
    lv_obj_set_size(obj, 320, 240);

    lv_obj_t *label = lv_label_create(obj);
    lv_label_set_text(label, "Genopfyldning");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 5);

    lv_obj_t * list_cont = lv_obj_create(obj);
    lv_obj_set_size(list_cont, 320, 160);
    lv_obj_align(list_cont, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_flex_flow(list_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(list_cont, 2, 0);

    lv_obj_t * header = lv_obj_create(list_cont);
    lv_obj_set_size(header, 310, 25);
    lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(header, lv_palette_lighten(LV_PALETTE_GREY, 3), 0);

    lv_obj_t * h1 = lv_label_create(header); lv_label_set_text(h1, "Type"); lv_obj_align(h1, LV_ALIGN_LEFT_MID, 40, 0);
    lv_obj_t * h2 = lv_label_create(header); lv_label_set_text(h2, "Dosis"); lv_obj_align(h2, LV_ALIGN_LEFT_MID, 140, 0);
    lv_obj_t * h3 = lv_label_create(header); lv_label_set_text(h3, "Status"); lv_obj_align(h3, LV_ALIGN_LEFT_MID, 210, 0);
    lv_obj_t * h4 = lv_label_create(header); lv_label_set_text(h4, "Nr:"); lv_obj_align(h4, LV_ALIGN_LEFT_MID, 270, 0);

    for(int i=0; i<8; i++) {
        lv_obj_t * row = lv_obj_create(list_cont);
        lv_obj_set_size(row, 310, 30);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);

        lv_obj_t * cb = lv_checkbox_create(row);
        lv_checkbox_set_text(cb, ""); 
        lv_obj_align(cb, LV_ALIGN_LEFT_MID, 5, 0);
        lv_obj_add_event_cb(cb, refill_check_cb, LV_EVENT_VALUE_CHANGED, (void*)(intptr_t)i);

        char buf[16];
        lv_obj_t * name_lbl = lv_label_create(row); 
        lv_label_set_text(name_lbl, db_meds[i].name); 
        lv_obj_align(name_lbl, LV_ALIGN_LEFT_MID, 40, 0); 

        lv_obj_t * dose_lbl = lv_label_create(row); 
        lv_label_set_text(dose_lbl, db_meds[i].dose); 
        lv_obj_align(dose_lbl, LV_ALIGN_LEFT_MID, 140, 0); 
        
        if(db_meds[i].stock > 0 || strcmp(db_meds[i].name, "Tom") != 0) {
            sprintf(buf, "%d stk", db_meds[i].stock);
            lv_obj_t * stk_lbl = lv_label_create(row); 
            lv_label_set_text(stk_lbl, buf); 
            lv_obj_align(stk_lbl, LV_ALIGN_LEFT_MID, 210, 0); 
            
            refill_stock_labels[i] = stk_lbl; 
        } else {
            refill_stock_labels[i] = NULL;
        }

        sprintf(buf, "%d", db_meds[i].chamber);
        lv_obj_t * ch_lbl = lv_label_create(row); 
        lv_label_set_text(ch_lbl, buf); 
        lv_obj_align(ch_lbl, LV_ALIGN_LEFT_MID, 275, 0); 
    }

    lv_obj_t *next_btn = lv_button_create(obj);
    lv_obj_set_size(next_btn, 100, 40);
    lv_obj_align(next_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_style_bg_color(next_btn, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_t *next_lbl = lv_label_create(next_btn);
    lv_label_set_text(next_lbl, "Videre");
    lv_obj_center(next_lbl);
    lv_obj_add_event_cb(next_btn, refill_next_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back = lv_button_create(obj);
    lv_obj_set_size(back, 30, 30);
    lv_obj_align(back, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_color(back, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(back, nav_to_screen_cb, LV_EVENT_CLICKED, (void*)(intptr_t)SCREEN_ID_MAIN);
}

// 9. REFILL QUANTITY
void create_screen_refill_qty() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.refill_qty = obj;
    lv_obj_set_size(obj, 320, 240);

    lv_obj_t *label = lv_label_create(obj);
    lv_label_set_text(label, "Genopfyldning");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    objects.refill_title_label = lv_label_create(obj);
    lv_label_set_text(objects.refill_title_label, "Kammer X - Name");
    lv_obj_align(objects.refill_title_label, LV_ALIGN_TOP_MID, 0, 40);

    objects.refill_add_label = lv_label_create(obj);
    lv_label_set_text(objects.refill_add_label, "0");
    lv_obj_set_style_text_font(objects.refill_add_label, &lv_font_montserrat_14, 0); 
    lv_obj_align(objects.refill_add_label, LV_ALIGN_CENTER, -100, 0);

    lv_obj_t *plus_btn = lv_button_create(obj);
    lv_obj_set_size(plus_btn, 40, 40);
    lv_obj_align(plus_btn, LV_ALIGN_CENTER, 0, -25);
    lv_obj_set_style_bg_color(plus_btn, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_t *plus_lbl = lv_label_create(plus_btn);
    lv_label_set_text(plus_lbl, "+");
    lv_obj_center(plus_lbl);
    lv_obj_add_event_cb(plus_btn, refill_math_cb, LV_EVENT_CLICKED, (void*)(intptr_t)1);

    lv_obj_t *minus_btn = lv_button_create(obj);
    lv_obj_set_size(minus_btn, 40, 40);
    lv_obj_align(minus_btn, LV_ALIGN_CENTER, 0, 25);
    lv_obj_set_style_bg_color(minus_btn, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_t *minus_lbl = lv_label_create(minus_btn);
    lv_label_set_text(minus_lbl, "-");
    lv_obj_center(minus_lbl);
    lv_obj_add_event_cb(minus_btn, refill_math_cb, LV_EVENT_CLICKED, (void*)(intptr_t)-1);

    lv_obj_t * ant_lbl = lv_label_create(obj);
    lv_label_set_text(ant_lbl, "Antal");
    lv_obj_align(ant_lbl, LV_ALIGN_CENTER, 50, -30);
    
    objects.refill_cur_label = lv_label_create(obj);
    lv_label_set_text(objects.refill_cur_label, "0");
    lv_obj_align(objects.refill_cur_label, LV_ALIGN_CENTER, 50, -10);

    lv_obj_t * tot_lbl = lv_label_create(obj);
    lv_label_set_text(tot_lbl, "Ny Total");
    lv_obj_align(tot_lbl, LV_ALIGN_CENTER, 110, -30);

    objects.refill_total_label = lv_label_create(obj);
    lv_label_set_text(objects.refill_total_label, "0"); 
    lv_obj_align(objects.refill_total_label, LV_ALIGN_CENTER, 110, -10);

    lv_obj_t *conf_btn = lv_button_create(obj);
    lv_obj_set_size(conf_btn, 130, 35);
    lv_obj_align(conf_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_style_bg_color(conf_btn, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_t *conf_lbl = lv_label_create(conf_btn);
    lv_label_set_text(conf_lbl, "Genopfyldning");
    lv_obj_center(conf_lbl);
    lv_obj_add_event_cb(conf_btn, refill_confirm_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back = lv_button_create(obj);
    lv_obj_set_size(back, 30, 30);
    lv_obj_align(back, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_color(back, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(back, nav_to_screen_cb, LV_EVENT_CLICKED, (void*)(intptr_t)SCREEN_ID_REFILL_SELECT);
}

// 10. REFILL SUCCESS
void create_screen_refill_success() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.refill_success = obj;
    lv_obj_set_size(obj, 320, 240);

    lv_obj_t *label = lv_label_create(obj);
    lv_label_set_text(label, "Genopfyldning");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *instr = lv_label_create(obj);
    lv_label_set_text(instr, "Når du høre bippet, skal du åbne lågen og\nindsætte:");
    lv_obj_set_style_text_align(instr, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(instr, LV_ALIGN_TOP_MID, 0, 50);

    objects.success_med_label = lv_label_create(obj);
    lv_label_set_text(objects.success_med_label, "Ipren"); 
    lv_obj_set_style_text_font(objects.success_med_label, &lv_font_montserrat_20, 0); 
    lv_obj_align(objects.success_med_label, LV_ALIGN_CENTER, 0, 10);

    lv_obj_t *done_btn = lv_button_create(obj);
    lv_obj_set_size(done_btn, 130, 40);
    lv_obj_align(done_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(done_btn, lv_palette_main(LV_PALETTE_GREY), 0);
    
    lv_obj_t *done_lbl = lv_label_create(done_btn);
    lv_label_set_text(done_lbl, "Færdig");
    lv_obj_center(done_lbl);
    
    lv_obj_add_event_cb(done_btn, nav_to_screen_cb, LV_EVENT_CLICKED, (void*)(intptr_t)SCREEN_ID_REFILL_SELECT);

    lv_obj_t *back = lv_button_create(obj);
    lv_obj_set_size(back, 30, 30);
    lv_obj_align(back, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_color(back, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(back, nav_to_screen_cb, LV_EVENT_CLICKED, (void*)(intptr_t)SCREEN_ID_REFILL_QTY);
}

// 11. FERIE SCREEN
void create_screen_ferie() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.ferie = obj;
    lv_obj_set_size(obj, 320, 240);

    lv_obj_t *label = lv_label_create(obj);
    lv_label_set_text(label, "Ferie");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *minus_btn = lv_button_create(obj);
    lv_obj_set_size(minus_btn, 40, 40);
    lv_obj_align(minus_btn, LV_ALIGN_CENTER, -120, 0);
    lv_obj_set_style_bg_color(minus_btn, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_t *minus_lbl = lv_label_create(minus_btn);
    lv_label_set_text(minus_lbl, "-");
    lv_obj_center(minus_lbl);
    lv_obj_add_event_cb(minus_btn, ferie_math_cb, LV_EVENT_CLICKED, (void*)(intptr_t)-1);

    lv_obj_t *bar = lv_obj_create(obj);
    lv_obj_set_size(bar, 180, 40);
    lv_obj_align(bar, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(bar, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(bar, 10, 0);

    objects.ferie_days_label = lv_label_create(bar);
    lv_label_set_text(objects.ferie_days_label, "5 dage");
    lv_obj_set_style_text_color(objects.ferie_days_label, lv_color_white(), 0);
    lv_obj_center(objects.ferie_days_label);

    lv_obj_t *plus_btn = lv_button_create(obj);
    lv_obj_set_size(plus_btn, 40, 40);
    lv_obj_align(plus_btn, LV_ALIGN_CENTER, 120, 0);
    lv_obj_set_style_bg_color(plus_btn, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_t *plus_lbl = lv_label_create(plus_btn);
    lv_label_set_text(plus_lbl, "+");
    lv_obj_center(plus_lbl);
    lv_obj_add_event_cb(plus_btn, ferie_math_cb, LV_EVENT_CLICKED, (void*)(intptr_t)1);

    lv_obj_t *next_btn = lv_button_create(obj);
    lv_obj_set_size(next_btn, 130, 40);
    lv_obj_align(next_btn, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_style_bg_color(next_btn, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_t *next_lbl = lv_label_create(next_btn);
    lv_label_set_text(next_lbl, "Videre");
    lv_obj_center(next_lbl);
    lv_obj_add_event_cb(next_btn, ferie_start_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back = lv_button_create(obj);
    lv_obj_set_size(back, 30, 30);
    lv_obj_align(back, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_color(back, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(back, nav_to_screen_cb, LV_EVENT_CLICKED, (void*)(intptr_t)SCREEN_ID_MAIN);
}

// 12. FERIE DISPENSE LOOP SCREEN
void create_screen_ferie_dispense() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.ferie_dispense = obj;
    lv_obj_set_size(obj, 320, 240);

    lv_obj_t *label = lv_label_create(obj);
    lv_label_set_text(label, "Ferie");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *sub = lv_label_create(obj);
    lv_label_set_text(sub, "Dispenser for:");
    lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 40);

    objects.ferie_time_label = lv_label_create(obj);
    lv_label_set_text(objects.ferie_time_label, "Morgen");
    lv_obj_set_style_text_font(objects.ferie_time_label, &lv_font_montserrat_20, 0); 
    lv_obj_align(objects.ferie_time_label, LV_ALIGN_TOP_MID, 0, 70);

    objects.ferie_day_count_label = lv_label_create(obj);
    lv_label_set_text(objects.ferie_day_count_label, "Dag 1");
    lv_obj_set_style_text_font(objects.ferie_day_count_label, &lv_font_montserrat_20, 0); 
    lv_obj_align(objects.ferie_day_count_label, LV_ALIGN_TOP_MID, 0, 100);

    lv_obj_t *disp_btn = lv_button_create(obj);
    lv_obj_set_size(disp_btn, 160, 40);
    lv_obj_align(disp_btn, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_bg_color(disp_btn, lv_palette_main(LV_PALETTE_RED), 0);
    
    lv_obj_t *disp_lbl = lv_label_create(disp_btn);
    lv_label_set_text(disp_lbl, "Dispenser");
    lv_obj_center(disp_lbl);
    
    lv_obj_add_event_cb(disp_btn, ferie_dispense_next_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back = lv_button_create(obj);
    lv_obj_set_size(back, 30, 30);
    lv_obj_align(back, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_color(back, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(back, nav_to_screen_cb, LV_EVENT_CLICKED, (void*)(intptr_t)SCREEN_ID_FERIE);
}

// 13. SETTINGS MAIN SCREEN
void create_screen_settings() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.settings = obj;
    lv_obj_set_size(obj, 320, 240);

    lv_obj_t *label = lv_label_create(obj);
    lv_label_set_text(label, "Indstillinger");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    const char * btn_names[] = {"Bruger info", "Lystyrke", "Lydstyrke", "Alarm", "Log", "Avanceret"};
    int screen_ids[] = {
        SCREEN_ID_SETTINGS_INFO, 
        SCREEN_ID_SETTINGS_LIGHT, 
        SCREEN_ID_SETTINGS_SOUND, 
        SCREEN_ID_SETTINGS_ALARM, 
        SCREEN_ID_SETTINGS_LOG, 
        SCREEN_ID_SETTINGS_ADVANCED 
    };

    // 2 Columns, 3 Rows Grid
    for(int i = 0; i < 6; i++) {
        lv_obj_t *btn = lv_button_create(obj);
        lv_obj_set_size(btn, 130, 45);
        
        int col = i % 2;
        int row = i / 2;
        int x_pos = (col == 0) ? -75 : 75;
        int y_pos = 50 + (row * 55);

        lv_obj_align(btn, LV_ALIGN_TOP_MID, x_pos, y_pos);
        lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_BLUE), 0);
        
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, btn_names[i]);
        lv_obj_center(lbl);

        lv_obj_add_event_cb(btn, nav_to_screen_cb, LV_EVENT_CLICKED, (void*)(intptr_t)screen_ids[i]);
    }

    lv_obj_t *back = lv_button_create(obj);
    lv_obj_set_size(back, 30, 30);
    lv_obj_align(back, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_color(back, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(back, nav_to_screen_cb, LV_EVENT_CLICKED, (void*)(intptr_t)SCREEN_ID_MAIN);
}

// 14. SETTINGS: BRUGER INFO
void create_screen_settings_info() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.settings_info = obj;
    lv_obj_set_size(obj, 320, 240);

    lv_obj_t *header = lv_label_create(obj);
    lv_label_set_text(header, "Bruger info");
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);

    const char * info_labels[] = {
        "Navn:", "Niels Hansen",
        "Alder:", "75",
        "Pin:", "0211",
        "Telefon nr:", "53754895",
        "Pårørendes navn:", "Helle Nielsen (Datter)",
        "Pårørendes nr:", "71356485"
    };

    int y_start = 40;
    for(int i=0; i<6; i++) {
        lv_obj_t * key = lv_label_create(obj);
        lv_label_set_text(key, info_labels[i*2]);
        lv_obj_align(key, LV_ALIGN_TOP_LEFT, 10, y_start + (i*25));

        lv_obj_t * val = lv_label_create(obj);
        lv_label_set_text(val, info_labels[i*2+1]);
        lv_obj_align(val, LV_ALIGN_TOP_RIGHT, -10, y_start + (i*25));
    }

    lv_obj_t *back = lv_button_create(obj);
    lv_obj_set_size(back, 30, 30);
    lv_obj_align(back, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_color(back, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(back, nav_to_screen_cb, LV_EVENT_CLICKED, (void*)(intptr_t)SCREEN_ID_SETTINGS);
}

// 15. SETTINGS: LYSTYRKE
void create_screen_settings_light() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.settings_light = obj;
    lv_obj_set_size(obj, 320, 240);

    lv_obj_t *header = lv_label_create(obj);
    lv_label_set_text(header, "Indstillinger");
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *sub = lv_label_create(obj);
    lv_label_set_text(sub, "Lysstyrke");
    lv_obj_align(sub, LV_ALIGN_CENTER, 0, -40);

    lv_obj_t *slider = lv_slider_create(obj);
    lv_obj_set_size(slider, 200, 10);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 0);
    lv_slider_set_value(slider, 20, LV_ANIM_OFF);

    lv_obj_t *zero = lv_label_create(obj);
    lv_label_set_text(zero, "0%");
    lv_obj_align(zero, LV_ALIGN_CENTER, -100, 20);

    lv_obj_t *hundred = lv_label_create(obj);
    lv_label_set_text(hundred, "100%");
    lv_obj_align(hundred, LV_ALIGN_CENTER, 100, 20);

    objects.light_slider_label = lv_label_create(obj);
    lv_label_set_text(objects.light_slider_label, "20%");
    lv_obj_set_style_text_font(objects.light_slider_label, &lv_font_montserrat_20, 0);
    lv_obj_align(objects.light_slider_label, LV_ALIGN_CENTER, 0, 40);

    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, objects.light_slider_label);

    lv_obj_t *back = lv_button_create(obj);
    lv_obj_set_size(back, 30, 30);
    lv_obj_align(back, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_color(back, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(back, nav_to_screen_cb, LV_EVENT_CLICKED, (void*)(intptr_t)SCREEN_ID_SETTINGS);
}

// 16. SETTINGS: LYDSTYRKE
void create_screen_settings_sound() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.settings_sound = obj;
    lv_obj_set_size(obj, 320, 240);

    lv_obj_t *header = lv_label_create(obj);
    lv_label_set_text(header, "Indstillinger");
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *sub = lv_label_create(obj);
    lv_label_set_text(sub, "Lydstyrke");
    lv_obj_align(sub, LV_ALIGN_CENTER, 0, -40);

    lv_obj_t *slider = lv_slider_create(obj);
    lv_obj_set_size(slider, 200, 10);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 0);
    lv_slider_set_value(slider, 20, LV_ANIM_OFF);

    lv_obj_t *zero = lv_label_create(obj);
    lv_label_set_text(zero, "0%");
    lv_obj_align(zero, LV_ALIGN_CENTER, -100, 20);

    lv_obj_t *hundred = lv_label_create(obj);
    lv_label_set_text(hundred, "100%");
    lv_obj_align(hundred, LV_ALIGN_CENTER, 100, 20);

    objects.sound_slider_label = lv_label_create(obj);
    lv_label_set_text(objects.sound_slider_label, "20%");
    lv_obj_set_style_text_font(objects.sound_slider_label, &lv_font_montserrat_20, 0);
    lv_obj_align(objects.sound_slider_label, LV_ALIGN_CENTER, 0, 40);

    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, objects.sound_slider_label);

    lv_obj_t *back = lv_button_create(obj);
    lv_obj_set_size(back, 30, 30);
    lv_obj_align(back, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_color(back, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(back, nav_to_screen_cb, LV_EVENT_CLICKED, (void*)(intptr_t)SCREEN_ID_SETTINGS);
}

// 17. SETTINGS: ALARM (TOGGLE BUTTONS)
void create_screen_settings_alarm() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.settings_alarm = obj;
    lv_obj_set_size(obj, 320, 240);

    lv_obj_t *header = lv_label_create(obj);
    lv_label_set_text(header, "Indstillinger");
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *sub = lv_label_create(obj);
    lv_label_set_text(sub, "Alarm");
    lv_obj_align(sub, LV_ALIGN_CENTER, 0, -40);

    // Left Button (ON)
    objects.alarm_on_btn = lv_button_create(obj);
    lv_obj_set_size(objects.alarm_on_btn, 100, 40);
    lv_obj_align(objects.alarm_on_btn, LV_ALIGN_CENTER, -60, 0);
    lv_obj_add_event_cb(objects.alarm_on_btn, alarm_toggle_cb, LV_EVENT_CLICKED, (void*)(intptr_t)1);

    objects.alarm_on_label = lv_label_create(objects.alarm_on_btn);
    lv_obj_center(objects.alarm_on_label);

    // Right Button (OFF)
    objects.alarm_off_btn = lv_button_create(obj);
    lv_obj_set_size(objects.alarm_off_btn, 100, 40);
    lv_obj_align(objects.alarm_off_btn, LV_ALIGN_CENTER, 60, 0);
    lv_obj_add_event_cb(objects.alarm_off_btn, alarm_toggle_cb, LV_EVENT_CLICKED, (void*)(intptr_t)0);

    objects.alarm_off_label = lv_label_create(objects.alarm_off_btn);
    lv_obj_center(objects.alarm_off_label);

    // Initial State Update
    update_alarm_ui();

    lv_obj_t *back = lv_button_create(obj);
    lv_obj_set_size(back, 30, 30);
    lv_obj_align(back, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_color(back, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(back, nav_to_screen_cb, LV_EVENT_CLICKED, (void*)(intptr_t)SCREEN_ID_SETTINGS);
}

// 18. SETTINGS: LOG
void create_screen_settings_log() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.settings_log = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 240);
    
    lv_obj_t *parent_obj = obj;

    // TITLE
    {
        lv_obj_t *lbl = lv_label_create(parent_obj);
        lv_obj_set_pos(lbl, 0, 8);
        lv_obj_set_size(lbl, 320, 18);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(lbl, "Log");
    }

    // BACK BUTTON
    {
        lv_obj_t *btn = lv_button_create(parent_obj);
        lv_obj_set_pos(btn, 10, 200);
        lv_obj_set_size(btn, 30, 30);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xff6c6e71), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_event_cb(btn, nav_to_screen_cb, LV_EVENT_CLICKED, (void*)(intptr_t)SCREEN_ID_SETTINGS);
    }

    // HORIZONTAL LINES
    {
        lv_obj_t *line = lv_line_create(parent_obj);
        lv_obj_set_pos(line, 0, 30);
        static lv_point_precise_t p[] = { { 0, 0 }, { 320, 0 } };
        lv_line_set_points(line, p, 2);
    }
    {
        lv_obj_t *line = lv_line_create(parent_obj);
        lv_obj_set_pos(line, 0, 50);
        static lv_point_precise_t p[] = { { 0, 0 }, { 320, 0 } };
        lv_line_set_points(line, p, 2);
    }
    {
        lv_obj_t *line = lv_line_create(parent_obj);
        lv_obj_set_pos(line, 0, 122);
        static lv_point_precise_t p[] = { { 0, 0 }, { 320, 0 } };
        lv_line_set_points(line, p, 2);
    }
    {
        lv_obj_t *line = lv_line_create(parent_obj);
        lv_obj_set_pos(line, 0, 192);
        static lv_point_precise_t p[] = { { 0, 0 }, { 320, 0 } };
        lv_line_set_points(line, p, 2);
    }

    // VERTICAL LINES
    {
        lv_obj_t *line = lv_line_create(parent_obj);
        lv_obj_set_pos(line, 115, 30);
        static lv_point_precise_t p[] = { { 0, 0 }, { 0, 162 } };
        lv_line_set_points(line, p, 2);
    }
    {
        lv_obj_t *line = lv_line_create(parent_obj);
        lv_obj_set_pos(line, 245, 30);
        static lv_point_precise_t p[] = { { 0, 0 }, { 0, 162 } };
        lv_line_set_points(line, p, 2);
    }

    // HEADERS
    {
        lv_obj_t *lbl = lv_label_create(parent_obj);
        lv_obj_set_pos(lbl, 0, 33);
        lv_obj_set_size(lbl, 115, LV_SIZE_CONTENT);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_text(lbl, "Dato");
    }
    {
        lv_obj_t *lbl = lv_label_create(parent_obj);
        lv_obj_set_pos(lbl, 115, 33);
        lv_obj_set_size(lbl, 130, LV_SIZE_CONTENT);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_text(lbl, "Tidspunkt");
    }
    {
        lv_obj_t *lbl = lv_label_create(parent_obj);
        lv_obj_set_pos(lbl, 245, 33);
        lv_obj_set_size(lbl, 75, LV_SIZE_CONTENT);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_text(lbl, "Status");
    }

    // --- ROW 1 (Day 1) ---
    // Date
    {
        lv_obj_t *lbl = lv_label_create(parent_obj);
        lv_obj_set_pos(lbl, 0, 77);
        lv_obj_set_size(lbl, 115, LV_SIZE_CONTENT);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_text(lbl, "20-01-2026");
    }
    // Times
    {
        lv_obj_t *l1 = lv_label_create(parent_obj); lv_label_set_text(l1, "Morgen");
        lv_obj_set_pos(l1, 116, 56); lv_obj_set_size(l1, 129, LV_SIZE_CONTENT); 
        lv_obj_set_style_text_align(l1, LV_TEXT_ALIGN_CENTER, 0);

        lv_obj_t *l2 = lv_label_create(parent_obj); lv_label_set_text(l2, "Middag");
        lv_obj_set_pos(l2, 116, 80); lv_obj_set_size(l2, 129, LV_SIZE_CONTENT); 
        lv_obj_set_style_text_align(l2, LV_TEXT_ALIGN_CENTER, 0);

        lv_obj_t *l3 = lv_label_create(parent_obj); lv_label_set_text(l3, "Aften");
        lv_obj_set_pos(l3, 117, 103); lv_obj_set_size(l3, 129, LV_SIZE_CONTENT); 
        lv_obj_set_style_text_align(l3, LV_TEXT_ALIGN_CENTER, 0);
    }
    // Checkboxes (All checked)
    {
        lv_obj_t *cb1 = lv_checkbox_create(parent_obj); lv_checkbox_set_text(cb1, "");
        lv_obj_set_pos(cb1, 272, 54); lv_obj_add_state(cb1, LV_STATE_CHECKED); lv_obj_remove_flag(cb1, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *cb2 = lv_checkbox_create(parent_obj); lv_checkbox_set_text(cb2, "");
        lv_obj_set_pos(cb2, 272, 77); lv_obj_add_state(cb2, LV_STATE_CHECKED); lv_obj_remove_flag(cb2, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *cb3 = lv_checkbox_create(parent_obj); lv_checkbox_set_text(cb3, "");
        lv_obj_set_pos(cb3, 272, 100); lv_obj_add_state(cb3, LV_STATE_CHECKED); lv_obj_remove_flag(cb3, LV_OBJ_FLAG_CLICKABLE);
    }

    // --- ROW 2 (Day 2) ---
    // Date
    {
        lv_obj_t *lbl = lv_label_create(parent_obj);
        lv_obj_set_pos(lbl, 0, 147);
        lv_obj_set_size(lbl, 115, LV_SIZE_CONTENT);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_text(lbl, "21-01-2026");
    }
    // Times
    {
        lv_obj_t *l1 = lv_label_create(parent_obj); lv_label_set_text(l1, "Morgen");
        lv_obj_set_pos(l1, 116, 128); lv_obj_set_size(l1, 129, LV_SIZE_CONTENT); 
        lv_obj_set_style_text_align(l1, LV_TEXT_ALIGN_CENTER, 0);

        lv_obj_t *l2 = lv_label_create(parent_obj); lv_label_set_text(l2, "Middag");
        lv_obj_set_pos(l2, 116, 152); lv_obj_set_size(l2, 129, LV_SIZE_CONTENT); 
        lv_obj_set_style_text_align(l2, LV_TEXT_ALIGN_CENTER, 0);

        lv_obj_t *l3 = lv_label_create(parent_obj); lv_label_set_text(l3, "Aften");
        lv_obj_set_pos(l3, 117, 175); lv_obj_set_size(l3, 129, LV_SIZE_CONTENT); 
        lv_obj_set_style_text_align(l3, LV_TEXT_ALIGN_CENTER, 0);
    }
    // Checkboxes (Only Middle Checked)
    {
        lv_obj_t *cb1 = lv_checkbox_create(parent_obj); lv_checkbox_set_text(cb1, "");
        lv_obj_set_pos(cb1, 272, 125); lv_obj_remove_flag(cb1, LV_OBJ_FLAG_CLICKABLE); 

        lv_obj_t *cb2 = lv_checkbox_create(parent_obj); lv_checkbox_set_text(cb2, "");
        lv_obj_set_pos(cb2, 272, 148); lv_obj_add_state(cb2, LV_STATE_CHECKED); lv_obj_remove_flag(cb2, LV_OBJ_FLAG_CLICKABLE); 

        lv_obj_t *cb3 = lv_checkbox_create(parent_obj); lv_checkbox_set_text(cb3, "");
        lv_obj_set_pos(cb3, 272, 171); lv_obj_remove_flag(cb3, LV_OBJ_FLAG_CLICKABLE); 
    }
}

// 19. SETTINGS: ADVANCED (NEW: LOADING SPINNER)
void create_screen_settings_advanced() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.settings_advanced = obj;
    lv_obj_set_size(obj, 320, 240);

    // Label
    lv_obj_t *label = lv_label_create(obj);
    lv_label_set_text(label, "Tilslutter telefon\nvia bluetooth...");
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -20); 

    // Spinner (LVGL v9 style)
    lv_obj_t *spinner = lv_spinner_create(obj);
    lv_spinner_set_anim_params(spinner, 1000, 60);
    lv_obj_set_size(spinner, 50, 50);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 40); 

    // Back Button
    lv_obj_t *back = lv_button_create(obj);
    lv_obj_set_size(back, 30, 30);
    lv_obj_align(back, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_color(back, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(back, nav_to_screen_cb, LV_EVENT_CLICKED, (void*)(intptr_t)SCREEN_ID_SETTINGS);
}

void create_screens() {
    create_screen_login();
    create_screen_dispenser();
    create_screen_main();
    create_screen_daily_dose();
    create_screen_dispense_confirm();
    create_screen_single_dose();
    create_screen_single_dose_qty();
    create_screen_refill_select(); 
    create_screen_refill_qty();    
    create_screen_refill_success(); 
    create_screen_ferie(); 
    create_screen_ferie_dispense();
    create_screen_settings(); 
    create_screen_settings_info();
    create_screen_settings_light();
    create_screen_settings_sound();
    create_screen_settings_alarm();
    create_screen_settings_log();
    create_screen_settings_advanced(); 
    create_screen_main_hanne(); // Hanne Last
}

void tick_screen(int screen_index) {
}