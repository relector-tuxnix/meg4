Music Tracks
============

<imgt ../img/music.png> Click on the music note icon (or press <kbd>F7</kbd>) to edit the music tracks.

The music you create here can be played in your program using the [music] command.

You'll see five coloumns and below a piano.

<imgc ../img/musicscr.png><fig>Music Tracks</fig>

## Tracks

On the left the first coloumn selects which music track to edit.

| Key Combination              | Description                                                                                  |
|------------------------------|----------------------------------------------------------------------------------------------|
| <kbd>Page Up</kbd>           | Switch to previous track.                                                                    |
| <kbd>Page Down</kbd>         | Switch to next track.                                                                        |
| <kbd>Space</kbd>             | Start / stop playing the track.                                                              |

Below the track selector you can see the DSP status registers, but this block only comes alive when music playback is on.

## Channels

Next to it, you'll see four similar coloumns, each with notes. These are the 4 channels that the music can simultaniously play.
This is similar to standard music sheets, for more info read the [General MIDI] section below.

| Key Combination              | Description                                                                                  |
|------------------------------|----------------------------------------------------------------------------------------------|
| <kbd>◂</kbd>                 | Switch to previous channel.                                                                  |
| <kbd>▸</kbd>                 | Switch to next channel.                                                                      |
| <kbd>▴</kbd>                 | Switch to previous row.                                                                      |
| <kbd>▾</kbd>                 | Switch to next row.                                                                          |
| <kbd>Home</kbd>              | Switch to first row.                                                                         |
| <kbd>End</kbd>               | Switch to last row.                                                                          |
| <kbd>Ins</kbd>               | Insert a row. Move everything below down one row.                                            |
| <kbd>Del</kbd>               | Delete a row. Move everything below up one row.                                              |
| <kbd>Backspace</kbd>         | Clear note.                                                                                  |

## Note Editor

Under the channels you can see the note editor, with some buttons on the left and a big piano on the right.

Notes have three parts, the first one (on the top in the editor), the pitch consist of further three sub-parts: the note itself,
like `C` or `D`. Then the `-` character if it's a flat note, or `#` for sharp notes. The third part is simply the octave, from
`0` (lowest pitch) to `7` (highest pitch). The 440 Hz pitch for the standard musical note A for example is therefore written as
`A-4`. Using the piano you can easily select the pitch.

After the pitch comes the aforementioned instrument (the middle part of the note) that chooses the waveform to be used, from
`01` to `1F`. The value 0 is printed as `..` and means keep using the same waveform as before.

Finally, you can also add special effects to the notes (the last part of the note), like arpeggio (make it sound like a chord),
portamento, vibrato, tremolo etc. See the [memory map](#digital_signal_processor) for a full list. This has a numeric parameter,
usually interpreted as "how much". It is printed as three hex numbers, where the first represents the effect type and the last two
are the parameter, or `...` if it's not set. For example `C00` means set the volume to zero, so silence the channel.

<h3 mus_kbd>Keyboard</h3>

<imgc ../img/pianokbd.png><fig>Piano Keyboard arrangement</fig>

1. wave selectors
2. effect selectors
3. octave selectors
4. piano keyboard

| Key Combination              | Description                                                                                  |
|------------------------------|----------------------------------------------------------------------------------------------|
| <kbd>1</kbd> - <kbd>0</kbd>  | Select wave 1 to 10 (or if pressed with <kbd>Shift</kbd> 11 to 20).                          |
| <kbd>Q</kbd>                 | Clear all effects on note (but leave pitch and sample untouched).                            |
| <kbd>W</kbd>                 | Arpeggio dur (major chord).                                                                  |
| <kbd>E</kbd>                 | Arpeggio moll (minor chord).                                                                 |
| <kbd>R</kbd>                 | Slide up a full note.                                                                        |
| <kbd>T</kbd>                 | Slide up a half note.                                                                        |
| <kbd>Y</kbd>                 | Slide down a half note.                                                                      |
| <kbd>U</kbd>                 | Slide down a full note.                                                                      |
| <kbd>I</kbd>                 | Vibrato small.                                                                               |
| <kbd>O</kbd>                 | Vibrato large.                                                                               |
| <kbd>P</kbd>                 | Tremolo small.                                                                               |
| <kbd>\[</kbd>                | Tremolo large.                                                                               |
| <kbd>\]</kbd>                | Silence the channel "effect".                                                                |
| <kbd>Z</kbd>                 | Switch one octave down.                                                                      |
| <kbd>.</kbd>                 | Switch one octave up.                                                                        |
| <kbd>X</kbd>                 | C note on the current octave.                                                                |
| <kbd>D</kbd>                 | C# note on the current octave.                                                               |
| <kbd>C</kbd>                 | D note on the current octave.                                                                |
| <kbd>F</kbd>                 | D# note on the current octave.                                                               |
| <kbd>V</kbd>                 | E note on the current octave.                                                                |
| <kbd>B</kbd>                 | F note on the current octave.                                                                |
| <kbd>H</kbd>                 | F# note on the current octave.                                                               |
| <kbd>N</kbd>                 | G note on the current octave.                                                                |
| <kbd>J</kbd>                 | G# note on the current octave.                                                               |
| <kbd>M</kbd>                 | A note on the current octave.                                                                |
| <kbd>K</kbd>                 | A# note on the current octave.                                                               |
| <kbd>,</kbd>                 | B note on the current octave.                                                                |

NOTE: Keys on the English keyboard have been used in this table. But it doesn't actually matter what keyboard layout you use,
the only thing that matters is the location of these keys on the English keyboard. For example, if you have AZERTY layout, then
clearing effects will be <kbd>A</kbd> for you, and on a QWERTZ keyboard <kbd>Z</kbd> will add slide down effect, and <kbd>Y</kbd>
will change the octave.

It is important to mention that not all features have a keyboard shortcut. For example you might have 31 wavepatterns, but only
the first 20 have shortcuts. Similarily, there are several magnitudes more effects than what's accessible through shortcuts.

## General MIDI

You can import music (at least the note sheets) from MIDI files. Very simply put, if a classic music note sheet is stored on
a computer in a digitalized form, then it is using the MIDI format to do so. Now these are suitable from a single instrument
to a huge orchestra, so they can store a lot more than what the MEG-4 is capable of, therefore

WARN: Not all MIDI files can be imported properly.

Before we could continue, we must talk about the terms, because unfortunately both the MIDI specification and the MEG-4 uses
the same nomenclature - but for totally different things.

- MIDI song: a single *.mid* file (SMF2 format not supported).
- MIDI track: one row on a classic music notes sheet.
- MIDI channel: only exists because MIDI was created by morons, who thought it is fun to squash everything into a single track
    when storing in files, otherwise exactly the same as a track. You have 16 of these.
- MIDI instrument: a code (standardized by the General MIDI Patch), which describes the instrument a particular channel is using
    (except when that's channel 10, don't even ask), you have 128 of these instrument codes.
- MIDI note: one note in the range C on octave -1 to G on octave 9, 128 different notes in total.
- MIDI concurrent notes: the number of notes that can be played at once at any given time. There are 16 channels and 128 notes
    in MIDI, so this is 2048.
- MEG-4 track: a song, you can have 8 of these.
- MEG-4 sample: a wavepattern stored as a series of PCM samples, analogous to instrument, you have 31 of these.
- MEG-4 channel: the number of concurrent notes that can be played at once at any given time, this is 4.
- MEG-4 note: one note in the range C on octave 0 to B on octave 7, 96 different notes in total.

To avoid confusion, hereafter we'll talk about one MEG-4 track only, and "track" will refer to MIDI channels, and "channel"
will refer to MEG-4 channels.

### Instruments

Concerning instruments, in total there are 16 families with 8 instruments in each. MEG-4 doesn't have 128 wave banks, so best it
can do is, assigning two wavepatterns per family (families 15 and 16 are for sound effects):

| Family     | SF | Patch | How should sound like | SF | Patch | How should sound like |
|------------|---:|-------|-----------------------|---:|-------|-----------------------|
| Piano      | 01 |   1-4 | Acoustic Grand Piano  | 02 |   5-8 | Electric Piano        |
| Chromatic  | 03 |  9-12 | Celesta               | 04 | 13-16 | Tubular Bells         |
| Organ      | 05 | 17-20 | Church Organ          | 06 | 21-24 | Accordion             |
| Guitar     | 07 | 25-28 | Acoustic Guitar       | 08 | 29-32 | Electric Guitar       |
| Bass       | 09 | 33-36 | Acoustic Bass         | 0A | 37-40 | Slap Bass             |
| Strings    | 0B | 41-44 | Violin                | 0C | 45-48 | Orchestral Harp       |
| Ensemble   | 0D | 49-52 | String Ensemble       | 0E | 53-56 | Choir Aahs            |
| Brass      | 0F | 57-60 | Trumpet               | 10 | 61-64 | French Horn           |
| Reed       | 11 | 65-68 | Saxophone             | 12 | 69-72 | Oboe                  |
| Pipe       | 13 | 73-76 | Piccolo               | 14 | 77-80 | Blown Bottle          |
| Synth Lead | 15 | 81-84 | Synth 1               | 16 | 85-88 | Synth 2               |
| Synth Pad  | 17 | 89-92 | Synth 3               | 18 | 93-96 | Synth 3               |

In short, the General MIDI instrument will become the `(patch - 1) / 4 + 1`th wave in the soundfont.

Note that MEG-4 assigns waves dynamically, so these number mean the soundfont's wave number. For example if your MIDI file
uses two instruments only, let's say Grand Piano and Electric Guitar, then piano would be assigned wave 1, and guitar wave 2. You
can load all waves apriori from the soundfont in the [sound effects] editor, and then your imported MIDI file will use exactly
these wave numbers.

### Patterns

The MEG-4 patterns are analogous to classic music note sheets, but while on a classic sheet time goes from left to right and
each MIDI track is tied to an instrument, in MEG-4 the time goes from top to bottom, and you can dynamically assign which
waveform to use on a particular channel. Consider the following example (taken from the General MIDI specification):

<imgc ../img/notes.png><fig>Classic musical note sheet on the left, its MEG-4 pattern equivalent on the right</fig>

On the left we have three tracks, Electric Piano 2, Harp and Bassoon. The first notes to be played are on the bassoon, right two
notes at the same time. Note the bass key on the 3rd track, so these are notes C on octave 3 and C on octave 4, and they both
last 4 quoter-notes, so whole notes.

On the right you can see the MEG-4 pattern equivalent. The first row describes these two notes played on a bassoon: `C-3` on the
first channel, and `C-4` on the second. The sample `12` (hex, provided that you have manually loaded the soundfont apriori,
otherwise the number would be different) selects the oboe waveform, which isn't exactly a bassoon, but that's the closest we have
in the soundfont. The `C30` part means the velocity at which these notes are played, which is analogous to the volume (the harder
you hit a key on the piano, the louder its sound will be). MEG-4 note volumes go from 0 (silence) to 64 (40 in hex, full volume).
So 30 in hex means 75% of the full volume.

The next note to be played is on the harp, starts a quoter-note later than the bassoon, it is a G on octave 4 and lasts 3
quoter-notes long. On the MEG-4 pattern you can see this as `G-4` in the fourth row (because it starts at that time), and since
channels 1 and 2 are still playing the bassoon, it is played on channel 3. If we were to put this on channel 1 or 2, then the
previously played note on that channel would be silenced and replaced by this new one. The sample here is `0C` (hex), which
is Orchestral Harp.

The last note is on the first MIDI track, which starts half a note later from the start, is an E on octave 5, and lasts 2
quoter-notes, or with other words a half-note long. Because it starts half a note from the start, you can see `E-5` in the
8th row, and since we already have 3 notes to be played, so it is assigned to channel 4. The sample `02` selects the waveform
for Electric Piano, which isn't the same as MIDI Electric Piano 2, but pretty close.

Now we have two whole long notes, one half and a quoter long and another half note long; started at the beginning, quoter and half
note later in this order. This means they all must end at the same time. You can see this in the 16th (or 10 in hex) row, all
channels have a `C00`, "set volume to 0" command.

### Tempo

MIDI silently assumes 120 BPM and defines a divisor to quoter-notes. Then it might also define the length of a quoter-note in
milliseconds, or not. The point is, it is very complex, and not all combinations can be translated to MEG-4 properly. I've
written the importer in a way to discard accumulating rounding errors, and only care about the same relative delta times between
two consecutive notes. This way the MEG-4 song's tempo never will be exactly the same as the MIDI song's, but it should sound
similar and should never deviate too much either.

The tempo on the MEG-4 is much simpler. You have a fixed 3000 ticks per minute, and the default tempo is 6 ticks per row. This
means to achive 125 BPM using the default tempo, you should put notes in every 4th rows (because 3000 / 6 / 4 = 125). If you set
the tempo (see effect `Fxx`) to half of that, 3 ticks per row, then each row will last half the time, therefore you'll get 250 BPM
if you were using every 4th row. To get 125 BPM with tempo 3, you would have to use every 8th rows. If you set the tempo to 12,
then each row will last twice the time, therefore every 4th row will get you 62.5 BPM, and you'd have to use every 2nd rows for
125 BPM. Hope this makes sense to you.

This only sets when a note should be started to play, and totally independent of how long that sound lasts. For the latter you
should use a new note on the same channel, or you should use a `C00` "set volume to 0" effect in the row when you want the note
to be cut off. If you change the tempo in between, that won't influence the sound played, just how long it's played (because
the row to turn it off would now reached at a different time).

Now notes will stop playing too if their wavepattern ends. When that happens, depends on the pitch and the number of samples in
the wavepattern (playing at C-4 requires 882 samples of the wave to be sent to the speaker on every ticks). There's a trick here:
you can set a so called "loop" on the wavepattern, which means after all the samples are played once, the selected region of the
wave will be repeated indefinitely (so you'll have to explicitly cut it off, otherwise the sound would really never stop).
