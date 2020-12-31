
    Welcome to PSPZX81

Original Author of XZ81

  Russell Marks (russell.marks@ntlworld.com)

Author of the PSP port version 

  Ludovic.Jacomme also known as Zx-81 (zx81.zx81@gmail.com)


1. INTRODUCTION
   ------------

  XZ81 is one of the best emulator of the Sinclair ZX81 computer
  running on Unix systems. 
  See http://rus.members.beeb.net/ for further informations.

  PSPZX81 is a port on PSP of the Unix version 2.1 of xz81.

  Thanks to Danzel and Jeff Chen for their virtual keyboard,
  and to all PSPSDK developpers.

  Special thanks to Murilo for his feedback and good advices, and
  to Raven for his beautiful icons (http://www.criticalraven.be/PSP/)

  This package is under GPL Copyright, read COPYING file for
  more information about it.


2. INSTALLATION
   ------------

  Unzip the zip file, and copy the content of the directory fw3.x or fw1.5
  (depending of the version of your firmware) on the psp/game, psp/game150,
  or psp/game3xx if you use custom firmware 3.xx-OE/M33.

  Put your program image files on "prg" sub-directory.

  It has been developped on linux for Firmware 3.71-m33 and i hope it works
  also for other firmwares.

  For any comments or questions on this version, please visit 
  http://zx81.zx81.free.fr, http://www.dcemu.co.uk

3. CONTROL
   ------------

3.1 - Virtual keyboard

  In the ZX81 emulator window, there are three different mapping 
  (standard, left trigger, and right Trigger mappings). 
  You can toggle between while playing inside the emulator using 
  the two PSP trigger keys.

    -------------------------------------
    PSP        ZX81             (standard)
  
    Up         Up
    Down       Down
    Left       Left
    Right      Right
    Triangle   NewLine
    Circle     Y
    Cross      N
    Square     Rubout/Delete

    A-Pad      A,B,C,D

    -------------------------------------
    PSP        ZX81   (left trigger)
  
    Up         Up
    Down       Down
    Left       Render
    Right      Render
    Triangle   Load state
    Circle     Analog joystick
    Cross      Save state
    Square     FPS

    -------------------------------------
    PSP        ZX81   (right trigger)
  
    Up         Up
    Down       Down
    Left       Left
    Right      Right
    Triangle   NewLine
    Circle     Y
    Cross      N
    Square     Rubout/Delete

  
    Press Start  + L + R   to exit and return to eloader.
    Press Select           to enter in emulator main menu.
    Press Start            open/close the On-Screen keyboard

  In the main menu

    L+R+Start  Exit the emulator
    R Trigger  Reset the ZX81

    Triangle   Go Up directory
    Cross      Valid
    Circle     Valid
    Square     Go Back to the emulator window

  The On-Screen Keyboard of "Danzel" and "Jeff Chen"

    Use Analog stick to choose one of the 9 squares, and
    use Triangle, Square, Cross and Circle to choose one
    of the 4 letters of the highlighted square.

    Use LTrigger and RTrigger to see other 9 squares 
    figures.

3.2 - IR keyboard

  You can also use IR keyboard. Edit the pspirkeyb.ini file
  to specify your IR keyboard model, and modify eventually
  layout keyboard files in the keymap directory.

  The following mapping is done :

  IR-keyboard   PSP

  Cursor        Digital Pad

  Tab           Start
  Ctrl-W        Start

  Escape        Select
  Ctrl-Q        Select

  Ctrl-E        Triangle
  Ctrl-X        Cross
  Ctrl-S        Square
  Ctrl-F        Circle
  Ctrl-Z        L-trigger
  Ctrl-C        R-trigger

  In the emulator window you can use the IR keyboard to
  enter letters, special characters and digits.


4. LOADING ZX81 PROGRAM FILES
   ------------

  If you want to load program files in your emulator, you have to put your file
  (with .p or .81 file extension) on your PSP memory stick in the 'prg' 
  directory.

  Then, while inside PSPZX81 emulator, just press SELECT to enter in 
  the emulator main menu, and then using the file selector choose one 
  program file to load in your emulator.

  Back to the emulator window, the program should stard automatically.

5. LOADING KEY MAPPING FILES
   ------------

  For given games, the default keyboard mapping between PSP Keys and
  ZX81 keys, is not suitable, and the game can't be played on PSPZX81.

  To overcome the issue, you can write your own mapping file. Using notepad for
  example you can edit a file with the .kbd extension and put it in the kbd 
  directory.

  For the exact syntax of those mapping files, have a look on sample files already
  presents in the kbd directory (default.kbd etc ...).

  After writting such keyboard mapping file, you can load them using the main menu
  inside the emulator.

  If the keyboard filename is the same as the program filename (.zip etc ...)
  then when you load this program, the corresponding keyboard file is automatically 
  loaded !

  You can now use the Keyboard menu and edit, load and save your
  keyboard mapping files inside the emulator. The Save option save the .kbd
  file in the kbd directory using the "Game Name" as filename. The game name
  is displayed on the right corner in the emulator menu.

6. COMPILATION
   ------------

  It has been developped under Linux using gcc with PSPSDK. 
  To rebuild the homebrew run the Makefile in the src archive.
