#include "registry.h"
#include <stdexcept>

int IntRegistry::registerType(registryValue funcPtr) {
  values.push_back(funcPtr);
  return values.size() - 1;
}

void *IntRegistry::getType(int key, uintptr_t input0, int input1) {
  if (key < 0 || key >= values.size()) {
    throw std::invalid_argument("Given invalid key!");
  }
  if (values[key] != NULL) {
    return values[key](input0, input1);
  }
  return NULL;
}

void IntRegistry::clear() {
  values.clear();
}

void StringRegistry::registerType(std::string key, registryValue funcPtr) {
  if (values.count(key) > 0) {
    throw std::invalid_argument("Key already exists!");
  }
  values[key] = funcPtr;
}

void *StringRegistry::getType(std::string key, uintptr_t input0, int input1) {
  std::map<std::string, registryValue>::iterator it = values.find(key);
  if (it != values.end()) {
    if (it->second != NULL) {
      return it->second(input0, input1);
    }
    return NULL;
  }
  throw std::invalid_argument("Given invalid key!");
}

void StringRegistry::clear() {
  values.clear();
}