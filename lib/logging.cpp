#include "logging.hpp"

namespace nfcdoorz::logging {
  using namespace std;

  static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;

  void init(
    map<string, docopt::value> args,
    plog::Severity default_severity
  ) {
    switch(args["--verbose"].asLong()) {
      case 1:
        default_severity = plog::info;
        break;
      case 2:
        default_severity = plog::debug;
        break;
      case 3:
        default_severity = plog::verbose;
        break;
    }

    plog::init(default_severity, &consoleAppender);
  }
}
