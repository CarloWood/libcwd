#define FOREGROUND_COLOR black
#define BACKGROUND_COLOR rgb(247, 252, 248)

#define FOREGROUND_COLOR_ADDRESS FOREGROUND_COLOR
#define BACKGROUND_COLOR_ADDRESS #eee

#define FOREGROUND_COLOR_HOVER FOREGROUND_COLOR
#define BACKGROUND_COLOR_HOVER #feffc6

#define FOREGROUND_COLOR_EXAMPLE #2323DC
#define BACKGROUND_COLOR_EXAMPLE INHERIT

#define FOREGROUND_COLOR_CODE #044c2f
#define BACKGROUND_COLOR_CODE INHERIT

#define FOREGROUND_COLOR_OUTPUT rgb(40, 0, 100)
#define BACKGROUND_COLOR_OUTPUT INHERIT

#ifndef NETSCAPE4
#define INHERIT inherit
#else
#define INHERIT BACKGROUND_COLOR	/* Otherwise it becomes green?! */
#endif

#define NORMAL_FONT font-family: arial, sans-serif; font-size: NORMAL_SIZE; font-size-adjust: none
/* Don't use helvetica, it is broken on RedHat which has an alias "-alias-helvetica-medium-i-normal--*-iso8859-1"
 * installed that is actually a horrible looking japanese font.  See https://bugzilla.redhat.com/bugzilla/show_bug.cgi?id=59911 */
#ifndef IE
#define NORMAL_ITALICS_FONT font-family: sans-serif; font-size: NORMAL_SIZE; font-size-adjust: none
#else
#define NORMAL_ITALICS_FONT NORMAL_FONT
#endif

#ifdef KONQUEROR
#define HSIZE1 24pt
#define HSIZE2 20pt
#define HSIZE3 16pt
#define HSIZE4 13pt
#define HSIZE5 10pt
#define HSIZE6 8pt
#elif NETSCAPE4
#define HSIZE1 32pt	/* Netscape 4 can't display larger than 24pt. */
#define HSIZE2 24pt
#define HSIZE3 16pt
#define HSIZE4 14pt
#define HSIZE5 12pt
#define HSIZE6 10pt
#elif NETSCAPE6
#define HSIZE1 32pt
#define HSIZE2 20pt
#define HSIZE3 15pt
#define HSIZE4 13pt
#define HSIZE5 10pt
#define HSIZE6 8pt
#elif MOZILLA
#define HSIZE1 32pt
#define HSIZE2 24pt
#define HSIZE3 16pt
#define HSIZE4 14pt
#define HSIZE5 12pt
#define HSIZE6 10pt
#elif IE
#define HSIZE1 21pt
#define HSIZE2 18pt
#define HSIZE3 16pt
#define HSIZE4 13pt
#define HSIZE5 10pt
#define HSIZE6 8pt
#endif

#ifdef KONQUEROR
#define SIZE2 16pt
#define SIZE3 13pt
#define SIZE4 11pt
#define SIZE5 10pt
#define SIZE6 8pt
#elif NETSCAPE4
#define SIZE2 18pt
#define SIZE3 16pt; font-weight: bold	/* Netscape 4 doesn't scale, this size looks like SIZE4 */
#define SIZE4 14pt
#define SIZE5 12pt
#define SIZE6 10pt
#elif NETSCAPE6
#define SIZE2 14pt
#define SIZE3 13pt
#define SIZE4 11pt
#define SIZE5 10pt
#define SIZE6 8pt
#elif MOZILLA
#define SIZE2 18pt
#define SIZE3 14pt
#define SIZE4 13pt
#define SIZE5 12pt
#define SIZE6 10pt
#elif IE
#define SIZE2 16pt
#define SIZE3 13pt
#define SIZE4 11pt
#define SIZE5 10pt
#define SIZE6 8pt
#endif

#define NORMAL_SIZE SIZE4

