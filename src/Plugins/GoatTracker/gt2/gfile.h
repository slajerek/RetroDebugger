#ifndef GFILE_H
#define GFILE_H

#define MAX_DIRFILES 16384
#define MAX_FILENAME 60
#define MAX_PATHNAME 256

typedef struct
{
  char *name;
  int attribute;
} DIRENTRY;

void initpaths(void);
int fileselector(char *name, char *path, char *filter, char *title, int filemode);
void editstring(char *buffer, int maxlength);
int cmpname(char *string1, char *string2);

#endif

