Sound Effect
============

In this tutorial we'll prepare and import a sound effect. We'll use Audacity, but other wave editors should work too.

Load Wave
---------

First, let's open the desired wave file in Audacity.

<imgc ../img/tut_snd1.png><fig>Open the sound wave in Audacity</fig>

We can see right away that there are two waves, meaning our sound sample is stereo. MEG-4 can only handle mono, so go to
`Tracks` > `Mix` > `Mix Stereo Down to Mono` to convert it. If you see only one waveform, you can skip this step.

Tuning and Volume
-----------------

Now MEG-4 does tune the samples on its own, and for that to work, all imported waves must be tuned to a specific pitch. For some reason pitch detection is broken in Audacity,
so you'll have to do it manually. Press <kbd>Ctrl</kbd>+<kbd>A</kbd> to select the wave, and then go to `Analyze` > `Plot Spectrum...`.

<imgc ../img/tut_snd2.png><fig>Analyzing the spectrum</fig>

Move your cursor above the biggest peak, and the current pitch will show up below (which is `C3` in this example). If the shown note isn't `C4`, then select `Effect` >
`Pitch and Tempo` > `Change Pitch...`.

<imgc ../img/tut_snd3.png><fig>Changing the pitch</fig>

In the "from" field, enter the value that you saw in the spectrum window, and in the "to" field enter C-4, then press "Apply".

<imgc ../img/tut_snd4.png><fig>Changing the volume</fig>

Next thing is to normalize the volume. Go to `Effect` > `Volume and Compression` > `Amplify...`. In the popup window just press "Apply" (everything is autodetected correctly, no
need to change anything).

Number of Samples
-----------------

MEG-4 supports no more than 16376 samples per waves. If you have fewer samples than this in the first place, then you can skip this step.

Under the waveform you'll see the selection in milliseconds, click on that small "s" and change it to "samples".

<imgc ../img/tut_snd5.png><fig>Changing the unit</fig>

In our example that's more than the allowed maximum. The number of samples is calculated as the value under "Project Sample Rate" multiplied by the length. So to lower the
number of samples, either we lower the rate or we cut off the end of the wave. In this tutorial we'll do both.

Select everything let's say after 1.0, and press <kbd>Del</kbd> to delete. This does the trick, but makes the ending sound harsh. To fix that, select a reasonable portion at
the end and go to `Effect` > `Fading` > `Fade Out`. This will make the wave end nicely.

<imgc ../img/tut_snd6.png><fig>Chopping off and fading out the end</fig>

Our wave is still too long (44380 samples), but we can't cut off more without ruining the sample. This is where the sample rate comes in. In previous versions of Audacity, this
was comfortably displayed at the bottom on the toolbar as "Project Rate (Hz)". But not any more, on newer Audacity it is a lot more complicated. First, click `Audio Setup` on the
toolbar and select `Audio Settings...`. In the popup window, look for "Quality" / "Project Sample Rate", and from the drop-down select "Other..." to make the actual input field
editable.

WARNING: Make sure you calculate the number correctly. Audacity is incapable of undoing this step, so you can't give it another try!

Enter a number here, which is 16376 divided by the length of your wave (1.01 secs in our example) and press "OK".
 
<imgc ../img/tut_snd7.png><fig>Lowering the number of samples</fig>

Select the entire wave (press <kbd>Ctrl</kbd>+<kbd>A</kbd>) then you should see that the end of the selection is below 16376.

Save and Import
---------------

Finally, save the new modified wave by selecting `File` > `Export` > `Export as WAV`. Make sure encoding is "Unsigned 8-bit PCM". As filename, enter `dspXX.wav`, where
`XX` is a hex number between `01` and `1F`, the MEG-4 wave slot where you want to load this sample (using a different filename works too, but then the wave will be loaded
at the first free slot).

<imgc ../img/tut_snd8.png><fig>Imported wave form</fig>

Once you have the file, just drag'n'drop it into the MEG-4 window and you're done.
