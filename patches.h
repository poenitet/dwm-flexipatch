/*
 * This file contains patch control flags.
 *
 * In principle you should be able to mix and match any patches
 * you may want. In cases where patches are logically incompatible
 * one patch may take precedence over the other as noted in the
 * relevant descriptions.
 */

/* The alpha patch adds transparency for the status bar.
 * https://dwm.suckless.org/patches/alpha/
 */
#define ALPHA_PATCH 0

/* The systray patch adds systray for the status bar.
 * https://dwm.suckless.org/patches/systray/
 */
#define SYSTRAY_PATCH 0

/* The pertag patch adds nmaster, mfacts and layouts per tag rather
 * than per monitor (default).
 * https://dwm.suckless.org/patches/pertag/
 */
#define PERTAG_PATCH 0

/* This controls whether or not to also store bar position on a per
 * tag basis, or leave it as one bar per monitor.
 */
#define PERTAGBAR_PATCH 0