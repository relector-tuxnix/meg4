Memory Overlays
===============

<imgt ../img/overlay.png> Click on the RAM icon (or press <kbd>F8</kbd>) to modify the memory overlays.

Overlays are very useful, because they allow switching portions of the RAM, so that you can handle more data than what actually
fits in the memory. You can use these to dynamically load sprites, maps, fonts or any arbitrary program data in run-time with
the [memload] function.

They have another very useful feature: if you use [memsave] in your program, then the contents of the overlay will be saved on
the user's computer. Next time you call `memload`, it won't load the overlay's data from your floppy, rather from the user's
computer. With this, you can create permanent storage to store high-scores for example.

<imgc ../img/overlayscr.png><fig>Memory Overlays</fig>

Overlay Selector
----------------

On the top you can see the memory overlays' overview with each overlay's length. The darker entries mean that particular
overlay isn't set. You have 256 overlay slots, from 00 to FF.

Overlay Contents
----------------

Below the table you can see the hexdump of the overlay's contents (only if a non-empty overlay is selected).

Hexdump is a pretty simple and straightforward format: in the first coloumn you can see the address, which is always
dividable by 16. This is followed by the hex representation of 16 bytes at that address, followed by the character representation
of the same 16 bytes. That's all to it.

Overlay Menu
------------

On the menu bar, you can specify a memory address and a size, and press on the <ui1>Save</ui1> button to store data in the
selected overlay. Pressing the <ui1>Load</ui1> button will load the contents of the overlay into the specified memory address,
but this time the size only specifies the upper limit how much bytes to load.

The <ui1>Export</ui1> button will bring up the save file modal, and will allow you to save and modify the binary data with a
third party editor. To import a memory overlay back, all you need to do is naming the file `memXX.bin`, where `XX` is the number
of the overlay you want use, from 00 to FF, and just drag'n'drop that file into the MEG-4's screen.
