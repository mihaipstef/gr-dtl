FROM node:19

WORKDIR /monitor

RUN apt-get -y update \
    && apt-get -y upgrade \
    && apt-get install -y python3.10 \
    && apt-get install -y pip \
    && apt-get install -y sqlite3 libsqlite3-dev

# Add unstable repo for newer version of gnuradio
RUN echo "deb http://deb.debian.org/debian unstable main contrib non-free" >> /etc/apt/sources.list \
    && apt-get update \
    && apt-get install -y gnuradio=3.10.5.1-2

# Copy collector
RUN mkdir -p collector
COPY collector/ collector/
# Install collector dependencies
RUN pip3 install -r collector/requirements.txt


# Copy ui
RUN mkdir -p ui
COPY ui/ ui/
# Install UI dependencies and build
RUN npm install --prefix ui/ ui/
RUN npm --prefix ui/ run build

# Setup SQLite
RUN mkdir /db

# Copy start script
COPY ./bin/start.sh /bin

CMD /bin/start.sh