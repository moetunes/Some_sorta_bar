#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xlocale.h>

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#define TOP_BAR 0        // 0=Bar at top, 1=Bar at bottom
#define BAR_HEIGHT 16
#define BAR_WIDTH 0      // 0=Full width or num pixels
#define BAR_CENTER 0     // 0=Screen center or pos/neg to move right/left
// If font isn't found "fixed" will be used
#define FONT "-*-terminusmod.icons-medium-r-*-*-12-*-*-*-*-*-*-*,-*-stlarch-medium-r-*-*-12-*-*-*-*-*-*-*"
#define FONTS_ERROR 1      // 0 to have missing fonts error shown
// colours are background then eight for the text
#define colour1 "#003040"  // Background colour. The rest colour the text
#define colour2 "#dddddd"  // &2
#define colour3 "#669921"
#define colour4 "#00dd99"
#define colour5 "#ffffff"
#define colour6 "#ffff00"
#define colour7 "#ff00ff"
#define colour8 "#f0f0f0"
#define colour9 "#ff0000"  // &9

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
    unsigned int fh;                      /* Y coordinate to draw characters */
    unsigned int ascent;
    unsigned int descent;
} Iammanyfonts;

static void get_font();
static void print_text();
static int wc_size(char *string);

static const char *defaultcolor[] = { colour1, colour2, colour3, colour4, colour5, colour6, colour7, colour8, colour9, };
static const char *font_list = FONT;

static unsigned int count, j, k;
static unsigned int text_length, c_start, c_end, r_start;
static unsigned int l_length, c_length, r_length;
static char output[256] = {"What's going on here then?"};

static Display *dis;
static unsigned int first_run;
static unsigned int sw;
static unsigned int sh;
static unsigned int height;
static unsigned int width;
static unsigned int screen;
static Window root;
static Window barwin;
static Drawable winbar;

static Iammanyfonts font;

void get_font() {
	char *def, **missing;
	int i, n;

	missing = NULL;
	if(strlen(font_list) > 0)
	    font.fontset = XCreateFontSet(dis, (char *)font_list, &missing, &n, &def);
	if(missing) {
		if(FONTS_ERROR < 1)
            while(n--)
                fprintf(stderr, ":: SSB :: missing fontset: %s\n", missing[n]);
		XFreeStringList(missing);
	}
	if(font.fontset) {
		XFontStruct **xfonts;
		char **font_names;

		font.ascent = font.descent = 0;
		n = XFontsOfFontSet(font.fontset, &xfonts, &font_names);
		for(i = 0, font.ascent = 0, font.descent = 0; i < n; i++) {
			if (font.ascent < (*xfonts)->ascent) font.ascent = (*xfonts)->ascent;
            if (font.descent < (*xfonts)->descent) font.descent = (*xfonts)->descent;
			xfonts++;
		}
		font.width = XmbTextEscapement(font.fontset, " ", 1);
	} else {
		fprintf(stderr, ":: SSB :: Font '%s' Not Found\n:: SSB :: Trying Font 'Fixed'\n", font_list);
		if(!(font.font = XLoadQueryFont(dis, font_list))
		&& !(font.font = XLoadQueryFont(dis, "fixed")))
			fprintf(stderr, ":: SSB :: Error, cannot load font: '%s'\n", font_list);
		font.ascent = font.font->ascent;
		font.descent = font.font->descent;
		font.width = XTextWidth(font.font, " ", 1);
	}
	font.height = font.ascent + font.descent;
}

void update_output(int nc) {
    j=2; k=0;
    l_length = 0; c_length = 0; r_length = 0, text_length = 0;
    unsigned int n, blank_l = 0;
    int bc = BAR_CENTER;
    ssize_t num;
    char win_name[256];

    if(nc < 1) {
        if(!(num = read(STDIN_FILENO, output, sizeof(output)))) {
            fprintf(stderr, "SSB :: FAILED TO READ STDIN!!\n");
            strncpy(output, "FAILED TO READ STDIN!!", 24);
        }
    }
    count = 0;
    text_length = strlen(output);
    //output[text_length] = '\0';
    XFillRectangle(dis, winbar, theme[0].gc, 0, 0, width, height);
    for(k=0;k<width;k++) {
        if(count <= text_length) {
            if(output[count] == '&' && output[count+1] == 'L') count +=2;
            if(output[count] == '&' && output[count+1] == 'C') {
                count += 2;
                l_length = k;
                for(n=count;n<=text_length;n++) {
                    if(output[n] == '&' && output[n+1] == 'R') break;
                    while(output[n] == '&' && output[n+1]-'0' < 10 && output[n+1]-'0' > 0) n += 2;
                    if(output[n] == '\n' || output[n] == '\r') {
                        c_length--;
                        break;
                    }
                    win_name[c_length] = output[n];
                    c_length++;
                }
                win_name[c_length+1] = '\0';
                c_length = wc_size(win_name);
                c_start = (width/2 - c_length/2)+bc;
                for(k=l_length;k<c_start;k+=font.width) {
                     win_name[blank_l] = ' ';
                     blank_l++;
                }
                win_name[blank_l] = '\0';
                if(font.fontset)
                    XmbDrawImageString(dis, winbar, font.fontset, theme[1].gc, l_length, font.fh, win_name, blank_l);
                else
                    XDrawImageString(dis, winbar, theme[1].gc, l_length, font.fh, win_name, blank_l);
            }
            if(output[count] == '&' && output[count+1] == 'R') {
                blank_l = 0;
                count += 2;
                c_end = k;
                for(n=count;n<=text_length;n++) {
                    while(output[n] == '&') n += 2;
                    if(output[n] == '\n' || output[n] == '\r') {
                        r_length--;
                        break;
                    }
                    win_name[r_length] = output[n];
                    r_length++;
                }
                win_name[r_length+1] = '\0';
                r_length = wc_size(win_name);
                r_start = width - r_length-1;
                for(k=c_end;k<r_start-1;k+=font.width) {
                     win_name[blank_l] = ' ';
                     blank_l++;
                }
                k--;
                win_name[blank_l] = '\0';
                if(font.fontset)
                    XmbDrawImageString(dis, winbar, font.fontset, theme[1].gc, c_end, font.fh, win_name, blank_l);
                else
                    XDrawImageString(dis, winbar, theme[1].gc, c_end, font.fh, win_name, blank_l);
                //k += blank_l;
            }
            print_text();
            //printf("k=%d,", k);
        } else {
            if(font.fontset)
                XmbDrawImageString(dis, winbar, font.fontset, theme[1].gc, k, font.fh, " ", 1);
            else
                XDrawImageString(dis, winbar, theme[1].gc, k, font.fh, " ", 1);
            //k =+ font.width;
        }
    }
    XCopyArea(dis, winbar, barwin, theme[1].gc, 0, 0, width, height, 1, 0);
    for(n=0;n<256;n++)
        output[n] = '\0';
    XSync(dis, False);
    return;
}

int wc_size(char *string) {
    int num;
    XRectangle rect;

    num = strlen(string);
    if(font.fontset) {
        XmbTextExtents(font.fontset, string, num, NULL, &rect);
        return rect.width;
    } else {
        return XTextWidth(font.font, string, num);
    }
}

void print_text() {
    char astring[256];
    unsigned int wsize, breaker=0, n=0;

    while(output[count] == '&') {
        if((output[count+1] == 'L') || (output[count+1] == 'C') || (output[count+1] == 'R')) {
            return;
        } else if(output[count+1]-'0' < 10 && output[count+1]-'0' > 0) {
            j = output[count+1]-'0';
            if(j > 1 || j < 10) {
                 j--;
            } else  j = 2;
            count += 2;
        } else {
            breaker = 1;
        }
        if(breaker == 1) break;
    }
    if(output[count] == '&') {
        astring[n] = output[count];
        n++;count++;
    }
    while(output[count] != '&' && output[count] != '\0' && output[count] != '\n' && output[count] != '\r') {
        astring[n] = output[count];
        n++;count++;
    }
    if(n < 1) return;
    astring[n] = '\0';
    wsize = wc_size(astring);
    if((k+wsize) > width) {
        k = width;
        return;
    }
    if(font.fontset)
        XmbDrawImageString(dis, winbar, font.fontset, theme[j].gc, k, font.fh, astring, strlen(astring));
    else
        XDrawImageString(dis, winbar, theme[j].gc, k, font.fh, astring, strlen(astring));
    k += wsize-1;
    for(n=0;n<256;n++)
        astring[n] = '\0';
}

unsigned long getcolor(const char* color) {
    XColor c;
    Colormap map = DefaultColormap(dis,screen);

    if(!XAllocNamedColor(dis,map,color,&c,&c)) {
        fprintf(stderr, "\033[0;31mSSB :: Error parsing color!");
        return 1;
    }
    return c.pixel;
}

int main(int argc, char ** argv){
    unsigned int i, y = 0;
    XEvent ev;
    XSetWindowAttributes attr;
	char *loc;
	fd_set readfds;
    struct timeval tv;

    dis = XOpenDisplay(NULL);
    if (!dis) {fprintf(stderr, "SSB :: unable to connect to display");return 7;}

    root = DefaultRootWindow(dis);
    screen = DefaultScreen(dis);
    sw = XDisplayWidth(dis,screen);
    sh = XDisplayHeight(dis,screen);
    loc = setlocale(LC_ALL, "");
    if (!loc || !strcmp(loc, "C") || !strcmp(loc, "POSIX") || !XSupportsLocale())
        fprintf(stderr, "SSB :: LOCALE FAILED\n");
    get_font();
    if(BAR_HEIGHT > font.height) height = BAR_HEIGHT;
    else height = font.height+2;
    font.fh = ((height - font.height)/2) + font.ascent;
    if(BAR_WIDTH == 0) width = sw-2;  // Take off border width
    else width = BAR_WIDTH-2;
    if (TOP_BAR != 0) y = sh - height-2; // Take off border width

    for(i=0;i<9;i++)
        theme[i].color = getcolor(defaultcolor[i]);
    XGCValues values;

    for(i=0;i<9;i++) {
        values.background = theme[0].color;
        values.foreground = theme[i].color;
        values.line_width = 2;
        values.line_style = LineSolid;
        if(font.fontset) {
            theme[i].gc = XCreateGC(dis, root, GCBackground|GCForeground|GCLineWidth|GCLineStyle,&values);
        } else {
            values.font = font.font->fid;
            theme[i].gc = XCreateGC(dis, root, GCBackground|GCForeground|GCLineWidth|GCLineStyle|GCFont,&values);
        }
    }

    winbar = XCreatePixmap(dis, root, width, height, DefaultDepth(dis, screen));
    XFillRectangle(dis, winbar, theme[0].gc, 0, 0, width, height);
    barwin = XCreateSimpleWindow(dis, root, 0, y, width, height, 1, theme[0].color,theme[0].color);
    attr.override_redirect = True;
    XChangeWindowAttributes(dis, barwin, CWOverrideRedirect, &attr);
    XSelectInput(dis,barwin,ExposureMask);
    XMapRaised(dis, barwin);
    first_run = 0;
    while(1){
       	tv.tv_sec = 0;
       	tv.tv_usec = 200000;
       	FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        select(STDIN_FILENO+1, &readfds, NULL, NULL, &tv);

    	if (FD_ISSET(STDIN_FILENO, &readfds))
    	    update_output(0);
        while(XPending(dis) != 0) {
            XNextEvent(dis, &ev);
            switch(ev.type){
                case Expose:
                    XCopyArea(dis, winbar, barwin, theme[1].gc, 0, 0, width, height, 1, 0);
                    XSync(dis, False);
                    break;
            }
        }
    }

    return (0);
}
