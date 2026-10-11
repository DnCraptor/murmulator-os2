#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <system/ver.h>
#include <system/cmd.h>
#include <system/clock.h>

// the kernel clamps to the same range, here it is checked to report a mistype
#define VREG_MIN_MV 850
#define VREG_MAX_MV 1700

int __required_m_api_verion(void) {
    return M_API_VERSION;
}

static void show(void) {
    int mv = get_vreg_mv();
    fprintf(stdout, "CPU: %lu MHz; VREG: %d.%02d V (%s)\n",
            (unsigned long)(get_overclocking_khz() / 1000),
            mv / 1000, (mv % 1000) / 10,
            get_vreg_override_mv() ? "fixed" : "auto");
}

static int is_auto(const char* s) {
    return (s[0] == 'a' || s[0] == 'A') && (s[1] == 'u' || s[1] == 'U') &&
           (s[2] == 't' || s[2] == 'T') && (s[3] == 'o' || s[3] == 'O') && !s[4];
}

// "1.60", "1600", "auto"; returns -1 for a wrong value
static int vreg_arg(const char* s) {
    int mv = parse_vreg_mv(s);
    if (mv == 0 && strcmp(s, "0") != 0 && !is_auto(s)) return -1;
    if (mv && (mv < VREG_MIN_MV || mv > VREG_MAX_MV)) return -1;
    return mv;
}

static int usage(void) {
    fprintf(stderr,
           "Use:\n"
           " cpu              - show current settings;\n"
           " cpu MHZ [V]      - set freq. in MHz (e.g. 504), optionally the core voltage;\n"
           " cpu -v V         - set the core voltage only;\n"
           "   V: 1.60 (volts), 1600 (mV) or auto (by freq.), %d.%02d..%d.%02d V;\n"
           " cpu VCO DIV1 DIV2 - set PLL directly (VCO in Hz, no voltage/timings change).\n"
           "Note: video/audio keep their dividers until restart, so prefer\n"
           "      CPU=MHZ and VREG=V in config.sys.\n",
           VREG_MIN_MV / 1000, (VREG_MIN_MV % 1000) / 10, VREG_MAX_MV / 1000, (VREG_MAX_MV % 1000) / 10);
    return 1;
}

int main(int argc, char** argv) {
    if (argc == 1) {
        show();
        return 0;
    }
    if (argc == 3 && strcmp(argv[1], "-v") == 0) {
        int mv = vreg_arg(argv[2]);
        if (mv < 0) {
            fprintf(stderr, "Unsupported core voltage: %s\n", argv[2]);
            return usage();
        }
        set_vreg_mv(mv);
        overclocking(); // the same freq., new voltage
        show();
        return 0;
    }
    if (argc == 2 || argc == 3) {
        int cpu = atoi(argv[1]);
        if (cpu < 50 || cpu > 800) {
            fprintf(stderr, "Unable to change CPU freq. to %s\n", argv[1]);
            return usage();
        }
        if (argc == 3) {
            int mv = vreg_arg(argv[2]);
            if (mv < 0) {
                fprintf(stderr, "Unsupported core voltage: %s\n", argv[2]);
                return usage();
            }
            set_vreg_mv(mv);
        }
        set_overclocking(cpu * 1000);
        overclocking(); // voltage, flash/PSRAM timings and PLL in the safe order
        if (get_overclocking_khz() != (uint32_t)cpu * 1000) {
            fprintf(stderr, "CPU freq. %d MHz is not achievable by PLL\n", cpu);
        }
        show();
        return 0;
    }
    if (argc == 4) {
        uint32_t vco = atoi(argv[1]);
        uint32_t div1 = atoi(argv[2]);
        uint32_t div2 = atoi(argv[3]);
        fprintf(stdout, "Attempt to set VCO: %lu Hz; DIV1: %lu; DIV2: %lu\n",
                (unsigned long)vco, (unsigned long)div1, (unsigned long)div2);
        set_sys_clock_pll(vco, div1, div2);
        return 0;
    }
    return usage();
}
