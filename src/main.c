#include <pebble.h>
#include "main.h"

static Window *s_main_window;
static TextLayer *s_time_layer , *s_date_layer;

static Layer  *s_battery_layer;
static int s_battery_level;
static GFont s_font;

ClaySettings settings;

// Initialize the default settings
static void prv_default_settings() {
  settings.BackgroundColor = GColorWhite;
  settings.TextColor = GColorBlack;
  settings.BarColor = GColorBrightGreen;
}

static void prv_load_settings() {
  // Load the default settings
  prv_default_settings();
  // Read settings from persistent storage, if they exist
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

// Update the display elements
static void prv_update_display() {

  text_layer_set_text_color(s_time_layer, settings.TextColor);
  window_set_background_color(s_main_window,settings.BackgroundColor);
  text_layer_set_text_color(s_date_layer, settings.TextColor);
}

// Save the settings to persistent storage
static void prv_save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
  // Update the display based on new settings
  prv_update_display();
}

static void prv_inbox_received_handler(DictionaryIterator *iter, void *context) {
  //get background color value
  Tuple *bg_color_t = dict_find(iter, MESSAGE_KEY_BackgroundColor);
  if(bg_color_t) {
    settings.BackgroundColor = GColorFromHEX(bg_color_t->value->int32);
  }
  //get text color value
  Tuple *text_color_t = dict_find(iter, MESSAGE_KEY_TextColor);
  if(text_color_t) {
    settings.TextColor = GColorFromHEX(text_color_t->value->int32);
  }
  
  //get bar color value
  Tuple *bar_color_t = dict_find(iter, MESSAGE_KEY_BarColor);
  if(bar_color_t) {
    settings.BarColor = GColorFromHEX(bar_color_t->value->int32);
  }
  
  prv_save_settings();
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H.%M" : "%I.%M", tick_time);

  //get date 
  static char date_buffer[16];
  strftime(date_buffer, sizeof(date_buffer), "%A, %e %b %Y", tick_time);
  
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
}


static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Find the width of the bar
  int width = (int)(float)(((float)s_battery_level / 100.0F) * 114.0F);

  // Draw the background
  graphics_context_set_fill_color(ctx, GColorDarkGray );
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Draw the bar
  graphics_context_set_fill_color(ctx, GColorBrightGreen);
  graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
}

static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
  layer_mark_dirty(s_battery_layer);
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(0, 4, bounds.size.w, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer,fonts_get_system_font(FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  // Create battery meter Layer
  s_battery_layer = layer_create(GRect(14, 55, 115, 3));
  layer_add_child(window_get_root_layer(s_main_window), s_battery_layer);
  
  //create datelayer
  s_date_layer = text_layer_create(GRect(0, 59, bounds.size.w, 30));
  text_layer_set_text_color(s_date_layer, GColorBlack);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_font(s_time_layer,fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  layer_destroy(s_battery_layer);
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();
  prv_load_settings();
  
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  s_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_KASIH_48));
  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // Make sure the time is displayed from the start
  update_time();
  
  
  // Register with battery service
  battery_state_service_subscribe(battery_callback);
  battery_callback(battery_state_service_peek());
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  
  // Listen for AppMessages
  app_message_register_inbox_received(prv_inbox_received_handler);
  app_message_open(128, 128);
  
  // Register with TickTimerService  
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}