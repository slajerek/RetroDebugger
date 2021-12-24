// BME IO module header file

int io_open(char *name);
int io_lseek(int handle, int bytes, int whence);
int io_read(int handle, void *buffer, int size);
void io_close(int handle);
int io_opendatafile(char *name);
int io_openlinkeddatafile(unsigned char *ptr);
void io_setfilemode(int usedf);
unsigned io_read8(int handle);
unsigned io_readle16(int handle);
unsigned io_readle32(int handle);
unsigned io_readhe16(int handle);
unsigned io_readhe32(int handle);
