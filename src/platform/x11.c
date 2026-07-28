#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "d_event.h"
#include "doomkeys.h"
#include "i_system.h"
#include "platform/pixel_format.h"
#include "platform/platform.h"

static Display *display;
static Window window;
static Atom wm_delete_window;
static GC gc;
static uint32_t* screen_buffer;
static XImage* image = NULL;
Cursor empty_cursor;

static void I_Platform_SetupMouse(void)
{
    char data[1];
    XColor color;
    Pixmap pixmap;

    data[0] = 0;
    color.red = color.green = color.blue = 0;
    pixmap = XCreateBitmapFromData(display, DefaultRootWindow(display), data, 1, 1);
    if (pixmap)
    {
        empty_cursor = XCreatePixmapCursor(display, pixmap, pixmap, &color, &color, 0, 0);
        XFreePixmap(display, pixmap);
    }
    XDefineCursor(display, window, empty_cursor);
}

static void I_Platform_WaitForMapNotify(void)
{
    XEvent e;

    while(1)
    {
        XNextEvent(display, &e);
        if (e.type == MapNotify)
        {
            break;
        }
    }
}

void I_Platform_InitGraphics(screen_t* s)
{
    int                 screen;
    Window              root;
    Visual             *visual;
    XWindowAttributes   attr;

    display = XOpenDisplay(NULL);

    if (display == None)
    {
        I_Error("Failed to initialize display\n");
    }

    root = DefaultRootWindow(display);

    if (root == None)
    {
        I_Error("No root window found\n");
    }

    window = XCreateSimpleWindow(display, root, 0, 0, 1024, 768, 0, 0, 0xffffffff);

    if (window == None)
    {
        I_Error("Failed to create window\n");
    }

    XSelectInput(display, window, StructureNotifyMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask);

    XMapWindow(display, window);

    wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wm_delete_window, 1);

    gc = XCreateGC(display, window, 0, NULL);

    I_Platform_WaitForMapNotify();

    screen = DefaultScreen(display);
    visual = DefaultVisual(display, screen);

    XGetWindowAttributes(display, window, &attr);

    I_Platform_SetupMouse();

    screen_buffer = calloc(attr.width * attr.height, sizeof(uint32_t));

    image = XCreateImage(display, visual, attr.depth, ZPixmap, 0, (char*)screen_buffer, attr.width, attr.height, 32, 0);

    s->resx   = attr.width;
    s->resy   = attr.height;
    s->pixels = screen_buffer;

    SET_PIXEL_FORMAT_G8B8R8A8(s);
}

void I_Platform_ShutdownGraphics(screen_t* s)
{
    UNUSED(s);
    if (empty_cursor)
    {
        XUndefineCursor(display, window);
        XFreeCursor(display, empty_cursor);
    }
    if (window)
    {
        XDestroyWindow(display, window);
    }
    if (display)
    {
        XCloseDisplay(display);
    }
    if (screen_buffer)
    {
        free(screen_buffer);
    }
}

void I_Platform_SetWindowTitle(char* title)
{
    if (window)
    {
        XChangeProperty(display, window, XA_WM_NAME, XA_STRING, 8, PropModeReplace, (const uint8_t*)title, strlen(title));
    }
    else
    {
        I_Printf("Cannot set title, window not initialized\n");
    }
}

void I_Platform_RenderFrame(void)
{
    XPutImage(display, window, gc, image, 0, 0, 0, 0, image->width, image->height);
}

void I_Platform_InitInput(void)
{
}

void I_Platform_ShutdownInput(void)
{
}

static uint8_t I_Platform_ConvertToDoomKey(unsigned int key)
{
    switch (key)
    {
        case XK_Control_L:
        case XK_Control_R:  return KEY_FIRE;
        case XK_Shift_L:
        case XK_Shift_R:    return KEY_RSHIFT;
        case XK_Alt_L:      return KEY_LALT;
        case XK_Alt_R:      return KEY_RALT;
        case XK_Escape:     return KEY_ESCAPE;
        case XK_space:      return KEY_USE;
        case XK_Return:     return KEY_ENTER;
        case XK_BackSpace:  return KEY_BACKSPACE;
        case XK_Up:         return KEY_UPARROW;
        case XK_Down:       return KEY_DOWNARROW;
        case XK_Left:       return KEY_LEFTARROW;
        case XK_Right:      return KEY_RIGHTARROW;

#define FN_KEY(k) case XK_F##k: return KEY_F##k
        FN_KEY(1);
        FN_KEY(2);
        FN_KEY(3);
        FN_KEY(4);
        FN_KEY(5);
        FN_KEY(6);
        FN_KEY(7);
        FN_KEY(8);
        FN_KEY(9);
        FN_KEY(10);
        FN_KEY(11);
        FN_KEY(12);

        default:
            return tolower(key);
    }
}

void I_Platform_ReadEvents(void)
{
    XEvent e;
    KeySym sym;
    event_t event;

    if (!display)
    {
        return;
    }

    while (XPending(display) > 0)
    {
        XNextEvent(display, &e);

        switch (e.type)
        {
            case ClientMessage:
                if (e.xclient.data.l[0] == (long)wm_delete_window)
                {
                    event.type = ev_quit;
                    event.data1 = 0;
                    event.data2 = 0;
                    D_PostEvent(&event);
                }
                break;

            case KeyPress:
            case KeyRelease:
                sym = XkbKeycodeToKeysym(display, e.xkey.keycode, 0, 0);
                event.type = e.type == KeyPress ? ev_keydown : ev_keyup;
                event.data1 = I_Platform_ConvertToDoomKey(sym);
                event.data2 = isascii(sym) ? tolower(sym) : 0;
                D_PostEvent(&event);
                break;

            case ButtonPress:
                event.type = ev_mouse;
                event.data1 = e.xbutton.button;
                event.data2 = 0;
                event.data3 = 0;
                D_PostEvent(&event);
                break;

            case ButtonRelease:
                event.type = ev_mouse;
                event.data1 = 0;
                event.data2 = 0;
                event.data3 = 0;
                D_PostEvent(&event);
                break;
        }
    }
}
