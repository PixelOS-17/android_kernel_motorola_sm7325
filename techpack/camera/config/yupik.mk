# SPDX-License-Identifier: GPL-2.0-only

# Localized KCONFIG settings, as in yupikcamera.conf. The other blocks it
# enables are already part of lahaina.mk.
CONFIG_SPECTRA_LRME := y

# Flags to pass into C preprocessor
ccflags-y += -DCONFIG_SPECTRA_LRME=1
