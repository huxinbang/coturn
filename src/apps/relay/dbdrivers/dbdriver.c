/*
 * Copyright (C) 2011, 2012, 2013 Citrix Systems
 * Copyright (C) 2014 Vivocha S.p.A.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "../mainrelay.h"

#include "apputils.h"

#include "dbdriver.h"
#include "dbd_pgsql.h"
#include "dbd_mysql.h"
#include "dbd_mongo.h"
#include "dbd_redis.h"

static turn_dbdriver_t * _driver;

int convert_string_key_to_binary(char* keysource, hmackey_t key, size_t sz) {
	char is[3];
	size_t i;
	unsigned int v;
	is[2]=0;
	for(i=0;i<sz;i++) {
		is[0]=keysource[i*2];
		is[1]=keysource[i*2+1];
		sscanf(is,"%02x",&v);
		key[i]=(unsigned char)v;
	}
	return 0;
}

persistent_users_db_t * get_persistent_users_db(void) {
	return &(turn_params.default_users_db.persistent_users_db);
}

turn_dbdriver_t * get_dbdriver() {
  if (!_driver) {
    switch(turn_params.default_users_db.userdb_type) {
#if !defined(TURN_NO_PQ)
    case TURN_USERDB_TYPE_PQ:
      _driver = get_pgsql_dbdriver();
      break;
#endif
#if !defined(TURN_NO_MYSQL)
    case TURN_USERDB_TYPE_MYSQL:
      _driver = get_mysql_dbdriver();
      break;
#endif
#if !defined(TURN_NO_MONGO)
    case TURN_USERDB_TYPE_MONGO:
      _driver = get_mongo_dbdriver();
      break;
#endif
#if !defined(TURN_NO_HIREDIS)
    case TURN_USERDB_TYPE_REDIS:
      _driver = get_redis_dbdriver();
      break;
#endif
    default:
      break;
    }
  }
  return _driver;
}

/////////// OAUTH /////////////////

void convert_oauth_key_data_raw(const oauth_key_data_raw *raw, oauth_key_data *oakd)
{
	if(raw && oakd) {

		ns_bzero(oakd,sizeof(oauth_key_data));

		oakd->timestamp = (turn_time_t)raw->timestamp;
		oakd->lifetime = raw->lifetime;

		ns_bcopy(raw->as_rs_alg,oakd->as_rs_alg,sizeof(oakd->as_rs_alg));
		ns_bcopy(raw->auth_alg,oakd->auth_alg,sizeof(oakd->auth_alg));
		ns_bcopy(raw->hkdf_hash_func,oakd->hkdf_hash_func,sizeof(oakd->hkdf_hash_func));
		ns_bcopy(raw->kid,oakd->kid,sizeof(oakd->kid));

		if(raw->ikm_key[0]) {
			size_t ikm_key_size = 0;
			char *ikm_key = (char*)base64_decode(raw->ikm_key,strlen(raw->ikm_key),&ikm_key_size);
			if(ikm_key) {
				ns_bcopy(ikm_key,oakd->ikm_key,ikm_key_size);
				oakd->ikm_key_size = ikm_key_size;
				turn_free(ikm_key,ikm_key_size);
			}
		}

		if(raw->as_rs_key[0]) {
			size_t as_rs_key_size = 0;
			char *as_rs_key = (char*)base64_decode(raw->as_rs_key,strlen(raw->as_rs_key),&as_rs_key_size);
			if(as_rs_key) {
				ns_bcopy(as_rs_key,oakd->as_rs_key,as_rs_key_size);
				oakd->as_rs_key_size = as_rs_key_size;
				turn_free(as_rs_key,as_rs_key_size);
			}
		}

		if(raw->auth_key[0]) {
			size_t auth_key_size = 0;
			char *auth_key = (char*)base64_decode(raw->auth_key,strlen(raw->auth_key),&auth_key_size);
			if(auth_key) {
				ns_bcopy(auth_key,oakd->auth_key,auth_key_size);
				oakd->auth_key_size = auth_key_size;
				turn_free(auth_key,auth_key_size);
			}
		}

	}
}

