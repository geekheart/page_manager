#pragma once

#include "page_config.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct page_base_t page_base_t;
    typedef struct page_manager_t page_manager_t;

    typedef enum
    {
        PAGE_STATE_IDLE,
        PAGE_STATE_LOAD,
        PAGE_STATE_WILL_APPEAR,
        PAGE_STATE_DID_APPEAR,
        PAGE_STATE_ACTIVITY,
        PAGE_STATE_WILL_DISAPPEAR,
        PAGE_STATE_DID_DISAPPEAR,
        PAGE_STATE_UNLOAD,
        _PAGE_STATE_LAST
    } page_state_t;

    // 数据块
    typedef struct
    {
        void *ptr;
        uint32_t size;
    } page_stash_t;

    // 页面切换动画属性
    typedef struct
    {
        uint8_t type;
        uint16_t time;
        lv_anim_path_cb_t path;
    } page_anim_attr_t;

    typedef struct
    {
        /**
         * @brief 同步用户自定义属性配置
         *  @note 在install阶段被调用，用于打开一些页面设置,此时root页面未创建
         */
        void (*on_custom_attr_config)(page_base_t *self);

        /**
         * @brief 页面加载
         *  @note 在push/pop后创建完root页面后调用,is_cache是true时跳过
         */
        void (*on_view_load)(page_base_t *self);

        /**
         * @brief 页面加载完成
         *  @note 比on_view_load多初始化了拖拽功能（根据动画参数自动开启）后调用,is_cache是true时跳过
         */
        void (*on_view_did_load)(page_base_t *self);

        /**
         * @brief 页面即将显示
         *  @note 比on_view_load多了更新is_cache相关的内容
         */
        void (*on_view_will_appear)(page_base_t *self);

        /**
         * @brief 页面显示
         *  @note 动画结束后调用
         */
        void (*on_view_did_appear)(page_base_t *self);

        /**
         * @brief 页面即将消失
         *  @note 页面被切换时第一时间调用
         */
        void (*on_view_will_disappear)(page_base_t *self);

        /**
         * @brief 页面消失完成
         *  @note 页面消失时调用
         */
        void (*on_view_did_disappear)(page_base_t *self);

        /**
         * @brief 页面卸载完成
         *  @note 页面被卸载的时候会被调用,is_cache是true时跳过
         */
        void (*on_view_did_unload)(page_base_t *self);
    } page_vtable_t;

    typedef struct page_base_t
    {
        page_vtable_t* base;
        lv_obj_t *root;
        lv_event_cb_t root_event_cb; // 根对象回调
        page_manager_t *manager;
        const char *name;
        void *user_data;
        struct
        {
            bool req_enable_cache;       // 页面缓存启用标志位
            bool req_disable_auto_cache; // 页面自动缓存管理启用标志位
            bool is_disable_auto_cache;  // 页面自动缓存标志位
            bool is_cached;              // 页面缓存标志位
            page_stash_t stash;          // push时传入参数
            page_state_t state;          // 页面状态
            /* 动画状态  */
            struct
            {
                bool is_enter;         // 进入还是退出动画
                bool is_busy;          // 动画是否正在播放
                page_anim_attr_t attr; // lvgl动画属性
            } anim;
        } priv;
    } page_base_t;

    /**
     * @brief 设置自动缓存
     *
     * @param self 页面对象
     * @param en 是都开启自动缓存
     */
    void page_set_custom_auto_cache_enable(page_base_t *self, bool en);

    /**
     * @brief 手动设置缓存是否开启
     *
     * @param self 页面对象
     * @param en 开启或者关闭缓存
     */
    void page_set_custom_cache_enable(page_base_t *self, bool en);

    /**
     * @brief 设置用户加载动画的参数
     *
     * @param self 页面对象
     * @param anim_type 动画类型
     * @param time 动画持续时间
     * @param path 动画路径
     */
    void page_set_custom_load_anim_type(page_base_t *self, uint8_t anim_type, uint16_t time, lv_anim_path_cb_t path);

    /**
     * @brief 设置用户根对象事件回调函数
     *
     * @param self 页面对象
     * @param root_event_cb 根对象回调函数
     */
    void page_set_custom_root_event_cb(page_base_t *self, lv_event_cb_t root_event_cb);

    /**
     * @brief 获取缓存区里的数据,这里的缓存区是页面push的时候存放自己的数据
     *
     * @param self 页面对象
     * @param ptr 缓存区指针
     * @param size 数据长度
     * @return true 成功获取数据
     * @return false 获取数据失败
     */
    bool page_get_stash(page_base_t *self, void *ptr, uint32_t size);

#ifdef __cplusplus
} /* extern "C" */
#endif