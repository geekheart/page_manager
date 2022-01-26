#pragma once

#include "page_manager.h"

/* page_base */
page_base_t *page_base_create(void);
void page_base_delete(void *self);
page_base_t *find_page_pool(page_manager_t *self, const char *name);
page_base_t *find_page_stack(page_manager_t *self, const char *name);
page_base_t *get_stack_top(page_manager_t *self);
page_base_t *get_stack_top_after(page_manager_t *self);
void set_satck_clear(page_manager_t *self, bool keep_bottom);
const char *get_page_prev_name(page_manager_t *self);

/* page_anim */
page_load_anim_t page_get_current_load_anim_type(page_manager_t *self);
bool page_get_current_load_anim_attr(page_manager_t *self, page_load_anim_attr_t *attr);

/* page_drag */
void page_root_drag_event(lv_obj_t *obj, lv_event_t event);
void root_enable_drag(lv_obj_t *root);
void root_get_drag_predict(lv_coord_t *x, lv_coord_t *y);

/* page_router */
bool fource_unload(page_base_t *base);
void switch_anim_create(page_manager_t *self, page_base_t *base);
void anim_default_init(page_manager_t *self, lv_anim_t *a);

/* page_state */
void page_state_update(page_manager_t *self, page_base_t *base);
page_state_t state_unload_execute(page_base_t *base);