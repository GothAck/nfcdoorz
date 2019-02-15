#include <string>
#include <map>

#include <docopt/docopt.h>
#include <plog/Log.h>
#include <plog/Appenders/ColorConsoleAppender.h>

#pragma once
namespace nfcdoorz::logging {
  void init(
    std::map<std::string, docopt::value> args,
    plog::Severity default_severity = plog::warning
  );
}
