/* Bridge: the simulated loop (Switchyard and three stand-in pedals) driven by text commands,
 * so the test tool's protocol code (pedals/controller/tools/linework-test/lw.js) can be checked
 * against the real core without hardware. One command per line on stdin:
 *   M <hex bytes>   a MIDI message from the app (USB)
 *   R <ms>          run the simulation for this many ms
 *   S +1 | S -1     press Up or Down
 *   C <k> | F <k>   cut or fix cable k (0 = Switchyard's out, 1-2 between pedals, 3 = back in)
 *   Q               quit
 * After each command it prints what happened, then "OK":
 *   A <hex>  message to the app      O <hex>  message on the other-brands out
 *   D <text> <dp>   the display changed */
#include "world.h"

static int shown_app, shown_other;
static char shown_disp[5] = "????";
static uint8_t shown_dp = 0xFF;

static void hex(const char *tag, const uint8_t *b, int n)
{
    printf("%s", tag);
    for (int i = 0; i < n; i++)
        printf(" %02X", b[i]);
    printf("\n");
}

static void report(void)
{
    for (; shown_app < napp; shown_app++)
        hex("A", app_log[shown_app].b, app_log[shown_app].n);
    for (; shown_other < nother; shown_other++)
        hex("O", other_log[shown_other].b, other_log[shown_other].n);
    if (memcmp(disp, shown_disp, 4) != 0 || disp_dp != shown_dp) {
        memcpy(shown_disp, disp, 4);
        shown_dp = disp_dp;
        printf("D %.4s %d\n", disp, disp_dp);
    }
    /* the logs are small: start them again once everything has been shown */
    if (napp > MAXLOG - 200)
        napp = shown_app = 0;
    if (nother > 60)
        nother = shown_other = 0;
    printf("OK\n");
    fflush(stdout);
}

int main(void)
{
    lw_parser_init(&app_parse, app_msg, NULL);
    memset(store, 0xFF, sizeof store);
    T = 1000;
    for (int k = 0; k < NPED; k++)
        make_pedal(k);
    sy_init(&SY, &syhal);
    run(2000);
    report();

    char line[2048];
    while (fgets(line, sizeof line, stdin)) {
        char *p = line + 1;
        switch (line[0]) {
        case 'M': {
            uint8_t b[512];
            int n = 0;
            unsigned v;
            int used;
            while (n < (int)sizeof b && sscanf(p, " %x%n", &v, &used) == 1) {
                b[n++] = (uint8_t)v;
                p += used;
            }
            for (int i = 0; i < n; i++)
                sy_app_byte(&SY, b[i]);
            break;
        }
        case 'R': run(atoi(p)); break;
        case 'S': sy_step(&SY, atoi(p) > 0 ? +1 : -1); break;
        case 'C': case 'F': {
            int k = atoi(p);
            if (k >= 0 && k <= NPED)
                link_[k].cut = line[0] == 'C';
            break;
        }
        case 'Q': return 0;
        default: break;
        }
        report();
    }
    return 0;
}
