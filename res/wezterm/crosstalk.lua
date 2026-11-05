local wezterm = require 'wezterm'
local act = wezterm.action
local config = wezterm.config_builder()

config.front_end = 'Software'

config.font_dirs = { wezterm.config_dir .. '/../fonts' }
config.font = wezterm.font 'Anonymous Pro'
config.font_size = 14.0
config.window_padding = { left = 0, right = 0, top = 0, bottom = 0 }
config.enable_scroll_bar = false
config.hide_tab_bar_if_only_one_tab = true
config.audible_bell = 'Disabled'
config.window_close_confirmation = 'NeverPrompt'
config.colors = { background = '#0A0A14' }

config.enable_kitty_keyboard = false
config.allow_win32_input_mode = true

config.disable_default_key_bindings = true
config.keys = {
    {
        key = 'c',
        mods = 'CTRL',
        action = wezterm.action_callback(function(window, pane)
            local sel = window:get_selection_text_for_pane(pane)
            if sel ~= nil and sel ~= '' then
                window:perform_action(act.CopyTo 'Clipboard', pane)
                window:perform_action(act.ClearSelection, pane)
            else
                window:perform_action(act.SendKey { key = 'c', mods = 'CTRL' }, pane)
            end
        end),
    },
    { key = 'v', mods = 'CTRL', action = act.PasteFrom 'Clipboard' },
}

wezterm.on('gui-startup', function(cmd)
    local _, _, window = wezterm.mux.spawn_window(cmd or {})
    window:gui_window():maximize()
end)

return config
