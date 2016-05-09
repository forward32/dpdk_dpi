#ifndef CMD_ARGS_
#define CMD_ARGS_

struct CmdArgs {
  const char *config_file = "";
  uint16_t stats_interval = 0;
};

CmdArgs ParseArgs(int argc, char *argv[]);

#endif // CMD_ARGS_
