#include <pebble.h>

enum {
  KEY_TEMPERATURE = 0,
  KEY_CONDITIONS = 1,
  KEY_EVENT_NAME = 2,
  KEY_EVENT_DAY = 3,
  KEY_EVENT_MONTH = 4,
  KEY_EVENT_YEAR = 5,
  KEY_EVENT_HOUR = 6,
  KEY_EVENT_MINUTE = 7
};
  
static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_weather_layer;
static GBitmap *s_weather_icon_bitmap;
static BitmapLayer *s_weather_icon_layer;
static GBitmap *s_bitmap;
static BitmapLayer *s_bitmap_layer;
static GBitmap *s_card_bitmap;
static BitmapLayer *s_card_bitmap_layer;
static GBitmap *s_battery_bitmap;
static BitmapLayer *s_battery_bitmap_layer;
static Layer *s_battery_inside_layer;
static GBitmap *s_bluetooth_bitmap;
static BitmapLayer *s_bluettoth_bitmap_layer;

// Event vars
static GFont s_res_gothic_14;
static TextLayer *s_textlayer_time;
static TextLayer *s_textlayer_event_title;
static TextLayer *s_textlayer_event_time;

// Store incoming information
static char temperature_buffer[8];
static char event_name_buffer[64];
static char event_time_buffer[16];
static char time_to_event_buffer[16];

// struct to keep the event time once retrieved
static struct tm event_time;
static int nextappointment = 1;
static int battery_percent = 100;

/******************************************** ANIMATIONS ***************************************************/


/******************************************** BLUETOOTH STATUS ***************************************************/

static void handle_bluetooth(bool connected) {
  if (connected){
    s_bluetooth_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BLUETOOTH_ON_ICON);
  }
  else {
    s_bluetooth_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BLUETOOTH_OFF_ICON); 
  }
  bitmap_layer_set_bitmap(s_bluettoth_bitmap_layer,s_bluetooth_bitmap);
}

/******************************************** BATTERY STATUS ***************************************************/

static void draw_battery_status(Layer *battery_layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(battery_layer);

  int battery_width = (bounds.size.w * battery_percent) / 100;

  if (battery_percent <= 20) {
    graphics_context_set_fill_color(ctx,GColorRed);
  }
  else {
    graphics_context_set_fill_color(ctx,GColorWhite); 
  }

  graphics_fill_rect(ctx,GRect(bounds.origin.x, bounds.origin.y, battery_width, bounds.size.h),0,GCornerNone);
}

static void handle_battery(BatteryChargeState charge_state) {
  battery_percent = charge_state.charge_percent;
  layer_mark_dirty(s_battery_inside_layer);
}

/******************************************** EVENTS AND MESSAGES ***************************************************/

static void calculate_time_to_event(){
  // Also calculate the time to event for the top label
  time_t now = time(NULL);
  int seconds = 0;

  seconds = difftime(mktime(&event_time), now);
  if (seconds < 0) {
    seconds = 0;
  }
  APP_LOG(APP_LOG_LEVEL_INFO, "seconds to event: %d", seconds);

  if (seconds > 60) {
    seconds /= 60;
    //minutes
    if (seconds > 60) {
      seconds /= 60;
      // hours
      if (seconds > 24) {
        seconds /= 24;
        // days
        if (seconds > 31) {
          seconds /= 31;
          //months
          snprintf(time_to_event_buffer, sizeof(time_to_event_buffer), "%d months", seconds);
        }
        else {
          snprintf(time_to_event_buffer, sizeof(time_to_event_buffer), "%d days", seconds);
        }
      }
      else {
        snprintf(time_to_event_buffer, sizeof(time_to_event_buffer), "%d hours", seconds);
      }
    }
    else {
      snprintf(time_to_event_buffer, sizeof(time_to_event_buffer), "%d minutes", seconds);
    }
  }
  else {
    snprintf(time_to_event_buffer, sizeof(time_to_event_buffer), "%d seconds", seconds);
  }

  text_layer_set_text(s_textlayer_time, time_to_event_buffer);
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "00:00";
  static char date_buffer[10];

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }
  strftime(date_buffer, sizeof(date_buffer), "%a %e", tick_time);
  
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
  text_layer_set_text(s_date_layer, date_buffer);

  // Update time to event
  calculate_time_to_event();
}

// this function is called when the event time is received from the google calendar
static void handle_event_time(int day, int month, int year, int hour, int minutes) {
  // Assemble event time
  event_time.tm_mday = day;
  event_time.tm_mon = month;
  event_time.tm_year = year - 1900;
  event_time.tm_min = minutes;
  event_time.tm_hour = hour;
  mktime(&event_time);

  strftime(event_time_buffer, sizeof(event_time_buffer), "%H:%M", &event_time);
  text_layer_set_text(s_textlayer_event_time, event_time_buffer);

  calculate_time_to_event();
}

// Called when receiving the weather conditions
static void handle_weather_icon(int conditions_id){
  int icon_resource_id = 0;
  switch (conditions_id) {
    case 1:
      icon_resource_id = RESOURCE_ID_WEATHER_ICON_CLEAR;
      break;
    case 2: 
      icon_resource_id = RESOURCE_ID_WEATHER_ICON_FEW_CLOUDS;
      break;
    case 3: 
      icon_resource_id = RESOURCE_ID_WEATHER_ICON_SCATTERED_CLOUDS;
      break;
    case 4:
      icon_resource_id = RESOURCE_ID_WEATHER_ICON_BROKEN_CLOUDS;
      break;
    case 9: 
      icon_resource_id = RESOURCE_ID_WEATHER_ICON_SHOWER_RAIN;
      break;
    case 10: 
      icon_resource_id = RESOURCE_ID_WEATHER_ICON_RAIN;
      break;
    case 11: 
      icon_resource_id = RESOURCE_ID_WEATHER_ICON_THUNDERSTORM;
      break;
    case 13:
      icon_resource_id = RESOURCE_ID_WEATHER_ICON_SNOW;
      break;
    case 50:
      icon_resource_id = RESOURCE_ID_WEATHER_ICON_MIST;
      break;
    default:
      icon_resource_id = 0;
  }

  if (icon_resource_id > 0){
    s_weather_icon_bitmap = gbitmap_create_with_resource(icon_resource_id);
    bitmap_layer_set_bitmap(s_weather_icon_layer,s_weather_icon_bitmap);
  }

}

/******************************************** UI ***************************************************/

static void main_window_load(Window *window) {
  // Create the background image
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BG_IMAGE);

  s_bitmap_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);
  #ifdef PBL_PLATFORM_APLITE
    bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpAssign);
  #elif PBL_PLATFORM_BASALT
    bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpSet);
  #endif
    layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer));
  
  
  // Create time TextLayer
  if (nextappointment==0){
    s_time_layer = text_layer_create(GRect(0, 56, 144, 40));
  }
  else {
    s_time_layer = text_layer_create(GRect(0, 26, 144, 40));
  }
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  // Time TextLayer Font
  #ifdef PBL_PLATFORM_BASALT
      text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_38_BOLD_NUMBERS));
  #elif PBL_PLATFORM_APLITE
      text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  #endif
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));

  // Create date TextLayer
  if (nextappointment==0){
    s_date_layer = text_layer_create(GRect(22, 93, 47, 15));
  }
  else {
    s_date_layer = text_layer_create(GRect(22, 63, 47, 15));
  }
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentLeft);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
  
  // Create temperature Layer
  s_weather_layer = text_layer_create(GRect(84, 0, 40, 20));
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, GColorWhite);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentRight);
  text_layer_set_font(s_weather_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text(s_weather_layer, "...");
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));

  // Create weather conditions icon layer
  s_weather_icon_layer = bitmap_layer_create(GRect(124,0,20,20));
  bitmap_layer_set_compositing_mode(s_weather_icon_layer, GCompOpSet);
  layer_add_child(window_layer,bitmap_layer_get_layer(s_weather_icon_layer));

  // Create battery Layer
  s_battery_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_ICON);
  s_battery_bitmap_layer = bitmap_layer_create(GRect(11,7,14,7));
  bitmap_layer_set_bitmap(s_battery_bitmap_layer, s_battery_bitmap);
  bitmap_layer_set_compositing_mode(s_battery_bitmap_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_battery_bitmap_layer));

  // Create layer to draw the battery level inside
  s_battery_inside_layer = layer_create(GRect(12,8,10,5));
  layer_add_child(window_layer, s_battery_inside_layer);  
  layer_set_update_proc(s_battery_inside_layer,draw_battery_status);

  // Bluetooth status Layer
  s_bluettoth_bitmap_layer = bitmap_layer_create(GRect(28, 3, 15, 15));
  bitmap_layer_set_compositing_mode(s_bluettoth_bitmap_layer,GCompOpSet);

  if (bluetooth_connection_service_peek()){
    s_bluetooth_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BLUETOOTH_ON_ICON);
  }
  else {
    s_bluetooth_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BLUETOOTH_OFF_ICON); 
  }
  bitmap_layer_set_bitmap(s_bluettoth_bitmap_layer,s_bluetooth_bitmap);
  layer_add_child(window_layer,bitmap_layer_get_layer(s_bluettoth_bitmap_layer));

  // Card layer
  s_card_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CARD_IMAGE);
  s_card_bitmap_layer = bitmap_layer_create(GRect(4,89,136,75));
  bitmap_layer_set_bitmap(s_card_bitmap_layer, s_card_bitmap);
  bitmap_layer_set_compositing_mode(s_card_bitmap_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_card_bitmap_layer));

  // Load font 
  s_res_gothic_14 = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  // Time to the event
  s_textlayer_time = text_layer_create(GRect(16, 98, 120, 17));
  text_layer_set_text(s_textlayer_time, "10 min");
  text_layer_set_background_color(s_textlayer_time, GColorClear);
  layer_add_child(window_get_root_layer(window), (Layer *)s_textlayer_time);
  
  // Title of the event
  s_textlayer_event_title = text_layer_create(GRect(16, 113, 120, 20));
  text_layer_set_text(s_textlayer_event_title, "Picnic with friends");
  text_layer_set_font(s_textlayer_event_title, s_res_gothic_14);
  text_layer_set_background_color(s_textlayer_event_title, GColorClear);
  layer_add_child(window_get_root_layer(window), (Layer *)s_textlayer_event_title);
  
  // Time and place (or other info)
  s_textlayer_event_time = text_layer_create(GRect(16, 136, 120, 26));
  text_layer_set_text(s_textlayer_event_time, "12:30 PM");
  text_layer_set_font(s_textlayer_event_time, s_res_gothic_14);
  text_layer_set_background_color(s_textlayer_event_time, GColorClear);
  layer_add_child(window_get_root_layer(window), (Layer *)s_textlayer_event_time);
 
}

static void main_window_unload(Window *window) {
    // Destroy TextLayer
    text_layer_destroy(s_time_layer);
    text_layer_destroy(s_date_layer);
    bitmap_layer_destroy(s_bitmap_layer);
    gbitmap_destroy(s_bitmap);
    text_layer_destroy(s_weather_layer);
    bitmap_layer_destroy(s_weather_icon_layer);
    gbitmap_destroy(s_weather_icon_bitmap);
    bitmap_layer_destroy(s_battery_bitmap_layer);
    gbitmap_destroy(s_battery_bitmap);
    layer_destroy(s_battery_inside_layer);
    bitmap_layer_destroy(s_bluettoth_bitmap_layer);
    gbitmap_destroy(s_bluetooth_bitmap);
    bitmap_layer_destroy(s_card_bitmap_layer);
    gbitmap_destroy(s_card_bitmap);
    text_layer_destroy(s_textlayer_time);
    text_layer_destroy(s_textlayer_event_title);
    text_layer_destroy(s_textlayer_event_time);
    battery_state_service_unsubscribe();
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  handle_battery(battery_state_service_peek());
  // Get weather update every 30 minutes
  if(tick_time->tm_min % 30 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
  
    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);
  
    // Send the message!
    app_message_outbox_send();
  }

}


/******************************************** APP MESSAGES ***************************************************/

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Read first item
  Tuple *t = dict_read_first(iterator);

  int weather_conditions_id = 0;

	int day = 0;
  int month = 0;
  int year = 0;
  int hour = 0;
  int minutes = 0;

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
      case KEY_TEMPERATURE:
        snprintf(temperature_buffer, sizeof(temperature_buffer), "%d°C", (int)t->value->int32);
        text_layer_set_text(s_weather_layer, temperature_buffer);
        break;
      case KEY_CONDITIONS:
        weather_conditions_id = (int)t->value->int32;
        break;
      case KEY_EVENT_NAME:
    		snprintf(event_name_buffer, sizeof(event_name_buffer), "%s", t->value->cstring);
    		text_layer_set_text(s_textlayer_event_title, event_name_buffer);
        break;
      case KEY_EVENT_DAY:
        //12-6-2015 23:55 
     	  day = (int)t->value->int32;
        break;
      case KEY_EVENT_MONTH:
        month = (int)t->value->int32;
        break;
      case KEY_EVENT_YEAR:
        year = (int)t->value->int32;
        break;
      case KEY_EVENT_HOUR:
        hour = (int)t->value->int32;
        break;
      case KEY_EVENT_MINUTE:
        minutes = (int)t->value->int32;
        break;

      default:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
        break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }

  // weather conditions icon
  handle_weather_icon(weather_conditions_id);

  // handle the event time
  handle_event_time(day, month, year, hour, minutes);
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


/******************************************** INIT & DEINIT ***************************************************/


static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();
  
#ifdef PBL_SDK_2
  window_set_fullscreen(s_main_window, true);
#endif
  window_set_background_color(s_main_window, COLOR_FALLBACK(GColorBlue, GColorBlack));
  
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(handle_battery);
  // Register with BT service to check the BT conection status
  bluetooth_connection_service_subscribe(handle_bluetooth);

  // Make sure the time is displayed from the start
  update_time();
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
