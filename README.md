# NTPS2

A simple NTP client for the PS2. A project I found interesting and appealing, my first bit of homebrew and first work written for a retro console. The app reads and writes to the RTC in the PS2 and gets time from pool.ntp.org.

Not much else to be said here... Does the PS2 need an NTP client? Probably not. But was it a fun challenge to make? Yeah. Pretty much just makes it so you don't have to worry about setting the time after replacing the clock battery anymore, but that's a rare occurance to begin with.

## Controls
Cross - Saves proposed time from NTP server to RTC

Circle - Exits app

Triangle - Toggle Widescreen/Standard

Square (Can only be used on startup) - Sets resolution and switches from interlaced (480i NTSC or 576i PAL) to progressive (480p) video output

## Configuration
This app includes a method to launch a specified ELF binary on exit from the program and to skip any need for human input at all, disabling the controls. This way, you can have NTPS2 set to launch on startup using the FMCB configurator, set the time, and then exit to a specific app like OPL automatically. To configure this, simply place a file called "NTPS2.txt" in the same directory you save the NTPS2.ELF binary executable to, note it is case sensitive. In the first line of the text file, specify a 1 or 0 corresponding to if human input is to be skipped or not respectively. Then using a comma as a seperator, specify the path to the desired ELF to be launched.

For example "1,mc1:Apps/OPL.ELF", note that the path is also case sensitive. An example text file is included in the repo soruce as well as under releases. Also note that the flag can be set without specifying a path for the default exit as "1," as long as the comma seperator is included, however the inverse is not true and a flag is required if setting the path in the configuration file. With no configuration file present or if the file is improperly formatted, the app defaults to requiring user input and exiting to the OSDSYS browser.

Note that launching an ELF on exit is only supported on either memory card or USB mass storage. The ELF to be launched, NTPS2, and the config file must each be on one of these storage mediums. Hard drive or network launching is currently not supported. I don't have a model of PS2 with the HDD expansion bay, but if there is interest and someone willing to contribute, this could be addressed in a pull request in the future.

## Acknowledgements
This app was influenced and inspired by and uses components of the following individuals' work in addition to using the PS2SDK made by the PS2 Homebrew Development Community:
- [NTPSP by joel16](https://github.com/joel16/NTPSP)
- [ps2ConnectToSocket by jmoseman01](https://github.com/jmoseman01/ps2ConnectToSocket/tree/main)
- [PS2SDK](https://github.com/ps2dev/ps2sdk)
- [wLaunchELF Loader Subprogram](https://github.com/ps2homebrew/wLaunchELF/tree/master/loader)
