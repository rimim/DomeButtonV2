This is a modified version of the Dome Button Controller sketch by Rotopod. The original code can be found here:

http://www.droidvillage.net/wp-content/uploads/2012/11/Dome_Bumps_Nov_16_Teeces1.txt

The modification is to support the V2 version of the board and general cleanup as well as improved handling for the rear PSI and support for the front PSI.

If USE_SERIAL is defined (default) then pins 3,4 are disabled.
If TEECES_PSI is defined (default) then pins 12,13,14 are disabled.

Button sequence is press both buttons for two seconds. Followed by left button presses (1-4). Followed by right button presses (1-4). Followed by pressing both buttons to end the sequence. This way you can activate pins 1-16.