// +---------------------------------------------------------------------------+
// | MM8DTiny v0.1 * Central controlling device                                |
// | Copyright (C) 2021 Pozsar Zsolt <pozsar.zsolt@szerafingomba.hu>           |
// | mm8dtiny.cpp                                                              |
// | Program for DOS                                                           |
// +---------------------------------------------------------------------------+

//   This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License 3.0 version.
//
//   This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.

// Exit codes:
// 0: normal exit
// 1: Cannot load main configuration file!
// 2: There is not enabled channel!
// 3: Cannot load environment configuration file CH #x!
// 4: Cannot access I/O port!

#include <stdio.h>
#include <string>
#include "messages.h"
#include "server_h.h"

#define    PRGNAME      "MM8DTiny"
#define    PRGVER       "v0.1"

using namespace std;

void writeheader(void)
{
  // write program information to console
  printf("%s%s%s", msg(1), msg(2), msg(3));
  for (int i = 0; i < 63; i++) printf("-");
  printf("\n");
  printf(msg(19));
}

int main(int argc, char *argv[])
{
  int rc = 0;
  if (argc >= 2)
  {
    // show help
    if ((std::string(argv[1]) == "-h") || (std::string(argv[1]) == "--help"))
    {
      printf(msg(13));
      printf(msg(25));
      printf(msg(26));
      printf(msg(27));
      return (0);
    }
    // show name and version
    if ((std::string(argv[1]) == "-v") || (std::string(argv[1]) == "--version"))
    {
      printf("%s %s\n",PRGNAME,PRGVER);
      return (0);
    }
    // run without loop
    if ((std::string(argv[1]) == "-n") || (std::string(argv[1]) == "--no-loop"))
    {
      writeheader();
      rc = server(false);
      return rc;
    }
    // show usage
    printf("%s\n", msg(13));
    return (0);
  } else
  {
    writeheader();
    rc = server(true);
  }
  return rc;
}
