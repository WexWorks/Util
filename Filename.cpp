//  Copyright (c) 2012 The 11ers. All rights reserved.

#include "Filename.h"

#include <dirent.h>
#include <sys/stat.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <vector>


void Filename::Split(const char *full, char dir[PATH_MAX], char base[NAME_MAX],
                     char ext[NAME_MAX]) {
  if (full == NULL || full[0] == '\0')
    return;

  dir[0] = '\0';
  base[0] = '\0';
  if (ext != NULL)
    ext[0] = '\0';

  const char *s = strrchr(full, '/');
  if (s != NULL) {
    int len = s - full + 1;
    memcpy(dir, full, len);
    dir[len] = '\0';
    s++;
  } else {
    s = full;
  }

  strcpy(base, s);

  if (ext != NULL) {
    char *b = strrchr(base, '.');
    if (b != NULL) {
      strcpy(ext, b);
      *b = '\0';
    }
  }
}


bool Filename::IsAccessible(const char *filename, bool isForWriting) {
  int state = isForWriting ? W_OK : R_OK;
  if (access(filename, state) == -1)
    return false;
  return true;
}


double Filename::ModEpochSec(const char *filename) {
  struct stat sbuf;
  if (stat(filename, &sbuf) < 0)
    return 0;
  double sec = sbuf.st_mtimespec.tv_sec + 1e-9 * sbuf.st_mtimespec.tv_nsec;
  return sec;
}


double Filename::AccessEpochSec(const char *filename) {
  struct stat sbuf;
  if (stat(filename, &sbuf) < 0)
    return 0;
  double sec = sbuf.st_atimespec.tv_sec + 1e-9 * sbuf.st_atimespec.tv_nsec;
  return sec;
}


size_t Filename::FileSize(const char *filename) {
  struct stat sbuf;
  if (stat(filename, &sbuf) < 0)
    return 0;
  return size_t(sbuf.st_size);
}


bool Filename::ListDirectory(const char *dirname,
                             std::vector<std::string> &filenameVec) {
  DIR *dir = opendir(dirname);
  if (!dir)
    return false;
  for (struct dirent *f = readdir(dir); f != NULL; f = readdir(dir)) {
    if (f->d_type == DT_REG)
      filenameVec.push_back(f->d_name);
  }
  closedir(dir);
  
  return true;
}
