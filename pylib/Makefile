# SPDX-License-Identifier: GPL-2.0-or-later

DESTDIR?=/
PREFIX?=/usr

purge_pycache:
	@find -name '__pycache__' | xargs rm -rf

install: purge_pycache
	python3 setup.py install --prefix=$(PREFIX) --root=$(DESTDIR)

ubuntu_install: purge_pycache
	python3 setup.py install --prefix=$(PREFIX) --root=$(DESTDIR) --no-compile --install-layout=deb
