#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;
static GFont intellecta_font;
static GFont intellecta_sm_font;
static TextLayer *s_weather_layer;
static TextLayer *s_message_layer;

static BitmapLayer *kitty_layer;
static BitmapLayer *battery_bitmap_layer;
static GBitmap *kitty;
static GBitmap *BatteryCharge;
static GBitmap *Battery100;
static GBitmap *Battery75;
static GBitmap *Battery50;
static GBitmap *Battery25;
static GBitmap *Battery0;

static void handle_battery(BatteryChargeState charge_state) {
  if (charge_state.is_charging) {
    bitmap_layer_set_bitmap(battery_bitmap_layer, BatteryCharge);
  } else {
    if (charge_state.charge_percent > 75){
      bitmap_layer_set_bitmap(battery_bitmap_layer, Battery100);
    } else if (charge_state.charge_percent >50){
      bitmap_layer_set_bitmap(battery_bitmap_layer, Battery75);
    } else if (charge_state.charge_percent > 25){
      bitmap_layer_set_bitmap(battery_bitmap_layer, Battery50);
    } else if (charge_state.charge_percent > 10){
      bitmap_layer_set_bitmap(battery_bitmap_layer, Battery25);
    } else if (charge_state.charge_percent > 0 || charge_state.charge_percent == 0){
      bitmap_layer_set_bitmap(battery_bitmap_layer, Battery0);
    }
  }
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void main_window_load(Window *window) {
  
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  intellecta_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_INTELLECTA_48));
  intellecta_sm_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_INTELLECTA_24));
  
  // Load graphics
  kitty = gbitmap_create_with_resource(RESOURCE_ID_KITTY);
  BatteryCharge = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_CHARGE);
  Battery100 = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_100);
  Battery75 = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_75);
  Battery50 = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_50);
  Battery25 = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_25);
  Battery0 = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_EMPTY);

  // Create the BitmapLayer
  kitty_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(kitty_layer, kitty);
  bitmap_layer_set_alignment(kitty_layer, GAlignBottomRight );
  
  
  // Time layer
  s_time_layer = text_layer_create(GRect(0, -5, bounds.size.w - 5, 58));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorMagenta);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentRight);
  text_layer_set_font(s_time_layer, intellecta_font);
  
  // Battery Layer  
  battery_bitmap_layer = bitmap_layer_create(GRect(3, 0, 20, 38));
  bitmap_layer_set_alignment(battery_bitmap_layer, GAlignBottomRight );
  
  // Weather Layer
  s_weather_layer = text_layer_create(GRect(3, 45, 45, 30));
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, GColorMagenta);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentLeft);
  text_layer_set_font(s_weather_layer, intellecta_sm_font);
  text_layer_set_text(s_weather_layer, "...");
  
  // Message Layer
  s_message_layer = text_layer_create(GRect(-10, bounds.size.h -55, bounds.size.w - 5, 58));
  text_layer_set_background_color(s_message_layer, GColorClear);
  text_layer_set_text_color(s_message_layer, GColorMagenta);
  text_layer_set_text_alignment(s_message_layer, GTextAlignmentCenter);
  text_layer_set_font(s_message_layer, intellecta_font);
  text_layer_set_text(s_message_layer, "Hello");
  
  // Put it all together
  battery_state_service_subscribe(handle_battery);
  layer_add_child(window_layer, bitmap_layer_get_layer(kitty_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(battery_bitmap_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_message_layer));
  handle_battery(battery_state_service_peek());
  
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  static char temperature_buffer[8];
  //static char conditions_buffer[32];
  static char weather_layer_buffer[32];
  
  // Read tuples for data
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
  //Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);
  
  // If all data is available, use it
  if(temp_tuple) {
    snprintf(temperature_buffer, sizeof(temperature_buffer), "%d", (int)temp_tuple->value->int32);
    //snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
  }
   // Assemble full string and display
  //snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
  snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s", temperature_buffer);
  text_layer_set_text(s_weather_layer, weather_layer_buffer);
    
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void main_window_unload(Window *window) {
  // Unload GFont
  fonts_unload_custom_font(intellecta_font);
  fonts_unload_custom_font(intellecta_sm_font);

  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  gbitmap_destroy(kitty);
  gbitmap_destroy(BatteryCharge);
  gbitmap_destroy(Battery100);
  gbitmap_destroy(Battery75);
  gbitmap_destroy(Battery50);
  gbitmap_destroy(Battery25);
  gbitmap_destroy(Battery0);
  bitmap_layer_destroy(kitty_layer);
  bitmap_layer_destroy(battery_bitmap_layer);
  text_layer_destroy(s_weather_layer);
  text_layer_destroy(s_message_layer);
}


static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // Make sure the time is displayed from the start
  update_time();

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  const int inbox_size = 128;
  const int outbox_size = 128;
  app_message_open(inbox_size, outbox_size);
  
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
 