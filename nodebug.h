#define AllocTag1(p)
#define AllocTag2(p, desc)
#define AllocTag_dynamic_description(p, data)
#define AllocTag(p, data)
#define Debug(STATEMENT...)
#define Dout(cntrl, data)
#define DoutFatal(cntrl, data) LibcwDoutFatal(/**/, /**/, cntrl, data)
#define ForAllDebugChannels(STATEMENT...)
#define ForAllDebugObjects(STATEMENT...)
#define LibcwDebug(dc_namespace, STATEMENT...)
#define LibcwDout(dc_namespace, d, cntrl, data)
#define LibcwDoutFatal(dc_namespace, d, cntrl, data) do { ::std::cerr << data << ::std::endl; ::std::exit(EXIT_FAILURE); } while(1)
#define LibcwdForAllDebugChannels(dc_namespace, STATEMENT...)
#define LibcwdForAllDebugObjects(dc_namespace, STATEMENT...)
#define NEW(x) new x
#define CWDEBUG_ALLOC 0
#define CWDEBUG_MAGIC 0
#define CWDEBUG_LOCATION 0
#define CWDEBUG_LIBBFD 0
#define CWDEBUG_DEBUG 0
#define CWDEBUG_DEBUGOUTPUT 0
#define CWDEBUG_DEBUGM 0
#define CWDEBUG_DEBUGT 0
#define CWDEBUG_MARKER 0
