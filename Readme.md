##some_sorta_bar
####A simple bar to display text from a pipe for lightweight window managers.

*e.g.* conky | some_sorta_bar

###Text can be aligned in blocks and alignment is set in the text by marking the *start* and *where it is to be split* with an ampersand and L, C or R

*e.g.* To have the text right aligned

***&R* some text on the right**

*or* To have some text centered and some on the right


***&C* some centered text *&R* some more text on the right**

Text defaults to left alignment if not marked with either *&C* or *&R*

Centered text can be offset 

**#define BAR_CENTER -100     // 0=Screen center or pos/neg to move right/left**

###The nine colours are set at compile time so edit them to suit.

The first colour is the background and the second to ninth are for the text.

To change the colour mark the text with an ampersand and the desired colour number.

*e.g.* To use the second and last colours, mark the text like -

**&R*&2* some text *&9* some more text**

Whether the bar is at *top or bottom* is set at compile time.

**#define TOP_BAR *0*        // 0=Bar at top, 1=Bar at bottom**

The height of the bar can be set at compile time.

**#define BAR_HEIGHT *16*    //**

The width of the bar can be set at compile time. 0 for fullscreen width or 
the number of pixels

**#define BAR_WIDTH *0*    //**

Fonts used are defined in a comma seperated list.
Font is set at compile time and the height of the bar is relevant to the font height 
if the bar height is too small.

*The window manager might need it's space for a bar adjusted.*

The bar has the override redirect flag set to true so it can be rebuilt and started again without a 'good' window manager trying to map it.
