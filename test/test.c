#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../triemap.h"
#include "minunit.h"
#include "time_sample.h"

void testTrie() {
  TrieMap *tm = NewTrieMap();

  char buf[32];

  for (int i = 0; i < 100; i++) {
    sprintf(buf, "key%d", i);
    int *pi = malloc(sizeof(int));
    *pi = i;
    int rc = TrieMap_Add(tm, buf, strlen(buf), NULL, NULL);
    mu_check(rc);
    rc = TrieMap_Add(tm, buf, strlen(buf), pi, NULL);
    mu_check(rc == 0);
  }
  mu_assert_int_eq(100, tm->cardinality);

  for (int i = 0; i < 100; i++) {
    sprintf(buf, "key%d", i);

    void *p = TrieMap_Find(tm, buf, strlen(buf));
    mu_check(p != NULL);
    mu_check(p != TRIEMAP_NOTFOUND);
    mu_check(*(int *)p == i);
  }

  for (int i = 0; i < 100; i++) {
    sprintf(buf, "key%d", i);

    int rc = TrieMap_Delete(tm, buf, strlen(buf), NULL);
    mu_check(rc);
    rc = TrieMap_Delete(tm, buf, strlen(buf), NULL);
    mu_check(rc == 0);
    mu_check(tm->cardinality == 100 - i - 1);
  }

  TrieMap_Free(tm, NULL);
}

void testTrieIterator() {
  TrieMap *tm = NewTrieMap();

  char buf[32];

  for (int i = 0; i < 100; i++) {
    sprintf(buf, "key%d", i);
    int *pi = malloc(sizeof(int));
    *pi = i;
    TrieMap_Add(tm, buf, strlen(buf), pi, NULL);
  }
  mu_assert_int_eq(100, tm->cardinality);

  TrieMapIterator *it = TrieMap_Iterate(tm, "key1", 4);
  mu_check(it);
  int count = 0;

  char *str = NULL;
  tm_len_t len = 0;
  void *ptr = NULL;

  while (0 != TrieMapIterator_Next(it, &str, &len, &ptr)) {
    mu_check(!strncmp("key1", str, 4));
    mu_check(str);
    mu_check(len > 0);
    mu_check(ptr);
    mu_check(*(int *)ptr > 0);
    count++;
  }
  mu_assert_int_eq(11, count);

  TrieMapIterator_Free(it);
  TrieMap_Free(tm, NULL);
}

int main(int argc, char **argv) {
  MU_RUN_TEST(testTrie);
  MU_RUN_TEST(testTrieIterator);
  MU_REPORT();
  return minunit_status;
}