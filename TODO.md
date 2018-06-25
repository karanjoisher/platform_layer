~~~~~~~~~ LINUX  ~~~~~~~~~~~~
[x] Memory allocation
[x] Dummy rendering function for testing.
[x] Resizing
[x] Basic input
[x] Timing Functions
[] Sound
https://www.linux.com/news/introduction-linux-sound-systems-and-apis-0
http://equalarea.com/paul/alsa-audio.html
[] Multiple Windows
[] Converge into API
[] Decide how to compress keyboard functionality
	Option 1: Have functions like isWPressed which would then call XQueryKeymap.
	Option 2: On every frame call XQueryKeyMap and expand the bit field to be an array for every keycode(This will be too expensive so mostly no!)
	Option 3: On every frame call XQueryKeyMap and determine the keypresses by bit swizzling.
	Option 4: Switch to event driven mode.(most preferable)
	I think Option 1 and Option 4 will give the least overhead.
	Between Option 1 and Option 4, I think Option 4 will be better as in Option 1 we have to read in a 32 byte array and then swizzle the bits to determine the key presses; whereas in Option 2, we need to only update the flat array 	of keycodes and checking for keypresses is as simple as indexing into that array. 


~~~~~~~~~ WINDOWS ~~~~~~~~~~~~
[] Window creation
[] BackBuffer
[] Memory Allocation
[] Dummy rendering function for testing.
[] Basic input and resizing.
[] Timing Functions
[] Sound
[] Multiple Windows

