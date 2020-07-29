make: shell.c 
	@echo You must have libreadline-dev to run this script. Run make install to install it.
	@echo Once in the shell, type exit to quit.
	gcc shell.c -lreadline -o shell

clean: 
	rm -f shell

install:
	@echo You must be root to install...
	apt-get install libreadline-dev

