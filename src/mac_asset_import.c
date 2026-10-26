/*
Prince of Persia Remastered: first-launch Macintosh graphics import.
Copyright (C) 2026 The-Alok
SPDX-License-Identifier: GPL-3.0-or-later

Native implementation of tools/build_mac_graphics_pack.py. Contains no game
images or disk data. Input images are read, never mounted or executed. Actor
positions, gameplay and rendering logic are not involved in this startup step.
*/
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include "mac_asset_import.h"
#include "mac_archive.h"
#include <SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#endif

typedef unsigned char Byte;
typedef struct { const Byte *p; size_t n; } Span;
typedef struct { char *text; size_t size; } Error;
typedef struct { unsigned kind, file, first; Byte extents[12]; } Overflow;
typedef struct { unsigned id, data_size, rsrc_size; Byte data_ext[12], rsrc_ext[12]; int color; } GameFile;
typedef struct {
    Span volume;
    unsigned block_size, allocation_start;
    Overflow overflow[4096]; size_t overflow_count;
    GameFile files[64]; size_t file_count;
    Error *error;
} Hfs;
typedef struct {
    char folder[16], ext[4]; unsigned id, w, h, colors, flags;
    Byte *payload; size_t size;
} Entry;

static unsigned be16(const Byte *p) { return (unsigned)p[0] * 256 + p[1]; }
static uint32_t be32(const Byte *p) { return (uint32_t)be16(p) * 65536 + be16(p + 2); }
static unsigned le16(const Byte *p) { return (unsigned)p[1] * 256 + p[0]; }
static uint32_t le32(const Byte *p) { return (uint32_t)le16(p + 2) * 65536 + le16(p); }
static void put16(Byte *p, unsigned v) { p[0] = v; p[1] = v >> 8; }
static void put32(Byte *p, uint32_t v) { put16(p, v); put16(p + 2, v >> 16); }
static int has(Span s, uint64_t at, uint64_t n) { return at <= s.n && n <= s.n - at; }
static Span part(Span s, size_t at, size_t n) { Span r = {s.p + at, n}; return r; }
static int fail(Error *e, const char *message) {
    if (e && e->size) SDL_snprintf(e->text, e->size, "%s", message);
    return 0;
}

static Byte *read_file(const char *name, size_t limit, size_t *size, Error *e) {
    SDL_RWops *rw = SDL_RWFromFile(name, "rb");
    if (!rw) { fail(e, "Cannot open the file for reading."); return NULL; }
    Sint64 n = SDL_RWsize(rw);
    if (n <= 0 || (uint64_t)n > limit) {
        SDL_RWclose(rw); fail(e, "The image is empty, too large, or cannot be read."); return NULL;
    }
    Byte *p = malloc((size_t)n);
    if (!p) { SDL_RWclose(rw); fail(e, "Not enough memory to read the image."); return NULL; }
    size_t got = SDL_RWread(rw, p, 1, (size_t)n);
    SDL_RWclose(rw);
    if (got != (size_t)n) { free(p); fail(e, "The image could not be read completely."); return NULL; }
    *size = (size_t)n;
    return p;
}

static int volume_of(Span image, Span *out, Error *e) {
    if (has(image, 1024, 2) && !memcmp(image.p + 1024, "BD", 2)) { *out = image; return 1; }
    if (has(image, 0, 84) && image.p[0] <= 63 && be16(image.p + 82) == 256) {
        uint32_t n = be32(image.p + 64), tags = be32(image.p + 68);
        if ((uint64_t)84 + n + tags != image.n) return fail(e, "The Disk Copy image is incomplete.");
        Span v = part(image, 84, n);
        if (has(v, 1024, 2) && !memcmp(v.p + 1024, "BD", 2)) { *out = v; return 1; }
    }
    unsigned block = has(image, 0, 4) && !memcmp(image.p, "ER", 2) ? be16(image.p + 2) : 512;
    if (block < 512 || block > 65536 || (block & (block - 1))) return fail(e, "Invalid partition block size.");
    if (has(image, block, 512) && !memcmp(image.p + block, "PM", 2)) {
        uint32_t count = be32(image.p + block + 4); int found = 0;
        if (!count || count >= image.n / block) return fail(e, "Invalid partition map.");
        for (uint32_t i = 1; i <= count; i++) {
            Span p = part(image, (size_t)i * block, 512);
            if (memcmp(p.p, "PM", 2)) return fail(e, "Invalid partition entry.");
            if (!memcmp(p.p + 48, "Apple_HFS", 9) && p.p[57] == 0) {
                uint64_t at = (uint64_t)be32(p.p + 8) * block, n = (uint64_t)be32(p.p + 12) * block;
                if (!has(image, at, n)) return fail(e, "The HFS partition is incomplete.");
                Span v = part(image, (size_t)at, (size_t)n);
                if (has(v, 1024, 2) && !memcmp(v.p + 1024, "BD", 2)) { *out = v; found++; }
            }
        }
        if (found == 1) return 1;
        if (found > 1) return fail(e, "Multiple HFS volumes: export the game volume as one .hfs image.");
    }
    return fail(e, "No classic HFS volume found. Use a Macintosh colour-game .hfs or .dsk (raw HFS, Disk Copy 4.2 or partitioned HFS).");
}

typedef int (*Visitor)(Span, void *);
static int leaves(Span tree, Visitor visit, void *context, Error *e) {
    if (!has(tree, 0, 34)) return fail(e, "Incomplete HFS catalog.");
    unsigned size = be16(tree.p + 32); uint32_t node = be32(tree.p + 24);
    if (size < 256 || (size & (size - 1))) return fail(e, "Invalid HFS node size.");
    size_t nodes = tree.n / size;
    Byte *seen = calloc(nodes ? nodes : 1, 1);
    if (!seen) return fail(e, "Not enough memory for the HFS catalog.");
    int ok = 1;
    while (node && ok) {
        if (node >= nodes || seen[node]) { ok = fail(e, "Invalid HFS leaf chain."); break; }
        seen[node] = 1; Span raw = part(tree, (size_t)node * size, size);
        unsigned count = be16(raw.p + 10);
        if (raw.p[8] != 255 || 2 * (count + 1) > size - 14) { ok = fail(e, "Invalid HFS leaf."); break; }
        for (unsigned i = 0; i <= count; i++) {
            unsigned at = be16(raw.p + size - 2 * (i + 1));
            if (at < 14 || at > size - 2 * (count + 1)) { ok = fail(e, "Invalid HFS record offset."); break; }
            if (i < count) {
                unsigned end = be16(raw.p + size - 2 * (i + 2));
                if (end < at || end > size - 2 * (count + 1)) { ok = fail(e, "Invalid HFS record length."); break; }
                if (!visit(part(raw, at, end - at), context)) { ok = 0; break; }
            }
        }
        node = be32(raw.p);
    }
    free(seen); return ok;
}

static Byte *fork_of(Hfs *h, unsigned length, const Byte initial[12], unsigned file, unsigned kind) {
    if (length > 64u * 1024 * 1024) { fail(h->error, "A game resource fork is unreasonably large."); return NULL; }
    Byte *out = malloc(length ? length : 1); size_t copied = 0; unsigned blocks = 0;
    const Byte *extents = initial;
    if (!out) { fail(h->error, "Not enough memory for game resources."); return NULL; }
    while (copied < length) {
        unsigned before = blocks;
        for (unsigned i = 0; i < 3 && copied < length; i++) {
            unsigned block = be16(extents + i * 4), count = be16(extents + i * 4 + 2);
            uint64_t at = h->allocation_start + (uint64_t)block * h->block_size, n = (uint64_t)count * h->block_size;
            if (!has(h->volume, at, n)) { fail(h->error, "An HFS file extends beyond the image."); goto bad; }
            size_t take = n < length - copied ? (size_t)n : length - copied;
            memcpy(out + copied, h->volume.p + (size_t)at, take); copied += take; blocks += count;
        }
        if (copied == length) break;
        if (blocks == before || blocks > 65535) { fail(h->error, "Invalid HFS file extents."); goto bad; }
        extents = NULL;
        for (size_t i = 0; i < h->overflow_count; i++) {
            Overflow *x = &h->overflow[i];
            if (x->kind == kind && x->file == file && x->first == blocks) { extents = x->extents; break; }
        }
        if (!extents) { fail(h->error, "Required HFS overflow extents are missing."); goto bad; }
    }
    return out;
bad:
    free(out); return NULL;
}

static int overflow_record(Span raw, void *context) {
    Hfs *h = context;
    if (!has(raw, 0, 20) || raw.p[0] != 7 || h->overflow_count == 4096) return fail(h->error, "Unsupported HFS overflow table.");
    Overflow *x = &h->overflow[h->overflow_count++];
    x->kind = raw.p[1]; x->file = be32(raw.p + 2); x->first = be16(raw.p + 6);
    memcpy(x->extents, raw.p + 8, 12); return 1;
}

static int catalog_record(Span raw, void *context) {
    Hfs *h = context;
    if (!has(raw, 0, 1)) return fail(h->error, "Empty HFS catalog record.");
    unsigned key = raw.p[0], at = (key + 2) & ~1u;
    if (key < 6 || !has(raw, 0, at + 1) || raw.p[6] > key - 6) return fail(h->error, "Invalid HFS catalog key.");
    if (raw.p[at] != 2) return 1;
    if (!has(raw, at, 102)) return fail(h->error, "Incomplete HFS file record.");
    const Byte *r = raw.p + at;
    int color = raw.p[6] == 13 && !memcmp(raw.p + 7, "Persia(COLOR)", 13);
    /* Persia(COLOR) is 13 bytes, including the closing parenthesis. */
    if (!color && memcmp(r + 4, "APPL", 4)) return 1;
    if (h->file_count == 64) return fail(h->error, "Too many candidate applications on this disk.");
    GameFile *f = &h->files[h->file_count++];
    f->color = color; f->id = be32(r + 20); f->data_size = be32(r + 26); f->rsrc_size = be32(r + 36);
    memcpy(f->data_ext, r + 74, 12); memcpy(f->rsrc_ext, r + 86, 12); return 1;
}

static int resource(Span fork, const char kind[4], int id, Span *out) {
    if (!has(fork, 0, 16)) return 0;
    uint32_t data = be32(fork.p), map = be32(fork.p + 4), data_size = be32(fork.p + 8), map_size = be32(fork.p + 12);
    if (!has(fork, data, data_size) || !has(fork, map, map_size) || map_size < 28) return 0;
    Span rm = part(fork, map, map_size); unsigned type_at = be16(rm.p + 24);
    if (!has(rm, type_at, 2)) return 0;
    Span types = part(rm, type_at, rm.n - type_at); unsigned count = be16(types.p) + 1;
    if (count > 4096 || !has(types, 2, (uint64_t)count * 8)) return 0;
    for (unsigned i = 0; i < count; i++) {
        const Byte *t = types.p + 2 + i * 8;
        if (memcmp(t, kind, 4)) continue;
        unsigned refs = be16(t + 6), n = be16(t + 4) + 1;
        if (n > 65535 || !has(types, refs, (uint64_t)n * 12)) return 0;
        for (unsigned j = 0; j < n; j++) {
            const Byte *r = types.p + refs + j * 12;
            if ((int16_t)be16(r) != id) continue;
            uint32_t at = ((uint32_t)r[5] << 16) | ((uint32_t)r[6] << 8) | r[7];
            if (at > data_size || data_size - at < 4) return 0;
            uint32_t length = be32(fork.p + data + at);
            if (length > data_size - at - 4) return 0;
            *out = part(fork, (size_t)data + at + 4, length); return 1;
        }
    }
    return 0;
}

static Byte *lzss(Span compressed, size_t size, Error *e) {
    Byte *out = malloc(size ? size : 1); size_t at = 0, written = 0;
    if (!out) { fail(e, "Not enough memory for graphics."); return NULL; }
    while (at < compressed.n && written < size) {
        unsigned flags = compressed.p[at++];
        for (unsigned bit = 0; bit < 8 && written < size; bit++) {
            if (at >= compressed.n) goto bad;
            if (flags & (1u << bit)) {
                if (!has(compressed, at, 2)) goto bad;
                unsigned a = compressed.p[at++], b = compressed.p[at++], n = (a >> 4) + 3, distance = ((a & 15) << 8) + b + 1;
                if (distance > written) goto bad;
                while (n-- && written < size) { out[written] = out[written - distance]; written++; }
            } else out[written++] = compressed.p[at++];
        }
    }
    if (written == size) return out;
bad:
    free(out); fail(e, "The compressed Macintosh graphics are incomplete or damaged."); return NULL;
}

static int pixels(Span commands, unsigned w, unsigned h, Byte *pix, Byte *alpha, Error *e) {
    struct { unsigned repeats; size_t at; } loops[128];
    size_t at = 0; unsigned x = 0, y = 0, depth = 0;
    for (unsigned guard = 0; guard < 10000000; guard++) {
        if (!has(commands, at, 1)) break;
        unsigned cmd = commands.p[at++], count = cmd & 31, op = cmd & 96;
        if (cmd & 128) { y++; x = 0; }
        if (count == 31) { if (!has(commands, at, 1)) break; count = commands.p[at++] + 32; }
        else count++;
        if (op == 0 && count == 1) return 1;
        if (op == 64) { x += count; continue; }
        if (op == 96) {
            count--;
            if (count) { if (depth == 128) break; loops[depth].repeats = count; loops[depth++].at = at; }
            else if (depth) {
                if (loops[depth - 1].repeats == 0) depth--;
                else { loops[depth - 1].repeats--; at = loops[depth - 1].at; }
            }
            continue;
        }
        unsigned bytes = op == 0 ? 1 : count;
        if (!has(commands, at, bytes)) break;
        for (unsigned i = 0; i < count; i++, x++) {
            if (y < h) {
                if (x >= w) return fail(e, "A Macintosh sprite writes outside its bounds.");
                pix[(size_t)y * w + x] = commands.p[at + (op ? i : 0)]; alpha[(size_t)y * w + x] = 1;
            }
        }
        at += bytes;
    }
    return fail(e, "Invalid Macintosh sprite command stream.");
}

static int make_image(Span raw, size_t at, const Byte palette[256][3], const Byte valid[256], Entry *entry, Error *e) {
    if (!has(raw, at, 8)) return fail(e, "Invalid Macintosh image offset.");
    const Byte *r = raw.p + at; unsigned w = be16(r), h = be16(r + 2);
    if (!w || !h || w > 4096 || h > 4096) return fail(e, "Unsupported Macintosh image dimensions.");
    size_t n = (size_t)w * h; Byte *pix = calloc(n, 1), *alpha = calloc(n, 1), *payload = malloc(n + 768);
    if (!pix || !alpha || !payload) { free(pix); free(alpha); free(payload); return fail(e, "Not enough memory for sprites."); }
    int ok = pixels(part(raw, at + 8, raw.n - at - 8), w, h, pix, alpha, e);
    Byte colors[256][3] = {{0}}; unsigned count = 1, transparent = 0;
    if (ok) for (size_t i = 0; i < n; i++) {
        if (!alpha[i]) { pix[i] = 0; transparent = 1; continue; }
        unsigned color = pix[i], index = 1;
        if (!valid[color]) { ok = fail(e, "A sprite uses a missing Macintosh palette colour."); break; }
        while (index < count && memcmp(colors[index], palette[color], 3)) index++;
        if (index == count) {
            if (count == 255) { ok = fail(e, "Too many colours in a Macintosh sprite."); break; }
            memcpy(colors[count++], palette[color], 3);
        }
        pix[i] = index;
    }
    if (ok) {
        memcpy(payload, colors, count * 3); memcpy(payload + count * 3, pix, n);
        entry->w = w; entry->h = h; entry->colors = count; entry->flags = 2 | transparent;
        entry->payload = payload; entry->size = count * 3 + n;
        Entry *origin = entry + 1; origin->payload = malloc(4); origin->size = 4;
        if (origin->payload) { put16(origin->payload, be16(r + 4)); put16(origin->payload + 2, be16(r + 6)); }
        else ok = fail(e, "Not enough memory for sprite origins.");
    } else free(payload);
    free(pix); free(alpha); return ok;
}

static const struct { unsigned id; const char *folder; } groups[] = {
    {40,"TITLE"},{50,"TITLE"},{150,"PRINCE"},{400,"KID"},{700,"PRINCE"},
    {800,"PV"},{850,"PV"},{900,"PV"},{950,"PV"},{960,"PV"},
    {1200,"VDUNGEON"},{1360,"VDUNGEON"},{1750,"GUARD"},
    {2200,"VPALACE"},{2360,"VPALACE"},{2750,"SKEL"},
    {3750,"FAT"},{4750,"SHADOW"},{5750,"VIZIER"}
};

static int build_entries(Span color, Span fork, Span clut, Entry *entries, Error *e) {
    Byte palette[256][3] = {{0}}, valid[256] = {0}; size_t count = 0;
    if (!has(clut, 0, 8)) return fail(e, "The Macintosh palette is missing.");
    unsigned n = be16(clut.p + 6) + 1;
    if (n > 256 || !has(clut, 8, (uint64_t)n * 8)) return fail(e, "Unsupported Macintosh palette.");
    for (unsigned i = 0; i < n; i++) {
        const Byte *p = clut.p + 8 + i * 8; unsigned index = be16(p);
        if (index > 255) return fail(e, "Invalid Macintosh palette index.");
        palette[index][0] = p[2]; palette[index][1] = p[4]; palette[index][2] = p[6]; valid[index] = 1;
    }
    for (size_t g = 0; g < sizeof(groups) / sizeof(groups[0]); g++) {
        Span descriptor, table;
        if (!resource(fork, "SHPD", groups[g].id, &descriptor) || descriptor.n != 12 ||
            !resource(fork, "SHPT", groups[g].id, &table) || table.n % 4)
            return fail(e, "This Macintosh colour edition is missing required graphics groups.");
        unsigned offset = be32(descriptor.p), compressed = be32(descriptor.p + 4), size = be32(descriptor.p + 8);
        if (!size || size > 64u * 1024 * 1024 || !has(color, offset, compressed ? compressed : size))
            return fail(e, "A Macintosh graphics group is incomplete.");
        Span raw = part(color, offset, compressed ? compressed : size); Byte *expanded = NULL;
        if (compressed) { expanded = lzss(raw, size, e); if (!expanded) return 0; raw.p = expanded; raw.n = size; }
        int ok = 1;
        for (size_t slot = 0; slot < table.n / 4; slot++) {
            uint32_t at = be32(table.p + slot * 4);
            if (at == UINT32_MAX) continue;
            if (count == 1640 || slot + groups[g].id % 1000 > 65535) { ok = fail(e, "Unsupported Macintosh sprite count."); break; }
            Entry *image = entries + count, *origin = image + 1;
            SDL_strlcpy(image->folder, groups[g].folder, sizeof(image->folder));
            SDL_strlcpy(origin->folder, groups[g].folder, sizeof(origin->folder));
            SDL_strlcpy(image->ext, "png", sizeof(image->ext)); SDL_strlcpy(origin->ext, "org", sizeof(origin->ext));
            image->id = origin->id = groups[g].id % 1000 + slot;
            /* Count both entries before decoding so error cleanup frees them. */
            count += 2;
            if (!make_image(raw, at, palette, valid, image, e)) { ok = 0; break; }
        }
        free(expanded); if (!ok) return 0;
    }
    return count == 1640 ? 1 : fail(e, "This colour edition does not contain the expected 820 sprites.");
}

#ifdef _WIN32
static wchar_t *wide_path(const char *path) {
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, NULL, 0);
    wchar_t *out = n ? malloc((size_t)n * sizeof(wchar_t)) : NULL;
    if (out && !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, out, n)) { free(out); return NULL; }
    return out;
}
#endif

static int entry_order(const void *a, const void *b) {
    const Entry *x = a, *y = b; int n = strcmp(x->folder, y->folder);
    if (n) return n;
    if (x->id != y->id) return x->id < y->id ? -1 : 1;
    return strcmp(x->ext, y->ext);
}

typedef struct {
#ifdef _WIN32
    HANDLE handle;
#else
    int handle;
#endif
} Writer;
static int write_bytes(Writer *w, const void *data, size_t n) {
    const Byte *p = data;
    while (n) {
#ifdef _WIN32
        DWORD wrote = 0, take = n > 1048576 ? 1048576 : (DWORD)n;
        if (!WriteFile(w->handle, p, take, &wrote, NULL) || !wrote) return 0;
#else
        ssize_t wrote = write(w->handle, p, n);
        if (wrote < 0 && errno == EINTR) continue;
        if (wrote <= 0) return 0;
#endif
        p += wrote; n -= wrote;
    }
    return 1;
}

static int install_entries(Entry *entries, const char *pack, Error *e) {
    size_t length = strlen(pack) + 80; char *temporary = malloc(length);
    if (!temporary) return fail(e, "Not enough memory for the output path.");
    qsort(entries, 1640, sizeof(*entries), entry_order);
#ifdef _WIN32
    SDL_snprintf(temporary, length, "%s.import-%lu-%llu.tmp", pack, (unsigned long)GetCurrentProcessId(), (unsigned long long)SDL_GetPerformanceCounter());
    wchar_t *tw = wide_path(temporary), *pw = wide_path(pack);
    Writer w = {tw ? CreateFileW(tw, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL) : INVALID_HANDLE_VALUE};
    if (w.handle == INVALID_HANDLE_VALUE || !pw) {
        if (w.handle != INVALID_HANDLE_VALUE) { CloseHandle(w.handle); DeleteFileW(tw); }
        free(tw); free(pw); free(temporary);
        return fail(e, "Cannot save graphics here. Move the whole game folder to a writable location and try again.");
    }
#else
    SDL_snprintf(temporary, length, "%s.import-%ld-%llu.tmp", pack, (long)getpid(), (unsigned long long)SDL_GetPerformanceCounter());
    Writer w = {open(temporary, O_WRONLY | O_CREAT | O_EXCL, 0600)};
    if (w.handle < 0) { free(temporary); return fail(e, "Cannot save graphics here. Move the whole game folder to a writable location and try again."); }
#endif
    Byte header[16] = {0}; memcpy(header, "POPSPAK1", 8); put32(header + 8, 1); put32(header + 12, 1640);
    int ok = write_bytes(&w, header, sizeof(header)); uint32_t offset = 16 + 1640 * 36;
    for (unsigned i = 0; i < 1640 && ok; i++) {
        Entry *x = entries + i; Byte row[36] = {0};
        memcpy(row, x->folder, 16); memcpy(row + 16, x->ext, 4);
        put16(row + 20, x->id); put16(row + 22, x->w); put16(row + 24, x->h);
        row[26] = x->colors; row[27] = x->flags; put32(row + 28, offset); put32(row + 32, x->size);
        if (x->size > UINT32_MAX - offset) { ok = 0; break; }
        offset += x->size; ok = write_bytes(&w, row, sizeof(row));
    }
    for (unsigned i = 0; i < 1640 && ok; i++) ok = write_bytes(&w, entries[i].payload, entries[i].size);
#ifdef _WIN32
    if (!FlushFileBuffers(w.handle)) ok = 0;
    if (!CloseHandle(w.handle)) ok = 0;
    if (ok) ok = MoveFileExW(tw, pw, MOVEFILE_WRITE_THROUGH) != 0; /* No replacement. */
    DeleteFileW(tw); free(tw); free(pw);
#else
    if (fsync(w.handle)) ok = 0;
    if (close(w.handle)) ok = 0;
    if (ok) {
        /* renameat2 also supports writable filesystems without hard links. */
#if defined(__linux__) && defined(RENAME_NOREPLACE)
        ok = renameat2(AT_FDCWD, temporary, AT_FDCWD, pack, RENAME_NOREPLACE) == 0;
        if (!ok && (errno == ENOSYS || errno == EINVAL)) ok = link(temporary, pack) == 0;
#else
        ok = link(temporary, pack) == 0;
#endif
    }
    unlink(temporary);
#endif
    free(temporary);
    return ok ? 1 : fail(e, "Could not finish saving the graphics. Check free space and permissions; existing files were preserved.");
}

typedef struct {
    Byte *color, *color_fork, *application;
    size_t color_size, color_fork_size, application_size;
    unsigned depth; Error *error;
} Sources;
static int collect_container(Sources *sources, Span data, const char *name);
static int collect_file(const char *name, const char type[4], const Byte *data, size_t data_size,
                        const Byte *fork, size_t fork_size, void *context) {
    Sources *s=context;Span r={fork,fork_size},palette;
    const char *base=strrchr(name,'/');base=base?base+1:name;
    Byte **destination=NULL;size_t *length=NULL;
    if(!SDL_strcasecmp(base,"Persia(COLOR)")) {
        if(s->color)return fail(s->error,"Multiple Persia(COLOR) graphics files found. Select one game copy.");
        if(data_size>64u*1024*1024||fork_size>64u*1024*1024)return fail(s->error,"Game resource file is too large.");
        s->color=malloc(data_size?data_size:1);s->color_fork=malloc(fork_size?fork_size:1);
        if(!s->color||!s->color_fork)return fail(s->error,"Not enough memory for game resources.");
        memcpy(s->color,data,data_size);memcpy(s->color_fork,fork,fork_size);
        s->color_size=data_size;s->color_fork_size=fork_size;return 1;
    }
    if(fork&&resource(r,"clut",2000,&palette)) {
        if(s->application)return fail(s->error,"Multiple colour game applications found. Select one game copy.");
        destination=&s->application;length=&s->application_size;
    }
    if(destination){*destination=malloc(fork_size?fork_size:1);if(!*destination)return fail(s->error,"Not enough memory for game resources.");memcpy(*destination,fork,fork_size);*length=fork_size;return 1;}
    (void)type;
    Span bytes={data,data_size};
    if(mac_archive_kind(data,data_size,name)||
       (has(bytes,1024,2)&&!memcmp(data+1024,"BD",2))||
       (has(bytes,0,2)&&!memcmp(data,"ER",2))||
       (has(bytes,512,2)&&!memcmp(data+512,"PM",2))||
       (has(bytes,0,84)&&be16(data+82)==256))return collect_container(s,bytes,name);
    return 1;
}
static int collect_hfs(Sources *sources, Span image) {
    Error *e=sources->error;int ok=0;Byte *catalog=NULL,*ext=NULL;Hfs *h=calloc(1,sizeof(*h));
    if(!h)return fail(e,"Not enough memory for the HFS catalog.");
    h->error=e;
    if(!volume_of(image,&h->volume,e)||!has(h->volume,1024,162))goto done;
    const Byte *mdb=h->volume.p+1024;
    h->block_size=be32(mdb+20);h->allocation_start=be16(mdb+28)*512;
    if(h->block_size<512||h->block_size%512){fail(e,"Invalid HFS allocation block size.");goto done;}
    unsigned ext_size=be32(mdb+130),cat_size=be32(mdb+146);
    if(ext_size){ext=fork_of(h,ext_size,mdb+134,3,0);Span tree={ext,ext_size};if(!ext||!leaves(tree,overflow_record,h,e))goto done;}
    catalog=fork_of(h,cat_size,mdb+150,4,0);Span tree={catalog,cat_size};
    if(!catalog||!leaves(tree,catalog_record,h,e))goto done;
    for(size_t i=0;i<h->file_count;i++){
        GameFile *f=&h->files[i];Byte *fork=fork_of(h,f->rsrc_size,f->rsrc_ext,f->id,255),*data=NULL;
        if(!fork)goto done;
        if(f->color){data=fork_of(h,f->data_size,f->data_ext,f->id,0);if(!data){free(fork);goto done;}}
        int accepted=collect_file(f->color?"Persia(COLOR)":"Application","APPL",data,f->color?f->data_size:0,fork,f->rsrc_size,sources);
        free(data);free(fork);if(!accepted)goto done;
    }
    ok=1;
 done:free(h);free(catalog);free(ext);return ok;
}
static int collect_container(Sources *s, Span data, const char *name) {
    if(s->depth>=5)return fail(s->error,"Too many nested Macintosh containers.");
    s->depth++;
    /* An HFS/ISO bridge uses its Macintosh filesystem, preserving both forks.
     * Plain ISO9660 is traversed in memory for disks or fork-preserving files. */
    int is_hfs=(has(data,1024,2)&&!memcmp(data.p+1024,"BD",2))||
               (has(data,0,2)&&!memcmp(data.p,"ER",2))||
               (has(data,512,2)&&!memcmp(data.p+512,"PM",2))||
               (has(data,0,84)&&data.p[0]<=63&&be16(data.p+82)==256);
    int ok=is_hfs?collect_hfs(s,data):mac_archive_read(data.p,data.n,name,collect_file,s,s->error->text,s->error->size);
    s->depth--;return ok;
}
static void free_sources(Sources *s){free(s->color);free(s->color_fork);free(s->application);}
static int read_sources(const char *path, Sources *s) {
    size_t size=0;Byte *image=read_file(path,(size_t)2147483648ULL,&size,s->error);
    if(!image)return 0;
    Span bytes={image,size};int ok=collect_container(s,bytes,path);free(image);
    if(ok&&(!s->color||!s->application))return fail(s->error,"This file needs the Macintosh colour Prince of Persia application and Persia(COLOR) graphics, with resource forks preserved.");
    return ok;
}
int mac_import_source_bundle(const char *path, unsigned char **bundle, size_t *size, char *error, size_t error_size) {
    Error e={error,error_size};Sources s={0};s.error=&e;*bundle=NULL;*size=0;if(error_size)error[0]=0;
    if(!read_sources(path,&s)){free_sources(&s);return 0;}
    size_t length=20+s.color_size+s.color_fork_size+s.application_size;Byte *out=malloc(length);
    if(!out){free_sources(&s);return fail(&e,"Not enough memory for source forks.");}
    memcpy(out,"POPMACS1",8);put32(out+8,s.color_size);put32(out+12,s.color_fork_size);put32(out+16,s.application_size);
    memcpy(out+20,s.color,s.color_size);memcpy(out+20+s.color_size,s.color_fork,s.color_fork_size);
    memcpy(out+20+s.color_size+s.color_fork_size,s.application,s.application_size);
    free_sources(&s);*bundle=out;*size=length;return 1;
}
int mac_import_disk(const char *disk, const char *pack, char *error, size_t error_size) {
    Error e={error,error_size};int ok=0;
    if(error_size)error[0]=0;
    Entry *entries=calloc(1640,sizeof(*entries));Sources sources={0};sources.error=&e;
    if(!entries){fail(&e,"Not enough memory to import graphics.");goto done;}
    if(!read_sources(disk,&sources))goto done;
    Span color={sources.color,sources.color_size},cf={sources.color_fork,sources.color_fork_size},app={sources.application,sources.application_size},clut;
    if(!resource(app,"clut",2000,&clut)||!build_entries(color,cf,clut,entries,&e))goto done;
    ok=install_entries(entries,pack,&e);
 done:
    if(entries)for(unsigned i=0;i<1640;i++)free(entries[i].payload);
    free(entries);free_sources(&sources);return ok;
}

int mac_import_pack_status(const char *pack) {
    SDL_RWops *check = SDL_RWFromFile(pack, "rb");
    if (!check) return 0;
    SDL_RWclose(check);
    size_t size = 0; Byte *data = read_file(pack, 64u * 1024 * 1024, &size, NULL);
    if (!data) return -1;
    Span all = {data, size}; int ok = has(all, 0, 16) && !memcmp(data, "POPSPAK1", 8) && le32(data + 8) == 1 && le32(data + 12) == 1640;
    unsigned images = 0, origins = 0;
    if (ok) ok = has(all, 16, 1640 * 36);
    for (unsigned i = 0; i < 1640 && ok; i++) {
        const Byte *p = data + 16 + i * 36;
        unsigned offset = le32(p + 28), n = le32(p + 32);
        ok = offset >= 16 + 1640 * 36 && has(all, offset, n) && memchr(p, 0, 16) != NULL;
        if (!memcmp(p + 16, "png\0", 4)) {
            unsigned w = le16(p + 22), h = le16(p + 24);
            ok = ok && w && h && p[26] && (p[27] & 2) && (uint64_t)w * h + p[26] * 3 == n; images++;
        } else if (!memcmp(p + 16, "org\0", 4)) { ok = ok && n == 4; origins++; }
        else ok = 0;
    }
    free(data); return ok && images == 820 && origins == 820 ? 1 : -1;
}

static char *joined(const char *base, const char *name) {
    size_t n = strlen(base) + strlen(name) + 2; char *p = malloc(n);
    if (p) SDL_snprintf(p, n, "%s%s", base, name);
    return p;
}
static int disk_name(const char *name) {
    const char *ext = strrchr(name, '.');
    return ext && (!SDL_strcasecmp(ext, ".hfs") || !SDL_strcasecmp(ext, ".dsk") || !SDL_strcasecmp(ext, ".dc42") || !SDL_strcasecmp(ext, ".iso") || !SDL_strcasecmp(ext, ".hqx") || !SDL_strcasecmp(ext, ".sit"));
}
int mac_import_find_disk(const char *base, char **result) {
    int count = 0;
#ifdef _WIN32
    char *pattern = joined(base, "*"); wchar_t *wide = pattern ? wide_path(pattern) : NULL;
    WIN32_FIND_DATAW item; HANDLE h = wide ? FindFirstFileW(wide, &item) : INVALID_HANDLE_VALUE;
    free(pattern); free(wide);
    if (h == INVALID_HANDLE_VALUE) return 0;
    do {
        if (item.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        int n = WideCharToMultiByte(CP_UTF8, 0, item.cFileName, -1, NULL, 0, NULL, NULL);
        char *name = n ? malloc(n) : NULL;
        if (!name) continue;
        WideCharToMultiByte(CP_UTF8, 0, item.cFileName, -1, name, n, NULL, NULL);
        if (disk_name(name)) { count++; if (count == 1) *result = joined(base, name); }
        free(name);
    } while (FindNextFileW(h, &item));
    FindClose(h);
#else
    DIR *dir = opendir(base); struct dirent *item;
    if (!dir) return 0;
    while ((item = readdir(dir))) {
        if (!disk_name(item->d_name)) continue;
        char *path = joined(base, item->d_name); struct stat st;
        if (path && !stat(path, &st) && S_ISREG(st.st_mode)) { count++; if (count == 1) *result = path; else free(path); }
        else free(path);
    }
    closedir(dir);
#endif
    return count;
}
