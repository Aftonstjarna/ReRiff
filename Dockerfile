FROM python:3.11-slim-bookworm AS catalog-build
RUN apt-get update && apt-get install -y --no-install-recommends g++ nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY native/song_catalog.cpp /src/
RUN g++ -std=c++17 -O2 -shared -fPIC -static-libstdc++ -static-libgcc \
    song_catalog.cpp -o /src/libreriff-catalog.so

FROM python:3.11-slim-bookworm
RUN mkdir /app
COPY riffapi-local.py song_catalog.py admin.html /app/
COPY --from=catalog-build /src/libreriff-catalog.so /app/
COPY native/LICENSE.ReRiff /app/licenses/song-catalog.txt
COPY native/LICENSE.nlohmann /app/licenses/nlohmann-json.txt
WORKDIR /app
RUN pip install --no-cache-dir Flask
EXPOSE 5144/tcp
CMD ["python3", "riffapi-local.py"]
