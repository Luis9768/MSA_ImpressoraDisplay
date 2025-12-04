#pragma once
#include <lvgl.h>

// Declare external custom font symbol if you later add it
// Place generated file at `fonts/font_montserrat_pt_22.c` and declare:
//   extern const lv_font_t lv_font_montserrat_pt_22;

static inline const lv_font_t* pt_font_22() {
#ifdef USE_PT_FONT_22
  extern const lv_font_t* lv_font_montserrat_pt_22;
  return lv_font_montserrat_pt_22;
#else
  return &lv_font_montserrat_22;
#endif
}

static inline const lv_font_t* pt_font_18() {
#ifdef USE_PT_FONT_18
  extern const lv_font_t* lv_font_montserrat_pt_18;
  return lv_font_montserrat_pt_18;
#else
  return &lv_font_montserrat_18;
#endif
}
