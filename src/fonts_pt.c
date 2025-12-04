#include <lvgl.h>

// Provide linker-visible symbols for Portuguese fonts
// Until real generated fonts are added, alias to built-in Montserrat sizes.
extern const lv_font_t lv_font_montserrat_22;
extern const lv_font_t lv_font_montserrat_18;

// Provide pointer aliases instead of object copies to satisfy the linker
const lv_font_t* lv_font_montserrat_pt_22 = &lv_font_montserrat_22;
const lv_font_t* lv_font_montserrat_pt_18 = &lv_font_montserrat_18;
