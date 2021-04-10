// +---------------------------------------------------------------------------+
// | MM8DTiny v0.1 * Central controlling device                                |
// | Copyright (C) 2021 Pozsar Zsolt <pozsar.zsolt@szerafingomba.hu>           |
// | server_h.cpp                                                              |
// | Server with HTTP access                                                   |
// +---------------------------------------------------------------------------+

//   This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License 3.0 version.
//
//   This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.

#include <conio.h>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#ifdef __DOS__
#include <dos.h>
#endif

#include "profport.h"
#include "messages.h"

#define    AUTOFEED_FOR_POWER
#define    COMPMV6                   (0)
#define    COMPSV6                   (3)
#define    COMPMV7                   (0)
#define    COMPSV7                   (3)
#define    DELAY                     (10)
#define    HTTPGETPROG               "wget -qO"
#define    MAINCONFFILE              "mm8dty.ini"
#define    TEMPFILE                  "mm8dty.tmp"

#ifdef __DOS__
#define    DEG                       "ø"
#define    ENVCONFDIR                "channels\\"
#else
#define    DEG                       "Â°"
#define    ENVCONFDIR                "channels/"
#endif

using namespace std;

// general variables
bool       dem;
int        h, m;
int        mv, sv;
// main settings
char       addr_mm6dch[8][16];
char       addr_mm7dch[8][16];
char       usr_uid[255];
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
char      *outBuffer    = (char*) malloc(512);

bool writelocalports(void);

// common functions
void gettime()
{
  const time_t now = time(NULL);
  const tm calendar_time = *localtime(&now);
  h = calendar_time.tm_hour;
  m = calendar_time.tm_min;
}

void blinkactiveled(int on)
{
  lpt_d4_led_active = on;
  writelocalports();
}

bool openwebpage(char *url)
{
  int rc;
  char *commandline = (char*)malloc(255);
  sprintf(commandline,"%s %s \"http://%s\"",HTTPGETPROG,TEMPFILE,url);
  rc = system(commandline);
  free(commandline);
  if (rc == 0)
  {
    FILE *fp = fopen(TEMPFILE,"rb");
    fseek(fp,0,SEEK_END);
    long fsize = ftell(fp);
    rewind(fp);
    char *string = (char*) malloc(fsize + 1);
    fread(string,1,fsize,fp);
    fclose(fp);
    string[fsize] = 0;
    strcpy(outBuffer,string);
    free(string);
    return true;
  } else return false;
}

// local port controller functions
bool writelocalports()
{
  int outdata;
#ifndef __DOS__
  if (ioperm(lpt_adr[lpt_prt],1,1)) return false;
#endif
  outdata = 64 * lpt_d6_led_error +
            32 * lpt_d5_led_warning +
            16 * lpt_d4_led_active +
                 lpt_d0_alarm;
  outp(lpt_adr[lpt_prt],outdata);
  if (inp(lpt_adr[lpt_prt]) == outdata) return true; else return false;
}

bool readlocalports()
{
  int indata;
#ifndef __DOS__
  if (ioperm(lpt_adr[lpt_prt]+1,1,1)) return false;
#endif
  indata = inp(lpt_adr[lpt_prt] + 1);
  lpt_error_mainssensor = indata & 8;
  if (lpt_error_mainssensor > 1) lpt_error_mainssensor = 1;
  lpt_select_mainsbreaker1 = indata & 16;
  if (lpt_select_mainsbreaker1 > 1) lpt_select_mainsbreaker1 = 1;
  lpt_pe_mainsbreaker2 = indata & 32;
  if (lpt_pe_mainsbreaker2 > 1) lpt_pe_mainsbreaker2 = 1;
  lpt_ack_mainsbreaker3 = indata & 64;
  if (lpt_ack_mainsbreaker3 > 1) lpt_ack_mainsbreaker3 = 1;
  return true;
}

bool resetlocalports()
{
  int outdata;
  outdata = 0;
#ifndef __DOS__
  if (ioperm(lpt_adr[lpt_prt],1,1)) return false;
#endif
  outp(lpt_adr[lpt_prt],outdata);
  if (inp(lpt_adr[lpt_prt]) == outdata) return true; else return false;
}

// remote device controller functions
bool readwriteMM7Ddevice(int channel)
{
  bool rc;
  char *url = (char*) malloc(255);
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
  free(url);
  if (rc)
  {
    char *input = (char*) malloc(255);
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
    free(input);
  }
  return rc;
}

bool readwriteMM6Ddevice(int channel, int restorealarm)
{
  bool rc;
  char *url = (char*) malloc(255);
  blinkactiveled(1);
  sprintf(url,"%s/operation?uid=%s&a=%d&h=%d&l=%d&v=%d",
              addr_mm6dch[channel],
              usr_uid,
              restorealarm,
              out_heaters[channel],
              out_lamps[channel],
              out_vents[channel]);
  rc = openwebpage(url);
  free(url);
  if (rc)
  {
    char *input = (char*) malloc(255);
    strcpy(input,outBuffer);
    char *token = strtok(input,"\n");
    int line = 0;
    while (token != NULL)
    {
      if (line == 0) in_alarm[channel] = atoi(token);
      if (line == 1) in_opmode[channel] = atoi(token);
      if (line == 2) in_swmanu[channel] = atoi(token);
      if (line == 3) in_ocprot[channel] = atoi(token);
      token = strtok(NULL,"\n");
      line++;
    }
    free(input);
  }
  blinkactiveled(0);
  return rc;
}

bool resetMM6Ddevice(int channel)
{
  bool rc;
  char *url = (char*) malloc(255);
  blinkactiveled(1);
  sprintf(url,"%s/set/all/off?uid=%s",addr_mm6dch[channel],usr_uid);
  rc = openwebpage(url);
  free(url);
  blinkactiveled(0);
  return rc;
}

bool restoreMM6Dalarm(int channel)
{
  bool rc;
  char *url = (char*) malloc(255);
  blinkactiveled(1);
  sprintf(url,"%s/set/alarm/off?uid=%s",addr_mm6dch[channel],usr_uid);
  rc = openwebpage(url);
  free(url);
  blinkactiveled(0);
  return rc;
}

bool getcontrollerversion(int conttype, int channel)
{
  int rc;
  mv = 0;
  sv = 0;
  char *url = (char*) malloc(255);
  blinkactiveled(1);
  if (conttype == 6)
    sprintf(url,"%s/version",addr_mm6dch[channel]); else
    sprintf(url,"%s/version",addr_mm7dch[channel]);
  rc = openwebpage(url);
  free(url);
  if (rc)
  {
    mv = outBuffer[5] - 48;
    sv = outBuffer[7] - 48;
  }
  blinkactiveled(0);
  return rc;
}

// configuration loader functions
bool loadenvirconf(char *directory, int channel)
{
  bool rc = true;
  char *entry = (char*) malloc(32);
  char *filename = (char*) malloc(32);
  char *section = (char*) malloc(32);
  sprintf(filename,"%senv-ch%d.ini", directory, channel);
  printf("%s\n", filename);
  ifstream ifile;
  ifile.open(filename);
  if (ifile)
  {
    sprintf(section, "common");
    sprintf(entry, "gasconcentrate_max");
    c_gasconcentrate_max[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    sprintf(section, "hyphae");
    sprintf(entry, "humidity_min");
    h_humidity_min[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    sprintf(entry, "humidifier_on");
    h_humidifier_on[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    sprintf(entry, "humidifier_off");
    h_humidifier_off[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    sprintf(entry, "humidity_max");
    h_humidity_max[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    sprintf(entry, "temperature_min");
    h_temperature_min[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    sprintf(entry, "heater_on");
    h_heater_on[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    sprintf(entry, "heater_off");
    h_heater_off[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    sprintf(entry, "temperature_max");
    h_temperature_max[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    for (int h = 0; h < 24; h++)
    {
      if (h < 10)
      {
        sprintf(entry, "heater_disable_0X");
        entry[16] = h + '0';
      }
      if (h > 9)
      {
        sprintf(entry, "heater_disable_1X");
        entry[16] = h - 10 + '0';
      }
      if (h > 19)
      {
        sprintf(entry, "heater_disable_2X");
        entry[16] = h - 20 + '0';
      }
      h_heater_disable[channel - 1][h] = get_private_profile_int(section, entry, 0, filename);
    }
    sprintf(entry, "light_on1");
    h_light_on1[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    sprintf(entry, "light_off1");
    h_light_on1[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    sprintf(entry, "light_on2");
    h_light_on2[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    sprintf(entry, "light_off2");
    h_light_off2[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    sprintf(entry, "vent_on");
    h_vent_on[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    sprintf(entry, "vent_off");
    h_vent_off[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    for (int h = 0; h < 24; h++)
    {
      if (h < 10)
      {
        sprintf(entry, "vent_disable_0X");
        entry[14] = h + '0';
      }
      if (h > 9)
      {
        sprintf(entry, "vent_disable_1X");
        entry[14] = h - 10 + '0';
      }
      if (h > 19)
      {
        sprintf(entry, "vent_disable_2X");
        entry[14] = h - 20 + '0';
      }
      h_vent_disable[channel - 1][h] = get_private_profile_int(section, entry, 0, filename);
    }
    sprintf(section,"mushroom");
    sprintf(entry, "humidity_min");
    m_humidity_min[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    sprintf(entry, "humidifier_on");
    m_humidifier_on[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    sprintf(entry, "humidifier_off");
    m_humidifier_off[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    sprintf(entry, "humidity_max");
    m_humidity_max[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    sprintf(entry, "temperature_min");
    m_temperature_min[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    sprintf(entry, "heater_on");
    m_heater_on[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    sprintf(entry, "heater_off");
    m_heater_off[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    sprintf(entry, "temperature_max");
    m_temperature_max[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    for (int h = 0; h < 24; h++)
    {
      if (h < 10)
      {
        sprintf(entry, "heater_disable_0X");
        entry[16] = h + '0';
      }
      if (h > 9)
      {
        sprintf(entry, "heater_disable_1X");
        entry[16] = h - 10 + '0';
      }
      if (h > 19)
      {
        sprintf(entry, "heater_disable_2X");
        entry[16] = h - 20 + '0';
      }
      m_heater_disable[channel - 1][h] = get_private_profile_int(section, entry, 0, filename);
    }
    sprintf(entry, "light_on1");
    m_light_on1[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    sprintf(entry, "light_off1");
    m_light_on1[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    sprintf(entry, "light_on2");
    m_light_on2[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    sprintf(entry, "light_off2");
    m_light_off2[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    sprintf(entry, "vent_on");
    m_vent_on[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    sprintf(entry, "vent_off");
    m_vent_off[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    for (int h = 0; h < 24; h++)
    {
      if (h < 10)
      {
        sprintf(entry, "vent_disable_0X");
        entry[14] = h + '0';
      }
      if (h > 9)
      {
        sprintf(entry, "vent_disable_1X");
        entry[14] = h - 10 + '0';
      }
      if (h > 19)
      {
        sprintf(entry, "vent_disable_2X");
        entry[14] = h - 20 + '0';
      }
      m_vent_disable[channel - 1][h] = get_private_profile_int(section, entry, 0, filename);
    }
  } else rc = false;
  free(entry);
  free(section);
  free(filename);
  return rc;
}

bool loadmainconf(char *filename)
{
  bool rc = true;
  char *entry = (char*) malloc(32);
  char *section = (char*) malloc(32);
  ifstream ifile;
  ifile.open(filename);
  if (ifile)
  {
    sprintf(section,"enable");
    sprintf(entry,"ena_chX");
    for (int channel = 1; channel < 9; ++channel)
    {
      entry[6] = channel + '0';
      ena_ch[channel - 1] = get_private_profile_int(section, entry, 0, filename);
    }
    sprintf(section,"MM6D");
    sprintf(entry,"addr_mm6dchX");
    for (int channel = 1; channel < 9; ++channel)
    {
      entry[11] = channel + '0';
      get_private_profile_string(section,entry,"0.0.0.0",addr_mm6dch[channel - 1],255,filename);
    }
    sprintf(section,"MM7D");
    sprintf(entry,"addr_mm7dchX");
    for (int channel = 1; channel < 9; ++channel)
    {
      entry[11] = channel + '0';
      get_private_profile_string(section,entry,"0.0.0.0",addr_mm7dch[channel - 1],255,filename);
    }
    sprintf(section,"LPTport");
    sprintf(entry,"lpt_prt");
    lpt_prt = get_private_profile_int(section, entry, 0, filename);
    sprintf(section,"user");
    sprintf(entry,"usr_uid");
    get_private_profile_string(section,entry,"admin",usr_uid,255,filename);
  } else rc = false;
  free(entry);
  free(section);
  return rc;
}

// data analiser function
void analise(int section)
{
  gettime();
  lpt_d0_alarm = 0;
  lpt_d4_led_active = 0;
  lpt_d5_led_warning = 0;
  lpt_d6_led_error = 0;
  if (section == 1)
  {
    // local ports
    if (lpt_error_mainssensor == 1)
    {
      lpt_d6_led_error = 1;
      printf(msg(37));
    }
    if ((lpt_select_mainsbreaker1 == 1) ||
        (lpt_pe_mainsbreaker2 == 1) ||
        (lpt_ack_mainsbreaker3 == 1)) lpt_d6_led_error = 1;
    if (lpt_select_mainsbreaker1 == 1) printf(msg(38));
    if (lpt_pe_mainsbreaker2 == 1) printf(msg(39));
    if (lpt_ack_mainsbreaker3 == 1) printf(msg(40));
    // MM6D devices
    for (int channel = 0; channel < 8; channel++)
      if (ena_ch[channel] > 0)
      {
        if (in_alarm[channel] == 1)
        {
          lpt_d0_alarm = 1;
          printf("%s%d!\n", msg(42), channel+1);
          printf("%s%d!%s\n", msg(63), channel+1, msg(0));
          if (restoreMM6Dalarm(channel) == 0)
            printf(msg(4)); else
            printf(msg(5));
        }
        if (in_swmanu[channel] == 1)
        {
          lpt_d5_led_warning = 1;
          printf("%s%d.\n", msg(43));
        }
        if (in_ocprot[channel] == 1)
        {
          lpt_d6_led_error = 1;
          printf("%s%d!\n", msg(41));
        }
      }
    } else
    {
    for (int channel = 0; channel < 8; channel++)
      if (ena_ch[channel] > 0)
      {
        printf("  CH #%d:\n", channel+1);
        printf("%s%d %sC\n", msg(50), in_temperature[channel], DEG);
        printf("%s%d \%\n", msg(51), in_humidity[channel]);
        printf("%s%d \%\n", msg(52), in_gasconcentrate[channel]);
        if (in_opmode[channel] == 0)
        {
          // growing mushroom
          printf(msg(48));
          // bad temperature
          if (in_temperature[channel] < m_temperature_min[channel])
            printf("%s%s", msg(58), msg(61));
          if (in_temperature[channel] > m_temperature_max[channel])
            printf("%s%s", msg(58), msg(62));
          // bad humidity
          if (in_humidity[channel] < m_humidity_min[channel])
            printf("%s%s", msg(59), msg(61));
          if (in_humidity[channel] > m_humidity_max[channel])
            printf("%s%s", msg(59), msg(62));
          // bad gas concentrate
          if (in_gasconcentrate[channel] > c_gasconcentrate_max[channel])
            printf(msg(60));
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
          printf(msg(49));
          // bad temperature
          if (in_temperature[channel] < h_temperature_min[channel])
            printf("%s%s", msg(58), msg(61));
          if (in_temperature[channel] > h_temperature_max[channel])
            printf("%s%s", msg(58), msg(62));
          // bad humidity
          if (in_humidity[channel] < h_humidity_min[channel])
            printf("%s%s", msg(59), msg(61));
          if (in_humidity[channel] > h_humidity_max[channel])
            printf("%s%s", msg(59), msg(62));
          // bad gas concentrate
          if (in_gasconcentrate[channel] > c_gasconcentrate_max[channel])
            printf(msg(60));
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
        printf(msg(53));
        if (out_heaters[channel] == 1) printf(msg(56)); else printf(msg(57));
        printf(msg(54));
        if (out_lamps[channel] == 1) printf(msg(56)); else printf(msg(57));
        printf(msg(55));
        if (out_vents[channel] == 1) printf(msg(56)); else printf(msg(57));
        printf("\n");
      }
  }
}

// main function
int server(bool loop)
{
#ifdef AUTOFEED_FOR_POWER
#ifdef __DOS__
  outp(lpt_adr[lpt_prt]+2,0);
#else
  if (ioperm(lpt_adr[lpt_prt]+2,1,1)) outp(lpt_adr[lpt_prt]+2,0);
#endif
#endif
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
  printf("%s%s%s", msg(6), msg(7), msg(0));
  if (loadmainconf(MAINCONFFILE)) printf(msg(4)); else
  {
    printf(msg(5));
    printf(msg(44));
    return 1;
  }
  int ii;
  ii = 0;
  for (int channel = 0; channel < 8; ++channel) ii = ii + ena_ch[channel];
  if (ii == 0)
  {
    printf(msg(18));
    return 2;
  }
  printf("%s%d\n", msg(15), lpt_prt + 1);
  printf(msg(10));
  for (int channel = 0; channel < 8; ++channel)
    if (ena_ch[channel] > 0) printf(" #%d", channel+1);
  printf("\n");
  for (int channel = 0; channel < 8; ++channel)
  {
    if (ena_ch[channel] > 0) printf("%s%d: %s\n", msg(16), channel+1, addr_mm6dch[channel]);
    if (ena_ch[channel] > 0) printf("%s%d: %s\n", msg(17), channel+1, addr_mm6dch[channel]);
  }
  // checking version of remote devices
  printf(msg(21));
  for (int channel = 0; channel < 8; ++channel)
    if (ena_ch[channel] > 0)
    {
      printf("%s %d: ", msg(22), channel+1);
      if (getcontrollerversion(6,channel))
      {
        if (mv*10+sv < COMPMV6*10+COMPSV6)
        {
          printf(msg(64));
          ena_ch[channel] = 0;
        } else printf("v%d.%d\n", mv, sv);
      } else
      {
          printf(msg(24));
          ena_ch[channel] = 0;
      }
    }
  for (int channel = 0; channel < 8; ++channel)
    if (ena_ch[channel] > 0)
    {
      printf("%s %d: ", msg(23), channel+1);
      if (getcontrollerversion(7,channel))
      {
        if (mv*10+sv < COMPMV6*10+COMPSV6)
        {
          printf(msg(64));
          ena_ch[channel] = 0;
        } else printf("v%d.%d\n", mv, sv);
      } else
      {
          printf(msg(24));
          ena_ch[channel] = 0;
      }
    }
  // check number of enabled channels
  ii = 0;
  for (int channel = 0; channel < 8; ++channel) ii = ii + ena_ch[channel];
  if (ii == 0)
  {
    printf(msg(18));
    exit(2);
  }
  // load environment parameter settings
  printf("%s%s%s", msg(6), msg(8), msg(0));
  for (int channel = 0; channel < 8; ++channel)
    if (ena_ch[channel] > 0)
      if (! loadenvirconf(ENVCONFDIR,channel+1))
      {
        printf(msg(5));
        printf("%s%d!\n", msg(45), channel+1);
        return 3;
       }
  printf(msg(4));
  // set local ports to default state
  printf("%s%s", msg(9), msg(0));
  if (resetlocalports()) printf(msg(4)); else
  {
    printf(msg(5));
    printf(msg(46));
    return 4;
  }
  // *** start loop ***
  printf(msg(20));
  do
  {
    printf(msg(35));
    // section #1:
    // read data from local port
    printf("%s%s", msg(29), msg(0));
    if (readlocalports()) printf(msg(4)); else
    {
      printf(msg(5));
      printf(msg(46));
    }
    // analise data
    printf("%s%s\n", msg(31), msg(0));
    analise(1);
    // write data to local port
    printf("%s%s", msg(30), msg(0));
    if (writelocalports()) printf(msg(4)); else
    {
      printf(msg(5));
      printf(msg(46));
    }
    // set outputs of MM6D
    for (int channel = 0; channel < 8; channel++)
      if (ena_ch[channel] > 0)
      {
        printf("%s%d%s", msg(34), channel+1, msg(0));
        if (readwriteMM6Ddevice(channel,1))
          printf(msg(4)); else
          printf(msg(5));
   }
    // section #2:
    // get parameters of air from MM7Ds
    for (int channel = 0; channel < 8; channel++)
      if (ena_ch[channel] > 0)
      {
        printf("%s%d%s", msg(32), channel+1, msg(0));
        if (readwriteMM7Ddevice(channel))
          printf(msg(4)); else
          printf(msg(5));
      }
    // analise data
    printf("%s%s\n", msg(31), msg(0));
    analise(2);
    printf(msg(36));
#ifdef __DOS__
    if (loop) delay(1000 * DELAY);
#else
    if (loop) usleep(1000000 * DELAY);
#endif
  } while (loop);
  // *** stop loop ***
  printf(msg(14));
  // set local ports to default state
  printf("%s%s", msg(9), msg(0));
  if (resetlocalports()) printf(msg(4)); else
  {
    printf(msg(5));
    printf(msg(46));
  }
  // set remote devices to default state
  for (int channel = 0; channel < 8; channel++)
    if (ena_ch[channel] > 0)
    {
      printf("%s%d to OFF%s", msg(34), channel+1, msg(0));
      if (resetMM6Ddevice(channel))
        printf(msg(4)); else
        printf(msg(5));
    }
  // exit to OS
  remove(TEMPFILE);
  return 0;
}
