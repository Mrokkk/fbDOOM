#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

#include "d_event.h"
#include "doomkeys.h"
#include "i_system.h"
#include "i_video.h"
#include "m_argv.h"
#include "platform/platform.h"

static uint8_t bytes_per_pixel;
static Display *display;
static Window window;
static Atom WM_DELETE_WINDOW;
static Atom _NET_WM_STATE;
static Atom _NET_WM_STATE_MAXIMIZED_VERT;
static Atom _NET_WM_STATE_MAXIMIZED_HORZ;
static Atom _NET_WM_STATE_FULLSCREEN;
static XVisualInfo *vinfo;
static GC gc;
static uint32_t* screen_buffer;
static XImage* image = NULL;
static Cursor empty_cursor;
static boolean use_shm = false;
static XShmSegmentInfo shminfo;
static boolean focused = false;

static void X11_CreateAtoms(void)
{
#define CREATE_ATOM(a) a = XInternAtom(display, #a, False)
    CREATE_ATOM(WM_DELETE_WINDOW);
    CREATE_ATOM(_NET_WM_STATE);
    CREATE_ATOM(_NET_WM_STATE_MAXIMIZED_VERT);
    CREATE_ATOM(_NET_WM_STATE_MAXIMIZED_HORZ);
    CREATE_ATOM(_NET_WM_STATE_FULLSCREEN);
}

static uint8_t X11_GetBitsPerPixel(void)
{
    int                  i, n;
    uint16_t             bpp;
    XPixmapFormatValues* f;

    bpp = vinfo->depth;

    if (bpp == 24)
    {
        f = XListPixmapFormats(display, &n);

        if (!f)
        {
            I_Error("Failed to get pixmap formats\n");
        }

        for (i = 0; i < n; ++i)
        {
            if (f[i].depth == 24)
            {
                bpp = f[i].bits_per_pixel;
                break;
            }
        }
        XFree(f);
    }

    return bpp;
}

static void X11_CalculateLenAndOffset(unsigned long xmask, uint8_t* out_len, uint8_t* out_offset)
{
    uint8_t offset, len;
    for (offset = 0; (xmask & 1) == 0; xmask >>= 1, ++offset);
    for (len = 0; xmask; xmask >>= 1, ++len);
    *out_len = len;
    *out_offset = offset;
}

static void X11_SetPixelFormat(screen_t* s)
{
    uint8_t off, len;

    X11_CalculateLenAndOffset(vinfo->red_mask, &len, &off);
    s->red.offset   = off;
    s->red.len      = len;

    X11_CalculateLenAndOffset(vinfo->green_mask, &len, &off);
    s->green.offset = off;
    s->green.len    = len;

    X11_CalculateLenAndOffset(vinfo->blue_mask, &len, &off);
    s->blue.offset  = off;
    s->blue.len     = len;
}

static void X11_HideCursor(void)
{
    char    data = 0;
    XColor  color = {};
    Pixmap  pixmap;

    pixmap = XCreateBitmapFromData(display, DefaultRootWindow(display), &data, 1, 1);

    if (pixmap)
    {
        empty_cursor = XCreatePixmapCursor(display, pixmap, pixmap, &color, &color, 0, 0);
        XFreePixmap(display, pixmap);
    }

    XDefineCursor(display, window, empty_cursor);
}

static void X11_WaitForExposure(void)
{
    XEvent e;
    do
    {
        XNextEvent(display, &e);
    }
    while (e.type != Expose);
}

static void X11_SetupXShm(XWindowAttributes* attr)
{
    image = XShmCreateImage(
        /* dpy      = */ display,
        /* visual   = */ vinfo->visual,
        /* depth    = */ attr->depth,
        /* format   = */ ZPixmap,
        /* data     = */ NULL,
        /* shminfo  = */ &shminfo,
        /* width    = */ attr->width,
        /* height   = */ attr->height);

    I_ErrorWhen(image == None, "Failed to create XImage\n");

    shminfo.shmid = shmget(
        IPC_PRIVATE,
        image->bytes_per_line * image->height,
        IPC_CREAT | 0777);

    I_ErrorWhen(shminfo.shmid == -1, "Failed to get shared memory\n");

    shminfo.shmaddr = image->data = shmat(shminfo.shmid, 0, 0);
    shminfo.readOnly = False;

    I_ErrorWhen(!shminfo.shmaddr, "Failed to get shared memory\n");
    I_ErrorWhen(!XShmAttach(display, &shminfo), "Failed to attach shared memory to X\n");

    screen_buffer = (uint32_t*)shminfo.shmaddr;

    memset(image->data, 0, image->bytes_per_line * image->height);

    I_Printf("Using X shared memory\n");
}

static void X11_SetupStandard(XWindowAttributes* attr)
{
    screen_buffer = calloc(attr->width * attr->height, bytes_per_pixel);

    I_ErrorWhen(!screen_buffer, "Failed to allocate screen buffer\n");

    image = XCreateImage(
        /* display        = */ display,
        /* visual         = */ vinfo->visual,
        /* depth          = */ attr->depth,
        /* format         = */ ZPixmap,
        /* offset         = */ 0,
        /* data           = */ (char*)screen_buffer,
        /* width          = */ attr->width,
        /* height         = */ attr->height,
        /* bitmap_pad     = */ bytes_per_pixel * 8,
        /* bytes_per_line = */ bytes_per_pixel * attr->width);

    I_ErrorWhen(image == None, "Failed to create XImage\n");
}

void I_Platform_InitGraphics(screen_t* s)
{
    uint8_t              bits_per_pixel;
    int                  screen, tmp, template_mask, num_visuals;
    Window               root;
    XWindowAttributes    attr;
    XVisualInfo          template = {};
    XSetWindowAttributes set_attr = {};

    I_ErrorWhen((display = XOpenDisplay(NULL)) == None, "Failed to initialize display\n");
    I_ErrorWhen((root    = DefaultRootWindow(display)) == None, "No root window found\n");

    screen = DefaultScreen(display);

    template.visualid = XVisualIDFromVisual(XDefaultVisual(display, screen));
    template_mask = VisualIDMask;

    vinfo = XGetVisualInfo(display, template_mask, &template, &num_visuals);

    I_ErrorWhen(vinfo == None, "Failed to read visual info\n");
    I_ErrorWhen(num_visuals != 1, "Incorrect number of matching VisualInfos: %d\n", num_visuals);

    bits_per_pixel = X11_GetBitsPerPixel();
    bytes_per_pixel = (bits_per_pixel + 7) / 8;

    set_attr.event_mask = ExposureMask;

    window = XCreateWindow(
        /* display      = */ display,
        /* parent       = */ root,
        /* x            = */ 0,
        /* y            = */ 0,
        /* width        = */ SCREENWIDTH * 3,
        /* height       = */ SCREENHEIGHT * 3,
        /* border_width = */ 0,
        /* depth        = */ vinfo->depth,
        /* class        = */ InputOutput,
        /* visual       = */ vinfo->visual,
        /* valuemask    = */ CWEventMask,
        /* attributes   = */ &set_attr);

    I_ErrorWhen(window == None, "Failed to create window\n");

    X11_CreateAtoms();

    if (M_CheckParm("-fullscreen"))
    {
        Atom atoms[] = {
            _NET_WM_STATE_FULLSCREEN,
        };
        XChangeProperty(display, window, _NET_WM_STATE, XA_ATOM, 32, PropModeReplace, (unsigned char*)atoms, arrlen(atoms));
    }
    else if (M_CheckParm("-maximized"))
    {
        Atom atoms[] = {
            _NET_WM_STATE_MAXIMIZED_VERT,
            _NET_WM_STATE_MAXIMIZED_HORZ,
        };
        XChangeProperty(display, window, _NET_WM_STATE, XA_ATOM, 32, PropModeReplace, (unsigned char*)atoms, arrlen(atoms));
    }

    XMapWindow(display, window);

    X11_WaitForExposure();

    XSetWMProtocols(display, window, &WM_DELETE_WINDOW, 1);

    gc = XCreateGC(display, window, 0, NULL);

    XSelectInput(display, window, StructureNotifyMask | KeyPressMask | KeyReleaseMask | ButtonPressMask
            | ButtonReleaseMask | PointerMotionMask | EnterWindowMask | LeaveWindowMask);

    XkbSetDetectableAutoRepeat(display, true, &tmp);

    XGetWindowAttributes(display, window, &attr);

    X11_HideCursor();

    if (!M_CheckParm("-noshm") && XShmQueryExtension(display))
    {
        X11_SetupXShm(&attr);
        use_shm = true;
    }
    else
    {
        X11_SetupStandard(&attr);
        use_shm = false;
    }

    XSync(display, False);

    focused = true;

    s->resx             = attr.width;
    s->resy             = attr.height;
    s->pixels           = screen_buffer;
    s->bits_per_pixel   = bits_per_pixel;
    s->bytes_per_pixel  = bytes_per_pixel;

    X11_SetPixelFormat(s);
}

void I_Platform_ShutdownGraphics(screen_t* s)
{
    UNUSED(s);
    if (empty_cursor)
    {
        XUndefineCursor(display, window);
        XFreeCursor(display, empty_cursor);
    }
    if (shminfo.shmaddr)
    {
        XShmDetach(display, &shminfo);
        shmdt(shminfo.shmaddr);
        shmctl(shminfo.shmid, IPC_RMID, 0);
    }
    else if (screen_buffer)
    {
        free(screen_buffer);
    }
    if (vinfo)
    {
        XFree(vinfo);
    }
    if (window)
    {
        XDestroyWindow(display, window);
    }
    if (display)
    {
        XCloseDisplay(display);
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
    if (use_shm)
    {
        XShmPutImage(
            /* dpy          = */ display,
            /* d            = */ window,
            /* gc           = */ gc,
            /* image        = */ image,
            /* src_x        = */ 0,
            /* src_y        = */ 0,
            /* dest_x       = */ 0,
            /* dest_y       = */ 0,
            /* src_width    = */ image->width,
            /* src_height   = */ image->height,
            /* send_event   = */ False);
    }
    else
    {
        XPutImage(
            /* display  = */ display,
            /* d        = */ window,
            /* gc       = */ gc,
            /* image    = */ image,
            /* src_x    = */ 0,
            /* src_y    = */ 0,
            /* dest_x   = */ 0,
            /* dest_y   = */ 0,
            /* width    = */ image->width,
            /* height   = */ image->height);
    }
    XSync(display, False);
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

static void X11_ResizeWindow(int width, int height)
{
    XWindowAttributes attr;

    if (shminfo.shmaddr)
    {
        XShmDetach(display, &shminfo);
        shmdt(shminfo.shmaddr);
        shmctl(shminfo.shmid, IPC_RMID, 0);
        image->data = NULL;
    }

    if (image)
    {
        XDestroyImage(image);
    }

    XGetWindowAttributes(display, window, &attr);

    attr.width  = width;
    attr.height = height;

    if (use_shm)
    {
        X11_SetupXShm(&attr);
    }
    else
    {
        X11_SetupStandard(&attr);
    }

    I_ResetScreen(width, height, image->data);
}

static int mouse_button;

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
                if (e.xclient.data.l[0] == (long)WM_DELETE_WINDOW)
                {
                    event.type = ev_quit;
                    event.data1 = 0;
                    event.data2 = 0;
                    event.data3 = 0;
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
                event.data1 = mouse_button = e.xbutton.button;
                event.data2 = 0;
                event.data3 = 0;
                D_PostEvent(&event);
                break;

            case ButtonRelease:
                event.type = ev_mouse;
                event.data1 = mouse_button = 0;
                event.data2 = 0;
                event.data3 = 0;
                D_PostEvent(&event);
                break;

            case MotionNotify:
                event.type = ev_mouse;
                event.data1 = mouse_button;
                event.data2 = (e.xmotion.x - image->width / 2) * 3;
                event.data3 = 0;
                D_PostEvent(&event);
                break;

            case EnterNotify:
                focused = true;
                break;

            case LeaveNotify:
                focused = false;
                break;

            case ConfigureNotify:
                if (e.xconfigure.send_event &&
                    (e.xconfigure.width != image->width ||
                    e.xconfigure.height != image->height))
                {
                    X11_ResizeWindow(e.xconfigure.width, e.xconfigure.height);
                }
                break;
        }
    }

    if (focused)
    {
        XWarpPointer(display, None, window, 0, 0, 0, 0, image->width / 2, image->height / 2);
    }
}

struct music_module* I_Platform_GetMusicModule(void)
{
    return NULL;
}

struct sound_module* I_Platform_GetSoundModule(void)
{
    return NULL;
}
