#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <X11/Xatom.h>
#include <signal.h>

#define TOP_BAR 0        // 0=Bar at top, 1=Bar at bottom
#define BAR_HEIGHT 16
#define colour1 "#003040"  // Background colour
#define colour2 "#ff0000"  // The rest colour the text
#define colour3 "#449921"
#define colour4 "#00dd99"
#define colour5 "#ffffff"
#define colour6 "#ffff00"
#define colour7 "#ff00ff"
#define colour8 "#f0f0f0"
#define colour9 "#00ff00"

typedef struct {
    unsigned long color;
    GC gc;
} Theme;
static Theme theme[10];

static void print_text();

// colours are background then eight for the text
static const char *defaultcolor[] = { colour1, colour2, colour3, colour4, colour5, colour6, colour7, colour8, colour9, };
// If font isn't found "fixed" will be used
static const char *fontname = "-*-terminusmod.icons-medium-r-*-*-12-*-*-*-*-*-*-*";

static int i, j, k, fl, fh; // fl is filtered length of text, fh is height for font
static int text_length, c_start, c_end, r_start;
static int total_w, l_length, c_length, r_length, do_l, do_c, do_r;
static char output[256] = {"What's going on here then?"};

static Display *dis;
static int first_run;
static int sw;
static int sh;
static int height;
static int screen;
static XFontStruct *fontbar;
static Window root;
static Window barwin;

void update_output() {
    j=2; k=0; fl=0;
    do_l =0; do_c = 0; do_r = 0;
    l_length = 0; c_length = 0; r_length = 0;
    char *win_name;

    if(!(XFetchName(dis, root, &win_name))) {
        first_run += 1;
        if(first_run > 1) {
            printf("\033[0;31m Failed to get status output.\n  \033[0:m \n");
        } else printf("\tSSB :: Must be starting\n");
    } else {
        for(i=0;i<strlen(win_name);++i) {
            output[i] = win_name[i];
        }
        output[strlen(win_name)] = '\0';
    }
    XFree(win_name);

    if(strlen(output) > 256) text_length = 256;
    else text_length = strlen(output);
    for(i=0;i<text_length;i++) { // Find the legth of text without markers
        while(output[i] == '&') {
            if(output[i+1] == 'L') {
                do_l = 1;
                i += 2;
            } else if(output[i+1] == 'C') {
                do_c = 1;
                if(do_l == 1) l_length = fl;
                i += 2;
            } else if(output[i+1] == 'R') {
                do_r = 1;
                if(do_c == 1) c_length = fl - l_length;
                else if(do_l == 1) l_length = fl;
                i += 2;
            } else if(output[i+1]-'0' < 10 && output[i+1]-'0' > 0) {
                //printf("\t :: i+1 == %c\n", output[i+1]);
                i += 2;
            } else {
                break;
            }
        }
        fl++;
    }
    if(do_l != 1 && do_c != 1 && do_r != 1) do_c = 1;
    if(do_r == 1) r_length = fl - l_length - c_length;
    if(do_c == 1 && c_length == 0) c_length = fl - l_length - r_length;
    if(do_l == 1 && l_length == 0) l_length = fl - c_length - r_length;
    c_start = ((sw/XTextWidth(fontbar, " ", 1)) - c_length)/2;
    c_end = (c_start + c_length);
    r_start = (sw/XTextWidth(fontbar, " ", 1)) - r_length;
    total_w = sw/XTextWidth(fontbar, " ", 1);
    //printf("\t cs == %d - ce == %d - rs == %d - tw == %d\n", c_start,c_end,r_start,total_w);
    //printf("\t ll = %d - cl = %d - rl = %d - fl = %d - textl = %d\n", l_length, c_length, r_length, fl, text_length);
    for(i=1;i<total_w;i++) {
        if(do_r == 1 && i >= r_start) print_text();
        else if(do_c == 1 && i > c_end)
            XDrawImageString(dis, barwin, theme[1].gc, XTextWidth(fontbar, " ", i), fh, " ", 1);
        else if(do_c == 1 && i > c_start) print_text();
        else if(i > l_length)
            XDrawImageString(dis, barwin, theme[1].gc, XTextWidth(fontbar, " ", i), fh, " ", 1);
        else if(do_l == 1) print_text();
    }

    return;
}

void print_text() {
    while(output[k] == '&') {
        if(output[k+1] == 'L') {
            k += 2;
        } else if(output[k+1] == 'C') {
            k += 2;
        } else if(output[k+1] == 'R') {
            k += 2;
        } else if(output[k+1]-'0' < 10 && output[k+1]-'0' > 0) {
            j = output[k+1]-'0';
            if(j > 1 || j < 10) {
                 j--;
            } else  j = 2;
            k += 2;
        } else break;
    }
    XDrawImageString(dis, barwin, theme[j].gc, XTextWidth(fontbar, " ", i), fh, &output[k], 1);
    k++;
}

unsigned long getcolor(const char* color) {
    XColor c;
    Colormap map = DefaultColormap(dis,screen);

    if(!XAllocNamedColor(dis,map,color,&c,&c)) {
        printf("\033[0;31mError parsing color!");
        return 1;
    }
    return c.pixel;
}

void propertynotify(XEvent *e) {
    XPropertyEvent *ev = &e->xproperty;
    if(ev->window == root && ev->atom == XA_WM_NAME) update_output();
}

int main(int argc, char ** argv){
    int i, font_height, y = 0;
    XEvent ev;
    XSetWindowAttributes attr;

    dis = XOpenDisplay(NULL);
    if (!dis) {fprintf(stderr, "unable to connect to display");return 7;}

    root = DefaultRootWindow(dis);
    screen = DefaultScreen(dis);
    sw = XDisplayWidth(dis,screen);
    sh = XDisplayHeight(dis,screen);
    fontbar = XLoadQueryFont(dis, fontname);
    if (!fontbar) {
        fprintf(stderr,"\033[0;34m :: simbar :\033[0;31m unable to load preferred font: %s using fixed", fontname);
        fontbar = XLoadQueryFont(dis, "fixed");
    }
    font_height = fontbar->ascent+fontbar->descent+2;
    if(BAR_HEIGHT > font_height) height = BAR_HEIGHT;
    else height = font_height;
    if (TOP_BAR != 0) y = sh - height;
    fh = ((height - font_height)/2) + fontbar->ascent + 1;
    for(i=0;i<9;i++)
        theme[i].color = getcolor(defaultcolor[i]);
    XGCValues values;

    for(i=1;i<9;i++) {
        values.background = theme[0].color;
        values.foreground = theme[i].color;
        values.line_width = 2;
        values.line_style = LineSolid;
        values.font = fontbar->fid;
        theme[i].gc = XCreateGC(dis, root, GCBackground|GCForeground|GCLineWidth|GCLineStyle|GCFont,&values);
    }

    barwin = XCreateSimpleWindow(dis, root, 0, y, sw, height, 1, theme[0].color,theme[0].color);
    attr.override_redirect = True;
    XChangeWindowAttributes(dis, barwin, CWOverrideRedirect, &attr);
    XSelectInput(dis,barwin,ExposureMask);
    XMapRaised(dis, barwin);
    XSelectInput(dis,root,PropertyChangeMask);
    first_run = 0;
    while(1){
        XNextEvent(dis, &ev);
        switch(ev.type){
            case PropertyNotify:
                propertynotify(&ev);
                break;
            case Expose:
                update_output();
                break;
        }
    }

    return (0);
}
