/*
 * Bruce Shark desktop shell for Flipper Zero firmware.
 * This replaces the desktop home screen inside the firmware, not a .fap app.
 * Relies on Flipper native apps for RF/NFC/IR/USB/etc; cannot run ESP32 code.
 * License: MIT (this new file), depends on upstream Flipper GPL software.
 */
#include <furi.h>
#include <furi_hal.h>
#include <gui/view.h>
#include <gui/canvas.h>
#include <input/input.h>
#include <notification/notification_app.h>
#include <notification/notification_messages.h>
#include <power/power_service/power.h>
#include <stdio.h>
#include <string.h>
#include "desktop_view_main.h"

#define BRUCE_CATEGORIES 12
#define BRUCE_POWEROFF_MS 5000

typedef enum {
    BruceSplash,
    BruceHome,
    BruceList,
    BruceInfo,
    BruceClock,
    BruceBattery,
} BrucePage;

typedef enum {
    BruceLaunch,
    BruceApps,
    BruceClockAction,
    BruceBatteryAction,
    BruceLED,
    BruceVibe,
    BruceBacklight,
    BruceLock,
    BrucePower,
    BruceInfoAction,
} BruceAction;

typedef struct {
    const char* title;
    BruceAction action;
    const char* target;
    const char* info;
} BruceItem;

typedef struct {
    const char* label;
    const BruceItem* items;
    uint8_t count;
} BruceCategory;

#define ITEM(t, a, x, msg) {t, a, x, msg}
#define CAT(n, arr) {n, arr, sizeof(arr) / sizeof(arr[0])}

static const BruceItem wifi_items[] = {
    ITEM("WiFi hardware", BruceInfoAction, NULL, "Needs ESP32 WiFi board"),
    ITEM("Installed WiFi apps", BruceApps, NULL, NULL),
};
static const BruceItem ble_items[] = {
    ITEM("Bluetooth settings", BruceLaunch, "Bluetooth", NULL),
    ITEM("Installed BLE apps", BruceApps, NULL, NULL),
};
static const BruceItem rf_items[] = {
    ITEM("Sub-GHz", BruceLaunch, "Sub-GHz", NULL),
    ITEM("Saved signals", BruceLaunch, "Archive", NULL),
};
static const BruceItem nfc_items[] = {
    ITEM("NFC tools", BruceLaunch, "NFC", NULL),
    ITEM("Saved NFC", BruceLaunch, "Archive", NULL),
};
static const BruceItem rfid_items[] = {
    ITEM("125 kHz RFID", BruceLaunch, "125 kHz RFID", NULL),
    ITEM("Saved RFID", BruceLaunch, "Archive", NULL),
};
static const BruceItem ir_items[] = {
    ITEM("Infrared", BruceLaunch, "Infrared", NULL),
    ITEM("Saved remotes", BruceLaunch, "Archive", NULL),
};
static const BruceItem ibutton_items[] = {
    ITEM("iButton", BruceLaunch, "iButton", NULL),
    ITEM("Saved keys", BruceLaunch, "Archive", NULL),
};
static const BruceItem usb_items[] = {
    ITEM("Bad USB", BruceLaunch, "Bad USB", NULL),
    ITEM("U2F key", BruceLaunch, "U2F", NULL),
    ITEM("More USB apps", BruceApps, NULL, NULL),
};
static const BruceItem gpio_items[] = {
    ITEM("GPIO tools", BruceLaunch, "GPIO", NULL),
    ITEM("ESP32 expansion", BruceInfoAction, NULL, "ESP32 board required"),
};
static const BruceItem files_items[] = {
    ITEM("File archive", BruceLaunch, "Archive", NULL),
    ITEM("Installed apps", BruceApps, NULL, NULL),
};
static const BruceItem tools_items[] = {
    ITEM("Clock", BruceClockAction, NULL, NULL),
    ITEM("Battery", BruceBatteryAction, NULL, NULL),
    ITEM("Blink LED", BruceLED, NULL, NULL),
    ITEM("Vibrate", BruceVibe, NULL, NULL),
    ITEM("Toggle backlight", BruceBacklight, NULL, NULL),
};
static const BruceItem config_items[] = {
    ITEM("Bluetooth", BruceLaunch, "Bluetooth", NULL),
    ITEM("Apps / settings", BruceApps, NULL, NULL),
    ITEM("Lock device", BruceLock, NULL, NULL),
    ITEM("Power off", BrucePower, NULL, NULL),
    ITEM("About Bruce Shark", BruceInfoAction, NULL, "Flipper native firmware"),
};

static const BruceCategory bruce_categories[BRUCE_CATEGORIES] = {
    CAT("WiFi", wifi_items), CAT("BLE", ble_items), CAT("RF", rf_items),
    CAT("NFC", nfc_items), CAT("RFID", rfid_items), CAT("IR", ir_items),
    CAT("iButton", ibutton_items), CAT("USB", usb_items), CAT("GPIO", gpio_items),
    CAT("Files", files_items), CAT("Others", tools_items), CAT("Config", config_items),
};

typedef struct {
    BrucePage page;
    uint8_t category;
    uint8_t selected;
    bool led_on;
    bool backlight_on;
    const char* info;
} BruceViewModel;

struct DesktopMainView {
    View* view;
    DesktopMainViewCallback callback;
    void* context;
    FuriTimer* poweroff_timer;
    bool dummy_mode;
    const char* launch_target;
};

/* A custom monochrome shark outline and grin for the Bruce splash identity.
 * The screen is 128x64 (Flipper hardware); T-Embed's colored TFT is different.
 */
static void bruce_draw_shark(Canvas* c, int16_t x, int16_t y) {
    /* Body, fins and pointed snout */
    canvas_draw_line(c, x+1, y+12, x+8, y+6);
    canvas_draw_line(c, x+8, y+6, x+26, y+2);
    canvas_draw_line(c, x+26, y+2, x+35, y+6);
    canvas_draw_line(c, x+35, y+6, x+44, y+3);
    canvas_draw_line(c, x+44, y+3, x+40, y+11);
    canvas_draw_line(c, x+40, y+11, x+51, y+14);
    canvas_draw_line(c, x+51, y+14, x+41, y+17);
    canvas_draw_line(c, x+41, y+17, x+33, y+22);
    canvas_draw_line(c, x+33, y+22, x+21, y+24);
    canvas_draw_line(c, x+21, y+24, x+15, y+30);
    canvas_draw_line(c, x+15, y+30, x+12, y+21);
    canvas_draw_line(c, x+12, y+21, x+1, y+12);
    /* Predator's curved grin and sawtooth teeth */
    canvas_draw_line(c, x+8, y+15, x+18, y+19);
    canvas_draw_line(c, x+18, y+19, x+32, y+17);
    canvas_draw_line(c, x+32, y+17, x+42, y+13);
    for(int i=0; i<6; ++i) {
        canvas_draw_line(c, x+12+i*4, y+17+(i>3?0:1), x+14+i*4, y+21-(i>3?1:0));
    }
    canvas_draw_disc(c, x+34, y+10, 2);
    canvas_set_color(c, ColorWhite);
    canvas_draw_dot(c, x+34, y+10);
    canvas_set_color(c, ColorBlack);
}

static void bruce_draw_category_icon(Canvas* c, uint8_t cat) {
    const int16_t x=64, y=35;
    switch(cat) {
    case 0: /* Wifi waves */
        canvas_draw_line(c,x-18,y-7,x,y-14); canvas_draw_line(c,x,y-14,x+18,y-7);
        canvas_draw_line(c,x-12,y-1,x,y-6); canvas_draw_line(c,x,y-6,x+12,y-1);
        canvas_draw_line(c,x-6,y+5,x,y+2); canvas_draw_line(c,x,y+2,x+6,y+5);
        canvas_draw_disc(c,x,y+11,2); break;
    case 1: /* BLE */
        canvas_draw_line(c,x,y-16,x,y+16); canvas_draw_line(c,x,y-16,x+11,y-6);
        canvas_draw_line(c,x+11,y-6,x-10,y+10); canvas_draw_line(c,x-10,y-10,x+11,y+6);
        canvas_draw_line(c,x+11,y+6,x,y+16); break;
    case 2: /* RF antenna */
        canvas_draw_line(c,x,y-9,x,y+15); canvas_draw_line(c,x-6,y+15,x+6,y+15);
        canvas_draw_circle(c,x,y-11,2);
        canvas_draw_line(c,x-10,y-15,x-15,y-7);canvas_draw_line(c,x-15,y-7,x-10,y+1);
        canvas_draw_line(c,x+10,y-15,x+15,y-7);canvas_draw_line(c,x+15,y-7,x+10,y+1); break;
    case 3: /* NFC */
        canvas_draw_rframe(c,x-19,y-15,38,30,4);
        canvas_draw_line(c,x-9,y-8,x-3,y);canvas_draw_line(c,x-3,y,x-9,y+8);
        canvas_draw_line(c,x+1,y-8,x+7,y);canvas_draw_line(c,x+7,y,x+1,y+8); break;
    case 4: /* RFID */
        canvas_draw_rframe(c,x-17,y-15,34,30,3);
        canvas_draw_rframe(c,x-11,y-10,22,20,3);
        canvas_draw_line(c,x-6,y,x+6,y);canvas_draw_line(c,x,y-5,x,y+5); break;
    case 5: /* IR */
        canvas_draw_rframe(c,x-10,y-16,20,32,3);
        canvas_draw_disc(c,x,y-8,3);canvas_draw_circle(c,x,y+3,4);
        canvas_draw_line(c,x+15,y-7,x+20,y-9);canvas_draw_line(c,x+15,y,x+21,y); break;
    case 6: /* iButton */
        canvas_draw_circle(c,x,y,16);canvas_draw_circle(c,x,y,10);
        canvas_draw_disc(c,x,y,4); break;
    case 7: /* USB */
        canvas_draw_line(c,x,y-16,x,y+13);canvas_draw_line(c,x,y-16,x-5,y-10);
        canvas_draw_line(c,x,y-16,x+5,y-10);canvas_draw_line(c,x,y+4,x-11,y-2);
        canvas_draw_disc(c,x-11,y-2,2);canvas_draw_line(c,x,y-2,x+11,y-8);
        canvas_draw_box(c,x+9,y-10,5,5);canvas_draw_frame(c,x-3,y+13,6,5); break;
    case 8: /* GPIO chip */
        canvas_draw_rframe(c,x-16,y-11,32,22,3);
        for(int i=0;i<4;i++){canvas_draw_line(c,x-21,y-9+i*6,x-16,y-9+i*6);
                             canvas_draw_line(c,x+16,y-9+i*6,x+21,y-9+i*6);}
        canvas_set_font(c,FontSecondary);canvas_draw_str_aligned(c,x,y+3,AlignCenter,AlignBottom,"IC");break;
    case 9: /* files */
        canvas_draw_frame(c,x-18,y-10,36,27);canvas_draw_frame(c,x-18,y-14,15,5);
        canvas_draw_line(c,x-12,y-2,x+12,y-2);canvas_draw_line(c,x-12,y+4,x+7,y+4);break;
    case 10: /* tools */
        canvas_draw_circle(c,x,y,15);canvas_draw_line(c,x,y,x,y-9);
        canvas_draw_line(c,x,y,x+8,y+5);canvas_draw_disc(c,x,y,2);break;
    default: /* config */
        canvas_draw_circle(c,x,y,12);canvas_draw_circle(c,x,y,5);
        for(int i=-1;i<=1;i++){canvas_draw_box(c,x+i*8-2,y-17,4,4);
                                 canvas_draw_box(c,x+i*8-2,y+13,4,4);}break;
    }
}

static void bruce_draw_header(Canvas* c) {
    Power* power = furi_record_open(RECORD_POWER);
    PowerInfo info;
    power_get_info(power, &info);
    furi_record_close(RECORD_POWER);
    char stats[24];
    snprintf(stats, sizeof(stats), "BAT %u%%", (unsigned)info.charge);
    canvas_set_color(c, ColorBlack);
    canvas_set_font(c, FontSecondary);
    canvas_draw_str(c, 3, 9, "BRUCE SHARK");
    canvas_draw_str_aligned(c, 125, 9, AlignRight, AlignBottom, stats);
    canvas_draw_line(c, 2, 12, 125, 12);
}

static void bruce_draw(Canvas* c, void* context) {
    BruceViewModel* m = context;
    if(!m) return;
    canvas_clear(c);
    bruce_draw_header(c);
    canvas_set_font(c, FontSecondary);
    if(m->page == BruceSplash) {
        bruce_draw_shark(c, 38, 17);
        canvas_set_font(c, FontPrimary);
        canvas_draw_str_aligned(c, 64, 59, AlignCenter, AlignBottom, "BRUCE");
        canvas_set_font(c, FontSecondary);
        canvas_draw_str_aligned(c, 125, 62, AlignRight, AlignBottom, ">");
    } else if(m->page == BruceHome) {
        bruce_draw_category_icon(c, m->category);
        canvas_draw_str(c, 8, 40, "<");canvas_draw_str(c, 116, 40, ">");
        canvas_draw_line(c, 30, 52, 98, 52);
        canvas_set_font(c, FontPrimary);
        canvas_draw_str_aligned(c, 64, 63, AlignCenter, AlignBottom,
                                bruce_categories[m->category].label);
    } else if(m->page == BruceList) {
        const BruceCategory* cat = &bruce_categories[m->category];
        canvas_draw_str(c, 4, 23, cat->label);
        for(uint8_t i = 0; i < cat->count; ++i) {
            int16_t offset = (int16_t)i - (int16_t)m->selected;
            if(offset < -1 || offset > 2) continue;
            int16_t y = 36 + offset * 10;
            if(y < 18 || y > 59) continue;
            if(i == m->selected) {
                canvas_draw_rbox(c, 2, y-9, 124, 11, 2);
                canvas_set_color(c, ColorWhite);
            }
            canvas_set_font(c, FontSecondary);
            canvas_draw_str(c, 8, y, cat->items[i].title);
            canvas_set_color(c, ColorBlack);
        }
    } else if(m->page == BruceClock) {
        DateTime t; furi_hal_rtc_get_datetime(&t);
        char time[20]; snprintf(time, sizeof(time), "%02u:%02u:%02u",t.hour,t.minute,t.second);
        canvas_set_font(c,FontPrimary);
        canvas_draw_str_aligned(c,64,36,AlignCenter,AlignBottom,time);
        canvas_set_font(c,FontSecondary);
        snprintf(time,sizeof(time),"%04u-%02u-%02u",t.year,t.month,t.day);
        canvas_draw_str_aligned(c,64,50,AlignCenter,AlignBottom,time);
        canvas_draw_str(c,4,62,"< BACK");
    } else if(m->page == BruceBattery) {
        Power* power = furi_record_open(RECORD_POWER);
        PowerInfo info; power_get_info(power,&info); furi_record_close(RECORD_POWER);
        char str[32];
        snprintf(str,sizeof(str),"Charge  %u%%",(unsigned)info.charge);
        canvas_set_font(c,FontPrimary);canvas_draw_str(c,5,30,str);
        canvas_set_font(c,FontSecondary);
        snprintf(str,sizeof(str),"Health %u%%",(unsigned)info.health);
        canvas_draw_str(c,5,43,str);
        canvas_draw_str(c,5,54,info.is_charging ? "Charging" : "Battery power");
    } else { /* BruceInfo */
        bruce_draw_shark(c,39,18);
        canvas_draw_str_aligned(c,64,61,AlignCenter,AlignBottom,
             m->info ? m->info : "Bruce Shark firmware");
    }
}

static void bruce_notify(const NotificationSequence* seq) {
    NotificationApp* n = furi_record_open(RECORD_NOTIFICATION);
    notification_message(n, seq);
    furi_record_close(RECORD_NOTIFICATION);
}

static void bruce_main_action(DesktopMainView* main_view, BruceViewModel* m) {
    const BruceItem* item = &bruce_categories[m->category].items[m->selected];
    switch(item->action) {
    case BruceLaunch:
        if(item->target && strcmp(item->target, "Archive") == 0) {
            main_view->callback(DesktopMainEventOpenArchive, main_view->context);
        } else {
            main_view->launch_target = item->target;
            main_view->callback(DesktopMainEventBruceLaunch, main_view->context);
        }
        break;
    case BruceApps:
        main_view->callback(DesktopMainEventOpenMenu, main_view->context);
        break;
    case BruceClockAction:m->page = BruceClock;break;
    case BruceBatteryAction:m->page = BruceBattery;break;
    case BruceLED:
        m->led_on = !m->led_on;
        bruce_notify(m->led_on ? &sequence_blink_start_blue : &sequence_blink_stop);
        m->info = m->led_on ? "LED blinking ON" : "LED blinking OFF";
        m->page = BruceInfo;break;
    case BruceVibe:
        bruce_notify(&sequence_single_vibro);
        m->info = "Vibration sent";m->page = BruceInfo;break;
    case BruceBacklight:
        m->backlight_on = !m->backlight_on;
        bruce_notify(m->backlight_on ? &sequence_display_backlight_enforce_on :
                                        &sequence_display_backlight_enforce_auto);
        m->info = m->backlight_on ? "Backlight always ON" : "Backlight automatic";
        m->page = BruceInfo;break;
    case BruceLock:
        main_view->callback(DesktopMainEventLock, main_view->context);break;
    case BrucePower:
        main_view->callback(DesktopMainEventOpenPowerOff, main_view->context);break;
    case BruceInfoAction:
        m->info = item->info;m->page = BruceInfo;break;
    }
}

static void bruce_poweroff_timeout(void* context) {
    DesktopMainView* main_view = context;
    if(main_view->callback)
        main_view->callback(DesktopMainEventOpenPowerOff, main_view->context);
}

static bool bruce_input(InputEvent* event, void* context) {
    DesktopMainView* v = context;
    BruceViewModel* m = view_get_model(v->view);
    if(event->key == InputKeyBack) {
        if(event->type == InputTypePress) {
            furi_timer_start(v->poweroff_timer, BRUCE_POWEROFF_MS);
        } else if(event->type == InputTypeRelease) {
            furi_timer_stop(v->poweroff_timer);
        }
    }
    if(event->type == InputTypeLong && event->key == InputKeyUp) {
        if(v->callback) v->callback(DesktopMainEventLock, v->context);
    } else if(event->type == InputTypeShort) {
        if(event->key == InputKeyBack) {
            if(m->page == BruceSplash) {
                m->page = BruceHome;
            } else if(m->page == BruceHome) {
                if(v->callback) v->callback(DesktopMainEventOpenMenu,v->context);
            } else if(m->page == BruceList) {
                m->page = BruceHome;
            } else {
                m->page = BruceList;
            }
        } else if(m->page == BruceSplash) {
            m->page = BruceHome;
        } else if(m->page == BruceHome) {
            if(event->key == InputKeyLeft) {
                m->category = (m->category + BRUCE_CATEGORIES - 1) % BRUCE_CATEGORIES;
            } else if(event->key == InputKeyRight) {
                m->category = (m->category + 1) % BRUCE_CATEGORIES;
            } else if(event->key == InputKeyOk || event->key == InputKeyDown) {
                m->selected = 0;
                m->page = BruceList;
            } else if(event->key == InputKeyUp) {
                if(v->callback) v->callback(DesktopMainEventOpenLockMenu, v->context);
            }
        } else if(m->page == BruceList) {
            const BruceCategory* cat = &bruce_categories[m->category];
            if(event->key == InputKeyUp) {
                m->selected = (m->selected + cat->count - 1) % cat->count;
            } else if(event->key == InputKeyDown) {
                m->selected = (m->selected + 1) % cat->count;
            } else if(event->key == InputKeyOk && v->callback) {
                bruce_main_action(v, m);
            } else if(event->key == InputKeyLeft) {
                m->page = BruceHome;
            }
        } else if(event->key == InputKeyOk) {
            m->page = BruceList;
        }
    }
    view_commit_model(v->view, true);
    return true;
}

void desktop_main_set_callback(DesktopMainView* v, DesktopMainViewCallback cb, void* context) {
    furi_assert(v);furi_assert(cb);
    v->callback=cb;v->context=context;
}
View* desktop_main_get_view(DesktopMainView* v) {furi_assert(v);return v->view;}
void desktop_main_set_dummy_mode_state(DesktopMainView* v, bool dummy) {
    furi_assert(v);v->dummy_mode=dummy;
}
const char* desktop_main_get_launch_target(DesktopMainView* v) {
    furi_assert(v); return v->launch_target;
}
DesktopMainView* desktop_main_alloc(void) {
    DesktopMainView* v = malloc(sizeof(DesktopMainView));
    v->view=view_alloc();
    view_allocate_model(v->view,ViewModelTypeLockFree,sizeof(BruceViewModel));
    BruceViewModel* m = view_get_model(v->view);
    memset(m,0,sizeof(BruceViewModel));
    m->page=BruceSplash;
    v->launch_target=NULL;
    view_set_context(v->view,v);
    view_set_draw_callback(v->view,bruce_draw);
    view_set_input_callback(v->view,bruce_input);
    v->poweroff_timer=furi_timer_alloc(bruce_poweroff_timeout,FuriTimerTypeOnce,v);
    return v;
}
void desktop_main_free(DesktopMainView* v) {
    furi_assert(v);
    furi_timer_stop(v->poweroff_timer);
    furi_timer_free(v->poweroff_timer);
    view_free(v->view);
    free(v);
}
