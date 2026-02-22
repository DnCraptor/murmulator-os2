
#define PIPE_BUF_SIZE 4096  // или брать из конфига?

typedef struct pipe_buf {
    uint8_t  buf[PIPE_BUF_SIZE];
    uint32_t read_pos;
    uint32_t write_pos;
    uint32_t count;          // байт в буфере сейчас
    int      readers;        // счётчик открытых read-концов
    int      writers;        // счётчик открытых write-концов
    SemaphoreHandle_t mutex;
    TaskHandle_t      reader_task;  // для уведомления при записи
    TaskHandle_t      writer_task;  // для уведомления при чтении
    TaskHandle_t poll_task;  // таск ожидающий в poll()
} pipe_buf_t;


typedef struct FDESC_s {
    FIL* fp;
    unsigned int flags;
    char* path;
    pipe_buf_t*  pipe;   // != NULL => это pipe-дескриптор
    int          pipe_end; // 0 = read, 1 = write
} FDESC;
