/* Memory-only classic Macintosh container reader. GPL-3.0-or-later. */
#ifndef MAC_ARCHIVE_H
#define MAC_ARCHIVE_H
#include <stddef.h>
typedef int (*MacArchiveVisitor)(const char *name, const char type[4],
    const unsigned char *data, size_t data_size,
    const unsigned char *resource, size_t resource_size, void *context);
int mac_archive_kind(const unsigned char *data, size_t size, const char *name);
int mac_archive_read(const unsigned char *data, size_t size, const char *name,
    MacArchiveVisitor visit, void *context, char *error, size_t error_size);
#endif
