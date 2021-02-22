/**
 * @file lv_apng.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_apng.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

/*********************
*      DEFINES
*********************/
#define LV_OBJX_NAME "lv_apng"

/**********************
*      TYPEDEFS
**********************/
typedef struct {
    png_structp png_ptr;
    png_infop  info_ptr;
    uint8_t *render_buf;
    uint8_t *base_buf;
    uint8_t *pcur_base;
    png_bytepp   prows;
    uint32_t      width;
    uint32_t     height;
    uint16_t total_frames;
    uint16_t current_frame;
    uint16_t framerate;
    uint8_t first_hidden;
    lv_fs_file_t fd;
} apng_info_t;

typedef struct {
    lv_img_ext_t img_ext;
    apng_info_t *apng;
    lv_task_t * task;
    lv_img_dsc_t imgdsc;
    uint32_t last_call;
}lv_apng_ext_t;

/**********************
*  STATIC PROTOTYPES
**********************/
static void png_read_callback(png_structp png_ptr, png_bytep data, size_t length);
static png_voidp png_malloc_callback(png_structp png_ptr,png_alloc_size_t size);
static void png_free_callback(png_structp png_ptr, png_voidp ptr);
static lv_res_t png_close(apng_info_t *papng);
static apng_info_t *load_apng_from_file(const char *path);

static void next_frame_task_cb(lv_task_t * t);
static lv_res_t lv_apng_signal(lv_obj_t * img, lv_signal_t sign, void * param);
/**********************
*  STATIC VARIABLES
**********************/
static lv_signal_cb_t ancestor_signal;

/**********************
*      MACROS
**********************/

/**********************
*   GLOBAL FUNCTIONS
**********************/
lv_res_t apng_restart(lv_obj_t * img)
{
    lv_apng_ext_t * ext = lv_obj_get_ext_attr(img);
    apng_info_t *papng = ext->apng;

    png_destroy_read_struct(&ext->apng->png_ptr, &ext->apng->info_ptr, NULL);

    png_structp png_ptr = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, 
        NULL, NULL, NULL, NULL, png_malloc_callback, png_free_callback);
    if(png_ptr == NULL){
        return LV_RES_INV;
    }
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if(png_ptr == NULL){
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return LV_RES_INV;
    }

    png_set_read_fn(png_ptr, &papng->fd, png_read_callback);

    png_set_sig_bytes(png_ptr, 0);
    png_read_info(png_ptr, info_ptr);

    png_set_expand(png_ptr);
    png_set_strip_16(png_ptr);
    png_set_gray_to_rgb(png_ptr);
    png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_AFTER);
    png_set_bgr(png_ptr);
    png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type;
    png_get_IHDR(png_ptr, info_ptr, &width, &height, 
        &bit_depth, &color_type, &interlace_type, NULL, NULL);
    
    papng->info_ptr= info_ptr;
    papng->png_ptr = png_ptr;
    papng->width   = width;
    papng->height  = height;
    papng->framerate     = 0;
    papng->current_frame = 0;

    lv_task_set_prio(ext->task, LV_TASK_PRIO_HIGH);

    return LV_RES_OK;
}

lv_obj_t * lv_apng_create_from_file(lv_obj_t * parent, const char * path)
{
    lv_obj_t *img = lv_img_create(parent, NULL);
    lv_apng_ext_t *ext = lv_obj_allocate_ext_attr(img, sizeof(lv_apng_ext_t));
    LV_ASSERT_MEM(ext);

    if(ancestor_signal == NULL) ancestor_signal = lv_obj_get_signal_cb(img);
    lv_obj_set_signal_cb(img, lv_apng_signal);

    ext->apng = load_apng_from_file(path);
    if (ext->apng == NULL) return img;

   ext->imgdsc.data = ext->apng->render_buf;
   ext->imgdsc.header.always_zero = 0;
   ext->imgdsc.header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
   ext->imgdsc.header.h = ext->apng->height;
   ext->imgdsc.header.w = ext->apng->width;
   ext->last_call = lv_tick_get();

   lv_img_set_src(img, &ext->imgdsc);

   ext->task = lv_task_create(next_frame_task_cb, 10, LV_TASK_PRIO_HIGH, img);
   next_frame_task_cb(ext->task);    /*Immediately process the first frame*/

   return img;
}

/**********************
*   STATIC FUNCTIONS
**********************/
static void png_read_callback(png_structp png_ptr, png_bytep data, size_t length)
{
    void *file_p = png_get_io_ptr(png_ptr);
    lv_fs_read(file_p, data, length, NULL);
}

static png_voidp png_malloc_callback(png_structp png_ptr,png_alloc_size_t size)
{
    LV_UNUSED(png_ptr);
    return lv_mem_alloc(size);
}

static void png_free_callback(png_structp png_ptr, png_voidp ptr)
{
    LV_UNUSED(png_ptr);

    if (ptr == NULL){
        return;
    }
    lv_mem_free(ptr);
}

static lv_res_t png_close(apng_info_t *papng)
{
    png_structp png_ptr = papng->png_ptr;
    png_infop info_ptr = papng->info_ptr;

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    lv_mem_free(papng->render_buf);
    lv_mem_free(papng->prows);
    lv_fs_close(&papng->fd);
    lv_mem_free(papng);

    return LV_RES_OK;
}

static apng_info_t *load_apng_from_file(const char *path)
{
    png_structp png_ptr = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, 
        NULL, NULL, NULL, NULL, png_malloc_callback, png_free_callback);
    if(png_ptr == NULL){
        return NULL;
    }
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if(png_ptr == NULL){
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return NULL;
    }

#if 0
    if(setjmp(png_jmpbuf(png_ptr))){
        LV_LOG_WARN("png_jmpbuf error.");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return NULL;
    }
#endif

    apng_info_t *papng = lv_mem_alloc(sizeof(apng_info_t));
    if(papng == NULL){
        LV_LOG_WARN("papng mem_alloc error");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return NULL;
    }

    lv_fs_res_t res = lv_fs_open(&papng->fd, path, LV_FS_MODE_RD);
    if(res != LV_FS_RES_OK){
        LV_LOG_WARN("open %s error.",path);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        lv_mem_free(papng);
        return NULL;
    }
    png_set_read_fn(png_ptr, &papng->fd, png_read_callback);

    png_set_sig_bytes(png_ptr, 0);
    png_read_info(png_ptr, info_ptr);
    if (!png_get_valid(png_ptr, info_ptr, PNG_INFO_acTL)){
        LV_LOG_WARN("Not a apng image.");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        lv_mem_free(papng);
        lv_fs_close(&papng->fd);
        return NULL;
    }

    uint32_t frame_count, num_plays;
    png_get_acTL(png_ptr, info_ptr, &frame_count, &num_plays);
    png_set_expand(png_ptr);
    png_set_strip_16(png_ptr);
    png_set_gray_to_rgb(png_ptr);
    png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_AFTER);
    png_set_bgr(png_ptr);
    png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type;
    png_get_IHDR(png_ptr, info_ptr, &width, &height, 
        &bit_depth, &color_type, &interlace_type, NULL, NULL);
    
    int m_nRowSize = png_get_rowbytes(png_ptr, info_ptr);
    uint8_t *render_buff = lv_mem_alloc(height * m_nRowSize * 2);
    if(render_buff == NULL){
        LV_LOG_WARN("render_buff mem_alloc error");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        lv_mem_free(papng);
        lv_fs_close(&papng->fd);
        return NULL;
    }
    png_bytepp prows = (png_bytepp)lv_mem_alloc(height * sizeof(png_bytep));
    if(prows == NULL){
        LV_LOG_WARN("prows mem_alloc error");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        lv_mem_free(render_buff);
        lv_mem_free(papng);
        lv_fs_close(&papng->fd);
        return NULL;
    }

    uint8_t first_hidden = png_get_first_frame_is_hidden(png_ptr, info_ptr) > 0 ? 1 : 0;

    papng->info_ptr= info_ptr;
    papng->png_ptr = png_ptr;
    papng->width   = width;
    papng->height  = height;
    papng->framerate     = 0;
    papng->current_frame = 0;
    papng->total_frames  = frame_count;
    papng->prows = prows;
    papng->render_buf = render_buff;
    papng->base_buf   = render_buff + height * m_nRowSize;
    papng->pcur_base  = render_buff;
    papng->first_hidden = first_hidden;

    LV_LOG_TRACE("tatal_frames:%d,width:%d,height:%d,color_type:%d,depth:%d", frame_count,width,height,color_type,bit_depth);
    return papng;
}

static void apng_blend_to_render(apng_info_t *papng, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    uint8_t *prender = papng->render_buf;
    uint8_t *pbase = papng->base_buf;
    int height = papng->height;
    int width  = papng->width;
    int row_size = width * 4;
    int opa_i = 3;

    if(papng->pcur_base == pbase){
        memcpy(prender,pbase, y * row_size);

        int offset = (y + h) * row_size;
        memcpy(prender + offset,pbase + offset, (height - y - h) * row_size);

        int end = y + h;
        for(int i = y; i < end; i ++){
            memcpy(&prender[i * row_size],&pbase[i * row_size], x*4);
            memcpy(&prender[i * row_size + (x + w)*4],&pbase[i * row_size + (x + w)*4], (width - x - w)*4);
        }
    }

    int u, v, al;
    int i_end = y + h;
    int k_end = (x + w) * 4;
    for(int i = y; i < i_end; i ++){
        int row_offset = i * row_size;
        for(int k = x*4; k < k_end; k += 4){
            if(prender[row_offset + k + opa_i] == 255){//什么都不用干
            }
            else if(prender[row_offset + k + opa_i] == 0){//用base填充
                memcpy(&prender[row_offset + k],&pbase[row_offset + k],4);
            }
            else if(1){
                int index = row_offset + k;
                if(pbase[index + opa_i] != 0){
                    u = prender[index + opa_i] * 255;
                    v = (255 - prender[index + opa_i]) * pbase[index + opa_i];
                    al = u + v;
                    prender[index + 0] = (prender[index + 0] * u + pbase[index + 0] * v) / al;
                    prender[index + 1] = (prender[index + 1] * u + pbase[index + 1] * v) / al;
                    prender[index + 2] = (prender[index + 2] * u + pbase[index + 2] * v) / al;
                    prender[index + 3] = al / 255;
                }
            }
        }
    }
}

static void apng_fill_to_render(apng_info_t *papng, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    uint8_t *prender = papng->render_buf;
    uint8_t *pbase = papng->base_buf;
    int height = papng->height;
    int width  = papng->width;
    int row_size = width * 4;

    if(papng->pcur_base == pbase){
        memcpy(prender,pbase, y * row_size);

        int offset = (y + h) * row_size;
        memcpy(prender + offset,pbase + offset, (height - y - h) * row_size);

        int end = y + h;
        for(int i = y; i < end; i ++){
            memcpy(&prender[i * row_size],&pbase[i * row_size], x*4);
            memcpy(&prender[i * row_size + (x + w)*4],&pbase[i * row_size + (x + w)*4], (width - x - w)*4);
        }
    }
}

static void apng_cpy_area_to_base(apng_info_t *papng, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    uint8_t *prender = papng->render_buf;
    uint8_t *pbase = papng->base_buf;
    int width  = papng->width;
    int row_size = width * 4;

    int end = y + h;
    for(int i = y; i < end; i ++){
        memcpy(&pbase[i * row_size + x*4],&prender[i * row_size + x*4], w*4);
    }
}

static void apng_base_area_clear(apng_info_t *papng, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    uint8_t *prender = papng->render_buf;
    uint8_t *pbase = papng->base_buf;
    int height = papng->height;
    int width  = papng->width;
    int row_size = width * 4;

    memcpy(pbase, prender, height * width * 4);

    int end = y + h;
    for(int i = y; i < end; i ++){
        memset(&pbase[i * row_size + x*4], 0x00, w*4);
    }
}

static int apng_render_next_frame(apng_info_t *papng)
{
    png_structp png_ptr =  papng->png_ptr;
    png_infop info_ptr  = papng->info_ptr;

    png_uint_32 x0 = 0;
    png_uint_32 y0 = 0;
    png_uint_32 w0 = papng->width;
    png_uint_32 h0 = papng->height;
    png_uint_16 delay_num = 1;
    png_uint_16 delay_den = 100;
    png_byte dispose_op = 0;
    png_byte blend_op = 0;

    papng->current_frame++;
    if(papng->current_frame >= papng->total_frames){
        papng->current_frame = 0;
        return 0;
    }

    png_read_frame_head(png_ptr, info_ptr);
    png_get_next_frame_fcTL(png_ptr, info_ptr, &w0, &h0,
        &x0, &y0,&delay_num, &delay_den, &dispose_op, &blend_op);
    LV_LOG_TRACE("w:%d,h:%d,x:%d,y:%d,delay:(%d/%d),dispose:%d,blend:%d\n",
        w0,h0,x0,y0,delay_num,delay_den,dispose_op,blend_op);

    int height = papng->height;
    int width  = papng->width;
    uint8_t *prender = papng->render_buf;
    uint8_t *pbase   = papng->base_buf;
    png_bytepp prows = papng->prows;

    delay_den = delay_den == 0 ? 100 : delay_den;
    papng->framerate = delay_num * 1000 / delay_den;

    if(papng->current_frame == 1){
        blend_op = PNG_BLEND_OP_SOURCE;
        if (dispose_op == PNG_DISPOSE_OP_PREVIOUS){
            dispose_op = PNG_DISPOSE_OP_BACKGROUND;
        }
    }

    for (uint32_t j = 0; j < h0; j++){
        prows[j] = prender + (j + y0) * width * 4 + x0 * 4;
    }

    if(papng->current_frame == 1){
        papng->pcur_base = prender;
    }

    if(papng->pcur_base == prender){
        if(dispose_op == PNG_DISPOSE_OP_PREVIOUS){
            memcpy(pbase,prender,height * width * 4);
            papng->pcur_base = pbase;
        }
        else if (blend_op == PNG_BLEND_OP_OVER){
            apng_cpy_area_to_base(papng, x0, y0, w0, h0);
        }
    }

    png_read_image(png_ptr, prows);

    if (blend_op == PNG_BLEND_OP_OVER){
        apng_blend_to_render(papng, x0, y0, w0, h0);
    }
    else{
        apng_fill_to_render(papng, x0, y0, w0, h0);
    }

    if(dispose_op == PNG_DISPOSE_OP_BACKGROUND){
        apng_base_area_clear(papng, x0, y0, w0, h0);
        papng->pcur_base = pbase;
    }
    else{
        papng->pcur_base = prender;
    }

    return papng->current_frame;
}

static void next_frame_task_cb(lv_task_t * t)
{
   lv_obj_t * img = t->user_data;
   lv_apng_ext_t * ext = lv_obj_get_ext_attr(img);

   uint32_t elaps = lv_tick_elaps(ext->last_call);
   if(elaps < ext->apng->framerate){
       return;
   }

   ext->last_call = lv_tick_get();
   int has_next = apng_render_next_frame(ext->apng);
   if(has_next == 0) {
       lv_res_t res = lv_signal_send(img, LV_SIGNAL_LEAVE, NULL);
       if(res != LV_RES_OK) return;
     
       res = lv_event_send(img, LV_EVENT_LEAVE, NULL);
       if(res != LV_RES_OK) return;
   }

   lv_img_cache_invalidate_src(lv_img_get_src(img));
   lv_obj_invalidate(img);
}

/**
* Signal function of the image
* @param img pointer to a image object
* @param sign a signal type from lv_signal_t enum
* @param param pointer to a signal specific variable
* @return LV_RES_OK: the object is not deleted in the function; LV_RES_INV: the object is deleted
*/
static lv_res_t lv_apng_signal(lv_obj_t * img, lv_signal_t sign, void * param)
{
   lv_res_t res;

   /* Include the ancient signal function */
   res = ancestor_signal(img, sign, param);
   if(res != LV_RES_OK) return res;
   if(sign == LV_SIGNAL_GET_TYPE) return lv_obj_handle_get_type_signal(param, LV_OBJX_NAME);

   lv_apng_ext_t * ext = lv_obj_get_ext_attr(img);

   if(sign == LV_SIGNAL_CLEANUP) {
       png_close(ext->apng);
       lv_mem_free(ext->apng);
       ext->apng = NULL;
       lv_task_del(ext->task);
   } else if (sign == LV_SIGNAL_LEAVE) {
       lv_task_set_prio(ext->task, LV_TASK_PRIO_OFF);
   }

   return LV_RES_OK;
}
