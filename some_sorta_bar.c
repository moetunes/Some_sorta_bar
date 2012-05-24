#define _XOPEN_SOURCE 400
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xlocale.h>

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <signal.h>
#include <wchar.h>

#define TOP_BAR 0        // 0=Bar at top, 1=Bar at bottom
#define BAR_HEIGHT 16
#define FONT "-*-terminusmod.icons-medium-r-*-*-12-*-*-*-*-*-*-*,*"
#define colour1 "#003040"  // Background colour
#define colour2 "#dddddd"  // The rest colour the text
#define colour3 "#669921"
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

typedef struct {
    XFontStruct *font;          /* font structure */
    XFontSet fontset;           /* fontset structure */
    int height;                 /* height of the font */
    int width;
    int fh;                      /* Y coordinate to draw characters */
    int ascent;
    int descent;
} Iammanyfonts;

static void get_font();
static void print_text();
static int wc_size(char *string);

// colours are background then eight for the text
static const char *defaultcolor[] = { colour1, colour2, colour3, colour4, colour5, colour6, colour7, colour8, colour9, };
// If font isn't found "fixed" will be used
static const char *font_list = FONT;

static int i, j, k;
static int text_length, c_start, c_end, r_start;
static int total_w, l_length, c_length, r_length;
static char output[256] = {"What's going on here then?"};

static Display *dis;
static int first_run;
static int sw;
static int sh;
static int height;
static int screen;
static Window root;
static Window barwin;

static Iammanyfonts font;

void get_font() {
	char *def, **missing;
	int i, n;
	XRectangle rect;

	missing = NULL;
	font.fontset = XCreateFontSet(dis, (char *)font_list, &missing, &n, &def);
	if(missing) {
		while(n--)
			fprintf(stderr, ":: missing fontset: %s\n", missing[n]);
		XFreeStringList(missing);
	}
	if(font.fontset) {
		//printf("\n\t HAVE FONT\n");
		XFontStruct **xfonts;
		char **font_names;

		font.ascent = font.descent = 0;
		n = XFontsOfFontSet(font.fontset, &xfonts, &font_names);
		for(i = 0, font.ascent = 0, font.descent = 0; i < n; i++) {
			if (font.ascent < (*xfonts)->ascent) font.ascent = (*xfonts)->ascent;
            if (font.descent < (*xfonts)->descent) font.descent = (*xfonts)->descent;
			xfonts++;
		}
		XmbTextExtents(font.fontset, " ", 1, NULL, &rect);
		font.width = rect.width;
	}
	else {
		//printf("\t DON'T HAVE FONT\n");
		if(!(font.font = XLoadQueryFont(dis, font_list))
		&& !(font.font = XLoadQueryFont(dis, "fixed")))
			printf("\tSSB :: Error, cannot load font: '%s'\n", font_list);
		font.ascent = font.font->ascent;
		font.descent = font.font->descent;
		font.width = XTextWidth(font.font, " ", 1);
	}
	font.height = font.ascent + font.descent + 2;
}

void update_output() {
    j=2; k=0, i=0;
    l_length = 0; c_length = 0; r_length = 0, text_length = 0;
    int status, n;
    XTextProperty text_prop;
    char win_name[256];

    status = XGetWMName(dis, root, &text_prop);
    if (!status || !text_prop.value || !text_prop.nitems) {
        first_run += 1;
        if(first_run > 1) {
            printf("\033[0;31m Failed to get status output.\n  \033[0:m \n");
        } else printf("\tSSB :: Must be starting\n");
    } else {
        strncpy(win_name, (char *)text_prop.value, 256);
        while(win_name[text_length] != '\0' && text_length < 256) {
            output[text_length] = win_name[text_length];
            ++text_length;
        }
        output[text_length] = '\0';
    }
    if(text_prop.value) XFree(text_prop.value);

    total_w = sw/font.width;
    for(k=0;k<total_w;k++) {
        if(i < text_length-1) {
            if(output[i] == '&' && output[i+1] == 'C') {
                l_length = k;
                for(n=i;n<text_length;n++) {
                    if(output[n] == '&' && output[n+1] == 'R') break;
                    while(output[n] == '&') n += 2;
                    win_name[c_length] = output[n];
                    c_length++;
                }
                win_name[c_length] = '\0';
                c_length = wc_size(win_name);
                c_start = (total_w/2 - c_length/2);
                for(k=l_length;k<c_start;k++) {
                    XmbDrawImageString(dis, barwin, font.fontset, theme[1].gc, k*font.width, font.fh, " ", 1);
                }
            }
            if(output[i] == '&' && output[i+1] == 'R') {
                c_end = k;
                for(n=i;n<text_length;n++) {
                    while(output[n] == '&') n += 2;
                    win_name[r_length] = output[n];
                    r_length++;
                }
                win_name[r_length] = '\0';
                r_length = wc_size(win_name);
                r_start = total_w -r_length;
                for(k=c_end;k<r_start;k++) {
                    XmbDrawImageString(dis, barwin, font.fontset, theme[1].gc, k*font.width, font.fh, " ", 1);
                }
            }
            print_text();
        } else {
            XmbDrawImageString(dis, barwin, font.fontset, theme[1].gc, k*font.width, font.fh, " ", 1);
        }
    }
    for(k=0;k<256;k++)
        output[k] = '\0';
    XSync(dis, False);
    return;
}

int wc_size(char *string) {
    wchar_t *wp;
    int len, wlen, wsize, n;

    n = strlen(string);
    len = n * sizeof(wchar_t);
    wp = (wchar_t *)malloc(1+len);
    wlen = mbstowcs(wp, string, len);
    wsize = wcswidth(wp, wlen);
    if(wsize < 1) wsize = n;
    free(wp);
    return wsize;
}

void print_text() {
    char astring[100];
    int wsize, n=0;

    while(output[i] == '&') {
        if(output[i+1] == 'L') {
            i += 2;
        } else if(output[i+1] == 'C') {
            i += 2;
        } else if(output[i+1] == 'R') {
            i += 2;
        } else if(output[i+1]-'0' < 10 && output[i+1]-'0' > 0) {
            j = output[i+1]-'0';
            if(j > 1 || j < 10) {
                 j--;
            } else  j = 2;
            i += 2;
        } else break;
    }
    while(output[i] != '&' && output[i] != '\0') {
        astring[n] = output[i];
        n++;i++;
    }
    if(n < 1) return;
    astring[n] = '\0';
    wsize = wc_size(astring);
    XmbDrawImageString(dis, barwin, font.fontset, theme[j].gc, k*font.width, font.fh, astring, strlen(astring));
    k += wsize-1;
    for(n=0;n<100;n++)
        astring[n] = '\0';
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
    int i, y = 0;
    XEvent ev;
    XSetWindowAttributes attr;
	char *loc;

    dis = XOpenDisplay(NULL);
    if (!dis) {fprintf(stderr, "unable to connect to display");return 7;}

    root = DefaultRootWindow(dis);
    screen = DefaultScreen(dis);
    sw = XDisplayWidth(dis,screen);
    sh = XDisplayHeight(dis,screen);
    loc = setlocale(LC_ALL, "");
    if (!loc || !strcmp(loc, "C") || !strcmp(loc, "POSIX") || !XSupportsLocale())
        printf("\tLOCALE FAILED\n");
    for(i=0;i<9;i++)
        theme[i].color = getcolor(defaultcolor[i]);
    XGCValues values;

    for(i=1;i<9;i++) {
        values.background = theme[0].color;
        values.foreground = theme[i].color;
        values.line_width = 2;
        values.line_style = LineSolid;
        theme[i].gc = XCreateGC(dis, root, GCBackground|GCForeground|GCLineWidth|GCLineStyle,&values);
    }
    get_font();
    if(BAR_HEIGHT > font.height) height = BAR_HEIGHT;
    else height = font.height;
    font.fh = ((height - font.height)/2) + font.ascent;
    if (TOP_BAR != 0) y = sh - height;

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
