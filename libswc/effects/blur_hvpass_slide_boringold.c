//blur_hvpass_slide.c, part of the shader effects package

/*
#####################################################################
#######                 HV passed Box Blur                ###########
#######     - Now with sliding window optimizations -     ###########
#####################################################################
*/

#include "effects.h"
#include "blur_hvpass_slide.h"
#include "../surface.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Forward declarations, so we can keep a nice readable structure

static void
blur_pass(int radius,
          uint32_t *src,
          uint32_t *dst,
          struct wld_buffer *buffer,
          pixman_region32_t *damage,
          int dx,
          int dy);

static void
directional_slide_fx(struct wld_buffer *buffer,
              uint32_t *src,
              uint32_t *dst,
              int fixed,
              int start,
              int end,
              int radius,
              int dx,
              int dy);

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

// bounds checker
static inline int
inside_buffer(struct wld_buffer *buffer, int x, int y)
{
    return x >= 0 &&
           y >= 0 &&
           x < buffer->width &&
           y < buffer->height;
}

/////////////////// Main effect functions ///////////////////////////////
void
effect_blur_hvpass_slide(struct wld_buffer *source,
                   pixman_region32_t *region)
{
  if(!wld_map(source))
    return;
  int radius = 3;

  size_t size =
    source->pitch * source->height;

  blur_context_resize(size);

  // optim selection
  pixman_region32_t expanded;
  pixman_region32_init(&expanded);
  pixman_region32_copy(&expanded, region);

  /*expand_region(&expanded,
                source->width, //for the clamping
                source->height, //for the clamping
                radius);*/

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
  blur_pass(radius,
      ctx.dst, //Ω SWITCHED STATE
      ctx.src,
      source,
      &expanded,
      0,
      1);

  copy_region(
      source->map,
      ctx.dst, // Ω extra copy?
      source,
      region);

 blur_pass(radius,
      ctx.src, //Ω switched, TO unALIGN
      ctx.dst,
      source,
      &expanded,
      1,
      0);

  copy_region(
      source->map,
      ctx.src, // Ω LAST try (OG .src
      source,
      region);

  pixman_region32_fini(&expanded);

  wld_unmap(source);
}

//////////////////////////      DIRECTIONAL BLUR PASS    ///////////////////////////
static void
blur_pass(int radius,
          uint32_t *src,
          uint32_t *dst,
          struct wld_buffer *buffer,
          pixman_region32_t *damage,
          int dx,
          int dy)
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
    pixman_box32_t *rect=&rects[r];
    if (dx)
    {
      for (int fixed = rect->y1;
           fixed < rect->y2;
           fixed++)
      {
        directional_slide_fx(buffer,
            dst, // SWITCHED, DIFFERENT BUFFER COPIES, TEST REVER MEBE Ω
            src,
            fixed, // y axis fixed
            rect->x1, // x start
            rect->x2, // x end
            radius, //radius
            dx,
            dy);

      }
    }
    else
    {
      for (int fixed = rect->x1;
           fixed < rect->x2;
           fixed++)
      {
        directional_slide_fx(buffer,
            dst, //  <-- PIN
            src,
            fixed, // x axis fixed
            rect->y1, // y start
            rect->y2, // y end
            radius, //radius
            dx,
            dy);

      }
    }
  }
}

// the effect
static void
directional_slide_fx(struct wld_buffer *buffer,
                     uint32_t *src,
                     uint32_t *dst,
                     int fixed,
                     int start,
                     int end,
                     int radius,
                     int dx,
                     int dy)
{
  uint32_t r_total = 0;
  uint32_t g_total = 0;
  uint32_t b_total = 0;
  int count = 0;

  int first_x = dx ? start : fixed;
  int first_y = dy ? start : fixed;

  for (int i = -radius; i <= radius; i++)
  {
    int position = start + i;

    int x = dx ? position : fixed;
    int y = dy ? position : fixed;

    if(x < 0 ||
       x >= buffer->width ||
       y < 0 ||
       y >= buffer->height)
    continue;

    uint32_t pixel =
      get_target(
          buffer,
          src,
          x,
          y);

    uint8_t r = (pixel >> 16) & 0xff;
    uint8_t g = (pixel >> 8) & 0xff;
    uint8_t b = pixel & 0xff;

    r_total += r;
    g_total += g;
    b_total += b;

    count++;
  }

  // average
  if(count <= 0)
    return;
  uint32_t r_avg = r_total / count;
  uint32_t g_avg = g_total / count;
  uint32_t b_avg = b_total / count;

  uint32_t fx_target =
      0xff000000 |
      (r_avg << 16) |
      (g_avg << 8) |
       b_avg;

  if(inside_buffer(buffer, first_x, first_y))
  {
    apply_fx(
            dst,
            buffer->pitch,
            first_x,
            first_y,
            fx_target);
  }

  // now sliiiiide, cha cha real smooth
  for (int position = start + 1; position < end; position++)
  {
    int x = dx ? position : fixed;
    int y = dy ? position : fixed;

    int leave_position = position - radius - 1;
    int leave_x = dx ? leave_position : fixed;
    int leave_y = dy ? leave_position : fixed;

    int enter_position = position + radius;
    int enter_x = dx ? enter_position : fixed;
    int enter_y = dy ? enter_position : fixed;

    // Slide calc
    if(inside_buffer(buffer, leave_x, leave_y))
    {
      uint32_t pixel_leave =
        get_target(
              buffer,
              src,
              leave_x,
              leave_y);

      uint8_t r_leave =
         (pixel_leave >> 16) & 0xff;
      uint8_t g_leave =
         (pixel_leave >> 8) & 0xff;
      uint8_t b_leave =
          pixel_leave & 0xff;

      r_total -= r_leave;
      g_total -= g_leave;
      b_total -= b_leave;
      count--;
    }

    if(inside_buffer(buffer, enter_x, enter_y))
    {
      uint32_t pixel_enter =
        get_target(
              buffer,
              src,
              enter_x,
              enter_y);

      uint8_t r_enter =
         (pixel_enter >> 16) & 0xff;
      uint8_t g_enter =
         (pixel_enter >> 8) & 0xff;
      uint8_t b_enter =
          pixel_enter & 0xff;

      r_total += r_enter;
      g_total += g_enter;
      b_total += b_enter;
      count++;
    }

    // average
    if(count <= 0)
      return;
    uint32_t r_avg = r_total / count;
    uint32_t g_avg = g_total / count;
    uint32_t b_avg = b_total / count;

    //pack
    fx_target =
      0xff000000 |
      (r_avg << 16) |
      (g_avg << 8) |
      b_avg;

    if(inside_buffer(buffer, x, y))
    {
      apply_fx(
              dst,
              buffer->pitch,
              x,
              y,
              fx_target); // we commit the slided values
    }
  }
}

// grab target pixel
static inline uint32_t
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
      + x * 4
    )
    = fx_target;
}