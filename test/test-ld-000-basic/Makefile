QUIET = >/dev/null 2>&1
MSG = $(QUIET) && echo -n 'Test passed: ' || echo -n 'Test failed: '; echo
LD = ../../src/hack-hcc-ld

all: clean $(LD)
	@! $(LD)			$(MSG) 'no input files'
	@$(LD) -h 			$(MSG) 'show help'
	@! $(LD) -o			$(MSG) 'missing argument to -o'
	@! $(LD) non-existent.o		$(MSG) 'can not read'
	@! $(LD) wrong-header.o		$(MSG) 'wrong header'
	@! $(LD) no-bootstrap.o		$(MSG) 'missing bootstrap'
	@! $(LD) no-subroutine-begin.o	$(MSG) 'missing subroutine begin'
	@! $(LD) no-subroutine-end.o	$(MSG) 'missing subroutine end'
	@$(LD) simplest.o		$(MSG) 'simplest...'
	@diff output.hack empty		$(MSG)  '...should produce empty file "output.hack"'
	@$(LD) -o out simplest.o 	$(MSG) 'output file name...'
	@diff out empty			$(MSG) '...should produce empty file "out"'
	@! $(LD) nested.o 		$(MSG) 'nested subroutine'
	@! $(LD) no-subroutine-symbol.o	$(MSG) 'missing subroutine symbol'
	@echo 'All tests passed'

#missing offset
#wrong command
#malformed instruction
#already defined
#instruction outside of subroutine
#relocation outside of subroutine
#unresolved symbol

clean:
	rm -f output.hack out
