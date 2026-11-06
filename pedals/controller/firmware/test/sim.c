/* PC simulation of a Linework loop: Switchyard and three stand-in pedals, connected by
 * MIDI "cables" that move bytes at the real 31,250 baud (about 3 bytes per ms), plus a fake
 * phone app on USB. Runs the core code unchanged and checks it against the
 * Linework Control Specification 0.2.
 *
 * Build and run (any C compiler):  make -C test   or   gcc -I../core -I../../../../libraries/linework sim.c ../core/switchyard.c ../../../../libraries/linework/lw_proto.c ../../../../libraries/linework/pedal.c ../../../../libraries/linework/pd_store.c -o sim && ./sim
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "switchyard.h"
#include "pedal.h"

#include "world.h"

/* ---------------------------------------------------------------- app helpers */

static void app(const uint8_t *m, int n)
{
    for (int i = 0; i < n; i++)
        sy_app_byte(&SY, m[i]);
}

static void app_lw(lw_msg *m) { app(m->b, m->n); }

/* first Linework message from mark onward with this command (and qq, unless qq < 0) */
static logged *find_lw(int mark, uint8_t cmd, int qq)
{
    for (int i = mark; i < napp; i++)
        if (lw_is_linework(app_log[i].b, app_log[i].n) && LW_CMD(app_log[i].b) == cmd
            && (qq < 0 || LW_QQ(app_log[i].b) == qq))
            return &app_log[i];
    return NULL;
}

static logged *find_last_lw(int mark, uint8_t cmd)
{
    logged *r = NULL;
    for (int i = mark; i < napp; i++)
        if (lw_is_linework(app_log[i].b, app_log[i].n) && LW_CMD(app_log[i].b) == cmd)
            r = &app_log[i];
    return r;
}

static int ack_st(int mark, uint8_t qq, uint8_t cmd)
{
    for (int i = mark; i < napp; i++) {
        uint8_t *b = app_log[i].b;
        if (lw_is_linework(b, app_log[i].n) && LW_CMD(b) == LW_ACK && LW_QQ(b) == qq && LW_BODY(b)[1] == cmd)
            return LW_BODY(b)[2];
    }
    return -1;
}

static int fails, checks;
#define CHECK(cond, ...) do { checks++; if (!(cond)) { fails++; printf("  FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

/* ---------------------------------------------------------------- setlist upload */

typedef struct { uint8_t song, part, n, pair[4][2]; } tpart;

static uint32_t entry_crc;
static void send_entry(lw_msg *m)
{
    entry_crc = lw_crc32(entry_crc, LW_BODY(m->b), (size_t)(m->n - (LW_HDR_LEN + 2) - 1));
    app_lw(m);
}

static int upload(uint8_t qq0, uint32_t id, bool wrap, const tpart *parts, int nparts, bool corrupt)
{
    int mark = napp;
    lw_msg m;
    lw_begin(&m, SY_SL_BEGIN, qq0);
    lw_put32(&m, id);
    lw_put(&m, wrap ? 1 : 0);
    lw_put(&m, 3);
    lw_put14(&m, (unsigned)nparts);
    lw_end(&m);
    app_lw(&m);
    run(50);
    if (ack_st(mark, qq0, SY_SL_BEGIN) != ST_OK)
        return -100;
    entry_crc = 0;
    /* table: 0 = pedal 2, 1 = pedal 1 (by serial), 2 = another brand on channel 3 */
    const int tp[2] = {1, 0};
    for (int i = 0; i < 2; i++) {
        lw_begin(&m, SY_SL_PEDAL, qq0 + 1);
        lw_put(&m, (uint8_t)i);
        lw_put(&m, 0);
        lw_put_n(&m, P[tp[i]].hal.serial, LW_SERIAL_LEN);
        lw_end(&m);
        send_entry(&m);
    }
    lw_begin(&m, SY_SL_PEDAL, qq0 + 1);
    lw_put(&m, 2);
    lw_put(&m, 1);
    lw_put(&m, 3);
    lw_end(&m);
    send_entry(&m);
    for (int i = 0; i < nparts; i++) {
        lw_begin(&m, SY_SL_PART, qq0 + 2);
        lw_put14(&m, (unsigned)i);
        lw_put(&m, parts[i].song);
        lw_put(&m, parts[i].part);
        for (int k = 0; k < parts[i].n; k++) {
            lw_put(&m, parts[i].pair[k][0]);
            lw_put(&m, parts[i].pair[k][1]);
        }
        lw_end(&m);
        send_entry(&m);
    }
    lw_begin(&m, SY_SL_FINISH, qq0 + 3);
    lw_put32(&m, corrupt ? entry_crc ^ 1 : entry_crc);
    lw_end(&m);
    app_lw(&m);
    run(5);
    return ack_st(mark, qq0 + 3, SY_SL_FINISH);
}

static void req(uint8_t cmd, uint8_t qq)
{
    lw_msg m;
    lw_begin(&m, cmd, qq);
    lw_end(&m);
    app_lw(&m);
}

/* ---------------------------------------------------------------- tests */

int main(void)
{
    lw_parser_init(&app_parse, app_msg, NULL);
    memset(store, 0xFF, sizeof store);
    T = 1000;
    for (int k = 0; k < NPED; k++)
        make_pedal(k);
    sy_init(&SY, &syhal);

    printf("1. Power-up scan\n");
    run(2000);
    CHECK(SY.npedals == 3, "found %d pedals", SY.npedals);
    for (int k = 0; k < NPED; k++) {
        CHECK(P[k].position == k + 1, "pedal %d position %d", k, P[k].position);
        CHECK(P[k].channel == k, "pedal %d channel %d", k, P[k].channel);
        CHECK(SY.pedal[k].pos == k + 1 && SY.pedal[k].ch == k, "table entry %d: pos %d ch %d", k, SY.pedal[k].pos, SY.pedal[k].ch);
        CHECK(SY.pedal[k].model[0] == 0x7F && SY.pedal[k].fw[1] == 1 && SY.pedal[k].spec == 2, "pedal %d identity", k);
    }
    CHECK(!SY.broken, "loop reported broken");
    CHECK(memcmp(disp, "----", 4) == 0, "display '%.4s' with no setlist", disp);

    printf("2. Identity request and Switchyard info\n");
    int mark = napp;
    const uint8_t idreq[] = {0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7};
    app(idreq, sizeof idreq);
    req(SY_INFO_REQ, 0x41);
    run(5);
    const uint8_t idrep[] = {0xF0, 0x7E, 0x7F, 0x06, 0x02, 0x7D, 0x4C, 0x57, 0x02, 0x00, 0, 1, 0, 2, 0xF7};
    CHECK(napp > mark && app_log[mark].n == sizeof idrep && memcmp(app_log[mark].b, idrep, sizeof idrep) == 0,
          "identity reply from Switchyard itself");
    logged *l = find_lw(mark, SY_INFO, 0x41);
    CHECK(l && l->n == LW_HDR_LEN + 2 + 4 + LW_SERIAL_LEN + 1 + 1 && LW_BODY(l->b)[14] == 1, "Switchyard info");

    printf("3. Get pedal list\n");
    mark = napp;
    req(SY_LIST_REQ, 0x42);
    run(5);
    l = find_lw(mark, SY_LIST_COUNT, 0x42);
    CHECK(l && LW_BODY(l->b)[0] == 3, "pedal count");
    int entries = 0;
    for (int i = mark; i < napp; i++)
        if (lw_is_linework(app_log[i].b, app_log[i].n) && LW_CMD(app_log[i].b) == SY_LIST_ENTRY) {
            const uint8_t *b = LW_BODY(app_log[i].b);
            CHECK(b[0] == entries + 1 && memcmp(&b[8], P[entries].hal.serial, LW_SERIAL_LEN) == 0, "list entry %d", entries);
            entries++;
        }
    CHECK(entries == 3, "%d list entries", entries);

    printf("4. App to pedal through the loop: Store preset, Read preset, Report settings\n");
    mark = napp;
    lw_msg m;
    lw_begin(&m, LW_STORE, 0x43);
    lw_put(&m, 1);          /* pedal 2 is on channel 1 */
    lw_put(&m, 5);          /* slot 5 */
    lw_put(&m, 0);
    const uint8_t pairs5[] = {20, 10, 21, 20, 22, 30, 80, 127};
    lw_put_n(&m, pairs5, sizeof pairs5);
    lw_end(&m);
    app_lw(&m);
    run(60);
    CHECK(ack_st(mark, 0x43, LW_STORE) == ST_OK, "store preset acknowledged by the pedal");
    CHECK(P[1].preset[5][0] == 10 && P[1].preset[5][3] == 127, "preset 5 stored");
    mark = napp;
    lw_begin(&m, LW_READ_REQ, 0x44);
    lw_put(&m, 1);
    lw_put(&m, 5);
    lw_end(&m);
    app_lw(&m);
    run(60);
    l = find_lw(mark, LW_READ, 0x44);
    CHECK(l && memcmp(&LW_BODY(l->b)[2], pairs5, sizeof pairs5) == 0, "read preset");
    /* presets the setlist uses */
    uint8_t fill[][3] = {{1, 6, 40}, {0, 7, 50}, {0, 8, 60}};
    for (int i = 0; i < 3; i++) {
        lw_begin(&m, LW_STORE, 0x45 + i);
        lw_put(&m, fill[i][0]);
        lw_put(&m, fill[i][1]);
        lw_put(&m, 0);
        lw_put(&m, 20);
        lw_put(&m, fill[i][2]);
        lw_end(&m);
        app_lw(&m);
        run(40);
    }

    printf("5. Setlist upload\n");
    const tpart parts[] = {
        {1, 1, 2, {{0, 5}, {2, 10}}},
        {1, 2, 1, {{1, 7}}},
        {2, 1, 3, {{0, 6}, {1, 8}, {2, 11}}},
    };
    int st = upload(0x50, 1234567, false, parts, 3, false);
    CHECK(st == ST_OK, "setlist finish ack %d", st);
    CHECK(memcmp(disp, "011 ", 4) == 0 && disp_dp == 0x02, "display '%.4s' (01.1)", disp);
    mark = napp;
    req(SY_SL_INFO_REQ, 0x54);
    run(2);
    l = find_lw(mark, SY_SL_INFO, 0x54);
    CHECK(l && lw_get32(LW_BODY(l->b)) == 1234567 && lw_get32(LW_BODY(l->b) + 5) == entry_crc
          && lw_get14(LW_BODY(l->b) + 12) == 3, "setlist info");
    nother = 0;
    CHECK(P[1].last_preset == -1, "nothing sent at upload");

    printf("6. Jump and stepping\n");
    mark = napp;
    lw_begin(&m, SY_JUMP, 0x5A);
    lw_put14(&m, 0);
    lw_end(&m);
    app_lw(&m);
    run(20);
    CHECK(ack_st(mark, 0x5A, SY_JUMP) == ST_OK, "jump acknowledged");
    l = find_lw(mark, SY_POS, 0x5A);
    CHECK(l && LW_BODY(l->b)[4] == WHY_JUMP, "position report after jump");
    CHECK(P[1].last_preset == 5 && P[1].value[0] == 10, "pedal 2 recalled preset 5");
    CHECK(nother == 1 && other_log[0].b[0] == 0xC3 && other_log[0].b[1] == 10, "other brand got program change 10 on channel 3");
    l = find_lw(mark, 0, -1);
    int ccs = 0;                                   /* the recalled pedal reports its settings */
    for (int i = mark; i < napp; i++)
        if (app_log[i].b[0] == 0xB1)
            ccs++;
    CHECK(ccs == 4, "%d control changes reported back after the recall", ccs);

    mark = napp;
    sy_step(&SY, +1);
    run(20);
    CHECK(P[0].last_preset == 7, "pedal 1 recalled preset 7");
    l = find_last_lw(mark, SY_POS);
    CHECK(l && lw_get14(LW_BODY(l->b)) == 1 && LW_BODY(l->b)[4] == WHY_UP, "position report Up");
    CHECK(memcmp(disp, "012 ", 4) == 0, "display '%.4s' (01.2)", disp);
    sy_step(&SY, +1);
    run(20);
    CHECK(P[1].last_preset == 6 && P[0].last_preset == 8 && other_log[nother - 1].b[1] == 11, "part 3 recalls");
    CHECK(memcmp(disp, "021 ", 4) == 0, "display '%.4s' (02.1)", disp);
    mark = napp;
    sy_step(&SY, +1);
    run(5);
    l = find_last_lw(mark, SY_POS);
    CHECK(SY.cur == 2 && l && LW_BODY(l->b)[4] == WHY_UP_END, "Up at the end without wrap");
    sy_step(&SY, -1);
    sy_step(&SY, -1);
    mark = napp;
    sy_step(&SY, -1);
    run(5);
    l = find_last_lw(mark, SY_POS);
    CHECK(SY.cur == 0 && l && LW_BODY(l->b)[4] == WHY_DOWN_START, "Down at the start without wrap");

    printf("7. A failed upload keeps the old setlist\n");
    const tpart bad[] = {{5, 1, 1, {{0, 1}}}};
    st = upload(0x60, 999, true, bad, 1, true);
    CHECK(st == ST_SETLIST_FAIL, "bad CRC ack %d", st);
    CHECK(SY.hdr.id == 1234567 && SY.hdr.nparts == 3, "old setlist still active");

    printf("8. Wrap setting\n");
    st = upload(0x64, 77, true, parts, 3, false);
    CHECK(st == ST_OK && SY.hdr.gen == 2, "second setlist (generation %u)", SY.hdr.gen);
    SY.cur = 2;
    mark = napp;
    sy_step(&SY, +1);
    run(5);
    l = find_last_lw(mark, SY_POS);
    CHECK(SY.cur == 0 && l && LW_BODY(l->b)[4] == WHY_UP, "Up wraps to the first part");

    printf("9. Other brands and app program changes\n");
    mark = napp;
    nother = 0;
    lw_begin(&m, SY_OTHER_SEND, 0x4A);
    lw_put(&m, 1);
    lw_put(&m, 5);
    lw_put(&m, 7);
    lw_put(&m, 100);
    lw_end(&m);
    app_lw(&m);
    run(2);
    CHECK(ack_st(mark, 0x4A, SY_OTHER_SEND) == ST_OK && nother == 1 && other_log[0].b[0] == 0xB5
          && other_log[0].b[2] == 100, "control change to another brand");
    const uint8_t pc[] = {0xC2, 0x05};               /* pedal 3, channel 2 */
    app(pc, 2);
    run(20);
    CHECK(P[2].last_preset == 5, "app program change reaches pedal 3");
    mark = napp;
    pd_set_control(&P[0], 1, 99);                    /* a knob turned by hand on pedal 1 */
    run(20);
    bool seen = false;
    for (int i = mark; i < napp; i++)
        if (app_log[i].n == 3 && app_log[i].b[0] == 0xB0 && app_log[i].b[1] == 21 && app_log[i].b[2] == 99)
            seen = true;
    CHECK(seen, "hand change reported to the app");
    for (int i = mark; i < napp; i++)
        CHECK(!(app_log[i].n == 1 && app_log[i].b[0] == 0xFE), "Active Sensing leaked to the app");

    printf("10. Loop break between pedals 2 and 3, then repair\n");
    mark = napp;
    link_[2].cut = true;                             /* pedal 2's out cable pulled */
    run(3500);
    l = find_last_lw(mark, SY_STATUS);
    CHECK(SY.broken && l && LW_BODY(l->b)[0] == 1 && LW_BODY(l->b)[2] == 2, "status: broken, look at pedal 2 (bb=%d)",
          l ? LW_BODY(l->b)[2] : -1);
    mark = napp;
    link_[2].cut = false;
    run(2000);
    l = find_last_lw(mark, SY_STATUS);
    CHECK(!SY.broken && l && LW_BODY(l->b)[0] == 0, "status: repaired");
    CHECK(SY.npedals == 3 && SY.scan == SCAN_IDLE, "rescanned after repair");

    printf("11. Cable from Switchyard's out pulled\n");
    mark = napp;
    link_[0].cut = true;
    run(3500);
    l = find_last_lw(mark, SY_STATUS);
    CHECK(l && LW_BODY(l->b)[2] == 0, "status: look at Switchyard's MIDI out cable (bb=%d)", l ? LW_BODY(l->b)[2] : -1);
    link_[0].cut = false;
    run(2000);
    CHECK(!SY.broken, "repaired");

    printf("12. Duplicate channel fixed by a scan\n");
    P[2].channel = 0;                                /* clashes with pedal 1 */
    mark = napp;
    req(SY_SCAN, 0x42);
    run(1500);
    CHECK(P[0].channel == 0 && P[1].channel == 1 && P[2].channel == 2, "channels now %d %d %d",
          P[0].channel, P[1].channel, P[2].channel);
    l = find_lw(mark, SY_LIST_COUNT, 0x42);
    CHECK(l && LW_BODY(l->b)[0] == 3, "scan answered with the pedal list");

    printf("13. Power cycle: Switchyard returns to the saved part\n");
    SY.cur = 0;
    sy_step(&SY, +1);                                /* part 2 */
    run(2500);                                       /* saved after 2 s */
    P[0].last_preset = P[1].last_preset = -1;
    mark = napp;
    sy_init(&SY, &syhal);
    CHECK(SY.cur == 1 && SY.hdr.id == 77, "restored part %d of setlist %u", SY.cur, SY.hdr.id);
    run(2000);
    CHECK(P[0].last_preset == 7, "part 2's program change resent at power-up");
    l = find_last_lw(mark, SY_POS);
    CHECK(l && LW_BODY(l->b)[4] == WHY_POWERUP, "power-up position report");

    printf("14. Unknown Switchyard command\n");
    mark = napp;
    req(0x5F, 0x70);
    run(2);
    CHECK(ack_st(mark, 0x70, 0x5F) == ST_UNKNOWN, "unknown command acknowledged with 01");

    printf("15. Switchyard's own firmware update\n");
    {
        static uint8_t img[20000];
        for (unsigned i = 0; i < sizeof img; i++)
            img[i] = (uint8_t)(i * 13 + (i >> 9));
        uint32_t crc = lw_crc32(0, img, sizeof img);
        int blocks = (int)((sizeof img + FW_BLOCK - 1) / FW_BLOCK);
        uint32_t id_before = SY.hdr.id;
        int cur_before = SY.cur;
        uint8_t qq = 0x40;
        #define NEXTQ() (qq = (uint8_t)(qq >= 0x7F ? 0x40 : qq + 1))

        /* builders: Begin to a serial; a data block with ch 7F; Finish with ch 7F */
        #define BEGIN(serial, c) do { lw_msg m_; lw_begin(&m_, LW_FW_BEGIN, NEXTQ()); lw_put_n(&m_, serial, LW_SERIAL_LEN); \
            lw_put(&m_, sizeof img & 0x7F); lw_put(&m_, (sizeof img >> 7) & 0x7F); lw_put(&m_, (sizeof img >> 14) & 0x7F); \
            lw_put32(&m_, c); lw_end(&m_); app_lw(&m_); } while (0)
        uint8_t packed[FW_PACKED];
        #define BLOCK(n, damage) do { uint8_t d_[FW_BLOCK]; memset(d_, 0xFF, sizeof d_); \
            unsigned o_ = (unsigned)(n) * FW_BLOCK, k_ = sizeof img - o_ < FW_BLOCK ? sizeof img - o_ : FW_BLOCK; \
            memcpy(d_, &img[o_], k_); lw_pack7(d_, FW_BLOCK, packed); uint8_t x_ = lw_fw_checksum(packed, FW_PACKED); \
            if (damage) packed[5] ^= 0x01; \
            lw_msg m_; lw_begin(&m_, LW_FW_DATA, NEXTQ()); lw_put(&m_, 0x7F); lw_put14(&m_, (unsigned)(n)); \
            lw_put_n(&m_, packed, FW_PACKED); lw_put(&m_, x_); lw_end(&m_); app_lw(&m_); } while (0)
        #define FINISH() do { lw_msg m_; lw_begin(&m_, LW_FW_FINISH, NEXTQ()); lw_put(&m_, 0x7F); lw_end(&m_); app_lw(&m_); } while (0)

        /* the block number in the acknowledgement of a data block with this qq, or -1 */
        #define BLOCK_ACK(mk, q, st_out) ({ int r_ = -1; for (int i_ = (mk); i_ < napp; i_++) { uint8_t *b_ = app_log[i_].b; \
            if (lw_is_linework(b_, app_log[i_].n) && LW_CMD(b_) == LW_ACK && LW_QQ(b_) == (q) && LW_BODY(b_)[1] == LW_FW_DATA \
                && LW_BODY(b_)[0] == 0x7F) { st_out = LW_BODY(b_)[2]; r_ = (int)lw_get14(&LW_BODY(b_)[3]); } } r_; })

        mark = napp;
        loop_fw_msgs = 0;
        static const uint8_t other_serial[LW_SERIAL_LEN] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        BEGIN(other_serial, crc);
        run(100);
        CHECK(loop_fw_msgs == 1 && ack_st(mark, qq, LW_FW_BEGIN) == -1 && !syu_active(&SY.upd),
              "Begin for another serial goes into the loop (%d), unanswered", loop_fw_msgs);
        mark = napp;
        FINISH();
        CHECK(ack_st(mark, qq, LW_FW_FINISH) == ST_RANGE, "Finish with no update: value out of range");

        mark = napp;
        sy_erases = 0;
        BEGIN(SY.hal.serial, crc);
        uint8_t begin_qq = qq;
        int st = -1;
        BLOCK(0, false);
        CHECK(BLOCK_ACK(mark, qq, st) == 0 && st == ST_BUSY, "a block during the erase: busy (%d)", st);
        run(1);
        CHECK(memcmp(disp, "----", 4) == 0 && disp_dp == 0, "display ---- while erasing ('%.4s')", disp);
        run(20);
        CHECK(ack_st(mark, begin_qq, LW_FW_BEGIN) == ST_OK && sy_erases == 5, "Begin acknowledged once 5 sectors erased (%d)", sy_erases);

        int bad = 0;
        for (int n = 0; n < blocks; n++) {
            int m0 = napp;
            if (n == 100) {
                BLOCK(n, true);
                CHECK(BLOCK_ACK(m0, qq, st) == 100 && st == ST_CHECKSUM, "damaged block 100: checksum failed (%d)", st);
                m0 = napp;
            }
            BLOCK(n, false);
            if (BLOCK_ACK(m0, qq, st) != n || st != ST_OK)
                bad++;
            if (n == blocks / 2) {
                run(1);
                CHECK(disp[0] == '-' && disp[1] == '-' && disp[2] == '5' && disp_dp == 0, "display --NN halfway ('%.4s')", disp);
                int was = SY.cur;
                sy_step(&SY, +1);
                CHECK(SY.cur == was, "footswitches wait during the update");
            }
        }
        CHECK(bad == 0, "every block acknowledged with its number, ch 7F (%d bad)", bad);
        CHECK(loop_fw_msgs == 1, "no firmware data went into the loop (%d messages)", loop_fw_msgs);
        CHECK(!SY.broken, "no loop break reported during the update");
        mark = napp;
        FINISH();
        CHECK(ack_st(mark, qq, LW_FW_FINISH) == ST_OK, "Finish: CRC matches");
        CHECK(sy_installs == 0, "not restarted until the ack has had time to leave");
        run(SYU_RESTART_MS + 50);
        CHECK(sy_installs == 1 && memcmp(&sy_chip[FW_APP_OFF], img, sizeof img) == 0, "restarted: the loader installed it");
        CHECK(SY.hdr.id == id_before && SY.cur == cur_before && SY.have_setlist, "setlist and part kept");
        run(3000);
        CHECK(SY.npedals == 3 && !SY.broken, "scanned the loop again after the restart");

        printf("16. A failed and an abandoned update\n");
        mark = napp;
        BEGIN(SY.hal.serial, crc ^ 1);
        run(20);
        for (int n = 0; n < blocks; n++)
            BLOCK(n, false);
        FINISH();
        run(SYU_RESTART_MS + 50);
        CHECK(ack_st(mark, qq, LW_FW_FINISH) == ST_FW_FAIL && sy_installs == 1 && !syu_active(&SY.upd),
              "wrong CRC: firmware check failed, nothing installed");
        CHECK(memcmp(disp, "----", 4) != 0 || !SY.have_setlist, "display back to the part ('%.4s')", disp);
        BEGIN(SY.hal.serial, crc);
        run(20);
        for (int n = 0; n < 3; n++)
            BLOCK(n, false);
        run(SYU_IDLE_MS + 100);
        CHECK(!syu_active(&SY.upd) && sy_installs == 1 && disp_dp == 0x02, "abandoned after 10 s quiet: back to normal ('%.4s')", disp);
    }

    printf("17. Byte helpers\n");
    lw_msg t;
    lw_begin(&t, 0, 0);
    lw_put14(&t, 300);
    CHECK(t.b[LW_HDR_LEN + 2] == 0x2C && t.b[LW_HDR_LEN + 3] == 0x02, "two-byte number 300 = 2C 02");
    CHECK(lw_crc32(0, (const uint8_t *)"123456789", 9) == 0xCBF43926u, "CRC-32 check value");

    printf("\n%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
