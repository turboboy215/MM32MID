/*Mega Man 3-5/Bionic Commando (Kouji Murata) (GB) to MIDI converter*/
/*By Will Trowbridge*/
/*Portions based on code by ValleyBell*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#define bankSize 16384

FILE* rom, * txt;
long bank;
long offset;
long tablePtrLoc;
long tableOffset;
int i, j;
char outfile[1000000];
int songNum;
long seqPtrs[4];
long songPtr;
int chanMask;
long bankAmt;
int foundTable = 0;
long firstPtr = 0;
int bcfix = 0;

unsigned static char* romData;

const char MagicBytesA[6] = { 0xCE, 0x00, 0x67, 0x2A, 0x66, 0x6F };
const char MagicBytesB[7] = { 0xCE, 0x00, 0x67, 0xF1, 0x2A, 0x66, 0x6F };

/*Function prototypes*/
unsigned short ReadLE16(unsigned char* Data);
static void Write8B(unsigned char* buffer, unsigned int value);
static void WriteBE32(unsigned char* buffer, unsigned long value);
static void WriteBE24(unsigned char* buffer, unsigned long value);
static void WriteBE16(unsigned char* buffer, unsigned int value);
void song2txt(int songNum, long ptr);

/*Convert little-endian pointer to big-endian*/
unsigned short ReadLE16(unsigned char* Data)
{
	return (Data[0] << 0) | (Data[1] << 8);
}

static void Write8B(unsigned char* buffer, unsigned int value)
{
	buffer[0x00] = value;
}

static void WriteBE32(unsigned char* buffer, unsigned long value)
{
	buffer[0x00] = (value & 0xFF000000) >> 24;
	buffer[0x01] = (value & 0x00FF0000) >> 16;
	buffer[0x02] = (value & 0x0000FF00) >> 8;
	buffer[0x03] = (value & 0x000000FF) >> 0;

	return;
}

static void WriteBE24(unsigned char* buffer, unsigned long value)
{
	buffer[0x00] = (value & 0xFF0000) >> 16;
	buffer[0x01] = (value & 0x00FF00) >> 8;
	buffer[0x02] = (value & 0x0000FF) >> 0;

	return;
}

static void WriteBE16(unsigned char* buffer, unsigned int value)
{
	buffer[0x00] = (value & 0xFF00) >> 8;
	buffer[0x01] = (value & 0x00FF) >> 0;

	return;
}

int main(int args, char* argv[])
{
	printf("Mega Man 3-5/Bionic Commando (GB) to TXT converter\n");
	if (args != 3)
	{
		printf("Usage: MM32TXT <rom> <bank>\n");
		return -1;
	}
	else
	{
		if ((rom = fopen(argv[1], "rb")) == NULL)
		{
			printf("ERROR: Unable to open file %s!\n", argv[1]);
			exit(1);
		}
		else
		{
			bank = strtol(argv[2], NULL, 16);
			if (bank != 1)
			{
				bankAmt = bankSize;
			}
			else
			{
				bankAmt = 0;
			}
			fseek(rom, ((bank - 1) * bankSize), SEEK_SET);
			romData = (unsigned char*)malloc(bankSize);
			fread(romData, 1, bankSize, rom);
			fclose(rom);

			if (romData[0x0099] == 0x44 && romData[0x009A] == 0x07)
			{
				bcfix = 1;
			}
			else
			{
				bcfix = 0;
			}

			/*Try to search the bank for song table loader - Method 1: Mega Man 3/Bionic Commando*/
			for (i = 0; i < bankSize; i++)
			{
				if ((!memcmp(&romData[i], MagicBytesA, 6)) && foundTable != 1)
				{
					tablePtrLoc = bankAmt + i - 4;
					printf("Found pointer to song table at address 0x%04x!\n", tablePtrLoc);
					tableOffset = romData[tablePtrLoc - bankAmt] + (romData[tablePtrLoc + 3 - bankAmt] * 0x100);
					printf("Song table starts at 0x%04x...\n", tableOffset);
					foundTable = 1;
				}
			}

			/*Method 2: Mega Man 4/5*/
			for (i = 0; i < bankSize; i++)
			{
				if ((!memcmp(&romData[i], MagicBytesB, 7)) && foundTable != 1)
				{
					tablePtrLoc = bankAmt + i - 4;
					printf("Found pointer to song table at address 0x%04x!\n", tablePtrLoc);
					tableOffset = romData[tablePtrLoc - bankAmt] + (romData[tablePtrLoc + 3 - bankAmt] * 0x100);
					printf("Song table starts at 0x%04x...\n", tableOffset);
					foundTable = 1;
				}
			}

			if (foundTable == 1)
			{
				i = tableOffset - bankAmt;
				firstPtr = ReadLE16(&romData[i]);
				songNum = 1;
				while ((i + bankAmt) != firstPtr)
				{
					songPtr = ReadLE16(&romData[i]);
					printf("Song %i: 0x%04X\n", songNum, songPtr);
					if (songPtr != 0)
					{
						song2txt(songNum, songPtr);
					}
					i += 2;
					songNum++;
				}
			}
			else
			{
				printf("ERROR: Magic bytes not found!\n");
				exit(-1);
			}
			
		}
	}
}

/*Convert the song data to TXT*/
void song2txt(int songNum, long ptr)
{
	int activeChan[4];
	int maskArray[4];
	unsigned char mask = 0;
	long romPos = 0;
	long seqPos = 0;
	int curTrack = 0;
	int seqEnd = 0;
	int curNote = 0;
	int curNoteLen = 0;
	int chanSpeed = 0;
	int octave = 0;
	int transpose = 0;
	long jumpPos = 0;
	long jumpPosRet = 0;
	int repeat1 = 0;
	int repeat2 = 0;
	unsigned char command[8];
	unsigned char lowNibble = 0;
	unsigned char highNibble = 0;

	sprintf(outfile, "song%i.txt", songNum);
	if ((txt = fopen(outfile, "wb")) == NULL)
	{
		printf("ERROR: Unable to write to file seqs.txt!\n");
		exit(2);
	}
	else
	{
		romPos = ptr - bankAmt;
		mask = romData[romPos];

		/*Try to get active channels for sound effects*/
		maskArray[3] = mask >> 7 & 1;
		maskArray[2] = mask >> 6 & 1;
		maskArray[1] = mask >> 5 & 1;
		maskArray[0] = mask >> 4 & 1;

		if (maskArray[0] == 0 && maskArray[1] == 0 && maskArray[2] == 0 && maskArray[3] == 0)
		{
			fprintf(txt, "Song type: Music\n");
			maskArray[3] = mask >> 3 & 1;
			maskArray[2] = mask >> 2 & 1;
			maskArray[1] = mask >> 1 & 1;
			maskArray[0] = mask & 1;
		}

		else
		{
			fprintf(txt, "Song type: Sound effect\n");
		}

		for (curTrack = 0; curTrack < 4; curTrack++)
		{
			if (maskArray[curTrack] != 0)
			{
				seqEnd = 0;
				romPos += 2;
				seqPos = ReadLE16(&romData[romPos]);
				fprintf(txt, "Channel %i: 0x%04X\n", curTrack + 1, seqPos);

				while (seqEnd == 0)
				{
					command[0] = romData[seqPos - bankAmt];
					command[1] = romData[seqPos + 1 - bankAmt];
					command[2] = romData[seqPos + 2 - bankAmt];
					command[3] = romData[seqPos + 3 - bankAmt];
					command[4] = romData[seqPos + 4 - bankAmt];
					command[5] = romData[seqPos + 5 - bankAmt];
					command[6] = romData[seqPos + 6 - bankAmt];
					command[7] = romData[seqPos + 7 - bankAmt];

					/*Rest*/
					if (command[0] >= 0xC0 && command[0] < 0xD0)
					{
						curNoteLen = command[0] - 0xC0;
						fprintf(txt, "Rest: %i\n", curNoteLen);
						seqPos++;
					}

					/*Initialize channel details*/
					else if (command[0] >= 0xD0 && command[0] < 0xE0)
					{
						chanSpeed = command[0] - 0xD0;
						fprintf(txt, "Initialize channel: Speed %i\n", chanSpeed);
						if (curTrack == 0 || curTrack == 1)
						{
							if (command[4] < 0x80)
							{
								seqPos += 5;
							}
							else
							{
								seqPos += 7;
							}

						}

						else if (curTrack == 2)
						{
							seqPos += 4;
						}

						else if (curTrack == 3)
						{
							/*Fix for Bionic Commando drums*/
							if (bcfix == 1)
							{
								seqPos += 5;
							}
							else
							{
								seqPos++;
							}

						}

					}

					/*Set octave*/
					else if (command[0] >= 0xE0 && command[0] < 0xE8)
					{
						octave = command[0] - 0xE0;
						fprintf(txt, "Set octave: %i\n", octave);
						seqPos++;
					}

					/*Change noise?*/
					else if (command[0] == 0xE8)
					{
						fprintf(txt, "Change noise?: %i\n", command[1]);
						seqPos += 2;
					}

					/*Change instrument parameter?*/
					else if (command[0] == 0xE9)
					{
						fprintf(txt, "Change instrument parameter? %i\n", command[1]);
						seqPos += 2;
					}

					/*Modulate sweep frequency?*/
					else if (command[0] == 0xEA)
					{
						fprintf(txt, "Modulate sweep frequency?: %i\n", command[1]);
						seqPos += 2;
					}

					/*Change vibrato?*/
					else if (command[0] == 0xEB)
					{
						fprintf(txt, "Change vibrato?: %i\n", command[1]);
						seqPos += 2;
					}

					/*Set channel transpose*/
					else if (command[0] == 0xEC)
					{
						transpose = (signed char)command[1];
						fprintf(txt, "Set transpose: %i\n", transpose);
						seqPos += 2;
					}

					/*Command ED*/
					else if (command[0] == 0xED)
					{
						fprintf(txt, "Command ED: %i\n", command[1]);
						seqPos += 2;
					}

					/*Turn on sweep?/others?*/
					else if (command[0] == 0xEE)
					{
						if (command[1] < 0x80)
						{
							seqPos += 2;
						}
						else
						{
							seqPos += 4;
						}
					}

					/*Set pitch tuning*/
					else if (command[0] == 0xEF)
					{
						fprintf(txt, "Set pitch tuning: %i\n", command[1]);
						seqPos += 2;
					}

					/*Set noise envelope?*/
					else if (command[0] == 0xF0)
					{
						fprintf(txt, "Set noise envelope?: %i\n", command[1]);
						seqPos += 2;
					}

					/*Set noise frequency?*/
					else if (command[0] == 0xF1)
					{
						fprintf(txt, "Set noise frequency?: %i\n", command[1]);
						seqPos += 2;
					}

					/*Change envelope*/
					else if (command[0] == 0xF2)
					{
						fprintf(txt, "Change envelope: %i\n", command[1]);
						seqPos += 2;
					}

					/*Set echo?*/
					else if (command[0] == 0xF3)
					{
						fprintf(txt, "Set echo?: %i\n", command[1]);
						seqPos += 2;
					}

					/*Noise sweep?*/
					else if (command[0] == 0xF4)
					{
						fprintf(txt, "Noise sweep?: %i\n", command[1]);
						seqPos += 2;
					}

					/*Change channel speed*/
					else if (command[0] == 0xF6)
					{
						chanSpeed = command[1];
						fprintf(txt, "Change channel speed: %i\n", chanSpeed);
						seqPos += 2;
					}

					/*Jump to position (1)*/
					else if (command[0] == 0xF7)
					{
						jumpPos = ReadLE16(&romData[seqPos + 1 - bankAmt]);
						fprintf(txt, "Jump position (1): 0x%04X\n", jumpPos);
						seqPos += 3;
					}

					/*Jump to position (2)*/
					else if (command[0] == 0xF8)
					{
						jumpPos = ReadLE16(&romData[seqPos + 1 - bankAmt]);
						fprintf(txt, "Jump position (1): 0x%04X\n", jumpPos);
						seqPos += 3;
					}

					/*Return from jump (1)*/
					else if (command[0] == 0xF9)
					{
						fprintf(txt, "Return from jump (1)\n");
						seqPos++;
					}

					/*Return from jump (2)*/
					else if (command[0] == 0xFA)
					{
						fprintf(txt, "Return from jump (2)\n");
						seqPos++;
					}

					/*Start of repeat/loop*/
					else if (command[0] == 0xFB)
					{
						fprintf(txt, "Start of repeat/loop\n");
						seqPos++;
					}

					/*Song loop point*/
					else if (command[0] == 0xFC)
					{
						fprintf(txt, "Song loop point\n");
						seqPos++;
					}

					/*Loop section amount of times*/
					else if (command[0] == 0xFD)
					{
						repeat1 = command[1];

						if (repeat1 == 0)
						{
							fprintf(txt, "Repeat section: infinite\n");
							seqEnd = 1;
						}
						else
						{
							fprintf(txt, "Repeat section: %i times\n", repeat1);
							seqPos += 2;
						}

					}

					/*Loop entire song amount of times*/
					else if (command[0] == 0xFE)
					{
						repeat2 = command[1];

						if (repeat2 == 0)
						{
							fprintf(txt, "Repeat entire song: infinite\n");
							seqEnd = 1;
						}
						else
						{
							fprintf(txt, "Repeat entire song: %i times\n", repeat2);
							seqPos++;
						}

					}

					/*End track*/
					else if (command[0] == 0xFF)
					{
						fprintf(txt, "End of track\n");
						seqEnd = 1;
					}

					/*Play note*/
					else if (command[0] < 0xC0)
					{
						lowNibble = (command[0] >> 4);
						highNibble = (command[0] & 15);
						curNote = lowNibble;
						curNoteLen = highNibble;
						fprintf(txt, "Play note: %i, length: %i\n", curNote, curNoteLen);
						seqPos++;
					}

					/*Unknown command*/
					else
					{
						fprintf(txt, "Unknown command: %01X\n", command[0]);
						seqPos++;
;					}
				}
				fprintf(txt, "\n");
			}
		}
		fclose(txt);
	}
}