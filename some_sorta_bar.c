#include <X11/Xlib.h>
#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <X11/Xatom.h>
#include <signal.h>

#define TOP_BAR 0        // 0=Bar at top, 1=Bar at bottom
#define BAR_HEIGHT 16

typedef struct {
    unsigned long color;
    GC gc;
} Theme;
static Theme theme[10];

static void print_text();

// colours are background then eight for the text
static const char *defaultcolor[] = { "#003040", "#ff0000", "#449921", "#00dd99", "#ffffff", "#ffff00", "#ff00ff", "#f0f0f0", "#00ff00", };
// If font isn't found "fixed" will be used
static const char *fontname = "-*-terminusmod.icons-medium-r-*-*-12-*-*-*-*-*-*-*";

static int i, j, k, fl, fh; // fl is filtered length of text, fh is height for font
static int text_length, c_start, c_end, r_start;
static int total_w, l_length, c_length, r_length, do_l, do_c, do_r;
static char output[256];

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
        if(first_run < 2) {
            strcpy(output, "What's going on here then?");
            printf("\033[0;31m Failed to get status output.\n  \033[0:m \n");
        }
    } else {
        strncpy(output, win_name, strlen(win_name));
        output[strlen(win_name)] = '\0';
    }
    XFree(win_name);

    if(strlen(output) > 256) text_length = 256;
    else text_length = strlen(output);
    for(i=0;i<text_length;i++) { // Find the legth of text without markers
        while(strncmp(&output[i], "&", 1) == 0) {
            if(strncmp(&output[i+1], "L", 1) == 0) do_l = 1;
            else if(strncmp(&output[i+1], "C", 1) == 0) {
                do_c = 1;
                if(do_l == 1) l_length = fl;
            }
            else if(strncmp(&output[i+1], "R", 1) == 0) {
                do_r = 1;
                if(do_c == 1) c_length = fl - l_length;
                else if(do_l == 1) l_length = fl;
            }
            i += 2;
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
    while(strncmp(&output[k], "&", 1) == 0) {
        if(strncmp(&output[k+1], "L", 1) == 0) {
            k += 2;
        } else if(strncmp(&output[k+1], "C", 1) == 0) {
            k += 2;
        } else if(strncmp(&output[k+1], "R", 1) == 0) {
            k += 2;
        } else {
            j = output[k+1]-'0';
            if(j > 1 || j < 10) j--;
            else j = 2;
            k += 2;
        }
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

    /* First connect to the display server, as specified in the DISPLAY environment variable. */
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

    //printf(" \033[0;33mStatus Bar called ...\n");
    for(i=1;i<9;i++) {
        values.background = theme[0].color;
        values.foreground = theme[i].color;
        values.line_width = 2;
        values.line_style = LineSolid;
        values.font = fontbar->fid;
        theme[i].gc = XCreateGC(dis, root, GCBackground|GCForeground|GCLineWidth|GCLineStyle|GCFont,&values);
    }

    barwin = XCreateSimpleWindow(dis, root, 0, y, sw, height, 1, theme[0].color,theme[0].color);
    attr.override_redirect = True; attr.save_under = True;
    XChangeWindowAttributes(dis, barwin, CWOverrideRedirect|CWSaveUnder, &attr);
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
