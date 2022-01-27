#include "page_manager_private.h"

static bool _switch_anim_state_check(page_manager_t *self);
static void _switch_anim_type_update(page_manager_t *self, page_base_t *base);
static void _page_switch(page_manager_t *self, page_base_t *new_node, bool is_push_act, const page_stash_t *stash);

/**
 * @brief 推送已安装的页面显示
 * 
 * @param self 页面管理器
 * @param name 页面名
 * @param stash push时用户的自定义参数
 */
void pm_push(page_manager_t *self, const char *name, const page_stash_t *stash)
{
    // 检查是否正在执行切换页面的动画
    if (!_switch_anim_state_check(self))
    {
        PM_LOG_WARN("Page stack anim, cat't pop");
        return;
    }

    // 检测是否处于栈区
    if (find_page_stack(self, name) != NULL)
    {
        PM_LOG_ERROR("Page(%s) was multi push", name);
        return;
    }

    // 检测页面是否在页面池中被注册
    page_base_t *base = find_page_pool(self, name);
    if (base == NULL)
    {
        PM_LOG_ERROR("Page(%s) was not install", name);
        return;
    }

    /* 同步自动缓存配置*/
    base->priv.is_disable_auto_cache = base->priv.req_disable_auto_cache;

    /* 页面压栈 */
    listAddNodeHead(self->page_stack, base);

    /* 切换页面 */
    _page_switch(self, base, true, stash);
}

/**
 * @brief 回退到上一个页面
 *
 * @param self 页面管理器对象
 */
void pm_pop(page_manager_t *self)
{
    // 检查是否正在执行切换页面的动画
    if (!_switch_anim_state_check(self))
    {
        PM_LOG_WARN("Page stack anim, cat't pop");
        return;
    }

    /* 获取栈顶页面 */
    page_base_t *top = get_stack_top(self);

    if (top == NULL)
    {
        PM_LOG_WARN("Page stack is empty, cat't pop");
        return;
    }

    if (!top->priv.is_disable_auto_cache)
    {
        PM_LOG_INFO("Page(%s) has auto cache, cache disabled", top->name);
        top->priv.is_cached = false;
    }

    PM_LOG_INFO("Page(%s) pop << [Screen]", top->name);

    // 页面出栈
    listDelNode(self->page_stack, listSearchKey(self->page_stack, top));

    top = get_stack_top(self);

    if (top != NULL)
    {
        /* 切换页面 */
        _page_switch(self, top, false, NULL);
    }
    else
    {
        PM_LOG_WARN("Page stack is empty, cat't pop");
    }
}

/**
 * @brief 切换页面
 *
 * @param self 页面管理器对象
 * @param new_node 新的页面
 * @param is_push_act 动画状态
 * @param stash 缓存区
 */
static void _page_switch(page_manager_t *self, page_base_t *new_node, bool is_push_act, const page_stash_t *stash)
{
    if (new_node == NULL)
    {
        PM_LOG_ERROR("newNode is nullptr");
        return;
    }

    if (self->anim_state.is_switch_req) // 确定没有页面在切换
    {
        PM_LOG_WARN("Page switch busy, reqire(%s) is ignore", new_node->name);
        return;
    }

    self->anim_state.is_switch_req = true; // 请求切换页面

    if (stash != NULL) // 如果有缓存区
    {
        PM_LOG_INFO("stash is detect, %s >> stash(%p) >> %s", get_page_prev_name(self), stash, new_node->name);

        void *buffer = NULL;

        //如果缓存区是空则申请内存
        if (new_node->priv.stash.ptr == NULL)
        {
            buffer = PM_MALLOC(stash->size);
            if (buffer == NULL)
            {
                PM_LOG_ERROR("stash malloc failed");
            }
            else
            {
                PM_LOG_INFO("stash(%p) malloc[%d]", buffer, stash->size);
            }
        }
        // 如果缓存区大小和现在大小一致。则获取内存地址（为下文营造非空判断）
        else if (new_node->priv.stash.size == stash->size)
        {
            buffer = new_node->priv.stash.ptr;
            PM_LOG_INFO("stash(%p) is exist", buffer);
        }

        // 将当前缓存的地址内容复制到缓存
        if (buffer != NULL)
        {
            memcpy(buffer, stash->ptr, stash->size);
            PM_LOG_INFO("stash memcpy[%d] %p >> %p", stash->size, stash->ptr, buffer);
            new_node->priv.stash.ptr = buffer;
            new_node->priv.stash.size = stash->size;
        }
    }

    // 当前页面更新
    self->page_current = new_node;

    // 如果页面有被缓存则跳过PAGE_STATE_LOAD
    if (self->page_current->priv.is_cached)
    {
        PM_LOG_INFO("Page(%s) has cached, appear driectly", self->page_current->name);
        self->page_current->priv.state = PAGE_STATE_WILL_APPEAR;
    }
    else
    {
        self->page_current->priv.state = PAGE_STATE_LOAD;
    }

    // 如果上一个页面存在则将is_enter标志位置0
    if (self->page_prev != NULL)
    {
        self->page_prev->priv.anim.is_enter = false;
    }

    // 把当前的设置为进入
    self->page_current->priv.anim.is_enter = true;
    // 设置动画为推送状态
    self->anim_state.is_pushing = is_push_act;

    // 如果动画标志位为进入
    if (self->anim_state.is_pushing)
    {
        // 根据当前页面更新动画配置
        _switch_anim_type_update(self, self->page_current);
    }

    // 更新页面
    page_state_update(self, self->page_prev);
    page_state_update(self, self->page_current);

    // 改变页面前后关系
    if (self->anim_state.is_pushing)
    {
        PM_LOG_INFO("Page PUSH is detect, move Page(%s) to foreground", self->page_current->name);
        if (self->page_prev)
            lv_obj_move_foreground(self->page_prev->root);
        lv_obj_move_foreground(self->page_current->root);
    }
    else
    {
        PM_LOG_INFO("Page POP is detect, move Page(%s) to foreground", get_page_prev_name(self));
        lv_obj_move_foreground(self->page_current->root);
        if (self->page_prev)
            lv_obj_move_foreground(self->page_prev->root);
    }
}

/**
 * @brief 强制卸载当前页面
 *
 * @param base 页面管理器
 * @return true
 * @return false
 */
bool fource_unload(page_base_t *base)
{
    if (base == NULL)
    {
        PM_LOG_ERROR("Page is nullptr, Unload failed");
        return false;
    }

    PM_LOG_INFO("Page(%s) Fource unloading...", base->name);

    if (base->priv.state == PAGE_STATE_ACTIVITY)
    {
        PM_LOG_INFO("Page state is ACTIVITY, Disappearing...");
        base->base->on_view_will_disappear(base);
        base->base->on_view_did_disappear(base);
    }

    base->priv.state = state_unload_execute(base);

    return true;
}

/**
 * @brief 返回主界面
 *
 * @param self 页面管理器对象
 * @return true 返回成功
 * @return false 页面正在切换
 */
bool pm_back_home(page_manager_t *self)
{
    // 检查是否正在执行切换页面的动画
    if (!_switch_anim_state_check(self))
    {
        return false;
    }
    set_satck_clear(self, true);
    self->page_prev = NULL;
    page_base_t *home = get_stack_top(self);
    _page_switch(self, home, false, NULL);
    return true;
}

/**
 * @brief 检测界面是否在切换
 *
 * @param self 页面管理器对象
 * @return true 页面正在切换
 * @return false 页面不在切换
 */
static bool _switch_anim_state_check(page_manager_t *self)
{
    if (self->anim_state.is_switch_req || self->anim_state.is_busy)
    {
        PM_LOG_WARN(
            "Page switch busy[self->anim_state.IsSwitchReq = %d,"
            "self->anim_state.IsBusy = %d],"
            "request ignored",
            self->anim_state.is_switch_req,
            self->anim_state.is_busy);
        return false;
    }
    return true;
}

/**
 * @brief 检测切换动画是否完成
 *
 * @param self 页面管理器对象
 * @return true 动画已完成
 * @return false 动画未完成
 */
static bool _switch_req_check(page_manager_t *self)
{
    bool ret = false;
    bool last_node_busy = self->page_prev && self->page_prev->priv.anim.is_busy;

    if (!self->page_current->priv.anim.is_busy && !last_node_busy)
    {
        PM_LOG_INFO("----Page switch was all finished----");
        self->anim_state.is_switch_req = false;
        ret = true;
        self->page_prev = self->page_current;
    }
    else
    {
        if (self->page_current->priv.anim.is_busy)
        {
            PM_LOG_WARN("Page PageCurrent(%s) is busy", self->page_current->name);
        }
        else
        {
            PM_LOG_WARN("Page PagePrev(%s) is busy", get_page_prev_name(self));
        }
    }
    return ret;
}

/**
 * @brief 动画完成后更新页面状态
 *
 * @param a lvgl动画对象
 */
static void _switch_anim_finsh(lv_anim_t *a)
{
    page_base_t *base = (page_base_t *)a->user_data;
    page_manager_t *manager = base->manager;

    PM_LOG_INFO("Page(%s) Anim finish", base->name);

    page_state_update(manager, base);
    base->priv.anim.is_busy = false;
    bool is_finished = _switch_req_check(manager);

    if (!manager->anim_state.is_pushing && is_finished)
    {
        _switch_anim_type_update(manager, manager->page_current);
    }
}

/**
 * @brief 创建切换动画
 *
 * @param self 页面管理器对象
 * @param base 页面对象
 */
void switch_anim_create(page_manager_t *self, page_base_t *base)
{
    page_load_anim_attr_t anim_attr;
    // 获取当前页面参数
    if (!page_get_current_load_anim_attr(self, &anim_attr))
    {
        return;
    }
    PM_LOG_INFO("page anim create");
    lv_anim_t a;
    anim_default_init(self, &a);

    a.user_data = base;
    lv_anim_set_var(&a, base->root);
    lv_anim_set_ready_cb(&a, _switch_anim_finsh);
    lv_anim_set_exec_cb(&a, anim_attr.setter);

    int32_t start = 0;

    if (anim_attr.getter)
    {
        start = anim_attr.getter(base->root);
    }

    // 根据标志位更新动画
    if (self->anim_state.is_pushing)
    {
        if (base->priv.anim.is_enter)
        {
            lv_anim_set_values(
                &a,
                anim_attr.push.enter.start,
                anim_attr.push.enter.end);
        }
        else /* Exit */
        {
            lv_anim_set_values(
                &a,
                start,
                anim_attr.push.exit.end);
        }
    }
    else /* Pop */
    {
        if (base->priv.anim.is_enter)
        {
            lv_anim_set_values(
                &a,
                anim_attr.pop.enter.start,
                anim_attr.pop.enter.end);
        }
        else /* Exit */
        {
            lv_anim_set_values(
                &a,
                start,
                anim_attr.pop.exit.end);
        }
    }

    lv_anim_start(&a);
    base->priv.anim.is_busy = true;
}

/**
 * @brief 设置页面管理器全局默认动画参数
 *
 * @param self 页面管理器对象
 * @param anim 动画类型
 * @param time 动画持续时间
 * @param path 动画路径
 */
void pm_set_global_load_anim_type(page_manager_t *self, page_load_anim_t anim, uint16_t time, lv_anim_path_cb_t path)
{
    if (anim > _LOAD_ANIM_LAST)
    {
        anim = LOAD_ANIM_NONE;
    }

    self->anim_state.global.type = anim;
    self->anim_state.global.time = time;
    self->anim_state.global.path = path;

    PM_LOG_INFO("Set global load anim type = %d", anim);
}

/**
 * @brief 
 * 
 * @param self 
 * @param base 
 */
void _switch_anim_type_update(page_manager_t *self, page_base_t *base)
{
    if (base->priv.anim.attr.type == LOAD_ANIM_GLOBAL)
    {
        PM_LOG_INFO(
            "Page(%s) Anim.Type was not set, use self->anim_state.Global.Type = %d",
            base->name,
            self->anim_state.global.type);
        self->anim_state.current = self->anim_state.global;
    }
    else
    {
        if (base->priv.anim.attr.type > _LOAD_ANIM_LAST)
        {
            PM_LOG_ERROR("Page(%s)", base->name);
            PM_LOG_ERROR("ERROR custom Anim.Type = %d", base->priv.anim.attr.type);
            PM_LOG_ERROR("use self->anim_state.Global.Type = %d", self->anim_state.global.type);
            base->priv.anim.attr = self->anim_state.global;
        }
        else
        {
            PM_LOG_INFO(
                "Page(%s) custom Anim.Type set = %d",
                base->name,
                base->priv.anim.attr.type);
        }
        self->anim_state.current = base->priv.anim.attr;
    }
}

/**
 * @brief 默认动画初始化
 * 
 * @param self 页面管理器对象
 * @param a lvgl动画对象
 */
void anim_default_init(page_manager_t *self, lv_anim_t *a)
{
    lv_anim_init(a);

    uint32_t time = (page_get_current_load_anim_type(self) == LOAD_ANIM_NONE) ? 0 : self->anim_state.current.time;
    lv_anim_set_time(a, time);

    lv_anim_path_t path;
    lv_anim_path_init(&path);
    lv_anim_path_set_cb(&path, self->anim_state.current.path);
    PM_LOG_INFO("(%s) current path is (%p)", self->page_current->name, self->anim_state.current.path);
    
    lv_anim_set_path(a, &path);
}