#include "page_manager_private.h"

#define PM_EMPTY_PAGE_NAME "EMPTY_PAGE"

/**
 * @brief 创建页面管理器对象
 *
 * @return page_manager_t* 页面管理器对象
 */
page_manager_t *page_manager_create(void)
{
    page_manager_t *page_manager = (page_manager_t *)PM_MALLOC(sizeof(page_manager_t));
    if (page_manager == NULL)
    {
        PM_LOG_ERROR("page_manager alloc error\n");
        return NULL;
    }
    PM_LOG_INFO("page_manager alloc sucess\n");
    memset(page_manager, 0, sizeof(page_manager_t));
    page_manager->page_pool = listCreate();
    listSetFreeMethod(page_manager->page_pool, page_base_delete);
    page_manager->page_stack = listCreate();
    return page_manager;
}

/**
 * @brief 删除页面管理器对象
 *
 * @param self 页面管理器对象
 */
void page_manager_delete(page_manager_t *self)
{
    if (self == NULL)
    {
        PM_LOG_ERROR("page_manager is NULL\n");
        return;
    }
    listRelease(self->page_pool);
    listRelease(self->page_stack);
    self->page_current = NULL;
    self->page_prev = NULL;
    PM_FREE(self);
    PM_LOG_INFO("page_manager free sucess\n");
}

/**
 * @brief 通过名字在页面池中到到页面对象
 *
 * @param self 页面管理器对象
 * @param name 页面名称
 * @return page_base_t* 页面对象
 */
page_base_t *find_page_pool(page_manager_t *self, const char *name)
{
    listIter *iter = listGetIterator(self->page_pool, AL_START_HEAD);
    for (listNode *node = listNext(iter); node != NULL; node = listNext(iter))
    {
        if (strcmp(((page_base_t *)node->value)->name, name) == 0)
        {
            PM_LOG_INFO("find object %s addr[%p]\n", name, node);
            listReleaseIterator(iter);
            return (page_base_t *)node->value;
        }
    }
    listReleaseIterator(iter);
    return NULL;
}

/**
 * @brief 从栈中找到页面
 *
 * @param self 页面管理器对象
 * @param name 页面名称
 * @return page_base_t*
 */
page_base_t *find_page_stack(page_manager_t *self, const char *name)
{
    listIter *iter = listGetIterator(self->page_stack, AL_START_HEAD);
    for (listNode *node = listNext(iter); node != NULL; node = listNext(iter))
    {
        if (strcmp(((page_base_t *)node->value)->name, name) == 0)
        {
            PM_LOG_INFO("find object %s addr[%p]\n", name, node);
            listReleaseIterator(iter);
            return (page_base_t *)node->value;
        }
    }
    listReleaseIterator(iter);
    return NULL;
}

/**
 * @brief 向页面管理器中注册页面
 *
 * @param self 页面管理器对象
 * @param base 页面对象
 */
static void pm_register(page_manager_t *self, page_base_t *base)
{
    if (find_page_pool(self, base->name) != NULL)
    {
        PM_LOG_ERROR("Page(%s) was multi registered", base->name);
        return;
    }

    base->manager = self;
    listAddNodeTail(self->page_pool, base);
    PM_LOG_INFO("page(%s) manager register", base->name);
}

/**
 * @brief 向页面管理器中注销页面
 *
 * @param self 页面管理器对象
 * @param base 页面对象
 */
static void pm_unregister(page_manager_t *self, const char *name)
{
    PM_LOG_INFO("Page(%s) unregister...", name);

    page_base_t *base = find_page_stack(self, name);
    if (base != NULL)
    {
        PM_LOG_ERROR("Page(%s) was in recycle_pool", name);
        return;
    }

    if (find_page_pool(self, name) != NULL)
    {
        PM_LOG_ERROR("Page(%s) was multi registered", name);
    }

    listDelNode(self->page_pool, listSearchKey(self->page_pool, base));
    PM_LOG_INFO("Unregister OK");
}

/**
 * @brief 安装页面到页面管理器中
 *
 * @param self 页面管理器对象
 * @param page_param 页面调度函数
 */
void pm_install(page_manager_t *self, const char *name, page_vtable_t* page_param)
{
    page_base_t *page_base = page_base_create();

    page_base->base = page_param;
    page_base->name = name;
    page_base->manager = NULL;
    page_base->root = NULL;
    page_base->root_event_cb = NULL;
    page_base->user_data = NULL;
    memset(&page_base->priv, 0, sizeof(page_base->priv));

    page_base->base->on_custom_attr_config(page_base);

    pm_register(self, page_base);
}

/**
 * @brief 页面管理器中卸载页面
 *
 * @param self 页面管理器对象
 * @param name 页面名称
 */
void pm_uninstall(page_manager_t *self, const char *name)
{
    PM_LOG_INFO("Page(%s) uninstall...", name);
    page_base_t *base = find_page_pool(self, name);
    if (base == NULL)
    {
        PM_LOG_ERROR("Page(%s) was not found", name);
        return;
    }

    pm_unregister(self, name);

    if (base->priv.is_cached)
    {
        PM_LOG_WARN("Page(%s) has cached, unloading...", name);
        base->priv.state = PAGE_STATE_UNLOAD;
        page_state_update(self, base);
    }
    else
    {
        PM_LOG_INFO("Page(%s) has not cache", name);
    }

    PM_LOG_INFO("Uninstall OK");
}

/**
 * @brief 获取上一个页面的名字
 *
 * @param self 页面管理器对象
 * @return const char* 页面名称
 */
const char *get_page_prev_name(page_manager_t *self)
{
    if (self->page_prev != NULL)
    {
        return self->page_prev->name;
    }
    return PM_EMPTY_PAGE_NAME;
}

/**
 * @brief 获取栈顶页面
 *
 * @param self 页面管理器对象
 * @return page_base_t* 页面对象
 */
page_base_t *get_stack_top(page_manager_t *self)
{
    listNode *base_node = listIndex(self->page_stack, 0);
    if (base_node == NULL)
    {
        PM_LOG_ERROR("page_manage is empty");
        return NULL;
    }
    return (page_base_t *)base_node->value;
}

/**
 * @brief 获取栈顶页面后面的页面
 *
 * @param self 页面管理器对象
 * @return page_base_t* 页面对象
 */
page_base_t *get_stack_top_after(page_manager_t *self)
{
    listNode *base_node = listIndex(self->page_stack, 1);
    if (base_node == NULL)
    {
        PM_LOG_ERROR("page_manage is empty or only heve one page");
        return NULL;
    }
    return (page_base_t *)base_node->value;
}

/**
 * @brief 清除页面
 * 
 * @param self 页面管理器对象
 * @param keep_bottom 是否保留栈底页面
 */
void set_satck_clear(page_manager_t *self, bool keep_bottom)
{
    while (1)
    {
        page_base_t *top = get_stack_top(self);
        if (top == NULL)
        {
            PM_LOG_INFO("Page stack is empty, breaking...");
            break;
        }

        page_base_t *top_after = get_stack_top_after(self);

        if (top_after == NULL)
        {
            if (keep_bottom)
            {
                self->page_prev = top;
                PM_LOG_INFO("Keep page stack bottom(%s), breaking...", top->name);
                break;
            }
            else
            {
                self->page_prev = NULL;
            }
        }

        fource_unload(top);
        listDelNode(self->page_stack, listSearchKey(self->page_stack, top));
    }
    PM_LOG_INFO("Stack clear done");
}