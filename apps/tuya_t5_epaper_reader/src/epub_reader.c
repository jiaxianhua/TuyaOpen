#include "epub_reader.h"

#include "inflate_stream.h"
#include "tal_api.h"
#include "tkl_fs.h"
#include "tuya_error_code.h"

#include <stdio.h>
#include <string.h>

#define EPUB_TAIL_SCAN_MAX (65536u + 64u)
#define EPUB_META_MAX      (256u * 1024u)
#define EPUB_NAME_MAX      384u

typedef struct {
    uint32_t comp_size;
    uint32_t uncomp_size;
    uint32_t local_off;
    uint16_t method;
    uint16_t flags;
} zip_entry_meta_t;

typedef struct {
    uint8_t *buf;
    size_t len;
    size_t cap;
    size_t max;
} mem_out_t;

typedef struct {
    TUYA_FILE out;
    int in_tag;
    int in_entity;
    int skip_text;
    int tag_is_close;
    char tag[16];
    int tag_len;
    char ent[10];
    int ent_len;
    int last_out;
} html_filter_t;

static uint16_t le16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint32_t fnv1a32(const char *s)
{
    uint32_t h = 2166136261u;
    while (s && *s) {
        h ^= (uint8_t)*s++;
        h *= 16777619u;
    }
    return h;
}

static int mkdir_p(const char *path)
{
    if (!path || !path[0]) return -1;
    char tmp[256];
    snprintf(tmp, sizeof(tmp), "%s", path);
    size_t n = strlen(tmp);
    if (n == 0) return -1;
    if (tmp[n - 1] == '/') tmp[n - 1] = 0;

    for (char *p = tmp + 1; *p; p++) {
        if (*p != '/') continue;
        *p = 0;
        if (tmp[0]) {
            if (tkl_fs_mkdir(tmp) != 0) {
                TUYA_DIR d = NULL;
                if (tkl_dir_open(tmp, &d) != OPRT_OK) {
                    *p = '/';
                    return -1;
                }
                tkl_dir_close(d);
            }
        }
        *p = '/';
    }

    if (tkl_fs_mkdir(tmp) != 0) {
        TUYA_DIR d = NULL;
        if (tkl_dir_open(tmp, &d) != OPRT_OK) return -1;
        tkl_dir_close(d);
    }
    return 0;
}

static int is_ext(const char *name, const char *ext)
{
    const char *dot = strrchr(name, '.');
    if (!dot || dot == name) return 0;
    dot++;
    while (*dot && *ext) {
        char a = *dot++;
        char b = *ext++;
        if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
        if (a != b) return 0;
    }
    return (*dot == 0 && *ext == 0);
}

static int is_html_name(const char *name)
{
    return is_ext(name, "xhtml") || is_ext(name, "html") || is_ext(name, "htm");
}

static int is_text_name(const char *name)
{
    return is_html_name(name) || is_ext(name, "txt");
}

static int html_emit(html_filter_t *f, char c)
{
    if (!f || !f->out) return -1;
    if (tkl_fwrite(&c, 1, f->out) != 1) return -1;
    f->last_out = (int)c;
    return 0;
}

static int html_emit_space(html_filter_t *f)
{
    if (!f) return -1;
    if (f->last_out == ' ' || f->last_out == '\n' || f->last_out == '\r') return 0;
    return html_emit(f, ' ');
}

static int html_emit_newline(html_filter_t *f)
{
    if (!f) return -1;
    if (f->last_out == '\n') return 0;
    return html_emit(f, '\n');
}

static int html_emit_str(html_filter_t *f, const char *s)
{
    if (!f || !s) return -1;
    for (; *s; s++) {
        if (html_emit(f, *s) != 0) return -1;
    }
    return 0;
}

static int html_decode_entity(html_filter_t *f)
{
    if (!f) return -1;
    f->ent[f->ent_len] = 0;
    if (strcmp(f->ent, "amp") == 0) return html_emit(f, '&');
    if (strcmp(f->ent, "lt") == 0) return html_emit(f, '<');
    if (strcmp(f->ent, "gt") == 0) return html_emit(f, '>');
    if (strcmp(f->ent, "quot") == 0) return html_emit(f, '"');
    if (strcmp(f->ent, "apos") == 0) return html_emit(f, '\'');
    if (strcmp(f->ent, "nbsp") == 0) return html_emit(f, ' ');
    if (f->ent[0] == '#') {
        unsigned long v = 0;
        if (f->ent[1] == 'x' || f->ent[1] == 'X') {
            v = strtoul(f->ent + 2, NULL, 16);
        } else {
            v = strtoul(f->ent + 1, NULL, 10);
        }
        if (v > 0 && v < 0x80) return html_emit(f, (char)v);
        return html_emit(f, '?');
    }
    if (html_emit(f, '&') != 0) return -1;
    return html_emit_str(f, f->ent);
}

static int html_filter_push(html_filter_t *f, const uint8_t *data, size_t len)
{
    if (!f || !data) return -1;
    for (size_t i = 0; i < len; i++) {
        char c = (char)data[i];
        if (f->in_entity) {
            if (c == ';') {
                if (html_decode_entity(f) != 0) return -1;
                f->in_entity = 0;
                f->ent_len = 0;
            } else if (f->ent_len + 1 < (int)sizeof(f->ent) && c >= 0x20 && c != '<') {
                f->ent[f->ent_len++] = c;
            } else {
                if (html_emit(f, '&') != 0) return -1;
                if (f->ent_len) {
                    f->ent[f->ent_len] = 0;
                    if (html_emit_str(f, f->ent) != 0) return -1;
                }
                f->in_entity = 0;
                f->ent_len = 0;
                if (c == '&') {
                    f->in_entity = 1;
                } else {
                    i--;
                }
            }
            continue;
        }

        if (f->in_tag) {
            if (c == '>') {
                f->tag[f->tag_len] = 0;
                if (!f->tag_is_close && (strcmp(f->tag, "script") == 0 || strcmp(f->tag, "style") == 0)) {
                    f->skip_text = 1;
                } else if (f->tag_is_close && f->skip_text &&
                           (strcmp(f->tag, "script") == 0 || strcmp(f->tag, "style") == 0)) {
                    f->skip_text = 0;
                }

                if (!f->skip_text) {
                    if (strcmp(f->tag, "br") == 0 || strcmp(f->tag, "p") == 0 || strcmp(f->tag, "div") == 0 ||
                        strcmp(f->tag, "li") == 0 || strcmp(f->tag, "tr") == 0 ||
                        strcmp(f->tag, "h1") == 0 || strcmp(f->tag, "h2") == 0 || strcmp(f->tag, "h3") == 0 ||
                        strcmp(f->tag, "h4") == 0 || strcmp(f->tag, "h5") == 0 || strcmp(f->tag, "h6") == 0) {
                        if (html_emit_newline(f) != 0) return -1;
                    }
                }
                f->in_tag = 0;
                f->tag_len = 0;
                f->tag_is_close = 0;
            } else if (f->tag_len == 0 && c == '/') {
                f->tag_is_close = 1;
            } else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '/') {
                if (f->tag_len + 1 < (int)sizeof(f->tag)) {
                    if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
                    f->tag[f->tag_len++] = c;
                }
            }
            continue;
        }

        if (f->skip_text) {
            if (c == '<') {
                f->in_tag = 1;
                f->tag_len = 0;
                f->tag_is_close = 0;
            }
            continue;
        }

        if (c == '<') {
            f->in_tag = 1;
            f->tag_len = 0;
            f->tag_is_close = 0;
            continue;
        }
        if (c == '&') {
            f->in_entity = 1;
            f->ent_len = 0;
            continue;
        }
        if (c == '\r' || c == '\n' || c == '\t' || c == ' ') {
            if (html_emit_space(f) != 0) return -1;
            continue;
        }
        if (html_emit(f, c) != 0) return -1;
    }
    return 0;
}

static int mem_out_cb(void *user, const uint8_t *data, size_t len)
{
    mem_out_t *out = (mem_out_t *)user;
    if (!out || !data || len == 0) return 0;
    if (out->len + len > out->max) return -1;
    if (out->len + len > out->cap) {
        size_t new_cap = out->cap ? out->cap * 2 : 4096u;
        while (new_cap < out->len + len) new_cap *= 2;
        uint8_t *nb = (uint8_t *)tal_malloc(new_cap);
        if (!nb) return -1;
        if (out->buf && out->len) memcpy(nb, out->buf, out->len);
        if (out->buf) tal_free(out->buf);
        out->buf = nb;
        out->cap = new_cap;
    }
    memcpy(out->buf + out->len, data, len);
    out->len += len;
    return 0;
}

static int inflate_raw_to_cb(TUYA_FILE f, uint32_t comp_size, int (*cb)(void *, const uint8_t *, size_t), void *user)
{
    if (!f || !cb) return -1;
    inflate_stream_t infl;
    inflate_stream_init(&infl, cb, user);
    uint8_t zhdr[2] = {0x78, 0x9C};
    size_t consumed = 0;
    if (inflate_stream_push(&infl, zhdr, sizeof(zhdr), &consumed) < 0) {
        inflate_stream_deinit(&infl);
        return -1;
    }

    uint8_t buf[2048];
    uint32_t left = comp_size;
    while (left > 0) {
        uint32_t chunk = (left > sizeof(buf)) ? (uint32_t)sizeof(buf) : left;
        int rd = tkl_fread(buf, (int)chunk, f);
        if (rd <= 0) {
            inflate_stream_deinit(&infl);
            return -1;
        }
        left -= (uint32_t)rd;
        size_t use = 0;
        if (inflate_stream_push(&infl, buf, (size_t)rd, &use) < 0) {
            inflate_stream_deinit(&infl);
            return -1;
        }
    }

    uint8_t ztrail[4] = {0, 0, 0, 0};
    if (inflate_stream_push(&infl, ztrail, sizeof(ztrail), &consumed) < 0) {
        inflate_stream_deinit(&infl);
        return -1;
    }
    if (inflate_stream_finish(&infl) != 0) {
        inflate_stream_deinit(&infl);
        return -1;
    }
    inflate_stream_deinit(&infl);
    return 0;
}

static int inflate_raw_to_mem(TUYA_FILE f, uint32_t comp_size, mem_out_t *out)
{
    return inflate_raw_to_cb(f, comp_size, mem_out_cb, out);
}

static int plain_copy_to_mem(TUYA_FILE f, uint32_t comp_size, mem_out_t *out)
{
    if (!out) return -1;
    uint8_t buf[2048];
    uint32_t left = comp_size;
    while (left > 0) {
        uint32_t chunk = (left > sizeof(buf)) ? (uint32_t)sizeof(buf) : left;
        int rd = tkl_fread(buf, (int)chunk, f);
        if (rd <= 0) return -1;
        left -= (uint32_t)rd;
        if (mem_out_cb(out, buf, (size_t)rd) != 0) return -1;
    }
    return 0;
}

static int find_eocd(TUYA_FILE f, uint32_t fsize, uint32_t *cd_off, uint16_t *total_entries)
{
    if (!f || !cd_off || !total_entries) return -1;
    uint32_t scan = fsize < EPUB_TAIL_SCAN_MAX ? fsize : EPUB_TAIL_SCAN_MAX;
    uint32_t start = fsize - scan;
    if (tkl_fseek(f, (int64_t)start, SEEK_SET) != 0) return -1;
    uint8_t *buf = (uint8_t *)tal_malloc(scan);
    if (!buf) return -1;
    int rd = tkl_fread(buf, (int)scan, f);
    if (rd <= 0) {
        tal_free(buf);
        return -1;
    }
    int found = -1;
    for (int i = rd - 4; i >= 0; i--) {
        if (buf[i] == 0x50 && buf[i + 1] == 0x4B && buf[i + 2] == 0x05 && buf[i + 3] == 0x06) {
            found = i;
            break;
        }
    }
    if (found < 0 || found + 22 > rd) {
        tal_free(buf);
        return -1;
    }
    *total_entries = le16(buf + found + 10);
    *cd_off = le32(buf + found + 16);
    tal_free(buf);
    return 0;
}

static int zip_read_local_data_off(TUYA_FILE f, uint32_t local_off, uint32_t *data_off)
{
    uint8_t hdr[30];
    if (tkl_fseek(f, (int64_t)local_off, SEEK_SET) != 0) return -1;
    if (tkl_fread(hdr, (int)sizeof(hdr), f) != (int)sizeof(hdr)) return -1;
    if (le32(hdr) != 0x04034B50u) return -1;
    uint16_t name_len = le16(hdr + 26);
    uint16_t extra_len = le16(hdr + 28);
    *data_off = local_off + 30u + (uint32_t)name_len + (uint32_t)extra_len;
    return 0;
}

static int zip_find_entry_by_name(TUYA_FILE f, uint32_t cd_off, uint16_t total, const char *name, zip_entry_meta_t *out_meta)
{
    if (!f || !name || !out_meta) return -1;
    if (tkl_fseek(f, (int64_t)cd_off, SEEK_SET) != 0) return -1;
    for (uint16_t i = 0; i < total; i++) {
        uint8_t hdr[46];
        if (tkl_fread(hdr, (int)sizeof(hdr), f) != (int)sizeof(hdr)) return -1;
        if (le32(hdr) != 0x02014B50u) return -1;
        uint16_t name_len = le16(hdr + 28);
        uint16_t extra_len = le16(hdr + 30);
        uint16_t comment_len = le16(hdr + 32);
        uint32_t comp_size = le32(hdr + 20);
        uint32_t uncomp_size = le32(hdr + 24);
        uint16_t method = le16(hdr + 10);
        uint16_t flags = le16(hdr + 8);
        uint32_t local_off = le32(hdr + 42);

        char entry_name[EPUB_NAME_MAX];
        if (name_len >= sizeof(entry_name)) {
            if (tkl_fseek(f, (int64_t)name_len + extra_len + comment_len, SEEK_CUR) != 0) return -1;
            continue;
        }
        if (tkl_fread(entry_name, name_len, f) != (int)name_len) return -1;
        entry_name[name_len] = 0;
        if (tkl_fseek(f, (int64_t)extra_len + comment_len, SEEK_CUR) != 0) return -1;

        if (strcmp(entry_name, name) == 0) {
            out_meta->comp_size = comp_size;
            out_meta->uncomp_size = uncomp_size;
            out_meta->local_off = local_off;
            out_meta->method = method;
            out_meta->flags = flags;
            return 0;
        }
    }
    return -1;
}

static int zip_find_first_text_entry(TUYA_FILE f, uint32_t cd_off, uint16_t total, char *out_name, size_t out_len)
{
    if (!f || !out_name || out_len == 0) return -1;
    if (tkl_fseek(f, (int64_t)cd_off, SEEK_SET) != 0) return -1;
    for (uint16_t i = 0; i < total; i++) {
        uint8_t hdr[46];
        if (tkl_fread(hdr, (int)sizeof(hdr), f) != (int)sizeof(hdr)) return -1;
        if (le32(hdr) != 0x02014B50u) return -1;
        uint16_t name_len = le16(hdr + 28);
        uint16_t extra_len = le16(hdr + 30);
        uint16_t comment_len = le16(hdr + 32);
        if (name_len == 0 || name_len >= out_len || name_len >= EPUB_NAME_MAX) {
            if (tkl_fseek(f, (int64_t)name_len + extra_len + comment_len, SEEK_CUR) != 0) return -1;
            continue;
        }
        if (tkl_fread(out_name, (int)name_len, f) != (int)name_len) return -1;
        out_name[name_len] = 0;
        if (tkl_fseek(f, (int64_t)extra_len + comment_len, SEEK_CUR) != 0) return -1;

        size_t nlen = strlen(out_name);
        if (nlen > 0 && out_name[nlen - 1] == '/') continue;
        if (!is_text_name(out_name)) continue;
        if (strncmp(out_name, "META-INF/", 9) == 0) continue;
        if (strcmp(out_name, "mimetype") == 0) continue;
        return 0;
    }
    return -1;
}

static char *zip_read_entry_to_mem(TUYA_FILE f, const zip_entry_meta_t *meta, size_t max_out, size_t *out_len)
{
    if (!f || !meta || !out_len) return NULL;
    uint32_t data_off = 0;
    if (zip_read_local_data_off(f, meta->local_off, &data_off) != 0) return NULL;
    if (tkl_fseek(f, (int64_t)data_off, SEEK_SET) != 0) return NULL;

    mem_out_t out = {0};
    out.max = max_out;
    int r = -1;
    if (meta->method == 0) {
        r = plain_copy_to_mem(f, meta->comp_size, &out);
    } else if (meta->method == 8) {
        r = inflate_raw_to_mem(f, meta->comp_size, &out);
    }
    if (r != 0) {
        if (out.buf) tal_free(out.buf);
        return NULL;
    }
    if (out.len + 1 > out.cap) {
        uint8_t *nb = (uint8_t *)tal_malloc(out.len + 1);
        if (!nb) {
            if (out.buf) tal_free(out.buf);
            return NULL;
        }
        if (out.buf && out.len) memcpy(nb, out.buf, out.len);
        if (out.buf) tal_free(out.buf);
        out.buf = nb;
        out.cap = out.len + 1;
    }
    out.buf[out.len] = 0;
    *out_len = out.len;
    return (char *)out.buf;
}

static int extract_attr_value(const char *tag, const char *attr, char *out, size_t out_len)
{
    if (!tag || !attr || !out || out_len == 0) return -1;
    const size_t attr_len = strlen(attr);
    const char *p = tag;
    while ((p = strstr(p, attr)) != NULL) {
        char prev = (p == tag) ? ' ' : *(p - 1);
        char next = *(p + attr_len);
        if ((prev == ' ' || prev == '\t' || prev == '\r' || prev == '\n' || prev == '<' || prev == '/') &&
            (next == '=' || next == ' ' || next == '\t' || next == '\r' || next == '\n')) {
            break;
        }
        p += attr_len;
    }
    if (!p) return -1;
    p = strchr(p, '=');
    if (!p) return -1;
    p++;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    char quote = 0;
    if (*p == '"' || *p == '\'') {
        quote = *p;
        p++;
    }
    size_t n = 0;
    while (*p && n + 1 < out_len) {
        if (quote) {
            if (*p == quote) break;
        } else if (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' || *p == '>') {
            break;
        }
        out[n++] = *p++;
    }
    out[n] = 0;
    return (n > 0) ? 0 : -1;
}

static int parse_container_for_opf(const char *xml, char *out_path, size_t out_len)
{
    if (!xml || !out_path) return -1;
    const char *p = strstr(xml, "rootfile");
    if (!p) p = xml;
    return extract_attr_value(p, "full-path", out_path, out_len);
}

static int parse_opf_for_first_href(const char *opf, char *out_href, size_t out_len)
{
    if (!opf || !out_href) return -1;
    const char *spine = strstr(opf, "<spine");
    if (!spine) spine = opf;
    const char *itemref = strstr(spine, "itemref");
    if (!itemref) return -1;
    char idref[96] = {0};
    if (extract_attr_value(itemref, "idref", idref, sizeof(idref)) != 0) return -1;

    const char *p = opf;
    while ((p = strstr(p, "<item")) != NULL) {
        char id[96] = {0};
        char href[EPUB_NAME_MAX] = {0};
        if (extract_attr_value(p, "id", id, sizeof(id)) == 0 && strcmp(id, idref) == 0) {
            if (extract_attr_value(p, "href", href, sizeof(href)) == 0) {
                snprintf(out_href, out_len, "%s", href);
                return 0;
            }
        }
        p += 5;
    }
    return -1;
}

static void join_epub_path(const char *base, const char *rel, char *out, size_t out_len)
{
    if (!out || out_len == 0) return;
    if (!rel || !rel[0]) {
        out[0] = 0;
        return;
    }
    while (rel[0] == '/') rel++;
    if (!base || !base[0]) {
        snprintf(out, out_len, "%s", rel);
        return;
    }
    size_t blen = strlen(base);
    if (base[blen - 1] == '/') {
        snprintf(out, out_len, "%s%s", base, rel);
    } else {
        snprintf(out, out_len, "%s/%s", base, rel);
    }
}

static void dirname_of(const char *path, char *out, size_t out_len)
{
    if (!out || out_len == 0) return;
    if (!path) {
        out[0] = 0;
        return;
    }
    snprintf(out, out_len, "%s", path);
    char *slash = strrchr(out, '/');
    if (slash) *slash = 0;
    else out[0] = 0;
}

static int write_plain_bytes(TUYA_FILE out, const uint8_t *data, size_t len)
{
    if (!out || !data || len == 0) return 0;
    return (tkl_fwrite((void *)data, (int)len, out) == (int)len) ? 0 : -1;
}

static int html_out_cb(void *user, const uint8_t *data, size_t len)
{
    return html_filter_push((html_filter_t *)user, data, len);
}

static int plain_out_cb(void *user, const uint8_t *data, size_t len)
{
    return write_plain_bytes((TUYA_FILE)user, data, len);
}

static int extract_entry_to_text(TUYA_FILE f, const zip_entry_meta_t *meta, const char *entry_name, const char *out_path)
{
    if (!f || !meta || !out_path) return -1;
    uint32_t data_off = 0;
    if (zip_read_local_data_off(f, meta->local_off, &data_off) != 0) return -1;
    if (tkl_fseek(f, (int64_t)data_off, SEEK_SET) != 0) return -1;

    TUYA_FILE out = tkl_fopen(out_path, "w");
    if (!out) return -1;

    int use_html = entry_name ? is_html_name(entry_name) : 1;
    html_filter_t filter;
    memset(&filter, 0, sizeof(filter));
    filter.out = out;

    int rc = 0;
    if (meta->method == 0) {
        uint8_t buf[2048];
        uint32_t left = meta->comp_size;
        while (left > 0) {
            uint32_t chunk = (left > sizeof(buf)) ? (uint32_t)sizeof(buf) : left;
            int rd = tkl_fread(buf, (int)chunk, f);
            if (rd <= 0) {
                rc = -1;
                break;
            }
            left -= (uint32_t)rd;
            if (use_html) {
                if (html_filter_push(&filter, buf, (size_t)rd) != 0) {
                    rc = -1;
                    break;
                }
            } else {
                if (write_plain_bytes(out, buf, (size_t)rd) != 0) {
                    rc = -1;
                    break;
                }
            }
        }
    } else if (meta->method == 8) {
        if (use_html) {
            rc = inflate_raw_to_cb(f, meta->comp_size, html_out_cb, &filter);
        } else {
            rc = inflate_raw_to_cb(f, meta->comp_size, plain_out_cb, out);
        }
    } else {
        rc = -1;
    }

    tkl_fclose(out);
    return rc;
}

int epub_extract_text(const char *epub_path, const char *cache_dir, char *out_text_path, size_t out_text_path_len)
{
    if (!epub_path || !cache_dir || !out_text_path || out_text_path_len == 0) return -1;
    uint32_t h = fnv1a32(epub_path);
    snprintf(out_text_path, out_text_path_len, "%s/epub_%08x.txt", cache_dir, (unsigned int)h);

    if (mkdir_p(cache_dir) != 0) return -1;
    tkl_fs_remove(out_text_path);

    int fsize = tkl_fgetsize(epub_path);
    if (fsize <= 0) return -1;
    TUYA_FILE f = tkl_fopen(epub_path, "rb");
    if (!f) f = tkl_fopen(epub_path, "r");
    if (!f) return -1;

    uint32_t cd_off = 0;
    uint16_t total = 0;
    if (find_eocd(f, (uint32_t)fsize, &cd_off, &total) != 0) {
        tkl_fclose(f);
        return -1;
    }

    char opf_path[EPUB_NAME_MAX] = {0};
    zip_entry_meta_t meta;
    size_t meta_len = 0;
    if (zip_find_entry_by_name(f, cd_off, total, "META-INF/container.xml", &meta) == 0) {
        char *container = zip_read_entry_to_mem(f, &meta, EPUB_META_MAX, &meta_len);
        if (container) {
            parse_container_for_opf(container, opf_path, sizeof(opf_path));
            tal_free(container);
        }
    }

    char target_name[EPUB_NAME_MAX] = {0};
    if (opf_path[0]) {
        if (zip_find_entry_by_name(f, cd_off, total, opf_path, &meta) == 0) {
            char *opf = zip_read_entry_to_mem(f, &meta, EPUB_META_MAX, &meta_len);
            if (opf) {
                char href[EPUB_NAME_MAX] = {0};
                if (parse_opf_for_first_href(opf, href, sizeof(href)) == 0) {
                    char opf_dir[EPUB_NAME_MAX] = {0};
                    dirname_of(opf_path, opf_dir, sizeof(opf_dir));
                    join_epub_path(opf_dir, href, target_name, sizeof(target_name));
                }
                tal_free(opf);
            }
        }
    }

    if (target_name[0] == 0) {
        if (zip_find_first_text_entry(f, cd_off, total, target_name, sizeof(target_name)) != 0) {
            tkl_fclose(f);
            return -1;
        }
    }

    if (zip_find_entry_by_name(f, cd_off, total, target_name, &meta) != 0) {
        tkl_fclose(f);
        return -1;
    }

    int rc = extract_entry_to_text(f, &meta, target_name, out_text_path);
    tkl_fclose(f);
    return rc;
}
