// license:BSD-3-Clause
// copyright-holders:Manuel Abadia, Peter Ferrie, David Haywood
/***************************************************************************

Thunder Hoop II: Strikes Back (c) 1994 Gaelco

Driver by Manuel Abadia <emumanu+mame@gmail.com>

updated by Peter Ferrie <peter.ferrie@gmail.com>

There is a priority bug on the title screen (Gaelco logo is hidden by black
borders)  It seems sprite priority is hacked around on most of the older
Gaelco drivers.


REF.940411
+-------------------------------------------------+
|       C1                                  6116  |
|  VOL  C2*                                 6116  |
|          30MHz                            6116  |
|    M6295                    +----------+  6116  |
|     1MHz                    |TMS       |        |
|       6116                  |TPC1020AFN|        |
|J      6116                  |   -084C  |    H8  |
|A     +------------+         +----------+        |
|M     |DS5002FP Box|         +----------+        |
|M     +------------+         |TMS       |    H12 |
|A             65756          |TPC1020AFN|        |
|              65756          |   -084C  |        |
|                             +----------+        |
|SW1                                   PAL   65764|
|     24MHz    MC68000P12                    65764|
|SW2           C22                    6116        |
|      PAL     C23                    6116        |
+-------------------------------------------------+

  CPU: MC68000P12 & DS5002FP (used for protection)
Sound: OKI M6295
  OSC: 30MHz, 24MHz & 1MHz resonator
  RAM: MHS HM3-65756K-5  32K x 8 SRAM (x2)
       MHS HM3-65764E-5  8K x 8 SRAM (x2)
       UM6116BK-35  2K x 8 SRAM (x8)
  PAL: TI F20L8-25CNT DIP24 (x2)
  VOL: Volume pot
   SW: Two 8 switch dipswitches

DS5002FP Box contains:
  Dallas DS5002SP @ 12MHz
  KM62256BLG-7L - 32Kx8 Low Power CMOS SRAM
  3.6v Battery
  JP1 - 5 pin port to program SRAM

Measurements from actual PCB:
  DS5002FP - 12MHz
  OKI MSM6295 - 1MHz, pin 7 is disconnected (neither pulled LOW or HIGH)
  H-SYNC - 15.151KHz
  V-SYNC - 59.24Hz

***************************************************************************/

#include "emu.h"
#include "includes/thoop2.h"

#include "machine/gaelco_ds5002fp.h"

#include "cpu/m68000/m68000.h"
#include "cpu/mcs51/mcs51.h"
#include "machine/74259.h"
#include "machine/watchdog.h"
#include "sound/okim6295.h"

#include "screen.h"
#include "speaker.h"


void thoop2_state::machine_start()
{
	membank("okibank")->configure_entries(0, 16, memregion("oki")->base(), 0x10000);
}

WRITE8_MEMBER(thoop2_state::OKIM6295_bankswitch_w)
{
	membank("okibank")->set_entry(data & 0x0f);
}

WRITE_LINE_MEMBER(thoop2_state::coin1_lockout_w)
{
	machine().bookkeeping().coin_lockout_w(0, !state);
}

WRITE_LINE_MEMBER(thoop2_state::coin2_lockout_w)
{
	machine().bookkeeping().coin_lockout_w(1, !state);
}

WRITE_LINE_MEMBER(thoop2_state::coin1_counter_w)
{
	machine().bookkeeping().coin_counter_w(0, state);
}

WRITE_LINE_MEMBER(thoop2_state::coin2_counter_w)
{
	machine().bookkeeping().coin_counter_w(1, state);
}

WRITE8_MEMBER(thoop2_state::shareram_w)
{
	// why isn't there an AM_SOMETHING macro for this?
	reinterpret_cast<u8 *>(m_shareram.target())[BYTE_XOR_BE(offset)] = data;
}

READ8_MEMBER(thoop2_state::shareram_r)
{
	// why isn't there an AM_SOMETHING macro for this?
	return reinterpret_cast<u8 const *>(m_shareram.target())[BYTE_XOR_BE(offset)];
}


static ADDRESS_MAP_START( mcu_hostmem_map, 0, 8, thoop2_state )
	AM_RANGE(0x8000, 0xffff) AM_READWRITE(shareram_r, shareram_w) // confirmed that 0x8000 - 0xffff is a window into 68k shared RAM
ADDRESS_MAP_END


static ADDRESS_MAP_START( thoop2_map, AS_PROGRAM, 16, thoop2_state )
	AM_RANGE(0x000000, 0x0fffff) AM_ROM                                                 /* ROM */
	AM_RANGE(0x100000, 0x101fff) AM_RAM_WRITE(vram_w) AM_SHARE("videoram")   /* Video RAM */
	AM_RANGE(0x108000, 0x108007) AM_WRITEONLY AM_SHARE("vregs")                 /* Video Registers */
	AM_RANGE(0x10800c, 0x10800d) AM_DEVWRITE("watchdog", watchdog_timer_device, reset16_w)                           /* INT 6 ACK/Watchdog timer */
	AM_RANGE(0x200000, 0x2007ff) AM_RAM_DEVWRITE("palette", palette_device, write16) AM_SHARE("palette")/* Palette */
	AM_RANGE(0x440000, 0x440fff) AM_RAM AM_SHARE("spriteram")                       /* Sprite RAM */
	AM_RANGE(0x700000, 0x700001) AM_READ_PORT("DSW2")
	AM_RANGE(0x700002, 0x700003) AM_READ_PORT("DSW1")
	AM_RANGE(0x700004, 0x700005) AM_READ_PORT("P1")
	AM_RANGE(0x700006, 0x700007) AM_READ_PORT("P2")
	AM_RANGE(0x700008, 0x700009) AM_READ_PORT("SYSTEM")
	AM_RANGE(0x70000a, 0x70000b) AM_SELECT(0x000070) AM_DEVWRITE8_MOD("outlatch", ls259_device, write_d0, rshift<3>, 0x00ff)
	AM_RANGE(0x70000c, 0x70000d) AM_WRITE8(OKIM6295_bankswitch_w, 0x00ff)               /* OKI6295 bankswitch */
	AM_RANGE(0x70000e, 0x70000f) AM_DEVREADWRITE8("oki", okim6295_device, read, write, 0x00ff)                  /* OKI6295 data register */
	AM_RANGE(0xfe0000, 0xfe7fff) AM_RAM                                          /* Work RAM */
	AM_RANGE(0xfe8000, 0xfeffff) AM_RAM AM_SHARE("shareram")                     /* Work RAM (shared with D5002FP) */
ADDRESS_MAP_END


static ADDRESS_MAP_START( oki_map, 0, 8, thoop2_state )
	AM_RANGE(0x00000, 0x2ffff) AM_ROM
	AM_RANGE(0x30000, 0x3ffff) AM_ROMBANK("okibank")
ADDRESS_MAP_END


static INPUT_PORTS_START( thoop2 )
	PORT_START("DSW1")
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x40, 0x40, "Credit configuration" )
	PORT_DIPSETTING(    0x40, "Start 1C/Continue 1C" )
	PORT_DIPSETTING(    0x00, "Start 2C/Continue 1C" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("DSW2")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x18, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START("P1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START("P2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START("SYSTEM")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE2 )   /* test button */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


static const gfx_layout thoop2_tilelayout =
{
	8,8,                                    /* 8x8 tiles */
	0x400000/16,                            /* number of tiles */
	4,                                      /* 4 bpp */
	{ 0*0x400000*8+8, 0*0x400000*8, 1*0x400000*8+8, 1*0x400000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static const gfx_layout thoop2_tilelayout_16 =
{
	16,16,                                  /* 16x16 tiles */
	0x400000/64,                            /* number of tiles */
	4,                                      /* 4 bpp */
	{ 0*0x400000*8+8, 0*0x400000*8, 1*0x400000*8+8, 1*0x400000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		16*16+0, 16*16+1, 16*16+2, 16*16+3, 16*16+4, 16*16+5, 16*16+6, 16*16+7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};


static GFXDECODE_START( thoop2 )
	GFXDECODE_ENTRY( "gfx1", 0x000000, thoop2_tilelayout, 0,        64 )
	GFXDECODE_ENTRY( "gfx1", 0x000000, thoop2_tilelayout_16, 0, 64 )
GFXDECODE_END


MACHINE_CONFIG_START(thoop2_state::thoop2)

	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M68000,XTAL_24MHz / 2) // 12MHz verified
	MCFG_CPU_PROGRAM_MAP(thoop2_map)
	MCFG_CPU_VBLANK_INT_DRIVER("screen", thoop2_state,  irq6_line_hold)

	MCFG_DEVICE_ADD("gaelco_ds5002fp", GAELCO_DS5002FP, XTAL_24MHz / 2) // 12MHz verified
	MCFG_DEVICE_ADDRESS_MAP(0, mcu_hostmem_map)

	MCFG_DEVICE_ADD("outlatch", LS259, 0)
	MCFG_ADDRESSABLE_LATCH_Q0_OUT_CB(WRITELINE(thoop2_state, coin1_lockout_w))
	MCFG_ADDRESSABLE_LATCH_Q1_OUT_CB(WRITELINE(thoop2_state, coin2_lockout_w))
	MCFG_ADDRESSABLE_LATCH_Q2_OUT_CB(WRITELINE(thoop2_state, coin1_counter_w))
	MCFG_ADDRESSABLE_LATCH_Q3_OUT_CB(WRITELINE(thoop2_state, coin2_counter_w))
	MCFG_ADDRESSABLE_LATCH_Q4_OUT_CB(NOOP) // unknown. Sound related?
	MCFG_ADDRESSABLE_LATCH_Q5_OUT_CB(NOOP) // unknown

	MCFG_WATCHDOG_ADD("watchdog")

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(59.24)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500) /* not accurate */)
	MCFG_SCREEN_SIZE(32*16, 32*16)
	MCFG_SCREEN_VISIBLE_AREA(0, 320-1, 16, 256-1)
	MCFG_SCREEN_UPDATE_DRIVER(thoop2_state, screen_update)
	MCFG_SCREEN_PALETTE("palette")

	MCFG_GFXDECODE_ADD("gfxdecode", "palette", thoop2)
	MCFG_PALETTE_ADD("palette", 1024)
	MCFG_PALETTE_FORMAT(xBBBBBGGGGGRRRRR)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_OKIM6295_ADD("oki", XTAL_1MHz, PIN7_HIGH) // 1MHz resonator - pin 7 not connected
	MCFG_DEVICE_ADDRESS_MAP(0, oki_map)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_CONFIG_END



ROM_START( thoop2 ) /* REF.940411 PCB */
	ROM_REGION( 0x100000, "maincpu", 0 )    /* 68000 code */
	ROM_LOAD16_BYTE(    "th2c23.c23",   0x000000, 0x080000, CRC(3e465753) SHA1(1ea1173b9fe5d652e7b5fafb822e2535cecbc198) )
	ROM_LOAD16_BYTE(    "th2c22.c22",   0x000001, 0x080000, CRC(837205b7) SHA1(f78b90c2be0b4dddaba26f074ea00eff863cfdb2) )

	ROM_REGION( 0x8000, "gaelco_ds5002fp:sram", 0 ) /* DS5002FP code */
	ROM_LOAD( "thoop2_ds5002fp.bin", 0x00000, 0x8000, CRC(6881384d) SHA1(c1eff5558716293e1325b766e2205783286c12f9) ) /* dumped from 3 boards, reconstructed with 2/3 wins rule, all bytes verified by hand as correct */

	ROM_REGION( 0x100, "gaelco_ds5002fp:mcu:internal", ROMREGION_ERASE00 )
	/* these are the default states stored in NVRAM */
	DS5002FP_SET_MON( 0x79 )
	DS5002FP_SET_RPCTL( 0x00 )
	DS5002FP_SET_CRCR( 0x80 )

	ROM_REGION( 0x800000, "gfx1", 0 )
	ROM_LOAD( "th2-h8.h8",     0x000000, 0x400000, CRC(60328a11) SHA1(fcdb374d2fc7ef5351a4181c471d192199dc2081) )
	ROM_LOAD( "th2-h12.h12",   0x400000, 0x400000, CRC(b25c2d3e) SHA1(d70f3e4e2432d80c2ac87cd81208ada303bac04a) )

	ROM_REGION( 0x100000, "oki", 0 )    /* ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "th2-c1.c1",     0x000000, 0x100000, CRC(8fac8c30) SHA1(8e49bb596144761eae95f3e1266e57fb386664f2) )
	/* 0x00000-0x2ffff is fixed, 0x30000-0x3ffff is bank switched */
ROM_END

ROM_START( thoop2a ) /* REF.940411 PCB */
	ROM_REGION( 0x100000, "maincpu", 0 )    /* 68000 code */
	ROM_LOAD16_BYTE(    "3.c23",   0x000000, 0x080000, CRC(6cd4a8dc) SHA1(7d0cdce64b390c3f9769b07d57cf1eee1e6a7bf5) )
	ROM_LOAD16_BYTE(    "2.c22",   0x000001, 0x080000, CRC(59ba9b43) SHA1(6c6690a2e389fc9f1e166c87748da1175e3b58f8) )

	ROM_REGION( 0x8000, "gaelco_ds5002fp:sram", 0 ) /* DS5002FP code */
	ROM_LOAD( "thoop2_ds5002fp.bin", 0x00000, 0x8000, CRC(6881384d) SHA1(c1eff5558716293e1325b766e2205783286c12f9) ) /* dumped from 3 boards, reconstructed with 2/3 wins rule, all bytes verified by hand as correct */

	ROM_REGION( 0x100, "gaelco_ds5002fp:mcu:internal", ROMREGION_ERASE00 )
	/* these are the default states stored in NVRAM */
	DS5002FP_SET_MON( 0x79 )
	DS5002FP_SET_RPCTL( 0x00 )
	DS5002FP_SET_CRCR( 0x80 )

	ROM_REGION( 0x800000, "gfx1", 0 )
	ROM_LOAD( "th2-h8.h8",     0x000000, 0x400000, CRC(60328a11) SHA1(fcdb374d2fc7ef5351a4181c471d192199dc2081) )
	ROM_LOAD( "th2-h12.h12",   0x400000, 0x400000, CRC(b25c2d3e) SHA1(d70f3e4e2432d80c2ac87cd81208ada303bac04a) )

	ROM_REGION( 0x100000, "oki", 0 )    /* ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "th2-c1.c1",     0x000000, 0x100000, CRC(8fac8c30) SHA1(8e49bb596144761eae95f3e1266e57fb386664f2) )
	/* 0x00000-0x2ffff is fixed, 0x30000-0x3ffff is bank switched */
ROM_END

GAME( 1994, thoop2,       0, thoop2, thoop2, thoop2_state,  0, ROT0, "Gaelco", "TH Strikes Back (Non North America, Version 1.0, Checksum 020E0867)", MACHINE_IMPERFECT_GRAPHICS | MACHINE_SUPPORTS_SAVE )
GAME( 1994, thoop2a, thoop2, thoop2, thoop2, thoop2_state,  0, ROT0, "Gaelco", "TH Strikes Back (Non North America, Version 1.0, Checksum 020EB356)", MACHINE_IMPERFECT_GRAPHICS | MACHINE_SUPPORTS_SAVE )
