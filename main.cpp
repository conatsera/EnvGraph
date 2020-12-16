#include <unistd.h>

#include "engine.h"

int main()
{
    xcb_connection_t* xConnection = xcb_connect(NULL, NULL);

    const xcb_setup_t* xSetup = xcb_get_setup(xConnection);
    xcb_screen_iterator_t xScreenIter = xcb_setup_roots_iterator(xSetup);
    xcb_screen_t* mainScreen = xScreenIter.data;

    xcb_window_t window = xcb_generate_id(xConnection);
    xcb_create_window(xConnection,
    XCB_COPY_FROM_PARENT,
    window,
    mainScreen->root,
    0, 0,
    1536, 1536,
    10,
    XCB_WINDOW_CLASS_INPUT_OUTPUT,
    mainScreen->root_visual,
    0, NULL);

    xcb_map_window(xConnection, window);

    xcb_flush(xConnection);

    __attribute__((unused)) Engine::Engine* aEngine = new Engine::Engine(xConnection, window);

    pause();

    //delete aEngine;

    xcb_disconnect(xConnection);

    return 0;
}