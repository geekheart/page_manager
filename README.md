# page_manager

base on X-TRACK [PageManager](https://github.com/FASTSHIFT/X-TRACK/tree/main/Software/X-Track/USER/App/Utils/PageManager)
Thanks to [FASTSHIFT](https://github.com/FASTSHIFT) for authorization

## introduce
page management framework base on lvgl 

## how to use
1. First you have to have a compileable lvgl project
2. Put the framework as a component in the project
3. Create and use page managers
```C
page_manager_t *manager = page_manager_create(); // create page manager
pm_install(manager, "demo", demo_create("demo")); // install your page
pm_set_global_load_anim_type(manager, LOAD_ANIM_OVER_TOP, 500, lv_anim_path_overshoot); // setting global anim type
pm_push(manager, "demo", NULL); // push to show installed pages
pm_pop(manager); // pop up the page that has already been shown
```

