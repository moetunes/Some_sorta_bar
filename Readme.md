##some_sorta_bar
####A simple bar to display the root window name for lightweight window managers.

###The root window name can be set from ~/.xinitrc with something like

>	conky | while read -r; do xsetroot -name "$REPLY"; done &

or

>	while true ; do

>		xsetroot -name "$(awk 'sub(/,/,"") {print $3, $4}' <(acpi -b))"

>		sleep 1m
>	done &

###Alignment is set in the text by marking the start with an ampersand and L, C or R

**e.g.** To have the text right aligned

&R some text here.

###Colours are set at compile time so edit them to suit.
The first colour is the background and the following eight are for the text.

To change the colour mark the text with an ampersand and the desired colour number.

**e.g.** To use the first and last colours mark the text like -

&R&0 some text &7 some more text.

Font is set at compile time and the height of the bar is relevant to the font height.

*The window manager might need it's space for a bar adjusted.*

The bar has the override redirect flag set to true so it can be rebuilt and started again without the wm trying to map it.
