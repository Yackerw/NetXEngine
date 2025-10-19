#ifndef _REGISTRY
#define _REGISTRY
#include <vector>
#include <stdexcept>

#define registryValue(x) T (*x)(void* staticArg, void* callArg)

template <typename T>
class Registry {
private:
  struct RegistryData {
    registryValue(function);
    void *staticArg;
  };
  std::vector<RegistryData> values;
public:
  int registerType(registryValue(funcPtr), void* staticArg);
  T getType(int key, void* callArg);
  void clear();
};

template <typename T>
int Registry<T>::registerType(registryValue(funcPtr), void *staticArg) {
  RegistryData data;
  data.function = funcPtr;
  data.staticArg = staticArg;
  values.push_back(data);
  return values.size() - 1;
}

template <typename T>
T Registry<T>::getType(int key, void *callArg) {
  if (key < 0 || key >= values.size()) {
    throw std::invalid_argument("Given invalid key!");
  }
  if (values[key].function != NULL) {
    return values[key].function(values[key].staticArg, callArg);
  }
  return NULL;
}

template <typename T>
void Registry<T>::clear() {
  values.clear();
}

#undef registryValue

#endif