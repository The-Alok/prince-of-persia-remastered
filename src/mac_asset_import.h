/* Prince of Persia Remastered. GPL-3.0-or-later; see COPYING. */
#ifndef MAC_ASSET_IMPORT_H
#define MAC_ASSET_IMPORT_H
#include <stddef.h>

/* Find .hfs/.dsk/.dc42 files beside the launcher; caller frees result. */
int mac_import_find_disk(const char *base, char **result);
/* Read-only disk import with atomic, no-overwrite installation. */
int mac_import_disk(const char *disk, const char *pack, char *error, size_t error_size);
/* 1: structurally valid native pack; 0: absent; -1: incomplete/invalid. */
int mac_import_pack_status(const char *pack);
#endif
