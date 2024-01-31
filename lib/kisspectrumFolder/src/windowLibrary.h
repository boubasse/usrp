
#include <cstdio> // perror
#include <iostream> // exit
#include <sys/time.h>
#include <cstring>
// sudo apt install x11proto-dev
// sudo apt install libx11-dev
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>


class windowLibrary {

public:
    
    windowLibrary(){
    
    }
    
    ~windowLibrary(){
    
    }
  
    void init(const char *name, int _x, int _y, int _w, int _h) {
      m_buttons = 0;
      m_clicks = 0;
      m_mmoved = false;
      m_width = _w;
      m_height = _h;
      m_display = XOpenDisplay(getenv("DISPLAY"));
      if(!m_display){ perror("display"); exit(1); }
      m_screen = DefaultScreen(m_display);
      XSetWindowAttributes xswa;
      xswa.event_mask = (ExposureMask|
			 StructureNotifyMask|
			 ButtonPressMask|
			 ButtonReleaseMask|
			 KeyPressMask|
			 KeyReleaseMask|
			 PointerMotionMask);
      xswa.background_pixel = BlackPixel(m_display, m_screen);
      m_window = XCreateWindow(m_display, DefaultRootWindow(m_display), 
			     100,100, m_width,m_height, 10, CopyFromParent,InputOutput,
			     CopyFromParent, CWEventMask|CWBackPixel,
			     &xswa);
      if(!m_window){ perror("window"); exit(1); }
      XStoreName(m_display, m_window, name);
      XMapWindow(m_display, m_window);
      if(_x>=0 && _y>=0){ XMoveWindow(m_display, m_window, _x, _y); }
      m_dbuf = XCreatePixmap(m_display, m_window, m_width, m_height, DefaultDepth(m_display,m_screen));
      m_gc = XCreateGC(m_display, m_dbuf, 0, nullptr);
      if(!m_gc){ perror("gc"); exit(1); }
    }
    
    void clear(){
      setfg(0, 0, 0);
      XFillRectangle(m_display, m_dbuf, m_gc, 0, 0, m_width, m_height);
    }
    
    void show(){
      XCopyArea(m_display, m_dbuf, m_window, m_gc, 0, 0, m_width, m_height, 0, 0);
    }
    
    void sync(){
      XSync(m_display, False);
    }
    
    void events() {
        XEvent ev;
        while(XCheckWindowEvent(m_display,m_window,-1,&ev)){
			switch(ev.type){
				case ButtonPress: {
				  int b = ev.xbutton.button;
				  m_buttons |= 1<<b;
				  m_clicks |= 1<<b;
				  m_mx = ev.xbutton.x;
				  m_my = ev.xbutton.y;
				  break;
				}
				case ButtonRelease: {
				  int b = ev.xbutton.button;
				  m_buttons &= ~(1<<b);
				  m_mx = ev.xbutton.x;
				  m_my = ev.xbutton.y;
				  break;
				}
				case MotionNotify: {
				  m_mx = ev.xbutton.x;
				  m_my = ev.xbutton.y;
				  m_mmoved = true;
				  break;
				}
			}
        }
    }
    
    void setfg(unsigned char r, unsigned char g, unsigned char b) {
      XColor c;
      c.red = r<<8; c.green = g<<8; c.blue = b<<8;
      c.flags = DoRed | DoGreen | DoBlue;
      if(! XAllocColor(m_display, DefaultColormap(m_display,m_screen), &c)){ perror("color"); exit(1); }
      XSetForeground(m_display, m_gc, c.pixel);
    }
    
    void point(int x, int y) {
      XDrawPoint(m_display, m_dbuf, m_gc, x, y);
    }
    
    void line(int x0, int y0, int x1, int y1) {
      XDrawLine(m_display, m_dbuf, m_gc, x0,y0, x1,y1);
    }
    
    void text(int x, int y, const char *s) {
      XDrawString(m_display, m_dbuf, m_gc, x,y, s, strlen(s));
    }
    
    void transient_text(int x, int y, const char *s) {
      XDrawString(m_display, m_window, m_gc, x,y, s, strlen(s));
    }
    
    int m_width, m_height;
    int m_buttons;  // Mask of button states (2|4|8)
    int m_clicks;   // Same, accumulated (must be cleared by owner)
    int m_mx, m_my;   // Cursor position
    bool m_mmoved;  // Pointer moved (must be cleared by owner)
    
private:
    
    Display   *m_display;
    Pixmap    m_dbuf;    
    Window    m_window;
    GC        m_gc;
    int       m_screen; //   
};
