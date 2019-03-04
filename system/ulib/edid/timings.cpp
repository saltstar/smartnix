
#include "lib/edid/timings.h"

namespace edid {
namespace internal {

#define TIMING_PARAMS(pf, ha, hf, hs, hb, va, vf, vs, vb, hp, vp, i, avb, dclk, vr) \
    { \
        .pixel_freq_10khz = pf, \
        .horizontal_addressable = ha, \
        .horizontal_front_porch = hf, \
        .horizontal_sync_pulse = hs, \
        .horizontal_blanking = hb, \
        .vertical_addressable = va, \
        .vertical_front_porch = vf, \
        .vertical_sync_pulse = vs, \
        .vertical_blanking = vb, \
        .flags = (hp ? timing_params::kPositiveHsync : 0) \
                | (vp ? timing_params::kPositiveVsync : 0) \
                | (i ? timing_params::kInterlaced : 0) \
                | (avb ? timing_params::kAlternatingVblank : 0) \
                | (dclk ? timing_params::kDoubleClocked : 0), \
        .vertical_refresh_e2 = vr, \
    }

// Timings taken from DMT v1.11
// TODO: Handle reduced blanking
const timing_params_t dmt_timings_arr[] = {
    TIMING_PARAMS(3150, 640, 32, 64, 192, 350, 32, 3, 95, 1, 0, 0, 0, 0, 8508), /* 01h */
    TIMING_PARAMS(3150, 640, 32, 64, 192, 400, 1, 3, 45, 0, 1, 0, 0, 0, 8508), /* 02h */
    TIMING_PARAMS(3550, 720, 36, 72, 216, 400, 1, 3, 46, 0, 1, 0, 0, 0, 8504), /* 03h */
    TIMING_PARAMS(3550, 720, 36, 72, 216, 400, 1, 3, 46, 0, 1, 0, 0, 0, 8504), /* 03h */
    TIMING_PARAMS(2518, 640, 8, 96, 144, 480, 2, 2, 29, 0, 0, 0, 0, 0, 5994), /* 04h */
    TIMING_PARAMS(2518, 640, 8, 96, 144, 480, 2, 2, 29, 0, 0, 0, 0, 0, 5994), /* 04h */
    TIMING_PARAMS(3150, 640, 16, 40, 176, 480, 1, 3, 24, 0, 0, 0, 0, 0, 7281), /* 05h */
    TIMING_PARAMS(3150, 640, 16, 40, 176, 480, 1, 3, 24, 0, 0, 0, 0, 0, 7281), /* 05h */
    TIMING_PARAMS(3150, 640, 16, 64, 200, 480, 1, 3, 20, 0, 0, 0, 0, 0, 7500), /* 06h */
    TIMING_PARAMS(3150, 640, 16, 64, 200, 480, 1, 3, 20, 0, 0, 0, 0, 0, 7500), /* 06h */
    TIMING_PARAMS(3600, 640, 56, 56, 192, 480, 1, 3, 29, 0, 0, 0, 0, 0, 8501), /* 07h */
    TIMING_PARAMS(3600, 640, 56, 56, 192, 480, 1, 3, 29, 0, 0, 0, 0, 0, 8501), /* 07h */
    TIMING_PARAMS(3600, 800, 24, 72, 224, 600, 1, 2, 25, 1, 1, 0, 0, 0, 5625), /* 08h */
    TIMING_PARAMS(3600, 800, 24, 72, 224, 600, 1, 2, 25, 1, 1, 0, 0, 0, 5625), /* 08h */
    TIMING_PARAMS(4000, 800, 40, 128, 256, 600, 1, 4, 28, 1, 1, 0, 0, 0, 6032), /* 09h */
    TIMING_PARAMS(4000, 800, 40, 128, 256, 600, 1, 4, 28, 1, 1, 0, 0, 0, 6032), /* 09h */
    TIMING_PARAMS(5000, 800, 56, 120, 240, 600, 37, 6, 66, 1, 1, 0, 0, 0, 7219), /* 0Ah */
    TIMING_PARAMS(5000, 800, 56, 120, 240, 600, 37, 6, 66, 1, 1, 0, 0, 0, 7219), /* 0Ah */
    TIMING_PARAMS(4950, 800, 16, 80, 256, 600, 1, 3, 25, 1, 1, 0, 0, 0, 7500), /* 0Bh */
    TIMING_PARAMS(4950, 800, 16, 80, 256, 600, 1, 3, 25, 1, 1, 0, 0, 0, 7500), /* 0Bh */
    TIMING_PARAMS(5625, 800, 32, 64, 248, 600, 1, 3, 31, 1, 1, 0, 0, 0, 8506), /* 0Ch */
    TIMING_PARAMS(5625, 800, 32, 64, 248, 600, 1, 3, 31, 1, 1, 0, 0, 0, 8506), /* 0Ch */
    TIMING_PARAMS(7325, 800, 48, 32, 160, 600, 3, 4, 36, 1, 0, 0, 0, 0, 11997), /* 0Dh */
    TIMING_PARAMS(7325, 800, 48, 32, 160, 600, 3, 4, 36, 1, 0, 0, 0, 0, 11997), /* 0Dh */
    TIMING_PARAMS(3375, 848, 16, 112, 240, 480, 6, 8, 37, 1, 1, 0, 0, 0, 6000), /* 0Eh */
    TIMING_PARAMS(3375, 848, 16, 112, 240, 480, 6, 8, 37, 1, 1, 0, 0, 0, 6000), /* 0Eh */
    TIMING_PARAMS(4490, 1024, 8, 176, 240, 768, 0, 4, 24, 1, 1, 1, 0, 0, 8696), /* 0Fh */
    TIMING_PARAMS(4490, 1024, 8, 176, 240, 768, 0, 4, 24, 1, 1, 1, 0, 0, 8696), /* 0Fh */
    TIMING_PARAMS(6500, 1024, 24, 136, 320, 768, 3, 6, 38, 0, 0, 0, 0, 0, 6000), /* 10h */
    TIMING_PARAMS(6500, 1024, 24, 136, 320, 768, 3, 6, 38, 0, 0, 0, 0, 0, 6000), /* 10h */
    TIMING_PARAMS(7500, 1024, 24, 136, 304, 768, 3, 6, 38, 0, 0, 0, 0, 0, 7006), /* 11h */
    TIMING_PARAMS(7500, 1024, 24, 136, 304, 768, 3, 6, 38, 0, 0, 0, 0, 0, 7006), /* 11h */
    TIMING_PARAMS(7875, 1024, 16, 96, 288, 768, 1, 3, 32, 1, 1, 0, 0, 0, 7503), /* 12h */
    TIMING_PARAMS(7875, 1024, 16, 96, 288, 768, 1, 3, 32, 1, 1, 0, 0, 0, 7503), /* 12h */
    TIMING_PARAMS(9450, 1024, 48, 96, 352, 768, 1, 3, 40, 1, 1, 0, 0, 0, 8500), /* 13h */
    TIMING_PARAMS(9450, 1024, 48, 96, 352, 768, 1, 3, 40, 1, 1, 0, 0, 0, 8500), /* 13h */
    TIMING_PARAMS(11550, 1024, 48, 32, 160, 768, 3, 4, 45, 1, 0, 0, 0, 0, 11999), /* 14h */
    TIMING_PARAMS(11550, 1024, 48, 32, 160, 768, 3, 4, 45, 1, 0, 0, 0, 0, 11999), /* 14h */
    TIMING_PARAMS(10800, 1152, 64, 128, 448, 864, 1, 3, 36, 1, 1, 0, 0, 0, 7500), /* 15h */
    TIMING_PARAMS(10800, 1152, 64, 128, 448, 864, 1, 3, 36, 1, 1, 0, 0, 0, 7500), /* 15h */
    TIMING_PARAMS(6825, 1280, 48, 32, 160, 768, 3, 7, 22, 1, 0, 0, 0, 0, 5999), /* 16h */
    TIMING_PARAMS(6825, 1280, 48, 32, 160, 768, 3, 7, 22, 1, 0, 0, 0, 0, 5999), /* 16h */
    TIMING_PARAMS(7950, 1280, 64, 128, 384, 768, 3, 7, 30, 0, 1, 0, 0, 0, 5987), /* 17h */
    TIMING_PARAMS(7950, 1280, 64, 128, 384, 768, 3, 7, 30, 0, 1, 0, 0, 0, 5987), /* 17h */
    TIMING_PARAMS(10225, 1280, 80, 128, 416, 768, 3, 7, 37, 0, 1, 0, 0, 0, 7489), /* 18h */
    TIMING_PARAMS(10225, 1280, 80, 128, 416, 768, 3, 7, 37, 0, 1, 0, 0, 0, 7489), /* 18h */
    TIMING_PARAMS(11750, 1280, 80, 136, 432, 768, 3, 7, 41, 0, 1, 0, 0, 0, 8484), /* 19h */
    TIMING_PARAMS(11750, 1280, 80, 136, 432, 768, 3, 7, 41, 0, 1, 0, 0, 0, 8484), /* 19h */
    TIMING_PARAMS(14025, 1280, 48, 32, 160, 768, 3, 7, 45, 1, 0, 0, 0, 0, 11980), /* 1Ah */
    TIMING_PARAMS(14025, 1280, 48, 32, 160, 768, 3, 7, 45, 1, 0, 0, 0, 0, 11980), /* 1Ah */
    TIMING_PARAMS(7100, 1280, 48, 32, 160, 800, 3, 6, 23, 1, 0, 0, 0, 0, 5991), /* 1Bh */
    TIMING_PARAMS(7100, 1280, 48, 32, 160, 800, 3, 6, 23, 1, 0, 0, 0, 0, 5991), /* 1Bh */
    TIMING_PARAMS(8350, 1280, 72, 128, 400, 800, 3, 6, 31, 0, 1, 0, 0, 0, 5981), /* 1Ch */
    TIMING_PARAMS(8350, 1280, 72, 128, 400, 800, 3, 6, 31, 0, 1, 0, 0, 0, 5981), /* 1Ch */
    TIMING_PARAMS(10650, 1280, 80, 128, 416, 800, 3, 6, 38, 0, 1, 0, 0, 0, 7493), /* 1Dh */
    TIMING_PARAMS(10650, 1280, 80, 128, 416, 800, 3, 6, 38, 0, 1, 0, 0, 0, 7493), /* 1Dh */
    TIMING_PARAMS(12250, 1280, 80, 136, 432, 800, 3, 6, 43, 0, 1, 0, 0, 0, 8488), /* 1Eh */
    TIMING_PARAMS(12250, 1280, 80, 136, 432, 800, 3, 6, 43, 0, 1, 0, 0, 0, 8488), /* 1Eh */
    TIMING_PARAMS(14625, 1280, 48, 32, 160, 800, 3, 6, 47, 1, 0, 0, 0, 0, 11991), /* 1Fh */
    TIMING_PARAMS(14625, 1280, 48, 32, 160, 800, 3, 6, 47, 1, 0, 0, 0, 0, 11991), /* 1Fh */
    TIMING_PARAMS(10800, 1280, 96, 112, 520, 960, 1, 3, 40, 1, 1, 0, 0, 0, 6000), /* 20h */
    TIMING_PARAMS(10800, 1280, 96, 112, 520, 960, 1, 3, 40, 1, 1, 0, 0, 0, 6000), /* 20h */
    TIMING_PARAMS(14850, 1280, 64, 160, 448, 960, 1, 3, 51, 1, 1, 0, 0, 0, 8500), /* 21h */
    TIMING_PARAMS(14850, 1280, 64, 160, 448, 960, 1, 3, 51, 1, 1, 0, 0, 0, 8500), /* 21h */
    TIMING_PARAMS(17550, 1280, 48, 32, 160, 960, 3, 4, 57, 1, 0, 0, 0, 0, 11984), /* 22h */
    TIMING_PARAMS(17550, 1280, 48, 32, 160, 960, 3, 4, 57, 1, 0, 0, 0, 0, 11984), /* 22h */
    TIMING_PARAMS(10800, 1280, 48, 112, 408, 1024, 1, 3, 42, 1, 1, 0, 0, 0, 6002), /* 23h */
    TIMING_PARAMS(10800, 1280, 48, 112, 408, 1024, 1, 3, 42, 1, 1, 0, 0, 0, 6002), /* 23h */
    TIMING_PARAMS(13500, 1280, 16, 144, 408, 1024, 1, 3, 42, 1, 1, 0, 0, 0, 7503), /* 24h */
    TIMING_PARAMS(13500, 1280, 16, 144, 408, 1024, 1, 3, 42, 1, 1, 0, 0, 0, 7503), /* 24h */
    TIMING_PARAMS(15750, 1280, 64, 160, 448, 1024, 1, 3, 48, 1, 1, 0, 0, 0, 8502), /* 25h */
    TIMING_PARAMS(15750, 1280, 64, 160, 448, 1024, 1, 3, 48, 1, 1, 0, 0, 0, 8502), /* 25h */
    TIMING_PARAMS(18725, 1280, 48, 32, 160, 1024, 3, 7, 60, 1, 0, 0, 0, 0, 11996), /* 26h */
    TIMING_PARAMS(18725, 1280, 48, 32, 160, 1024, 3, 7, 60, 1, 0, 0, 0, 0, 11996), /* 26h */
    TIMING_PARAMS(8550, 1360, 64, 112, 432, 768, 3, 6, 27, 1, 1, 0, 0, 0, 6002), /* 27h */
    TIMING_PARAMS(8550, 1360, 64, 112, 432, 768, 3, 6, 27, 1, 1, 0, 0, 0, 6002), /* 27h */
    TIMING_PARAMS(14825, 1360, 48, 32, 160, 768, 3, 5, 45, 1, 0, 0, 0, 0, 11997), /* 28h */
    TIMING_PARAMS(14825, 1360, 48, 32, 160, 768, 3, 5, 45, 1, 0, 0, 0, 0, 11997), /* 28h */
    TIMING_PARAMS(10100, 1400, 48, 32, 160, 1050, 3, 4, 30, 1, 0, 0, 0, 0, 5995), /* 29h */
    TIMING_PARAMS(10100, 1400, 48, 32, 160, 1050, 3, 4, 30, 1, 0, 0, 0, 0, 5995), /* 29h */
    TIMING_PARAMS(12175, 1400, 88, 144, 464, 1050, 3, 4, 39, 0, 1, 0, 0, 0, 5998), /* 2Ah */
    TIMING_PARAMS(12175, 1400, 88, 144, 464, 1050, 3, 4, 39, 0, 1, 0, 0, 0, 5998), /* 2Ah */
    TIMING_PARAMS(15600, 1400, 104, 144, 496, 1050, 3, 4, 49, 0, 1, 0, 0, 0, 7487), /* 2Bh */
    TIMING_PARAMS(15600, 1400, 104, 144, 496, 1050, 3, 4, 49, 0, 1, 0, 0, 0, 7487), /* 2Bh */
    TIMING_PARAMS(17950, 1400, 104, 152, 512, 1050, 3, 4, 55, 0, 1, 0, 0, 0, 8496), /* 2Ch */
    TIMING_PARAMS(17950, 1400, 104, 152, 512, 1050, 3, 4, 55, 0, 1, 0, 0, 0, 8496), /* 2Ch */
    TIMING_PARAMS(20800, 1400, 48, 32, 160, 1050, 3, 4, 62, 1, 0, 0, 0, 0, 11990), /* 2Dh */
    TIMING_PARAMS(20800, 1400, 48, 32, 160, 1050, 3, 4, 62, 1, 0, 0, 0, 0, 11990), /* 2Dh */
    TIMING_PARAMS(8875, 1440, 48, 32, 160, 900, 3, 6, 26, 1, 0, 0, 0, 0, 5990), /* 2Eh */
    TIMING_PARAMS(8875, 1440, 48, 32, 160, 900, 3, 6, 26, 1, 0, 0, 0, 0, 5990), /* 2Eh */
    TIMING_PARAMS(10650, 1440, 80, 152, 464, 900, 3, 6, 34, 0, 1, 0, 0, 0, 5989), /* 2Fh */
    TIMING_PARAMS(10650, 1440, 80, 152, 464, 900, 3, 6, 34, 0, 1, 0, 0, 0, 5989), /* 2Fh */
    TIMING_PARAMS(13675, 1440, 96, 152, 496, 900, 3, 6, 42, 0, 1, 0, 0, 0, 7498), /* 30h */
    TIMING_PARAMS(13675, 1440, 96, 152, 496, 900, 3, 6, 42, 0, 1, 0, 0, 0, 7498), /* 30h */
    TIMING_PARAMS(15700, 1440, 104, 152, 512, 900, 3, 6, 48, 0, 1, 0, 0, 0, 8484), /* 31h */
    TIMING_PARAMS(15700, 1440, 104, 152, 512, 900, 3, 6, 48, 0, 1, 0, 0, 0, 8484), /* 31h */
    TIMING_PARAMS(18275, 1440, 48, 32, 160, 900, 3, 6, 53, 1, 0, 0, 0, 0, 11985), /* 32h */
    TIMING_PARAMS(18275, 1440, 48, 32, 160, 900, 3, 6, 53, 1, 0, 0, 0, 0, 11985), /* 32h */
    TIMING_PARAMS(16200, 1600, 64, 192, 560, 1200, 1, 3, 50, 1, 1, 0, 0, 0, 6000), /* 33h */
    TIMING_PARAMS(16200, 1600, 64, 192, 560, 1200, 1, 3, 50, 1, 1, 0, 0, 0, 6000), /* 33h */
    TIMING_PARAMS(17550, 1600, 64, 192, 560, 1200, 1, 3, 50, 1, 1, 0, 0, 0, 6500), /* 34h */
    TIMING_PARAMS(17550, 1600, 64, 192, 560, 1200, 1, 3, 50, 1, 1, 0, 0, 0, 6500), /* 34h */
    TIMING_PARAMS(18900, 1600, 64, 192, 560, 1200, 1, 3, 50, 1, 1, 0, 0, 0, 7000), /* 35h */
    TIMING_PARAMS(18900, 1600, 64, 192, 560, 1200, 1, 3, 50, 1, 1, 0, 0, 0, 7000), /* 35h */
    TIMING_PARAMS(20250, 1600, 64, 192, 560, 1200, 1, 3, 50, 1, 1, 0, 0, 0, 7500), /* 36h */
    TIMING_PARAMS(20250, 1600, 64, 192, 560, 1200, 1, 3, 50, 1, 1, 0, 0, 0, 7500), /* 36h */
    TIMING_PARAMS(22950, 1600, 64, 192, 560, 1200, 1, 3, 50, 1, 1, 0, 0, 0, 8500), /* 37h */
    TIMING_PARAMS(22950, 1600, 64, 192, 560, 1200, 1, 3, 50, 1, 1, 0, 0, 0, 8500), /* 37h */
    TIMING_PARAMS(26825, 1600, 48, 32, 160, 1200, 3, 4, 71, 1, 0, 0, 0, 0, 11992), /* 38h */
    TIMING_PARAMS(26825, 1600, 48, 32, 160, 1200, 3, 4, 71, 1, 0, 0, 0, 0, 11992), /* 38h */
    TIMING_PARAMS(11900, 1680, 48, 32, 160, 1050, 3, 6, 30, 1, 0, 0, 0, 0, 5988), /* 39h */
    TIMING_PARAMS(11900, 1680, 48, 32, 160, 1050, 3, 6, 30, 1, 0, 0, 0, 0, 5988), /* 39h */
    TIMING_PARAMS(14625, 1680, 104, 176, 560, 1050, 3, 6, 39, 0, 1, 0, 0, 0, 5995), /* 3Ah */
    TIMING_PARAMS(14625, 1680, 104, 176, 560, 1050, 3, 6, 39, 0, 1, 0, 0, 0, 5995), /* 3Ah */
    TIMING_PARAMS(18700, 1680, 120, 176, 592, 1050, 3, 6, 49, 0, 1, 0, 0, 0, 7489), /* 3Bh */
    TIMING_PARAMS(18700, 1680, 120, 176, 592, 1050, 3, 6, 49, 0, 1, 0, 0, 0, 7489), /* 3Bh */
    TIMING_PARAMS(21475, 1680, 128, 176, 608, 1050, 3, 6, 55, 0, 1, 0, 0, 0, 8494), /* 3Ch */
    TIMING_PARAMS(21475, 1680, 128, 176, 608, 1050, 3, 6, 55, 0, 1, 0, 0, 0, 8494), /* 3Ch */
    TIMING_PARAMS(24550, 1680, 48, 32, 160, 1050, 3, 6, 62, 1, 0, 0, 0, 0, 11999), /* 3Dh */
    TIMING_PARAMS(24550, 1680, 48, 32, 160, 1050, 3, 6, 62, 1, 0, 0, 0, 0, 11999), /* 3Dh */
    TIMING_PARAMS(20475, 1792, 128, 200, 656, 1344, 1, 3, 50, 0, 1, 0, 0, 0, 6000), /* 3Eh */
    TIMING_PARAMS(20475, 1792, 128, 200, 656, 1344, 1, 3, 50, 0, 1, 0, 0, 0, 6000), /* 3Eh */
    TIMING_PARAMS(26100, 1792, 96, 216, 664, 1344, 1, 3, 73, 0, 1, 0, 0, 0, 7500), /* 3Fh */
    TIMING_PARAMS(26100, 1792, 96, 216, 664, 1344, 1, 3, 73, 0, 1, 0, 0, 0, 7500), /* 3Fh */
    TIMING_PARAMS(33325, 1792, 48, 32, 160, 1344, 3, 4, 79, 1, 0, 0, 0, 0, 11997), /* 40h */
    TIMING_PARAMS(33325, 1792, 48, 32, 160, 1344, 3, 4, 79, 1, 0, 0, 0, 0, 11997), /* 40h */
    TIMING_PARAMS(21825, 1856, 96, 224, 672, 1392, 1, 3, 47, 0, 1, 0, 0, 0, 5999), /* 41h */
    TIMING_PARAMS(21825, 1856, 96, 224, 672, 1392, 1, 3, 47, 0, 1, 0, 0, 0, 5999), /* 41h */
    TIMING_PARAMS(28800, 1856, 128, 224, 704, 1392, 1, 3, 108, 0, 1, 0, 0, 0, 7500), /* 42h */
    TIMING_PARAMS(28800, 1856, 128, 224, 704, 1392, 1, 3, 108, 0, 1, 0, 0, 0, 7500), /* 42h */
    TIMING_PARAMS(35650, 1856, 48, 32, 160, 1392, 3, 4, 82, 1, 0, 0, 0, 0, 11997), /* 43h */
    TIMING_PARAMS(35650, 1856, 48, 32, 160, 1392, 3, 4, 82, 1, 0, 0, 0, 0, 11997), /* 43h */
    TIMING_PARAMS(15400, 1920, 48, 32, 160, 1200, 3, 6, 35, 1, 0, 0, 0, 0, 5995), /* 44h */
    TIMING_PARAMS(15400, 1920, 48, 32, 160, 1200, 3, 6, 35, 1, 0, 0, 0, 0, 5995), /* 44h */
    TIMING_PARAMS(19325, 1920, 136, 200, 672, 1200, 3, 6, 45, 0, 1, 0, 0, 0, 5988), /* 45h */
    TIMING_PARAMS(19325, 1920, 136, 200, 672, 1200, 3, 6, 45, 0, 1, 0, 0, 0, 5988), /* 45h */
    TIMING_PARAMS(24525, 1920, 136, 208, 688, 1200, 3, 6, 55, 0, 1, 0, 0, 0, 7493), /* 46h */
    TIMING_PARAMS(24525, 1920, 136, 208, 688, 1200, 3, 6, 55, 0, 1, 0, 0, 0, 7493), /* 46h */
    TIMING_PARAMS(28125, 1920, 144, 208, 704, 1200, 3, 6, 62, 0, 1, 0, 0, 0, 8493), /* 47h */
    TIMING_PARAMS(28125, 1920, 144, 208, 704, 1200, 3, 6, 62, 0, 1, 0, 0, 0, 8493), /* 47h */
    TIMING_PARAMS(31700, 1920, 48, 32, 160, 1200, 3, 6, 71, 1, 0, 0, 0, 0, 11991), /* 48h */
    TIMING_PARAMS(31700, 1920, 48, 32, 160, 1200, 3, 6, 71, 1, 0, 0, 0, 0, 11991), /* 48h */
    TIMING_PARAMS(23400, 1920, 128, 208, 680, 1440, 1, 3, 60, 0, 1, 0, 0, 0, 6000), /* 49h */
    TIMING_PARAMS(23400, 1920, 128, 208, 680, 1440, 1, 3, 60, 0, 1, 0, 0, 0, 6000), /* 49h */
    TIMING_PARAMS(29700, 1920, 144, 224, 720, 1440, 1, 3, 60, 0, 1, 0, 0, 0, 7500), /* 4Ah */
    TIMING_PARAMS(29700, 1920, 144, 224, 720, 1440, 1, 3, 60, 0, 1, 0, 0, 0, 7500), /* 4Ah */
    TIMING_PARAMS(38050, 1920, 48, 32, 160, 1440, 3, 4, 85, 1, 0, 0, 0, 0, 11996), /* 4Bh */
    TIMING_PARAMS(38050, 1920, 48, 32, 160, 1440, 3, 4, 85, 1, 0, 0, 0, 0, 11996), /* 4Bh */
    TIMING_PARAMS(26850, 2560, 48, 32, 160, 1600, 3, 6, 46, 1, 0, 0, 0, 0, 5997), /* 4Ch */
    TIMING_PARAMS(26850, 2560, 48, 32, 160, 1600, 3, 6, 46, 1, 0, 0, 0, 0, 5997), /* 4Ch */
    TIMING_PARAMS(34850, 2560, 192, 280, 944, 1600, 3, 6, 58, 0, 1, 0, 0, 0, 5999), /* 4Dh */
    TIMING_PARAMS(34850, 2560, 192, 280, 944, 1600, 3, 6, 58, 0, 1, 0, 0, 0, 5999), /* 4Dh */
    TIMING_PARAMS(44325, 2560, 208, 280, 976, 1600, 3, 6, 72, 0, 1, 0, 0, 0, 7497), /* 4Eh */
    TIMING_PARAMS(44325, 2560, 208, 280, 976, 1600, 3, 6, 72, 0, 1, 0, 0, 0, 7497), /* 4Eh */
    TIMING_PARAMS(50525, 2560, 208, 280, 976, 1600, 3, 6, 82, 0, 1, 0, 0, 0, 8495), /* 4Fh */
    TIMING_PARAMS(50525, 2560, 208, 280, 976, 1600, 3, 6, 82, 0, 1, 0, 0, 0, 8495), /* 4Fh */
    TIMING_PARAMS(55275, 2560, 48, 32, 160, 1600, 3, 6, 94, 1, 0, 0, 0, 0, 11996), /* 50h */
    TIMING_PARAMS(55275, 2560, 48, 32, 160, 1600, 3, 6, 94, 1, 0, 0, 0, 0, 11996), /* 50h */
};

const timing_params_t* dmt_timings = dmt_timings_arr;
const uint32_t dmt_timings_count = sizeof(dmt_timings_arr) / sizeof(dmt_timings_arr[0]);

// Timings taken from ANSI/CTA-861-F
const timing_params_t cea_timings_arr[] = {
    TIMING_PARAMS(2518, 640, 16, 96, 160, 480, 10, 2, 45, 0, 0, 0, 0, 0, 5994), /* 1 */
    TIMING_PARAMS(2700, 720, 16, 62, 138, 480, 9, 6, 45, 0, 0, 0, 0, 0, 5994), /* 2 */
    TIMING_PARAMS(2700, 720, 16, 62, 138, 480, 9, 6, 45, 0, 0, 0, 0, 0, 5994), /* 3 */
    TIMING_PARAMS(7425, 1280, 110, 40, 370, 720, 5, 5, 30, 1, 1, 0, 0, 0, 6000), /* 4 */
    TIMING_PARAMS(7425, 1920, 88, 44, 280, 1080, 2, 5, 22, 1, 1, 1, 1, 0, 6000), /* 5 */
    TIMING_PARAMS(2700, 1440, 38, 124, 276, 480, 4, 3, 22, 0, 0, 1, 1, 1, 5994), /* 6 */
    TIMING_PARAMS(2700, 1440, 38, 124, 276, 480, 4, 3, 22, 0, 0, 1, 1, 1, 5994), /* 7 */
    TIMING_PARAMS(2700, 1440, 38, 124, 276, 240, 4, 3, 22, 0, 0, 0, 0, 1, 6005), /* 8 */
    TIMING_PARAMS(2700, 1440, 38, 124, 276, 240, 4, 3, 22, 0, 0, 0, 0, 1, 6005), /* 9 */
    TIMING_PARAMS(5400, 2880, 76, 248, 552, 480, 4, 3, 22, 0, 0, 1, 1, 1, 5994), /* 10 */
    TIMING_PARAMS(5400, 2880, 76, 248, 552, 480, 4, 3, 22, 0, 0, 1, 1, 1, 5994), /* 11 */
    TIMING_PARAMS(5400, 2880, 76, 248, 552, 240, 4, 3, 22, 0, 0, 0, 0, 1, 6005), /* 12 */
    TIMING_PARAMS(5400, 2880, 76, 248, 552, 240, 4, 3, 22, 0, 0, 0, 0, 1, 6005), /* 13 */
    TIMING_PARAMS(5400, 1440, 32, 124, 276, 480, 9, 6, 45, 0, 0, 0, 0, 1, 5994), /* 14 */
    TIMING_PARAMS(5400, 1440, 32, 124, 276, 480, 9, 6, 45, 0, 0, 0, 0, 1, 5994), /* 15 */
    TIMING_PARAMS(14850, 1920, 88, 44, 280, 1080, 4, 5, 45, 1, 1, 0, 0, 0, 6000), /* 16 */
    TIMING_PARAMS(2700, 720, 12, 64, 144, 576, 5, 5, 49, 0, 0, 0, 0, 0, 5000), /* 17 */
    TIMING_PARAMS(2700, 720, 12, 64, 144, 576, 5, 5, 49, 0, 0, 0, 0, 0, 5000), /* 18 */
    TIMING_PARAMS(7425, 1280, 440, 40, 700, 720, 5, 5, 30, 1, 1, 0, 0, 0, 5000), /* 19 */
    TIMING_PARAMS(7425, 1920, 528, 44, 720, 1080, 2, 5, 22, 1, 1, 1, 1, 0, 5000), /* 20 */
    TIMING_PARAMS(2700, 1440, 24, 126, 288, 576, 2, 3, 24, 0, 0, 1, 1, 1, 5000), /* 21 */
    TIMING_PARAMS(2700, 1440, 24, 126, 288, 576, 2, 3, 24, 0, 0, 1, 1, 1, 5000), /* 22 */
    TIMING_PARAMS(2700, 1440, 24, 126, 288, 288, 2, 3, 24, 0, 0, 0, 0, 1, 5008), /* 23 */
    TIMING_PARAMS(2700, 1440, 24, 126, 288, 288, 2, 3, 24, 0, 0, 0, 0, 1, 5008), /* 24 */
    TIMING_PARAMS(5400, 2880, 48, 252, 576, 576, 2, 3, 24, 0, 0, 1, 1, 1, 5000), /* 25 */
    TIMING_PARAMS(5400, 2880, 48, 252, 576, 576, 2, 3, 24, 0, 0, 1, 1, 1, 5000), /* 26 */
    TIMING_PARAMS(5400, 2880, 48, 252, 576, 288, 2, 3, 24, 0, 0, 0, 0, 1, 5008), /* 27 */
    TIMING_PARAMS(5400, 2880, 48, 252, 576, 288, 2, 3, 24, 0, 0, 0, 0, 1, 5008), /* 28 */
    TIMING_PARAMS(5400, 1440, 24, 128, 288, 576, 5, 5, 49, 0, 0, 0, 0, 1, 5000), /* 29 */
    TIMING_PARAMS(5400, 1440, 24, 128, 288, 576, 5, 5, 49, 0, 0, 0, 0, 1, 5000), /* 30 */
    TIMING_PARAMS(14850, 1920, 528, 44, 720, 1080, 4, 5, 45, 1, 1, 0, 0, 0, 5000), /* 31 */
    TIMING_PARAMS(7425, 1920, 638, 44, 830, 1080, 4, 5, 45, 1, 1, 0, 0, 0, 2400), /* 32 */
    TIMING_PARAMS(7425, 1920, 528, 44, 720, 1080, 4, 5, 45, 1, 1, 0, 0, 0, 2500), /* 33 */
    TIMING_PARAMS(7425, 1920, 88, 44, 280, 1080, 4, 5, 45, 1, 1, 0, 0, 0, 3000), /* 34 */
    TIMING_PARAMS(10800, 2880, 64, 248, 552, 480, 9, 6, 45, 0, 0, 0, 0, 1, 5994), /* 35 */
    TIMING_PARAMS(10800, 2880, 64, 248, 552, 480, 9, 6, 45, 0, 0, 0, 0, 1, 5994), /* 36 */
    TIMING_PARAMS(10800, 2880, 48, 256, 576, 576, 5, 5, 49, 0, 0, 0, 0, 1, 5000), /* 37 */
    TIMING_PARAMS(10800, 2880, 48, 256, 576, 576, 5, 5, 49, 0, 0, 0, 0, 1, 5000), /* 38 */
    TIMING_PARAMS(7200, 1920, 32, 168, 384, 1080, 23, 5, 85, 1, 0, 1, 0, 0, 5000), /* 39 */
    TIMING_PARAMS(14850, 1920, 528, 44, 720, 1080, 2, 5, 22, 1, 1, 1, 1, 0, 10000), /* 40 */
    TIMING_PARAMS(14850, 1280, 440, 40, 700, 720, 5, 5, 30, 1, 1, 0, 0, 0, 10000), /* 41 */
    TIMING_PARAMS(5400, 720, 12, 64, 144, 576, 5, 5, 49, 0, 0, 0, 0, 0, 10000), /* 42 */
    TIMING_PARAMS(5400, 720, 12, 64, 144, 576, 5, 5, 49, 0, 0, 0, 0, 0, 10000), /* 43 */
    TIMING_PARAMS(5400, 1440, 24, 126, 288, 576, 2, 3, 24, 0, 0, 1, 1, 1, 10000), /* 44 */
    TIMING_PARAMS(5400, 1440, 24, 126, 288, 576, 2, 3, 24, 0, 0, 1, 1, 1, 10000), /* 45 */
    TIMING_PARAMS(14850, 1920, 88, 44, 280, 1080, 2, 5, 22, 1, 1, 1, 1, 0, 12000), /* 46 */
    TIMING_PARAMS(14850, 1280, 110, 40, 370, 720, 5, 5, 30, 1, 1, 0, 0, 0, 12000), /* 47 */
    TIMING_PARAMS(5400, 720, 16, 62, 138, 480, 9, 6, 45, 0, 0, 0, 0, 0, 11988), /* 48 */
    TIMING_PARAMS(5400, 720, 16, 62, 138, 480, 9, 6, 45, 0, 0, 0, 0, 0, 11988), /* 49 */
    TIMING_PARAMS(5400, 1440, 38, 124, 276, 480, 4, 3, 22, 0, 0, 1, 1, 1, 11988), /* 50 */
    TIMING_PARAMS(5400, 1440, 38, 124, 276, 480, 4, 3, 22, 0, 0, 1, 1, 1, 11988), /* 51 */
    TIMING_PARAMS(10800, 720, 12, 64, 144, 576, 5, 5, 49, 0, 0, 0, 0, 0, 20000), /* 52 */
    TIMING_PARAMS(10800, 720, 12, 64, 144, 576, 5, 5, 49, 0, 0, 0, 0, 0, 20000), /* 53 */
    TIMING_PARAMS(10800, 1440, 24, 126, 288, 576, 2, 3, 24, 0, 0, 1, 1, 1, 20000), /* 54 */
    TIMING_PARAMS(10800, 1440, 24, 126, 288, 576, 2, 3, 24, 0, 0, 1, 1, 1, 20000), /* 55 */
    TIMING_PARAMS(10800, 720, 16, 62, 138, 480, 9, 6, 45, 0, 0, 0, 0, 0, 23976), /* 56 */
    TIMING_PARAMS(10800, 720, 16, 62, 138, 480, 9, 6, 45, 0, 0, 0, 0, 0, 23976), /* 57 */
    TIMING_PARAMS(10800, 1440, 38, 124, 276, 480, 4, 3, 22, 0, 0, 1, 1, 1, 23976), /* 58 */
    TIMING_PARAMS(10800, 1440, 38, 124, 276, 480, 4, 3, 22, 0, 0, 1, 1, 1, 23976), /* 59 */
    TIMING_PARAMS(5940, 1280, 1760, 40, 2020, 720, 5, 5, 30, 1, 1, 0, 0, 0, 2400), /* 60 */
    TIMING_PARAMS(7425, 1280, 2420, 40, 2680, 720, 5, 5, 30, 1, 1, 0, 0, 0, 2500), /* 61 */
    TIMING_PARAMS(7425, 1280, 1760, 40, 2020, 720, 5, 5, 30, 1, 1, 0, 0, 0, 3000), /* 62 */
    TIMING_PARAMS(29700, 1920, 88, 44, 280, 1080, 4, 5, 45, 1, 1, 0, 0, 0, 12000), /* 63 */
    TIMING_PARAMS(29700, 1920, 528, 44, 720, 1080, 4, 5, 45, 1, 1, 0, 0, 0, 10000), /* 64 */
    TIMING_PARAMS(5940, 1280, 1760, 40, 2020, 720, 5, 5, 30, 1, 1, 0, 0, 0, 2400), /* 65 */
    TIMING_PARAMS(7425, 1280, 2420, 40, 2680, 720, 5, 5, 30, 1, 1, 0, 0, 0, 2500), /* 66 */
    TIMING_PARAMS(7425, 1280, 1760, 40, 2020, 720, 5, 5, 30, 1, 1, 0, 0, 0, 3000), /* 67 */
    TIMING_PARAMS(7425, 1280, 440, 40, 700, 720, 5, 5, 30, 1, 1, 0, 0, 0, 5000), /* 68 */
    TIMING_PARAMS(7425, 1280, 110, 40, 370, 720, 5, 5, 30, 1, 1, 0, 0, 0, 6000), /* 69 */
    TIMING_PARAMS(14850, 1280, 440, 40, 700, 720, 5, 5, 30, 1, 1, 0, 0, 0, 10000), /* 70 */
    TIMING_PARAMS(14850, 1280, 110, 40, 370, 720, 5, 5, 30, 1, 1, 0, 0, 0, 12000), /* 71 */
    TIMING_PARAMS(7425, 1920, 638, 44, 830, 1080, 4, 5, 45, 1, 1, 0, 0, 0, 2400), /* 72 */
    TIMING_PARAMS(7425, 1920, 528, 44, 720, 1080, 4, 5, 45, 1, 1, 0, 0, 0, 2500), /* 73 */
    TIMING_PARAMS(7425, 1920, 88, 44, 280, 1080, 4, 5, 45, 1, 1, 0, 0, 0, 3000), /* 74 */
    TIMING_PARAMS(14850, 1920, 528, 44, 720, 1080, 4, 5, 45, 1, 1, 0, 0, 0, 5000), /* 75 */
    TIMING_PARAMS(14850, 1920, 88, 44, 280, 1080, 4, 5, 45, 1, 1, 0, 0, 0, 6000), /* 76 */
    TIMING_PARAMS(29700, 1920, 528, 44, 720, 1080, 4, 5, 45, 1, 1, 0, 0, 0, 10000), /* 77 */
    TIMING_PARAMS(29700, 1920, 88, 44, 280, 1080, 4, 5, 45, 1, 1, 0, 0, 0, 12000), /* 78 */
    TIMING_PARAMS(5940, 1680, 1360, 40, 1620, 720, 5, 5, 30, 1, 1, 0, 0, 0, 2400), /* 79 */
    TIMING_PARAMS(5940, 1680, 1228, 40, 1488, 720, 5, 5, 30, 1, 1, 0, 0, 0, 2500), /* 80 */
    TIMING_PARAMS(5940, 1680, 700, 40, 960, 720, 5, 5, 30, 1, 1, 0, 0, 0, 3000), /* 81 */
    TIMING_PARAMS(8250, 1680, 260, 40, 520, 720, 5, 5, 30, 1, 1, 0, 0, 0, 5000), /* 82 */
    TIMING_PARAMS(9900, 1680, 260, 40, 520, 720, 5, 5, 30, 1, 1, 0, 0, 0, 6000), /* 83 */
    TIMING_PARAMS(16500, 1680, 60, 40, 320, 720, 5, 5, 105, 1, 1, 0, 0, 0, 10000), /* 84 */
    TIMING_PARAMS(19800, 1680, 60, 40, 320, 720, 5, 5, 105, 1, 1, 0, 0, 0, 12000), /* 85 */
    TIMING_PARAMS(9900, 2560, 998, 44, 1190, 1080, 4, 5, 20, 1, 1, 0, 0, 0, 2400), /* 86 */
    TIMING_PARAMS(9000, 2560, 448, 44, 640, 1080, 4, 5, 45, 1, 1, 0, 0, 0, 2500), /* 87 */
    TIMING_PARAMS(11880, 2560, 768, 44, 960, 1080, 4, 5, 45, 1, 1, 0, 0, 0, 3000), /* 88 */
    TIMING_PARAMS(18563, 2560, 548, 44, 740, 1080, 4, 5, 45, 1, 1, 0, 0, 0, 5000), /* 89 */
    TIMING_PARAMS(19800, 2560, 248, 44, 440, 1080, 4, 5, 20, 1, 1, 0, 0, 0, 6000), /* 90 */
    TIMING_PARAMS(37125, 2560, 218, 44, 410, 1080, 4, 5, 170, 1, 1, 0, 0, 0, 10000), /* 91 */
    TIMING_PARAMS(49500, 2560, 548, 44, 740, 1080, 4, 5, 170, 1, 1, 0, 0, 0, 12000), /* 92 */
    TIMING_PARAMS(29700, 3840, 1276, 88, 1660, 2160, 8, 10, 90, 1, 1, 0, 0, 0, 2400), /* 93 */
    TIMING_PARAMS(29700, 3840, 1056, 88, 1440, 2160, 8, 10, 90, 1, 1, 0, 0, 0, 2500), /* 94 */
    TIMING_PARAMS(29700, 3840, 176, 88, 560, 2160, 8, 10, 90, 1, 1, 0, 0, 0, 3000), /* 95 */
    TIMING_PARAMS(59400, 3840, 1056, 88, 1440, 2160, 8, 10, 90, 1, 1, 0, 0, 0, 5000), /* 96 */
    TIMING_PARAMS(59400, 3840, 176, 88, 560, 2160, 8, 10, 90, 1, 1, 0, 0, 0, 6000), /* 97 */
    TIMING_PARAMS(29700, 4096, 1020, 88, 1404, 2160, 8, 10, 90, 1, 1, 0, 0, 0, 2400), /* 98 */
    TIMING_PARAMS(29700, 4096, 968, 88, 1184, 2160, 8, 10, 90, 1, 1, 0, 0, 0, 2500), /* 99 */
    TIMING_PARAMS(29700, 4096, 88, 88, 304, 2160, 8, 10, 90, 1, 1, 0, 0, 0, 3000), /* 100 */
    TIMING_PARAMS(59400, 4096, 968, 88, 1184, 2160, 8, 10, 90, 1, 1, 0, 0, 0, 5000), /* 101 */
    TIMING_PARAMS(59400, 4096, 88, 88, 304, 2160, 8, 10, 90, 1, 1, 0, 0, 0, 6000), /* 102 */
    TIMING_PARAMS(29700, 3840, 1276, 88, 1660, 2160, 8, 10, 90, 1, 1, 0, 0, 0, 2400), /* 103 */
    TIMING_PARAMS(29700, 3840, 1056, 88, 1440, 2160, 8, 10, 90, 1, 1, 0, 0, 0, 2500), /* 104 */
    TIMING_PARAMS(29700, 3840, 176, 88, 560, 2160, 8, 10, 90, 1, 1, 0, 0, 0, 3000), /* 105 */
    TIMING_PARAMS(59400, 3840, 1056, 88, 1440, 2160, 8, 10, 90, 1, 1, 0, 0, 0, 5000), /* 106 */
    TIMING_PARAMS(59400, 3840, 176, 88, 560, 2160, 8, 10, 90, 1, 1, 0, 0, 0, 6000), /* 107 */
};

const timing_params_t* cea_timings = cea_timings_arr;
const uint32_t cea_timings_count = sizeof(cea_timings_arr) / sizeof(cea_timings_arr[0]);

} // namespace internal
} // namespace edid
