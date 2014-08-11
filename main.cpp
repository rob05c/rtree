#include "rtree.h"
#include <string>
#include <iostream>
#include <cstdlib>

namespace {
using std::cout;
using std::string;
using rtree::rect;
using rtree::data;
using rtree::ord_t;

const string default_app_name = "rtree";

struct app_arguments {
  bool success;
  string app_name;
  size_t array_size;
};

app_arguments parseArgs(const int& argc, const char** argv) {
  app_arguments args;
  args.success = false;

  if(argc < 1)
    return args;
  args.app_name = argv[0];

  if(argc < 2)
    return args;
  args.array_size = strtol(argv[1], NULL, 10);

  args.success = true;
  return args;
}

string usage_message(const string& app_name) {
  return "usage: " + (app_name.empty() ? default_app_name : app_name) + " size" + "\n";
}

ord_t random_ord(ord_t min, ord_t max) {
  const auto rand_ord = (ord_t) rand() / RAND_MAX;
  return min + rand_ord * (max - min);
}

} // namespace

int main(int argc, char* argv[]) {
  srand(time(NULL));

/*
  app_arguments args = parseArgs(argc, (const char**)argv);
  if(!args.success) {
    cout << usage_message(args.app_name);
    return 0;
  }
*/

  rtree::tree tree;
  const auto NUM = 1000u;
  const auto RANGE = 100u;
  const auto SIZE_RANGE = 10u;
  for(size_t i = 0, end = NUM; i != end; ++i) {
    const auto top = random_ord(0, RANGE);
    const auto left = random_ord(0, RANGE);
    const auto bottom = top + SIZE_RANGE;
    const auto right = left + SIZE_RANGE;
    const rect r = {top, left, bottom, right};
    const auto key = (int) i;
    const data d = {r, key};
    tree.insert(d);
  }

  cout << tree.str();

  return 0;
}
