/*
 * LZO 1x decompression
 * Copyright (c) 2006 Reimar Doeffinger
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <stdlib.h>
#include <string.h>
#include "lzo.h"

/*! define if we may write up to 12 bytes beyond the output buffer */
/* #define OUTBUF_PADDED 1 */
/*! define if we may read up to 8 bytes beyond the input buffer */
/* #define INBUF_PADDED 1 */
typedef struct LZOContext
{
  const guint8 *in, *in_end;
  guint8 *out_start, *out, *out_end;
  int error;
} LZOContext;

/*
 * \brief read one byte from input buffer, avoiding overrun
 * \return byte read
 */
static inline int
get_byte (LZOContext * c)
{
  if (c->in < c->in_end)
    return *c->in++;
  c->error |= LZO_INPUT_DEPLETED;
  return 1;
}

#ifdef INBUF_PADDED
#define GETB(c) (*(c).in++)
#else
#define GETB(c) get_byte(&(c))
#endif

/*
 * \brief decode a length value in the coding used by lzo
 * \param x previous byte value
 * \param mask bits used from x
 * \return decoded length value
 */
static inline int
get_len (LZOContext * c, int x, int mask)
{
  int cnt = x & mask;
  if (!cnt) {
    while (!(x = get_byte (c)))
      cnt += 255;
    cnt += mask + x;
  }
  return cnt;
}

/*#define UNALIGNED_LOADSTORE */
#define BUILTIN_MEMCPY
#ifdef UNALIGNED_LOADSTORE
#define COPY2(d, s) *(guint16 *)(d) = *(guint16 *)(s);
#define COPY4(d, s) *(guint32 *)(d) = *(guint32 *)(s);
#elif defined(BUILTIN_MEMCPY)
#define COPY2(d, s) memcpy(d, s, 2);
#define COPY4(d, s) memcpy(d, s, 4);
#else
#define COPY2(d, s) (d)[0] = (s)[0]; (d)[1] = (s)[1];
#define COPY4(d, s) (d)[0] = (s)[0]; (d)[1] = (s)[1]; (d)[2] = (s)[2]; (d)[3] = (s)[3];
#endif

/*
 * \brief copy bytes from input to output buffer with checking
 * \param cnt number of bytes to copy, must be >= 0
 */
static inline void
copy (LZOContext * c, int cnt)
{
  register const guint8 *src = c->in;
  register guint8 *dst = c->out;
  if (cnt > c->in_end - src) {
    cnt = MAX (c->in_end - src, 0);
    c->error |= LZO_INPUT_DEPLETED;
  }
  if (cnt > c->out_end - dst) {
    cnt = MAX (c->out_end - dst, 0);
    c->error |= LZO_OUTPUT_FULL;
  }
#if defined(INBUF_PADDED) && defined(OUTBUF_PADDED)
  COPY4 (dst, src);
  src += 4;
  dst += 4;
  cnt -= 4;
  if (cnt > 0)
#endif
    memcpy (dst, src, cnt);
  c->in = src + cnt;
  c->out = dst + cnt;
}

/*
 * \brief copy previously decoded bytes to current position
 * \param back how many bytes back we start
 * \param cnt number of bytes to copy, must be >= 0
 *
 * cnt > back is valid, this will copy the bytes we just copied,
 * thus creating a repeating pattern with a period length of back.
 */
static inline void
copy_backptr (LZOContext * c, int back, int cnt)
{
  register const guint8 *src = &c->out[-back];
  register guint8 *dst = c->out;
  if (src < c->out_start || src > dst) {
    c->error |= LZO_INVALID_BACKPTR;
    return;
  }
  if (cnt > c->out_end - dst) {
    cnt = MAX (c->out_end - dst, 0);
    c->error |= LZO_OUTPUT_FULL;
  }
  if (back == 1) {
    memset (dst, *src, cnt);
    dst += cnt;
  } else {
#ifdef OUTBUF_PADDED
    COPY2 (dst, src);
    COPY2 (dst + 2, src + 2);
    src += 4;
    dst += 4;
    cnt -= 4;
    if (cnt > 0) {
      COPY2 (dst, src);
      COPY2 (dst + 2, src + 2);
      COPY2 (dst + 4, src + 4);
      COPY2 (dst + 6, src + 6);
      src += 8;
      dst += 8;
      cnt -= 8;
    }
#endif
    if (cnt > 0) {
      int blocklen = back;
      while (cnt > blocklen) {
        memcpy (dst, src, blocklen);
        dst += blocklen;
        cnt -= blocklen;
        blocklen <<= 1;
      }
      memcpy (dst, src, cnt);
    }
    dst += cnt;
  }
  c->out = dst;
}

/*
 * \brief decode LZO 1x compressed data
 * \param out output buffer
 * \param outlen size of output buffer, number of bytes left are returned here
 * \param in input buffer
 * \param inlen size of input buffer, number of bytes left are returned here
 * \return 0 on success, otherwise error flags, see lzo.h
 *
 * make sure all buffers are appropriately padded, in must provide
 * LZO_INPUT_PADDING, out must provide LZO_OUTPUT_PADDING additional bytes
 */
int
lzo1x_decode (void *out, int *outlen, const void *in, int *inlen)
{
  int state = 0;
  int x;
  LZOContext c;
  c.in = in;
  c.in_end = (const guint8 *) in + *inlen;
  c.out = c.out_start = out;
  c.out_end = (guint8 *) out + *outlen;
  c.error = 0;
  x = GETB (c);
  if (x > 17) {
    copy (&c, x - 17);
    x = GETB (c);
    if (x < 16)
      c.error |= LZO_ERROR;
  }
  if (c.in > c.in_end)
    c.error |= LZO_INPUT_DEPLETED;
  while (!c.error) {
    int cnt, back;
    if (x > 15) {
      if (x > 63) {
        cnt = (x >> 5) - 1;
        back = (GETB (c) << 3) + ((x >> 2) & 7) + 1;
      } else if (x > 31) {
        cnt = get_len (&c, x, 31);
        x = GETB (c);
        back = (GETB (c) << 6) + (x >> 2) + 1;
      } else {
        cnt = get_len (&c, x, 7);
        back = (1 << 14) + ((x & 8) << 11);
        x = GETB (c);
        back += (GETB (c) << 6) + (x >> 2);
        if (back == (1 << 14)) {
          if (cnt != 1)
            c.error |= LZO_ERROR;
          break;
        }
      }
    } else if (!state) {
      cnt = get_len (&c, x, 15);
      copy (&c, cnt + 3);
      x = GETB (c);
      if (x > 15)
        continue;
      cnt = 1;
      back = (1 << 11) + (GETB (c) << 2) + (x >> 2) + 1;
    } else {
      cnt = 0;
      back = (GETB (c) << 2) + (x >> 2) + 1;
    }
    copy_backptr (&c, back, cnt + 2);
    state = cnt = x & 3;
    copy (&c, cnt);
    x = GETB (c);
  }
  *inlen = c.in_end - c.in;
  if (c.in > c.in_end)
    *inlen = 0;
  *outlen = c.out_end - c.out;
  return c.error;
}

#ifdef TEST
#include <stdio.h>
#include <lzo/lzo1x.h>
#include "log.h"
#define MAXSZ (10*1024*1024)
int
main (int argc, char *argv[])
{
  FILE *in = fopen (argv[1], "rb");
  guint8 *orig = av_malloc (MAXSZ + 16);
  guint8 *comp = av_malloc (2 * MAXSZ + 16);
  guint8 *decomp = av_malloc (MAXSZ + 16);
  gsize s = fread (orig, 1, MAXSZ, in);
  lzo_uint clen = 0;
  long tmp[LZO1X_MEM_COMPRESS];
  int inlen, outlen;
  int i;
  av_log_level = AV_LOG_DEBUG;
  lzo1x_999_compress (orig, s, comp, &clen, tmp);
  for (i = 0; i < 300; i++) {
    START_TIMER inlen = clen;
    outlen = MAXSZ;
#ifdef LIBLZO
    if (lzo1x_decompress_safe (comp, inlen, decomp, &outlen, NULL))
#elif defined(LIBLZO_UNSAFE)
    if (lzo1x_decompress (comp, inlen, decomp, &outlen, NULL))
#else
    if (lzo1x_decode (decomp, &outlen, comp, &inlen))
#endif
      av_log (NULL, AV_LOG_ERROR, "decompression error\n");
    STOP_TIMER ("lzod")
  }
  if (memcmp (orig, decomp, s))
    av_log (NULL, AV_LOG_ERROR, "decompression incorrect\n");
  else
    av_log (NULL, AV_LOG_ERROR, "decompression ok\n");

  fclose (in);
  return 0;
}
#endif
