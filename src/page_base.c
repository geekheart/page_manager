#include "page_base.h"

/**
 * @brief 创建基础页面
 *
 * @return page_base_t* 返回页面对象
 */
page_base_t *page_base_create(void)
{
    page_base_t *page_base = (page_base_t *)PM_MALLOC(sizeof(page_base_t));
    if (page_base == NULL)
    {
        PM_LOG_ERROR("page_base alloc error\n");
        return NULL;
    }
    return page_base;
}

/**
 * @brief 删除页面对象
 *
 * @param self 页面对象
 */
void page_base_delete(void *self)
{
    if (self == NULL)
    {
        PM_LOG_ERROR("page_base is NULL\n");
        return;
    }
    PM_FREE(self);
}

/**
 * @brief 设置自动缓存
 *
 * @param self 页面对象
 * @param en 是都开启自动缓存
 */
void page_set_custom_auto_cache_enable(page_base_t *self, bool en)
{
    self->priv.req_disable_auto_cache = !en;
}

/**
 * @brief 手动设置缓存是否开启
 *
 * @param self 页面对象
 * @param en 开启或者关闭缓存
 */
void page_set_custom_cache_enable(page_base_t *self, bool en)
{
    page_set_custom_auto_cache_enable(self, false);
    self->priv.req_enable_cache = en;
}

/**
 * @brief 设置用户加载动画的参数
 *
 * @param self 页面对象
 * @param anim_type 动画类型
 * @param time 动画持续时间
 * @param path 动画路径
 */
void page_set_custom_load_anim_type(page_base_t *self, uint8_t anim_type, uint16_t time, lv_anim_path_cb_t path)
{
    self->priv.anim.attr.type = anim_type;
    self->priv.anim.attr.time = time;
    self->priv.anim.attr.path = path;
}

/**
 * @brief 设置用户根对象事件回调函数
 *
 * @param self 页面对象
 * @param root_event_cb 根对象回调函数
 */
void page_set_custom_root_event_cb(page_base_t *self, lv_event_cb_t root_event_cb)
{
    self->root_event_cb = root_event_cb;
}

/**
 * @brief 获取缓存区里的数据
 *
 * @param self 页面对象
 * @param ptr 缓存区指针
 * @param size 数据长度
 * @return true 成功获取数据
 * @return false 获取数据失败
 */
bool page_get_stash(page_base_t *self, void *ptr, uint32_t size)
{
    bool retval = false;
    if (self->priv.stash.ptr != NULL && self->priv.stash.size == size)
    {
        memcpy(ptr, self->priv.stash.ptr, self->priv.stash.size);
        retval = true;
    }
    return retval;
}