#include <pebble.h>

#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1
#define KEY_LOCATION 2
#define KEY_SUNSET 3
#define KEY_SUNRISE 4

static Window *s_main_window;
static TextLayer *s_time_layer, *s_date_layer;
static TextLayer *s_weather_layer;
static TextLayer *s_sun_layer;
static int s_battery_level;
static Layer *s_battery_layer;
static GColor textColour;
static GColor textBackgroundColour;
static GColor batteryBarColour;

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  static char location_buffer[32];
  static char sunset_buffer[9];
  static char sunrise_buffer[9];
  
  static char sun_layer_buffer[32];
  static char weather_layer_buffer[32];

  // Read tuples for data
  Tuple *temp_tuple = dict_find(iterator, KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, KEY_CONDITIONS);
  Tuple *location_tuple = dict_find(iterator, KEY_LOCATION);
  Tuple *sunset_tuple = dict_find(iterator, KEY_SUNSET);
  Tuple *sunrise_tuple = dict_find(iterator, KEY_SUNRISE);
  
  // If all data is available, use it
  if(temp_tuple && conditions_tuple) {
    snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", (int)temp_tuple->value->int32);
    snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
    snprintf(location_buffer, sizeof(location_buffer), "%s", location_tuple->value->cstring);
    snprintf(sunset_buffer, sizeof(sunset_buffer), "%s", sunset_tuple->value->cstring);
    snprintf(sunrise_buffer, sizeof(sunrise_buffer), "%s", sunrise_tuple->value->cstring);
    
    // Assemble full string and display
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s & %s in %s", temperature_buffer, conditions_buffer, location_buffer);
    // Assemble sunset & sunrise string
    snprintf(sun_layer_buffer, sizeof(sun_layer_buffer), "%s || %s", sunset_buffer, sunrise_buffer);
    
    text_layer_set_text(s_sun_layer, sun_layer_buffer);
    text_layer_set_text(s_weather_layer, weather_layer_buffer);
  }
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

static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
  // Update meter
  layer_mark_dirty(s_battery_layer);
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);
  
  // Copy date into buffer from tm structure
static char date_buffer[16];
strftime(date_buffer, sizeof(date_buffer), "%a %d %b", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
  // Show the date
  text_layer_set_text(s_date_layer, date_buffer);
  
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Find the width of the bar
  int width = (int)(float)(((float)s_battery_level / 100.0F) * 114.0F);

  // Draw the background
  graphics_context_set_fill_color(ctx, batteryBarColour);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Draw the bar
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, PBL_IF_ROUND_ELSE(50, 50), bounds.size.w, 25), 0, GCornerNone);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();

  // Get weather update every 30 minutes
  if(tick_time->tm_min % 15 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);

    // Send the message!
    app_message_outbox_send();
  }
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create the TextLayer with specific bounds GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));
  s_time_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(10, 10), bounds.size.w, 50));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, textBackgroundColour);
  text_layer_set_text_color(s_time_layer, textColour);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT ));

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
  // Create date TextLayer
s_date_layer = text_layer_create(
  GRect(0, PBL_IF_ROUND_ELSE(10, 70), bounds.size.w, 50));
  
  //GColorBlack
text_layer_set_background_color(s_date_layer, textBackgroundColour);
  text_layer_set_text_color(s_date_layer, textColour);
  text_layer_set_text(s_date_layer, "dd:mm:yyyy");
text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));

// Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  
  // Create temperature Layer
  s_weather_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(125, 130), bounds.size.w, 30));

  // Style the text
  text_layer_set_background_color(s_weather_layer, textBackgroundColour);
  text_layer_set_text_color(s_weather_layer, textColour);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
  text_layer_set_text(s_weather_layer, "hold on boss...");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT ));
  
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));
  
  // Sun Layer 
  s_sun_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(95, 95), bounds.size.w, 25));
  text_layer_set_background_color(s_sun_layer, textBackgroundColour);
  text_layer_set_text_color(s_sun_layer, textColour);
  text_layer_set_text_alignment(s_sun_layer, GTextAlignmentCenter);
  text_layer_set_text(s_sun_layer, "<><><><><><>");
  text_layer_set_font(s_sun_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_sun_layer));
  
  // Create battery meter Layer
s_battery_layer = layer_create(GRect(14, 54, 115, 2));
layer_set_update_proc(s_battery_layer, battery_update_proc);
  
  // Add to Window
layer_add_child(window_get_root_layer(window), s_battery_layer);
  
}

static void main_window_unload(Window *window) {
  // Destroy time layer
  text_layer_destroy(s_time_layer);

  // Destroy weather elements
  text_layer_destroy(s_weather_layer);
  
  // Distroy Date layer 
  text_layer_destroy(s_date_layer);
  
  // Distory battery layer 
  layer_destroy(s_battery_layer);
  
  // Distory sun layer 
  text_layer_destroy(s_sun_layer);
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set the background color
  window_set_background_color(s_main_window, GColorLightGray);
  
  textColour = GColorImperialPurple;
  textBackgroundColour = GColorLightGray;
  batteryBarColour = GColorMintGreen;

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
  
  // Register for battery level updates
  battery_state_service_subscribe(battery_callback);
  
  // Ensure battery level is displayed from the start
  battery_callback(battery_state_service_peek());
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
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