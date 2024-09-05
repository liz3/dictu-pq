#ifndef DICTU_PG_H
#define DICTU_PG_H

#include <dictu-include.h>
#include "portable_endian.h"
#include <libpq-fe.h>

#define PG_EPOCH_IN_UNIX 946684800
#define BASE_YEAR 2000
#define BASE_MONTH 1
#define BASE_DAY 1

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