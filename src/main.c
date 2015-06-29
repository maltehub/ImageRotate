#include <pebble.h>

#define KEY_TEMPERATURE   0
#define KEY_CONDITIONS    1
#define KEY_EVENT_NAME    2
#define KEY_EVENT_DAY     3
#define KEY_EVENT_MONTH   4
#define KEY_EVENT_YEAR    5
#define KEY_EVENT_HOUR    6
#define KEY_EVENT_MINUTE  7

  
static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_weather_layer;
static GBitmap *s_bitmap;
static BitmapLayer *s_bitmap_layer;
// Event vars
static GFont s_res_gothic_14;
static TextLayer *s_textlayer_time;
static TextLayer *s_textlayer_event_title;
static TextLayer *s_textlayer_event_time;

// Store incoming information
static char temperature_buffer[8];
static char conditions_buffer[32];
static char weather_layer_buffer[32];
static char event_name_buffer[64];
static char event_time_buffer[16];
static char time_to_event_buffer[16];

// struct to keep the event time once retrieved
static struct tm event_time;

static TextLayer *s_battery_layer;
static int nextappointment=0;

static void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[] = "100%";

  if (charge_state.is_charging) {
    snprintf(battery_text, sizeof(battery_text), "charging");
  } else {
    snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
  }
  text_layer_set_text(s_battery_layer, battery_text);
}

static void calculate_time_to_event(){
  // Also calculate the time to event for the top label
  time_t now = time(NULL);
  int seconds = 0;

  seconds = difftime(mktime(&event_time), now);
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
          text_layer_set_text(s_textlayer_time, time_to_event_buffer);
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
  time_t event_time_t = mktime(&event_time);

  strftime(event_time_buffer, sizeof(event_time_buffer), "%H:%M", &event_time);
  text_layer_set_text(s_textlayer_event_time, event_time_buffer);

  calculate_time_to_event();
}

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
  s_weather_layer = text_layer_create(GRect(40, 0, 100, 25));
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, GColorWhite);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentRight);
  text_layer_set_font(s_weather_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text(s_weather_layer, "Loading...");
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));

  // Create battery Layer
  s_battery_layer = text_layer_create(GRect(4, 0, bounds.size.w, 25));
  text_layer_set_text_color(s_battery_layer, GColorWhite);
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentLeft);
  text_layer_set_text(s_battery_layer, "100%");
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_battery_layer));
	
  // Load font 
  s_res_gothic_14 = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  // Time to the event
  s_textlayer_time = text_layer_create(GRect(8, 98, 125, 17));
  text_layer_set_text(s_textlayer_time, "10 min");
  layer_add_child(window_get_root_layer(window), (Layer *)s_textlayer_time);
  
  // Title of the event
  s_textlayer_event_title = text_layer_create(GRect(8, 113, 127, 20));
  text_layer_set_text(s_textlayer_event_title, "Picnic with friends");
  text_layer_set_font(s_textlayer_event_title, s_res_gothic_14);
  layer_add_child(window_get_root_layer(window), (Layer *)s_textlayer_event_title);
  
  // Time and place (or other info)
  s_textlayer_event_time = text_layer_create(GRect(8, 136, 125, 26));
  text_layer_set_text(s_textlayer_event_time, "12:30 PM");
  text_layer_set_font(s_textlayer_event_time, s_res_gothic_14);
  layer_add_child(window_get_root_layer(window), (Layer *)s_textlayer_event_time);
 
}

static void main_window_unload(Window *window) {
    // Destroy TextLayer
    text_layer_destroy(s_time_layer);
    text_layer_destroy(s_date_layer);
    bitmap_layer_destroy(s_bitmap_layer);
    gbitmap_destroy(s_bitmap);
    text_layer_destroy(s_weather_layer);
    text_layer_destroy(s_battery_layer);
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

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Read first item
  Tuple *t = dict_read_first(iterator);

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
        break;
      case KEY_CONDITIONS:
        snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", t->value->cstring);
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
  // Assemble full string and display
  snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", conditions_buffer, temperature_buffer);
  text_layer_set_text(s_weather_layer, weather_layer_buffer);


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
