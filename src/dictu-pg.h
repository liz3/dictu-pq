#ifndef DICTU_PG_H
#define DICTU_PG_H

#include <dictu-include.h>
#include <libpq-fe.h>

#define PG_EPOCH_IN_UNIX 946684800
#define BASE_YEAR 2000
#define BASE_MONTH 1
#define BASE_DAY 1

#ifdef __APPLE__
// https://gist.github.com/yinyin/2027912

#include <libkern/OSByteOrder.h>

#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)

#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)

#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)

#endif 
typedef struct {
  PGconn* conn;
  char* connection_string;
  bool connected;
} DictuPqClient;

#define AS_PQ_CLIENT(v) ((DictuPqClient *)AS_ABSTRACT(v)->data)

static Value dictuPqClientCreateClient(DictuVM *vm, int argCount, Value *args);
static Value dictuPqClientConnect(DictuVM *vm, int argCount, Value *args);
static Value dictuPqClientExecute(DictuVM *vm, int argCount, Value *args);
#endif