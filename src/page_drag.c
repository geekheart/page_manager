#include "page_manager_private.h"
#include <math.h>

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) > (y) ? (y) : (x))
#define CONSTRAIN(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))

/* The distance threshold to trigger the drag */
#define PM_INDEV_DEF_DRAG_THROW 20

static void _on_root_async_leavel(void *data);
static void _on_root_anim_finish(lv_anim_t *a);

/**
 * @brief 页面拖动事件回调
 * 
 * @param obj lvgl对象
 * @param event 事件类型
 */
void page_root_drag_event(lv_obj_t * obj, lv_event_t event)
{
    page_base_t *base = (page_base_t *)lv_obj_get_user_data(obj);

    if (base == NULL)
    {
        PM_LOG_ERROR("Page base is NULL");
        return;
    }

    page_manager_t *manager = base->manager;
    page_load_anim_attr_t anim_attr;

    if (!page_get_current_load_anim_attr(manager, &anim_attr))
    {
        PM_LOG_ERROR("Can't get current anim attr");
        return;
    }
    if (base->root_event_cb != NULL)
    {
        base->root_event_cb(obj, event);
    }

    switch (event)
    {
    case LV_EVENT_PRESSED:
    {
        if (manager->anim_state.is_switch_req)
            return;
        if (!manager->anim_state.is_busy)
            return;

        PM_LOG_INFO("Root anim interrupted");
        lv_anim_del(obj, anim_attr.setter);
        manager->anim_state.is_busy = false;
    }
    break;
    case LV_EVENT_PRESSING:
    {
        lv_coord_t cur = anim_attr.getter(obj);

        lv_coord_t max = MAX(anim_attr.pop.exit.start, anim_attr.pop.exit.end);
        lv_coord_t min = MIN(anim_attr.pop.exit.start, anim_attr.pop.exit.end);

        lv_point_t offset;
        lv_indev_get_vect(lv_indev_get_act(), &offset);

        if (anim_attr.drag_dir == ROOT_DRAG_DIR_HOR)
        {
            cur += offset.x;
        }
        else if (anim_attr.drag_dir == ROOT_DRAG_DIR_VER)
        {
            cur += offset.y;
        }

        anim_attr.setter(obj, CONSTRAIN(cur, min, max));
    }
    break;
    case LV_EVENT_RELEASED:
    {
        if (manager->anim_state.is_switch_req)
        {
            return;
        }

        lv_coord_t offset_sum = anim_attr.push.enter.end - anim_attr.push.enter.start;

        lv_coord_t x_predict = 0;
        lv_coord_t y_predict = 0;
        root_get_drag_predict(&x_predict, &y_predict);

        lv_coord_t start = anim_attr.getter(obj);
        lv_coord_t end = start;

        if (anim_attr.drag_dir == ROOT_DRAG_DIR_HOR)
        {
            end += x_predict;
            PM_LOG_INFO("Root drag x_predict = %d", end);
        }
        else if (anim_attr.drag_dir == ROOT_DRAG_DIR_VER)
        {
            end += y_predict;
            PM_LOG_INFO("Root drag y_predict = %d", end);
        }

        if (abs(end) > abs((int)offset_sum) / 2)
        {
            lv_async_call(_on_root_async_leavel, base);
        }
        else if (end != anim_attr.push.enter.end)
        {
            manager->anim_state.is_busy = true;

            lv_anim_t a;
            anim_default_init(manager, &a);
            a.user_data = manager;
            lv_anim_set_var(&a, obj);
            lv_anim_set_values(&a, start, anim_attr.push.enter.end);
            lv_anim_set_exec_cb(&a, anim_attr.setter);
            lv_anim_set_ready_cb(&a, _on_root_anim_finish);
            lv_anim_start(&a);
            PM_LOG_INFO("Root anim start");
        }
    }
    break;

    default:
        break;
    }
}

/**
 * @brief 拖动动画结束事件回调
 * 
 * @param a 动画对象
 */
static void _on_root_anim_finish(lv_anim_t *a)
{
    page_manager_t *manager = (page_manager_t *)a->user_data;
    PM_LOG_INFO("Root anim finish");
    manager->anim_state.is_busy = false;
}

/**
 * @brief 开启root的拖拽功能
 * 
 * @param root 页面根对象
 */
void root_enable_drag(lv_obj_t *root)
{
    lv_obj_set_event_cb(root, page_root_drag_event); 
    PM_LOG_INFO("Root drag enabled");
}

/**
 * @brief 拖动结束时的异步回调
 * 
 * @param data 页面基本对象
 */
static void _on_root_async_leavel(void *data)
{
    page_base_t *base = (page_base_t *)data;
    PM_LOG_INFO("Page(%s) send event: LV_EVENT_LEAVE, need to handle...", base->name);
    lv_event_send(base->root, LV_EVENT_LEAVE, NULL);
}

/**
 * @brief 获取拖曳惯性预测停止点
 * 
 * @param x 
 * @param y 
 */
void root_get_drag_predict(lv_coord_t* x, lv_coord_t* y)
{
    lv_indev_t* indev = lv_indev_get_act();
    lv_point_t vect;
    lv_indev_get_vect(indev, &vect);

    lv_coord_t y_predict = 0;
    lv_coord_t x_predict = 0;

    while (vect.y != 0)
    {
        y_predict += vect.y;
        vect.y = vect.y * (100 - PM_INDEV_DEF_DRAG_THROW) / 100;
    }

    while (vect.x != 0)
    {
        x_predict += vect.x;
        vect.x = vect.x * (100 - PM_INDEV_DEF_DRAG_THROW) / 100;
    }

    *x = x_predict;
    *y = y_predict;
}
