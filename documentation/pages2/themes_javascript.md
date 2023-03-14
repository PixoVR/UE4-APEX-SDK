
\page themes_javascript Themes For The Javascript Client

Themes are implemented using named css3 variables.

A default style.css is included automatically, and a named theme css file overloads variable values.

Name			| Default (from style.css)	| Use				| Example Tags	|
------------------------|-------------------------------|-------------------------------|---------------|
\--BG0:			| \#cecece;			| Main Background		|		|
\--BG1:			| \#EEE;			| Brighter Background		|		|
\--BG2:			| \#AAA;			| Darker Background		|		|
\--BG3:			| \#888;			| Even Darker Background	|		|
\--FG0:			| \#000;			| Main Foreground Contrast	|		|
\--FG1:			| \#111;			| Brighter Foreground		|		|
\--FG2:			| \#222;			| Brighter Foreground		|		|
\--FG3:			| \#DDD;			| Additional Foreground		| HR		|
\--Accent1:		| \#1dbad9;			| Accent Color			|		|
\--Accent2:		| \#73c4dd;			| Accent Color Dimmed		|		|
\--ArrowColor:		| \#fcf4d3;			| Background Color for Arrow Menus | arrowMessage |
\--BorderColor1:	| \#888888;			| Main Border Color		|		|
\--BorderColor2:	| \#000;			| Darker Border Color		|		|
\--Border1:		| 1px solid var(--BorderColor1); | Main Border			|		|
\--Border2:		| 1px solid var(--BorderColor2); | Darker Border		|		|
\--HoverBackground:	| \#444444;			| Background for items being hovered over		|	|	
\--HoverBorder:		| \#666666;			| Border for items being hovered over			|	|
\--HoverFontColor:	| \#FFF;			| Font color for items which are being hovered over	|	|
\--ActiveBackground:	| \#888888;			| Background for items which are being interacted with	|	|
\--ActiveBorder:	| white;			| Border for items which are being interacted with	|	|
\--ActiveFontColor:	| white;			| Font color for items which are being interacted with	|	|	
\--FontFace:		| "QuickTypeCondensed";		| Main Font Face		|		|
\--FontSize:		| 14px;				| Main Font Size		|		|
\--FontColorMain:	| \#000;			| Main Font Color		|		|
\--FontColorDim:	| \#888;			| Dimmed or inactive font color	|		|			
\--InputBG:		| \#FBFBFB;			| Background for Input widgets	|		|
\--InputBorder:		| var(--Border1);		| Border for input widgets	|		|
\--InputColor:		| var(--FontColorMain);		| Font color for input widgets	|		|
\--InputFontFace:	| var(--FontFace);		| Font face for input widgets	|		|
\--InputPlaceholderColor: | var(--FontColorDim);	| Color for input placeholders	|		|


\todo provide more example code and a deeper explanation of the uses for the colors

