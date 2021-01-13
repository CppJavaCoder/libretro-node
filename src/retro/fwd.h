#pragma once

#include <string>
#include <unordered_map>

namespace RETRO {

namespace Cheat {

struct Entry;
struct Block;
using BlockMap = std::unordered_map<std::string, Block>;

}

class Core;
struct Error;
struct Version;

}
