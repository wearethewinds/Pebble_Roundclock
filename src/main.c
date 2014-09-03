#include <pebble.h>
  
const int hours_stroke_width = 22;
const int minutes_stroke_width = 14;

static Window *s_main_window;

static Layer *s_hours;
static Layer *s_minutes;

static GPath *hour_hider;
static GPath *minute_hider;

static Layer *s_hours_structure;
static Layer *s_minutes_structure;

static struct tm *now;

static void draw_hider(GContext *ctx, GPath *hider, GPoint center, int radius, int completion) {
  int angle = TRIG_MAX_ANGLE * abs(completion) / 100;
  int dx = 0;
  int new_x = sin_lookup(angle) * radius / TRIG_MAX_RATIO;
  int dy = 0;
  int new_y = -cos_lookup(angle) * radius / TRIG_MAX_RATIO;
  dy = new_y + center.y;
  struct GPathInfo gp_info;
  if (completion < 0) {
    completion = abs(completion);
    dx = center.x - new_x;
    if (completion <= 50) {
      gp_info.num_points = 6;
      gp_info.points = (GPoint []) {
        {center.x, 0},
        {center.x, center.y},
        {dx, dy},
        {-1, dy},
        {-1, center.y - radius - 1},
        {-1, center.y - radius - 1}
      };
    } else {
      gp_info.num_points = 7;
      gp_info.points = (GPoint []) {
        {center.x, 0},
        {center.x, center.y},
        {dx, dy},
        {center.x + radius + 1, dy},
        {center.x + radius + 1, center.y + radius + 1},
        {-1, center.y + radius + 1},
        {-1, center.y - radius - 1}
      };
    }
  }
  else {
    dx = new_x + center.x;
    if (completion <= 50) {
      gp_info.num_points = 5;
      gp_info.points = (GPoint []) {
        {center.x, 0},
        {center.x, center.y},
        {dx, dy},
        {center.x + radius + 1, dy},
        {center.x + radius + 1, center.y - radius -1}
      };
    }
    else {
      gp_info.num_points = 7;
      gp_info.points = (GPoint []) {
        {center.x, 0},
        {center.x, center.y},
        {dx, dy},
        {-1, dy},
        {-1, 2 * center.y + 1},
        {center.x + radius + 1, center.y + radius + 1},
        {center.x + radius + 1, center.y - radius - 1}
      };
    }
  }
  hider = gpath_create(&gp_info);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  gpath_draw_filled(ctx, hider);  
}

static void draw_circle(GContext *ctx, GPath *hider, GPoint center, int radius, int stroke_width, int completion) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, radius);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, radius - stroke_width);
  draw_hider(ctx, hider, center, radius, completion);
}

static void draw_structure_circles(GContext *ctx, GPoint center, int radius, int stroke_width) {
  graphics_context_set_stroke_color(ctx, GColorWhite);
  for (int i = 0; i < stroke_width; i++) {
    graphics_draw_circle(ctx, center, radius - i);
  }
}

static void draw_layer(Layer *layer, GContext *ctx, GPath *hider, int stroke_width, int completion) {
  GRect bounds = layer_get_bounds(layer);
  const int half_h = bounds.size.h / 2;
  const int half_w = bounds.size.w / 2;
  GPoint center = GPoint(half_w, half_h);
  draw_circle(ctx, hider, center, half_w - 1, stroke_width, completion);
}

static void draw_hours_structure_layer(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  const int half_h = bounds.size.h / 2;
  const int half_w = bounds.size.w / 2;
  GPoint center = GPoint(half_w, half_h);
  draw_structure_circles(ctx, center, half_w - 1, 3);
  draw_structure_circles(ctx, center, half_w - 1 - hours_stroke_width, 3);
}

static void draw_minutes_structure_layer(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  const int half_h = bounds.size.h / 2;
  const int half_w = bounds.size.w / 2;
  GPoint center = GPoint(half_w, half_h);
  draw_structure_circles(ctx, center, half_w -1, 3);
  draw_structure_circles(ctx, center, half_w - 1 - minutes_stroke_width, 3);
}

static void draw_minutes_layer(Layer *layer, GContext *ctx) {
  int completion = (now->tm_min * 100) / 60;
  if (now->tm_hour % 2 == 1) { completion = -100 + completion; }
  draw_layer(layer, ctx, minute_hider, minutes_stroke_width, completion);
}

static void draw_hours_layer(Layer *layer, GContext *ctx) {
  int completion = ((((now->tm_hour % 12) * 60) + now->tm_min) * 100) / 720;
  if (now->tm_hour < 12) { completion = -100 + completion; }
  draw_layer(layer, ctx, hour_hider, hours_stroke_width, completion);
}

static GRect get_bound_of_minute_circle(GRect window_bound) {
  return GRect(window_bound.size.w * 0.26, window_bound.size.h * 0.26, window_bound.size.w * 0.48 - 1, window_bound.size.h * 0.48 - 1);
}

static void initializeTime() {
  time_t rawtime;
  time(&rawtime);
  now = gmtime(&rawtime);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bound = layer_get_bounds(window_layer);
  initializeTime();
  s_hours = layer_create(window_bound);
  s_minutes = layer_create(get_bound_of_minute_circle(window_bound));
  s_hours_structure = layer_create(window_bound);
  s_minutes_structure = layer_create(get_bound_of_minute_circle(window_bound));                                  
  layer_set_update_proc(s_hours, draw_hours_layer);
  layer_set_update_proc(s_minutes, draw_minutes_layer);
  layer_set_update_proc(s_hours_structure, draw_hours_structure_layer);
  layer_set_update_proc(s_minutes_structure, draw_minutes_structure_layer);
  layer_add_child(window_get_root_layer(window), s_hours);
  layer_add_child(window_get_root_layer(window), s_minutes);
  layer_add_child(window_get_root_layer(window), s_hours_structure);
  layer_add_child(window_get_root_layer(window), s_minutes_structure);
  window_set_background_color(s_main_window, GColorBlack);
}

static void main_window_unload(Window *window) {
  
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  now = tick_time;
  layer_mark_dirty(s_hours);
  layer_mark_dirty(s_minutes);
}

void init(void) {
  s_main_window = window_create();

  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  window_stack_push(s_main_window, true);
}

void deinit(void) {
  layer_destroy(s_minutes);
  layer_destroy(s_hours);
  layer_destroy(s_hours_structure);
  layer_destroy(s_minutes_structure);
  window_destroy(s_main_window);
  
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
