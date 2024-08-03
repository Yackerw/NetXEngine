#ifndef _WIN32
#include "myfnmatch.h"
#include <cstring>
#endif
#include "glob.h"

#include <iostream>

static std::pair<std::string, std::string> SplitPath(const std::string &path)
{
  std::string::size_type last_sep = path.find_last_of("/");
  if (last_sep != std::string::npos)
  {
    return std::make_pair(std::string(path.begin(), path.begin() + last_sep),
                          std::string(path.begin() + last_sep + 1, path.end()));
  }
  return std::make_pair(".", path);
}

#ifdef _WIN32

Glob::Glob(const std::string &pattern)
    : ok_(false)
    , find_handle_(INVALID_HANDLE_VALUE)
{
  std::pair<std::string, std::string> dir_and_mask = SplitPath(pattern);
  _base                                            = dir_and_mask.first;

  find_handle_ = FindFirstFile(widen(pattern).c_str(), &find_data_);
  ok_          = find_handle_ != INVALID_HANDLE_VALUE;
}

Glob::~Glob()
{
  if (find_handle_ != INVALID_HANDLE_VALUE)
  {
    FindClose(find_handle_);
  }
}

bool Glob::Next()
{
  while (ok_ = (FindNextFile(find_handle_, &find_data_) != 0))
  {
    if (strcmp(narrow(find_data_.cFileName).c_str(), ".") != 0
        && strcmp(narrow(find_data_.cFileName).c_str(), "..") != 0)
      break;
  }
  return ok_;
}

#else

Glob::Glob(const std::string &pattern)
    : dir_(0)
    , dir_entry_(0)
{
  std::pair<std::string, std::string> dir_and_mask = SplitPath(pattern);
  _base                                            = dir_and_mask.first;
  dir_                                             = opendir(dir_and_mask.first.c_str());
  pattern_                                         = dir_and_mask.second;

  if (dir_ != 0)
  {
    Next();
  }
}

Glob::~Glob()
{
  if (dir_ != 0)
  {
    closedir(dir_);
  }
}

bool Glob::Next()
{
  while ((dir_entry_ = readdir(dir_)) != 0)
  {
    if
    (
      !myfnmatch(pattern_.c_str(), dir_entry_->d_name)
      && strcmp(dir_entry_->d_name, ".") != 0
      && strcmp(dir_entry_->d_name, "..") !=0
    )
    {
//      printf("%s\n", dir_entry_->d_name);
      return true;
    }
  }
  return false;
}

#endif