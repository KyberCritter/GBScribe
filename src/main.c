// GBScribe — word processor + pixel art for GBA, printing to the Game Boy Printer.
// M0: hello-world boot test (toolchain + mGBA + EZ-Flash Omega verification).

#include <tonc.h>

int main(void)
{
    irq_init(NULL);
    irq_enable(II_VBLANK);

    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0;

    tte_init_se_default(0, BG_CBB(0) | BG_SBB(31));
    tte_init_con();

    tte_write("#{P:72,64}GBScribe");
    tte_write("#{P:60,80}M0: it boots.");

    while (1)
        VBlankIntrWait();

    return 0;
}
