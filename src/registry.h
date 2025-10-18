#ifndef _REGISTRY
#define _REGISTRY
#include <vector>
#include <map>
#include <string>

typedef void *(*registryValue)(uintptr_t freeValue0, int freeValue1);

class IntRegistry {
private:
  std::vector<registryValue> values;
public:
  int registerType(registryValue funcPtr);
  void *getType(int key, uintptr_t input0, int input1);
  void clear();
};

class StringRegistry {
private:
  std::map<std::string, registryValue> values;
public:
  void registerType(std::string key, registryValue funcPtr);
  void *getType(std::string key, uintptr_t input0, int input1);
  void clear();
};

#endif