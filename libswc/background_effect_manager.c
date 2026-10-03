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
##############################################################################
###########          BACKGROUND EFFECT MANAGER          ######################
###########         -! Custom shader engine !-          #######################
#### - implementation of ext_background_effect_v1 wayland protocol - #########
##############################################################################
*/


#include "background_effect_manager.h"

#include "compositor.h"
#include "internal.h"
#include "seat.h"
#include "surface.h"
#include "util.h"

#include "ext-background-effect-v1-server-protocol.h"
#include <assert.h>
#include <stdlib.h>
#include <wayland-server.h>


/*
TODO:
Declare unused paramateres as void to shut up compiler
(void)client;
(void)resource;
(void)region;
(void)surface_resource;
*/


/*
 * Represents one ext_background_effect_surface_v1 object.
 * The wl_resource user data points here.
*/
struct background_effect_surface {
  struct surface *surface;
  struct wl_resource *resource;
  // struct wl_listener surface_destroy; // <--- FOR LATER
};


// Forward declarations to handle constant structs at bottom
static const struct ext_background_effect_surface_v1_interface
background_effect_surface_impl;

static const struct ext_background_effect_manager_v1_interface
background_effect_manager_impl;


// Destroy
static void
destroy_manager(struct wl_client *client,
                struct wl_resource *resource)
{
    (void)client;
    wl_resource_destroy(resource);
}

static void
destroy_surface(struct wl_client *client,
                struct wl_resource *resource)
{
    (void)client;
    wl_resource_destroy(resource);
}

static void
background_effect_destroy(struct wl_resource *resource)
{
  struct background_effect_surface *effect =
    wl_resource_get_user_data(resource);

  free(effect);
}

// Set Blur region
static void
set_blur_region(struct wl_client *client,
                struct wl_resource *resource,
                struct wl_resource *region_resource)
{
  (void)client;
  struct background_effect_surface *effect =
    wl_resource_get_user_data(resource);

  struct surface *surface = effect->surface;

  surface->pending.commit |= SURFACE_COMMIT_BLUR;

  if (region_resource) {
    pixman_region32_t *region =
      wl_resource_get_user_data(region_resource);

    pixman_region32_copy(
      &surface->pending.state.blur,
      region);
  } else {
    pixman_region32_clear(
      &surface->pending.state.blur);
  }
}


// Get Effect ---------- MEAT AND BREAD! -----------------
static void
get_background_effect(struct wl_client *client,
                      struct wl_resource *resource,
                      uint32_t id,
                      struct wl_resource *surface_resource)
{
  struct surface *surface =
    wl_resource_get_user_data(surface_resource);

  struct wl_resource *effect;

  effect = wl_resource_create(
    client,
    &ext_background_effect_surface_v1_interface,
    wl_resource_get_version(resource),
    id);

  if (!effect) {
    wl_client_post_no_memory(client);
    return;
  }

  // Wrapper
  struct background_effect_surface *background_effect;

  background_effect = malloc(sizeof(*background_effect));

  if (!background_effect) {
    wl_resource_destroy(effect);
    wl_client_post_no_memory(client);
    return;
  }

  background_effect->surface = surface;
  background_effect->resource = effect;
  //

  wl_resource_set_implementation(
    effect,
    &background_effect_surface_impl,
    background_effect,
    &background_effect_destroy);
}


// Bind Effect
static void
bind_background_effect(struct wl_client *client,
                       void *data,
                       uint32_t version,
                       uint32_t id)
{
    (void)data;
    (void)version;

    struct wl_resource *resource;
    resource =
      wl_resource_create(client,
                         &ext_background_effect_manager_v1_interface,
                         version,
                         id);
    if (!resource) {
      return;
    }

    wl_resource_set_implementation(
        resource,
        &background_effect_manager_impl,
        NULL,
        NULL);

    ext_background_effect_manager_v1_send_capabilities(
        resource,
        EXT_BACKGROUND_EFFECT_MANAGER_V1_CAPABILITY_BLUR);
        //0);
}

// Create Effect
struct wl_global *
background_effect_create(struct wl_display *display)
{
    return wl_global_create(
        display,
        &ext_background_effect_manager_v1_interface,
        1,
        NULL,
        bind_background_effect);
}

// Static structs
static const struct ext_background_effect_surface_v1_interface
background_effect_surface_impl = {
    .destroy = destroy_surface,
    .set_blur_region = set_blur_region,
};

static const struct ext_background_effect_manager_v1_interface
background_effect_manager_impl = {
    .destroy = destroy_manager,
    .get_background_effect = get_background_effect,
};
