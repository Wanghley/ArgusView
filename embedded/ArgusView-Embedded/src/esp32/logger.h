#ifndef LOGGER_H
#define LOGGER_H

void log_info(const char *tag, const char *fmt, ...);
void log_error(const char *tag, const char *fmt, ...);
void log_debug(const char *tag, const char *fmt, ...);
void log_warn(const char *tag, const char *fmt, ...);
void logger_init(const char *log_url);

#endif // LOGGER_H
