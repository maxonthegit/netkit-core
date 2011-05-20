/* Copyright 2008 Jeff Dike & Kartoch
 * Licensed under the GPL
 */

#ifndef __UML_SWITCH_H__
#define __UML_SWITCH_H__

#include <stdint.h>
#include <sys/un.h>

enum request_type { 
  REQ_NEW_CONTROL,
  REQ_NEW_DUMP
};

struct request_v0 {
  enum request_type type;
  union {
    struct {
      unsigned char addr[ETH_ALEN];
      struct sockaddr_un name;
    } new_control;
  } u;
};

#define SWITCH_MAGIC 0xfeedface

struct request_v1 {
  uint32_t magic;
  enum request_type type;
  union {
    struct {
      unsigned char addr[ETH_ALEN];
      struct sockaddr_un name;
    } new_control;
  } u;
};

struct request_v2 {
  uint32_t magic;
  uint32_t version;
  enum request_type type;
  struct sockaddr_un sock;
};

struct reply_v2 {
  unsigned char mac[ETH_ALEN];
  struct sockaddr_un sock;
};

struct request_v3 {
  uint32_t magic;
  uint32_t version;
  enum request_type type;
  struct sockaddr_un sock;
};

union request {
  struct request_v0 v0;
  struct request_v1 v1;
  struct request_v2 v2;
  struct request_v3 v3;
};

#endif


