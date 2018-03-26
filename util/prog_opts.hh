#ifndef _PROG_OPTS_HH_
#define _PROG_OPTS_HH_

#include <string>

const size_t expected_args{1};

struct ProgOpts {
  std::string prog_name;
  std::string input_file_str;
};

ProgOpts getProgOptions(int argc, char* argv[]);

std::string printUsage();

#endif  // _PROG_OPTS_HH_
