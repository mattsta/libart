#include "../deps/check-0.9.8/src/check.h"
#include "test_art.c"
#include <stdio.h>
#include <syslog.h>

int main(void) {
    setlogmask(LOG_UPTO(LOG_DEBUG));

    Suite *s1 = suite_create("art");
    TCase *tc1 = tcase_create("art");
    SRunner *sr = srunner_create(s1);
    int nf;

    // Add the art tests
    suite_add_tcase(s1, tc1);
    tcase_add_test(tc1, test_artInit_and_destroy);
    tcase_add_test(tc1, test_artInsert);
    tcase_add_test(tc1, test_artInsert_verylong);
    tcase_add_test(tc1, test_artInsert_search);
    tcase_add_test(tc1, test_artInsert_delete);
    tcase_add_test(tc1, test_artInsert_random_delete);
    tcase_add_test(tc1, test_artInsert_iter);
    tcase_add_test(tc1, test_artIterPrefix);
    tcase_add_test(tc1, test_artLong_prefix);
    tcase_add_test(tc1, test_artInsert_search_uuid);
    tcase_add_test(tc1, test_artMax_prefix_len_scan_prefix);
    tcase_set_timeout(tc1, 180);

    srunner_run_all(sr, CK_ENV);
    nf = srunner_ntests_failed(sr);
    srunner_free(sr);
    return nf == 0 ? 0 : 1;
}
