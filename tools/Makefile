all: rnc godpack apultra pack-ice

rnc:
	$(MAKE) -C rnc

godpack:
	$(MAKE) -C godpack

apultra:
	$(MAKE) -C apultra

pack-ice:
	$(MAKE) -C pack-ice

clean:
	$(MAKE) -C rnc clean
	$(MAKE) -C godpack clean
	$(MAKE) -C apultra clean
	$(MAKE) -C pack-ice clean

.PHONY: all rnc godpack apultra pack-ice clean
