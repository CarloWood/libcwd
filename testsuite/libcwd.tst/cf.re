// input lines 2
// output till ^MARGINNOTICE
(BFD     : Loading debug info from.*
)*
// type exact
MARGINNOTICE  MARKER   xMARGINNOTICE  MARKER   yMARGINNOTICE  MARKER   z<newline>
MARGINNOTICE  MARKER   <no flags>
noprefix_cf
MARGINnolabel_cf
      NOTICE  MARKER   blank_margin_cf
MARGIN        MARKER   blank_label_cf
MARGINNOTICE           blank_marker_cf
aMARGINb      NOTICE  MARKER   cMARGIN        MARKER   dMARGINNOTICE           eMARGINNOTICE  MARKER   f
nolabel_cf|noprefix_cf
blank_margin_cf|noprefix_cf
blank_label_cf|noprefix_cf
blank_marker_cf|noprefix_cf
      blank_margin_cf|nolabel_cf
MARGINblank_label_cf|nolabel_cf
MARGINblank_marker_cf|nolabel_cf
              MARKER   blank_label_cf|blank_margin_cf
      NOTICE           blank_marker_cf|blank_margin_cf
MARGIN                 blank_marker_cf|blank_label_cf
// type regexp
MARGINNOTICE  MARKER   error_cf: (EAGAIN|EDEADLK) \(Resource temporarily unavailable\)
// type exact
MARGINNOTICE  MARKER   cerr_cf
