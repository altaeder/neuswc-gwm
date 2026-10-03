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
#include <emmintrin.h>

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

static inline __m128i
pixel_to_vector(uint32_t pixel)
{
  return _mm_setr_epi32 // <-- set*r* r=reverse (why not default intel, why?)
  (
    0,
    (pixel >> 16) & 0xff,
    (pixel >> 8) & 0xff,
    pixel & 0xff
  );
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
      ctx.dst,
      ctx.src,
      source,
      &expanded,
      0,
      1);

  copy_region(
      source->map,
      ctx.dst,
      source,
      region);

 blur_pass(radius,
      ctx.src,
      ctx.dst,
      source,
      &expanded,
      1,
      0);

  copy_region(
      source->map,
      ctx.src,
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
            dst,
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
  __m128i
  color_total =
    _mm_set_epi32
    (
       0,
       0,
       0,
       0
     );

  int count = 0;

  int first_x = dx ? start : fixed;
  int first_y = dy ? start : fixed;

  // FIRST PIXEL GRAB
  for (int i = -radius; i <= radius; i++)
  {
    int position = start + i;

    int x = dx ? position : fixed;
    int y = dy ? position : fixed;

    if (x < 0 ||
        x >= buffer->width ||
        y < 0 ||
        y >= buffer->height)
      continue;

    uint32_t pixel_init =
      get_target(
          buffer,
          src,
          x,
          y);

    // Ω SIMD MAGICK Ω
    __m128i
    color_init =
      pixel_to_vector(pixel_init);

    color_total =
      _mm_add_epi32
      (
        color_total,
        color_init
      );

    count++;
  }
  // FIRST PIXEL GRAB END

  if(count <= 0)
    return;

  uint32_t rgb[4]; // Ω STORE Ω
    _mm_storeu_si128(
        (__m128i*)rgb,
        color_total);

  // average
  uint32_t r_avg = rgb[1] / count;
  uint32_t g_avg = rgb[2] / count;
  uint32_t b_avg = rgb[3] / count;

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
  //----------- SLIDE, cha cha real smooth
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


    // Slide calc minus
    if(inside_buffer(buffer, leave_x, leave_y))
    {
      uint32_t pixel_leave =
        get_target(
              buffer,
              src,
              leave_x,
              leave_y);

      // Ω Ω SIMD MAGICK Ω Ω

      __m128i // Ω STORE, reg->SimD Ω
      color_leaving =
        pixel_to_vector(pixel_leave);

      // __m128i
      color_total =
      _mm_sub_epi32
        (
          color_total,
          color_leaving
        );

      count--;
    }

    // Slide calc plus
    if(inside_buffer(buffer, enter_x, enter_y))
    {
      uint32_t pixel_enter =
        get_target(
              buffer,
              src,
              enter_x,
              enter_y);

      // SIMD MAGICK Ω

      __m128i // Ω STORE funct, reg->SimD Ω
      color_entering =
        pixel_to_vector(pixel_enter);

      color_total = // Ω SimD CALC Ω
        _mm_add_epi32
        (
          color_total,
          color_entering
        );

      count++;
    }
    // X and Y DONE

    if(__builtin_expect(count <= 0, 0))
      continue;

    // Ω store to regular, simd->reg
    uint32_t rgb_slide[4];
      _mm_storeu_si128(
          (__m128i*)rgb_slide,
         color_total);

    // average
    uint32_t r_avg = rgb_slide[1] / count;
    uint32_t g_avg = rgb_slide[2] / count;
    uint32_t b_avg = rgb_slide[3] / count;

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