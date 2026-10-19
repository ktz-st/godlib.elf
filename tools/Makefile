all: rnc godpack apultra pack-ice cutbuild toolbase assetview

toolbase:
	$(MAKE) -C toolbase

assetview:
	$(MAKE) -C assetview

rnc:
	$(MAKE) -C rnc

godpack:
	$(MAKE) -C godpack

apultra:
	$(MAKE) -C apultra

pack-ice:
	$(MAKE) -C pack-ice

cutbuild:
	$(MAKE) -C cutbuild

clean:
	$(MAKE) -C assetview clean
	$(MAKE) -C toolbase clean
	$(MAKE) -C cutbuild clean
	$(MAKE) -C rnc clean
	$(MAKE) -C godpack clean
	$(MAKE) -C apultra clean
	$(MAKE) -C pack-ice clean

.PHONY: all rnc godpack apultra pack-ice cutbuild toolbase assetview clean
