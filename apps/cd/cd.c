// cd - change the current directory (CD variable of the command context)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <system/ver.h>
#include <system/cmd.h>

int __required_m_api_verion(void) {
    return M_API_VERSION;
}

int main(int argc, char** argv) {
    cmd_ctx_t* ctx = get_cmd_ctx();
    if (argc < 2) { // like DOS: show the current directory
        const char* cd = get_ctx_var(ctx, "CD");
        printf("%s\n", cd && cd[0] ? cd : "/");
        return 0;
    }
    if (argc > 2) {
        fprintf(stderr, "Unable to change directory to more than one target\n");
        return 1;
    }
    // relative names ("src", "..", "./a/../b") are resolved against the current directory
    char* path = (char*)malloc(PATH_MAX + 1);
    if (!path) {
        fprintf(stderr, "Not enough memory\n");
        return 1;
    }
    struct stat st;
    if (!realpath(argv[1], path) || stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Unable to find directory: '%s'\n", argv[1]);
        free(path);
        return 1;
    }
    set_ctx_var(ctx, "CD", path);
    free(path);
    return 0;
}
