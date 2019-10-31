# lv_png_decoder

Allow the use of PNG images in LittlevGL. This implementation uses [lodepng](https://github.com/lvandeve/lodepng) library.

## Get started
- Download or clone this repository
  - [Download from GitHub](https://github.com/littlevgl/lv_lib_lodepng/archive/master.zip)
  - Clone: `git clone https://github.com/littlevgl/lv_lib_lodepng.git`
- Include the library: `#include "lv_lib_lodepng/lv_lodepng.h"`
- Initalize the decocer with `lv_lodepng_init()`
- Test with the following code:
```c
    LV_IMG_DECLARE(png_decoder_test);
    lv_obj_t * img = lv_img_create(lv_scr_act(), NULL);
    lv_img_set_src(img, &png_decoder_test);
```

## Use PNG images from file
`lodepng` uses the POSIX filesystem API. PNG files can be used like this:
```c
lv_img_set_src(img, "./lv_lib_lodepng/png_decoder_test.png");
```

Note that the path of the file might be different.

## Use PNG images from flash
To store a PNG images in flash it needs to be converted to C array with [Online Image converter](https://littlevgl.com/image-to-c-array). Choose `Raw with alpha` Color format and `C array` Output format. Copy the result C array to your project and use it like this:
```c
    LV_IMG_DECLARE(my_test_img);
    lv_obj_t * img = lv_img_create(lv_scr_act(), NULL);
    lv_img_set_src(img, &my_test_img);
```

## Learn more
To learn more about the PNG decoder itself read [this blog post](https://blog.littlevgl.com/2018-10-05/png_converter)

To learn more about the Image decoder interface of LittlevGL read the realevant part of the [documentation](https://docs.littlevgl.com/en/html/overview/image.html#image-decoder).
