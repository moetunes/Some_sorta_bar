#include <X11/Xlib.h>
#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <X11/Xatom.h>
#include <signal.h>

// colours are background then eight for the text
static const char *defaultcolor[] = { "#003040", "#77aa99", "#449921", "#00dd99", "#ffffff", "#ffff00", "#ff00ff", "#f0f0f0", "#0f0f0f", };
// If font isn't found "fixed" will be used
static const char *fontbarname = "-*-terminusmod.icons-medium-r-*-*-12-*-*-*-*-*-*-*";

static int i, j, k, m;
static int text_length, text_start, text_space, text_end;
static char output[256];

typedef struct {
    unsigned long color;
    GC gc;
} Theme;
static Theme theme[9];

static void align_left();
static void align_center();
static void align_right();

static Display *dis;
static int sw;
static int height;
static int screen;
static XFontStruct *fontbar;
static Window root;
static Window barwin;

void update_output() {
    j=2, k=0, m=0;
    char *win_name;

    if(!(XFetchName(dis, root, &win_name))) {
        strcpy(output, "What's going on here then?");
            printf("\033[0;31m Failed to get status output. \n");
    } else {
        strncpy(output, win_name, strlen(win_name));
        output[strlen(win_name)] = '\0';
    }
    XFree(win_name);

    if(strlen(output) > 255) text_length = 255;
    else text_length = strlen(output);
    for(i=0;i<text_length;i++) { // Find the legth of text without markers
        m++;
        if(strncmp(&output[i], "&", 1) == 0)
            i += 2;
    }
    for(i=0;i<text_length;i++) {
        if(strncmp(&output[i], "&", 1) == 0) {
            if(strncmp(&output[i+1], "L", 1) == 0) align_left();
            else if(strncmp(&output[i+1], "R", 1) == 0) align_right();
            else align_center();
            break;
        }
    }
    return;
}

void align_left() {
    for(i=0;i<text_length;i++) {
        if(strncmp(&output[i], "&", 1) == 0) {
            if(strncmp(&output[i+1], "L", 1) == 0) {
                i += 1;
                continue;
            }
            j = output[i+1]-'0';
            i += 2;
        }
        k++;
        XDrawImageString(dis, barwin, theme[j].gc, XTextWidth(fontbar, " ", k), fontbar->ascent+1, &output[i], 1);
    }
    text_end = XTextWidth(fontbar, " ", k);
    text_space = sw-text_end;
   for(i=0;i<text_space;i++)
        XDrawImageString(dis, barwin, theme[1].gc, text_end+XTextWidth(fontbar, " ", i), fontbar->ascent+1, " ", 1);
    output[0] ='\0';
    return;
}

void align_center() {
    text_start = (sw/2)-(XTextWidth(fontbar, " ",m)/2);
    text_space = text_start/XTextWidth(fontbar, " ", 1); /* pixel width of text */
    for (i=0;i<text_space;i++)
        XDrawImageString(dis, barwin, theme[1].gc, text_start-XTextWidth(fontbar, " ", i), fontbar->ascent+1, " ", 1);
    for(i=0;i<text_length;i++) {
        if(strncmp(&output[i], "&", 1) == 0) {
            if(strncmp(&output[i+1], "C", 1) == 0) {
                i += 1;
                continue;
            }
            j = output[i+1]-'0';
            i += 2;
        }
        k++;
        XDrawImageString(dis, barwin, theme[j].gc, text_start+XTextWidth(fontbar, " ", k), fontbar->ascent+1, &output[i], 1);
    }
    text_end = text_start + XTextWidth(fontbar, " ",m-1);
    for (i=0;i<text_space; i++)
        XDrawImageString(dis, barwin, theme[1].gc, text_end+XTextWidth(fontbar, " ", i), fontbar->ascent+1, " ", 1);
    output[0] ='\0';
    return;
}

void align_right() {
    text_start = sw-XTextWidth(fontbar, " ",m);
    text_space = text_start/XTextWidth(fontbar, " ", 1); /* pixel width of text */
    for (i=text_space+1;i>0;i--)
        XDrawImageString(dis, barwin, theme[1].gc, 0+XTextWidth(fontbar, " ", i), fontbar->ascent+1, " ", 1);
    for(i=0;i<text_length;i++) {
        if(strncmp(&output[i], "&", 1) == 0) {
            if(strncmp(&output[i+1], "R", 1) == 0) {
                i += 1;
                continue;
            }
            j = output[i+1]-'0';
            i += 2;
        }
        k++;
        XDrawImageString(dis, barwin, theme[j].gc, text_start+XTextWidth(fontbar, " ", k), fontbar->ascent+1, &output[i], 1);
    }
    output[0] ='\0';
    return;
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
    int i;
    XEvent ev;
    XSetWindowAttributes attr; attr.override_redirect = True;

    /* First connect to the display server, as specified in the DISPLAY environment variable. */
    dis = XOpenDisplay(NULL);
    if (!dis) {fprintf(stderr, "unable to connect to display");return 7;}

    root = DefaultRootWindow(dis);
    screen = DefaultScreen(dis);
    sw = XDisplayWidth(dis,screen);
    fontbar = XLoadQueryFont(dis, fontbarname);
    if (!fontbar) {
        fprintf(stderr,"\033[0;34m :: some_sorta_bar :\033[0;31m unable to load preferred font: %s using fixed", fontbarname);
        fontbar = XLoadQueryFont(dis, "fixed");
    }
    height = fontbar->ascent+fontbar->descent+2;
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

    barwin = XCreateSimpleWindow(dis, root, 0, 0, sw, height, 1, theme[0].color,theme[0].color);
    XChangeWindowAttributes(dis, barwin, CWOverrideRedirect, &attr);
    XMapRaised(dis, barwin);
    XSelectInput(dis,root,PropertyChangeMask);
    while(1){
        XNextEvent(dis, &ev);
        switch(ev.type){
        case PropertyNotify:
            propertynotify(&ev);
            break;
        }
    }

    return (0);
}
