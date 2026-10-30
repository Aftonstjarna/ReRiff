FROM python:3.13-slim-bookworm

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
COPY scripts/trace-server.py /app/trace-server.py
COPY docker/server.py docker/healthcheck.py /app/docker/
COPY LICENSE /app/LICENSE

USER 10001:10001
VOLUME ["/data"]
EXPOSE 5144
HEALTHCHECK --interval=30s --timeout=3s --start-period=10s --retries=3 \
    CMD ["python", "/app/docker/healthcheck.py"]
CMD ["python", "/app/docker/server.py"]
