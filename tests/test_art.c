#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "../deps/check-0.9.8/src/check.h"

#include "../src/art.h"

START_TEST(test_artInit_and_destroy) {
    art *t = artNew();
    artInit(t);
    fail_unless(artCount(t) == 0);

    artFree(t);
}
END_TEST

START_TEST(test_artInsert) {
    art *t = artNew();

    int len;
    char buf[512];
    FILE *f = fopen("tests/words.txt", "r");

    uintptr_t line = 1;
    while (fgets(buf, sizeof buf, f)) {
        len = strlen(buf);
        buf[len - 1] = '\0';
        fail_unless(true == artInsert(t, buf, len, (void *)line, NULL));
        fail_unless(artCount(t) == line);
        line++;
    }

    artFree(t);
}
END_TEST

START_TEST(test_artInsert_verylong) {
    art *t = artNew();

    unsigned char key1[300] = {
        16,  0,   0,   0,   7,   10,  0,   0,   0,   2,   17,  10,  0,   0,
        0,   120, 10,  0,   0,   0,   120, 10,  0,   0,   0,   216, 10,  0,
        0,   0,   202, 10,  0,   0,   0,   194, 10,  0,   0,   0,   224, 10,
        0,   0,   0,   230, 10,  0,   0,   0,   210, 10,  0,   0,   0,   206,
        10,  0,   0,   0,   208, 10,  0,   0,   0,   232, 10,  0,   0,   0,
        124, 10,  0,   0,   0,   124, 2,   16,  0,   0,   0,   2,   12,  185,
        89,  44,  213, 251, 173, 202, 211, 95,  185, 89,  110, 118, 251, 173,
        202, 199, 101, 0,   8,   18,  182, 92,  236, 147, 171, 101, 150, 195,
        112, 185, 218, 108, 246, 139, 164, 234, 195, 58,  177, 0,   8,   16,
        0,   0,   0,   2,   12,  185, 89,  44,  213, 251, 173, 202, 211, 95,
        185, 89,  110, 118, 251, 173, 202, 199, 101, 0,   8,   18,  180, 93,
        46,  151, 9,   212, 190, 95,  102, 178, 217, 44,  178, 235, 29,  190,
        218, 8,   16,  0,   0,   0,   2,   12,  185, 89,  44,  213, 251, 173,
        202, 211, 95,  185, 89,  110, 118, 251, 173, 202, 199, 101, 0,   8,
        18,  180, 93,  46,  151, 9,   212, 190, 95,  102, 183, 219, 229, 214,
        59,  125, 182, 71,  108, 180, 220, 238, 150, 91,  117, 150, 201, 84,
        183, 128, 8,   16,  0,   0,   0,   2,   12,  185, 89,  44,  213, 251,
        173, 202, 211, 95,  185, 89,  110, 118, 251, 173, 202, 199, 101, 0,
        8,   18,  180, 93,  46,  151, 9,   212, 190, 95,  108, 176, 217, 47,
        50,  219, 61,  134, 207, 97,  151, 88,  237, 246, 208, 8,   18,  255,
        255, 255, 219, 191, 198, 134, 5,   223, 212, 72,  44,  208, 250, 180,
        14,  1,   0,   0,   8,   '\0'};
    unsigned char key2[303] = {
        16,  0,   0,   0,   7,   10,  0,   0,   0,   2,   17,  10,  0,   0,
        0,   120, 10,  0,   0,   0,   120, 10,  0,   0,   0,   216, 10,  0,
        0,   0,   202, 10,  0,   0,   0,   194, 10,  0,   0,   0,   224, 10,
        0,   0,   0,   230, 10,  0,   0,   0,   210, 10,  0,   0,   0,   206,
        10,  0,   0,   0,   208, 10,  0,   0,   0,   232, 10,  0,   0,   0,
        124, 10,  0,   0,   0,   124, 2,   16,  0,   0,   0,   2,   12,  185,
        89,  44,  213, 251, 173, 202, 211, 95,  185, 89,  110, 118, 251, 173,
        202, 199, 101, 0,   8,   18,  182, 92,  236, 147, 171, 101, 150, 195,
        112, 185, 218, 108, 246, 139, 164, 234, 195, 58,  177, 0,   8,   16,
        0,   0,   0,   2,   12,  185, 89,  44,  213, 251, 173, 202, 211, 95,
        185, 89,  110, 118, 251, 173, 202, 199, 101, 0,   8,   18,  180, 93,
        46,  151, 9,   212, 190, 95,  102, 178, 217, 44,  178, 235, 29,  190,
        218, 8,   16,  0,   0,   0,   2,   12,  185, 89,  44,  213, 251, 173,
        202, 211, 95,  185, 89,  110, 118, 251, 173, 202, 199, 101, 0,   8,
        18,  180, 93,  46,  151, 9,   212, 190, 95,  102, 183, 219, 229, 214,
        59,  125, 182, 71,  108, 180, 220, 238, 150, 91,  117, 150, 201, 84,
        183, 128, 8,   16,  0,   0,   0,   3,   12,  185, 89,  44,  213, 251,
        133, 178, 195, 105, 183, 87,  237, 150, 155, 165, 150, 229, 97,  182,
        0,   8,   18,  161, 91,  239, 50,  10,  61,  150, 223, 114, 179, 217,
        64,  8,   12,  186, 219, 172, 150, 91,  53,  166, 221, 101, 178, 0,
        8,   18,  255, 255, 255, 219, 191, 198, 134, 5,   208, 212, 72,  44,
        208, 250, 180, 14,  1,   0,   0,   8,   '\0'};

    fail_unless(true == artInsert(t, key1, 299, (void *)key1, NULL));
    fail_unless(true == artInsert(t, key2, 302, (void *)key2, NULL));
    artInsert(t, key2, 302, (void *)key2, NULL);
    fail_unless(artCount(t) == 2);

    artFree(t);
}
END_TEST

START_TEST(test_artInsert_search) {
    art *t = artNew();

    int len;
    char buf[512];
    FILE *f = fopen("tests/words.txt", "r");

    uintptr_t line = 1;
    while (fgets(buf, sizeof buf, f)) {
        len = strlen(buf);
        buf[len - 1] = '\0';
        fail_unless(true == artInsert(t, buf, len, (void *)line, NULL));
        line++;
    }

    // Seek back to the start
    fseek(f, 0, SEEK_SET);

    // Search for each line
    line = 1;
    while (fgets(buf, sizeof buf, f)) {
        len = strlen(buf);
        buf[len - 1] = '\0';

        void *val = NULL;
        artSearch(t, buf, len, &val);
        fail_unless(line == (uintptr_t)val,
                    "Line: %d Val: %" PRIuPTR " Str: %s\n", line, val, buf);
        line++;
    }

    // Check the minimum
    artLeaf *l = artMinimum(t);
    fail_unless(l && strcmp((char *)artLeafKeyOnly(l), "A") == 0);

    // Check the maximum
    l = artMaximum(t);
    fail_unless(l && strcmp((char *)artLeafKeyOnly(l), "zythum") == 0);

    artFree(t);
}
END_TEST

START_TEST(test_artInsert_delete) {
    art *t = artNew();

    int len;
    char buf[512];
    FILE *f = fopen("tests/words.txt", "r");

    uintptr_t line = 1, nlines;
    while (fgets(buf, sizeof buf, f)) {
        len = strlen(buf);
        buf[len - 1] = '\0';
        fail_unless(true == artInsert(t, buf, len, (void *)line, NULL));
        line++;
    }

    nlines = line - 1;

    // Seek back to the start
    fseek(f, 0, SEEK_SET);

    // Search for each line
    line = 1;
    while (fgets(buf, sizeof buf, f)) {
        len = strlen(buf);
        buf[len - 1] = '\0';

        // Search first, ensure all entries still
        // visible
        void *val = NULL;
        artSearch(t, buf, len, &val);
        fail_unless(line == (uintptr_t)val,
                    "Line: %d Val: %" PRIuPTR " Str: %s\n", line, val, buf);

        // Delete, should get lineno back
        artDelete(t, buf, len, &val);
        fail_unless(line == (uintptr_t)val,
                    "Line: %d Val: %" PRIuPTR " Str: %s\n", line, val, buf);

        // Check the size
        fail_unless(artCount(t) == nlines - line);
        line++;
    }

    // Check the minimum and maximum
    fail_unless(!artMinimum(t));
    fail_unless(!artMaximum(t));

    artFree(t);
}
END_TEST

START_TEST(test_artInsert_random_delete) {
    art *t = artNew();

    int len;
    char buf[512];
    FILE *f = fopen("tests/words.txt", "r");

    uintptr_t line = 1;
    while (fgets(buf, sizeof buf, f)) {
        len = strlen(buf);
        buf[len - 1] = '\0';
        fail_unless(true == artInsert(t, buf, len, (void *)line, NULL));
        line++;
    }

    // Can be improved ensuring one delete on each node type
    // A is in 48 node
    uintptr_t lineno = 1;
    // Search first, ensure all entries are visible
    void *val = NULL;
    artSearch(t, "A", strlen("A") + 1, &val);
    fail_unless(lineno == (uintptr_t)val,
                "Line: %d Val: %" PRIuPTR " Str: %s\n", lineno, val, "A");

    // Delete a single entry, should get lineno back
    val = NULL;
    artDelete(t, "A", strlen("A") + 1, &val);
    fail_unless(lineno == (uintptr_t)val,
                "Line: %d Val: %" PRIuPTR " Str: %s\n", lineno, val, "A");

    // Ensure  the entry is no longer visible
    val = NULL;
    artSearch(t, "A", strlen("A") + 1, &val);
    fail_unless(0 == (uintptr_t)val, "Line: %d Val: %" PRIuPTR " Str: %s\n", 0,
                val, "A");

    artFree(t);
}
END_TEST

int iter_cb(void *data, const void *key_, uint32_t key_len, void *val) {
    uint64_t *out = (uint64_t *)data;
    uintptr_t line = (uintptr_t)val;
    const uint8_t *restrict key = key_;
    uint64_t mask = (line * (key[0] + key_len));
    out[0]++;
    out[1] ^= mask;
    return 0;
}

START_TEST(test_artInsert_iter) {
    art *t = artNew();

    int len;
    char buf[512];
    FILE *f = fopen("tests/words.txt", "r");

    uint64_t xor_mask = 0;
    uintptr_t line = 1, nlines;
    while (fgets(buf, sizeof buf, f)) {
        len = strlen(buf);
        buf[len - 1] = '\0';
        fail_unless(true == artInsert(t, buf, len, (void *)line, NULL));

        xor_mask ^= (line * (buf[0] + len));
        line++;
    }
    nlines = line - 1;

    uint64_t out[] = {0, 0};
    fail_unless(artIter(t, iter_cb, &out) == 0);

    fail_unless(out[0] == nlines);
    fail_unless(out[1] == xor_mask);

    artFree(t);
}
END_TEST

typedef struct {
    int count;
    int max_count;
    const char **expected;
} prefix_data;

static int test_prefix_cb(void *data, const void *k, uint32_t k_len,
                          void *val) {
    prefix_data *p = (prefix_data *)data;
    fail_unless(p->count < p->max_count);
    fail_unless(memcmp(k, p->expected[p->count], k_len) == 0,
                "Key: %s Expect: %s", k, p->expected[p->count]);
    p->count++;
    return 0;
}

START_TEST(test_artIterPrefix) {
    art *t = artNew();

    const char *s = "api.foo.bar";
    fail_unless(true == artInsert(t, s, strlen(s) + 1, NULL, NULL));

    s = "api.foo.baz";
    fail_unless(true == artInsert(t, s, strlen(s) + 1, NULL, NULL));

    s = "api.foe.fum";
    fail_unless(true == artInsert(t, s, strlen(s) + 1, NULL, NULL));

    s = "abc.123.456";
    fail_unless(true == artInsert(t, s, strlen(s) + 1, NULL, NULL));

    s = "api.foo";
    fail_unless(true == artInsert(t, s, strlen(s) + 1, NULL, NULL));

    s = "api";
    fail_unless(true == artInsert(t, s, strlen(s) + 1, NULL, NULL));

    // Iterate over api
    const char *expected[] = {"api", "api.foe.fum", "api.foo", "api.foo.bar",
                              "api.foo.baz"};
    prefix_data p = {0, 5, expected};
    fail_unless(!artIterPrefix(t, "api", 3, test_prefix_cb, &p));
    fail_unless(p.count == p.max_count, "Count: %d Max: %d", p.count,
                p.max_count);

    // Iterate over 'a'
    const char *expected2[] = {"abc.123.456", "api",         "api.foe.fum",
                               "api.foo",     "api.foo.bar", "api.foo.baz"};
    prefix_data p2 = {0, 6, expected2};
    fail_unless(!artIterPrefix(t, "a", 1, test_prefix_cb, &p2));
    fail_unless(p2.count == p2.max_count);

    // Check a failed iteration
    prefix_data p3 = {0, 0, NULL};
    fail_unless(!artIterPrefix(t, "b", 1, test_prefix_cb, &p3));
    fail_unless(p3.count == 0);

    // Iterate over api.
    const char *expected4[] = {"api.foe.fum", "api.foo", "api.foo.bar",
                               "api.foo.baz"};
    prefix_data p4 = {0, 4, expected4};
    fail_unless(!artIterPrefix(t, "api.", 4, test_prefix_cb, &p4));
    fail_unless(p4.count == p4.max_count, "Count: %d Max: %d", p4.count,
                p4.max_count);

    // Iterate over api.foo.ba
    const char *expected5[] = {"api.foo.bar"};
    prefix_data p5 = {0, 1, expected5};
    fail_unless(!artIterPrefix(t, "api.foo.bar", 11, test_prefix_cb, &p5));
    fail_unless(p5.count == p5.max_count, "Count: %d Max: %d", p5.count,
                p5.max_count);

    // Check a failed iteration on api.end
    prefix_data p6 = {0, 0, NULL};
    fail_unless(!artIterPrefix(t, "api.end", 7, test_prefix_cb, &p6));
    fail_unless(p6.count == 0);

    // Iterate over empty prefix
    prefix_data p7 = {0, 6, expected2};
    fail_unless(!artIterPrefix(t, "", 0, test_prefix_cb, &p7));
    fail_unless(p7.count == p7.max_count);

    artFree(t);
}
END_TEST

START_TEST(test_artLong_prefix) {
    art *t = artNew();

    uintptr_t v;
    const char *s;

    s = "this:key:has:a:long:prefix:3";
    v = 3;
    fail_unless(true == artInsert(t, s, strlen(s) + 1, (void *)v, NULL));

    s = "this:key:has:a:long:common:prefix:2";
    v = 2;
    fail_unless(true == artInsert(t, s, strlen(s) + 1, (void *)v, NULL));

    s = "this:key:has:a:long:common:prefix:1";
    v = 1;
    fail_unless(true == artInsert(t, s, strlen(s) + 1, (void *)v, NULL));

    // Search for the keys
    s = "this:key:has:a:long:common:prefix:1";
    void *val = NULL;
    artSearch(t, s, strlen(s) + 1, &val);
    fail_unless(1 == (uintptr_t)val);

    s = "this:key:has:a:long:common:prefix:2";
    artSearch(t, s, strlen(s) + 1, &val);
    fail_unless(2 == (uintptr_t)val);

    s = "this:key:has:a:long:prefix:3";
    artSearch(t, s, strlen(s) + 1, &val);
    fail_unless(3 == (uintptr_t)val);

    const char *expected[] = {
        "this:key:has:a:long:common:prefix:1",
        "this:key:has:a:long:common:prefix:2",
        "this:key:has:a:long:prefix:3",
    };
    prefix_data p = {0, 3, expected};
    fail_unless(!artIterPrefix(t, "this:key:has", 12, test_prefix_cb, &p));
    fail_unless(p.count == p.max_count, "Count: %d Max: %d", p.count,
                p.max_count);

    artFree(t);
}
END_TEST

START_TEST(test_artInsert_search_uuid) {
    art *t = artNew();

    int len;
    char buf[512];
    FILE *f = fopen("tests/uuid.txt", "r");

    uintptr_t line = 1;
    while (fgets(buf, sizeof buf, f)) {
        len = strlen(buf);
        buf[len - 1] = '\0';
        fail_unless(true == artInsert(t, buf, len, (void *)line, NULL));
        line++;
    }

    // Seek back to the start
    fseek(f, 0, SEEK_SET);

    // Search for each line
    line = 1;
    while (fgets(buf, sizeof buf, f)) {
        len = strlen(buf);
        buf[len - 1] = '\0';

        void *val = NULL;
        artSearch(t, buf, len, &val);
        fail_unless(line == (uintptr_t)val,
                    "Line: %d Val: %" PRIuPTR " Str: %s\n", line, val, buf);
        line++;
    }

    // Check the minimum
    artLeaf *l = artMinimum(t);
    fail_unless(l && strcmp((char *)artLeafKeyOnly(l),
                            "00026bda-e0ea-4cda-8245-522764e9f325") == 0);

    // Check the maximum
    l = artMaximum(t);
    fail_unless(l && strcmp((char *)artLeafKeyOnly(l),
                            "ffffcb46-a92e-4822-82af-a7190f9c1ec5") == 0);

    artFree(t);
}
END_TEST

START_TEST(test_artMax_prefix_len_scan_prefix) {
    art *t = artNew();

    char *key1 = "foobarbaz1-test1-foo";
    fail_unless(true == artInsert(t, key1, strlen(key1) + 1, NULL, NULL));

    char *key2 = "foobarbaz1-test1-bar";
    fail_unless(true == artInsert(t, key2, strlen(key2) + 1, NULL, NULL));

    char *key3 = "foobarbaz1-test2-foo";
    fail_unless(true == artInsert(t, key3, strlen(key3) + 1, NULL, NULL));

    fail_unless(artCount(t) == 3);

    // Iterate over api
    const char *expected[] = {key2, key1};
    prefix_data p = {0, 2, expected};
    char *prefix = "foobarbaz1-test1";
    fail_unless(!artIterPrefix(t, prefix, strlen(prefix), test_prefix_cb, &p));
    fail_unless(p.count == p.max_count, "Count: %d Max: %d", p.count,
                p.max_count);

    artFree(t);
}
END_TEST
