#ifndef LOG_H_
#define LOG_H_

/* init function */
void log_init(const char *path, const char *filename, const char *file_extension);

/* logging functions */
void log_print(const char *format, ...);
void log_sync(const char *format, ...);

/* cleanup / flush */
void log_flush();
void log_close();

#endif /* LOG_H_ */