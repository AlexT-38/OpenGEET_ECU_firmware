#ifndef __SCREENS_H__
#define __SCREENS_H__

#define SCREEN_W          480
#define SCREEN_H          272

#define BORDER            4
#define BOX_WIDTH         2

#define GRID_SX(N)            (int)(SCREEN_W/N)
#define GRID_SY(N)            (int)(SCREEN_H/N)
#define GRID_XL(n,N)         (int)(n*GRID_SX(N))
#define GRID_YT(n,N)         (int)(n*GRID_SY(N))
#define GRID_XC(n,N)         (int)(((n<<1)+1)*GRID_SX(N<<1))
#define GRID_YC(n,N)         (int)(((n<<1)+1)*GRID_SY(N<<1))
#define GRID_XR(n,N)         (int)((n+1)*GRID_SX(N))
#define GRID_YB(n,N)         (int)((n+1)*GRID_SY(N))

#define SUBPIXEL_BITS    4
#define SCREEN_BUILD_ID  4

#define PTSP(px)         (px<<SUBPIXEL_BITS)

#define C_BKG_STOPPED    RGB(0x00,0x20,0x40)
#define C_BKG_RUNNING    RGB(0x00,0x40,0x20)
#define C_BKG_WARNING    RGB(0x20,0x40,0x00)
#define C_BKG_FAULT      RGB(0x40,0x20,0x00)

#define C_BKG_NORMAL     RGB(0x00,0x00,0x00)

#define C_LABEL          RGB(0xC0,0xD0,0xD0)
#define C_VALUE          RGB(0xFF,0xFF,0xD0)

#define C_BUTTON_FG      RGB(0x00,0x10,0xD0)

#define A_OPAQUE         255
#define A_CLEAR          0
#define A_BKG_WINDOW     64


//touch and tracker registers
#define REG_TOUCH_TAG   0x30212C
#define REG_TOUCH_XY    0x302124
#define REG_TRACKER     0x307000

//touch event types
#define TOUCH_OFF       0
#define TOUCH_ON        1
#define TOUCH_NONE      255

//tough tag assignments


typedef enum tags
{
  TAG_NONE     =   0,
  

  TAG_SCREEN_1,
  TAG_SCREEN_2,
  TAG_SCREEN_3,
  TAG_SCREEN_4,
  TAG_SCREEN_5,

  TAG_LOG_START, //could be one tag: start/stop
  TAG_LOG_STOP,
  TAG_LOG_TOGGLE_SDCARD,
  TAG_LOG_TOGGLE_SERIAL,
  TAG_LOG_TOGGLE_SDCARD_HEX,
  TAG_LOG_TOGGLE_SERIAL_HEX,
  
  TAG_CAL_SV0_TOP,
  TAG_CAL_SV1_TOP,
  TAG_CAL_SV2_TOP,

  TAG_CAL_SV0_BOT,
  TAG_CAL_SV1_BOT,
  TAG_CAL_SV2_BOT,

  TAG_ENGINE_START,
  TAG_ENGINE_STOP,

  TAG_INVALID   =  255,
  
}TAGS_EN;

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
