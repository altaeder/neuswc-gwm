#ifndef SWC_BACKGROUND_EFFECT_MANAGER_H
#define SWC_BACKGROUND_EFFECT_MANAGER_H

struct wl_display;
struct wl_global;

struct wl_global *
background_effect_create(struct wl_display *display);

#endif