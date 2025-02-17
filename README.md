# NTPS2

A simple NTP client for the PS2. A project I found interesting and appealing, my first bit of homebrew and first work written for a retro console. The app reads and writes to the RTC in the PS2 and gets time from pool.ntp.org.

Not much else to be said here... Does the PS2 need an NTP client? Probably not. But was it a fun challenge to make? Yeah. Pretty much just makes it so you don't have to worry about setting the time after replacing the clock battery anymore, but that's a rare occurance to begin with.

## Controls
Cross - Saves proposed time from NTP server to RTC

Circle - Exits app

Triangle - Toggle Widescreen/Standard

Square (Can only be used on startup) - Sets resolution and switches from interlaced (480i NTSC or 576i PAL) to progressive (480p NTSC or 576p PAL) video output

## Acknowledgements
This app was influenced and inspired by and uses components of the following individuals' work in addition to using the PS2SDK made by the PS2 Homebrew Development Community:
- [NTPSP by joel16](https://github.com/joel16/NTPSP)
- [ps2ConnectToSocket by jmoseman01](https://github.com/jmoseman01/ps2ConnectToSocket/tree/main)
- [PS2SDK](https://github.com/ps2dev/ps2sdk)
