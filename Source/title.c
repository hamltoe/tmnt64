#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>
#include "core.h"
#include "setup.h"

#define FADE_DURATION 1.2f

static rdpq_font_t* global_font;
static float global_flash;
static float global_fade;
static bool global_is_fading;
static sprite_t *global_sprite_n64brew;
static sprite_t *global_sprite_jam;
static wav64_t sfx_title_confirm;
static xm64player_t global_music;

static T3DModel *model;
static T3DSkeleton skeleton;
static T3DViewport viewport;
static T3DMat4FP *mtx;
static T3DMat4FP *mtx2;
static rspq_block_t *model_block;
static rspq_syncpoint_t sync;

static color_t color;

typedef struct
{
    const char *path;
    float scale;
    T3DVec3 offset;
    T3DVec3 rotation;
    bool has_skeleton;
} model_data;

static model_data models[] = {
    {.path = "rom:/avanto/guy.t3dm", .scale = 0.45f, .offset = {{0, -35, 0}}, .has_skeleton = true},
    {.path = "rom:/avanto/ukko.t3dm", .scale = 0.45f, .offset = {{0, -35, 0}}, .has_skeleton = true},
    {.path = "rom:/furball/fastcat.t3dm", .scale = 0.07f, .offset = {{0, -7, 0}}, .has_skeleton = true},
    {.path = "rom:/larcenygame/foxThief.t3dm", .scale = 0.35f, .offset = {{0, -35, 0}}, .has_skeleton = true},
    {.path = "rom:/larcenygame/dogGuard.t3dm", .scale = 0.35f, .offset = {{0, -35, 0}}, .has_skeleton = true},
    {.path = "rom:/lucker/snake.t3dm", .scale = 0.2f, .offset = {{0, -28, 0}}, .has_skeleton = true},
    {.path = "rom:/rampage/Jira_01.t3dm", .scale = 0.2f, .offset = {{0, -25, 0}}, .has_skeleton = true},
    {.path = "rom:/spacewaves/enemycraft.t3dm", .scale = 0.2f, .offset = {{0, 0, -20}}, .has_skeleton = false},
    {.path = "rom:/strawberry_byte/dogman.t3dm", .scale = 0.23f, .offset = {{0, -35, 0}}, .rotation = {{-80, 0, 0}}, .has_skeleton = true},
    {.path = "rom:/strawberry_byte/mew.t3dm", .scale = 0.28f, .offset = {{0, -32, 0}}, .rotation = {{-80, 0, 0}}, .has_skeleton = true},
    {.path = "rom:/strawberry_byte/s4ys.t3dm", .scale = 0.25f, .offset = {{0, -32, 0}}, .rotation = {{-80, 0, 0}}, .has_skeleton = true},
    {.path = "rom:/strawberry_byte/wolfie.t3dm", .scale = 0.27f, .offset = {{0, -35, 0}}, .rotation = {{-80, 0, 0}}, .has_skeleton = true},
    {.path = "rom:/tohubohu/player.t3dm", .scale = 0.2f, .offset = {{0, -35, 0}}, .has_skeleton = true},
    {.path = "rom:/core/brew_logo.t3dm", .scale = 0.057f, .offset = {{0, -11, 0}}, .has_skeleton = false},
    {.path = "rom:/paintball/char.t3dm", .scale = 0.175f, .offset = {{0, -43, 0}}, .has_skeleton = false},
    {.path = "rom:/riistahillo/controller.t3dm", .scale = 0.3f, .offset = {{0, -18, 0}}, .rotation = {{0, 90, 0}}, .has_skeleton = false},
    {.path = "rom:/snowmen/p1.t3dm", .scale = 0.0038f, .offset = {{0, -35, 0}}, .has_skeleton = true},
};

static model_data *cur_model;

static bool controller_isstart()
{
    for (int i=0; i<MAXPLAYERS; i++)
        if (joypad_get_buttons_pressed(i).start)
            return true;
    return false;
}

void titlescreen_init()
{
    global_sprite_n64brew = sprite_load("rom:/n64brew.ia8.sprite");
    global_sprite_jam = sprite_load("rom:/jam.rgba32.sprite");
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE);
    global_font = rdpq_font_load("rom:/squarewave_l.font64");
    rdpq_text_register_font(1, global_font);
    wav64_open(&sfx_title_confirm, "rom:/core/title_confirm.wav64");
    xm64player_open(&global_music, "rom:/core/Menus.xm64");
    xm64player_seek(&global_music, 0, 0, 0);
    xm64player_set_vol(&global_music, 1.0f);
    xm64player_play(&global_music, 0);

    int model_count = sizeof(models)/sizeof(models[0]);
    cur_model = &models[rand()%model_count];
    //cur_model = &models[model_count-3];

    model = t3d_model_load(cur_model->path);
    if (cur_model->has_skeleton) {
        skeleton = t3d_skeleton_create(model);
    }

    global_fade = FADE_DURATION;
    global_is_fading = false;

    t3d_init((T3DInitParams){});
    viewport = t3d_viewport_create();

    mtx = malloc_uncached(sizeof(T3DMat4FP));
    mtx2 = malloc_uncached(sizeof(T3DMat4FP));

    rspq_block_begin();
    if (cur_model->has_skeleton) {
        t3d_model_draw_skinned(model, &skeleton);
    } else {
        t3d_model_draw(model);
    }
    model_block = rspq_block_end();

    color_t colors[] = {
        PLAYERCOLOR_1,
        PLAYERCOLOR_2,
        PLAYERCOLOR_3,
        PLAYERCOLOR_4
    };
    color = colors[rand()%MAXPLAYERS];
}

void titlescreen_loop(float deltatime)
{
    surface_t* disp;
    
    if (controller_isstart() & !global_is_fading) {
        global_is_fading = true;
        wav64_play(&sfx_title_confirm, 30);
    }

    if (global_is_fading) {
        global_fade -= deltatime;
        xm64player_set_vol(&global_music, global_fade/FADE_DURATION);
        if  (global_fade < 0.0f) {
            core_level_changeto(LEVEL_GAMESETUP);
        }
    }

    global_flash += deltatime;

    if (sync) rspq_syncpoint_wait(sync);
    
    if (cur_model->has_skeleton) {
        t3d_skeleton_update(&skeleton);
    }
    t3d_mat4fp_from_srt_euler(mtx,
        (float[3]){cur_model->scale, cur_model->scale, cur_model->scale},
        (float[3]){0, 0 + global_flash, 0},
        cur_model->offset.v
    );
    t3d_mat4fp_from_srt_euler(mtx2,
        (float[3]){1, 1, 1},
        (float[3]){cur_model->rotation.x, cur_model->rotation.y, cur_model->rotation.z},
        (float[3]){0, 0, 0}
    );

    // Get a framebuffer
    disp = display_get();
    rdpq_attach(disp, display_get_zbuf());

    rdpq_clear(RGBA32(200, 200, 0, 255));
    rdpq_clear_z(ZBUF_MAX);

    T3DVec3 camPos = {{0, 0.0f, 50.0f}};
    T3DVec3 camTarget = {{0, 0.0f, 0}};
    uint8_t colorAmbient[4] = {0xff, 0xff, 0xff, 0xFF};

    rdpq_set_prim_color(color);

    t3d_frame_start();
    t3d_viewport_set_projection(&viewport, T3D_DEG_TO_RAD(85.0f), 20.0f, 160.0f);
    t3d_viewport_look_at(&viewport, &camPos, &camTarget, &(T3DVec3){{0,1,0}});
    t3d_viewport_attach(&viewport);
    t3d_light_set_ambient(colorAmbient);
    t3d_light_set_count(0);
    t3d_matrix_push(mtx);
    t3d_matrix_push(mtx2);
    rspq_block_run(model_block);
    sync = rspq_syncpoint_new();
    t3d_matrix_pop(1);
    t3d_matrix_pop(1);

    rdpq_sync_pipe();

    rdpq_set_mode_standard();
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_combiner(RDPQ_COMBINER1((PRIM,ZERO,TEX0,ZERO), (0,0,0,TEX0)));
    rdpq_set_prim_color(RGBA32(242,209,155,0xFF));
    rdpq_sprite_blit(global_sprite_n64brew, 45, 40, NULL);

    rdpq_set_mode_standard();
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_filter(FILTER_BILINEAR);
    rdpq_sprite_blit(global_sprite_jam, 45+170+21, 30+31, &(rdpq_blitparms_t) {
        .cx = 21,
        .cy = 31,
        .theta = fm_sinf(global_flash*4) * 0.2f
    });

    if (((int)global_flash % 2) == 0)
        rdpq_text_print(&(rdpq_textparms_t){.width=320, .align=ALIGN_CENTER}, 1, 0, 240/2+64, "Press START");

    drawfade(global_fade);

    rdpq_detach_show();
}

void titlescreen_cleanup()
{
    rdpq_text_unregister_font(1);
    rdpq_font_free(global_font);
    display_close();
    sprite_free(global_sprite_jam);
    sprite_free(global_sprite_n64brew);
    wav64_close(&sfx_title_confirm);
    xm64player_stop(&global_music);
    xm64player_close(&global_music);
    rspq_block_free(model_block);
    free_uncached(mtx2);
    if (cur_model->has_skeleton) {
        t3d_skeleton_destroy(&skeleton);
    }
    t3d_model_free(model);
    t3d_destroy();
}
