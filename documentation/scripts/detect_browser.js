// From http://developer.netscape.com/docs/examples/javascript/browser_type.html

// convert all characters to lowercase to simplify testing
var agt=navigator.userAgent.toLowerCase();

// *** BROWSER VERSION ***
// Note: On IE5, these return 4, so use is_ie5up to detect IE5.
var is_major = parseInt(navigator.appVersion);
var is_minor = parseFloat(navigator.appVersion);

// Note: Opera and WebTV spoof Mozilla.  We do strict client detection.
var is_mozilla  = ((agt.indexOf('mozilla')!=-1) && (agt.indexOf('spoofer')==-1)
	    && (agt.indexOf('compatible') == -1) && (agt.indexOf('opera')==-1)
	    && (agt.indexOf('webtv')==-1) && (agt.indexOf('hotjava')==-1));
var is_mozilla2 = (is_mozilla && (is_major == 2));
var is_mozilla3 = (is_mozilla && (is_major == 3));
var is_mozilla4 = (is_mozilla && (is_major == 4));
var is_mozilla4up = (is_mozilla && (is_major >= 4));
var is_mozilla5 = (is_mozilla && (is_major == 5));
var is_mozilla5up = (is_mozilla && (is_major >= 5));
var is_gecko = (agt.indexOf('gecko') != -1);
var is_netscape6 = (is_mozilla5 && (agt.indexOf('netscape6')!=-1));

var is_ie     = ((agt.indexOf("msie") != -1) && (agt.indexOf("opera") == -1));
var is_ie3    = (is_ie && (is_major < 4));
var is_ie4    = (is_ie && (is_major == 4) && (agt.indexOf("msie 4")!=-1) );
var is_ie4up  = (is_ie && (is_major >= 4));
var is_ie5    = (is_ie && (is_major == 4) && (agt.indexOf("msie 5.0")!=-1) );
var is_ie5_5  = (is_ie && (is_major == 4) && (agt.indexOf("msie 5.5") !=-1));
var is_ie5up  = (is_ie && !is_ie3 && !is_ie4);
var is_ie5_5up =(is_ie && !is_ie3 && !is_ie4 && !is_ie5);
var is_ie6    = (is_ie && (is_major == 4) && (agt.indexOf("msie 6.")!=-1) );
var is_ie6up  = (is_ie && !is_ie3 && !is_ie4 && !is_ie5 && !is_ie5_5);

// KNOWN BUG: On AOL4, returns false if IE3 is embedded browser
// or if this is the first browser window opened.  Thus the
// variables is_aol, is_aol3, and is_aol4 aren't 100% reliable.
var is_aol   = (agt.indexOf("aol") != -1);
var is_aol3  = (is_aol && is_ie3);
var is_aol4  = (is_aol && is_ie4);
var is_aol5  = (agt.indexOf("aol 5") != -1);
var is_aol6  = (agt.indexOf("aol 6") != -1);

var is_opera = (agt.indexOf("opera") != -1);
var is_opera2 = (agt.indexOf("opera 2") != -1 || agt.indexOf("opera/2") != -1);
var is_opera3 = (agt.indexOf("opera 3") != -1 || agt.indexOf("opera/3") != -1);
var is_opera4 = (agt.indexOf("opera 4") != -1 || agt.indexOf("opera/4") != -1);
var is_opera5 = (agt.indexOf("opera 5") != -1 || agt.indexOf("opera/5") != -1);
var is_opera5up = (is_opera && !is_opera2 && !is_opera3 && !is_opera4);

var is_webtv = (agt.indexOf("webtv") != -1); 

var is_TVNavigator = ((agt.indexOf("navio") != -1) || (agt.indexOf("navio_aoltv") != -1)); 
var is_AOLTV = is_TVNavigator;

var is_hotjava = (agt.indexOf("hotjava") != -1);
var is_hotjava3 = (is_hotjava && (is_major == 3));
var is_hotjava3up = (is_hotjava && (is_major >= 3));

var is_konqueror = (navigator.appName.indexOf("Konqueror") != -1);

var need_style_tag_cw = 0;
var need_style_tutorial = 0;
var need_style_doxygen = 0;

