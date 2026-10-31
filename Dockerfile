FROM docker.io/library/gcc:14-bookworm AS catalog-build
WORKDIR /src
COPY ServerSync/ /src/
RUN mkdir /out && g++ -std=c++17 -O2 -shared -fPIC -static-libstdc++ -static-libgcc \
    catalog.cpp -o /out/libreriff-catalog.so

FROM docker.io/library/python:3.13-slim-bookworm

LABEL org.opencontainers.image.source="https://github.com/Aftonstjarna/ReRiff"

ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1 \
    RERIFF_DATA_DIR=/data
WORKDIR /app

COPY packaging/requirements.txt /app/packaging/requirements.txt
COPY docker/requirements.txt /app/docker/requirements.txt
RUN python -m pip install --no-cache-dir -r /app/docker/requirements.txt \
    && groupadd --gid 10001 reriff \
    && useradd --uid 10001 --gid 10001 --no-create-home reriff \
    && mkdir -p /data \
    && chown 10001:10001 /data

COPY vendor/riffapi-local-extended/ /app/server/
COPY --from=catalog-build /out/libreriff-catalog.so /app/server/libreriff-catalog.so
COPY scripts/trace-server.py /app/trace-server.py
COPY docker/server.py docker/healthcheck.py /app/docker/
COPY LICENSE /app/LICENSE
COPY ServerSync/third_party/LICENSE.MIT /app/licenses/nlohmann-json.txt

USER 10001:10001
VOLUME ["/data"]
EXPOSE 5144
HEALTHCHECK --interval=30s --timeout=3s --start-period=10s --retries=3 \
    CMD ["python", "/app/docker/healthcheck.py"]
CMD ["python", "/app/docker/server.py"]
