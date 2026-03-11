# Runnable-by-default profile.
# Set VIA_ENABLE=yes in your build command/environment to enable VIA.
VIA_ENABLE = no
VIAL_ENABLE = no
DYNAMIC_KEYMAP_ENABLE = no

# VIA requires persistent storage for dynamic keymaps/macros.
ifeq ($(VIA_ENABLE),yes)
EEPROM_DRIVER = wear_leveling
WEAR_LEVELING_DRIVER = embedded_flash
VIAL_ENABLE = no
DYNAMIC_KEYMAP_ENABLE = yes
endif