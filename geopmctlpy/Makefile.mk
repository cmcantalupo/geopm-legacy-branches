#  Copyright (c) 2015 - 2021, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

prefix ?= /usr
exec_prefix ?= $(prefix)
bindir ?= $(prefix)/bin
libexecdir ?= $(exec_prefix)/libexec

EXTRA_DIST += geopmctlpy/MANIFEST.in \
              geopmctlpy/requirements.txt \
              geopmctlpy/setup.py \
              geopmctlpy/test/README.md \
              # end

GEOPMCTLPYTEST_TESTS = geopmctlpy/test/pytest_links/TestMonitor.test_sleep \
               # end

TESTS += $(GEOPMCTLPYTEST_TESTS)

geopmctlpytest-checkprogs: $(GEOPMCTLPYTEST_TESTS)

PHONY_TARGETS += geopmctlpytest-checkprogs

$(GEOPMCTLPYTEST_TESTS): geopmctlpy/test/pytest_links/%:
	mkdir -p geopmctlpy/test/pytest_links
	rm -f $@
	ln -s $(abs_builddir)/geopmctlpy/test/geopmctlpy_test.sh $@

clean-local-geopmctlpytest-script-links:
	rm -f geopmctlpy/test/pytest_links/*

clean-local-geopmctlpy: geopmctlpy/setup.py
	cd $(abs_srcdir)/geopmctlpy && $(PYTHON) ./setup.py clean --all

CLEAN_LOCAL_TARGETS += clean-local-geopmctlpytest-script-links \
                       clean-local-geopmctlpy \
                       # end

# Move version.py into source for out of place builds
$(abs_srcdir)/geopmctlpy/geopmpy/version.py: geopmctlpy/geopmpy/version.py
	cp $^ $@

install-geopmctlpy: geopmctlpy/setup.py $(abs_srcdir)/geopmctlpy/geopmpy/version.py
	cd $(abs_srcdir)/geopmctlpy && \
	$(PYTHON) ./setup.py install -O1 --root $(DESTDIR)/ --prefix $(prefix)
