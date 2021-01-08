/* lightweight logging library */
#ifndef LOG_H_
#define LOG_H_

typedef enum {
    LOG_ERRORS,
    LOG_EVERYTHING
} log_level;

/* other functions */
void log_init(const char* path, const char* filename, const char* file_extension);
void log_set_level(log_level level);
log_level log_get_level();

/* logging functions */
void log_debug(const char* format, ...);
void log_error(const char* format, ...);

/* cleanup / flush */
void log_flush();
void log_close();

#endif /* LOG_H_ */