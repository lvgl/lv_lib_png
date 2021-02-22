/*
 * Copyright (C) 2020 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "lvgl/lvgl.h"
#include "lv_apng.h"

LV_EVENT_CB_DECLARE(png_end_cb)
{
    if (e == LV_EVENT_LEAVE) {
        //lv_obj_del_async(obj);
        apng_restart(obj);
    }
}

void apng_test(void)
{
    lv_obj_t *apng = lv_apng_create_from_file(lv_scr_act(), "P:./apng/clock.png");
    lv_obj_align(apng,NULL,LV_ALIGN_CENTER,0,0);
    lv_obj_set_event_cb(apng,png_end_cb);
}

#if 0
int main(int argc, FAR char* argv[])
{
    /*LittlevGL init*/
    lv_init();

    lv_fs_interface_init();
    /*Linux frame buffer device init*/
    fbdev_init();

    /*A small buffer for LittlevGL to draw the screen's content*/
    static lv_color_t buf[DISP_BUF_SIZE];

    /*Initialize a descriptor for the buffer*/
    static lv_disp_buf_t disp_buf;
    lv_disp_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE);

    /*Initialize and register a display driver*/
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.buffer = &disp_buf;
    disp_drv.flush_cb = fbdev_flush;
    lv_disp_drv_register(&disp_drv);

    tp_init();

    /*Register a touchpad input device*/
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = tp_read;
    lv_indev_drv_register(&indev_drv);

    /* Start TP calibration */
#ifdef CONFIG_EXAMPLES_BANDXX_CALIBRATE
    tp_cal_create();
#else
    lv_point_t p[4];

    /* top left */
    p[0].x = 0;
    p[0].y = 0;

    /* top right */
    p[1].x = LV_HOR_RES;
    p[1].y = 0;

    /* bottom left */
    p[2].x = 0;
    p[2].y = LV_VER_RES;

    /* bottom right */
    p[3].x = LV_HOR_RES;
    p[3].y = LV_VER_RES;
    tp_set_cal_values(&p[0], &p[1], &p[2], &p[3]);
#endif

    apng_test();

    printf("run ok");fflush(stdout);
    /*Handle LitlevGL tasks (tickless mode)*/
    while (1) {
        lv_task_handler();
        usleep(5000);
    }

    return 0;
}
#endif
