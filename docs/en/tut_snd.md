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

Now MEG-4 does tune the samples on its own, and for that to work, all imported waves must be tuned to a specific pitch. Press <kbd>Ctrl</kbd>+<kbd>A</kbd>
to select the wave, and then go to `Analyze` > `Plot Spectrum...`.

<imgc ../img/tut_snd2.png><fig>Analyzing the spectrum</fig>

For some reason peak detection is broken in Audacity, so you'll have to do it manually. Move your cursor above the biggest peak, and the current pitch
will show up below (which is C3 in this example). If the shown note isn't C-4, then select `Effect` > `Pitch and Tempo` > `Change Pitch...`.

<imgc ../img/tut_snd3.png><fig>Changing the pitch</fig>

In the "from" field, enter the value that you saw in the spectrum window, and in the "to" field enter C-4, then press "Apply".

<imgc ../img/tut_snd4.png><fig>Changing the volume</fig>

Next thing is to normalize the volume. Go to `Effect` > `Volume and Compression` > `Amplify...`. In the popup window just press "Apply" (everything is autodetected correctly).

Number of Samples
-----------------

MEG-4 supports no more than 16376 samples per waves. Under the waveform you'll see the selection in milliseconds, click on that small "s" and change it to "samples".

<imgc ../img/tut_snd5.png><fig>Changing the unit</fig>

In our example that's more than the allowed maximum. The number of samples is calculated as the value under "Project Rate (Hz)" multiplied by the the length. So to lower the
number of samples, either we lower the rate or we cut off the end of the wave. In this tutorial we'll do both.

Select everything after 1.0, and press <kbd>Del</kbd> to delete. This does the trick, but makes the ending sound harsh. To fix that, select a reasonable portion at the end
and go to `Effect` > `Fading` > `Fade Out`. This will make the wave end nicely.

<imgc ../img/tut_snd6.png><fig>Chopping off and fading out the end</fig>

Our wave is still too long, but we can't cut off more. This is where the "Project Rate (Hz)" comes in. Lower it until the number of samples goes below 16376.

<imgc ../img/tut_snd7.png><fig>Lowering the number of samples</fig>

If you have fewer samples, you can skip this step.

Save and Import
---------------

Finally, save the new modified wave by selecting `File` > `Export` > `Export as WAV`. Make sure encoding is "Unsigned 8-bit PCM". As filename, enter `dspXX.wav`, where
`XX` is a hex number between `01` and `1F`, the MEG-4 wave slot where you want to load this sample (using a different filename works too, but then the wave will be loaded
at the first free slot).

<imgc ../img/tut_snd8.png><fig>Imported wave form</fig>

Once you have the file, just drag'n'drop it into the MEG-4 window and you're done.
