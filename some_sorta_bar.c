/* some_sorta_bar.c
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

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

/* ***************** DEFINES ******************* */
#define TOP_BAR 1        // 0=Bar at top, 1=Bar at bottom
#define BAR_HEIGHT 16
#define BAR_WIDTH 0      // 0=Full width or use num pixels
#define BAR_CENTER 0     // 0=Screen center or pos/neg to move right/left
// If font isn't found "fixed" will be used
#define FONT "-*-terminusmod.icons-medium-r-*-*-12-*-*-*-*-*-*-*,-*-stlarch-medium-r-*-*-12-*-*-*-*-*-*-*"
#define FONTS_ERROR 1      // 0 to have missing fonts error shown
// colours are background then eight for the text
#define colour0 "#003040"  // Background colour. The rest colour the text
#define colour1 "#ffffff"  // &1
#define colour2 "#004050"  // &2
#define colour3 "#005060"
#define colour4 "#006070"
#define colour5 "#664422"
#define colour6 "#aaaa00"
#define colour7 "#bbbbbb"
#define colour8 "#997755"
#define colour9 "#00dd99"  // &9

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
    unsigned int fh;            /* Y coordinate to draw characters */
    unsigned int ascent;
    unsigned int descent;
} Iammanyfonts;

static void get_font();
static void print_text();
static int wc_size(char *string, int num);

static const char *defaultcolor[] = { colour0, colour1, colour2, colour3, colour4, colour5, colour6, colour7, colour8, colour9, };
static const char *font_list = FONT;

static unsigned int count, j, k, bg, text_length, c_length;
static char output[256] = {"Some_Sorta_Bar "};

static Display *dis;
static unsigned int sw, sh;
static unsigned int height, width;
static unsigned int screen;
static Window root, barwin;
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
    j=1; text_length = 0; count = 0;
    unsigned int n;
    int bc = BAR_CENTER;
    ssize_t num;
    char win_name[256];

    for(k=0;k<257;k++)
        output[k] = '\0';
    if(nc < 1) {
        if(!(num = read(STDIN_FILENO, output, sizeof(output)))) {
            fprintf(stderr, "SSB :: FAILED TO READ STDIN!!\n");
            strncpy(output, "FAILED TO READ STDIN!!", 24);
        }
    }
    text_length = strlen(output);
    XFillRectangle(dis, winbar, theme[0].gc, 0, 0, width, height);
    for(k=0;k<width;k++) {
        if(count <= text_length) {
            if(output[count] == '\n' || output[count] == '\r') {
                count += 1;
            }
            if(output[count] == '&' && output[count+1] == 'L') count +=2;
            if(output[count] == '&' && (output[count+1] == 'C' || output[count+1] == 'R')) {
                count += 2; c_length=0;
                for(n=count;n<=text_length;n++) {
                    if(output[n] == '&' && output[n+1] == 'R') break;
                    while(output[n] == '&') {
                        if(output[n+1]-'0' < 10 && output[n+1]-'0' >= 0) n += 2;
                        if(output[n+1] == 'B' && output[n+2]-'0' < 10 && output[n+2]-'0' >= 0)
                            n += 3;
                    }
                    if(output[n] == '\n' || output[n] == '\r') {
                        c_length--;
                        break;
                    }
                    win_name[c_length] = output[n];
                    c_length++;
                }
                win_name[c_length] = '\0';
                c_length = wc_size(win_name, c_length);
                if(output[count-1] == 'C')
                    k = (width/2 - c_length/2)+bc;
                if(output[count-1] == 'R')
                    k = width-c_length;
            }
            print_text();
        }
    }
    XCopyArea(dis, winbar, barwin, theme[1].gc, 0, 0, width, height, 0, 0);
    XSync(dis, False);
    return;
}

int wc_size(char *string, int num) {
    XRectangle rect;

    if(font.fontset) {
        XmbTextExtents(font.fontset, string, num, NULL, &rect);
        return rect.width;
    } else {
        return XTextWidth(font.font, string, num);
    }
}

void print_text() {
    char astring[256];
    unsigned int wsize, n=0;

    while(output[count] == '&') {
        if((output[count+1] == 'L') || (output[count+1] == 'C') || (output[count+1] == 'R')) {
            count--;
            break;
        } else if(output[count+1]-'0' < 10 && output[count+1]-'0' >= 0) {
            j = output[count+1]-'0';
            count += 2;
        } else if(output[count+1] == 'B' && output[count+2]-'0' < 10 && output[count+2]-'0' >= 0) {
            bg = output[count+2]-'0';
            count += 3;
        } else break;
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
    wsize = wc_size(astring, n);
    XFillRectangle(dis, winbar, theme[bg].gc, k, 0, wsize, height);
    if((k+wsize) > width) {
        k = width;
        return;
    }
    if(font.fontset)
        XmbDrawString(dis, winbar, font.fontset, theme[j].gc, k, font.fh, astring, n);
    else
        XDrawString(dis, winbar, theme[j].gc, k, font.fh, astring, n);
    k += wsize-1;
    for(wsize=0;wsize<n;wsize++)
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
    height = (BAR_HEIGHT > font.height) ? BAR_HEIGHT : font.height+2;
    font.fh = ((height - font.height)/2) + font.ascent;
    width = (BAR_WIDTH == 0) ? sw : BAR_WIDTH;
    if (TOP_BAR != 0) y = sh - height;

    for(i=0;i<10;i++)
        theme[i].color = getcolor(defaultcolor[i]);
    XGCValues values;

    for(i=0;i<10;i++) {
        values.foreground = theme[i].color;
        values.line_width = 2;
        values.line_style = LineSolid;
        if(font.fontset) {
            theme[i].gc = XCreateGC(dis, root, GCForeground|GCLineWidth|GCLineStyle,&values);
        } else {
            values.font = font.font->fid;
            theme[i].gc = XCreateGC(dis, root, GCForeground|GCLineWidth|GCLineStyle|GCFont,&values);
        }
    }

    winbar = XCreatePixmap(dis, root, width, height, DefaultDepth(dis, screen));
    XFillRectangle(dis, winbar, theme[0].gc, 0, 0, width, height);
    barwin = XCreateSimpleWindow(dis, root, 0, y, width, height, 0, theme[0].color,theme[0].color);
    attr.override_redirect = True;
    XChangeWindowAttributes(dis, barwin, CWOverrideRedirect, &attr);
    XSelectInput(dis,barwin,ExposureMask);
    XMapWindow(dis, barwin);
    int x11_fd = ConnectionNumber(dis);
    while(1){
       	FD_ZERO(&readfds);
        FD_SET(x11_fd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 200000;
        select(x11_fd+1, &readfds, NULL, NULL, &tv);

    	if (FD_ISSET(STDIN_FILENO, &readfds))
    	    update_output(0);
        while(XPending(dis) != 0) {
            XNextEvent(dis, &ev);
            switch(ev.type){
                case Expose:
                    XCopyArea(dis, winbar, barwin, theme[1].gc, 0, 0, width, height, 0, 0);
                    XSync(dis, False);
                    break;
            }
        }
    }

    return (0);
}
