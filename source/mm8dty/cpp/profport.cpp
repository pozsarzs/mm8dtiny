/***************************************************************************
 PORTABLE ROUTINES FOR WRITING PRIVATE PROFILE STRINGS --  by Joseph J. Graf
 Header file containing prototypes and compile-time configuration.
***************************************************************************/

// source: Multiplatform .INI Files By Joseph J. Graf, March 01, 1994
// https://www.drdobbs.com/architecture-and-design/multiplatform-ini-files/184409195
//
// Thanks! :-)

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH    80

int read_line(FILE *fp, char *bp)
{
  char c = '\0';
  int i = 0;
  while ((c = getc(fp)) != '\n')
  {
    if (c == EOF) return(0);
    bp[i++] = c;
  }
  bp[i] = '\0';
  return(1);
}

int get_private_profile_int(char *section, char *entry, int def, char *file_name)
{
  FILE *fp = fopen(file_name,"r");
  char buff[MAX_LINE_LENGTH];
  char *ep;
  char t_section[MAX_LINE_LENGTH];
  char value[6];
  int len = strlen(entry);
  int i;
  if (!fp) return(0);
  sprintf(t_section,"[%s]",section);
  do
  {
    if (!read_line(fp,buff))
    {
      fclose(fp);
      return(def);
    }
  } while (strcmp(buff,t_section));
  do
  {
    if (!read_line(fp,buff) || buff[0] == '\0')
    {
      fclose(fp);
      return(def);
    }
  }  while (strncmp(buff,entry,len));
  ep = strrchr(buff,'=');
  ep++;
  if (!strlen(ep)) return(def);
  for (i = 0; isdigit(ep[i]); i++ ) value[i] = ep[i];
  value[i] = '\0';
  fclose(fp);
  return(atoi(value));
}

int get_private_profile_string(char *section, char *entry, char *def, char *buffer, int buffer_len, char *file_name)
{
  FILE *fp = fopen(file_name,"r");
  char buff[MAX_LINE_LENGTH];
  char *ep;
  char t_section[MAX_LINE_LENGTH];
  int len = strlen(entry);
  if (!fp) return(0);
  sprintf(t_section,"[%s]",section);
  do
  {
    if (!read_line(fp,buff))
    {
      fclose(fp);
      strncpy(buffer,def,buffer_len);
      return(strlen(buffer));
    }
  } while (strcmp(buff,t_section));
  do
  {
    if (!read_line(fp,buff) || buff[0] == '\0')
    {
      fclose(fp);
      strncpy(buffer,def,buffer_len);
      return(strlen(buffer));
    }
  } while (strncmp(buff,entry,len));
  ep = strrchr(buff,'=');
  ep++;
  strncpy(buffer,ep,buffer_len - 1);
  buffer[buffer_len] = '\0';
  fclose(fp);
  return(strlen(buffer));
}
