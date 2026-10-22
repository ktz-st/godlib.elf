all: imploder rnc godpack apultra pack-ice cutbuild febuild toolbase assetview

imploder:
	$(MAKE) -C imploder

febuild:
	$(MAKE) -C febuild

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
	$(MAKE) -C imploder clean
	$(MAKE) -C febuild clean
	$(MAKE) -C assetview clean
	$(MAKE) -C toolbase clean
	$(MAKE) -C cutbuild clean
	$(MAKE) -C rnc clean
	$(MAKE) -C godpack clean
	$(MAKE) -C apultra clean
	$(MAKE) -C pack-ice clean

.PHONY: all imploder rnc godpack apultra pack-ice cutbuild febuild toolbase assetview clean
