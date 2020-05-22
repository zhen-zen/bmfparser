// SPDX-License-Identifier: GPL-2.0-only
//
//  main.hpp
//  bmfparser
//
//  Created by Zhen on 2020/5/21.
//  Copyright © 2020 zhen. All rights reserved.
//

#ifndef main_h
#define main_h

//#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <uuid/uuid.h>

#define header "FOMB"
#define footer "BMOFQUALFLAVOR11"

#undef DEBUG

#ifndef KERNEL
#define error(str) do { fprintf(stderr, "error %s at %s:%d\n", str, __func__, __LINE__); exit(1); } while (0)
#else
#define error(str) do { IOLog("error %s at %s:%d\n", str, __func__, __LINE__); } while (0)
#endif

#ifdef DEBUG
#define DebugLog(name, value) do { std::cout << std::string(indent, '\t') << name << value << std::endl; } while (0)
#else
#define DebugLog(name, value) do { } while (0)
#endif

enum mof_offset_type {
  MOF_OFFSET_UNKNOWN,
  MOF_OFFSET_BOOLEAN = 0x01,
  MOF_OFFSET_OBJECT = 0x02,
  MOF_OFFSET_STRING = 0x03,
  MOF_OFFSET_SINT32 = 0x11,
};


enum mof_data_type {
  MOF_UNKNOWN,
  MOF_SINT16 = 0x02, // Unused
  MOF_SINT32 = 0x03,
  MOF_STRING = 0x08,
  MOF_BOOLEAN = 0x0B,
  MOF_OBJECT = 0x0D,
  MOF_SINT8 = 0x10, // Unused
  MOF_UINT8 = 0x11, // Unused
  MOF_UINT16 = 0x12, // Unused
  MOF_UINT32 = 0x13, // Unused
  MOF_SINT64 = 0x14, // Unused
  MOF_UINT64 = 0x15, // Unused
  MOF_DATETIME = 0x65, // Unused
};

class MOF {
    
public:
    MOF(char *data, uint32_t size) {buf = data; this->size = size;};
    MOF(const char filename[]);
    bool parse_bmf();
private:
    char *parse_string(char *buf, uint32_t size);
    uint16_t parse_valuemap(wchar_t *buf, bool map, uint32_t i);
    uint32_t parse_valuemap(int32_t *buf, bool map, uint32_t i);
    char to_ascii(char c);
    void dump_bytes(char *buf, uint32_t size);
    
    bool parse_class(uint32_t *buf);
    bool parse_method(uint32_t *buf, uint32_t verify = 0);
//    bool parse_container(uint32_t *buf);
//    bool parse_item(uint32_t *buf, uint32_t verify = 0);

    int indent;

    char * buf;
    char ** valuemap;
    uint32_t size;
};

#endif /* main_h */
