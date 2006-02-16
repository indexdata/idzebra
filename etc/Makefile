# $Id: Makefile,v 1.1 2006-02-16 10:43:15 mike Exp $

# This doesn't actually build anything, it just tests whether the
# configuration files here correspond with the schema.  The master
# schema is in Relax-NG Compact (.rnc) format: this cannot be used
# directly as xmllint does not support it; but from this, we generate
# schemas expressed in Relax-NG XML (.rng), XML Schema (.xsd) and XML
# DTD (.dtd), each of which is then tested.

test: test-rng test-xsd test-dtd

test-rng: test-rng-example test-rng-itunes test-rng-minimal
test-rng-%: gfs-%.xml gfs-config.rng
	xmllint --noout --relaxng gfs-config.rng $<

test-xsd: test-xsd-example test-xsd-itunes test-xsd-minimal
test-xsd-%: gfs-%.xml gfs-config.xsd
	xmllint --noout --schema gfs-config.xsd $<

test-dtd:
	@echo "# DTD testing skipped due to lack of namespace support"
#test-dtd: test-dtd-example test-dtd-itunes test-dtd-minimal
test-dtd-%: gfs-%.xml gfs-config.dtd
	xmllint --noout --dtdvalid gfs-config.dtd $<

gfs-config.rng: gfs-config.rnc
	rm -f $@
	trang $? $@
	chmod ugo-w $@

# Trang generates "explain.xsd" as a side-effect
gfs-config.xsd: gfs-config.rnc
	rm -f $@ explain.xsd
	trang $? $@
	chmod ugo-w $@ explain.xsd

gfs-config.dtd: gfs-config.rnc
	rm -f $@
	trang $? $@
	chmod ugo-w $@

clean:
	rm -f gfs-config.rng gfs-config.xsd explain.xsd gfs-config.dtd 

