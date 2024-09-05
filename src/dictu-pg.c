#include "dictu-pg.h"
#include "dictu-include.h"
#include "libpq-fe.h"
#ifdef _WIN32
#include "server/catalog/pg_type_d.h"
#else
#include "postgresql/server/catalog/pg_type_d.h"
#endif
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

time_t pg_timestamptz_to_unix(int64_t pg_timestamp) {
  // PostgreSQL timestamp is stored as an int64 in microseconds
  // int64_t pg_timestamp;

  // Copy the 8 bytes into the int64_t variable
  // pg_timestamp = ((int64_t)pg_timestamptz_binary[0] << 56) |
  //                ((int64_t)pg_timestamptz_binary[1] << 48) |
  //                ((int64_t)pg_timestamptz_binary[2] << 40) |
  //                ((int64_t)pg_timestamptz_binary[3] << 32) |
  //                ((int64_t)pg_timestamptz_binary[4] << 24) |
  //                ((int64_t)pg_timestamptz_binary[5] << 16) |
  //                ((int64_t)pg_timestamptz_binary[6] <<  8) |
  //                (int64_t)pg_timestamptz_binary[7];

  // Convert PostgreSQL microseconds since 2000 to Unix time in seconds
  time_t unix_time = PG_EPOCH_IN_UNIX + (pg_timestamp / 1000000);

  return unix_time;
}

// Function to convert Unix timestamp to UTC string
void unix_to_utc_string(time_t unix_time, char *buffer, size_t buffer_size) {
  struct tm *tm_info;

  // Convert Unix time to UTC tm structure
  tm_info = gmtime(&unix_time);

  // Format as UTC string
  strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S UTC", tm_info);
}
void uuid_bin_to_str(const unsigned char *bin_uuid, char *str_uuid) {
  sprintf(
      str_uuid,
      "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
      bin_uuid[0], bin_uuid[1], bin_uuid[2], bin_uuid[3], bin_uuid[4],
      bin_uuid[5], bin_uuid[6], bin_uuid[7], bin_uuid[8], bin_uuid[9],
      bin_uuid[10], bin_uuid[11], bin_uuid[12], bin_uuid[13], bin_uuid[14],
      bin_uuid[15]);
}
void convert_days_to_date(int days_since_base, char *date_str, size_t size) {
  struct tm base_date = {0};
  base_date.tm_year = BASE_YEAR - 1900; // tm_year is years since 1900
  base_date.tm_mon = BASE_MONTH - 1;    // tm_mon is months since January
  base_date.tm_mday = BASE_DAY;

  time_t base_time = mktime(&base_date);
  time_t target_time = base_time + (days_since_base * 24 * 60 * 60);
  struct tm *target_tm = localtime(&target_time);
  strftime(date_str, size, "%Y-%m-%d", target_tm);
}

int dictu_ffi_init(DictuVM *vm, Table *method_table) {

  // defineNative(vm, &abstract->values, "shouldClose", dictuUIShouldClose);
  defineNative(vm, method_table, "createClient", dictuPqClientCreateClient);

  return 0;
}

void freeDictuPqClient(DictuVM *vm, ObjAbstract *abstract) {
  // FREE_ARRAY(vm, uint8_t, buffer->bytes, buffer->size);
  if (abstract->data) {
    DictuPqClient *cl = (DictuPqClient *)abstract->data;
    if (cl->conn) {
      PQfinish(cl->conn);
      cl->conn = NULL;
      cl->connected = false;
    }
    if (cl->connection_string) {
      free(cl->connection_string);
      cl->connection_string = NULL;
    }
    FREE(vm, DictuPqClient, abstract->data);
    abstract->data = NULL;
  }
}

char *DictuPqClientToString(ObjAbstract *abstract) {
  UNUSED(abstract);

  char *bufferString = malloc(sizeof(char) * 11);
  snprintf(bufferString, 11, "<PqClient>");
  return bufferString;
}
static Value dictuPqClientConnect(DictuVM *vm, int argCount, Value *args) {
  DictuPqClient *client = AS_PQ_CLIENT(args[0]);
  if (client->connected)
    return NIL_VAL;
  client->conn = PQconnectdb(client->connection_string);
  if (PQstatus(client->conn) != CONNECTION_OK) {
    return BOOL_VAL(false);
  }
  PGresult *res =
      PQexec(client->conn,
             "SELECT pg_catalog.set_config('search_path', 'public', false)");
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    PQclear(res);
    return BOOL_VAL(false);
  }
  PQclear(res);
  client->connected = true;
  return BOOL_VAL(true);
}
static Value dictuPqClientExecute(DictuVM *vm, int argCount, Value *args) {
  DictuPqClient *client = AS_PQ_CLIENT(args[0]);
  if (!client->connected)
    return newResultError(vm, "Client is not connected");
  if (argCount < 1 || !IS_STRING(args[1]))
    return newResultError(vm,
                          "first argument needs to be a SQL statement string");
  ObjString *query_string = AS_STRING(args[1]);
  PGresult *res;

  if (argCount == 2 && IS_LIST(args[2])) {
    ObjList *params = AS_LIST(args[2]);
    int nParams = params->values.count;
    char **values = malloc(sizeof(char *) * nParams);
    for (size_t i = 0; i < nParams; i++)
      values[i] = valueToString(params->values.values[i]);
    res = PQexecParams(client->conn, query_string->chars, nParams, NULL, (const char *const *)values,
                       NULL, NULL, 1);
    for (size_t i = 0; i < nParams; i++)
      free(values[i]);
    free(values);
  } else {
    res = PQexecParams(client->conn, query_string->chars, 0, NULL, NULL, NULL,
                       NULL, 1);
  }
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    PQclear(res);
    char *error = PQerrorMessage(client->conn);
    return newResultError(vm, error);
  }
  ObjDict *d = newDict(vm);
  push(vm, OBJ_VAL(d));
  int nFields = PQnfields(res);
  { // fields
    ObjList *fieldNames = newList(vm);
    push(vm, OBJ_VAL(fieldNames));
    for (size_t i = 0; i < nFields; i++) {
      char *n = PQfname(res, i);
      writeValueArray(vm, &fieldNames->values,
                      OBJ_VAL(copyString(vm, n, strlen(n))));
    }
    pop(vm);
    dictSet(vm, d, OBJ_VAL(copyString(vm, "fields", 6)), OBJ_VAL(fieldNames));
  }
  {
    ObjList *rows = newList(vm);
    push(vm, OBJ_VAL(rows));
    for (size_t i = 0; i < PQntuples(res); i++) {
      ObjList *row = newList(vm);
      push(vm, OBJ_VAL(row));
      for (size_t j = 0; j < nFields; j++) {
        char *result = PQgetvalue(res, i, j);

        if (PQgetisnull(res, i, j)) {
          writeValueArray(vm, &row->values, NIL_VAL);
        } else {
          int byte_len = PQgetlength(res, i, j);
          Oid type = PQftype(res, j);
          if (type == BOOLOID) {
            bool *b = (bool *)result;
            writeValueArray(vm, &row->values, BOOL_VAL(b[0]));
          } else if (type == BYTEAOID || type == CHAROID) {
            writeValueArray(vm, &row->values,
                            type == CHAROID ? OBJ_VAL(copyString(vm, result, 1))
                                            : NUMBER_VAL(result[0]));
          } else if (type == INT8OID) {
            int64_t *ptr = (int64_t *)result;
            writeValueArray(vm, &row->values,
                            NUMBER_VAL((int64_t)be64toh(ptr[0])));
          } else if (type == TIMEOID || type == TIMETZOID) {
            uint64_t *ptr = (uint64_t *)result;
            writeValueArray(vm, &row->values, NUMBER_VAL(be64toh(ptr[0])));
          } else if (type == INT4OID) {
            int32_t *ptr = (int32_t *)result;
            writeValueArray(vm, &row->values,
                            NUMBER_VAL((int32_t)be32toh(ptr[0])));
          } else if (type == INT2OID) {
            int16_t *ptr = (int16_t *)result;
            writeValueArray(vm, &row->values,
                            NUMBER_VAL((int16_t)be16toh(ptr[0])));
          } else if (type == FLOAT4OID) {
            int32_t r = be32toh(*(int32_t *)result);
            float f = *(float *)&r;
            writeValueArray(vm, &row->values, NUMBER_VAL(f));
          } else if (type == FLOAT8OID) {
            int64_t r = be64toh(*(int64_t *)result);
            double f = *(double *)&r;
            writeValueArray(vm, &row->values, NUMBER_VAL(f));
          } else if (type == TIMESTAMPOID || type == TIMESTAMPTZOID) {
            int64_t r = be64toh(*(int64_t *)result);
            time_t unix_time = pg_timestamptz_to_unix(r);
            char utc_string[30];
            unix_to_utc_string(unix_time, utc_string, sizeof(utc_string));
            writeValueArray(
                vm, &row->values,
                OBJ_VAL(copyString(vm, utc_string, strlen(utc_string))));
          } else if (type == UUIDOID) {
            char str_uuid[37];
            uuid_bin_to_str(result, str_uuid);
            writeValueArray(vm, &row->values,
                            OBJ_VAL(copyString(vm, str_uuid, 36)));
          } else if (type == DATEOID) {
            int32_t ptr = be32toh(*(int32_t *)result);
            char date_str[11];

            convert_days_to_date(ptr, date_str, sizeof(date_str));

            writeValueArray(
                vm, &row->values,
                OBJ_VAL(copyString(vm, date_str, strlen(date_str))));
          } else {
            writeValueArray(vm, &row->values,
                            OBJ_VAL(copyString(vm, result, byte_len)));
          }
        }
      }
      pop(vm);
      writeValueArray(vm, &rows->values, OBJ_VAL(row));
    }
    pop(vm);
    dictSet(vm, d, OBJ_VAL(copyString(vm, "result", 6)), OBJ_VAL(rows));
  }
  pop(vm);
  PQclear(res);
  return newResultSuccess(vm, OBJ_VAL(d));
}
static Value dictuPqClientClose(DictuVM *vm, int argCount, Value *args) {
  DictuPqClient *client = AS_PQ_CLIENT(args[0]);
  if (client->connected) {
    PQfinish(client->conn);
    client->conn = NULL;
    client->connected = false;
  }

  return NIL_VAL;
}
static Value dictuPqClientCreateClient(DictuVM *vm, int argCount, Value *args) {

  ObjAbstract *abstract =
      newAbstract(vm, freeDictuPqClient, DictuPqClientToString);
  push(vm, OBJ_VAL(abstract));
  DictuPqClient *client = ALLOCATE(vm, DictuPqClient, 1);
  memset(client, 0, sizeof(DictuPqClient));
  if (argCount == 1 && IS_STRING(args[0])) {
    ObjString *connection_str = AS_STRING(args[0]);
    client->connection_string = malloc(connection_str->length + 1);
    memcpy(client->connection_string, connection_str->chars,
           connection_str->length);
    client->connection_string[connection_str->length] = 0x00;
  }
  defineNative(vm, &abstract->values, "connect", dictuPqClientConnect);
  defineNative(vm, &abstract->values, "exec", dictuPqClientExecute);
  abstract->data = client;
  pop(vm);
  return OBJ_VAL(abstract);
}