#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <set>
#include <algorithm>

const uint32_t CLUSTER_SIZE = (16 * 1024);
#define FT_COUNT 1
#define CLUSTER_FILE_COUNT 2

#define BIG_ENDIAN 1
#define LIT_ENDIAN !BIG_ENDIAN

#define VERSION "1.0.0"

#define FIRST_ELEM_OFFSET 0x20

#define ITS_FOLDER 1
#define ITS_FILE 0

#define WO_MODE 0
#define RO_MODE 1

#define DEBUG 1

const uint32_t end_data_block = 0xffffffff;
const uint32_t next_clast_offset = 0x01000000;