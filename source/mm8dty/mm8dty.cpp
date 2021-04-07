// +---------------------------------------------------------------------------+
// | MM8Dtiny v0.3 * Central controlling device                                |
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
// 4: E #04: There is not enabled channel!

#include <conio.h>
#include <cstring>
#include <ctime>
#include <ctype.h>
#include <dos.h>
#include <io.h>
#include <iostream>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "profport.h"

#define    COMPMV                    (0)
#define    COMPSV                    (2)
#define    MAINCONFFILE              "mm8dtiny.ini"
#define    ENVCONFDIR                "channels/"
#define    DELAY                     (1)
#define    AUTOFEED_FOR_POWER_LPT1
/*#define    AUTOFEED_FOR_POWER_LPT2*/
/*#define    AUTOFEED_FOR_POWER_LPT3*/

using namespace std;

// general variables
bool       dem;
char      *msg[66];
int        h, m;
// main settings
char       addr_mm6dch[8][16];
char       addr_mm7dch[8][16];
char       usr_uid[255];
char       httpgetprog[255];
int        mv, sv;
int        ena_ch[8];
int        lpt_prt;
int        lpt_adr[3]                = {0x378,0x278,0x3bc};
// environment parameters
int        c_gasconcentrate_max[8];
int        h_humidity_min[8];
int        h_humidifier_on[8];
int        h_humidifier_off[8];
int        h_humidity_max[8];
int        h_temperature_min[8];
int        h_heater_on[8];
int        h_heater_off[8];
int        h_temperature_max[8];
int        h_heater_disable[8][24];
int        h_light_on1[8];
int        h_light_off1[8];
int        h_light_on2[8];
int        h_light_off2[8];
int        h_vent_on[8];
int        h_vent_off[8];
int        h_vent_disable[8][24];
int        m_humidity_min[8];
int        m_humidifier_on[8];
int        m_humidifier_off[8];
int        m_humidity_max[8];
int        m_temperature_min[8];
int        m_heater_on[8];
int        m_heater_off[8];
int        m_temperature_max[8];
int        m_heater_disable[8][24];
int        m_light_on1[8];
int        m_light_off1[8];
int        m_light_on2[8];
int        m_light_off2[8];
int        m_vent_on[8];
int        m_vent_off[8];
int        m_vent_disable[8][24];
// states
int        in_alarm[8];
int        in_humidity[8];
int        in_ocprot[8];
int        in_opmode[8];
int        in_swmanu[8];
int        in_temperature[8];
int        in_gasconcentrate[8];
int        out_heaters[8];
int        out_lamps[8];
int        out_leds_green[8];
int        out_leds_red[8];
int        out_leds_yellow[8];
int        out_vents[8];
// local port
int        lpt_error_mainssensor;
int        lpt_select_mainsbreaker1;
int        lpt_pe_mainsbreaker2;
int        lpt_ack_mainsbreaker3;
int        lpt_d0_alarm;
int        lpt_d4_led_active;
int        lpt_d5_led_warning;
int        lpt_d6_led_error;
char      *outBuffer;

void gettime()
{
  const time_t now = time(NULL);
  const tm calendar_time = *localtime(&now);
  h = calendar_time.tm_hour;
  m = calendar_time.tm_min;
}

int writelocalports()
{
  int outdata;
  outdata = 64 * lpt_d6_led_error +
            32 * lpt_d5_led_warning +
            16 * lpt_d4_led_active +
                 lpt_d0_alarm;
  outp(lpt_adr[lpt_prt],outdata);
  if (inp(lpt_adr[lpt_prt]) == outdata) return 0; else return 1;
}

void readlocalports()
{
#ifdef AUTOFEED_FOR_POWER_LPT1
  outp(0x37a,0);
#endif
#ifdef AUTOFEED_FOR_POWER_LPT2
  outp(0x27a,0);
#endif
#ifdef AUTOFEED_FOR_POWER_LPT3
  outp(0x3be,0);
#endif
  int indata;
  indata = inp(lpt_adr[lpt_prt] + 1);
  lpt_error_mainssensor = indata & 8;
  if (lpt_error_mainssensor > 1) lpt_error_mainssensor = 1;
  lpt_select_mainsbreaker1 = indata & 16;
  if (lpt_select_mainsbreaker1 > 1) lpt_select_mainsbreaker1 = 1;
  lpt_pe_mainsbreaker2 = indata & 32;
  if (lpt_pe_mainsbreaker2 > 1) lpt_pe_mainsbreaker2 = 1;
  lpt_ack_mainsbreaker3 = indata & 64;
  if (lpt_ack_mainsbreaker3 > 1) lpt_ack_mainsbreaker3 = 1;
}

int resetlocalports()
{
  int outdata;
  outdata = 0;
  outp(lpt_adr[lpt_prt],outdata);
  if (inp(lpt_adr[lpt_prt]) == outdata) return 0; else return 1;
}

void blinkactiveled(int on)
{
  lpt_d4_led_active = on;
  writelocalports();
  delay(250);
}

int openwebpage(char *url)
{
  int rc;
  char commandline[255];
  sprintf(commandline,"%s out.dat http://%s", httpgetprog, url);
  rc = system(commandline);
  if (rc == 0)
  {
    char *string;
    FILE *fp = fopen("out.dat","rb");
    fseek(fp,0,SEEK_END);
    long fsize = ftell(fp);
    rewind(fp);
    string = (char*) malloc(fsize + 1);
    fread(string,1,fsize,fp);
    fclose(fp);
    string[fsize] = 0;
    strcpy(outBuffer,string);
    free(string);
  }
  return rc;
}

int readwriteMM7Ddevice(int channel)
{
  int rc;
  char url[255];
  blinkactiveled(1);
  if (in_opmode[channel] == 0)
    sprintf(url,"%s/operation?uid=%s&h1=%d&h2=%d&h3=%d&h4=%d&t1=%d&t2=%d&t3=%d&t4=%d&g=%d",
                addr_mm7dch[channel],
                usr_uid,
                m_humidity_min[channel],
                m_humidifier_on[channel],
                m_humidifier_off[channel],
                m_humidity_max[channel],
                m_temperature_min[channel],
                m_heater_on[channel],
                m_heater_off[channel],
                m_temperature_max[channel],
                c_gasconcentrate_max[channel]); else
    sprintf(url,"%s/operation?uid=%s&h1=%d&h2=%d&h3=%d&h4=%d&t1=%d&t2=%d&t3=%d&t4=%d&g=%d",
                addr_mm7dch[channel],
                usr_uid,
                h_humidity_min[channel],
                h_humidifier_on[channel],
                h_humidifier_off[channel],
                h_humidity_max[channel],
                h_temperature_min[channel],
                h_heater_on[channel],
                h_heater_off[channel],
                h_temperature_max[channel],
                c_gasconcentrate_max[channel]);
  rc = openwebpage(url);
  if (rc == 0)
  {
    char *input;
    strcpy(input,outBuffer);
    char *token = strtok(input,"\n");
    int line = 0;
    while (token != NULL)
    {
      if (line == 0) in_gasconcentrate[channel] = atoi(token);
      if (line == 1) in_humidity[channel] = atoi(token);
      if (line == 2) in_temperature[channel] = atoi(token);
      token = strtok(NULL,"\n");
      line++;
    }
  }
  return rc;
}

int readwriteMM6Ddevice(int channel, int restorealarm)
{
  int rc;
  char url[127];
  blinkactiveled(1);
  sprintf(url,"%s/operation?uid=%s&a=%d&h=%d&l=%d&v=%d",
              addr_mm6dch[channel],
              usr_uid,
              restorealarm,
              out_heaters[channel],
              out_lamps[channel],
              out_vents[channel]);
  rc = openwebpage(url);
  if (rc == 0)
  {
    char *input;
    strcpy(input,outBuffer);
    char *token = strtok(input,"\n");
    int line = 0;
    while (token != NULL)
    {
      if (line == 0) in_alarm[channel] = atoi(token);
      if (line == 1) in_opmode[channel] = atoi(token);
      if (line == 2) in_swmanu[channel] = atoi(token);
      if (line == 4) in_ocprot[channel] = atoi(token);
      token = strtok(NULL,"\n");
      line++;
    }
  }
  blinkactiveled(0);
  return rc;
}

int resetMM6Ddevice(int channel)
{
  int rc;
  char url[127];
  blinkactiveled(1);
  sprintf(url,"%s/set/all/off?uid=%s",addr_mm6dch[channel],usr_uid);
  rc = openwebpage(url);
  blinkactiveled(0);
  return rc;
}

int restoreMM6Dalarm(int channel)
{
  int rc;
  char url[127];
  blinkactiveled(1);
  sprintf(url,"%s/set/alarm/off?uid=%s",addr_mm6dch[channel],usr_uid);
  rc = openwebpage(url);
  blinkactiveled(0);
  return rc;
}

int getcontrollerversion(int conttype, int channel)
{
  int rc;
  mv = 0;
  sv = 0;
  char url[127];
  blinkactiveled(1);
  if (conttype == 6)
    sprintf(url,"%s/version",addr_mm6dch[channel]); else
    sprintf(url,"%s/version",addr_mm7dch[channel]);
  rc = openwebpage(url);
  if (rc==0)
  {
    mv = outBuffer[5] - 48;
    sv = outBuffer[7] - 48;
  }
  blinkactiveled(0);
  return rc;
}

void loadenvirconf(char *directory, int channel)
{
  char *section;
  char *entry;
  char filename[30];
  sprintf(filename,"%senv-ch%d.ini",directory,channel);
  section = "common";
  entry = "gasconcentrate_max";
  c_gasconcentrate_max[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  section = "hyphae";
  entry = "humidity_min";
  h_humidity_min[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  entry = "humidifier_on";
  h_humidifier_on[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  entry = "humidifier_off";
  h_humidifier_off[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  entry = "humidity_max";
  h_humidity_max[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  entry = "temperature_min";
  h_temperature_min[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  entry = "heater_on";
  h_heater_on[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  entry = "heater_off";
  h_heater_off[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  entry = "temperature_max";
  h_temperature_max[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  for (int h = 0; h < 24; h++)
  {
    if (h < 10)
    {
      entry = "heater_disable_0X";
      entry[16] = h + '0';
    }
    if (h > 9)
    {
      entry = "heater_disable_1X";
      entry[16] = h - 10 + '0';
    }
    if (h > 19)
    {
      entry = "heater_disable_2X";
      entry[16] = h - 20 + '0';
    }
    h_heater_disable[channel - 1][h] = get_private_profile_int(section, entry, 0, filename);
  }
  entry = "light_on1";
  h_light_on1[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  entry = "light_off1";
  h_light_on1[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  entry = "light_on2";
  h_light_on2[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  entry = "light_off2";
  h_light_off2[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  entry = "vent_on";
  h_vent_on[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  entry = "vent_off";
  h_vent_off[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  for (int h = 0; h < 24; h++)
  {
    if (h < 10)
    {
      entry = "vent_disable_0X";
      entry[14] = h + '0';
    }
    if (h > 9)
    {
      entry = "vent_disable_1X";
      entry[14] = h - 10 + '0';
    }
    if (h > 19)
    {
      entry = "vent_disable_2X";
      entry[14] = h - 20 + '0';
    }
    h_vent_disable[channel - 1][h] = get_private_profile_int(section, entry, 0, filename);
  }
  section = "mushroom";
  entry = "humidity_min";
  m_humidity_min[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  entry = "humidifier_on";
  m_humidifier_on[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  entry = "humidifier_off";
  m_humidifier_off[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  entry = "humidity_max";
  m_humidity_max[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  entry = "temperature_min";
  m_temperature_min[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  entry = "heater_on";
  m_heater_on[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  entry = "heater_off";
  m_heater_off[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  entry = "temperature_max";
  m_temperature_max[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  for (int h = 0; h < 24; h++)
  {
    if (h < 10)
    {
      entry = "heater_disable_0X";
      entry[16] = h + '0';
    }
    if (h > 9)
    {
      entry = "heater_disable_1X";
      entry[16] = h - 10 + '0';
    }
    if (h > 19)
    {
      entry = "heater_disable_2X";
      entry[16] = h - 20 + '0';
    }
    m_heater_disable[channel - 1][h] = get_private_profile_int(section, entry, 0, filename);
  }
  entry = "light_on1";
  m_light_on1[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  entry = "light_off1";
  m_light_on1[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  entry = "light_on2";
  m_light_on2[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  entry = "light_off2";
  m_light_off2[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  entry = "vent_on";
  m_vent_on[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  entry = "vent_off";
  m_vent_off[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  for (int h = 0; h < 24; h++)
  {
    if (h < 10)
    {
      entry = "vent_disable_0X";
      entry[14] = h + '0';
    }
    if (h > 9)
    {
      entry = "vent_disable_1X";
      entry[14] = h - 10 + '0';
    }
    if (h > 19)
    {
      entry = "vent_disable_2X";
      entry[14] = h - 20 + '0';
    }
    m_vent_disable[channel - 1][h] = get_private_profile_int(section, entry, 0, filename);
  }
}

void loadmainconf(char *filename)
{
  char *section;
  char *entry;
  section = "enable";
  entry = "ena_chX";
  for (int channel = 1; channel < 9; ++channel)
  {
    entry[6] = channel + '0';
    ena_ch[channel - 1] = get_private_profile_int(section, entry, 0, filename);
  }
  section = "MM6D";
  entry = "addr_mm6dchX";
  for (int channel = 1; channel < 9; ++channel)
  {
    entry[11] = channel + '0';
    get_private_profile_string(section,entry,"0.0.0.0",addr_mm6dch[channel - 1],255,filename);
  }
  section = "MM7D";
  entry = "addr_mm7dchX";
  for (int channel = 1; channel < 9; ++channel)
  {
    entry[11] = channel + '0';
    get_private_profile_string(section,entry,"0.0.0.0",addr_mm7dch[channel - 1],255,filename);
  }
  section = "LPTport";
  entry = "lpt_prt";
  lpt_prt = get_private_profile_int(section, entry, 0, filename);
  section = "user";
  entry = "usr_uid";
  get_private_profile_string(section,entry,"admin",usr_uid,255,filename);
  section = "httpget";
  entry = "prog";
  get_private_profile_string(section,entry,"htget.exe -quiet -o",httpgetprog,255,filename);
}

void analise(int section)
{
  gettime();
  lpt_d0_alarm = 0;
  lpt_d4_led_active = 0;
  lpt_d5_led_warning = 0;
  lpt_d6_led_error = 0;
  // local ports
  if (section == 1)
  {
    if (lpt_error_mainssensor == 1)
    {
      lpt_d6_led_error = 1;
      cerr << msg[37] << "\n";
      if (dem) cout << msg[37] << "\n";
    }
    if ((lpt_select_mainsbreaker1 == 1) ||
        (lpt_pe_mainsbreaker2 == 1) ||
        (lpt_ack_mainsbreaker3 == 1)) lpt_d6_led_error = 1;
    if (lpt_select_mainsbreaker1 == 1)
    {
      cerr << msg[38] << "\n";
      if (dem) cout << msg[38] << "\n";
    }
    if (lpt_pe_mainsbreaker2 == 1)
    {
      cerr << msg[39] << "\n";
      if (dem) cout << msg[39] << "\n";
    }
    if (lpt_ack_mainsbreaker3 == 1)
    {
      cerr << msg[40] << "\n";
      if (dem) cout << msg[40] << "\n";
    }
    // MM6D devices
    for (int channel = 0; channel < 8; channel++)
      if (ena_ch[channel] > 0)
      {
        if (in_alarm[channel] == 1)
        {
          lpt_d0_alarm = 1;
          cerr << msg[42] << channel+1 << "!\n";
          if (dem) cout << msg[42] << channel+1  << "!\n";
          cout << msg[63] << channel+1 << msg[0];
          if (restoreMM6Dalarm(channel) == 0)
            cout << msg[4] << "\n"; else
            cout << msg[5] << "\n";
        }
        if (in_swmanu[channel] == 1)
        {
          lpt_d5_led_warning = 1;
          cout << msg[43] << channel+1 << ".\n";
          if (dem) cout << msg[43] << channel+1  << ".\n";
        }
        if (in_ocprot[channel] == 1)
        {
          lpt_d6_led_error = 1;
          cerr << msg[41] << channel+1 << "!\n";
          if (dem) cout << msg[41] << channel+1  << "!\n";
        }
      }
    } else
    {
    for (int channel = 0; channel < 8; channel++)
      if (ena_ch[channel] > 0)
      {
        cout << "  CH#" << channel+1 << ":\n";
        cout << msg[50] << in_temperature[channel] << " øC\n";
        cout << msg[51] << in_humidity[channel] << "%\n";
        cout << msg[52] << in_gasconcentrate[channel] << "%\n";
        if (in_opmode[channel] == 0)
        {
          // growing mushroom
          cout << msg[48] << "\n";
          // LEDs
          out_leds_green[channel] = 1;
          out_leds_red[channel] = 0;
          out_leds_yellow[channel] = 0;
          // bad temperature
          if (in_temperature[channel] < m_heater_on[channel])
            out_leds_yellow[channel] = 1;
          if (in_temperature[channel] < m_temperature_min[channel])
          {
            out_leds_red[channel] = 1;
            cout << msg[58] << msg[61] << "\n";
          }
          if (in_temperature[channel] > m_temperature_max[channel])
          {
            out_leds_red[channel] = 1;
            cout << msg[58] << msg[62] <<"\n";
          }
          // bad humidity
          if (in_humidity[channel] < m_humidity_min[channel])
          {
            out_leds_red[channel] = 1;
            cout << msg[59] << msg[61] << "\n";
          }
          if (in_humidity[channel] > m_humidity_max[channel])
          {
            out_leds_red[channel] = 1;
            cout << msg[59] << msg[62] << "\n";
          }
          if (out_leds_red[channel] == 1)
          {
            out_leds_green[channel] = 0;
            out_leds_yellow[channel] = 0;
          }
          // bad gas concentrate
          if (in_gasconcentrate[channel] > c_gasconcentrate_max[channel])
          {
            out_leds_red[channel] = 1;
            cout << msg[60] << "\n";
          }
          if (out_leds_red[channel] == 1)
          {
            out_leds_green[channel] = 0;
            out_leds_yellow[channel] = 0;
          }
          // heaters
          out_heaters[channel] = 0;
          if (in_temperature[channel] < m_heater_on[channel])
            out_heaters[channel] = 1;
          if (in_temperature[channel] > m_heater_off[channel])
            out_heaters[channel] = 0;
          if (m_heater_disable[channel][h] == 1) out_heaters[channel] = 0;
          // lights
          out_lamps[channel] = 0;
          if ((h >= m_light_on1[channel]) && (h < m_light_off1[channel]))
            out_lamps[channel] = 1;
          if ((h >= m_light_on2[channel]) && (h < m_light_off2[channel]))
            out_lamps[channel] = 1;
          // ventilators
          out_vents[channel] = 0;
          if ((m > m_vent_on[channel]) && (m < m_vent_off[channel]))
            out_vents[channel] = 1;
          if (m_vent_disable[channel][h] == 1) out_vents[channel] = 0;
          if (in_humidity[channel] > m_humidity_max[channel])
               out_vents[channel] = 1;
          if (in_gasconcentrate[channel] > c_gasconcentrate_max[channel])
               out_vents[channel] = 1;
          if (in_temperature[channel] > m_temperature_max[channel])
               out_vents[channel] = 1;
        } else
        {
          // growing hyphae
          cout << msg[49] << "\n";
          // LEDs
          out_leds_green[channel] = 1;
          out_leds_red[channel] = 0;
          out_leds_yellow[channel] = 0;
          // bad temperature
          if (in_temperature[channel] < h_heater_on[channel])
            out_leds_yellow[channel] = 1;
          if (in_temperature[channel] < h_temperature_min[channel])
          {
            out_leds_red[channel] = 1;
            cout << msg[58] << msg[61] << "\n";
          }
          if (in_temperature[channel] > h_temperature_max[channel])
          {
            out_leds_red[channel] = 1;
            cout << msg[58] << msg[62] << "\n";
          }
          // bad humidity
          if (in_humidity[channel] < h_humidity_min[channel])
          {
            out_leds_red[channel] = 1;
            cout << msg[59] << msg[61] << "\n";
          }
          if (in_humidity[channel] > h_humidity_max[channel])
          {
            out_leds_red[channel] = 1;
            cout << msg[59] << msg[62] << "\n";
          }
          // bad gas concentrate
          if (in_gasconcentrate[channel] > c_gasconcentrate_max[channel])
          {
            out_leds_red[channel] = 1;
            cout << msg[60] << "\n";
          }
          if (out_leds_red[channel] == 1)
          {
            out_leds_green[channel] = 0;
            out_leds_yellow[channel] = 0;
          }
          // heaters
          out_heaters[channel] = 0;
          if (in_temperature[channel] < h_heater_on[channel])
            out_heaters[channel] = 1;
          if (in_temperature[channel] > h_heater_off[channel])
            out_heaters[channel] = 0;
          if (h_heater_disable[channel][h] == 1) out_heaters[channel] = 0;
          // lamps
          out_lamps[channel] = 0;
          if ((h >= h_light_on1[channel]) && (h < h_light_off1[channel]))
            out_lamps[channel] = 1;
          if ((h >= h_light_on2[channel]) && (h < h_light_off2[channel]))
            out_lamps[channel] = 1;
          // ventilators
          out_vents[channel] = 0;
          if ((m > h_vent_on[channel]) && (m < h_vent_off[channel]))
            out_vents[channel] = 1;
          if (h_vent_disable[channel][h] == 1) out_vents[channel] = 0;
          if (in_humidity[channel] > h_humidity_max[channel])
               out_vents[channel] = 1;
          if (in_gasconcentrate[channel] > c_gasconcentrate_max[channel])
               out_vents[channel] = 1;
          if (in_temperature[channel] > h_temperature_max[channel])
               out_vents[channel] = 1;
        }
        // messages
        cout << msg[53];
        if (out_heaters[channel] == 1)
          cout << msg[56] << "\n"; else
          cout << msg[57] << "\n";
        cout << msg[54];
        if (out_lamps[channel] == 1)
          cout << msg[56] << "\n"; else
          cout << msg[57] << "\n";
        cout << msg[55];
        if (out_vents[channel] == 1)
          cout << msg[56] << "\n"; else
          cout << msg[57] << "\n";
        cout << "\n";
      }
  }
}

void messages()
{
  msg[0] = "...";
  msg[1] = "MM8Dtiny v0.3 * Central remote controlling device";
  msg[2] = "Copyright (C) 2021 Pozsar Zsolt <pozsar.zsolt@szerafingomba.hu>";
  msg[3] = "Web: http://www.szerafingomba.hu/equipments/";
  msg[4] = "completed.";
  msg[5] = "failed!";
  msg[6] = "* Load";
  msg[7] = " main settings";
  msg[8] = " environment parameters";
  msg[9] = "* Reset local ports";
  msg[10] = "  Enabled channel(s): ";
  msg[11] = "* Key hit detected. (Hint: [ALT]-[X] - stop program.)";
  msg[12] = "* Set MM6D device to default state on CH#";
  msg[13] = "";
  msg[14] = "STOPPING CONTROLLER PROGRAM...";
  msg[15] = "  Interface port: LPT";
  msg[16] = "  IP address of MM6D device on CH#";
  msg[17] = "  IP address of MM7D device on CH#";
  msg[18] = "* E #01: There is not enabled channel!";
  msg[19] = "INITIALIZING CONTROLLER PROGRAM...";
  msg[20] = "STARTING PROCESSING LOOP...";
  msg[21] = "* Checking remote devices...";
  msg[22] = "  Version of MM6D device on CH#";
  msg[23] = "  Version of MM7D device on CH#";
  msg[24] = "cannot access, disabled!";
  msg[25] = "\0";
  msg[26] = "\0";
  msg[27] = "\0";
  msg[28] = "* Get status of MM6D on CH#";
  msg[29] = "* Read data from local port.";
  msg[30] = "* Write data to local port.";
  msg[31] = "* Analise data.";
  msg[32] = "* Get parameters of air from MM7D on CH#";
  msg[33] = "* Set status LEDs of MM7D on CH#";
  msg[34] = "* Set outputs of MM6D on CH#";
  msg[35] = "* * * Begin of processing loop * * * ";
  msg[36] = "* * * End of processing loop * * *";
  msg[37] = "* E #05: No mains voltage!";
  msg[38] = "* E #06: Overcurrent breaker of house is opened!";
  msg[39] = "* E #07: Overcurrent breaker of odd side of tents is opened!";
  msg[40] = "* E #08: Overcurrent breaker of even side of tents is opened!";
  msg[41] = "* E #09: Overcurrent breaker of MM6D is opened on CH#";
  msg[42] = "* E #10: Alarm event detected on CH#";
  msg[43] = "* W #01: Manual mode switch is on position on CH#";
  msg[44] = "Usage: mm8dtiny.exe [parameter]";
  msg[45] = "  [--double] | [-d]: double error message (for console and serial consol)";
  msg[46] = "  [--help] | [-h]:   this screen";
  msg[47] = " Exited to DOS.";
  msg[48] = "    operation mode:\t\tgrowing mushroom";
  msg[49] = "    operation mode:\t\tgrowing hyphae";
  msg[50] = "    temperature:\t\t";
  msg[51] = "    relative humidity:\t\t";
  msg[52] = "    relative gas concentrate:\t";
  msg[53] = "    heaters:\t\t\t";
  msg[54] = "    lamps:\t\t\t";
  msg[55] = "    ventilators:\t\t";
  msg[56] = "ON";
  msg[57] = "OFF";
  msg[58] = "    Temperature is too ";
  msg[59] = "    Relative humidity is too ";
  msg[60] = "    Unwanted gas concentrate is too high!";
  msg[61] = "low!";
  msg[62] = "high!";
  msg[63] = "* Restore alarm input of MM6D device on CH#";
  msg[64] = "not compatible version, disabled!";
  msg[65] = "* Set all MM6D devices to default state...";
}

int main(int argc, char *argv[])
{
  // usage screen
  if ((std::string(argv[1]) == "-h") || (std::string(argv[1]) == "--help"))
  {
    cout << msg[44] << "\n" << msg[45] << "\n" << msg[46] << "\n";
    return (0);
  }
  // normal start
  if ((std::string(argv[1]) == "-d") || (std::string(argv[1]) == "--double"))
    dem = 1; else dem = 0;
  // load messages
  messages();
  // write program information to console
  cout << msg[1] << "\n" << msg[2] << "\n" << msg[3] << "\n";
  for (int i = 0; i < 63; i++) cout << "-";
  cout << "\n";
  cout << msg[19] << "\n";
  delay(500);
  // reset variables
  for (int channel = 0; channel < 8; channel++)
  {
    in_ocprot[channel] = 0;
    in_opmode[channel] = 0;
    in_swmanu[channel] = 0;
    in_alarm[channel] = 0;
    in_humidity[channel] = 0;
    in_temperature[channel] = 0;
    in_gasconcentrate[channel] = 0;
    out_lamps[channel] = 0;
    out_vents[channel] = 0;
    out_heaters[channel] = 0;
    out_leds_green[channel] = 0;
    out_leds_yellow[channel] = 0;
    out_leds_red[channel] = 0;
  }
  lpt_error_mainssensor = 0;
  lpt_select_mainsbreaker1 = 0;
  lpt_pe_mainsbreaker2 = 0;
  lpt_ack_mainsbreaker3 = 0;
  lpt_d0_alarm = 0;
  lpt_d4_led_active = 0;
  lpt_d5_led_warning = 0;
  lpt_d6_led_error = 0;
  // load main settings
  cout << msg[6] << msg[7] << msg[0];
  loadmainconf(MAINCONFFILE);
  cout << msg[4] << "\n";
  int ii;
  ii = 0;
  for (int channel = 0; channel < 8; ++channel) ii = ii + ena_ch[channel];
  if (ii == 0)
  {
    cerr << msg[18] << "\n";
    if (dem) cout << msg[18] << msg[47] << "\n";
    exit(4);
  }
  cout << msg[15] << lpt_prt + 1 << "\n";
  cout << msg[10];
  for (int channel = 0; channel < 8; ++channel)
    if (ena_ch[channel] > 0) cout << "CH#" << channel+1 << " ";
  cout << "\n";
  for (int channel = 0; channel < 8; ++channel)
  {
    if (ena_ch[channel] > 0) cout << msg[16] << channel+1 << ": " << addr_mm6dch[channel] << "\n";
    if (ena_ch[channel] > 0) cout << msg[17] << channel+1 << ": " << addr_mm7dch[channel] << "\n";
  }
  // checking version of remote devices
  cout << msg[21] << "\n";
  for (int channel = 0; channel < 8; ++channel)
    if (ena_ch[channel] > 0)
    {
      cout << msg[22] << channel+1 << ": ";
      if (getcontrollerversion(6,channel) == 0 )
      {
        if ((mv != COMPMV) && (sv != COMPSV))
        {
          cout << msg[64] << "\n";
          ena_ch[channel] = 0;
        } else cout << "v" << mv << "." << sv << "\n";
      } else
      {
          cout << msg[24] << "\n";
          ena_ch[channel] = 0;
      }
    }
  for (int channel = 0; channel < 8; ++channel)
    if (ena_ch[channel] > 0)
    {
      cout << msg[23] << channel+1 << ": ";
      if (getcontrollerversion(7,channel) == 0 )
      {
        if ((mv != COMPMV) && (sv != COMPSV))
        {
          cout << msg[64] << "\n";
          ena_ch[channel] = 0;
        } else cout << "v" << mv << "." << sv << "\n";
      } else
      {
          cout << msg[24] << "\n";
          ena_ch[channel] = 0;
      }
  }
  // check number of enabled channels
  ii = 0;
  for (int channel = 0; channel < 8; ++channel) ii = ii + ena_ch[channel];
  if (ii == 0)
  {
    cerr << msg[18] << "\n";
    if (dem) cout << msg[18] << msg[47] << "\n";
    exit(4);
  }
  // load environment parameter settings
  cout << msg[6] << msg[8] << msg[0];
  for (int channel = 0; channel < 8; ++channel)
    if (ena_ch[channel] > 0) loadenvirconf(ENVCONFDIR,channel+1);
  cout << msg[4] << "\n";
  // set local ports to default state
  cout << msg[9] << msg[0];
  if (resetlocalports() == 0)
    cout << msg[4] << "\n"; else
    cout << msg[5] << "\n";
  cout << msg[20] << "\n";
  // *** start loop ***
  bool repeat = true;
  while (repeat)
  {
    cout << msg[35] << "\n";
    // section #1:
    // read data from local port
    cout << msg[29] << msg[0];
    readlocalports();
    cout << msg[4] << "\n";
    // analise data
    cout << msg[31] << msg[0] << "\n";
    analise(1);
    // write data to local port
    cout << msg[30] << msg[0];
    if (writelocalports() == 0)
      cout << msg[4] << "\n"; else
      cout << msg[5] << "\n";
    // set outputs of MM6D
    for (int channel = 0; channel < 8; channel++)
      if (ena_ch[channel] > 0)
      {
        cout << msg[34] << channel+1 << msg[0];
        if (readwriteMM6Ddevice(channel,1) == 0)
          cout << msg[4] << "\n"; else
          cout << msg[5] << "\n";
      }
    // section #2:
    // get parameters of air from MM7Ds
    for (int channel = 0; channel < 8; channel++)
      if (ena_ch[channel] > 0)
      {
        cout << msg[32] << channel+1 << msg[0];
        if (readwriteMM7Ddevice(channel) == 0)
          cout << msg[4] << "\n"; else
          cout << msg[5] << "\n";
      }
    // analise data
    cout << msg[31] << msg[0] << "\n";
    analise(2);
    cout << msg[36] << "\n";
    // check key hit (ALT-X is quit)
    if (kbhit())
    {
      char ch = getch();
      if (ch == 0)
      {
        ch = getch();
        if (ch == 45) repeat = false; else cout << msg[11] << "\n";
      }
    }
    delay(DELAY);
  }
  // *** stop loop ***
  cout << msg[14] << "\n";
  // set local ports to default state
  cout << msg[9] << msg[0];
  if (resetlocalports() == 0)
    cout << msg[4] << "\n"; else
    cout << msg[5] << "\n";
  // set remote devices to default state
  cout << msg[65] << msg[0];
  for (int channel = 0; channel < 8; channel++)
    if (ena_ch[channel] > 0)
      if (resetMM6Ddevice(channel))
        cout << msg[4] << "\n"; else
        cout << msg[5] << "\n";
  cout << msg[4] << "\n";
  // exit to DOS
  remove("out.dat");
  return 0;
}
