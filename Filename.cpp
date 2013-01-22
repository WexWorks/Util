//  Copyright (c) 2012 The 11ers. All rights reserved.

#include "Filename.h"

#include <string.h>
#include <unistd.h>


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


bool Filename::Access(const char *filename, bool isForWriting) {
  int state = isForWriting ? W_OK : R_OK;
  if (access(filename, state) == -1)
    return false;
  return true;
}