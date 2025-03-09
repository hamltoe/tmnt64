#ifndef GAMEJAM2024_SAVESTATE_H
#define GAMEJAM2024_SAVESTATE_H

    /*==============================
        savestate_initialize
        Initialize the savestate system and return whether EEPROM exists
        @return Whether EEPROM is present
    ==============================*/
    extern bool savestate_initialize();
    
    /*==============================
        savestate_checkcrashed
        Check if the game recently crashed
        @return Whether the game recently crashed
    ==============================*/
    extern bool savestate_checkcrashed();
    
    /*==============================
        savestate_save
        Save the current game state to EEPROM
    ==============================*/
    extern void savestate_save(bool configonly);
    
    /*==============================
        savestate_load
        Load the game state saved in EEPROM
    ==============================*/
    extern void savestate_load();
    
    /*==============================
        savestate_clear
        Clear the game state saved in EEPROM
    ==============================*/
    extern void savestate_clear();

    extern void savestate_setblacklist(bool* list);
    extern void savestate_getblacklist(bool* list);

    extern void loadsave_init();
    extern void loadsave_loop(float deltatime);
    extern void loadsave_cleanup();

#endif