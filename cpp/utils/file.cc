
#include "utils/file.h"

#include <cstdio>

#include "utils/exception.h"
#include "utils/log.h"

namespace utils
{
bool ends_with(const std::string& fname, const std::string& suffix)
{
  int diff = int(fname.size()) - int(suffix.size());
  return diff >= 0 && 0 == fname.compare(diff, suffix.size(), suffix);
}

std::string read_file(const std::string& fname)
{
  std::FILE* fp = std::fopen(fname.c_str(), "rb");
  std::fseek(fp, 0L, SEEK_END);
  size_t fsize = (size_t)std::ftell(fp);
  std::rewind(fp);
  std::string content(fsize, 0);
  if (fsize != std::fread(static_cast<void*>(&content[0]), 1, fsize, fp))
  {
    LOG(FATAL, "Unable to read file ", fname, ".");
    throw FileReadException("Unable to read file " + fname + ".");
  }
  std::fclose(fp);
  return content;
}

void write_file(const std::string& fname, const std::string& content)
{
  std::FILE* fp = std::fopen(fname.c_str(), "wb");
  if (!fp)
  {
    LOG(FATAL, "Unable to write file ", fname, ".");
    throw FileWriteException("Unable to write file " + fname + ".");
  }
  std::fwrite(content.data(), sizeof content[0], content.size(), fp);
  std::fclose(fp);
}

std::string repo_path(const std::string& rel_path) 
{
  std::string repo_dir(__FILE__);
  std::replace(repo_dir.begin(), repo_dir.end(), '\\', '/');
  
  // remove file name
  repo_dir.erase(repo_dir.rfind("/"));
  // remove 2 directories names
  repo_dir.erase(repo_dir.rfind("/"));
  repo_dir.erase(repo_dir.rfind("/"));

  repo_dir += '/';
  repo_dir += rel_path;
  return repo_dir;
}

std::string data_path(const std::string& rel_path)
{
  return repo_path("data/" + rel_path);
}

}  // namespace utils
