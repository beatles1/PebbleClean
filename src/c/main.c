#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_weather_layer;
static GFont s_time_font;
static GFont s_date_font;
static Layer *s_drawing_layer;

static GBitmap *s_background_bitmap;
static BitmapLayer *s_background_layer;

static GColor battery_bar_color;
static int battery_bar_width;
static bool bluetooth_status;
static char weather_text[64] = "...";
static int last_weather_update = 0;

static void update_weather() {
	text_layer_set_text(s_weather_layer, ".");
	
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);

	if (!iter) {
		// Error creating outbound message
		return;
	}

	int value = 1;
	dict_write_int(iter, 1, &value, sizeof(int), true);
	dict_write_end(iter);

	app_message_outbox_send();
}

static void update_time() {
	// Get a tm structure
	time_t temp = time(NULL); 
	struct tm *tick_time = localtime(&temp);

	// Create a long-lived buffer
	static char tbuffer[] = "00:00";
	static char dbuffer[] = "Mon Jan 01";

	// Write the current hours and minutes into the buffer
	if(clock_is_24h_style() == true) {
		// Use 24 hour format
		strftime(tbuffer, sizeof("00:00"), "%H:%M", tick_time);
	} else {
		// Use 12 hour format
		strftime(tbuffer, sizeof("00:00"), "%I:%M", tick_time);
	}
	
	// Write the current date into the buffer
	strftime(dbuffer, sizeof("Mon Jan 01"), "%a %b %d", tick_time);

	// Display this time & date on the TextLayer
	text_layer_set_text(s_time_layer, tbuffer);
	text_layer_set_text(s_date_layer, dbuffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	update_time();
	
	int now = mktime(tick_time);
	if (last_weather_update <= (now - 900)) {
		update_weather();
		last_weather_update = mktime(tick_time);
	}
}

static void drawing_layer_update(Layer *this_layer, GContext *ctx) {
	// Draw battery bar
	graphics_context_set_stroke_color(ctx, battery_bar_color);
	graphics_context_set_fill_color(ctx, battery_bar_color);
	graphics_fill_rect(ctx, GRect((144 / 2) - (battery_bar_width / 2), 91, battery_bar_width, 3), 0, GCornerNone);
	
	// Draw bluetooth status bar
	if (bluetooth_status) {
		graphics_context_set_stroke_color(ctx, GColorCadetBlue);
		graphics_context_set_fill_color(ctx, GColorCadetBlue);
	} else {
		graphics_context_set_stroke_color(ctx, GColorDarkCandyAppleRed);
		graphics_context_set_fill_color(ctx, GColorDarkCandyAppleRed);
	}
	graphics_fill_rect(ctx, GRect(0, 165, 144, 3), 0, GCornerNone);
}

static void update_battery_levels(BatteryChargeState charge_state) {
	if (charge_state.is_plugged) {
		battery_bar_color = GColorBlueMoon;
	} else if (charge_state.charge_percent >= 20) {
		battery_bar_color = GColorCadetBlue;
	} else {
		battery_bar_color = GColorDarkCandyAppleRed;
	}
	
	battery_bar_width = charge_state.charge_percent * 1.44;
}

static void battery_handler(BatteryChargeState charge_state) {
	update_battery_levels(charge_state);
	layer_mark_dirty(s_drawing_layer);
}

static void bluetooth_handler(bool bt_status) {
	bluetooth_status = bt_status;
	layer_mark_dirty(s_drawing_layer);
}

static void app_message_handler(DictionaryIterator *iter, void *context) {
	Tuple *temp_tuple = dict_find(iter, 2);
	
	if (temp_tuple != NULL && temp_tuple->length <= sizeof(weather_text)) {
		memcpy(weather_text, temp_tuple->value, temp_tuple->length);
		text_layer_set_text(s_weather_layer, weather_text);
	}
}

static void main_window_load(Window *window) {
	window_set_background_color(window, GColorBlack);
	/*s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_DIAMOND_BACKGROUND_GREEN);
	s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
	bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));*/
	
	
	s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SQUARES_BOLD_40));
	s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SQUARES_BOLD_16));
	
	// Create time TextLayer
	s_time_layer = text_layer_create(GRect(0, 45, 144, 50));
	text_layer_set_background_color(s_time_layer, GColorClear);
	text_layer_set_text_color(s_time_layer, GColorWhite);
	text_layer_set_font(s_time_layer, s_time_font);
	text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
	// Add it as a child layer to the Window's root layer
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
	
	// Create date TextLayer
	s_date_layer = text_layer_create(GRect(0, 94, 144, 50));
	text_layer_set_background_color(s_date_layer, GColorClear);
	text_layer_set_text_color(s_date_layer, GColorWhite);
	text_layer_set_font(s_date_layer, s_date_font);
	text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
	// Add it as a child layer to the Window's root layer
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
	
	// Create weather TextLayer
	s_weather_layer = text_layer_create(GRect(0, 0, 144, 50));
	text_layer_set_background_color(s_weather_layer, GColorClear);
	text_layer_set_text_color(s_weather_layer, GColorWhite);
	text_layer_set_font(s_weather_layer, s_date_font);
	text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
	// Add it as a child layer to the Window's root layer
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));
	
	// Grab battery levels before trying to draw them
	update_battery_levels(battery_state_service_peek());
	
	// Grab bluetooth status before trying to draw it
	bluetooth_status = bluetooth_connection_service_peek();
	
	// Create layer for Drawing
	s_drawing_layer = layer_create(GRect(0, 0, 144, 168));
	layer_set_update_proc(s_drawing_layer, drawing_layer_update);
	layer_add_child(window_get_root_layer(window), s_drawing_layer);
	
	// Make sure the time is displayed from the start
	update_time();
	update_weather();
}

static void main_window_unload(Window *window) {
	// Destroy TextLayer
	text_layer_destroy(s_time_layer);
	text_layer_destroy(s_date_layer);
	text_layer_destroy(s_weather_layer);
	fonts_unload_custom_font(s_time_font);
	fonts_unload_custom_font(s_date_font);
	gbitmap_destroy(s_background_bitmap);
	bitmap_layer_destroy(s_background_layer);
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