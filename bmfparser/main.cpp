// SPDX-License-Identifier: GPL-2.0-only
//
//  main.cpp
//  bmfparser
//
//  Created by Zhen on 2020/5/21.
//  Copyright © 2020 zhen. All rights reserved.
//

#include "main.hpp"

#define defaultfilename "WQDC.dec"

MOF::MOF(const char filename[])
{
    std::ifstream input (filename, std::ios::binary | std::ios::ate);
        
    if (input.is_open())
    {
        size = (uint32_t)input.tellg();
        
        std::cout << "length: " << size << " " << std::hex << size << std::endl;
        
        buf = new char[size];

        input.seekg (0, std::ios::beg);
        input.read(buf, size);
        
        if (input)
          std::cout << "all bytes read successfully.\n";
        else
          std::cout << "error: only " << input.gcount() << " could be read\n";
        input.close();
    }
    else
        error("Open file failure");
}

char *MOF::parse_string(char *buf, uint32_t size) {
  uint16_t *buf2 = (uint16_t *)buf;
  if (size % 2 != 0) error("Invalid size");
//  char *out = (char *)malloc(size+1);
//  if (!out) error("malloc failed");
  char *out = new char[size+1];
  uint32_t i, j;
  for (i=0, j=0; i<size/2; ++i) {
    if (buf2[i] == 0) {
      break;
    } else if (buf2[i] < 0x80) {
      out[j++] = buf2[i];
    } else if (buf2[i] < 0x800) {
      out[j++] = 0xC0 | (buf2[i] >> 6);
      out[j++] = 0x80 | (buf2[i] & 0x3F);
    } else if (buf2[i] >= 0xD800 && buf2[i] <= 0xDBFF && i+1 < size/2 && buf2[i+1] >= 0xDC00 && buf2[i+1] <= 0xDFFF) {
      uint32_t c = 0x10000 + ((buf2[i] - 0xD800) << 10) + (buf2[i+1] - 0xDC00);
      ++i;
      out[j++] = 0xF0 | (c >> 18);
      out[j++] = 0x80 | ((c >> 12) & 0x3F);
      out[j++] = 0x80 | ((c >> 6) & 0x3F);
      out[j++] = 0x80 | (c & 0x3F);
    } else if (buf2[i] >= 0xD800 && buf2[i] <= 0xDBFF && i+1 < size/2 && buf2[i+1] >= 0xDC00 && buf2[i+1] <= 0xDFFF) {
      uint32_t c = 0x10000 + ((buf2[i] - 0xD800) << 10) + (buf2[i+1] - 0xDC00);
      ++i;
      out[j++] = 0xF0 | (c >> 18);
      out[j++] = 0x80 | ((c >> 12) & 0x3F);
      out[j++] = 0x80 | ((c >> 6) & 0x3F);
      out[j++] = 0x80 | (c & 0x3F);
    } else {
      out[j++] = 0xE0 | (buf2[i] >> 12);
      out[j++] = 0x80 | ((buf2[i] >> 6) & 0x3F);
      out[j++] = 0x80 | (buf2[i] & 0x3F);
    }
  }
  out[j] = 0;
  return out;
}

char MOF::to_ascii(char c) {
  if (c >= 32 && c <= 126)
    return c;
  return '.';
}

void MOF::dump_bytes(char *buf, uint32_t size) {
  if (size>0x3ff) return;
  uint32_t i, ascii_cnt = 0;
  char ascii[17] = { 0, };
  for (i=0; i<size; i++) {
    if (i % 16 == 0) {
      if (i != 0) {
        fprintf(stderr, "  |%s|\n", ascii);
        ascii[0] = 0;
        ascii_cnt = 0;
      }
      fprintf(stderr, "%04X:", (unsigned int)i);
    }
    fprintf(stderr, " %02X", buf[i] & 0xFF);
    ascii[ascii_cnt] = to_ascii(buf[i]);
    ascii[ascii_cnt + 1] = 0;
    ascii_cnt++;
  }
  if (ascii[0]) {
    if (size % 16)
      for (i=0; i<16-(size%16); i++)
        fprintf(stderr, "   ");
    fprintf(stderr, "  |%s|\n", ascii);
  }
}

uint16_t MOF::parse_valuemap(wchar_t *buf, bool map, uint32_t i) {
    uint16_t * nbuf = (uint16_t *)buf;
    uint32_t len = 0;
    while (nbuf[len] != 0 && len < 0x99)
        len++;
    if (map)
        valuemap[i] = parse_string((char *)nbuf, len*2);
    else
        std::cout << std::string(indent, '\t') << valuemap[i] << ":" << parse_string((char *)nbuf, len*2) << std::endl;
    return len+1;
}

uint32_t MOF::parse_valuemap(int32_t *buf, bool map, uint32_t i) {
    if (map) {
        char * res = new char[10];
        snprintf(res, 10, "%d", buf[0]);
        valuemap[i] = res;
    }
    else
        std::cout << std::string(indent, '\t') << valuemap[i] << ":" << buf[0] << std::endl;
    return 1;
}

/*
 *    0                   1                   2                   3
 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |         Item  length          |          Item  type           |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |        Pattern  0x00          |     Name  length  (single)    |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |  Name  length  (array) / none |            Name               |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |                                                               |
 *   |                Value / Qualifiers / Parameter                 |
 *   |                                                               |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

bool MOF::parse_method(uint32_t *buf, uint32_t verify) {
    indent +=1;
    DebugLog("len1\t", buf[0]);
    uint8_t *type = (uint8_t *)buf + 4;

    switch (verify) {
        case 0:
            break;
            
        case MOF_OFFSET_BOOLEAN:
            if (type[0] == MOF_BOOLEAN) break;
            
        case MOF_OFFSET_OBJECT:
//            if (type[0] != MOF_OBJECT)
//                std::cout << std::string(indent-1, '\t') << "MOF_OBJECT" << std::endl;
            // can't verify "object:" of upper item
            break;
            
        case MOF_OFFSET_STRING:
            if (type[0] == MOF_STRING) break;
            
        case MOF_OFFSET_SINT32:
            if (type[0] == MOF_SINT32) break;
            
        default:
            std::cout << std::string(indent-1, '\t') << "(UNKNOWN) type mismatch" << std::endl;
            break;
    }

    std::cout << std::string(indent, '\t') << "type\t" << buf[1];
    switch (type[0]) {
        case MOF_BOOLEAN:
            std::cout << "(BOOLEAN";
            break;
            
        case MOF_STRING:
            std::cout << "(STRING";
            break;
            
        case MOF_SINT32:
            std::cout << "(SINT32";
            break;
            
        case MOF_OBJECT:
            std::cout << "(OBJECT";
            break;
            
        case MOF_UINT8:
            std::cout << "(UINT8";
            break;
            
        case MOF_UINT32:
            std::cout << "(UINT32";
            break;
            
        default:
            std::cout << "(UNKNOWN";
            break;
    }
    
    switch (type[1]) {
        case 0:
            std::cout << ")" << std::endl;
            break;
            
        case 0x20:
            std::cout << ", MAP)" << std::endl;
            break;
            
        default:
            assert(false);
            break;
    }

    assert(buf[2] == 0);
    uint32_t nlen = buf[3];
    uint32_t clen = buf[4];
    uint32_t *nbuf = buf + (buf[4] != 0xFFFFFFFF && buf[4] > 0xFFFF ? 4:5);

    std::cout << std::string(indent, '\t') << "value\t";

    // Variables or Qualifiers
    if (type[0] != MOF_OBJECT | type[1] != 0x20) {
        // Variable map or objects
        if (nlen == 0xFFFFFFFF) {
            std::cout << parse_string((char *)nbuf, clen) << std::endl;
            nbuf = (uint32_t *)((char *)nbuf + clen);
            DebugLog("len1\t", nbuf[0]);
            DebugLog("count\t", nbuf[1]);
            uint32_t count = nbuf[1];
            nbuf += 2;
            assert(count < 0xff);
            for (uint32_t i=0; i<count; i++) {
                std::cout << std::string(indent, '\t') << "vqualifier" << i << std::endl;
//                parse_item(nbuf, 1);
                parse_method(nbuf);
                nbuf = (uint32_t *)((char *)nbuf + nbuf[0]);
            }
        }
        else
        {
            std::cout << parse_string((char *)nbuf, nlen) << "\t";

            switch (verify) {
                case 0:
                    break;

                case MOF_OFFSET_BOOLEAN:
                    if (strcasecmp(parse_string((char *)nbuf, nlen), "Dynamic")!= 0)
                            std::cout << "!";
                    break;

                case MOF_OFFSET_STRING:
                    if (strcmp(parse_string((char *)nbuf, nlen), "CIMTYPE") != 0)
                            std::cout << "!";
                    // can't verify "object:" of upper item
                    break;

                case MOF_OFFSET_OBJECT:
                    break;

                case MOF_OFFSET_SINT32:
                    if (strcmp(parse_string((char *)nbuf, nlen), "ID") != 0)
                            std::cout << "!";
                    break;

                default:
                    std::cout << "?";
                    break;
            }
            std::cout << ":";

            if (clen == 0xFFFFFFFF)
                clen = buf[0]-0x14-nlen;
            else if (clen > 0xFFFF)
                clen = buf[0]-0x10-nlen;
            else {
//                std::cout << clen << std::endl;
                assert(clen == buf[0]-0x1c);
                clen -= nlen;
            }

            if (nbuf[0] == 0x00750067 && nbuf[1] == 0x00640069 && nbuf[2] == 0x00000000)
            {
                char guid_string[37];
                uuid_t guid_t;
                int result;
                switch (clen) {
                    case 0x50:
                        strncpy(guid_string, parse_string((char *)nbuf+nlen, clen)+1, 37);
                        guid_string[36] = 0;
                        result = uuid_parse(guid_string, guid_t);
                        uuid_unparse_lower(guid_t, guid_string);
//                        std::cout << result << " quoted "  << guid_string << std::endl;
                        std::cout << guid_string << std::endl;
                        break;

                    case 0x4C:
                        result = uuid_parse(parse_string((char *)nbuf+nlen, clen) , guid_t);
                        uuid_unparse_lower(guid_t, guid_string);
//                        std::cout << result << "plain" << std::endl;
                        std::cout << guid_string << std::endl;
                        break;

                    default:
                        std::cout << clen << "invalid" << std::endl;
                        break;
                }
                indent --;
                return true;
            }

            nbuf = (uint32_t *)((char *)nbuf + nlen);

            // ValueMap
            if (type[1] == 0x20)
            {
                std::cout << std::endl;
                assert(nbuf[0] == clen);
                assert(nbuf[1] == 1);
                assert(nbuf[3] == clen-0xc);
                uint32_t count = nbuf[2];
                assert(count < 0xff);
                nbuf +=4;
                if (!verify) {
                    bool map;
                    if (strcasecmp(parse_string((char *)(nbuf-4)-nlen, nlen), "ValueMap") == 0)
                        map = true;
                    else if (strcasecmp(parse_string((char *)(nbuf-4)-nlen, nlen), "Values") == 0)
                        map = false;
                    else
                    {
                        std::cout << std::endl << "Invalid ValueMap!" << std::endl;
                        return false;
                    }
                    if (map)
                        valuemap = new char*[count];
                    for (uint32_t i=0; i<count; i++) {
                        switch (type[0]) {
                            case MOF_STRING:
                                nbuf = (uint32_t *)((uint16_t *)nbuf + parse_valuemap((wchar_t *)nbuf, map, i));
                                break;
                            case MOF_SINT32:
                                nbuf += parse_valuemap((int32_t *)nbuf, map, i);
                                break;
                            default:
                                break;
                        }
                    }
                    if (!map)
                        delete valuemap;
                }
            }
            else {
                switch (type[0]) {
                    case MOF_BOOLEAN:
                        if (clen != 4 && clen != 2)
                        {
                            dump_bytes((char *)nbuf, clen);
                            assert(false);
                        }

                        switch (clen == 4 ? nbuf[0] : nbuf[0] & 0xFFFF) {
                            case 0:
                                std::cout << "False" << std::endl;
                                break;
                                
                            case 0xFFFF:
                                std::cout << "True" << std::endl;
                                break;
                                
                            default:
                                std::cout << "Invalid boolean: " << nbuf[0] << std::endl;
                                dump_bytes((char *)nbuf, clen);
                                assert(false);
                                break;
                        }
                        break;

                    case MOF_STRING:
                        std::cout << parse_string((char *)nbuf, clen) << std::endl;
                        break;

                    case MOF_SINT32:
                        std::cout << ((int32_t *)nbuf)[0] << std::endl;
                        assert(clen == 4);
                        break;

                    default:
                        std::cout << "Unknown type\t" << buf[1] << std::endl;
                        dump_bytes((char *)nbuf, clen);
                        assert(false);
                        break;
                }
            }
        }
    }
    // Method, or just a class with name?
    else
    {
        std::cout << parse_string((char *)(buf+5), nlen) << std::endl;
        nbuf = (uint32_t *)((char *)nbuf + nlen);
        DebugLog("len1\t", nbuf[0]);

        assert(nbuf[1] == 1);

        uint32_t count = nbuf[2];
        DebugLog("pcount\t", count);
        assert(count < 0xff);

        nbuf+=4;
        for (uint32_t i=0; i<count; i++) {
            parse_class(nbuf);
            nbuf = (uint32_t *)((char *)nbuf + nbuf[0]);
        }
        count = nbuf[1];
        nbuf +=2;
        for (uint32_t i=0; i<count; i++) {
            std::cout << std::string(indent, '\t') << "vqualifier" << i << std::endl;
            parse_method(nbuf);
            nbuf = (uint32_t *)((char *)nbuf + nbuf[0]);
        }
    }
    indent -=1;
    return true;
}

//bool MOF::parse_item(uint32_t *buf, uint32_t verify) {
////    indent ++;
//    uint32_t size = buf[0];
//    uint8_t *type = (uint8_t *)(buf+1);
//    uint32_t pattern = buf[2];
//    uint32_t name_len = buf[3];
//
//    if (type[0] == 0x0d && type[1] == 0x20) // 200d -> method:3
//    {
//        if (verify == 3)
//            return true;
//        std::cout << 3;
//    }
//    else
//    {
//        uint8_t *ftr = (uint8_t *)(buf + 4);
//        if (ftr[0] != 0 && ftr[1] == 0 && ftr[2] != 0 && ftr[3] == 0 ) {
//            if (verify == 1)
//                return true;
//            std::cout << 1;
//        } else {
//            if (verify == 2)
//                return true;
//            std::cout << 2;
//        }
//    }
//    std::cout << "verify" << verify << std::endl;
//    dump_bytes((char *)buf, size);
////    indent --;
//    return false;
//}
/*
*    0                   1                   2                   3
*    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |         Class  length         |          Class  type          |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |       Qualifier  length       |         Total  length         |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |          Pattern  0/1         |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |      Qualifiers  length       |      Qualifiers  count        |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |                                                               |
*   |                           Qualifiers                          |
*   |                                                               |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |       Variables  length       |       Variables  count        |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |                                                               |
*   |                           Variables                           |
*   |                                                               |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |        Methods  length        |        Methods  count         |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |                                                               |
*   |                            Methods                            |
*   |                                                               |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

bool MOF::parse_class(uint32_t *buf) {
    indent +=1;
        
    switch (buf[1]) {
        case 0:
            std::cout << std::string(indent-1, '\t') << "class" << std::endl;
            break;
            
        case 0xFFFFFFFF:
            std::cout << std::string(indent-1, '\t') << "parameter" << std::endl;
            break;
            
        default:
            assert(false);
            break;
    }

    uint32_t count;
    uint32_t * obuf = buf;
    DebugLog("length\t", buf[0]);
    if (buf[1])
        assert(buf[2] == 0);
    else
        DebugLog("len1\t", buf[2]); // 0:parameter or qlen
    DebugLog("len\t\t", buf[3]);
    assert(buf[4] == 0 | buf[4] == 1); // 0:class 1:parameter
    
    buf += 5;
    
    if (!obuf[1]) {
        DebugLog("qlen\t", buf[0]);
        count = buf[1];
        DebugLog("qcount\t", count);
        assert(count < 0xff);
        buf+=2;
        for (uint32_t i=0; i<count; i++) {
            std::cout << std::string(indent, '\t') << "qualifier" << i <<std::endl;
    //        parse_item(buf, (obuf[1] ? 2 : 1));
            parse_method(buf);
            buf = (uint32_t *)((char *)buf + buf[0]);
        }
    }

    DebugLog("vlen\t", buf[0]);
    count = buf[1];
    DebugLog("vcount\t", count);
    assert(count < 0xff);
    buf+=2;
    for (uint32_t i=0; i<count; i++) {
        std::cout << std::string(indent, '\t') << "variable" << i <<std::endl;
//        parse_item(buf, 2);
        parse_method(buf);
        buf = (uint32_t *)((char *)buf + buf[0]);
    }
    count = buf[1];
    DebugLog("mlen\t", buf[0]);
    DebugLog("mcount\t", count);
    assert(count < 0xff);
    buf+=2;
    for (uint32_t i=0; i<count; i++) {
        std::cout << std::string(indent, '\t') << (obuf[1] ? "mqualifier" : "method") << i <<std::endl;
//        parse_item(buf, (obuf[1] ? 1 : 3));
        parse_method(buf);
        buf = (uint32_t *)((char *)buf + buf[0]);
    }
    indent -=1;
    return true;
}

//bool MOF::parse_container(uint32_t *buf) {
////    indent ++;
//    uint32_t size = buf[0];
//    uint32_t type = buf[1]; // 0:class 0xFFFFFFFF:parameter
//    uint32_t pattern = buf[2];
//    uint32_t name_len = buf[3];
//
////    indent --;
//    return true;
//}
//
/*
*    0                   1                   2                   3
*    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |         Pattern  FOMB         |          BMF  length          |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |         Pattern  0x1          |         Pattern  0x1          |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |        Classes  count         |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |                                                               |
*   |                            Classes                            |
*   |                                                               |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |         Pattern  BMOF         |         Pattern  QUAL         |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |         Pattern  FLAV         |         Pattern  OR11         |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |         Offsets  count        |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |                               |                               |
*   |         Offset  address       |          Offset  type         |
*   |                               |                               |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

bool MOF::parse_bmf() {
    indent = 0;
    uint32_t *nbuf = (uint32_t *)buf;
    
    std::cout << std::string(indent, '\t');
    std::cout.write((char *)nbuf, 4) << std::endl; // FOMB
    assert(nbuf[0] == 0x424D4F46);
    DebugLog("len\t", nbuf[1]);
    assert(nbuf[2] == 1);
    assert(nbuf[3] == 1);
    uint32_t count = nbuf[4];

    nbuf+=5;
    assert(count < 0xff);
    for (uint32_t i=0; i<count; i++) {
        std::cout << std::string(indent, '\t') <<  i  << " ";
        parse_class(nbuf);
        nbuf = (uint32_t *)((char *)nbuf + nbuf[0]);
    }
    std::cout.write((char *)nbuf, 16) << std::endl; // BMOFQUALFLAVOR11
    assert(nbuf[0] == 0x464F4D42 && nbuf[1] == 0x4C415551 && nbuf[2] == 0x56414C46 && nbuf[3] == 0x3131524F);
    
    count = nbuf[4];
    nbuf += 5;
    DebugLog("ocount\t", count);
    assert(count < 0x1ff);
    for (uint32_t i=0; i<count; i++) {
        std::cout << std::string(indent, '\t') <<  "offset" << i << "@" << nbuf[0] << " type " << nbuf[1];
        switch (nbuf[1]) {
            case MOF_OFFSET_BOOLEAN:
                std::cout << "(BOOLEAN)";
                break;
                
            case MOF_OFFSET_OBJECT:
                std::cout << "(OBJECT, tosubclass)";
                break;
                
            case MOF_OFFSET_STRING:
                std::cout << "(STRING)";
                break;
                
            case MOF_OFFSET_SINT32:
                std::cout << "(SINT32)";
                break;
                
            default:
                std::cout << "(UNKNOWN)";
                break;
        }
        std::cout<< std::endl;
//        parse_item((uint32_t *)(buf+nbuf[0]), 1);
        parse_method((uint32_t *)(buf+nbuf[0]), nbuf[1]);
        nbuf+=2;
    }
    return true;
}

int main(int argc, const char * argv[]) {

    if (argc != 2) {
        std::cerr << "Invalid argument! Usage: ./bmfparser bmf.dec" << std::endl;
        exit(-1);
    }

    MOF mof(argc == 2 ? argv[1] : defaultfilename);
    mof.parse_bmf();
    
    return 0;
}
