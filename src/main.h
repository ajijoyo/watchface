#include <pebble.h>

#define SETTINGS_KEY 1

// A structure containing our settings
typedef struct ClaySettings {
  GColor BackgroundColor;
  GColor TextColor;
  GColor BarColor;
} __attribute__((__packed__)) ClaySettings;

static void prv_default_settings();
static void prv_load_settings();
static void prv_save_settings();
static void prv_update_display();
static void update_time();
static void init();
static void tick_handler(struct tm *tick_time, TimeUnits units_changed);
static void battery_update_proc(Layer *layer, GContext *ctx);
static void battery_callback(BatteryChargeState state);
static void prv_inbox_received_handler(DictionaryIterator *iter, void *context);
