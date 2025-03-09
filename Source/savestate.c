/***************************************************************
                          savestate.c
                               
The file contains a savestate system so that the game can be
loaded if it crashes
***************************************************************/

#include <libdragon.h>
#include "core.h"
#include "minigame.h"
#include "results.h"
#include "savestate.h"


/*********************************
            Structures
*********************************/

typedef struct {
    char header[4];
    uint32_t blacklist;
    uint8_t crashedflag;
    uint8_t aidiff;
    uint8_t pointstowin;
    uint8_t nextplaystyle;
    uint8_t playerconts[MAXPLAYERS];
    uint8_t points[MAXPLAYERS];
    uint8_t chooser;
    uint8_t curgame;
    uint8_t checksum;
} GameSave;


/*********************************
       Function Prototypes
*********************************/

static bool controller_isleft();
static bool controller_isright();
static bool controller_isa();


/*********************************
             Globals
*********************************/

static int global_selection;
static uint8_t global_cansave;
static GameSave global_gamesave;
static rdpq_font_t* global_font;


/*==============================
    calc_checksum
    Calculate a basic checksum for the save state
    This is done by just adding all the bytes together
    @return The checksum
==============================*/

static uint8_t calc_checksum()
{
    uint8_t checksum = 0;
    uint8_t* asarray = (uint8_t*)&global_gamesave;
    for (int i=0; i<sizeof(GameSave); i++)
        checksum += asarray[i];
    return checksum;
}


/*==============================
    savestate_test
    Test that EEPROM is present to save a game state to
    @return Whether EEPROM is present
==============================*/

bool savestate_initialize()
{
    global_cansave = 0;
    
    // Test for EEPROM
    if (eeprom_present() == EEPROM_NONE)
        return false;
    global_cansave = 1;
        
    // Read the savestate from EEPROM
    eeprom_read_bytes((uint8_t*)(&global_gamesave), 0, sizeof(GameSave));
   
    // If the EEPROM hasn't been initialized before, do so now
    if (strncmp(global_gamesave.header, "NBGJ", 4) != 0)
    {
        memset(&global_gamesave, 0, sizeof(GameSave));
        global_gamesave.header[0] = 'N';
        global_gamesave.header[1] = 'B';
        global_gamesave.header[2] = 'G';
        global_gamesave.header[3] = 'J';
        global_gamesave.checksum = calc_checksum();
    }
    
    // Success
    return true;
}


/*==============================
    savestate_checkcrashed
    Check if the game recently crashed
    @return Whether the game recently crashed
==============================*/

bool savestate_checkcrashed()
{
    return global_gamesave.crashedflag;
}


/*==============================
    savestate_save
    Save the current game state to EEPROM
    @param Whether to only save the game config as opposed to game state
==============================*/

void savestate_save(bool configonly)
{
    if (!global_cansave)
        return;
    
    // Grab the game state
    if (!configonly)
    {
        bool playerconts[MAXPLAYERS];
        global_gamesave.crashedflag = 1;
        core_get_playerconts(playerconts);
        for (int i=0; i<MAXPLAYERS; i++)
            global_gamesave.playerconts[i] = playerconts[i];
        global_gamesave.aidiff = core_get_aidifficulty();
        global_gamesave.pointstowin = results_get_points_to_win();
        for (int i=0; i<MAXPLAYERS; i++)
            global_gamesave.points[i] = results_get_points(PLAYER_1+i);
        global_gamesave.nextplaystyle = core_get_nextround();
        global_gamesave.chooser = core_get_curchooser();
        global_gamesave.curgame = minigame_get_index();
        global_gamesave.checksum = calc_checksum();
    }
    
    // Save to EEPROM
    eeprom_write_bytes((uint8_t*)(&global_gamesave), 0, sizeof(GameSave));
}


/*==============================
    savestate_load
    Load the game state saved in EEPROM
==============================*/

void savestate_load()
{
    bool playerconts[MAXPLAYERS];

    if (!global_cansave)
        return;
        
    // Recover the game state
    for (int i=0; i<MAXPLAYERS; i++)
        playerconts[i] = global_gamesave.playerconts[i];
    core_set_playercount(playerconts);
    core_set_aidifficulty(global_gamesave.aidiff);
    results_set_points_to_win(global_gamesave.pointstowin);
    results_set_points(PLAYER_1, global_gamesave.points[0]);
    results_set_points(PLAYER_2, global_gamesave.points[1]);
    results_set_points(PLAYER_3, global_gamesave.points[2]);
    results_set_points(PLAYER_4, global_gamesave.points[3]);
    core_set_nextround(global_gamesave.nextplaystyle);
    core_set_curchooser(global_gamesave.chooser);
    minigame_loadnext(global_minigame_list[global_gamesave.curgame].internalname);
}


/*==============================
    savestate_clear
    Clear the game state saved in EEPROM
==============================*/

void savestate_clear()
{
    if (!global_cansave)
        return;
    global_gamesave.crashedflag = 0;
    eeprom_write_bytes((uint8_t*)(&global_gamesave), 0, sizeof(GameSave));
}

void savestate_setblacklist(bool* list)
{
    uint32_t bitfield = 0;
    for (int i=0; i<global_minigame_count; i++)
        bitfield |= (list[i] & 0x01) << i;
    global_gamesave.blacklist = bitfield;
}

void savestate_getblacklist(bool* list)
{
    for (int i=0; i<global_minigame_count; i++)
        list[i] = (global_gamesave.blacklist >> i) & 0x01;
}


/*=============================================================

=============================================================*/

void loadsave_init()
{
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE);
    global_font = rdpq_font_load("rom:/squarewave_l.font64");
    rdpq_text_register_font(1, global_font);
    rdpq_font_style(global_font, 1, &(rdpq_fontstyle_t){.color = RGBA32(255, 255, 255, 255)});
    rdpq_font_style(global_font, 2, &(rdpq_fontstyle_t){.color = RGBA32(148, 145, 8, 255)});
    global_selection = 0;
    if (!savestate_checkcrashed() && global_cansave)
        core_level_changeto(LEVEL_MAINMENU);
}

void loadsave_loop(float deltatime)
{
    surface_t* disp;
    int maxselection = 2;

    if (!savestate_checkcrashed() && global_cansave)
        return;

    if (!global_cansave)
        maxselection = 1;

    // Handle controls
    if (controller_isleft())
    {
        global_selection++;
        if (global_selection > maxselection-1)
            global_selection = 0;
    }
    else if (controller_isright())
    {
        global_selection--;
        if (global_selection < 0)
            global_selection = maxselection-1;
    }
    else if (controller_isa())
    {
        if (savestate_checkcrashed() && global_selection == 0)
        {
            savestate_load();
            core_level_changeto(LEVEL_MINIGAME);
        }
        else
        {
            if (savestate_checkcrashed())
                savestate_clear();
            core_level_changeto(LEVEL_MAINMENU);
        }
    }

    // Get a framebuffer
    disp = display_get();
    rdpq_attach(disp, NULL);

    // Black background
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_set_prim_color(RGBA32(0, 0, 0, 255));
    rdpq_fill_rectangle(0, 0, 320, 240);

    // Render text
    if (savestate_checkcrashed())
    {
        rdpq_text_print(&(rdpq_textparms_t){.width=320, .align=ALIGN_CENTER, .style_id=1}, 1, 0, 240/2-32, "A crash was detected.\nWould you like to restore the save?");
        rdpq_text_print(&(rdpq_textparms_t){.style_id=((global_selection == 0) ? 2 : 1)}, 1, 320/2-64, 240/2+32, "Yes");
        rdpq_text_print(&(rdpq_textparms_t){.style_id=((global_selection == 1) ? 2 : 1)}, 1, 320/2+64, 240/2+32, "No");
    }
    else
    {
        rdpq_text_print(&(rdpq_textparms_t){.width=320, .align=ALIGN_CENTER, .style_id=1}, 1, 0, 240/2-48, "EEPROM save was not detected.\n\nIf the game crashes, you will\nnot be able to restore it.");
        rdpq_text_print(&(rdpq_textparms_t){.width=320, .align=ALIGN_CENTER, .style_id=((global_selection == 0) ? 2 : 1)}, 1, 0, 240/2+64, "Ok");
    }

    // Done
    rdpq_detach_show();
}

void loadsave_cleanup()
{
    rdpq_text_unregister_font(1);
    rdpq_font_free(global_font);
    display_close();
}

static bool controller_isleft()
{
    for (int i=0; i<MAXPLAYERS; i++)
    {
        joypad_inputs_t stick = joypad_get_inputs(i);
        if (joypad_get_buttons_pressed(i).c_left || joypad_get_buttons_pressed(i).d_left || (joypad_get_axis_pressed(i, JOYPAD_AXIS_STICK_X) == -1 && stick.stick_x < -20))
            return true;
    }
    return false;
}

static bool controller_isright()
{
    for (int i=0; i<MAXPLAYERS; i++)
    {
        joypad_inputs_t stick = joypad_get_inputs(i);
        if (joypad_get_buttons_pressed(i).c_right || joypad_get_buttons_pressed(i).d_right || (joypad_get_axis_pressed(i, JOYPAD_AXIS_STICK_X) == 1 && stick.stick_x > 20))
            return true;
    }
    return false;
}

static bool controller_isa()
{
    for (int i=0; i<MAXPLAYERS; i++)
        if (joypad_get_buttons_pressed(i).a)
            return true;
    return false;
}