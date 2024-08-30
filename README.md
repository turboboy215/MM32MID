# MM32MID
Mega Man 3-5/Bionic Commando (Game Boy) to MIDI converter

This tool converts music (and sound effects) from Game Boy games using Kouji Murata's sound engine to MIDI format. The sound engine was used almost exclusively in Capcom-produced games, namely Mega Man 3-5 and Bionic Commando (this program gets its name for Mega Man III). It was also used in the Japanese game Itsudemo Nyanto Wonderful, as well as most unlicensed games developed by Vast Fame (e.g. Zook Hero games), which use a revere-engineered sound engine from MM5 with the original sound effects unaltered.

It works with ROM images. To use it, you must specify the name of the ROM followed by the number of the bank containing the sound data (in hex). For games that contain two banks of music, you must run the program twice specifying where each different bank is located. However, in order to prevent files from being overwritten, the MIDI files from the previous bank must either be moved to a separate folder or renamed.

Examples:
* MC2MID "Megaman III (E) [!].gb" 3
* MC2MID "Megaman IV (E) [S].gb" 3
* MC2MID "Megaman IV (E) [S].gb" 18
* MC2MID "Bionic Commando (E) [!].gb" 8

This tool was based on my own reverse-engineering, with almost no actual disassembly involved. Like most of my other programs, another converter, MM32TXT, is also included, which prints out information about the song data from each game. This is essentially a prototype of MM32TXT.

Supported games:
* Bionic Commando
* Itsudemo Nyanto Wonderful
* Mega Man III
* Mega Man IV
* Mega Man V

## To do:
  * Panning support
  * GBS file support
