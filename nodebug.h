#define Debug(/*STATEMENTS*/...)
#define Dout(cntrl, ...)
#define DoutFatal(cntrl, ...) LibcwDoutFatal(, , cntrl, __VA_ARGS__)
#define ForAllDebugChannels(/*STATEMENT*/...)
#define ForAllDebugObjects(/*STATEMENT*/...)
#define LibcwDebug(dc_namespace, /*STATEMENTS*/...)
#define LibcwDout(dc_namespace, d, cntrl, ...)
#define LibcwDoutFatal(dc_namespace, d, cntrl, ...) \
  do                                                \
  {                                                 \
    ::std::cerr << __VA_ARGS__ << ::std::endl;      \
    ::std::exit(EXIT_FAILURE);                      \
  } while (1)
#define LibcwdForAllDebugChannels(dc_namespace, /*STATEMENT*/...)
#define LibcwdForAllDebugObjects(dc_namespace, /*STATEMENT*/...)
#define CWDEBUG_LOCATION 0
#define CWDEBUG_DEBUG 0
#define CWDEBUG_DEBUGOUTPUT 0
#define CWDEBUG_DEBUGT 0
