//debug.c, part of the shader effects package

/*
READ MEEEEEEEEEEEEEEEEEEE, plz

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
###########          COMPLETELY GODAWFUL AND          ####################
########### THEREFORE IMMIDIETLY VISIBLE COLOR INJECT #####################
#####################################################################
*/

#include "debug.h"

#include "../surface.h"
//#include "../compositor.h"

//include <wld/wld.h>
//#include <pixman.h>

#include <stdio.h>

void
effects_debug_color(struct wld_renderer *renderer,
                    pixman_region32_t *screen_region)
/*effects_debug_color(struct wld_renderer *renderer,
                    struct compositor_view *view,
                    pixman_region32_t *buffer_damage)*/
{
  //pixman_region32_t region;
  //pixman_region32_init(&region);

  //pixman_region32_copy(
  //  &region,
  //  buffer_damage //&surface->state.blur
  //);


  //pixman_region32_t region, geom_region, buffer_region, border_region, view_damage, buffer_damage, border_damage;

  //pixman_region32_init(&buffer_region);

  /*pixman_region32_translate(
    &region,
    &effect_region
  );*/

  wld_fill_region(
    renderer,
    0xaafe019a, // Hot pink :) (godawfulnnes up to interpretation)
    //region
    screen_region
  );

  //pixman_region32_fini(&region);
  //fprintf(stderr, "effect_debug_color()\n");
}