#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "apps/mp3wav/mp3dec.h"

START_TEST(test_mainbuf_bounds_invariant)
{
    // Invariant: mainDataBegin + nSlots and mainDataBytes + nSlots must not exceed MAINBUF_SIZE
    const struct {
        int mainDataBegin;
        int mainDataBytes;
        int nSlots;
    } payloads[] = {
        // Exploit case: values that would overflow mainBuf
        {MAINBUF_SIZE - 100, MAINBUF_SIZE - 50, 200},
        // Boundary case: exactly at limit
        {MAINBUF_SIZE - 100, MAINBUF_SIZE - 100, 100},
        // Valid case: well within bounds
        {100, 200, 50}
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        MP3DecInfo info;
        unsigned char inbuf[MAINBUF_SIZE] = {0};
        unsigned char *inbuf_ptr = inbuf;
        int bytesLeft = MAINBUF_SIZE;
        
        // Initialize decoder state
        memset(&info, 0, sizeof(info));
        info.mainBuf = malloc(MAINBUF_SIZE);
        ck_assert_ptr_nonnull(info.mainBuf);
        
        // Set adversarial values
        info.mainDataBegin = payloads[i].mainDataBegin;
        info.mainDataBytes = payloads[i].mainDataBytes;
        info.nSlots = payloads[i].nSlots;
        
        // Call the actual vulnerable function
        int result = mp3_decode_frame(&info, &inbuf_ptr, &bytesLeft);
        
        // Property: must not write beyond mainBuf bounds
        // If vulnerable, this would crash or corrupt memory
        ck_assert(result == 0 || result == 1); // Valid return codes
        
        free(info.mainBuf);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_mainbuf_bounds_invariant);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}