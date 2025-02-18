# docker build -t z210 -f examples/Dockerfile .
# docker run -p 210:9999 z210
FROM debian:bookworm-slim

WORKDIR /app

RUN apt-get update && apt-get install -y \
    make \
    gcc \
    libyaz-dev \
    libexpat1-dev \
    zlib1g-dev \
    pkg-config \
    autoconf \
    automake \
    libtool \
    docbook-xsl \
    xsltproc \
    inkscape

COPY m4 m4/
COPY bfile bfile/
COPY data1 data1/
COPY dfa dfa/
COPY dict dict/
COPY doc doc/
COPY etc etc/
COPY examples examples/
COPY include include/
COPY index index/
COPY isamb isamb/
COPY isamc isamc/
COPY isams isams/
COPY rset rset/
COPY tab tab/
COPY test test/
COPY util util/
COPY win win/
COPY buildconf.sh configure.ac Doxyfile.in IDMETA idzebra-config-2.0.in idzebra.spec Makefile.am zebra.pc.in ./

RUN ./buildconf.sh
RUN ./configure --with-yaz=pkg
RUN make -j4

RUN cd examples/gils && ../../index/zebraidx -d gils update records
RUN cd examples/gils && ../../index/zebraidx -d marc -t grs.marcxml.marc21 update ../marc21/sample-marc

EXPOSE 9999/tcp

ENTRYPOINT [ "/app/index/zebrasrv" ]
CMD [ "-w", "/app/examples/gils", "-c", "zebra.cfg", "-T" ]
