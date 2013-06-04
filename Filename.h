//  Copyright (c) 2012 The 11ers. All rights reserved.

#ifndef FILENAME_H
#define FILENAME_H

#ifndef PATH_MAX
  #define PATH_MAX 1024
#endif

#ifndef NAME_MAX
  #define NAME_MAX PATH_MAX
#endif


namespace Filename {


// Splits up the full filename into three components. Retains the trailing
// slash at the end of dir and the period at the beginning of ext.
// If ext==NULL, base contains the entire local name.
void Split(const char *full, char dir[PATH_MAX], char base[NAME_MAX],
           char ext[NAME_MAX]);

// Tests to see if we can access the specified filename.
bool Access(const char *filename, bool isForWriting=false);

// Return seconds since Epoch (1 Jan 1970) for file, or 0 on failure
double EpochSec(const char *filename);
  
};      // namespace Filename
#endif  // defined(FILENAME_H)
