#include "util/prog_opts.hh"

#include <iostream>
#include <sstream>

ProgOpts getProgOptions(int argc, char* argv[]) {
  if (argc != expected_args + 1) {
    std::cout << printUsage();
    throw std::invalid_argument("program options incomplete");
  }

  return ProgOpts{argv[0], InputOutputOpts{argv[1], "output.root"}};
}

std::string printUsage() {
  std::stringstream ss;
  ss << "Usage: ./program [input file]" << std::endl;
  return ss.str();
}

