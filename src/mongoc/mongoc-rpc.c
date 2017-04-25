/*
 * Copyright 2013 MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <bson.h>

#include "mongoc.h"
#include "mongoc-rpc-private.h"
#include "mongoc-trace-private.h"
#include "mongoc-util-private.h"
#include "mongoc-compression-private.h"
#include "mongoc-cluster-private.h"


#define RPC(_name, _code)                                               \
   static void _mongoc_rpc_gather_##_name (mongoc_rpc_##_name##_t *rpc, \
                                           mongoc_rpc_header_t *header, \
                                           mongoc_array_t *array)       \
   {                                                                    \
      mongoc_iovec_t iov;                                               \
      BSON_ASSERT (rpc);                                                \
      BSON_ASSERT (array);                                              \
      header->msg_len = 0;                                              \
      _code                                                             \
   }
#define UINT8_FIELD(_name)                   \
   iov.iov_base = (void *) &rpc->_name;      \
   iov.iov_len = 1;                          \
   header->msg_len += (int32_t) iov.iov_len; \
   _mongoc_array_append_val (array, iov);
#define INT32_FIELD(_name)                   \
   iov.iov_base = (void *) &rpc->_name;      \
   iov.iov_len = 4;                          \
   header->msg_len += (int32_t) iov.iov_len; \
   _mongoc_array_append_val (array, iov);
#define ENUM_FIELD INT32_FIELD
#define INT64_FIELD(_name)                   \
   iov.iov_base = (void *) &rpc->_name;      \
   iov.iov_len = 8;                          \
   header->msg_len += (int32_t) iov.iov_len; \
   _mongoc_array_append_val (array, iov);
#define CSTRING_FIELD(_name)                 \
   BSON_ASSERT (rpc->_name);                 \
   iov.iov_base = (void *) rpc->_name;       \
   iov.iov_len = strlen (rpc->_name) + 1;    \
   header->msg_len += (int32_t) iov.iov_len; \
   _mongoc_array_append_val (array, iov);
#define BSON_FIELD(_name)                       \
   do {                                         \
      int32_t __l;                              \
      memcpy (&__l, rpc->_name, 4);             \
      __l = BSON_UINT32_FROM_LE (__l);          \
      iov.iov_base = (void *) rpc->_name;       \
      iov.iov_len = __l;                        \
      header->msg_len += (int32_t) iov.iov_len; \
      _mongoc_array_append_val (array, iov);    \
   } while (0);
#define BSON_OPTIONAL(_check, _code) \
   if (rpc->_check) {                \
      _code                          \
   }
#define BSON_ARRAY_FIELD(_name)                 \
   if (rpc->_name##_len) {                      \
      iov.iov_base = (void *) rpc->_name;       \
      iov.iov_len = rpc->_name##_len;           \
      header->msg_len += (int32_t) iov.iov_len; \
      _mongoc_array_append_val (array, iov);    \
   }
#define IOVEC_ARRAY_FIELD(_name)                              \
   do {                                                       \
      ssize_t _i;                                             \
      BSON_ASSERT (rpc->n_##_name);                           \
      for (_i = 0; _i < rpc->n_##_name; _i++) {               \
         BSON_ASSERT (rpc->_name[_i].iov_len);                \
         header->msg_len += (int32_t) rpc->_name[_i].iov_len; \
         _mongoc_array_append_val (array, rpc->_name[_i]);    \
      }                                                       \
   } while (0);
#define RAW_BUFFER_FIELD(_name)              \
   iov.iov_base = (void *) rpc->_name;       \
   iov.iov_len = rpc->_name##_len;           \
   BSON_ASSERT (iov.iov_len);                \
   header->msg_len += (int32_t) iov.iov_len; \
   _mongoc_array_append_val (array, iov);
#define INT64_ARRAY_FIELD(_len, _name)       \
   iov.iov_base = (void *) &rpc->_len;       \
   iov.iov_len = 4;                          \
   header->msg_len += (int32_t) iov.iov_len; \
   _mongoc_array_append_val (array, iov);    \
   iov.iov_base = (void *) rpc->_name;       \
   iov.iov_len = rpc->_len * 8;              \
   BSON_ASSERT (iov.iov_len);                \
   header->msg_len += (int32_t) iov.iov_len; \
   _mongoc_array_append_val (array, iov);


#include "op-delete.def"
#include "op-get-more.def"
#include "op-insert.def"
#include "op-kill-cursors.def"
#include "op-msg.def"
#include "op-query.def"
#include "op-reply.def"
#include "op-compressed.def"
#include "op-update.def"


#undef RPC
#undef ENUM_FIELD
#undef UINT8_FIELD
#undef INT32_FIELD
#undef INT64_FIELD
#undef INT64_ARRAY_FIELD
#undef CSTRING_FIELD
#undef BSON_FIELD
#undef BSON_ARRAY_FIELD
#undef IOVEC_ARRAY_FIELD
#undef RAW_BUFFER_FIELD
#undef BSON_OPTIONAL


#if BSON_BYTE_ORDER == BSON_BIG_ENDIAN

#define RPC(_name, _code)                                                   \
   static void _mongoc_rpc_swab_to_le_##_name (mongoc_rpc_##_name##_t *rpc) \
   {                                                                        \
      BSON_ASSERT (rpc);                                                    \
      _code                                                                 \
   }
#define UINT8_FIELD(_name)
#define INT32_FIELD(_name) rpc->_name = BSON_UINT32_FROM_LE (rpc->_name);
#define ENUM_FIELD INT32_FIELD
#define INT64_FIELD(_name) rpc->_name = BSON_UINT64_FROM_LE (rpc->_name);
#define CSTRING_FIELD(_name)
#define BSON_FIELD(_name)
#define BSON_ARRAY_FIELD(_name)
#define IOVEC_ARRAY_FIELD(_name)
#define BSON_OPTIONAL(_check, _code) \
   if (rpc->_check) {                \
      _code                          \
   }
#define RAW_BUFFER_FIELD(_name)
#define INT64_ARRAY_FIELD(_len, _name)                        \
   do {                                                       \
      ssize_t i;                                              \
      for (i = 0; i < rpc->_len; i++) {                       \
         rpc->_name[i] = BSON_UINT64_FROM_LE (rpc->_name[i]); \
      }                                                       \
      rpc->_len = BSON_UINT32_FROM_LE (rpc->_len);            \
   } while (0);


#include "op-delete.def"
#include "op-get-more.def"
#include "op-insert.def"
#include "op-kill-cursors.def"
#include "op-msg.def"
#include "op-query.def"
#include "op-reply.def"
#include "op-compressed.def"
#include "op-update.def"

#undef RPC
#undef INT64_ARRAY_FIELD

#define RPC(_name, _code)                                                     \
   static void _mongoc_rpc_swab_from_le_##_name (mongoc_rpc_##_name##_t *rpc) \
   {                                                                          \
      BSON_ASSERT (rpc);                                                      \
      _code                                                                   \
   }
#define INT64_ARRAY_FIELD(_len, _name)                        \
   do {                                                       \
      ssize_t i;                                              \
      rpc->_len = BSON_UINT32_FROM_LE (rpc->_len);            \
      for (i = 0; i < rpc->_len; i++) {                       \
         rpc->_name[i] = BSON_UINT64_FROM_LE (rpc->_name[i]); \
      }                                                       \
   } while (0);


#include "op-delete.def"
#include "op-get-more.def"
#include "op-insert.def"
#include "op-kill-cursors.def"
#include "op-msg.def"
#include "op-query.def"
#include "op-reply.def"
#include "op-compressed.def"
#include "op-update.def"

#undef RPC
#undef ENUM_FIELD
#undef UINT8_FIELD
#undef INT32_FIELD
#undef INT64_FIELD
#undef INT64_ARRAY_FIELD
#undef CSTRING_FIELD
#undef BSON_FIELD
#undef BSON_ARRAY_FIELD
#undef IOVEC_ARRAY_FIELD
#undef BSON_OPTIONAL
#undef RAW_BUFFER_FIELD

#endif /* BSON_BYTE_ORDER == BSON_BIG_ENDIAN */


#define RPC(_name, _code)                                               \
   static void _mongoc_rpc_printf_##_name (mongoc_rpc_##_name##_t *rpc) \
   {                                                                    \
      BSON_ASSERT (rpc);                                                \
      _code                                                             \
   }
#define UINT8_FIELD(_name) printf ("  " #_name " : %u\n", rpc->_name);
#define INT32_FIELD(_name) printf ("  " #_name " : %d\n", rpc->_name);
#define ENUM_FIELD(_name) printf ("  " #_name " : %u\n", rpc->_name);
#define INT64_FIELD(_name) \
   printf ("  " #_name " : %" PRIi64 "\n", (int64_t) rpc->_name);
#define CSTRING_FIELD(_name) printf ("  " #_name " : %s\n", rpc->_name);
#define BSON_FIELD(_name)                     \
   do {                                       \
      bson_t b;                               \
      char *s;                                \
      int32_t __l;                            \
      memcpy (&__l, rpc->_name, 4);           \
      __l = BSON_UINT32_FROM_LE (__l);        \
      bson_init_static (&b, rpc->_name, __l); \
      s = bson_as_extended_json (&b, NULL);   \
      printf ("  " #_name " : %s\n", s);      \
      bson_free (s);                          \
      bson_destroy (&b);                      \
   } while (0);
#define BSON_ARRAY_FIELD(_name)                                       \
   do {                                                               \
      bson_reader_t *__r;                                             \
      bool __eof;                                                     \
      const bson_t *__b;                                              \
      __r = bson_reader_new_from_data (rpc->_name, rpc->_name##_len); \
      while ((__b = bson_reader_read (__r, &__eof))) {                \
         char *s = bson_as_extended_json (__b, NULL);                 \
         printf ("  " #_name " : %s\n", s);                           \
         bson_free (s);                                               \
      }                                                               \
      bson_reader_destroy (__r);                                      \
   } while (0);
#define IOVEC_ARRAY_FIELD(_name)                           \
   do {                                                    \
      ssize_t _i;                                          \
      size_t _j;                                           \
      for (_i = 0; _i < rpc->n_##_name; _i++) {            \
         printf ("  " #_name " : ");                       \
         for (_j = 0; _j < rpc->_name[_i].iov_len; _j++) { \
            uint8_t u;                                     \
            u = ((char *) rpc->_name[_i].iov_base)[_j];    \
            printf (" %02x", u);                           \
         }                                                 \
         printf ("\n");                                    \
      }                                                    \
   } while (0);
#define BSON_OPTIONAL(_check, _code) \
   if (rpc->_check) {                \
      _code                          \
   }
#define RAW_BUFFER_FIELD(_name)                      \
   {                                                 \
      ssize_t __i;                                   \
      printf ("  " #_name " :");                     \
      for (__i = 0; __i < rpc->_name##_len; __i++) { \
         uint8_t u;                                  \
         u = ((char *) rpc->_name)[__i];             \
         printf (" %02x", u);                        \
      }                                              \
      printf ("\n");                                 \
   }
#define INT64_ARRAY_FIELD(_len, _name)                                     \
   do {                                                                    \
      ssize_t i;                                                           \
      for (i = 0; i < rpc->_len; i++) {                                    \
         printf ("  " #_name " : %" PRIi64 "\n", (int64_t) rpc->_name[i]); \
      }                                                                    \
      rpc->_len = BSON_UINT32_FROM_LE (rpc->_len);                         \
   } while (0);


#include "op-delete.def"
#include "op-get-more.def"
#include "op-insert.def"
#include "op-kill-cursors.def"
#include "op-msg.def"
#include "op-query.def"
#include "op-reply.def"
#include "op-compressed.def"
#include "op-update.def"


#undef RPC
#undef ENUM_FIELD
#undef UINT8_FIELD
#undef INT32_FIELD
#undef INT64_FIELD
#undef INT64_ARRAY_FIELD
#undef CSTRING_FIELD
#undef BSON_FIELD
#undef BSON_ARRAY_FIELD
#undef IOVEC_ARRAY_FIELD
#undef BSON_OPTIONAL
#undef RAW_BUFFER_FIELD


#define RPC(_name, _code)                                             \
   static bool _mongoc_rpc_scatter_##_name (                          \
      mongoc_rpc_##_name##_t *rpc, const uint8_t *buf, size_t buflen) \
   {                                                                  \
      BSON_ASSERT (rpc);                                              \
      BSON_ASSERT (buf);                                              \
      BSON_ASSERT (buflen);                                           \
      _code return true;                                              \
   }
#define UINT8_FIELD(_name)       \
   if (buflen < 1) {             \
      return false;              \
   }                             \
   memcpy (&rpc->_name, buf, 1); \
   buflen -= 1;                  \
   buf += 1;
#define INT32_FIELD(_name)       \
   if (buflen < 4) {             \
      return false;              \
   }                             \
   memcpy (&rpc->_name, buf, 4); \
   buflen -= 4;                  \
   buf += 4;
#define ENUM_FIELD INT32_FIELD
#define INT64_FIELD(_name)       \
   if (buflen < 8) {             \
      return false;              \
   }                             \
   memcpy (&rpc->_name, buf, 8); \
   buflen -= 8;                  \
   buf += 8;
#define INT64_ARRAY_FIELD(_len, _name)              \
   do {                                             \
      size_t needed;                                \
      if (buflen < 4) {                             \
         return false;                              \
      }                                             \
      memcpy (&rpc->_len, buf, 4);                  \
      buflen -= 4;                                  \
      buf += 4;                                     \
      needed = BSON_UINT32_FROM_LE (rpc->_len) * 8; \
      if (needed > buflen) {                        \
         return false;                              \
      }                                             \
      rpc->_name = (int64_t *) buf;                 \
      buf += needed;                                \
      buflen -= needed;                             \
   } while (0);
#define CSTRING_FIELD(_name)                 \
   do {                                      \
      size_t __i;                            \
      bool found = false;                    \
      for (__i = 0; __i < buflen; __i++) {   \
         if (!buf[__i]) {                    \
            rpc->_name = (const char *) buf; \
            buflen -= __i + 1;               \
            buf += __i + 1;                  \
            found = true;                    \
            break;                           \
         }                                   \
      }                                      \
      if (!found) {                          \
         return false;                       \
      }                                      \
   } while (0);
#define BSON_FIELD(_name)              \
   do {                                \
      uint32_t __l;                    \
      if (buflen < 4) {                \
         return false;                 \
      }                                \
      memcpy (&__l, buf, 4);           \
      __l = BSON_UINT32_FROM_LE (__l); \
      if (__l < 5 || __l > buflen) {   \
         return false;                 \
      }                                \
      rpc->_name = (uint8_t *) buf;    \
      buf += __l;                      \
      buflen -= __l;                   \
   } while (0);
#define BSON_ARRAY_FIELD(_name)         \
   rpc->_name = (uint8_t *) buf;        \
   rpc->_name##_len = (int32_t) buflen; \
   buf = NULL;                          \
   buflen = 0;
#define BSON_OPTIONAL(_check, _code) \
   if (buflen) {                     \
      _code                          \
   }
#define IOVEC_ARRAY_FIELD(_name)              \
   rpc->_name##_recv.iov_base = (void *) buf; \
   rpc->_name##_recv.iov_len = buflen;        \
   rpc->_name = &rpc->_name##_recv;           \
   rpc->n_##_name = 1;                        \
   buf = NULL;                                \
   buflen = 0;
#define RAW_BUFFER_FIELD(_name)         \
   rpc->_name = (void *) buf;           \
   rpc->_name##_len = (int32_t) buflen; \
   buf = NULL;                          \
   buflen = 0;


#include "op-delete.def"
#include "op-get-more.def"
#include "op-header.def"
#include "op-insert.def"
#include "op-kill-cursors.def"
#include "op-msg.def"
#include "op-query.def"
#include "op-reply.def"
#include "op-reply-header.def"
#include "op-compressed.def"
#include "op-update.def"


#undef RPC
#undef ENUM_FIELD
#undef UINT8_FIELD
#undef INT32_FIELD
#undef INT64_FIELD
#undef INT64_ARRAY_FIELD
#undef CSTRING_FIELD
#undef BSON_FIELD
#undef BSON_ARRAY_FIELD
#undef IOVEC_ARRAY_FIELD
#undef BSON_OPTIONAL
#undef RAW_BUFFER_FIELD


void
_mongoc_rpc_gather (mongoc_rpc_t *rpc, mongoc_array_t *array)
{
   switch ((mongoc_opcode_t) rpc->header.opcode) {
   case MONGOC_OPCODE_REPLY:
      _mongoc_rpc_gather_reply (&rpc->reply, &rpc->header, array);
      return;
   case MONGOC_OPCODE_MSG:
      _mongoc_rpc_gather_msg (&rpc->msg, &rpc->header, array);
      return;
   case MONGOC_OPCODE_UPDATE:
      _mongoc_rpc_gather_update (&rpc->update, &rpc->header, array);
      return;
   case MONGOC_OPCODE_INSERT:
      _mongoc_rpc_gather_insert (&rpc->insert, &rpc->header, array);
      return;
   case MONGOC_OPCODE_QUERY:
      _mongoc_rpc_gather_query (&rpc->query, &rpc->header, array);
      return;
   case MONGOC_OPCODE_GET_MORE:
      _mongoc_rpc_gather_get_more (&rpc->get_more, &rpc->header, array);
      return;
   case MONGOC_OPCODE_DELETE:
      _mongoc_rpc_gather_delete (&rpc->delete_, &rpc->header, array);
      return;
   case MONGOC_OPCODE_KILL_CURSORS:
      _mongoc_rpc_gather_kill_cursors (&rpc->kill_cursors, &rpc->header, array);
      return;
   case MONGOC_OPCODE_COMPRESSED:
      _mongoc_rpc_gather_compressed (&rpc->compressed, &rpc->header, array);
      return;
   default:
      MONGOC_WARNING ("Unknown rpc type: 0x%08x", rpc->header.opcode);
      break;
   }
}


void
_mongoc_rpc_swab_to_le (mongoc_rpc_t *rpc)
{
#if BSON_BYTE_ORDER != BSON_LITTLE_ENDIAN
   mongoc_opcode_t opcode;

   opcode = rpc->header.opcode;

   switch (opcode) {
   case MONGOC_OPCODE_REPLY:
      _mongoc_rpc_swab_to_le_reply (&rpc->reply);
      break;
   case MONGOC_OPCODE_MSG:
      _mongoc_rpc_swab_to_le_msg (&rpc->msg);
      break;
   case MONGOC_OPCODE_UPDATE:
      _mongoc_rpc_swab_to_le_update (&rpc->update);
      break;
   case MONGOC_OPCODE_INSERT:
      _mongoc_rpc_swab_to_le_insert (&rpc->insert);
      break;
   case MONGOC_OPCODE_QUERY:
      _mongoc_rpc_swab_to_le_query (&rpc->query);
      break;
   case MONGOC_OPCODE_GET_MORE:
      _mongoc_rpc_swab_to_le_get_more (&rpc->get_more);
      break;
   case MONGOC_OPCODE_DELETE:
      _mongoc_rpc_swab_to_le_delete (&rpc->delete_);
      break;
   case MONGOC_OPCODE_KILL_CURSORS:
      _mongoc_rpc_swab_to_le_kill_cursors (&rpc->kill_cursors);
      break;
   case MONGOC_OPCODE_COMPRESSED:
      _mongoc_rpc_swab_to_le_compressed (&rpc->compressed);
      break;
   default:
      MONGOC_WARNING ("Unknown rpc type: 0x%08x", opcode);
      break;
   }
#endif
}


void
_mongoc_rpc_swab_from_le (mongoc_rpc_t *rpc)
{
#if BSON_BYTE_ORDER != BSON_LITTLE_ENDIAN
   mongoc_opcode_t opcode;

   opcode = BSON_UINT32_FROM_LE (rpc->header.opcode);

   switch (opcode) {
   case MONGOC_OPCODE_REPLY:
      _mongoc_rpc_swab_from_le_reply (&rpc->reply);
      break;
   case MONGOC_OPCODE_MSG:
      _mongoc_rpc_swab_from_le_msg (&rpc->msg);
      break;
   case MONGOC_OPCODE_UPDATE:
      _mongoc_rpc_swab_from_le_update (&rpc->update);
      break;
   case MONGOC_OPCODE_INSERT:
      _mongoc_rpc_swab_from_le_insert (&rpc->insert);
      break;
   case MONGOC_OPCODE_QUERY:
      _mongoc_rpc_swab_from_le_query (&rpc->query);
      break;
   case MONGOC_OPCODE_GET_MORE:
      _mongoc_rpc_swab_from_le_get_more (&rpc->get_more);
      break;
   case MONGOC_OPCODE_DELETE:
      _mongoc_rpc_swab_from_le_delete (&rpc->delete_);
      break;
   case MONGOC_OPCODE_KILL_CURSORS:
      _mongoc_rpc_swab_from_le_kill_cursors (&rpc->kill_cursors);
      break;
   case MONGOC_OPCODE_COMPRESSED:
      _mongoc_rpc_swab_from_le_compressed (&rpc->compressed);
      break;
   default:
      MONGOC_WARNING ("Unknown rpc type: 0x%08x", rpc->header.opcode);
      break;
   }
#endif
}


void
_mongoc_rpc_printf (mongoc_rpc_t *rpc)
{
   switch ((mongoc_opcode_t) rpc->header.opcode) {
   case MONGOC_OPCODE_REPLY:
      _mongoc_rpc_printf_reply (&rpc->reply);
      break;
   case MONGOC_OPCODE_MSG:
      _mongoc_rpc_printf_msg (&rpc->msg);
      break;
   case MONGOC_OPCODE_UPDATE:
      _mongoc_rpc_printf_update (&rpc->update);
      break;
   case MONGOC_OPCODE_INSERT:
      _mongoc_rpc_printf_insert (&rpc->insert);
      break;
   case MONGOC_OPCODE_QUERY:
      _mongoc_rpc_printf_query (&rpc->query);
      break;
   case MONGOC_OPCODE_GET_MORE:
      _mongoc_rpc_printf_get_more (&rpc->get_more);
      break;
   case MONGOC_OPCODE_DELETE:
      _mongoc_rpc_printf_delete (&rpc->delete_);
      break;
   case MONGOC_OPCODE_KILL_CURSORS:
      _mongoc_rpc_printf_kill_cursors (&rpc->kill_cursors);
      break;
   case MONGOC_OPCODE_COMPRESSED:
      _mongoc_rpc_printf_compressed (&rpc->compressed);
      break;
   default:
      MONGOC_WARNING ("Unknown rpc type: 0x%08x", rpc->header.opcode);
      break;
   }
}

bool
_mongoc_rpc_decompress (mongoc_rpc_t *rpc, uint8_t *buf, size_t buflen)
{
   size_t uncompressed_size = rpc->compressed.uncompressed_size;
   bool ok;

   BSON_ASSERT (uncompressed_size <= buflen);
   memcpy (buf, (void *) (&buflen), 4);
   memcpy (buf + 4, (void *) (&rpc->header.request_id), 4);
   memcpy (buf + 8, (void *) (&rpc->header.response_to), 4);
   memcpy (buf + 12, (void *) (&rpc->compressed.original_opcode), 4);

   ok = mongoc_uncompress (rpc->compressed.compressor_id,
                           rpc->compressed.compressed_message,
                           rpc->compressed.compressed_message_len,
                           buf + 16,
                           &uncompressed_size);
   if (ok) {
      return _mongoc_rpc_scatter (rpc, buf, buflen);
   }

   return false;
}

char *
_mongoc_rpc_compress (struct _mongoc_cluster_t *cluster,
                      int32_t compressor_id,
                      mongoc_rpc_t *rpc,
                      bson_error_t *error)
{
   char *output;
   size_t output_length = 0;
   size_t allocate = rpc->header.msg_len - 16;
   char *data;
   int size;
   int32_t compression_level = -1;

   if (compressor_id == MONGOC_COMPRESSOR_ZLIB_ID) {
      compression_level = mongoc_uri_get_option_as_int32 (
         cluster->uri, MONGOC_URI_ZLIBCOMPRESSIONLEVEL, -1);
   }

   BSON_ASSERT (allocate > 0);
   data = bson_malloc0 (allocate);
   size = _mongoc_cluster_buffer_iovec (
      cluster->iov.data, cluster->iov.len, 16, data);
   BSON_ASSERT (size);

   output_length =
      mongoc_compressor_max_compressed_length (compressor_id, size);
   if (!output_length) {
      bson_set_error (error,
                      MONGOC_ERROR_COMMAND,
                      MONGOC_ERROR_COMMAND_INVALID_ARG,
                      "Could not determine compression bounds for %s",
                      mongoc_compressor_id_to_name (compressor_id));
      bson_free (data);
      return NULL;
   }

   output = (char *) bson_malloc0 (output_length);
   if (mongoc_compress (compressor_id,
                        compression_level,
                        data,
                        size,
                        output,
                        &output_length)) {
      rpc->header.msg_len = 0;
      rpc->compressed.original_opcode = rpc->header.opcode;
      rpc->header.opcode = MONGOC_OPCODE_COMPRESSED;

      rpc->compressed.uncompressed_size = size;
      rpc->compressed.compressor_id = compressor_id;
      rpc->compressed.compressed_message = (const uint8_t *) output;
      rpc->compressed.compressed_message_len = output_length;
      bson_free (data);


      _mongoc_array_destroy (&cluster->iov);
      _mongoc_array_init (&cluster->iov, sizeof (mongoc_iovec_t));
      _mongoc_rpc_gather (rpc, &cluster->iov);
      return output;
   } else {
      MONGOC_WARNING ("Could not compress data with %s",
                      mongoc_compressor_id_to_name (compressor_id));
   }
   bson_free (data);
   return NULL;
}
bool
_mongoc_rpc_scatter (mongoc_rpc_t *rpc, const uint8_t *buf, size_t buflen)
{
   mongoc_opcode_t opcode;

   memset (rpc, 0, sizeof *rpc);

   if (BSON_UNLIKELY (buflen < 16)) {
      return false;
   }

   if (!_mongoc_rpc_scatter_header (&rpc->header, buf, 16)) {
      return false;
   }

   opcode = (mongoc_opcode_t) BSON_UINT32_FROM_LE (rpc->header.opcode);

   switch (opcode) {
   case MONGOC_OPCODE_COMPRESSED:
      return _mongoc_rpc_scatter_compressed (&rpc->compressed, buf, buflen);
   case MONGOC_OPCODE_REPLY:
      return _mongoc_rpc_scatter_reply (&rpc->reply, buf, buflen);
   case MONGOC_OPCODE_MSG:
      return _mongoc_rpc_scatter_msg (&rpc->msg, buf, buflen);
   case MONGOC_OPCODE_UPDATE:
      return _mongoc_rpc_scatter_update (&rpc->update, buf, buflen);
   case MONGOC_OPCODE_INSERT:
      return _mongoc_rpc_scatter_insert (&rpc->insert, buf, buflen);
   case MONGOC_OPCODE_QUERY:
      return _mongoc_rpc_scatter_query (&rpc->query, buf, buflen);
   case MONGOC_OPCODE_GET_MORE:
      return _mongoc_rpc_scatter_get_more (&rpc->get_more, buf, buflen);
   case MONGOC_OPCODE_DELETE:
      return _mongoc_rpc_scatter_delete (&rpc->delete_, buf, buflen);
   case MONGOC_OPCODE_KILL_CURSORS:
      return _mongoc_rpc_scatter_kill_cursors (&rpc->kill_cursors, buf, buflen);
   default:
      MONGOC_WARNING ("Unknown rpc type: 0x%08x", opcode);
      return false;
   }
}


bool
_mongoc_rpc_scatter_reply_header_only (mongoc_rpc_t *rpc,
                                       const uint8_t *buf,
                                       size_t buflen)
{
   if (BSON_UNLIKELY (buflen < sizeof (mongoc_rpc_reply_header_t))) {
      return false;
   }

   return _mongoc_rpc_scatter_reply_header (&rpc->reply_header, buf, buflen);
}

bool
_mongoc_rpc_get_first_document (mongoc_rpc_t *rpc, bson_t *reply)
{
   if (rpc->header.opcode == MONGOC_OPCODE_REPLY &&
       _mongoc_rpc_reply_get_first (&rpc->reply, reply)) {
      return true;
   }

   return false;
}

bool
_mongoc_rpc_reply_get_first (mongoc_rpc_reply_t *reply, bson_t *bson)
{
   int32_t len;

   if (!reply->documents || reply->documents_len < 4) {
      return false;
   }

   memcpy (&len, reply->documents, 4);
   len = BSON_UINT32_FROM_LE (len);
   if (reply->documents_len < len) {
      return false;
   }

   return bson_init_static (bson, reply->documents, len);
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_rpc_needs_gle --
 *
 *       Checks to see if an rpc requires a getlasterror command to
 *       determine the success of the rpc.
 *
 *       The write_concern is checked to ensure that the caller wants
 *       to know about a failure.
 *
 * Returns:
 *       true if a getlasterror should be delivered; otherwise false.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

bool
_mongoc_rpc_needs_gle (mongoc_rpc_t *rpc,
                       const mongoc_write_concern_t *write_concern)
{
   switch (rpc->header.opcode) {
   case MONGOC_OPCODE_REPLY:
   case MONGOC_OPCODE_QUERY:
   case MONGOC_OPCODE_MSG:
   case MONGOC_OPCODE_GET_MORE:
   case MONGOC_OPCODE_KILL_CURSORS:
   case MONGOC_OPCODE_COMPRESSED:
      return false;
   case MONGOC_OPCODE_INSERT:
   case MONGOC_OPCODE_UPDATE:
   case MONGOC_OPCODE_DELETE:
   default:
      break;
   }

   if (!write_concern || !mongoc_write_concern_get_w (write_concern)) {
      return false;
   }

   return true;
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_rpc_prep_command --
 *
 *       Prepare an RPC for mongoc_cluster_run_command_rpc. @cmd_ns and
 *       @command must not be freed or modified while the RPC is in use.
 *
 * Side effects:
 *       Fills out the RPC, including pointers into @cmd_ns and @command.
 *
 *--------------------------------------------------------------------------
 */

void
_mongoc_rpc_prep_command (mongoc_rpc_t *rpc,
                          const char *cmd_ns,
                          const bson_t *command,
                          mongoc_query_flags_t flags)
{
   rpc->header.msg_len = 0;
   rpc->header.request_id = 0;
   rpc->header.response_to = 0;
   rpc->header.opcode = MONGOC_OPCODE_QUERY;
   rpc->query.collection = cmd_ns;
   rpc->query.skip = 0;
   rpc->query.n_return = -1;
   rpc->query.fields = NULL;
   rpc->query.query = bson_get_data (command);

   /* Find, getMore And killCursors Commands Spec: "When sending a find command
    * rather than a legacy OP_QUERY find, only the slaveOk flag is honored."
    * For other cursor-typed commands like aggregate, only slaveOk can be set.
    * Clear bits except slaveOk; leave slaveOk set only if it is already.
    */
   rpc->query.flags = flags & MONGOC_QUERY_SLAVE_OK;
}


bool
_mongoc_populate_cmd_error (const bson_t *doc,
                            int32_t error_api_version,
                            bson_error_t *error)
{
   mongoc_error_domain_t domain =
      error_api_version >= MONGOC_ERROR_API_VERSION_2 ? MONGOC_ERROR_SERVER
                                                      : MONGOC_ERROR_QUERY;
   uint32_t code = MONGOC_ERROR_QUERY_FAILURE;
   bson_iter_t iter;
   const char *msg = "Unknown command error";

   ENTRY;

   BSON_ASSERT (doc);

   if (bson_iter_init_find (&iter, doc, "ok") && bson_iter_as_bool (&iter)) {
      /* no error */
      RETURN (false);
   }

   if (bson_iter_init_find (&iter, doc, "code") &&
       BSON_ITER_HOLDS_INT32 (&iter)) {
      code = (uint32_t) bson_iter_int32 (&iter);
   }

   if (code == MONGOC_ERROR_PROTOCOL_ERROR || code == 13390) {
      code = MONGOC_ERROR_QUERY_COMMAND_NOT_FOUND;
   }

   if (bson_iter_init_find (&iter, doc, "errmsg") &&
       BSON_ITER_HOLDS_UTF8 (&iter)) {
      msg = bson_iter_utf8 (&iter, NULL);
   } else if (bson_iter_init_find (&iter, doc, "$err") &&
              BSON_ITER_HOLDS_UTF8 (&iter)) {
      msg = bson_iter_utf8 (&iter, NULL);
   }

   bson_set_error (error, domain, code, "%s", msg);

   RETURN (true);
}


static void
_mongoc_populate_query_error (const bson_t *doc,
                              int32_t error_api_version,
                              bson_error_t *error)
{
   mongoc_error_domain_t domain =
      error_api_version >= MONGOC_ERROR_API_VERSION_2 ? MONGOC_ERROR_SERVER
                                                      : MONGOC_ERROR_QUERY;
   uint32_t code = MONGOC_ERROR_QUERY_FAILURE;
   bson_iter_t iter;
   const char *msg = "Unknown query failure";

   ENTRY;

   BSON_ASSERT (doc);

   if (bson_iter_init_find (&iter, doc, "code") &&
       BSON_ITER_HOLDS_INT32 (&iter)) {
      code = (uint32_t) bson_iter_int32 (&iter);
   }

   if (bson_iter_init_find (&iter, doc, "$err") &&
       BSON_ITER_HOLDS_UTF8 (&iter)) {
      msg = bson_iter_utf8 (&iter, NULL);
   }

   bson_set_error (error, domain, code, "%s", msg);

   EXIT;
}

bool
_mongoc_rpc_is_failure (mongoc_rpc_t *rpc,
                        bool is_command,
                        int32_t error_api_version,
                        bson_error_t *error)
{
   if (rpc->header.opcode == MONGOC_OPCODE_REPLY) {
      if (rpc->reply.flags & MONGOC_REPLY_QUERY_FAILURE) {
         bson_t b;

         if (_mongoc_rpc_get_first_document (rpc, &b)) {
            _mongoc_populate_query_error (&b, error_api_version, error);
            bson_destroy (&b);
         } else {
            bson_set_error (error,
                            MONGOC_ERROR_QUERY,
                            MONGOC_ERROR_QUERY_FAILURE,
                            "Unknown query failure.");
         }

         return true;
      }

      if (rpc->reply.flags & MONGOC_REPLY_CURSOR_NOT_FOUND) {
         bson_set_error (error,
                         MONGOC_ERROR_CURSOR,
                         MONGOC_ERROR_CURSOR_INVALID_CURSOR,
                         "The cursor is invalid or has expired.");

         return true;
      }
   }

   return false;
}


/* returns true if the reply is a server error
 *
 * note we deliberately do *not* check for writeConcernError
 */
static bool
_mongoc_rpc_parse_error (mongoc_rpc_t *rpc,
                         bool is_command,
                         int32_t error_api_version,
                         bson_error_t *error /* OUT */)
{
   bson_t b;
   bool r;

   ENTRY;

   BSON_ASSERT (rpc);

   if (rpc->header.opcode != MONGOC_OPCODE_REPLY) {
      bson_set_error (error,
                      MONGOC_ERROR_PROTOCOL,
                      MONGOC_ERROR_PROTOCOL_INVALID_REPLY,
                      "Received rpc other than OP_REPLY.");
      RETURN (true);
   }

   if (is_command) {
      if (rpc->reply.n_returned != 1) {
         bson_set_error (error,
                         MONGOC_ERROR_PROTOCOL,
                         MONGOC_ERROR_PROTOCOL_INVALID_REPLY,
                         "Expected only one reply document, got %d",
                         rpc->reply.n_returned);
         RETURN (true);
      }

      if (_mongoc_rpc_get_first_document (rpc, &b)) {
         r = _mongoc_populate_cmd_error (&b, error_api_version, error);
         bson_destroy (&b);
         RETURN (r);
      } else {
         bson_set_error (error,
                         MONGOC_ERROR_BSON,
                         MONGOC_ERROR_BSON_INVALID,
                         "Failed to decode document from the server.");
         RETURN (true);
      }
   } else if (_mongoc_rpc_is_failure (
                 rpc, is_command, error_api_version, error)) {
      RETURN (true);
   }


   RETURN (false);
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_rpc_parse_command_error --
 *
 *       Check if a server OP_REPLY is a command error message.
 *       Optionally fill out a bson_error_t from the server error.
 *
 * Returns:
 *       true if the reply is an error message, false otherwise.
 *
 * Side effects:
 *       If rpc is an error reply and @error is not NULL, set its
 *       domain, code, and message.
 *
 *--------------------------------------------------------------------------
 */

bool
_mongoc_rpc_parse_command_error (mongoc_rpc_t *rpc,
                                 int32_t error_api_version,
                                 bson_error_t *error)
{
   return _mongoc_rpc_parse_error (rpc, true, error_api_version, error);
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_rpc_parse_query_error --
 *
 *       Check if a server OP_REPLY is a query error message.
 *       Optionally fill out a bson_error_t from the server error.
 *
 * Returns:
 *       true if the reply is an error message, false otherwise.
 *
 * Side effects:
 *       If rpc is an error reply and @error is not NULL, set its
 *       domain, code, and message.
 *
 *--------------------------------------------------------------------------
 */

bool
_mongoc_rpc_parse_query_error (mongoc_rpc_t *rpc,
                               int32_t error_api_version,
                               bson_error_t *error)
{
   return _mongoc_rpc_parse_error (rpc, false, error_api_version, error);
}
