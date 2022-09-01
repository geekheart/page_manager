#include "page_manager_private.h"

static page_state_t _state_load_execute(page_manager_t *self, page_base_t *base);
static page_state_t _state_will_appear_execute(page_manager_t *self, page_base_t *base);
static page_state_t _state_did_appear_execute(page_base_t *base);
static page_state_t _state_will_disappear_execute(page_manager_t *self, page_base_t *base);
static page_state_t _state_did_disappear_execute(page_manager_t *self, page_base_t *base);
static bool _get_is_over_anim(uint8_t anim);

/**
 * @brief 页面更新
 * 
 * @param self 页面管理器对象
 * @param base 页面对象
 */
void page_state_update(page_manager_t *self, page_base_t *base)
{
    if (base == NULL)
        return;

    switch (base->priv.state)
    {
    // 页面被卸载后进入空闲状态
    case PAGE_STATE_IDLE:
        PM_LOG_INFO("Page(%s) state idle", base->name);
        break;
    // 页面没有被缓存时,第一次加载进入这里
    // 该状态下执行on_view_load函数,创建root对象
    // 立即切换到PAGE_STATE_WILL_APPEAR
    case PAGE_STATE_LOAD:
        base->priv.state = _state_load_execute(self, base);
        page_state_update(self, base);
        break;

    // 加载状态过后或者页面有被缓存会进入到这里
    // 该状态下在动画执行之前会执行on_view_will_appear函数并且初始化切换动画
    // 动画结束后切换到PAGE_STATE_DID_APPEAR
    case PAGE_STATE_WILL_APPEAR:
        base->priv.state = _state_will_appear_execute(self, base);
        break;

    // 由页面切换动画结束后进入这里
    // 该状态下会执行on_view_did_appear
    // 该状态执行完毕后会长期停留在PAGE_STATE_ACTIVITY状态
    case PAGE_STATE_DID_APPEAR:
        base->priv.state = _state_did_appear_execute(base);
        PM_LOG_INFO("Page(%s) state active", base->name);
        break;

    // 页面会在切换的时候,被切换的页面会进入这里
    // 该状态会立即转到PAGE_STATE_WILL_DISAPPEAR
    case PAGE_STATE_ACTIVITY:
        PM_LOG_INFO("Page(%s) state active break", base->name);
        base->priv.state = PAGE_STATE_WILL_DISAPPEAR;
        page_state_update(self, base);
        break;
    
    // 被切换的页面会进入到这里
    // 该状态会在动画开始前执行on_view_will_disappear,并且加载关机动画
    // 动画结束后进入PAGE_STATE_DID_DISAPPEAR
    case PAGE_STATE_WILL_DISAPPEAR:
        base->priv.state = _state_will_disappear_execute(self, base);
        break;

    // 结束动画播放完毕后进入这里
    // 该状态会执行on_view_did_disappear
    // 如果开启缓存,状态会转换成PAGE_STATE_WILL_APPEAR,如果没开启缓存则会进入PAGE_STATE_UNLOAD
    case PAGE_STATE_DID_DISAPPEAR:
        base->priv.state = _state_did_disappear_execute(self, base);
        if (base->priv.state == PAGE_STATE_UNLOAD)
        {
            page_state_update(self, base);
        }
        break;

    // 注销页面或者是关闭页面缓存的时候会进入该状态
    // 该状态下会回收相关的页面对象,并且执行on_view_did_unload
    // 该状态结束后进入PAGE_STATE_IDLE
    case PAGE_STATE_UNLOAD:
        base->priv.state = state_unload_execute(base);
        break;

    default:
        PM_LOG_ERROR("Page(%s) state[%d] was NOT FOUND!", base->name, base->priv.state);
        break;
    }
}

/**
 * @brief 将lvgl根对象初始化，并且调用on_view_load()
 * 
 * @param self 页面管理器对象 
 * @param base 页面对象
 * @return page_state_t 下一个页面状态
 */
static page_state_t _state_load_execute(page_manager_t *self, page_base_t *base)
{
    PM_LOG_INFO("Page(%s) state load", base->name);

    if (base->root != NULL)
    {
        PM_LOG_ERROR("Page(%s) root must be NULL", base->name);
    }

    // 创建根对象
    lv_obj_t *root_obj = lv_obj_create(lv_scr_act(), NULL);
    lv_obj_set_size(root_obj, LV_HOR_RES, LV_VER_RES);
    root_obj->user_data = base;
    base->root = root_obj;
    base->base->on_view_load(base);

    if (base->root_event_cb != NULL)
    {
        lv_obj_set_event_cb(root_obj, base->root_event_cb);
    }
    
    if (_get_is_over_anim(page_get_current_load_anim_type(self)))
    {
        page_base_t *bottom_page = get_stack_top_after(self);

        if (bottom_page != NULL && bottom_page->priv.is_cached)
        {
            page_load_anim_attr_t anim_attr;
            if (page_get_current_load_anim_attr(self, &anim_attr))
            {
                if (anim_attr.drag_dir != ROOT_DRAG_DIR_NONE)
                {
                    root_enable_drag(base->root);
                }
            }
        }
    }

    base->base->on_view_did_load(base);

    if (base->priv.is_disable_auto_cache)
    {
        PM_LOG_INFO("Page(%s) disable auto cache, ReqEnableCache = %d", base->name, base->priv.req_enable_cache);
        base->priv.is_cached = base->priv.req_enable_cache;
    }
    else
    {
        PM_LOG_INFO("Page(%s) AUTO cached", base->name);
        base->priv.is_cached = true;
    }

    return PAGE_STATE_WILL_APPEAR;
}

static page_state_t _state_will_appear_execute(page_manager_t *self, page_base_t *base)
{
    PM_LOG_INFO("Page(%s) state will appear", base->name);
    base->base->on_view_will_appear(base);
    switch_anim_create(self, base);
    return PAGE_STATE_DID_APPEAR;
}

static page_state_t _state_did_appear_execute(page_base_t *base)
{
    PM_LOG_INFO("Page(%s) state did appear", base->name);
    base->base->on_view_did_appear(base);
    return PAGE_STATE_ACTIVITY;
}

static page_state_t _state_will_disappear_execute(page_manager_t *self, page_base_t *base)
{
    PM_LOG_INFO("Page(%s) state will disappear", base->name);
    base->base->on_view_will_disappear(base);
    switch_anim_create(self, base);
    return PAGE_STATE_DID_DISAPPEAR;
}

static page_state_t _state_did_disappear_execute(page_manager_t *self, page_base_t *base)
{
    PM_LOG_INFO("Page(%s) state did disappear", base->name);
    if (page_get_current_load_anim_type(self) == LOAD_ANIM_FADE_ON)
    {
        PM_LOG_INFO("AnimState.TypeCurrent == LOAD_ANIM_FADE_ON, Page(%s) hidden", base->name);
    }
    base->base->on_view_did_disappear(base);
    if (base->priv.is_cached)
    {
        PM_LOG_INFO("Page(%s) has cached", base->name);
        return PAGE_STATE_WILL_APPEAR;
    }
    else
    {
        return PAGE_STATE_UNLOAD;
    }
}

page_state_t state_unload_execute(page_base_t* base)
{
    PM_LOG_INFO("Page(%s) state unload", base->name);
    if (base->root == NULL)
    {
        PM_LOG_WARN("Page is loaded!");
        goto Exit;
    }

    if (base->priv.stash.ptr != NULL && base->priv.stash.size != 0)
    {
        PM_LOG_INFO("Page(%s) free stash(0x%p)[%d]", base->name, base->priv.stash.ptr, base->priv.stash.size);
        PM_FREE(base->priv.stash.ptr);
        base->priv.stash.ptr = NULL;
        base->priv.stash.size = 0;
    }
    lv_obj_del_async(base->root);
    base->root = NULL;
    base->priv.is_cached = false;
    base->base->on_view_did_unload(base);

Exit:
    return PAGE_STATE_IDLE;
}

static bool _get_is_over_anim(uint8_t anim)
{
    return (anim >= LOAD_ANIM_OVER_LEFT && anim <= LOAD_ANIM_OVER_BOTTOM);
}