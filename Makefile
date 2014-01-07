.PHONY: all
all:
	@cd p2pn/ && make

.PHONY: clean
clean:
	@$(RM) *.pdf *.aux *.log *.bbl *~ *.blg *.dvi *.out
	@cd p2pn/ && make clean

