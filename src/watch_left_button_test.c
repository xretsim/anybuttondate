#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"


#define MY_UUID { 0x70, 0x8E, 0x85, 0x2E, 0x1A, 0x69, 0x48, 0xAC, 0x92, 0xA7, 0x5D, 0x6B, 0x54, 0xE9, 0xF5, 0xE8 }
PBL_APP_INFO(MY_UUID,
             "Any Button Date", "Glenn Loos-Austin",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_STANDARD_APP);

//doesn't work as APP_INFO_WATCH_FACE, even with window stacking, all buttons are ignored, alas.

Window _datewindow;
Window _timeWindow;
TextLayer _date;
TextLayer _time;
bool _timeShowing = false;

AppTimerHandle _timer_handle;
AppContextRef _storedctx;



//////////////////////////////////
// Pebble API Utility Functions //
//////////////////////////////////

void setupTextLayerCore ( TextLayer *layer, int x, int y, int width, int height, GFont font, GColor colorChoice, GColor bgChoice, GTextAlignment whatAlign)
{
    text_layer_init(layer, GRect(x, y, width, height)); //_window.layer.frame);
    text_layer_set_text_color(layer, colorChoice);
    text_layer_set_background_color(layer, bgChoice);
    text_layer_set_text_alignment(layer, whatAlign);
    layer_set_frame(&layer->layer, GRect(x, y, width, height));
    text_layer_set_font(layer, font );
}
void setupTextLayer( TextLayer *layer, Window *parent, int x, int y, int width, int height, GFont font, GColor colorChoice, GColor bgChoice, GTextAlignment whatAlign )
{
    setupTextLayerCore (layer,x,y,width,height,font,colorChoice,bgChoice,whatAlign);
    layer_add_child(&parent->layer, &layer->layer);
}

/*
void setupTextLayer_alt( TextLayer *layer, Layer *parent, int x, int y, int width, int height, GFont font, GColor colorChoice, GColor bgChoice, GTextAlignment whatAlign )
{
    setupTextLayerCore (layer,x,y,width,height,font,colorChoice,bgChoice,whatAlign);
    layer_add_child(parent, &layer->layer);
}
*/

//
// End Pebble API Utility Functions.
//



//////////////////////////////////
// My Time Window               //
//////////////////////////////////

void update_time() {

    PblTm tickTime;
    get_time(&tickTime);
    
    
    //
    // set the time
    //
    static char timeText[] = "00:00";
    const char *timeFormat = clock_is_24h_style() ? "%R" : "%l:%M";
    string_format_time(timeText, sizeof(timeText), timeFormat, &tickTime);
    
    text_layer_set_text(&_time, timeText);

}

void time_handle_load(Window *window) {
    _timeShowing = true;
    
    GFont timefont = fonts_get_system_font(FONT_KEY_GOTHAM_42_BOLD);
    setupTextLayer( &_time, &_timeWindow, 0, 50, 144, 50, timefont, GColorBlack, GColorWhite, GTextAlignmentCenter);

    update_time();
}

void time_handle_unload(Window *window) {
    _timeShowing = false;
    _timer_handle = app_timer_send_event(_storedctx, 1500, 1);
        //show the time again in 1.5 seconds.
}

void any_click_handler(ClickRecognizerRef recognizer, Window *window) {
    window_stack_pop(true);
}

void dummy_config_provider(ClickConfig **config, Window *window) {
    config[BUTTON_ID_UP]->click.handler = (ClickHandler) any_click_handler;
    config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) any_click_handler;
    config[BUTTON_ID_SELECT]->click.handler = (ClickHandler) any_click_handler;
    
    (void)window;
}

void start_time_window() {
    window_init(&_timeWindow, "The Time");
    window_set_fullscreen(&_timeWindow, true);
    window_set_click_config_provider(&_timeWindow, (ClickConfigProvider) dummy_config_provider);

    window_set_window_handlers(&_timeWindow, (WindowHandlers){
        .load = (WindowHandler)time_handle_load,
        .unload = (WindowHandler)time_handle_unload
    });
    
    window_stack_push(&_timeWindow, true); //false = no animation;
}

//
// My Time Window section end
//



//////////////////////////////////
// My Date Window               //
//////////////////////////////////

void update_date() {
    PblTm tickTime;
    get_time(&tickTime);
    
    //
    // set the date - only changing it when the day changes
    // format strings here: http://www.gnu.org/software/emacs/manual/html_node/elisp/Time-Parsing.html
    //
    static char dateText[] = "Sunday 01 September 00";
    static int lastShownDate = -1;
    int theDay = tickTime.tm_yday;
    if (theDay != lastShownDate)
    {
        lastShownDate = theDay;
        string_format_time(dateText, sizeof(dateText), "%A\n%B %e", &tickTime );
        text_layer_set_text(&_date, dateText);
    }
    
}

//
// My Date Window section End
//




//////////////////////////////////
// Core Watchface Tools         //
//////////////////////////////////


void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t) {
    update_date();
    if(_timeShowing) update_time();
}

void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t countdown) {
    (void)ctx;
    start_time_window();
}

void handle_deinit(AppContextRef ctx) {
    (void)ctx;
}

void handle_init(AppContextRef ctx) {
    (void)ctx;
    
    _storedctx = ctx;

    window_init(&_datewindow, "The Date");
    window_stack_push(&_datewindow, true /* Animated */);
    window_set_fullscreen(&_datewindow, true);

    GFont datefont = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    setupTextLayer( &_date, &_datewindow, 0, 50, 144, 50, datefont, GColorBlack, GColorWhite, GTextAlignmentCenter);
    update_date();
    
    _timer_handle = app_timer_send_event(_storedctx, 1500, 1);
}


void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
      .deinit_handler = &handle_deinit,
      .timer_handler = &handle_timer,
      .tick_info = {
          .tick_handler = &handle_minute_tick,
          .tick_units = MINUTE_UNIT
      }

  };
  app_event_loop(params, &handlers);
}
