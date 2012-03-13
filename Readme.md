##some_sorta_bar
####A simple bar to display the root window name for lightweight window managers.

###The root window name can be set from ~/.xinitrc with something like

>	conky | while read -r; do xsetroot -name "$REPLY"; done &

or

>	  while true ; do

>		xsetroot -name "$(awk 'sub(/,/,"") {print $3, $4}' <(acpi -b))"

>		sleep 1m
>	  done &

###Alignment is set in the text by marking the start with an ampersand and L, C or R

*e.g.* To have the text right aligned

**
*&R* some text on the right.**

*or* To have some text centered and some on the right


***&C* some centered text *&R* some more text on the right**

###Colours are set at compile time so edit them to suit.

**static const char *defaultcolor[] = { "#003040", "#77aa99", "#449921", "#00dd99", "#ffffff", "#ffff00", "#ff00ff", "#f0f0f0", "#0f0f0f", };**

The first colour is the background and the following eight are for the text.

To change the colour mark the text with an ampersand and the desired colour number.

*e.g.* To use the first and last colours mark the text like -

**&R*&0* some text *&7* some more text.**

Whether the bar is at *top or bottom* is set at compile time.

**#define TOP_BAR *0*        // 0=Bar at top, 1=Bar at bottom**

The height of the bar can be set at compile time.

**#define BAR_HEIGHT *16*
**

Font is set at compile time and the height of the bar is relevant to the font height 
if the bar height is too small.

*The window manager might need it's space for a bar adjusted.*

The bar has the override redirect flag set to true so it can be rebuilt and started again without a 'good' window manager trying to map it.
