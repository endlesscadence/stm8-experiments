#ifndef _HEXFILE_H_
#define _HEXFILE_H_

// read next line from RAM buffer
char *get_line(char **buf, char *line);

// read hexfile into memory buffer
void load_hexfile(const char *filename, char *buf, uint32_t bufsize);

// convert s19 format in memory buffer to memory image
void convert_s19(char *buf, uint32_t *addrStart, uint32_t *numBytes, char *image);

// convert intel hex format in memory buffer to memory image
void convert_hex(char *buf, uint32_t *addrStart, uint32_t *numBytes, char *image);

#endif // _HEXFILE_H_

