//
// Created by user on 5/4/25.
//

#pragma once


#define GLSG_KEY_UNKNOWN (-1)

#define GLSG_KEY_LEFT 18
#define GLSG_KEY_UP 19
#define GLSG_KEY_RIGHT 20
#define GLSG_KEY_DOWN 21

#define GLSG_KEY_SPACE 32
#define GLSG_KEY_APOSTROPHE 39 /* ' */
#define GLSG_KEY_COMMA 44      /* , */
#define GLSG_KEY_MINUS 45      /* - */
#define GLSG_KEY_PERIOD 46     /* . */
#define GLSG_KEY_SLASH 47      /* / */
#define GLSG_KEY_0 48
#define GLSG_KEY_1 49
#define GLSG_KEY_2 50
#define GLSG_KEY_3 51
#define GLSG_KEY_4 52
#define GLSG_KEY_5 53
#define GLSG_KEY_6 54
#define GLSG_KEY_7 55
#define GLSG_KEY_8 56
#define GLSG_KEY_9 57
#define GLSG_KEY_SEMICOLON 59 /* ; */
#define GLSG_KEY_EQUAL 61     /* = */
#define GLSG_KEY_A 65
#define GLSG_KEY_B 66
#define GLSG_KEY_C 67
#define GLSG_KEY_D 68
#define GLSG_KEY_E 69
#define GLSG_KEY_F 70
#define GLSG_KEY_G 71
#define GLSG_KEY_H 72
#define GLSG_KEY_I 73
#define GLSG_KEY_J 74
#define GLSG_KEY_K 75
#define GLSG_KEY_L 76
#define GLSG_KEY_M 77
#define GLSG_KEY_N 78
#define GLSG_KEY_O 79
#define GLSG_KEY_P 80
#define GLSG_KEY_Q 81
#define GLSG_KEY_R 82
#define GLSG_KEY_S 83
#define GLSG_KEY_T 84
#define GLSG_KEY_U 85
#define GLSG_KEY_V 86
#define GLSG_KEY_W 87
#define GLSG_KEY_X 88
#define GLSG_KEY_Y 89
#define GLSG_KEY_Z 90
#define GLSG_KEY_LEFT_BRACKET 91  /* [ */
#define GLSG_KEY_BACKSLASH 92     /* \ */
#define GLSG_KEY_RIGHT_BRACKET 93 /* ] */
#define GLSG_KEY_GRAVE_ACCENT 96  /* ` */
#define GLSG_KEY_WORLD_1 161      /* non-US #1 */
#define GLSG_KEY_WORLD_2 162      /* non-US #2 */

/* std::function keys */
#define GLSG_KEY_ESCAPE 256
#define GLSG_KEY_ENTER 257
#define GLSG_KEY_TAB 258
#define GLSG_KEY_BACKSPACE 259
#define GLSG_KEY_INSERT 260
#define GLSG_KEY_DELETE 261
#define GLSG_KEY_PAGE_UP 266
#define GLSG_KEY_PAGE_DOWN 267
#define GLSG_KEY_HOME 268
#define GLSG_KEY_END 269
#define GLSG_KEY_CAPS_LOCK 280
#define GLSG_KEY_SCROLL_LOCK 281
#define GLSG_KEY_NUM_LOCK 282
#define GLSG_KEY_PRINT_SCREEN 283
#define GLSG_KEY_PAUSE 284
#define GLSG_KEY_F1 290
#define GLSG_KEY_F2 291
#define GLSG_KEY_F3 292
#define GLSG_KEY_F4 293
#define GLSG_KEY_F5 294
#define GLSG_KEY_F6 295
#define GLSG_KEY_F7 296
#define GLSG_KEY_F8 297
#define GLSG_KEY_F9 298
#define GLSG_KEY_F10 299
#define GLSG_KEY_F11 300
#define GLSG_KEY_F12 301
#define GLSG_KEY_F13 302
#define GLSG_KEY_F14 303
#define GLSG_KEY_F15 304
#define GLSG_KEY_F16 305
#define GLSG_KEY_F17 306
#define GLSG_KEY_F18 307
#define GLSG_KEY_F19 308
#define GLSG_KEY_F20 309
#define GLSG_KEY_F21 310
#define GLSG_KEY_F22 311
#define GLSG_KEY_F23 312
#define GLSG_KEY_F24 313
#define GLSG_KEY_F25 314
#define GLSG_KEY_KP_0 320
#define GLSG_KEY_KP_1 321
#define GLSG_KEY_KP_2 322
#define GLSG_KEY_KP_3 323
#define GLSG_KEY_KP_4 324
#define GLSG_KEY_KP_5 325
#define GLSG_KEY_KP_6 326
#define GLSG_KEY_KP_7 327
#define GLSG_KEY_KP_8 328
#define GLSG_KEY_KP_9 329
#define GLSG_KEY_KP_DECIMAL 330
#define GLSG_KEY_KP_DIVIDE 331
#define GLSG_KEY_KP_MULTIPLY 332
#define GLSG_KEY_KP_SUBTRACT 333
#define GLSG_KEY_KP_ADD 334
#define GLSG_KEY_KP_ENTER 335
#define GLSG_KEY_KP_EQUAL 336
#define GLSG_KEY_LEFT_SHIFT 340
#define GLSG_KEY_LEFT_CONTROL 341
#define GLSG_KEY_LEFT_ALT 342
#define GLSG_KEY_LEFT_SUPER 343
#define GLSG_KEY_RIGHT_SHIFT 344
#define GLSG_KEY_RIGHT_CONTROL 345
#define GLSG_KEY_RIGHT_ALT 346
#define GLSG_KEY_RIGHT_SUPER 347
#define GLSG_KEY_MENU 348

#define GLSG_KEY_LAST GLSG_KEY_MENU

/*! @brief If this bit is set one or more Shift keys were held down.
 *
 *  If this bit is set one or more Shift keys were held down.
 */
#define GLSG_MOD_SHIFT 0x0001
/*! @brief If this bit is set one or more Control keys were held down.
 *
 *  If this bit is set one or more Control keys were held down.
 */
#define GLSG_MOD_CONTROL 0x0002
/*! @brief If this bit is set one or more Alt keys were held down.
 *
 *  If this bit is set one or more Alt keys were held down.
 */
#define GLSG_MOD_ALT 0x0004
/*! @brief If this bit is set one or more Super keys were held down.
 *
 *  If this bit is set one or more Super keys were held down.
 */
#define GLSG_MOD_SUPER 0x0008
/*! @brief If this bit is set the Caps Lock key is enabled.
 *
 *  If this bit is set the Caps Lock key is enabled and the @ref
 *  GLFW_LOCK_KEY_MODS input mode is set.
 */
#define GLSG_MOD_CAPS_LOCK 0x0010
/*! @brief If this bit is set the Num Lock key is enabled.
 *
 *  If this bit is set the Num Lock key is enabled and the @ref
 *  GLFW_LOCK_KEY_MODS input mode is set.
 */
#define GLSG_MOD_NUM_LOCK 0x0020

/*! @name Key and button actions
 *  @{ */
/*! @brief The key or mouse button was released.
 *
 *  The key or mouse button was released.
 *
 *  @ingroup input
 */
#define GLSG_RELEASE 0
/*! @brief The key or mouse button was pressed.
 *
 *  The key or mouse button was pressed.
 *
 *  @ingroup input
 */
#define GLSG_PRESS 1
/*! @brief The key was held down until it repeated.
 *
 *  The key was held down until it repeated.
 *
 *  @ingroup input
 */
#define GLSG_REPEAT 2
/*! @} */

/*! @defgroup buttons Mouse buttons
 *  @brief Mouse button IDs.
 *
 *  See [mouse button input](@ref input_mouse_button) for how these are used.
 *
 *  @ingroup input
 *  @{ */
#define GLSG_MOUSE_BUTTON_1 0
#define GLSG_MOUSE_BUTTON_2 1
#define GLSG_MOUSE_BUTTON_3 2
#define GLSG_MOUSE_BUTTON_4 3
#define GLSG_MOUSE_BUTTON_5 4
#define GLSG_MOUSE_BUTTON_6 5
#define GLSG_MOUSE_BUTTON_7 6
#define GLSG_MOUSE_BUTTON_8 7
#define GLSG_MOUSE_BUTTON_LAST GLSG_MOUSE_BUTTON_8
#define GLSG_MOUSE_BUTTON_LEFT GLSG_MOUSE_BUTTON_1
#define GLSG_MOUSE_BUTTON_RIGHT GLSG_MOUSE_BUTTON_2
#define GLSG_MOUSE_BUTTON_MIDDLE GLSG_MOUSE_BUTTON_3
/*! @} */

#define GLSG_CURSOR 0x00033001
#define GLSG_STICKY_KEYS 0x00033002
#define GLSG_STICKY_MOUSE_BUTTONS 0x00033003
#define GLSG_LOCK_KEY_MODS 0x00033004
#define GLSG_RAW_MOUSE_MOTION 0x00033005

#define GLSG_CURSOR_NORMAL 0x00034001
#define GLSG_CURSOR_HIDDEN 0x00034002
#define GLSG_CURSOR_DISABLED 0x00034003

// Internal key state used for sticky keys
#define _GLSG_STICK 3