
# ORGTools
*`version 1.0.0` by Dr_Glaucous*
GUI application for manipulating ORG music files based on the Dear ImGui library



![image](https://github.com/DrGlaucous/ORGTools/raw/main/Screenshots/Tool.PNG)


[Find the source code here](https://github.com/DrGlaucous/ORGTools)

This program is a combined suite of my other ORG manipulation tools,
[ORGCopy](https://github.com/DrGlaucous/ORGCopy) and [MIDI2ORG](https://github.com/DrGlaucous/MIDI2ORG).



For those who are unaware, the organya (ORG) song format is used with the popular indie videogame ["Cave Story" (Doukutsu Monogatari)](https://en.wikipedia.org/wiki/Cave_Story) initially released in 2004. [You can download it here.](https://www.cavestory.org/download/cave-story.php)
___

### Program features:
* Convert MIDI files into ORG files
* Copy ORG music tracks from one ORG to another
* Combine multiple ORG tracks into one
#### MIDI Conversion features:
* Drum channel handling
* Supports note volume conversion (only the initial volume of each note, though)
* Supports tempo conversion (once again, only the initial tempo)
* Supports time signature conversion
#### ORG Copy features:
* Automatic time signature adjustment to account for any differences between songs
* The ability to combine multiple tracks into 1 (TrackMASHing)

&nbsp;
&nbsp;
&nbsp;

___
### Usage:

Like my other tools, I tried my best to make something that was intuitive and easy to use.  Unlike my other applications though, this ones uses a GUI interface, which *(should)* make it even easier than before. So far, there are 2 tools included in this program: MIDI2ORG and ORGCopy. I will explain the details of each tool in the section below.

&nbsp;

**A word of note:
If, in attempt to launch the application, no window actually appears, OpenGL may need to be installed on your system because ImGui is built on top of glfw.**

___
#### ORGCopy

After selecting the files to be copied from and to, use the `+` and `-` buttons to add copy jobs to the queue for a max total of 16 jobs (the number of tracks in a single ORG file). Once the number of copy operations has been selected, press the `GO` button to begin the copy operations. ORGCopy will merge tracks in order from top to bottom.  

To combine tracks that are both populated via TrackMASHing, check the corresponding box in the job table queue, then use the buttons to select the priority track whose notes will be kept when notes come into conflict.

Tracks with different time signatures will automatically be re-sized to the least common multiple when notes are copied.
___

#### MIDI2ORG

Usage here is fairly self-explanatory, just open the path of the MIDI to be converted and check the boxes for any additional options the engine should need to perform a proper conversion. More info on what these boxes actually do can be found below.

**It is somewhat helpful to know basic MIDI structure when using this program.** The standard MIDI file has 16 channels, or "instruments" and an unlimited amount of tracks. Each track can use any of those 16 channels to play notes, though the notes each channel plays varies from track to track (I.E, channel 1 of track 2 may sound different from channel 1 of track 8). Channels are also not limited in the number of notes they are capable of playing at the same time. You can sit on the keyboard, and every last note will be recorded simultaneously.

The ORG format does not support this variety, so in order to capture as much of the MIDI data as possible, the MIDI2ORG program will create multiple orgs in a series of folders within the same directory as the MIDI that was converted.

Within the parent folder, there is 1 folder for each MIDI track, and anywhere from 0 to 16 ORGs inside representing the individual channels that track commands (Some MIDIs use track 0 as a conductor track, so no actual notes are played by it).

To combine these multiple files into one, use the ORGCopy tool in the other tab.
___
MIDI files are also **Much** more precise than ORGs. They can be 100s of times more fine in note placement than the former. Occasionally MIDI values share a common reduction factor, and values that were thousands long can be made into 100s or even 10s. The program automatically tries to find this reduction number, but more often than not, notes will be recorded in a way so that no common factor exists. In order to not lose any information, the program will use the values as-is, and the result is an ORG that is unreadable and very, *very* stretched out.

In order to combat this, MIDI2ORG will ask if the input requires **force-simplification**. The value of the shortest note the user wishes to accurately convey can be entered into the prompt, and the program will use that value to divide down the MIDI and round anything that doesn't get reduced evenly. **It is recommended to try auto-simplification first, and only resort to force-simplification if the result the first time was too big.** (Though in my experience, 90% of the time, force-simplification is required to get anything usable.)
___
MIDI2ORG can also process drum channels, but you have to tell it what channel the drums are on. The MIDI standard is to usually place the drums on the 10th channel. (actually channel **#9** when we include channel #0)

Drum-processed ORGs will divide the resultant notes into individual ORG tracks based on their note frequency. The actual instruments associated with each track will still have to be set manually. To get the max number of tracks out of an ORG, note tracks will be used in addition to the drum tracks, but moving or swapping tracks around in an editor like [ORGMaker2](https://www.cavestory.org/download/music-tools.php) is trivial.
___
#### Recommended Supplementary Tools
[Musescore](https://musescore.org/) - good for determining the shortest notes of the song in terms of standard music notes

[MidiEditor](https://www.midieditor.org/) - good for determining the relationship of the channels to the tracks and finding any drum channels

[ORGMaker2](https://www.cavestory.org/download/music-tools.php) - the program used to actually **view and edit** .org files [-and it's also open-source, too](https://github.com/shbow/organya)

___
### Changelog:
#### Version `1.0.0`
* First finalized release of ORGTools
* Current tools are MIDI2ORG and ORGCopy
* Built on the ImGui library
___
### Building:
In the same directory as the CMakeLists.txt, enter the following:

Generate Makefiles:
`cmake -B ./build`

(append >`-G"MSYS Makefiles"`< if using MSYS2 to build)

Optionally, these configuration options may be set to package the program into a single, library-independent executable.
`-DPKG_MSVC_STATIC_RUNTIME=ON`
`-DPKG_CONFIG_STATIC_LIBS=ON`



Generate executable:
`cmake --build ./build --config Release`

The final executable can be found in the "bin" directory



___
### Credits:
Organya Music Format: Daisuke "Pixel" Amaya

"File.cpp" (mostly borrowed from CSE2, a reverse-engineering of the cave story engine): Clownacy and whoever else...

[MIDI-Parser](https://github.com/MStefan99/Midi-Parser) library: MStefan99

Everything left over: Dr_Glaucous