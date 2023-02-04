/*
Copyright (c) 2013 Benedikt Bitterli

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/

//#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "File.hpp"

int fsize(FILE *fp) {
    int prev = ftell(fp);
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fseek(fp, prev, SEEK_SET);
    return size;
}
#ifdef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#define stat _stat

time_t ftime(const char* path) 
{
    struct stat result;
    if (stat(path, &result) == 0)
    {
        return result.st_mtime;
    }
    return 0;
}
#else

time_t ftime(const char *path) {
    struct stat stat;
    int file = open(path, O_RDONLY);
    if (!file)
        return 0;
    fstat(file, &stat);
    close(file);
#if __USE_XOPEN2K8 || __MINGW32__ || 1
    return stat.st_mtime;
#else
    return stat.st_mtimespec.tv_sec;
#endif
}

#endif
