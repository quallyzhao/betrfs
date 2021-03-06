/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
// vim: ft=cpp:expandtab:ts=8:sw=4:softtabstop=4:
#ident "$Id$"
/*
COPYING CONDITIONS NOTICE:

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation, and provided that the
  following conditions are met:

      * Redistributions of source code must retain this COPYING
        CONDITIONS NOTICE, the COPYRIGHT NOTICE (below), the
        DISCLAIMER (below), the UNIVERSITY PATENT NOTICE (below), the
        PATENT MARKING NOTICE (below), and the PATENT RIGHTS
        GRANT (below).

      * Redistributions in binary form must reproduce this COPYING
        CONDITIONS NOTICE, the COPYRIGHT NOTICE (below), the
        DISCLAIMER (below), the UNIVERSITY PATENT NOTICE (below), the
        PATENT MARKING NOTICE (below), and the PATENT RIGHTS
        GRANT (below) in the documentation and/or other materials
        provided with the distribution.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

COPYRIGHT NOTICE:

  TokuDB, Tokutek Fractal Tree Indexing Library.
  Copyright (C) 2007-2013 Tokutek, Inc.

DISCLAIMER:

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

UNIVERSITY PATENT NOTICE:

  The technology is licensed by the Massachusetts Institute of
  Technology, Rutgers State University of New Jersey, and the Research
  Foundation of State University of New York at Stony Brook under
  United States of America Serial No. 11/760379 and to the patents
  and/or patent applications resulting from it.

PATENT MARKING NOTICE:

  This software is covered by US Patent No. 8,185,551.
  This software is covered by US Patent No. 8,489,638.

PATENT RIGHTS GRANT:

  "THIS IMPLEMENTATION" means the copyrightable works distributed by
  Tokutek as part of the Fractal Tree project.

  "PATENT CLAIMS" means the claims of patents that are owned or
  licensable by Tokutek, both currently or in the future; and that in
  the absence of this license would be infringed by THIS
  IMPLEMENTATION or by using or running THIS IMPLEMENTATION.

  "PATENT CHALLENGE" shall mean a challenge to the validity,
  patentability, enforceability and/or non-infringement of any of the
  PATENT CLAIMS or otherwise opposing any of the PATENT CLAIMS.

  Tokutek hereby grants to you, for the term and geographical scope of
  the PATENT CLAIMS, a non-exclusive, no-charge, royalty-free,
  irrevocable (except as stated in this section) patent license to
  make, have made, use, offer to sell, sell, import, transfer, and
  otherwise run, modify, and propagate the contents of THIS
  IMPLEMENTATION, where such license applies only to the PATENT
  CLAIMS.  This grant does not include claims that would be infringed
  only as a consequence of further modifications of THIS
  IMPLEMENTATION.  If you or your agent or licensee institute or order
  or agree to the institution of patent litigation against any entity
  (including a cross-claim or counterclaim in a lawsuit) alleging that
  THIS IMPLEMENTATION constitutes direct or contributory patent
  infringement, or inducement of patent infringement, then any rights
  granted to you under this License shall terminate as of the date
  such litigation is filed.  If you or your agent or exclusive
  licensee institute or order or agree to the institution of a PATENT
  CHALLENGE, then Tokutek may terminate any rights granted to you
  under this License.
*/

#ident "Copyright (c) 2007-2013 Tokutek Inc.  All rights reserved."
#include "test.h"
#include <stdio.h>

#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <sys/stat.h>
#include <db.h>
#include <memory.h>

static void
test_789(void) {
    int r;

    /* setup test directory */
    toku_os_recursive_delete(TOKU_TEST_FILENAME);
    toku_os_mkdir(TOKU_TEST_FILENAME, S_IRWXU+S_IRWXG+S_IRWXO);

    /* setup environment */
    DB_ENV *env;
    {
        r = db_env_create(&env, 0); assert(r == 0);
        env->set_errfile(env, stdout);
        r = env->open(env, TOKU_TEST_FILENAME, DB_INIT_MPOOL + DB_INIT_LOG + DB_INIT_LOCK + DB_INIT_TXN + DB_PRIVATE + DB_CREATE, S_IRWXU+S_IRWXG+S_IRWXO); 
        assert(r == 0);
    }

    /* setup database */
    DB *db;
    {
        DB_TXN *txn = 0;
        r = env->txn_begin(env, 0, &txn, 0); assert(r == 0);

        r = db_create(&db, env, 0); assert(r == 0);
        r = db->open(db, txn, "test.db", 0, DB_BTREE, DB_CREATE, S_IRWXU+S_IRWXG+S_IRWXO); assert(r == 0);

        r = txn->commit(txn, 0); assert(r == 0);
    }

    /* insert, commit */
    {    
        DB_TXN *txn_master;
        r = env->txn_begin(env, 0, &txn_master, 0); assert(r == 0);
        DB_TXN *txn;
        r = env->txn_begin(env, txn_master, &txn, 0); assert(r == 0);
        int i;
        for (i=0; i<3; i++) {
            int k = htonl(i);
            int v = 0;
            DBT key, val;
            r = db->put(db, txn, dbt_init(&key, &k, sizeof k), dbt_init(&val, &v, sizeof v), 0);
            assert(r == 0);
        }
        r = txn->commit(txn, 0); assert(r == 0);
        r = txn_master->commit(txn_master, 0); assert(r == 0);
    }

    /* update, rollback */
    {    
        DB_TXN *txn_master;
        r = env->txn_begin(env, 0, &txn_master, 0); assert(r == 0);
        DB_TXN *txn;
        r = env->txn_begin(env, txn_master, &txn, 0); assert(r == 0);
        DBC *cursor;
        r = db->cursor(db, txn, &cursor, 0); assert(r == 0);
        DBT key, val;
        r = cursor->c_get(cursor, dbt_init_malloc(&key), dbt_init_malloc(&val), DB_NEXT); assert(r == 0);
        *(char*)val.data = 1;
        r = db->put(db, txn, &key, &val, 0); assert(r == 0);
        r = cursor->c_close(cursor); assert(r == 0);
        toku_free(key.data); toku_free(val.data);
        r = txn->commit(txn, 0); assert(r == 0);
        r = txn_master->abort(txn_master); assert(r == 0);
    }

    /* delete, rollback */
    {    
        DB_TXN *txn_master;
        r = env->txn_begin(env, 0, &txn_master, 0); assert(r == 0);
        DB_TXN *txn;
        r = env->txn_begin(env, txn_master, &txn, 0); assert(r == 0);
        DBC *cursor;
        r = db->cursor(db, txn, &cursor, 0); assert(r == 0);
        DBT key, val;
        r = cursor->c_get(cursor, dbt_init_malloc(&key), dbt_init_malloc(&val), DB_NEXT); assert(r == 0);
        r = db->del(db, txn, &key, DB_DELETE_ANY); assert(r == 0);
        r = cursor->c_close(cursor); assert(r == 0);
        toku_free(key.data); toku_free(val.data);
        r = txn->commit(txn, 0); assert(r == 0);
        r = txn_master->abort(txn_master); assert(r == 0);
    }

    /* update, commit */
    {    
        DB_TXN *txn_master;
        r = env->txn_begin(env, 0, &txn_master, 0); assert(r == 0);
        DB_TXN *txn;
        r = env->txn_begin(env, txn_master, &txn, 0); assert(r == 0);
        DBC *cursor;
        r = db->cursor(db, txn, &cursor, 0); assert(r == 0);
        DBT key, val;
        r = cursor->c_get(cursor, dbt_init_malloc(&key), dbt_init_malloc(&val), DB_NEXT); assert(r == 0);
        *(char*)val.data = 2;
        r = db->put(db, txn, &key, &val, 0); assert(r == 0);
        r = cursor->c_close(cursor); assert(r == 0);
        toku_free(key.data); toku_free(val.data);
        r = txn->commit(txn, 0); assert(r == 0);
        r = txn_master->commit(txn_master, 0); assert(r == 0);
    }

    /* delete, commit */
    {    
        DB_TXN *txn_master;
        r = env->txn_begin(env, 0, &txn_master, 0); assert(r == 0);
        DB_TXN *txn;
        r = env->txn_begin(env, txn_master, &txn, 0); assert(r == 0);
        DBC *cursor;
        r = db->cursor(db, txn, &cursor, 0); assert(r == 0);
        DBT key, val;
        r = cursor->c_get(cursor, dbt_init_malloc(&key), dbt_init_malloc(&val), DB_NEXT); assert(r == 0);
        r = db->del(db, txn, &key, DB_DELETE_ANY); assert(r == 0);
        r = cursor->c_close(cursor); assert(r == 0);
        toku_free(key.data); toku_free(val.data);
        r = txn->commit(txn, 0); assert(r == 0);
        r = txn_master->commit(txn_master, 0); assert(r == 0);
    }

    /* close db */
    r = db->close(db, 0); assert(r == 0);

    /* close env */
    r = env->close(env, 0); assert(r == 0);
}

extern "C" int test_test_789(void);
int test_test_789(void) {

    pre_setup();
    test_789();
    post_teardown();

    return 0;
}
