
#ifndef PLUS4_SAVER_H_
#define PLUS4_SAVER_H_

void save_generic_plus4_crt(unsigned int length, unsigned int banks, unsigned int address, unsigned int type, unsigned char game, unsigned char exrom);
void save_multicart_crt(unsigned int length, unsigned int banks, unsigned int address, unsigned int type, unsigned char game, unsigned char exrom);

#endif
