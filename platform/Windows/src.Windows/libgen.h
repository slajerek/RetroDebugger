#ifndef LIBGEN_H
#define LIBGEN_H

char *dirname(char *path);
char *basename(char *path);
char *realpath(const char *path, char *resolved_path);

#endif /* LIBGEN_H */
