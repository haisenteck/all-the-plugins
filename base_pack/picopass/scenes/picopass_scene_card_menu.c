#include "../picopass_i.h"

enum SubmenuIndex {
    SubmenuIndexSave,
    SubmenuIndexSaveAsLF,
    SubmenuIndexSaveAsSeader,
    SubmenuIndexChangeKey,
    SubmenuIndexWrite,
    SubmenuIndexEmulate,
    SubmenuIndexSavePartial,
};

void picopass_scene_card_menu_submenu_callback(void* context, uint32_t index) {
    Picopass* picopass = context;

    view_dispatcher_send_custom_event(picopass->view_dispatcher, index);
}

void picopass_scene_card_menu_on_enter(void* context) {
    Picopass* picopass = context;
    Submenu* submenu = picopass->submenu;
    PicopassPacs* pacs = &picopass->dev->dev_data.pacs;
    PicopassBlock* card_data = picopass->dev->dev_data.card_data;

    bool sio = 0x30 == card_data[PICOPASS_ICLASS_PACS_CFG_BLOCK_INDEX].data[0];
    bool secured = (card_data[PICOPASS_CONFIG_BLOCK_INDEX].data[7] & PICOPASS_FUSE_CRYPT10) !=
                   PICOPASS_FUSE_CRYPT0;
    bool zero_config = picopass_is_memset(
        card_data[PICOPASS_ICLASS_PACS_CFG_BLOCK_INDEX].data, 0x00, PICOPASS_BLOCK_LEN);
    bool no_credential = picopass_is_memset(pacs->credential, 0x00, sizeof(pacs->credential));
    bool no_key =
        picopass_is_memset(card_data[PICOPASS_SECURE_KD_BLOCK_INDEX].data, 0xFF, PICOPASS_BLOCK_LEN);

    if(secured && zero_config) {
        submenu_add_item(
            submenu,
            "Save Partial",
            SubmenuIndexSavePartial,
            picopass_scene_card_menu_submenu_callback,
            picopass);
    } else {
        submenu_add_item(
            submenu, "Save", SubmenuIndexSave, picopass_scene_card_menu_submenu_callback, picopass);
    }

    if(secured && (sio || pacs->sio)) {
        submenu_add_item(
            submenu,
            "Save in Seader fmt",
            SubmenuIndexSaveAsSeader,
            picopass_scene_card_menu_submenu_callback,
            picopass);
    }

    if(secured && !no_credential) {
        submenu_add_item(
            submenu,
            "Save as LFRFID",
            SubmenuIndexSaveAsLF,
            picopass_scene_card_menu_submenu_callback,
            picopass);
    }

    if(!zero_config && !no_key) {
        submenu_add_item(
            submenu,
            "Write",
            SubmenuIndexWrite,
            picopass_scene_card_menu_submenu_callback,
            picopass);
        submenu_add_item(
            submenu,
            "Emulate",
            SubmenuIndexEmulate,
            picopass_scene_card_menu_submenu_callback,
            picopass);
        if(secured) {
            submenu_add_item(
                submenu,
                "Change Key",
                SubmenuIndexChangeKey,
                picopass_scene_card_menu_submenu_callback,
                picopass);
        }
    }

    submenu_set_selected_item(
        picopass->submenu,
        scene_manager_get_scene_state(picopass->scene_manager, PicopassSceneCardMenu));

    view_dispatcher_switch_to_view(picopass->view_dispatcher, PicopassViewMenu);
}

bool picopass_scene_card_menu_on_event(void* context, SceneManagerEvent event) {
    Picopass* picopass = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == SubmenuIndexSave) {
            scene_manager_set_scene_state(
                picopass->scene_manager, PicopassSceneCardMenu, SubmenuIndexSave);
            scene_manager_next_scene(picopass->scene_manager, PicopassSceneSaveName);
            picopass->dev->format = PicopassDeviceSaveFormatHF;
            consumed = true;
        } else if(event.event == SubmenuIndexSavePartial) {
            scene_manager_set_scene_state(
                picopass->scene_manager, PicopassSceneCardMenu, SubmenuIndexSave);
            scene_manager_next_scene(picopass->scene_manager, PicopassSceneSaveName);
            picopass->dev->format = PicopassDeviceSaveFormatPartial;
            consumed = true;
        } else if(event.event == SubmenuIndexSaveAsSeader) {
            scene_manager_set_scene_state(
                picopass->scene_manager, PicopassSceneCardMenu, event.event);
            scene_manager_next_scene(picopass->scene_manager, PicopassSceneSaveName);
            picopass->dev->format = PicopassDeviceSaveFormatSeader;
            consumed = true;
        } else if(event.event == SubmenuIndexSaveAsLF) {
            scene_manager_set_scene_state(
                picopass->scene_manager, PicopassSceneCardMenu, SubmenuIndexSaveAsLF);
            picopass->dev->format = PicopassDeviceSaveFormatLF;
            scene_manager_next_scene(picopass->scene_manager, PicopassSceneSaveName);
            consumed = true;
        } else if(event.event == SubmenuIndexWrite) {
            scene_manager_next_scene(picopass->scene_manager, PicopassSceneWriteCard);
            consumed = true;
        } else if(event.event == SubmenuIndexEmulate) {
            scene_manager_next_scene(picopass->scene_manager, PicopassSceneEmulate);
            consumed = true;
        } else if(event.event == SubmenuIndexChangeKey) {
            scene_manager_set_scene_state(
                picopass->scene_manager, PicopassSceneCardMenu, SubmenuIndexChangeKey);
            scene_manager_next_scene(picopass->scene_manager, PicopassSceneKeyMenu);
            consumed = true;
        }
    } else if(event.type == SceneManagerEventTypeBack) {
        consumed = scene_manager_search_and_switch_to_previous_scene(
            picopass->scene_manager, PicopassSceneStart);
    }

    return consumed;
}

void picopass_scene_card_menu_on_exit(void* context) {
    Picopass* picopass = context;

    submenu_reset(picopass->submenu);
}
