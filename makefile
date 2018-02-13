!include makefile.public

shareloc = \\TKFilToolBox\tools\20174

install: csub.exe
	copy csub.exe D:\bin

share: csub.exe clean
	del /f /q $(shareloc)\*
	copy csub.* $(shareloc)
	copy makefile.public $(shareloc)\makefile
