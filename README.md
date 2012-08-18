Diablo 3 Language Liberator

URL: http://necrotoolz.sourceforge.net/d3/

****HOW IT WORKS AND HOW DO I MAKE IT WORK****

When you login to Blizzard's server, the client sends locale information in the form of a string which can be one of: enUS (American English), enGB (British English), esMX (Spanish-Mexican), ptBR (Portuguese), koKR (Korean), zhTW (Traditional Chinese), zhCN (Simplified Chinese), jpJP (Japanese), deDE (German), esES (Spanish-Spain), frFR (French), itIT (Italian), plPL (Polish), ruRU (Russian). If the locale is different than expected by the server, you receive an error 81/82/83/84. The error number depends on your location.

This program works by injecting a DLL (Dynamic Link Library) into Diablo 3's executable space and hooking WSASend. When locale information is found in an outgoing packet, the locale is replaced by a "fake" one.


1. Make sure d3injector.exe and apihook.dll are in the same folder
2. Run d3injector.exe
3. Run Diablo 3
4. When the game is about to start, a window should pop up asking you to choose 2 locales.
5. The first locale is the real one, so if you have an English game client, you'll choose enUS.
6. The second locale is the "fake" one, it's the one Blizzard wants you to have, for Brazilian users that would be ptBR.
7. Click OK! and the game will start.
8. Profit!
