#include "../nfc_magic_app_i.h"
#include "../lib/magic/protocols/gen4/gen4_poller_i.h"

enum {
    NfcMagicSceneGen4SetShadowModeStateCardSearch,
    NfcMagicSceneGen4SetShadowModeStateCardFound,
};

NfcCommand
    nfc_magic_scene_gen4_set_shd_mode_poller_callback(Gen4PollerEvent event, void* context) {
    NfcMagicApp* instance = context;
    furi_assert(event.data);

    NfcCommand command = NfcCommandContinue;
    if(event.type == Gen4PollerEventTypeCardDetected) {
        view_dispatcher_send_custom_event(
            instance->view_dispatcher, NfcMagicCustomEventCardDetected);
    } else if(event.type == Gen4PollerEventTypeRequestMode) {
        event.data->request_mode.mode = Gen4PollerModeSetShadowMode;
    } else if(event.type == Gen4PollerEventTypeSuccess) {
        view_dispatcher_send_custom_event(
            instance->view_dispatcher, NfcMagicCustomEventWorkerSuccess);
        command = NfcCommandStop;
    } else if(event.type == Gen4PollerEventTypeFail) {
        view_dispatcher_send_custom_event(
            instance->view_dispatcher, NfcMagicCustomEventWorkerFail);
        command = NfcCommandStop;
    }

    return command;
}

static void nfc_magic_scene_gen4_set_shd_mode_setup_view(NfcMagicApp* instance) {
    Popup* popup = instance->popup;
    popup_reset(popup);
    uint32_t state =
        scene_manager_get_scene_state(instance->scene_manager, NfcMagicSceneGen4SetShdMode);

    if(state == NfcMagicSceneGen4SetShadowModeStateCardSearch) {
        popup_set_icon(instance->popup, 0, 8, &I_NFC_manual_60x50);
        popup_set_text(
            instance->popup, "Apply the\ncard\nto the back", 128, 32, AlignRight, AlignCenter);
    } else {
        popup_set_icon(popup, 12, 23, &I_Loading_24);
        popup_set_header(popup, "Writing\nDon't move...", 52, 32, AlignLeft, AlignCenter);
    }

    view_dispatcher_switch_to_view(instance->view_dispatcher, NfcMagicAppViewPopup);
}

void nfc_magic_scene_gen4_set_shd_mode_on_enter(void* context) {
    NfcMagicApp* instance = context;

    uint8_t shadow_mode =
        scene_manager_get_scene_state(instance->scene_manager, NfcMagicSceneGen4SetShdMode);

    scene_manager_set_scene_state(
        instance->scene_manager,
        NfcMagicSceneGen4SetShdMode,
        NfcMagicSceneGen4SetShadowModeStateCardSearch);
    nfc_magic_scene_gen4_set_shd_mode_setup_view(instance);

    nfc_magic_app_blink_start(instance);

    instance->gen4_poller = gen4_poller_alloc(instance->nfc);
    gen4_poller_set_password(instance->gen4_poller, instance->gen4_password);
    instance->gen4_poller->shadow_mode = shadow_mode;

    gen4_poller_start(
        instance->gen4_poller, nfc_magic_scene_gen4_set_shd_mode_poller_callback, instance);
}

bool nfc_magic_scene_gen4_set_shd_mode_on_event(void* context, SceneManagerEvent event) {
    NfcMagicApp* instance = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == NfcMagicCustomEventCardDetected) {
            scene_manager_set_scene_state(
                instance->scene_manager,
                NfcMagicSceneGen4SetShdMode,
                NfcMagicSceneGen4SetShadowModeStateCardFound);
            nfc_magic_scene_gen4_set_shd_mode_setup_view(instance);
            consumed = true;
        } else if(event.event == NfcMagicCustomEventCardLost) {
            scene_manager_set_scene_state(
                instance->scene_manager,
                NfcMagicSceneGen4SetShdMode,
                NfcMagicSceneGen4SetShadowModeStateCardSearch);
            nfc_magic_scene_gen4_set_shd_mode_setup_view(instance);
            consumed = true;
        } else if(event.event == NfcMagicCustomEventWorkerSuccess) {
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneSuccess);
            consumed = true;
        } else if(event.event == NfcMagicCustomEventWorkerFail) {
            scene_manager_next_scene(instance->scene_manager, NfcMagicSceneGen4Fail);
            consumed = true;
        }
    }

    return consumed;
}

void nfc_magic_scene_gen4_set_shd_mode_on_exit(void* context) {
    NfcMagicApp* instance = context;

    gen4_poller_stop(instance->gen4_poller);
    gen4_poller_free(instance->gen4_poller);
    scene_manager_set_scene_state(
        instance->scene_manager,
        NfcMagicSceneGen4SetShdMode,
        NfcMagicSceneGen4SetShadowModeStateCardSearch);
    // Clear view
    popup_reset(instance->popup);

    nfc_magic_app_blink_stop(instance);
}
