CC=gcc -g


SUBDIRS := vm	


.PHONY build: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ build


clean:
	rm -f **/*.o
	rm -f ./a.out