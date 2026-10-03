// ./effects/effects.c

/*
AYO sup mårfockers, the use of this file is by my accounts completely unrestricted. I consider it free for whomever, to do whatever they wish. All use is on the responsibility of that user however, as i dont know wtf i'm doing. Thats a warning. It might break your system, summon the gates of hell or make you a millionaire. Probably not. Point being, if something breaks, i dont even know if im capable of fixing it myself. Take that as you will :)

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
###########   shader effects engine dispatcher   ####################
#####################################################################
*/

/* TODO
expand dispatcher to more generic handler
ex:
if (surface->state.blur)
    effect_blur(view);

if (surface->state.shadow)
    effect_shadow(view);

if (surface->state.tint)
    effect_tint(view);
*/

#include "effects.h"
#include "blur_box.h"
#include "blur_hvpass_slide.h"
#include "debug.h"
#include "../surface.h"

#include <string.h>
#include <stdio.h>

void
effects_apply(struct wld_renderer *renderer,
              struct compositor_view *view,
              int32_t screen_x,
              int32_t screen_y,
              pixman_region32_t *buffer_damage)
{
  struct wld_buffer *target = renderer->target;

  /*pixman_region32_t screen_region;
  pixman_region32_init(&screen_region);
  pixman_region32_copy(&screen_region, buffer_damage);
  */

  pixman_region32_t test_damage;
  pixman_region32_init(&test_damage);
  if (!view) {
    //fprintf(stderr, "SHADER DEBUG: no view :( \n");
    pixman_region32_fini(&test_damage);
    return;
  }
  pixman_region32_copy(&test_damage, buffer_damage);
  pixman_region32_translate(&test_damage,
                            screen_x,
                            screen_y);

  int radius = 0;
  expand_region(&test_damage,
                target->width,
                target->height,
                radius);


  // Effect ACTIVATE!!
  //if (pixman_region32_not_empty(&view->surface->state.blur))
    //effect_blur_hvpass_slide(renderer->target, &screen_region);
    //effect_blur_hvpass_slide(renderer->target, &test_damage);
  effect_blur_hvpass_slide(target, &test_damage);

  #ifdef DEBUG_EFFECTS
      effect_debug(renderer->target, test_damage);
  #endif

  //pixman_region32_fini(&screen_region);
  pixman_region32_fini(&test_damage);
}


//////////////////////////// Helper tools //////////////////////////////

// Cpu optimized region selecter
void
expand_region(pixman_region32_t *region,
              int32_t width,
              int32_t height,
              int amount)
{
  int n;
  pixman_box32_t *boxes =
    pixman_region32_rectangles(region, &n);

  pixman_region32_t expanded;
  pixman_region32_init(&expanded);

  for (int i = 0; i < n; i++)
  {
    pixman_box32_t box = boxes[i];

    box.x1 -= amount;
    box.y1 -= amount;
    box.x2 += amount;
    box.y2 += amount;

    if (box.x1 < 0)
        box.x1 = 0;
    if (box.y1 < 0)
        box.y1 = 0;
    if (box.x2 > width)
        box.x2 = width;
    if (box.y2 > height)
        box.y2 = height;

    pixman_region32_union_rect(
      &expanded,
      &expanded,
      box.x1,
      box.y1,
      box.x2 - box.x1,
      box.y2 - box.y1);
  }
  pixman_region32_copy(region, &expanded);
  pixman_region32_fini(&expanded);
}

// Optimized buffer&area-copying
void
copy_region(uint32_t *dst,
                 uint32_t *src,
                 struct wld_buffer *buffer,
                 pixman_region32_t *region)
{
  if (!dst || !src) {
    return;
  }

  int n;
  pixman_box32_t *rect =
    pixman_region32_rectangles(region, &n);

  for (int r = 0; r < n; r++)
  {
    pixman_box32_t *box = &rect[r];
    for (int y = box->y1; y < box->y2; y++)
    {
      memcpy(
        (uint8_t *)dst
          + y * buffer->pitch
          + box->x1 * 4,

        (uint8_t *)src
          + y * buffer->pitch
          + box->x1 * 4,

        (box->x2 - box->x1) * 4);

      /*(uint32_t *src_row =
        (uint32_t *)(
          (uint8_t *)source->map +
          y * source->pitch +
          box->x1 * 4);

      memcpy(
        dst_row,
        src_row,
        (box->x2 - box->x1) * 4);*/
    }
  }
}



/////////////////////////////////////////////////////////////////////////////////////////
//// Misc

/*
    // Force the pink!
    pixman_region32_t region;
    pixman_region32_init_rect(
        &region,
        x,
        y,
        200,
        200
    );

    wld_fill_region(
        renderer,
        0xfffe019a,
        &region
    );
    pixman_region32_fini(&region);
  */


  // LATER
  /*pixman_region32_intersect(
    &effect_region,
    &view->surface->state.blur,
    buffer_damage);*/



    //////////////////// buffer implement draft/////
/*
effects_apply()
{
    if (!buffer)
        return;

    if (!(wld_capabilities(renderer, buffer)
          & WLD_CAPABILITY_READ))
        return;

    if (!wld_map(buffer))
        return;

    effect_box_blur(buffer, region);

    wld_unmap(buffer);
}

*/