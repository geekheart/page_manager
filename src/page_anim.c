#include "page_manager_private.h"

/**
  * @brief  Set the transparency of obj
  * @param  obj: Pointer to obj
  * @param  opa_scale: Transparency value
  * @retval None
  */
static void _lv_obj_set_opa_scale(lv_obj_t* obj, int32_t opa_scale)
{
    lv_obj_set_style_bg_opa(obj, (lv_opa_t)opa_scale, LV_PART_MAIN);
}

/**
  * @brief  Get obj transparency
  * @param  obj: Pointer to obj
  * @retval Transparency value
  */
static int32_t _lv_obj_get_opa_scale(lv_obj_t* obj)
{
    return lv_obj_get_style_bg_opa(obj, LV_PART_MAIN);
}

/**
 * @brief 获取加载动画的参数
 *
 * @param anim 动画路径
 * @param attr [out]动画属性
 * @return true 获取成功
 * @return false 获取失败
 */
static bool _get_load_anim_attr(uint8_t anim, page_load_anim_attr_t *attr)
{
    lv_coord_t hor = LV_HOR_RES;
    lv_coord_t ver = LV_VER_RES;

    switch (anim)
    {
    case LOAD_ANIM_OVER_LEFT:
        attr->drag_dir = ROOT_DRAG_DIR_HOR;

        attr->push.enter.start = hor;
        attr->push.enter.end = 0;
        attr->push.exit.start = 0;
        attr->push.exit.end = 0;

        attr->pop.enter.start = 0;
        attr->pop.enter.end = 0;
        attr->pop.exit.start = 0;
        attr->pop.exit.end = hor;
        break;

    case LOAD_ANIM_OVER_RIGHT:
        attr->drag_dir = ROOT_DRAG_DIR_HOR;

        attr->push.enter.start = -hor;
        attr->push.enter.end = 0;
        attr->push.exit.start = 0;
        attr->push.exit.end = 0;

        attr->pop.enter.start = 0;
        attr->pop.enter.end = 0;
        attr->pop.exit.start = 0;
        attr->pop.exit.end = -hor;
        break;

    case LOAD_ANIM_OVER_TOP:
        attr->drag_dir = ROOT_DRAG_DIR_VER;

        attr->push.enter.start = ver;
        attr->push.enter.end = 0;
        attr->push.exit.start = 0;
        attr->push.exit.end = 0;

        attr->pop.enter.start = 0;
        attr->pop.enter.end = 0;
        attr->pop.exit.start = 0;
        attr->pop.exit.end = ver;
        break;

    case LOAD_ANIM_OVER_BOTTOM:
        attr->drag_dir = ROOT_DRAG_DIR_VER;

        attr->push.enter.start = -ver;
        attr->push.enter.end = 0;
        attr->push.exit.start = 0;
        attr->push.exit.end = 0;

        attr->pop.enter.start = 0;
        attr->pop.enter.end = 0;
        attr->pop.exit.start = 0;
        attr->pop.exit.end = -ver;
        break;

    case LOAD_ANIM_MOVE_LEFT:
        attr->drag_dir = ROOT_DRAG_DIR_HOR;

        attr->push.enter.start = hor;
        attr->push.enter.end = 0;
        attr->push.exit.start = 0;
        attr->push.exit.end = -hor;

        attr->pop.enter.start = -hor;
        attr->pop.enter.end = 0;
        attr->pop.exit.start = 0;
        attr->pop.exit.end = hor;
        break;

    case LOAD_ANIM_MOVE_RIGHT:
        attr->drag_dir = ROOT_DRAG_DIR_HOR;

        attr->push.enter.start = -hor;
        attr->push.enter.end = 0;
        attr->push.exit.start = 0;
        attr->push.exit.end = hor;

        attr->pop.enter.start = hor;
        attr->pop.enter.end = 0;
        attr->pop.exit.start = 0;
        attr->pop.exit.end = -hor;
        break;

    case LOAD_ANIM_MOVE_TOP:
        attr->drag_dir = ROOT_DRAG_DIR_VER;

        attr->push.enter.start = ver;
        attr->push.enter.end = 0;
        attr->push.exit.start = 0;
        attr->push.exit.end = -ver;

        attr->pop.enter.start = -ver;
        attr->pop.enter.end = 0;
        attr->pop.exit.start = 0;
        attr->pop.exit.end = ver;
        break;

    case LOAD_ANIM_MOVE_BOTTOM:
        attr->drag_dir = ROOT_DRAG_DIR_VER;

        attr->push.enter.start = -ver;
        attr->push.enter.end = 0;
        attr->push.exit.start = 0;
        attr->push.exit.end = ver;

        attr->pop.enter.start = ver;
        attr->pop.enter.end = 0;
        attr->pop.exit.start = 0;
        attr->pop.exit.end = -ver;
        break;

    case LOAD_ANIM_FADE_ON:
        attr->drag_dir = ROOT_DRAG_DIR_NONE;

        attr->push.enter.start = LV_OPA_TRANSP;
        attr->push.enter.end = LV_OPA_COVER;
        attr->push.exit.start = LV_OPA_COVER;
        attr->push.exit.end = LV_OPA_COVER;

        attr->pop.enter.start = LV_OPA_COVER;
        attr->pop.enter.end = LV_OPA_COVER;
        attr->pop.exit.start = LV_OPA_COVER;
        attr->pop.exit.end = LV_OPA_TRANSP;
        break;

    case LOAD_ANIM_NONE:
        memset(attr, 0, sizeof(page_load_anim_attr_t));
        return true;

    default:
        PM_LOG_ERROR("Load anim type error: %d", anim);
        return false;
    }

    /* 确定动画的setter和getter*/
    if (attr->drag_dir == ROOT_DRAG_DIR_HOR)
    {
        attr->setter = (lv_anim_setter_t)lv_obj_set_x;
        attr->getter = (lv_anim_getter_t)lv_obj_get_x;
    }
    else if (attr->drag_dir == ROOT_DRAG_DIR_VER)
    {
        attr->setter = (lv_anim_setter_t)lv_obj_set_y;
        attr->getter = (lv_anim_getter_t)lv_obj_get_y;
    }
    else
    {
        attr->setter = (lv_anim_setter_t)_lv_obj_set_opa_scale;
        attr->getter = (lv_anim_getter_t)_lv_obj_get_opa_scale;
    }

    return true;
}

/**
 * @brief 获取当前页面的用户动画参数
 *
 * @param self 页面管理器对象
 * @return page_load_anim_t 用户动画参数
 */
page_load_anim_t page_get_current_load_anim_type(page_manager_t *self)
{
    return (page_load_anim_t)self->anim_state.current.type;
}

/**
 * @brief 获取当前页面动画参数
 *
 * @param self 页面管理器对象
 * @param attr [out]页面动画参数
 * @return true 获取成功
 * @return false 获取失败
 */
bool page_get_current_load_anim_attr(page_manager_t *self, page_load_anim_attr_t *attr)
{
    return _get_load_anim_attr(page_get_current_load_anim_type(self), attr);
}
