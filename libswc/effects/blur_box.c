//blur_box.c, part of the shader effects package

/*
READ MEEEEEEEEEEEEEEEEEEE, plz

AYO
sup mårfockers, the use of this file is by my accounts completely unrestricted. I consider it free for whomever, to do whatever they wish. All use is on the responsibility of that user however, this is experimental software, USE AT YOUR OWN RISK.

(Who doesnt like living on the edge anyway?)

Do remember its bad style to not give credit. If you steal my code 1-1, thats a bit of a dick move my guy. I hope you stub your toe on the counter. A little shoutout somewhere is all, its just good manners.

That out of the way, have fun with it!
                                    ^(thats an order.)

all the best,
Thor Valentin, August 2026

Kudos:
Michael Forney
--
--
--
--
*/

/*
#####################################################################
###########               Box Blurring                ###############
###########           - simple and clean -            ###############
#####################################################################
*/

#include "effects.h"
#include "blur_box.h"
#include "../surface.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Forward declarations, so we can keep a nice readable structure

static void
blur_region(uint32_t *src,
            uint32_t *dst,
            struct wld_buffer *buffer,
            pixman_region32_t *damage);

static uint32_t
box_fx(struct wld_buffer *buffer,
       uint32_t *src,
       uint32_t x,
       uint32_t y,
       int radius);

static inline uint32_t
get_target(struct wld_buffer *buffer,
          uint32_t *src,
          int x,
          int y);

static inline void
apply_fx(uint32_t *dst,
         uint32_t pitch,
         int x,
         int y,
         uint32_t fx_target);

struct blur_context {
    uint32_t *src;
    uint32_t *dst;
    size_t size;
  };

static struct blur_context ctx = {
  .src = NULL,
  .dst = NULL,
  .size = 0
};
/////////


// Optimization helper functions
static void
blur_context_resize(size_t size)
{
  if (ctx.size >= size)
    return;

  uint32_t *new_src = malloc(size);
  uint32_t *new_dst = malloc(size);

  if (!new_src || !new_dst) {
    free(new_src);
    free(new_dst);
    return;
  }

  free(ctx.src);
  free(ctx.dst);

  ctx.src = new_src;
  ctx.dst = new_dst;
  ctx.size = size;

  memset(ctx.src, 0, size);
  memset(ctx.dst, 0, size);
}

/////////////////// Main effect functions ///////////////////////////////
void
effect_blur_box(struct wld_buffer *source,
                pixman_region32_t *region)
{
  if(!wld_map(source))
    return;

  size_t size =
    source->pitch * source->height;

  blur_context_resize(size);

  // optim selection
  pixman_region32_t expanded;
  pixman_region32_init(&expanded);
  pixman_region32_copy(&expanded, region);

  expand_region(&expanded,
                source->width, //for the clamping
                source->height, //for the clamping
                3);

  // optim memcpy
  copy_region(
    ctx.src,
    source->map,
    source,
    &expanded);

  copy_region(
    ctx.dst,
    ctx.src,
    source,
    &expanded);

  // effect call
  blur_region(
      ctx.src,
      ctx.dst,
      source,
      &expanded);

  copy_region(
      source->map,
      ctx.dst,
      source,
      region);

  pixman_region32_fini(&expanded);

  wld_unmap(source);
}

// pixel, buffer & region selection
static void
blur_region(uint32_t *src,
            uint32_t *dst,
            struct wld_buffer *buffer,
            pixman_region32_t *damage)
{
  int n;

  pixman_box32_t *rects =
    pixman_region32_rectangles(
      damage,
      &n);

  for (
    int r = 0;
     r < n;
     r++)
  {
    pixman_box32_t *rect = &rects[r];
    for (int y = rect->y1;
        y < rect->y2;
        y++)
    {
      for (int x = rect->x1;
          x < rect->x2;
          x++)
      {
        uint32_t fx_target =
        box_fx(buffer,
            src,
            x,
            y,
            3); // radius

        apply_fx(
            dst,
            buffer->pitch,
            x,
            y,
            fx_target);
      }
    }
  }
}

// the effect
static uint32_t
box_fx(struct wld_buffer *buffer,
       uint32_t *src,
       uint32_t x,
       uint32_t y,
       int radius)
{
  uint32_t r_total = 0;
  uint32_t g_total = 0;
  uint32_t b_total = 0;
  uint32_t count = 0;

  // loop
  for (int oy = -radius;
    oy <= radius;
    oy++)
  {
    for (int ox = -radius;
      ox <= radius;
      ox++)
    {
      int nx = x + ox;
      int ny = y + oy;

      if(
        nx < 0 ||
        ny < 0 ||
        nx >= buffer->width ||
        ny >= buffer->height)
      continue;

      uint32_t neighbour =
        get_target(
            buffer,
            src,
            nx,
            ny);

      // unpack
     uint8_t r =
         (neighbour >> 16) & 0xff;

      uint8_t g =
         (neighbour >> 8) & 0xff;

      uint8_t b =
          neighbour & 0xff;

      r_total += r;
      g_total += g;
      b_total += b;

      count++;
    }
  }

  // average
  r_total /= count;
  g_total /= count;
  b_total /= count;
  /*r_total = (r_total * (1000 + radius*17) >> 16;
  g_total = (g_total * (1000 + radius*17) >> 16;
  b_total = (b_total * (1000 + radius*17) >> 16; */

  //pack
  return
    0xff000000 |
    (r_total << 16) |
    (g_total << 8) |
    b_total;
}

// pr pixel neighbour calc
static inline uint32_t // <---- or is THIS the copy_region?
get_target(struct wld_buffer *buffer,
           uint32_t *src,
           int x,
           int y)
{
  uint32_t *pixel =
    (uint32_t *)
      (
        (uint8_t *)src +
        y * buffer->pitch
        //+ x * 4
      );
  return pixel[x];
}


// write pixel(s)
static inline void
apply_fx(uint32_t *dst,
         uint32_t pitch,
         int x,
         int y,
         uint32_t fx_target)
{
  *(uint32_t *)
    (
      (uint8_t *)dst
      + y * pitch
      + x * 4)
      = fx_target;
}





/* MISC

// Ω pixel state debuggers PRE Ω
  fprintf(stderr,
          "blur buffer=%p\n",
          source);
  fprintf(stderr,
        "first pixel: %08x\n",
        src[0]);
  fprintf(stderr,
        "SRC_PRE pixel: %08x\n",
        src[0]);
  fprintf(stderr,
        "DST pixel: %08x\n",
        dst[0]);

  // Ω pixel state debuggers POST Ω
  fprintf(stderr,
        "SRC_POST pixel: %08x\n",
        src[0]);
  fprintf(stderr,
        "DST pixel: %08x\n",
        dst[0]);
  fprintf(stderr,
        "src=%p dst=%p map=%p\n",
        src,
        dst,
        source->map);



*/