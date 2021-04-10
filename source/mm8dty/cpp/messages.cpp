// +---------------------------------------------------------------------------+
// | MM8DTiny v0.1 * Central controlling device                                |
// | Copyright (C) 2021 Pozsar Zsolt <pozsar.zsolt@szerafingomba.hu>           |
// | messages.cpp                                                              |
// | Messages                                                                  |
// +---------------------------------------------------------------------------+

//   This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License 3.0 version.
//
//   This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.

// get message
char const *msg(int const index)
{
  static char const * const message[] =
  {
     /*  0 */  "...",
     /*  1 */  "\nMM8DTiny v0.1 * Central remote controlling device\n",
     /*  2 */  "Copyright (C) 2021 Pozsar Zsolt <pozsar.zsolt@szerafingomba.hu>\n",
     /*  3 */  "Web: http://www.szerafingomba.hu/equipments/\n",
     /*  4 */  "completed.\n",
     /*  5 */  "failed!\n",
     /*  6 */  "* Load",
     /*  7 */  " main settings",
     /*  8 */  " environment parameters",
     /*  9 */  "* Reset local ports",
     /* 10 */  "  Enabled channel(s):",
     /* 11 */  "* Key hit detected. (Hint: [ALT]-[X] - stop program.)",
     /* 12 */  "* Set MM6D device to default state on CH #",
     /* 13 */  "\nUsage: mm8dtiny.exe [--help | -h] [--no-loop | -n] [--version | -v]\n",
     /* 14 */  "STOPPING CONTROLLER PROGRAM...\n",
     /* 15 */  "  Interface port: LPT",
     /* 16 */  "  IP address of MM6D device on CH #",
     /* 17 */  "  IP address of MM7D device on CH #",
     /* 18 */  "* E #02: There is not enabled channel!\n",
     /* 19 */  "INITIALIZING CONTROLLER PROGRAM...\n",
     /* 20 */  "STARTING PROCESSING LOOP...\n",
     /* 21 */  "* Checking remote devices...\n",
     /* 22 */  "  Version of MM6D device on CH #",
     /* 23 */  "  Version of MM7D device on CH #",
     /* 24 */  "cannot access, disabled!\n",
     /* 25 */  "\t[--help]    | [-h]:\tthis screen\n",
     /* 26 */  "\t[--no-loop] | [-n]:\tprogram version\n",
     /* 27 */  "\t[--version] | [-v]:\tprogram version\n\n",
     /* 28 */  "* Get status of MM6D on CH #",
     /* 29 */  "* Read data from local port",
     /* 30 */  "* Write data to local port",
     /* 31 */  "* Analise data",
     /* 32 */  "* Get parameters of air from MM7D on CH #",
     /* 33 */  "* Set status LEDs of MM7D on CH #",
     /* 34 */  "* Set outputs of MM6D on CH #",
     /* 35 */  "* * * Begin of processing loop * * *\n",
     /* 36 */  "* * * End of processing loop * * *\n",
     /* 37 */  "  E #05: No mains voltage!\n",
     /* 38 */  "  E #06: Overcurrent breaker of house is opened!\n",
     /* 39 */  "  E #07: Overcurrent breaker of odd side of tents is opened!\n",
     /* 40 */  "  E #08: Overcurrent breaker of even side of tents is opened!\n",
     /* 41 */  "  E #09: Overcurrent breaker of MM6D is opened on CH #",
     /* 42 */  "  E #10: Alarm event detected on CH #",
     /* 43 */  "  W #01: Manual mode switch is on position on CH #",
     /* 44 */  "* E #01: Cannot load main configuration file!\n",
     /* 45 */  "* E #03: Cannot load environment configuration file CH #",
     /* 46 */  "* E #04: Cannot access I/O port!\n",
     /* 47 */  " Exited to OS.",
     /* 48 */  "    operation mode:\t\tgrowing mushroom\n",
     /* 49 */  "    operation mode:\t\tgrowing hyphae\n",
     /* 50 */  "    temperature:\t\t",
     /* 51 */  "    relative humidity:\t\t",
     /* 52 */  "    relative gas concentrate:\t",
     /* 53 */  "    heaters:\t\t\t",
     /* 54 */  "    lamps:\t\t\t",
     /* 55 */  "    ventilators:\t\t",
     /* 56 */  "ON\n",
     /* 57 */  "OFF\n",
     /* 58 */  "  Temperature is too ",
     /* 59 */  "  Relative humidity is too ",
     /* 60 */  "  Unwanted gas concentrate is too high!\n",
     /* 61 */  "low!\n",
     /* 62 */  "high!\n",
     /* 63 */  "* Restore alarm input of MM6D device on CH #",
     /* 64 */  "not compatible version, disabled!\n",
  };
  return message[index];
}

