#include <string.h>
#include <sstream>
#include <getopt.h>
#include "cmd_args.h"
#include "common.h"

static const struct option long_opts[] = {
  {"config", required_argument, nullptr, 0},
  {"stats-interval", required_argument, nullptr, 0},
  {nullptr, no_argument, nullptr, 0},
};

CmdArgs ParseArgs(int argc, char *argv[]) {
  CmdArgs ret;

  optind = 1; // reset parsing

  int c, long_index;
  while ((c = getopt_long(argc, argv, "", long_opts, &long_index)) != -1) {
    if (c != 0) continue;

    if (!strcmp("config", long_opts[long_index].name)) {
      ret.config_file = optarg;
    }
    else if (!strcmp("stats-interval", long_opts[long_index].name)) {
      unsigned long stats_interval;
      if (!ParseInt(optarg, stats_interval)) {
        std::stringstream error_msg;
        error_msg << "Invalid stats-interval. Used \"" << optarg << '"';
        throw std::invalid_argument(error_msg.str());
      }
      ret.stats_interval = stats_interval;
    }
  }

  return ret;
}
