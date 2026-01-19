#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    // --- ORDER MUST MATCH ENUM EXACTLY ---
    lv_obj_t *login;             // 1
    lv_obj_t *dispenser;         // 2
    lv_obj_t *main;              // 3
    lv_obj_t *daily_dose;        // 4
    lv_obj_t *dispense_confirm;  // 5
    lv_obj_t *single_dose;       // 6
    lv_obj_t *single_dose_qty;   // 7
    lv_obj_t *refill_select;     // 8
    lv_obj_t *refill_qty;        // 9
    lv_obj_t *refill_success;    // 10
    lv_obj_t *ferie;             // 11
    lv_obj_t *ferie_dispense;    // 12
    lv_obj_t *settings;          // 13
    
    // Settings Sub-screens
    lv_obj_t *settings_info;     // 14
    lv_obj_t *settings_light;    // 15
    lv_obj_t *settings_sound;    // 16
    lv_obj_t *settings_alarm;    // 17
    lv_obj_t *settings_log;      // 18
    lv_obj_t *settings_advanced; // 19

    // Hanne's Screen (MOVED TO END TO MATCH ID 20)
    lv_obj_t *main_hanne;        // 20

    // --- NON-SCREEN OBJECTS (Order doesn't matter here) ---
    // Dynamic Labels
    lv_obj_t *confirm_label;     
    lv_obj_t *med_title_label;   
    lv_obj_t *qty_label;         
    
    // Refill Labels
    lv_obj_t *refill_title_label; 
    lv_obj_t *refill_add_label;   
    lv_obj_t *refill_cur_label;   
    lv_obj_t *refill_total_label; 
    lv_obj_t *success_med_label; 

    // Ferie Labels
    lv_obj_t *ferie_days_label; 
    lv_obj_t *ferie_time_label; 
    lv_obj_t *ferie_day_count_label; 

    // Settings Sliders
    lv_obj_t *light_slider_label;
    lv_obj_t *sound_slider_label;

    // Alarm Toggle Objects
    lv_obj_t *alarm_on_btn;
    lv_obj_t *alarm_on_label;
    lv_obj_t *alarm_off_btn;
    lv_obj_t *alarm_off_label;

} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_LOGIN = 1,
    SCREEN_ID_DISPENSER = 2,
    SCREEN_ID_MAIN = 3,
    SCREEN_ID_DAILY_DOSE = 4,
    SCREEN_ID_DISPENSE_CONFIRM = 5,
    SCREEN_ID_SINGLE_DOSE = 6,
    SCREEN_ID_SINGLE_DOSE_QTY = 7,
    SCREEN_ID_REFILL_SELECT = 8,
    SCREEN_ID_REFILL_QTY = 9,
    SCREEN_ID_REFILL_SUCCESS = 10,
    SCREEN_ID_FERIE = 11,
    SCREEN_ID_FERIE_DISPENSE = 12,
    SCREEN_ID_SETTINGS = 13,
    SCREEN_ID_SETTINGS_INFO = 14,   
    SCREEN_ID_SETTINGS_LIGHT = 15,  
    SCREEN_ID_SETTINGS_SOUND = 16,  
    SCREEN_ID_SETTINGS_ALARM = 17,  
    SCREEN_ID_SETTINGS_LOG = 18,
    SCREEN_ID_SETTINGS_ADVANCED = 19,
    SCREEN_ID_MAIN_HANNE = 20 
};

void create_screen_login();
void create_screen_dispenser();
void create_screen_main();
void create_screen_daily_dose();
void create_screen_dispense_confirm();
void create_screen_single_dose();
void create_screen_single_dose_qty();
void create_screen_refill_select();
void create_screen_refill_qty();
void create_screen_refill_success();
void create_screen_ferie();
void create_screen_ferie_dispense();
void create_screen_settings();
void create_screen_settings_info();   
void create_screen_settings_light();  
void create_screen_settings_sound();  
void create_screen_settings_alarm();  
void create_screen_settings_log();    
void create_screen_settings_advanced(); 
void create_screen_main_hanne(); // Hanne's Function
void create_screens();

void tick_screen(int screen_index);

#ifdef __cplusplus
}
#endif
#endif