#ifndef __SCREENS_H__
#define __SCREENS_H__

#define SCREEN_BUILD_ID  7

//#define TRACKERS_ENABLED
//#define TAG_BYPASS        //set defined if tags wont read

#define SCREEN_W          480
#define SCREEN_H          272

#define CELL_BORDER            4
#define BOX_WIDTH         2

#define GRID_SX(N)            (int)(SCREEN_W/(N))
#define GRID_SY(N)            (int)(SCREEN_H/(N))
#define GRID_XL(n,N)         (int)((n)*GRID_SX(N))
#define GRID_YT(n,N)         (int)((n)*GRID_SY(N))
#define GRID_XC(n,N)         (int)(GRID_XL(n,N)+(GRID_SX(N)>>1))
#define GRID_YC(n,N)         (int)(GRID_YT(n,N)+(GRID_SY(N)>>1))
#define GRID_XR(n,N)         (int)((n+1)*GRID_SX(N))
#define GRID_YB(n,N)         (int)((n+1)*GRID_SY(N))

#define DRO_OPT  (OPT_RIGHTX | OPT_CENTERY | OPT_SIGNED)

#define SUBPIXEL_BITS    4


#define PTSP(px)         (px<<SUBPIXEL_BITS)

#define C_BKG_STOPPED    RGB(0x00,0x20,0x40)
#define C_BKG_RUNNING    RGB(0x00,0x40,0x20)
#define C_BKG_WARNING    RGB(0x20,0x40,0x00)
#define C_BKG_FAULT      RGB(0x40,0x20,0x00)

#define C_BKG_NORMAL     RGB(0x00,0x00,0x00)

#define C_SLIDER_VERT     RGB(0x00,0x00,0x00)

#define C_LABEL          RGB(0xC0,0xD0,0xD0)
#define C_VALUE          RGB(0xFF,0xFF,0xD0)

#define C_BUTTON_FG      RGB(0x00,0x10,0xD0)

#define C_MARKER          RGB(0xFF, 0x50, 0x00)

#define C_BTN_LOGGING   RGB(0x80, 0,0)

#define A_OPAQUE         255
#define A_CLEAR          0
#define A_BKG_WINDOW     64







//touch and tracker registers
//#define REG_TOUCH_TAG   0x30212C //this is already defined by GD2.h
//#define REG_TOUCH_XY    0x302124  //this is already defined by GD2.h, and is different! Maybe this is why reading tags were wrong!
//#define REG_TRACKER     0x307000 //this is already defined by GD2.h, and is different!

//touch event types
#define TOUCH_OFF       0
#define TOUCH_ON        1
#define TOUCH_CANCEL    2
#define TOUCH_NONE      255



// touch cal registers

typedef struct gd_transform{
  long int a,b,c,d,e,f;
}GD_TRANSFORM;

//tough tag assignments
#define TAG_INVALID 255
typedef enum tags
{
  TAG_NONE     =   0,
  

  TAG_SCREEN_1,
  TAG_SCREEN_2,
  TAG_SCREEN_3,
  TAG_SCREEN_4,
  TAG_SCREEN_5,

  TAG_LOG_TOGGLE, //could be one tag: start/stop
  TAG_LOG_TOGGLE_SDCARD,
  TAG_LOG_TOGGLE_SDCARD_HEX,
  TAG_LOG_TOGGLE_SERIAL,
  TAG_LOG_TOGGLE_SERIAL_HEX,

  TAG_CAL_SV0_MIN,  //in order of drawing for simplified tag calculation
  TAG_CAL_SV0_MAX,
  TAG_CAL_SV1_MIN,
  TAG_CAL_SV1_MAX,
  TAG_CAL_SV2_MIN,
  TAG_CAL_SV2_MAX,

  TAG_EEPROM_SAVE,
  TAG_EEPROM_LOAD,

  TAG_EEPROM_EXPORT,
  TAG_EEPROM_IMPORT,

  TAG_MODE_SET_PID_RPM,

  TAG_CAL_PID_RPM_P,
  TAG_CAL_PID_RPM_I,
  TAG_CAL_PID_RPM_D,

  TAG_ENGINE_START,
  TAG_ENGINE_STOP,

  TAG_HOLD_INPUT,
  TAG_CAL_TORQUE_ZERO,
  TAG_CAL_TORQUE_MIN,
  TAG_CAL_TORQUE_MAX,

  TAG_PID_INVERT,

  NO_OF_TAGS,
  
}TAGS_EN;
#if NO_OF_TAGS > TAG_INVALID
#error "Too many tags"
#endif

typedef enum screen
{
  SCREEN_1,
  SCREEN_2,
  SCREEN_3,
  SCREEN_4,
  SCREEN_5,
  NO_OF_SCREENS
} SCREEN_EN;

extern SCREEN_EN current_screen;

#endif // __SCREENS_H__
