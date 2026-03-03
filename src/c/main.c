#include <pebble.h>

#define SETTINGS_KEY 1
typedef struct ClaySettings {
	int health_metric;
	int health_target;
	GColor health_colour;
	GColor battery_colour;
  int vertical_layout;
} ClaySettings;
static ClaySettings settings;

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_hours_layer;
static TextLayer *s_minutes_layer;
static TextLayer *s_date_layer;
static TextLayer *s_weather_layer;
static GFont s_time_font;
static GFont s_date_font;
static Layer *s_drawing_layer;

static GSize window_size;
//static GBitmap *s_background_bitmap;
//static BitmapLayer *s_background_layer;

static GColor battery_bar_color;
static float battery_bar_width;
static bool bluetooth_status;
static char weather_text[32] = "...";
static int last_weather_update = 0;
static HealthMetric health_metric;
static bool health_data_available;
static HealthValue health_value;
static int health_target;
static float health_bar_width;
static int time_text_bottom = 0;
static int healthBarY;

static void init_health() {
	switch (settings.health_metric) {
		case 2:
			health_metric = HealthMetricActiveSeconds;
			health_target = settings.health_target * 60;
			break;
		case 3:
			health_metric = HealthMetricWalkedDistanceMeters;
			health_target = settings.health_target;
			break;
		case 4:
			health_metric = HealthMetricSleepSeconds;
			health_target = settings.health_target * 60;
			break;
		case 5:
			health_metric = HealthMetricActiveKCalories;
			break;
		default:
			health_metric = HealthMetricStepCount;
			health_target = settings.health_target;
			break;
	}

	
	time_t start = time_start_of_today();
	time_t end = time(NULL);

	HealthServiceAccessibilityMask mask = health_service_metric_accessible(health_metric, start, end);
	health_data_available = mask & HealthServiceAccessibilityMaskAvailable;
}

static void update_health() {
	if (health_data_available) {
		health_value = health_service_sum_today(health_metric);
		health_bar_width = ((float)health_value / (float)health_target);
	} else {
		health_bar_width = 0.5;
	}
	//APP_LOG(APP_LOG_LEVEL_INFO, "Health Value: %d", (int)health_value);
}

static void update_weather() {
	text_layer_set_text(s_weather_layer, ".");
	
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);

	if (!iter) {
		// Error creating outbound message
		return;
	}

	int value = 1;
	dict_write_int(iter, MESSAGE_KEY_WeatherRequest, &value, sizeof(int), true);
	dict_write_end(iter);

	app_message_outbox_send();
}

static void update_time() {
	// Get a tm structure
	time_t temp = time(NULL); 
	struct tm *tick_time = localtime(&temp);
  
  if (settings.vertical_layout) {  // Vertical Layout
    
    static char hbuffer[] = "00";
    static char mbuffer[] = "00";
    
    if(clock_is_24h_style() == true) {
  		strftime(hbuffer, sizeof("00"), "%H", tick_time);
  	} else {
  		strftime(hbuffer, sizeof("00"), "%I", tick_time);
  	}
    strftime(mbuffer, sizeof("00"), "%M", tick_time);
    
    text_layer_set_text(s_hours_layer, hbuffer);
    text_layer_set_text(s_minutes_layer, mbuffer);
    
  } else {  // Horizontal Layout
    
  	// Create a long-lived buffer
  	static char tbuffer[] = "00:00";
  
  	// Write the current hours and minutes into the buffer
  	if(clock_is_24h_style() == true) {
  		// Use 24 hour format
  		strftime(tbuffer, sizeof("00:00"), "%H:%M", tick_time);
  	} else {
  		// Use 12 hour format
  		strftime(tbuffer, sizeof("00:00"), "%I:%M", tick_time);
  	}
  
  	// Display this time & date on the TextLayer
  	text_layer_set_text(s_time_layer, tbuffer);
    
  }
  
  // Date
  static char dbuffer[] = "Mon Jan 01";
  strftime(dbuffer, sizeof("Mon Jan 01"), "%a %b %d", tick_time);
  text_layer_set_text(s_date_layer, dbuffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	update_time();
	
	int now = mktime(tick_time);
	if (last_weather_update <= (now - 900)) {
		update_weather();
		last_weather_update = mktime(tick_time);
	}

	update_health();
}

static void drawing_layer_update(Layer *this_layer, GContext *ctx) {
	// Draw battery bar
	graphics_context_set_stroke_color(ctx, battery_bar_color);
	graphics_context_set_fill_color(ctx, battery_bar_color);
	graphics_fill_rect(ctx, GRect((window_size.w / 2) - ((window_size.w * battery_bar_width) / 2), (window_size.h - 4), (window_size.w * battery_bar_width), 4), 0, GCornerNone);
	
	// Draw health bar
	if (bluetooth_status) {
		graphics_context_set_stroke_color(ctx, settings.health_colour);
		graphics_context_set_fill_color(ctx, settings.health_colour);
	} else {
		graphics_context_set_stroke_color(ctx, GColorDarkCandyAppleRed);
		graphics_context_set_fill_color(ctx, GColorDarkCandyAppleRed);
	}
  
	graphics_fill_rect(ctx, GRect(((window_size.w / 2) - ((window_size.w * health_bar_width) / 2)), healthBarY, (window_size.w * health_bar_width), 4), 0, GCornerNone);
}

static void update_battery_levels(BatteryChargeState charge_state) {
	if (charge_state.is_plugged) {
		battery_bar_color = GColorBlueMoon;
	} else if (charge_state.charge_percent >= 20) {
		battery_bar_color = settings.battery_colour;
	} else {
		battery_bar_color = GColorDarkCandyAppleRed;
	}
	
	battery_bar_width = ((float)charge_state.charge_percent / 100);
}

static void battery_handler(BatteryChargeState charge_state) {
	update_battery_levels(charge_state);
	layer_mark_dirty(s_drawing_layer);
}

static void bluetooth_handler(bool bt_status) {
	bluetooth_status = bt_status;
	layer_mark_dirty(s_drawing_layer);
}

static void save_settings() {
	persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void load_settings() {
	settings.health_metric = 1;
	settings.health_target = 10000;
	settings.health_colour = GColorVividViolet;
	settings.battery_colour = GColorCadetBlue;
  settings.vertical_layout = 0;

	persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void app_message_handler(DictionaryIterator *iter, void *context) {
	// Weather
	Tuple *temp_tuple = dict_find(iter, MESSAGE_KEY_WeatherResponse);
	
	if (temp_tuple != NULL && temp_tuple->length <= sizeof(weather_text)) {
		memcpy(weather_text, temp_tuple->value, temp_tuple->length);
		text_layer_set_text(s_weather_layer, weather_text);
	}

	// Settings
	Tuple *health_metric_t = dict_find(iter, MESSAGE_KEY_HealthMetric);
	if(health_metric_t) {
		settings.health_metric = atoi(health_metric_t->value->cstring);
	}
	Tuple *health_target_t = dict_find(iter, MESSAGE_KEY_HealthTarget);
	if(health_target_t) {
		settings.health_target = atoi(health_target_t->value->cstring);
	}
	Tuple *health_colour_t = dict_find(iter, MESSAGE_KEY_HealthBarColour);
	if(health_colour_t) {
		settings.health_colour = GColorFromHEX(health_colour_t->value->int32);
	}
	Tuple *battery_colour_t = dict_find(iter, MESSAGE_KEY_BatteryBarColour);
	if(battery_colour_t) {
		settings.battery_colour = GColorFromHEX(battery_colour_t->value->int32);
	}
  Tuple *vertical_layout_t = dict_find(iter, MESSAGE_KEY_VerticalLayout);
	if(vertical_layout_t) {
		settings.vertical_layout = vertical_layout_t->value->int32;
	}
  
  // If a setting changed
	if (health_metric_t || health_target_t || health_colour_t || battery_colour_t || vertical_layout_t) {
		init_health();
		update_health();
    update_battery_levels(battery_state_service_peek());
		layer_mark_dirty(s_drawing_layer);
    
    if (settings.vertical_layout) {
      text_layer_set_text(s_time_layer, "");
      layer_set_frame(text_layer_get_layer(s_date_layer), GRect(0, window_size.h - 35, window_size.w, 50));
      if (window_size.h > 200) {
        healthBarY = time_text_bottom - 7;
      } else {
        healthBarY = time_text_bottom - 5;
      }
    } else {
      text_layer_set_text(s_hours_layer, "");
      text_layer_set_text(s_minutes_layer, "");
      layer_set_frame(text_layer_get_layer(s_date_layer), GRect(0, time_text_bottom + 11, window_size.w, 50));
      healthBarY = time_text_bottom + 7;
    }
    
    update_time();
		save_settings();
	}
}

static void main_window_load(Window *window) {
	Layer *root_layer = window_get_root_layer(window);
	window_size = layer_get_bounds(root_layer).size;


	window_set_background_color(window, GColorBlack);
	/*s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_DIAMOND_BACKGROUND_GREEN);
	s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
	bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
	layer_add_child(root_layer, bitmap_layer_get_layer(s_background_layer));*/
	
	int time_text_top = (window_size.h * 0.26);
	if (window_size.h > 200) {
		s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SQUARES_BOLD_60));
		time_text_bottom = time_text_top + 60;
		s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SQUARES_BOLD_20));
	} else {
		s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SQUARES_BOLD_40));
		time_text_bottom = time_text_top + 40;
		s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SQUARES_BOLD_16));
	}
  
  healthBarY = time_text_bottom + 7;
  if (settings.vertical_layout) {
    if (window_size.h > 200) {
      healthBarY = time_text_bottom - 7;
    } else {
      healthBarY = time_text_bottom - 5;
    }
  }
	
	// Create time TextLayer
	s_time_layer = text_layer_create(GRect(0, (window_size.h * 0.26), window_size.w, 60));
	text_layer_set_background_color(s_time_layer, GColorClear);
	text_layer_set_text_color(s_time_layer, GColorWhite);
	text_layer_set_font(s_time_layer, s_time_font);
	text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
	layer_add_child(root_layer, text_layer_get_layer(s_time_layer));
  
  // Create hours TextLayer (for vertical layout)
  int hoursY = (window_size.h * 0.5) - 55;
  if (window_size.h > 200) {hoursY = (window_size.h * 0.5) - 75;}
  s_hours_layer = text_layer_create(GRect(0, hoursY, window_size.w, 60));
	text_layer_set_background_color(s_hours_layer, GColorClear);
	text_layer_set_text_color(s_hours_layer, GColorWhite);
	text_layer_set_font(s_hours_layer, s_time_font);
	text_layer_set_text_alignment(s_hours_layer, GTextAlignmentCenter);
	layer_add_child(root_layer, text_layer_get_layer(s_hours_layer));
  
  // Create minutes TextLayer (for vertical layout)
  s_minutes_layer = text_layer_create(GRect(0, (window_size.h * 0.5) - 5, window_size.w, 60));
	text_layer_set_background_color(s_minutes_layer, GColorClear);
	text_layer_set_text_color(s_minutes_layer, GColorWhite);
	text_layer_set_font(s_minutes_layer, s_time_font);
	text_layer_set_text_alignment(s_minutes_layer, GTextAlignmentCenter);
	layer_add_child(root_layer, text_layer_get_layer(s_minutes_layer));
	
	// Create date TextLayer
  int dateY = time_text_bottom + 11;
  if (settings.vertical_layout) {
    if (window_size.h > 200) {
      dateY = window_size.h - 35;
    } else {
      dateY = window_size.h - 30;
    }
  }
	s_date_layer = text_layer_create(GRect(0, dateY, window_size.w, 50));
	text_layer_set_background_color(s_date_layer, GColorClear);
	text_layer_set_text_color(s_date_layer, GColorWhite);
	text_layer_set_font(s_date_layer, s_date_font);
	text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
	layer_add_child(root_layer, text_layer_get_layer(s_date_layer));
	
	// Create weather TextLayer
	s_weather_layer = text_layer_create(GRect(0, 0, window_size.w, 50));
	text_layer_set_background_color(s_weather_layer, GColorClear);
	text_layer_set_text_color(s_weather_layer, GColorWhite);
	text_layer_set_font(s_weather_layer, s_date_font);
	text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
	layer_add_child(root_layer, text_layer_get_layer(s_weather_layer));
	
	// Grab battery levels before trying to draw them
	update_battery_levels(battery_state_service_peek());
	
	// Grab bluetooth status before trying to draw it
	bluetooth_status = bluetooth_connection_service_peek();

	// Initialise health
	init_health();
	
	// Create layer for Drawing
	s_drawing_layer = layer_create(GRect(0, 0, window_size.w, window_size.h));
	layer_set_update_proc(s_drawing_layer, drawing_layer_update);
	layer_add_child(root_layer, s_drawing_layer);
	
	// Make sure the time is displayed from the start
	update_time();
	update_weather();
	update_health();
}

static void main_window_unload(Window *window) {
	// Destroy TextLayer
	text_layer_destroy(s_time_layer);
	text_layer_destroy(s_date_layer);
	text_layer_destroy(s_weather_layer);
	fonts_unload_custom_font(s_time_font);
	fonts_unload_custom_font(s_date_font);
	//gbitmap_destroy(s_background_bitmap);
	//bitmap_layer_destroy(s_background_layer);
}

static void init() {
	// Load settings
	load_settings();

	// Create main Window element and assign to pointer
	s_main_window = window_create();

	// Set handlers to manage the elements inside the Window
	window_set_window_handlers(s_main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload
	});

	// Show the Window on the watch, with animated=true
	window_stack_push(s_main_window, true);
	
	// Register with TickTimerService
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
	
	// Register battery handler
	battery_state_service_subscribe(battery_handler);
	
	// Register bluetooth handler
	bluetooth_connection_service_subscribe(bluetooth_handler);
	
	app_message_register_inbox_received(app_message_handler);
	app_message_open(64, 64);
}

static void deinit() {
	// Destroy Window
  	window_destroy(s_main_window);
	tick_timer_service_unsubscribe();
	battery_state_service_unsubscribe();
	bluetooth_connection_service_unsubscribe();
	app_message_deregister_callbacks();
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}