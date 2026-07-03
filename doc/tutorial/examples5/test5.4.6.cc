#include "debug.h"

int main()
{

  Debug(libcw_do.on());
  ForAllDebugChannels(if (!debugChannel.is_on()) debugChannel.on());

  Debug(libcw_do.margin().assign("<-- margin -->", 14));
  Debug(libcw_do.marker().assign("<-- marker -->", 14));
  Dout(dc::cat|dc::mouse, "The cat chases the mouse.");
  Dout(dc::mouse|dc::elephant, "The mouse chases the elephant.");
  Dout(dc::notice|nolabel_cf, "Setting indentation to 8 spaces:");
  Dout(dc::notice|blank_label_cf, "<------>");
  Debug(libcw_do.set_indent(8));
  Dout(dc::cat, "The cat sleeps.");
  Dout(dc::elephant, "The elephant looks around:");
  Dout(dc::elephant|blank_label_cf|blank_marker_cf, "where did the mouse go?");
}
