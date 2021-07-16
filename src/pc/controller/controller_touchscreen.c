//Feel free to use it in your port too, but please keep authorship!
//Touch Controls made by: VDavid003
#ifdef TOUCH_CONTROLS
#include <PR/ultratypes.h>
#include <PR/gbi.h>

#include "config.h"
#include "game/game_init.h"
#include "game/memory.h"
#include "game/segment2.h"
#include "gfx_dimensions.h"
#include "pc/gfx/gfx_pc.h"

#include "controller_api.h"
#include "controller_touchscreen.h"

#define SCREEN_WIDTH_API 1280
#define SCREEN_HEIGHT_API 960

#define LEFT_EDGE ((int)floorf(SCREEN_WIDTH_API / 2 - SCREEN_HEIGHT_API / 2 * gfx_current_dimensions.aspect_ratio))
#define RIGHT_EDGE ((int)ceilf(SCREEN_WIDTH_API / 2 + SCREEN_HEIGHT_API / 2 * gfx_current_dimensions.aspect_ratio))

#define CORRECT_TOUCH_X(x) ((x * (RIGHT_EDGE - LEFT_EDGE)) + LEFT_EDGE)
#define CORRECT_TOUCH_Y(y) (y * SCREEN_HEIGHT_API)

#define JOYSTICK_SIZE 280

enum ControlElementType {
    Joystick,
    Button
};

struct Position {
    s32 x,y;
};

struct ControlElement {
    enum ControlElementType type;
    struct Position (*GetPos)();
    u8 touchID; //0 = not being touched, 1-255 = Finger being used
    //Joystick
    int joyX, joyY;
    //Button
    int buttonID;
    char character;
    int slideTouch;
};

#include "controller_touchscreen_layouts.inc"

static struct ControlElement *ControlElements = ControlElementsDefault;
static int ControlElementsLength = sizeof(ControlElementsDefault)/sizeof(struct ControlElement);

#define TRIGGER_DETECT(size) (((pos.x + size / 2 > CORRECT_TOUCH_X(event->x)) && (pos.x - size / 2 < CORRECT_TOUCH_X(event->x))) &&\
                              ((pos.y + size / 2 > CORRECT_TOUCH_Y(event->y)) && (pos.y - size / 2 < CORRECT_TOUCH_Y(event->y))))

void touch_down(struct TouchEvent* event) {
    set_current_input(touchScreen);

    struct Position pos;
    for(int i = 0; i < ControlElementsLength; i++) {
        if (ControlElements[i].touchID == 0) {
            pos = ControlElements[i].GetPos();
            switch (ControlElements[i].type) {
                case Joystick:
                    if (TRIGGER_DETECT(JOYSTICK_SIZE)) {
                        ControlElements[i].touchID = event->touchID;
                        ControlElements[i].joyX = CORRECT_TOUCH_X(event->x) - pos.x;
                        ControlElements[i].joyY = CORRECT_TOUCH_Y(event->y) - pos.y;
                    }
                    break;
                case Button:
                    if (TRIGGER_DETECT(120)) {
                        ControlElements[i].touchID = event->touchID;
                    }
                    break;
            }
        }
    }
}

void touch_motion(struct TouchEvent* event) {
    struct Position pos;
    for(int i = 0; i < ControlElementsLength; i++) {
        pos = ControlElements[i].GetPos();
        if (ControlElements[i].touchID == event->touchID) {
            pos = ControlElements[i].GetPos();
                switch (ControlElements[i].type) {
                    case Joystick:
                        ; //workaround
                        s32 x,y;
                        x = CORRECT_TOUCH_X(event->x) - pos.x;
                        y = CORRECT_TOUCH_Y(event->y) - pos.y;
                        if (pos.x + JOYSTICK_SIZE/2 < CORRECT_TOUCH_X(event->x))
                            x = JOYSTICK_SIZE/2;
                        if (pos.x - JOYSTICK_SIZE/2 > CORRECT_TOUCH_X(event->x))
                            x = -JOYSTICK_SIZE/2;
                        if (pos.y + JOYSTICK_SIZE/2 < CORRECT_TOUCH_Y(event->y))
                            y = JOYSTICK_SIZE/2;
                        if (pos.y - JOYSTICK_SIZE/2 > CORRECT_TOUCH_Y(event->y))
                            y = -JOYSTICK_SIZE/2;

                        ControlElements[i].joyX = x;
                        ControlElements[i].joyY = y;
                        break;
                    case Button:
                        if (ControlElements[i].slideTouch && !TRIGGER_DETECT(120)) {
                            ControlElements[i].slideTouch = 0;
                            ControlElements[i].touchID = 0;
                        }
                        break;
                }
        }
        else {
            switch (ControlElements[i].type) {
                case Joystick:
                    break;
                case Button:
                    if (TRIGGER_DETECT(120)) {
                        ControlElements[i].slideTouch = 1;
                        ControlElements[i].touchID = event->touchID;
                    }
                    break;
            }
        }
    }
}

static void handle_touch_up(int i) {//seperated for when the layout changes
    ControlElements[i].touchID = 0;
    switch (ControlElements[i].type) {
        case Joystick:
            ControlElements[i].joyX = 0;
            ControlElements[i].joyY = 0;
            break;
        case Button:
            break;
    }
}

void touch_up(struct TouchEvent* event) {
    struct Position pos;
    for(int i = 0; i < ControlElementsLength; i++) {
        if (ControlElements[i].touchID == event->touchID) {
            handle_touch_up(i);
        }
    }
}

ALIGNED8 static const u8 texture_button[] = {
#include "textures/touchcontrols/touch_button.rgba16.inc.c"
};

ALIGNED8 static const u8 texture_button_dark[] = {
#include "textures/touchcontrols/touch_button_dark.rgba16.inc.c"
};

//Sprite drawing code stolen from src/game/print.c

static void select_button_texture(int dark) {
    const u8 *const *glyphs = segmented_to_virtual(main_hud_lut);

    gDPPipeSync(gDisplayListHead++);
    if (!dark) {
        gDPSetTextureImage(gDisplayListHead++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture_button);
    }
    else {
        gDPSetTextureImage(gDisplayListHead++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture_button_dark);
    }
    gSPDisplayList(gDisplayListHead++, dl_hud_img_load_tex_block);
}

static void select_char_texture(char num) {
    const u8 *const *glyphs = segmented_to_virtual(main_hud_lut);

    gDPPipeSync(gDisplayListHead++);
    gDPSetTextureImage(gDisplayListHead++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, glyphs[num - 87]);
    gSPDisplayList(gDisplayListHead++, dl_hud_img_load_tex_block);
}

static void DrawSprite(s32 x, s32 y, int scaling) {
    gSPTextureRectangle(gDisplayListHead++, x - (15 << scaling), y - (15 << scaling), x + (15 << scaling),
                        y + (15 << scaling), G_TX_RENDERTILE, 0, 0, 4 << (11 - scaling), 1 << (11 - scaling));
}

void render_touch_controls(void) {
    if(get_current_input()!=touchScreen)
        return;

    Mtx *mtx;

    mtx = alloc_display_list(sizeof(*mtx));

    if (mtx == NULL) {
        return;
    }

    guOrtho(mtx, 0.0f, SCREEN_WIDTH, 0.0f, SCREEN_HEIGHT, -10.0f, 10.0f, 1.0f);
    gSPPerspNormalize((Gfx *) (gDisplayListHead++), 0xFFFF);
    gSPMatrix(gDisplayListHead++, VIRTUAL_TO_PHYSICAL(mtx), G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPDisplayList(gDisplayListHead++, dl_hud_img_begin);

    struct Position pos;
    for (int i = 0; i < ControlElementsLength; i++) {
        select_button_texture(0);
        switch (ControlElements[i].type) {
            case Joystick:
                pos = ControlElements[i].GetPos();
                DrawSprite(pos.x, pos.y, 3);
                DrawSprite(pos.x + 4 + ControlElements[i].joyX, pos.y + 4 + ControlElements[i].joyY, 2);
                break;
            case Button:
                if (ControlElements[i].touchID)
                    select_button_texture(1);
                pos = ControlElements[i].GetPos();
                DrawSprite(pos.x - 8, pos.y, 2);
                select_char_texture(ControlElements[i].character);
                DrawSprite(pos.x, pos.y, 1);
                break;
        }
    }

    gSPDisplayList(gDisplayListHead++, dl_hud_img_end);
}

static void touchscreen_init() {
}

static void touchscreen_read(OSContPad *pad) {
    for(int i = 0; i < ControlElementsLength; i++) {
        switch (ControlElements[i].type) {
            case Joystick:
                if (ControlElements[i].joyX || ControlElements[i].joyY) {
                    pad->stick_x = (ControlElements[i].joyX + JOYSTICK_SIZE/2) * 255 / JOYSTICK_SIZE - 128;
                    pad->stick_y = (-ControlElements[i].joyY + JOYSTICK_SIZE/2) * 255 / JOYSTICK_SIZE - 128; //inverted for some reason
                }
                break;
            case Button:
                if (ControlElements[i].touchID) {
                    pad->button |= ControlElements[i].buttonID;
                }
                break;
        }
    }
}
struct ControllerAPI controller_touchscreen = {
    touchscreen_init,
    touchscreen_read
};
#endif
