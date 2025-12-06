#include "errno.h"
#include "unistd.h"
#include "spawn.h"
#include "cmd.h"
#include "sys_table.h"

pid_t __fork(void) {
    // unsupported, use posix_spawn
    errno = ENOSYS;
    return -1;
}

int __execve(const char *pathname, char *const argv[], char *const envp[]) {
    /// TODO:
    errno = ENOSYS;
    return -1;
}

pid_t __getpid(void) {
    const TaskHandle_t th = xTaskGetCurrentTaskHandle();
    return (pid_t)(intptr_t)th;
}

int __posix_spawn(
    pid_t *pid_out,
    const char *path,
    const posix_spawn_file_actions_t *actions,
    const posix_spawnattr_t *attr,
    char *const argv[],
    char *const envp[]
) {
    cmd_ctx_t* parent = get_cmd_ctx();
    cmd_ctx_t* child  = clone_ctx(parent);

//    apply_file_actions(child, actions);
//    if (envp)
  ///      override_child_env(child, envp);
/** TODO:
    xTaskCreate(
        vProcessTask,
        path,
        STACK_SIZE,
        child,
        PRIORITY,
        &child->task
    );
    if (pid_out)
        *pid_out = pid;
        */
    return 0;
}
