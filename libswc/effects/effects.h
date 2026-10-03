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
###########          shader effects handler          ####################
#####################################################################
*/

#ifndef SWC_EFFECT_H
#define SWC_EFFECT_H

#include "../view.h"
#include "../compositor.h"
//#include <wld/wld.h>
//#include <pixman.h>

struct surface;
struct wld_renderer;

void
effects_apply(struct wld_renderer *renderer,
              struct compositor_view *view,
              int32_t sceen_x,
              int32_t screen_y,
              pixman_region32_t *buffer_damage);

void
expand_region(pixman_region32_t *region,
              int32_t width,
              int32_t heigth,
              int amount);

void
copy_region(uint32_t *dst,
                 uint32_t *src,
                 struct wld_buffer *buffer,
                 pixman_region32_t *region);

#endif